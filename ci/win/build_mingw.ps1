[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$QtDirectory,
    [Parameter(Mandatory = $true)]
    [string]$MingwDirectory,
    [string]$CMakePath = '',
    [string]$NinjaPath = '',
    [string]$BuildDirectory = '',
    [string]$PackageDirectory = '',
    [ValidateRange(1, 128)]
    [int]$ParallelJobs = 6,
    [switch]$BuildVisibilityTests
)

$ErrorActionPreference = 'Stop'
$sourceDirectory = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
if ([string]::IsNullOrWhiteSpace($BuildDirectory)) {
    $BuildDirectory = Join-Path $sourceDirectory 'build\qt6-mingw-release'
}
$BuildDirectory = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($BuildDirectory)
$QtDirectory = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($QtDirectory)
$MingwDirectory = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($MingwDirectory)

function Resolve-BuildTool {
    param([string]$RequestedPath, [string]$BundledPath, [string]$CommandName)
    if (-not [string]::IsNullOrWhiteSpace($RequestedPath)) {
        return $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($RequestedPath)
    }
    if (Test-Path -LiteralPath $BundledPath -PathType Leaf) {
        return $BundledPath
    }
    $toolCommand = Get-Command $CommandName -CommandType Application -ErrorAction SilentlyContinue |
        Select-Object -First 1
    if ($toolCommand) {
        return $toolCommand.Source
    }
    throw "Build tool not found: $CommandName. Pass its path explicitly."
}

$qtInstallationRoot = Split-Path -Parent (Split-Path -Parent $QtDirectory)
$CMakePath = Resolve-BuildTool -RequestedPath $CMakePath -BundledPath (Join-Path $qtInstallationRoot 'Tools\CMake_64\bin\cmake.exe') -CommandName 'cmake.exe'
$NinjaPath = Resolve-BuildTool -RequestedPath $NinjaPath -BundledPath (Join-Path $qtInstallationRoot 'Tools\Ninja\ninja.exe') -CommandName 'ninja.exe'

$qtBinDirectory = Join-Path $QtDirectory 'bin'
$mingwBinDirectory = Join-Path $MingwDirectory 'bin'
$deployTool = Join-Path $qtBinDirectory 'windeployqt.exe'
$cCompiler = Join-Path $mingwBinDirectory 'gcc.exe'
$cxxCompiler = Join-Path $mingwBinDirectory 'g++.exe'
$resourceCompiler = Join-Path $mingwBinDirectory 'windres.exe'
foreach ($toolPath in @($CMakePath, $NinjaPath, $cCompiler, $cxxCompiler, $resourceCompiler)) {
    if (-not (Test-Path -LiteralPath $toolPath -PathType Leaf)) {
        throw "Required tool not found: $toolPath"
    }
}
if (-not (Test-Path -LiteralPath (Join-Path $QtDirectory 'lib\cmake\Qt6\Qt6Config.cmake') -PathType Leaf)) {
    throw "QtDirectory must refer to a Qt 6 MinGW kit: $QtDirectory"
}

$makePackage = -not [string]::IsNullOrWhiteSpace($PackageDirectory)
if ($makePackage) {
    $PackageDirectory = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($PackageDirectory)
    if (Test-Path -LiteralPath $PackageDirectory) {
        throw "PackageDirectory already exists; choose a new directory to avoid overwriting files: $PackageDirectory"
    }
    if (-not (Test-Path -LiteralPath $deployTool -PathType Leaf)) {
        throw "Qt deployment tool not found: $deployTool"
    }
}

function Invoke-Checked {
    param([string]$Program, [string[]]$Arguments)
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "$Program exited with code $LASTEXITCODE"
    }
}

