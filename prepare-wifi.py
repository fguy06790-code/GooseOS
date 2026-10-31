import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

source, config = map(Path, sys.argv[1:])
package = json.loads((config / 'assets/intel-wifi.json').read_text())

def digest(path, algorithm):
    value = hashlib.new(algorithm)
    with path.open('rb') as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b''):
            value.update(chunk)
    return value.hexdigest()

supplied = os.environ.get('WIFI_INSTALLER')
if supplied:
    installer = Path(supplied)
else:
    cache = source.parent / 'thyfos-downloads'
    cache.mkdir(parents=True, exist_ok=True)
    installer = cache / package['filename']
    if not installer.is_file() or digest(installer, 'sha256') != package['sha256']:
        temporary = installer.with_suffix('.download')
        subprocess.run(['curl', '-fL', '--retry', '3', '--connect-timeout', '30', '--max-time', '600',
                        package['url'], '-o', str(temporary)], check=True)
        if digest(temporary, 'sha256') != package['sha256']:
            raise SystemExit('Intel Wi-Fi download hash does not match the pinned package')
        temporary.replace(installer)
for algorithm in ('sha256', 'sha512'):
    if digest(installer, algorithm) != package[algorithm]:
        raise SystemExit(f'Intel Wi-Fi {algorithm} verification failed: {installer}')

module = source / 'modules/thyfos-wifi'
module.mkdir(parents=True, exist_ok=True)
shutil.copy2(installer, module / package['filename'])
(module / 'CMakeLists.txt').write_text(
    f'add_cd_file(FILE ${{CMAKE_CURRENT_SOURCE_DIR}}/{package["filename"]} '
    'DESTINATION reactos/DriverSetup FOR all)\n')
modules = source / 'modules/CMakeLists.txt'
text = modules.read_text()
if 'add_subdirectory(thyfos-wifi)' not in text:
    modules.write_text(text + '\nadd_subdirectory(thyfos-wifi)\n')
header = source / 'base/setup/libernt/thyfos-wifi.h'
header.write_text('#pragma once\n'
                  f'#define THYFOS_WIFI_FILENAME L"{package["filename"]}"\n'
                  f'#define THYFOS_WIFI_SHA256 L"{package["sha256"]}"\n')
print('Pinned Intel Wi-Fi installer verified and staged for offline first-boot setup')
