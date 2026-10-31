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
