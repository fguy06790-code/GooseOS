# THYFOS — Goose language port

An experimental migration of [THYFOS / LiberNT SuperLight](https://github.com/fguy06790-code/libernt-superlight) to the [Goose programming language](https://github.com/aardappel/goose). The language is pinned to **v2026.3**, runtime `4d3da949e563`.

**This is a partial language port.** The NT kernel, drivers, setup, Win32 adapters, desktop rendering, and most package-manager code are still C/C++. Goose's supplied runtime uses user-mode process APIs; converting the kernel and bootloader requires a kernel runtime and ABI work. See [the migration constraints](docs/goose-feasibility.md).

## Implemented in Goose

- `goose/desktop_layout.goose`: dock, launcher, task buttons, search box and resource/audio widget geometry. The native UI copies generated rectangles into Win32 structures.

- `goose/spectrum.goose`: the desktop's 1024-sample stereo FFT and 32-band audio spectrum. Native audio capture passes samples to the Goose implementation; the former C++ FFT is removed.
- `goose/catalog_retry.goose`: the Store's retry policy. It accepts the original seven transient network errors and excludes certificate, integrity, format and access failures.
- `goose/core.goose`: the library entry point. The pinned Goose compiler generates its C implementation and header during the build. Generated C is an output, not the source implementation or a C++ fallback.

The desktop and Store initialize the runtime on the same UI thread that calls the exports. Original attribution, the upstream code history, and the earlier Wi-Fi/Store/setup fixes are retained. The operating system's product branding remains THYFOS.

## Test the port

Install Python 3, CMake 3.20+, a C/C++ compiler and Ninja. From Bash on Linux x64:

```sh
GOOSE_COMPILER=$(python3 tools/bootstrap-goose.py)
cmake -S tests -B outputs/native-tests -G Ninja -DCMAKE_BUILD_TYPE=Debug -DGOOSE_COMPILER="$GOOSE_COMPILER"
cmake --build outputs/native-tests --parallel 2
ctest --test-dir outputs/native-tests --output-on-failure
```

The bootstrap downloads the official release and verifies its SHA-256 before extracting or executing it. It supports Linux x64 and Windows x64 hosts. The CMake helper rejects a compiler with a different runtime ABI.

The complete Windows suite also needs the pinned LiberNT checkout and MinGW-w64. Cross-compilation with Wine is supported for host regression checks; instructions are in [tests/README.md](tests/README.md). These checks do not establish Windows 11 parity, guest boot, or physical Wi-Fi compatibility. Current results are in [VALIDATION.md](VALIDATION.md).

## Build an experimental ISO

On Linux or WSL, install `build-essential git cmake ninja-build flex bison curl python3 xz-utils`, then run:

```sh
WORK_ROOT=/path/to/writable/work sh build-linux.sh
```

The script fetches pinned LiberNT source, installs the checksum-pinned Goose compiler, stages the Goose library before the native apps, and builds `outputs/THYFOS-amd64.iso`. `GOOSE_COMPILER` can select an existing matching host compiler. The base OS revision stays `346eba3d855d70214105670f859e9dc0bb1c8615`. This language port targets a 64-bit OS.

Regression CI runs on pushes and pull requests. ISO builds are manual through GitHub Actions and require the regression jobs to pass first. No Goose-port ISO has been released or validated yet. Chrome rendering and physical Wi-Fi connection remain unresolved inherited compatibility issues.

## Attribution and licensing

Source licensing is documented in [COPYING](COPYING) and file headers. The base is LiberNT, a ReactOS-derived independent NT implementation; it is not Microsoft Windows. Third-party installers retain their own licenses. The Goose compiler is downloaded separately under its own license and is not committed here. [Upstream product documentation](docs/THYFOS_README.md) describes the inherited native OS.