$previousPath = $env:PATH
try {
    # Put this kit first, even if Qt Creator or another MinGW is on PATH.
    $env:PATH = "$mingwBinDirectory;$qtBinDirectory;$(Split-Path -Parent $CMakePath);$(Split-Path -Parent $NinjaPath);$previousPath"
    $testOption = if ($BuildVisibilityTests) { 'ON' } else { 'OFF' }
    Invoke-Checked -Program $CMakePath -Arguments @(
        '-S', $sourceDirectory, '-B', $BuildDirectory, '-G', 'Ninja',
        '-DCMAKE_BUILD_TYPE=Release', '-DQT_DESIRED_VERSION=6',
        "-DCMAKE_PREFIX_PATH=$QtDirectory", "-DCMAKE_C_COMPILER=$cCompiler",
        "-DCMAKE_CXX_COMPILER=$cxxCompiler", "-DCMAKE_RC_COMPILER=$resourceCompiler",
        "-DCMAKE_MAKE_PROGRAM=$NinjaPath", "-DQTSCRCPY_BUILD_VISIBILITY_TESTS=$testOption"
    )
    Invoke-Checked -Program $CMakePath -Arguments @('--build', $BuildDirectory, '--parallel', "$ParallelJobs")

    $outputDirectory = Join-Path $sourceDirectory 'output\x64\Release'
    if ($BuildVisibilityTests) {
        $testRunner = Join-Path (Split-Path -Parent $CMakePath) 'ctest.exe'
        Invoke-Checked -Program $testRunner -Arguments @('--test-dir', $BuildDirectory, '--output-on-failure')
    }

    if ($makePackage) {
        New-Item -ItemType Directory -Path $PackageDirectory | Out-Null
        # Copy explicit release files, excluding static libraries and test tools.
        $runtimeFiles = @(
            'QtScrcpy.exe', 'adb.exe', 'AdbWinApi.dll', 'AdbWinUsbApi.dll',
            'avcodec-58.dll', 'avformat-58.dll', 'avutil-56.dll', 'swscale-5.dll',
            'swresample-3.dll', 'scrcpy-server', 'sndcpy.bat', 'sndcpy.apk'
        )
        foreach ($runtimeFile in $runtimeFiles) {
            Copy-Item -LiteralPath (Join-Path $outputDirectory $runtimeFile) -Destination $PackageDirectory
        }
        foreach ($dataDirectory in @('config', 'keymap')) {
            Copy-Item -LiteralPath (Join-Path $sourceDirectory $dataDirectory) -Destination $PackageDirectory -Recurse
        }
        Get-ChildItem -LiteralPath $sourceDirectory -File |
            Where-Object { $_.Name -like 'LICENSE*' -or $_.Name -like 'COPYING*' } |
            Copy-Item -Destination $PackageDirectory
        $usageGuide = Join-Path $sourceDirectory 'docs\WINDOW_VISIBILITY_zh.md'
        if (Test-Path -LiteralPath $usageGuide -PathType Leaf) {
            Copy-Item -LiteralPath $usageGuide -Destination (Join-Path $PackageDirectory '使用说明.md')
        }
        Invoke-Checked -Program $deployTool -Arguments @(
            '--release', '--compiler-runtime', (Join-Path $PackageDirectory 'QtScrcpy.exe')
        )
        foreach ($runtimeDll in @('libgcc_s_seh-1.dll', 'libstdc++-6.dll', 'libwinpthread-1.dll')) {
            Copy-Item -LiteralPath (Join-Path $mingwBinDirectory $runtimeDll) -Destination $PackageDirectory -Force
        }
        $translationDirectory = Join-Path $PackageDirectory 'translations'
        New-Item -ItemType Directory -Path $translationDirectory -Force | Out-Null
        Get-ChildItem -LiteralPath (Join-Path $QtDirectory 'translations') -File |
            Where-Object { $_.Name -like 'qtbase_*.qm' -or $_.Name -like 'qtmultimedia_*.qm' } |
            Copy-Item -Destination $translationDirectory -Force
        Write-Output "Portable release: $(Join-Path $PackageDirectory 'QtScrcpy.exe')"
    }
    Write-Output "Built executable: $(Join-Path $outputDirectory 'QtScrcpy.exe')"
}
finally {
    $env:PATH = $previousPath
}
