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
