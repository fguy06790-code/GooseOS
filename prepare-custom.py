import pathlib
import json
import re
import shutil
import sys

source, config = map(pathlib.Path, sys.argv[1:])
version = (config / 'VERSION').read_text().strip()
if not re.fullmatch(r'[0-9]+\.[0-9]+\.[0-9]+', version):
    raise SystemExit('VERSION must contain a numeric semantic version')
overlays = config / 'overlays'
if overlays.is_dir():
    for item in overlays.rglob('*'):
        if item.is_file():
            target = source / item.relative_to(overlays)
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(item, target)
for item in (source / 'sdk/lib/crt').rglob('*'):
    if item.is_file() and item.suffix in ('.asm', '.inc'):
        original = item.read_bytes()
        normalized = original.replace(b'\r\n', b'\n')
        if normalized != original:
            item.write_bytes(normalized)
modules = source / 'modules/CMakeLists.txt'
content = modules.read_text().replace('add_subdirectory(cairo)', '').replace('add_subdirectory(chrome)', '')
modules.write_text(content)
hive = source / 'boot/bootdata/hivesft.inf'
content = hive.read_text()
old = '"%SystemRoot%\\explorer.exe"'
previous = '"%SystemRoot%\\CairoDesktop\\CairoDesktop.exe"'
new = '"%SystemRoot%\\thyfos.exe"'
if old not in content and previous not in content and new not in content:
    raise SystemExit('Expected Winlogon shell entry was not found')
content = content.replace(old, new).replace(previous, new)
content = content.replace('"AutoAdminLogon",0x00000000,"1"', '"AutoAdminLogon",0x00000000,"0"')
policy = 'HKLM,"SOFTWARE\\Policies\\Microsoft\\Windows\\Personalization","NoLockScreen",0x00010001,0x00000001'
if policy not in content:
    content = content.replace('[AddReg]\n', '[AddReg]\n' + policy + '\n', 1)
hive.write_text(content)
hive = source / 'boot/bootdata/hivesys.inf'
content = hive.read_text()
for service in ('Spooler', 'Themes', 'RemoteRegistry', 'Dbgk'):
    pattern = r'(Services\\' + service + r'","Start",0x00010001,)0x00000002'
    content = re.sub(pattern, r'\g<1>0x00000004', content)
hive.write_text(content)
boot_image = config / 'assets/thyfos-boot.bmp'
shutil.copy2(boot_image if boot_image.is_file() else config / 'custom-bootlogo.bmp',
             source / 'ntoskrnl/inbv/resources/reactos_logo.bmp')
vga_image = config / 'assets/thyfos-boot-vga.bmp'
if vga_image.is_file():
    shutil.copy2(vga_image, source / 'ntoskrnl/inbv/resources/thyfos_boot.bmp')
wallpaper = config / 'assets/thyfos-wallpaper.bmp'
if wallpaper.is_file():
    shutil.copy2(wallpaper, source / 'base/shell/thyfos/thyfos-wallpaper.bmp')
    shell = source / 'base/shell/CMakeLists.txt'
    content = shell.read_text()
    if 'add_subdirectory(thyfos)' not in content:
        shell.write_text(content + '\nadd_subdirectory(thyfos)\n')
    hive = source / 'boot/bootdata/hivedef.inf'
    content = hive.read_text().replace('\\Web\\Wallpaper\\BlueHorizon.png', '\\Web\\Wallpaper\\thyfos-wallpaper.bmp')
    content = content.replace('"ThemeActive",2,"1"', '"ThemeActive",2,"0"')
    hive.write_text(content)
version_header = source / 'sdk/include/reactos/thyfosversion.h'
version_header.write_text('#pragma once\n#define THYFOS_VERSION L"' + version + '"\n')
revision = re.search(r'^SOURCE_COMMIT=([a-f0-9]{40})$', (config / 'build-linux.sh').read_text(), re.M).group(1)
(source / 'base/shell/thyfos/release.json').write_text(json.dumps({
    'product': 'THYFOS', 'version': version, 'architecture': 'amd64',
    'release_tag': 'v' + version, 'base': 'LiberNT', 'base_commit': revision
}, indent=2) + '\n')
print('THYFOS ' + version + ' boot, login, desktop, first-boot setup and native window controls prepared')
