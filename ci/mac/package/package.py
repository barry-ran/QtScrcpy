import dmgbuild
import os
import json
import sys

current_file_path = os.path.dirname(os.path.realpath(__file__))
dmg_settings_path = '%s/dmg-settings.json' % current_file_path
dmg_background_img = '%s/dmg-background.jpg' % current_file_path
app_path = '%s/../../build/QtScrcpy.app' % current_file_path
dmg_path = '%s/../../build/QtScrcpy.dmg' % current_file_path
app_name = 'QtScrcpy'

def console_print(msg):
    print(msg)
    sys.stdout.flush()

def generate_dmg_info():
    with open(dmg_settings_path, 'w') as file:
        info = {
            'title': app_name,
            'icon-size': 120,
            'background': dmg_background_img,
            'format': 'UDZO',
            'compression-level': 9,
            'window': {
                'position': {'x': 400, 'y': 200},
                'size': {'width': 780, 'height': 480}
                    },
            'contents': [
                 {
                 'x': 223,
                 'y': 227,
                 'type': 'file',
                 'path': app_path
                 },
                 {
                 'x': 550,
                 'y': 227,
                 'type': 'link',
                 'path': '/Applications'
                 }
                 ]
                }
        json.dump(info, file)

if __name__ == '__main__':
    console_print('generate dmg info')
    generate_dmg_info()
    console_print('build dmg: %s' % dmg_path)
    dmgbuild.build_dmg(dmg_path, app_name, dmg_settings_path)
    if not os.path.exists(dmg_path):
        console_print('fail to create %s' % dmg_path)
        sys.exit(1)
    
    sys.exit(0)