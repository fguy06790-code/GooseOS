# THYFOS / LiberNT SuperLight

Builds a customized amd64 LiberNT installation ISO. LiberNT source is pinned to `346eba3d855d70214105670f859e9dc0bb1c8615`. `overlays` contains the source changes applied to that version. `COPYING` contains the upstream GPL license.

## Included changes

- THYFOS boot image, glass sign-in using the OS wallpaper, and the supplied liquid-metal default wallpaper.
- Native desktop with a glass clock, empty dock and launcher. Running windows appear in the dock. The launcher opens File Explorer and installed Chrome. Settings opens from the desktop context menu or by typing `settings`.
- Red close, yellow minimize and green maximize/restore controls on the left of standard OS window frames. Apps drawing their own frames can override them.
- First-use account setup requires a password. Automatic sign-in is disabled after setup.
- A visible driver wizard scans hardware before applications download. It automatically installs available matching drivers for devices missing a driver, retaining driver trust checks and results. Settings > Devices opens the wizard for rescanning/retry and Device Manager for manual management.
- Chrome downloads from Google's official standalone installer during setup, retaining Authenticode signer verification. Failed application installs can be retried through the launcher or next sign-in. Internet access and compatible networking are required.
- File Explorer uses LiberNT's native `filebrowser.exe` and shell folder implementation.
- Automatic CPU/RAM/GPU and system-playback audio spectrum widgets. CPU uses system-time deltas, RAM uses physical memory counters, and GPU uses D3DKMT engine time when supported. The 32 audio bands use a 1024-point FFT over WASAPI output loopback. No microphone is used and audio is not saved. Unsupported counters/output show unavailable. Click the resource widget to open Task Manager.
- Settings > Personalization toggles each widget and selects the resource sampling interval (1-5 seconds).
- Release-build NDIS declaration fix and Linux assembler line-ending normalization required by this source version.

These changes do not establish compatibility with every PC, Windows app or driver. See `VALIDATION.md` for checks performed and remaining limits.

## Windows / WSL

Install Debian in WSL, then run:

```powershell
.\build-local.ps1 -InstallPrereqs
```

Subsequent builds use `.\build-local.ps1`. The working source and toolchains stay in `/opt/codex/work` in WSL. The ISO and checksum are copied to the repository's `outputs` folder. This wrapper uses the WSL root user to install packages and write the default cache.

## Linux / GitHub Actions

Install `build-essential git cmake ninja-build flex bison curl python3 xz-utils`, then run:

```sh
WORK_ROOT=/path/to/writable/work sh build-linux.sh
```

`BUILD_JOBS` defaults to 6; lower it for limited memory. `SOURCE_DIR`, `TOOLCHAIN_ROOT` and `OUTPUT_DIR` override working directories. Use a fresh source directory for a clean build. RosBE comes from LiberNT's official toolchain bootstrap. Compiler caches are disabled.

The GitHub Actions workflow runs the same build and uploads `THYFOS-amd64.iso` and `SHA256SUMS` as `THYFOS-amd64-ISO`. ISO binaries and downloaded Chrome installers are not committed.

The simple THYFOS logo replaces the setup watermark, setup icon and legacy setup logo resource. Setup and Driver Updater use THYFOS titles in English. Each Driver Updater scan automatically downloads and installs missing or newer matching Wi-Fi drivers, retaining the existing hardware matching, architecture filtering and trust checks. Bluetooth choices are excluded. An existing internet connection is needed; if networking is unavailable, connect Ethernet or supported USB tethering and scan again. A VM using a virtual wired adapter does not need the host laptop's Wi-Fi driver in the guest.

Compatible standard application windows default to 85% opacity (15% transparent). Settings > Personalization > Window opacity adjusts this from 30% to 100%. The shell leaves app-owned layered windows and windows using private/class drawing DCs unchanged. Desktop wallpaper remains solid.

Modern setup and motion:
- Setup uses a larger English layout, Selawik typography, a light blue sidebar, a visible step counter, rounded navigation buttons, and sliding page changes. Account, disk and component logic is preserved.
- The UEFI boot artwork includes an indeterminate moving blue loader. It indicates activity, not a percentage. The existing BIOS palette fade remains.
- The launcher eases in and out over 180 ms. Standard window animations and menu, tooltip, combo-box and smooth-scrolling preferences are enabled for new profiles that have not disabled animations.
- Settings > Personalization > Animations toggles desktop/setup motion and standard UI animation preferences. The early boot loader is independent of per-user motion preferences.
- Third-party apps can implement their own rendering and animation behavior.

