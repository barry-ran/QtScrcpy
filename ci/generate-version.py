import sys
import os

if __name__ == '__main__':
    p = os.popen('git rev-list --tags --max-count=1 2>NUL')
    commit = p.read().strip()
    p.close()

    if commit:
        p = os.popen('git describe --tags ' + commit + ' 2>NUL')
        tag = p.read().strip()
        p.close()
    else:
        tag = 'v4.2.0'

    if not tag or len(tag) < 2:
        tag = 'v4.2.0'

    # print('get tag:', tag)

    version = str(tag[1:])
    version_file = os.path.abspath(os.path.join(os.path.dirname(__file__), "../QtScrcpy/appversion"))
    file=open(version_file, 'w')
    file.write(version)
    file.close()
    sys.exit(0)