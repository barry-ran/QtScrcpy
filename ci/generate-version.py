import sys
import os

if __name__ == '__main__':
    p = os.popen('git rev-list --tags --max-count=1')
    commit = p.read()
    p.close()

    p = os.popen('git describe --tags ' + commit)
    tag = p.read()
    p.close()

    # print('get tag:', tag)

    version = str(tag[1:])
    version_file = os.path.abspath(os.path.join(os.path.dirname(__file__), "../QtScrcpy/version"))
    file=open(version_file, 'w')
    file.write(version)
    file.close()
    sys.exit(0)