Liquid-glass setup uses the OS content-backdrop material at 217/255 alpha (about 85% opacity), with rounded-corner preference and a white specular outline. The desktop glass cards and sign-in cards use the same fill alpha. The shell exempts material-backed windows from additional whole-window fading to avoid double blending. Compatible ordinary application windows default to 85% opacity. Existing app-owned transparency remains respected.

THYFOS Settings and desktop polish:
- Settings is a visible launcher entry and opens with Win+I. The launcher supports arrow-key selection, Enter activation and hover feedback; the dock remains unpinned.
- Settings opens to a THYFOS home page linking appearance/widgets, wallpaper, networking, sound, display, drivers, accounts and system information, with a Task Manager action.
- The appearance control now reads Window transparency, defaulting to 15%, instead of requiring users to convert opacity percentages.
- Changes broadcast to the desktop, setup and Settings glass materials. The Restore desktop defaults button restores 15% transparency, both widgets and a one-second resource refresh; it preserves the user's animation preference.
- Existing native Settings categories, search and navigation are retained. No claim is made that every hardware-dependent setting works on every device.

Rounded corners and Settings launch:
- Standard captioned windows and owned dialogs receive a 14-pixel compositor radius unless the app supplies its own radius, requests square corners or owns a custom window region. Maximized/minimized windows retain native edge behavior.
- System popup menus and combo dropdowns use the existing compositor's small rounded radius. Setup edits, combos and pushbuttons receive rounded clipping; the launcher and desktop cards already use rounded shapes.
- Personalization exposes a Rounded window corners toggle. Restore desktop defaults enables it.
- Settings launch resolves the actual installed path under %SystemRoot%\ImmersiveControlPanel, fixing the previous missing-app error from shell shortcuts and aliases.
- The launcher temporarily raises the desktop while open, so it can receive focus over an existing app; it lowers when dismissed or deactivated.

Bundled Intel Wi-Fi package:
- The amd64 ISO includes the exact Intel WiFi-24.80.0-Driver64-Win10-Win11.exe supplied by the user, under Windows\DriverSetup. The unchanged package keeps its Intel signature and notices.
- During first-boot setup, before online driver scans and Chrome downloads, setup checks for an Intel PCI wireless controller. If one is present, it verifies the pinned installer hash and attempts silent installation once, with Intel's documented -q -s switches and a log in Windows\DriverSetup\IntelWiFi-24.80.log.
- The attempt, installer result and reboot-required status are recorded under HKLM\SOFTWARE\THYFOS\Setup\IntelWiFi24.80. An installation attempt is marked before launching, so a restart or failure does not create an automatic reinstall loop. Installer failures are reported, and the package remains available for manual retry. Installation success does not establish that a driver has bound to hardware.
- VirtualBox's Intel PRO/1000 is a wired adapter. It is excluded; the VM uses its virtual Ethernet driver and Windows continues managing the host's physical Wi-Fi.
- Builders download the pinned package directly from Intel and verify SHA-256 and SHA-512. The EXE is not committed to GitHub. On Linux, set WIFI_INSTALLER to an existing copy to build without downloading it. On Windows use .\build-local.ps1 -WifiInstaller 'C:\path\WiFi-24.80.0-Driver64-Win10-Win11.exe'.
- This Intel package targets Windows 10/11. Actual installation and hardware operation on LiberNT/THYFOS are unverified.
- Package details: https://www.intel.com/content/www/us/en/download/19351/intel-wireless-wi-fi-drivers-for-windows-10-and-windows-11.html
- Silent installation reference: https://www.intel.com/content/www/us/en/support/articles/000055825/wireless.html

## THYFOS 1.0.0

Initial release: see RELEASE_NOTES.md for downloads, features and known issues. VERSION identifies the THYFOS product release; it does not change the base NT compatibility version. The ISO includes release.json and displays the version in setup and Settings.

## First-boot network setup

