import sys
import os

if __name__ == '__main__':
    commit = os.popen('git rev-list --tags --max-count=1').read().strip()
    
    if commit:
        tag = os.popen('git describe --tags ' + commit).read().strip()
        version = tag[1:] if tag else "0.0.0"
    else:
        version = "0.0.0"

    # Write version to file
    version_file = os.path.abspath(os.path.join(os.path.dirname(__file__), "../QtScrcpy/appversion"))
    with open(version_file, 'w') as file:
        file.write(version)
    
    print(f"Version written: {version}")
    sys.exit(0)