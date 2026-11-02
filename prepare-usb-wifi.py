import hashlib
import json
import os
from pathlib import Path
import shutil
import sys

source, config = map(Path, sys.argv[1:])
package = json.loads((config / 'assets/asus-usb-wifi.json').read_text(encoding='utf-8-sig'))
supplied = os.environ.get('ASUS_WIFI_DRIVER_DIR')
module = source / 'modules/thyfos-usb-wifi'
module.mkdir(parents=True, exist_ok=True)
files = []
if supplied:
    for entry in package['files']:
        path = Path(supplied) / entry['name']
        if hashlib.sha256(path.read_bytes()).hexdigest() != entry['sha256']:
            raise SystemExit('ASUS USB Wi-Fi file does not match the pinned driver package: ' + entry['name'])
        shutil.copy2(path, module / entry['name'])
        files.append(entry)
lines = [f'add_cd_file(FILE ${{CMAKE_CURRENT_SOURCE_DIR}}/{entry["name"]} DESTINATION reactos/DriverSetup/ASUS-WiFi FOR all)' for entry in files]
(module / 'CMakeLists.txt').write_text('\n'.join(lines) + '\n')
modules = source / 'modules/CMakeLists.txt'
text = modules.read_text()
if 'add_subdirectory(thyfos-usb-wifi)' not in text:
    modules.write_text(text + '\nadd_subdirectory(thyfos-usb-wifi)\n')
header = ['#pragma once', 'typedef struct { PCWSTR Name; PCWSTR Hash; } THYFOS_USB_WIFI_FILE;',
          f'#define THYFOS_USB_WIFI_FILE_COUNT {len(files)}',
          f'#define THYFOS_USB_WIFI_INF L"{package["inf"]}"',
          f'#define THYFOS_USB_WIFI_HARDWARE_ID L"USB\\\\VID_0B05&PID_184C"',
          'static const THYFOS_USB_WIFI_FILE ThyfosUsbWifiFiles[] = {']
header += [f'    {{L"{entry["name"]}", L"{entry["sha256"]}"}},' for entry in files]
header += ['    {NULL, NULL}', '};']
(source / 'base/setup/libernt/thyfos-usbwifi.h').write_text('\n'.join(header) + '\n')
print('Pinned ASUS USB Wi-Fi package staged' if files else 'ASUS USB Wi-Fi package omitted; set ASUS_WIFI_DRIVER_DIR to the pinned local export to include it')