First boot now runs language selection, network preparation, account creation, and app installation in that order. The network step automatically attempts the exact bundled Intel Wi-Fi 24.80.0 installer on matching Intel wireless hardware before offering Wi-Fi selection. On an available wireless interface, it opens Settings > Wi-Fi: choose a network, select Connect and enter its password. Return to setup and choose Next. The Choose Wi-Fi and Check again buttons allow reopening Settings or detecting an adapter that has become available. Setup does not collect or log your network password; the existing WLAN connection UI manages the profile.

Driver failures remain visible, and a requested restart is explained. Setup allows offline continuation, while preserving the once-only installer-attempt policy. Components-only repair retains the driver-before-download sequence. Other wireless vendors need their own matching drivers. This is an installer attempt, not a guarantee that the Windows driver will bind or work on THYFOS.

VirtualBox normally exposes virtual Ethernet rather than the host's Wi-Fi interface. Connect Windows to Wi-Fi and leave the VM's NAT adapter enabled; the guest then uses virtual Ethernet. Selecting a wireless SSID inside the guest requires a supported Wi-Fi adapter exposed to the guest, such as suitable USB passthrough hardware. The Intel installer is never run on the Windows host by these build or test steps.

## THYFOS Store and package manager

The connectivity build starts catalog loading immediately, retries temporary network errors up to three times, and offers Retry / F5 after an offline failure. A successful refresh clears the old error panel. Source and installer verification remain enabled; certificate, format and access errors are not automatically retried.

First-boot setup explicitly starts WLAN AutoConfig before Wi-Fi discovery. The Wi-Fi page refreshes once after its initial asynchronous scan. An experimental offline ASUS USB-AC53 Nano package can be included with `ASUS_WIFI_DRIVER_DIR` on Linux or `-AsusWifiDriverDirectory` in build-local.ps1. It must match assets/asus-usb-wifi.json. Export the matching installed package with Windows PnP Utility; this build used the local oem4.inf package, version 1030.52.1216.2025. Driver binaries are not committed. Every supplied file is hash-checked at build time and before installation; setup targets only USB\VID_0B05&PID_184C and records the one-time attempt and restart status. Check again repeats hardware discovery and service preparation without repeatedly installing an already-attempted package.

Physical Wi-Fi is **not confirmed working**: a real ASUS adapter passed through to a test guest exposed a Wi-Fi interface, but returned zero networks. Windows saw ten networks through that same adapter. A signed ASUS Native 802.11 driver variant was also tested without improving scanning, and was not selected for the build. This is a remaining driver/OS compatibility gap; installing a package does not establish wireless functionality. Keep NAT Ethernet available for internet and Store access. The user's active Intel host connection was preserved.

The next build adds THYFOS Store to the launcher, desktop context menu, Settings Home and Settings > Apps. Type `store` or `thyfos store` in the launcher, or run `thyfos-store.exe`. The graphical app shares the existing `lntget` backend: search by name or category, inspect publisher/version/installer details, download, install, refresh the catalog, and open installed-app management. It checks the community source signature and block map and verifies installer SHA-256 before execution.

THYFOS Store is an independent frontend for the WinGet community repository. It does not provide the official Microsoft Store, Microsoft accounts, purchases, DRM, or Microsoft Store-only packages. Catalog listings do not guarantee THYFOS compatibility. Installers retain their own licenses and may need elevation or a restart. A successful installer exit does not prove the application works.

From a command prompt:

```bat
lntget search --id 7zip.7zip -e
lntget show --id 7zip.7zip -e
lntget download --id 7zip.7zip -e --architecture x64
lntget install --id 7zip.7zip -e --interactive
lntget source update
```

Open Settings > Apps > Installed apps to uninstall software. This feature is newer than the published v1.0.0 ISO; rebuilding `main` includes it.


### Terminal and ricing tools

Open **Terminal** from the desktop launcher or type `terminal` in its search box. Use `lntget rice` to list the curated downloads.

- `lntget rice fastfetch` downloads the Fastfetch ZIP. Extract it and run `fastfetch.exe`. The `neofetch` name is an alias for Fastfetch.
- `lntget rice cava` downloads the CAVA x64 MSI. Run the installer from the printed path. Live visualization requires working guest audio.
- `lntget rice cmatrix` downloads the CMatrix executable. Run it from the printed path and press Ctrl+C to stop.
- `lntget search <name>` searches the broader community catalog. `lntget help` lists the download and installation commands.

The curated commands download hash-verified Windows builds to a per-package folder and print the full path. Internet access is required.
