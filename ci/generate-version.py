import os
import re
import subprocess
import sys


def run(cmd):
    try:
        return subprocess.check_output(cmd, stderr=subprocess.DEVNULL).decode().strip()
    except Exception:
        return ""


if __name__ == '__main__':
    commit = run(['git', 'rev-list', '--tags', '--max-count=1'])
    tag = run(['git', 'describe', '--tags', commit]) if commit else ""

    # Accept tags like v1.2.3 / 1.2.3 (optionally followed by a suffix).
    match = re.match(r'^[vV]?(\d+\.\d+\.\d+)', tag)
    if not match:
        # No usable tag (fresh repo / fork without tags): keep the version that
        # is already in the appversion file instead of writing an empty one,
        # otherwise CMake/RC fails with "RC2127: version WORDs separated by commas".
        print('generate-version: no usable git tag found (%r), keeping current appversion' % tag)
        sys.exit(0)

    version = match.group(1)
    version_file = os.path.abspath(os.path.join(os.path.dirname(__file__), "../QtScrcpy/appversion"))
    with open(version_file, 'w') as f:
        f.write(version)
    print('generate-version: appversion = ' + version)
    sys.exit(0)
