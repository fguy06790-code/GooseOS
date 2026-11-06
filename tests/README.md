# Regression tests

These tests exercise the changed production functions with controlled failures. They do not download packages, run installers, modify host drivers, or configure Wi-Fi. The desktop tests substitute resource/audio sampling and icon rendering; the Store test substitutes worker creation; setup and WLAN tests mock their external operations.

## Goose compiler

All suites compile the actual Goose core and require the pinned v2026.3 host compiler. Install it with `python3 tools/bootstrap-goose.py` (or `python` on Windows), then pass its printed path as `-DGOOSE_COMPILER=/path/to/goose` to CMake, or set the `GOOSE_COMPILER` environment variable. Bootstrap verifies the release SHA-256 before extraction/execution.

## Windows

Install CMake 3.20+, Ninja, and MinGW-w64 GCC, then point CMake at an unmodified checkout of LiberNT commit `346eba3d855d70214105670f859e9dc0bb1c8615` (the same revision as `build-linux.sh`). No ISO build or driver-package download is needed.

```sh
cmake -S tests -B outputs/tests -G Ninja -DCMAKE_BUILD_TYPE=Release -DLIBERNT_SOURCE_DIR=C:/path/to/LiberNT
cmake --build outputs/tests --parallel 2
ctest --test-dir outputs/tests --output-on-failure
```

The suite covers:

- Actual Goose retry policy, spectrum code and generated-header C ABI from a native host.
- Catalog retry exclusions for trust/format failures.
- Wireless hardware-ID matching, WLAN service transitions and error propagation.
- Driver-before-network setup ordering and offline continuation.
- Variable-length Wi-Fi scan results, malformed lengths and allocation cleanup.
- Launcher navigation, rounded-window policy and window-opacity ownership.
- Store recovery, loading/install exclusion, selection/double-click handling and restart reporting.
- Bounded setup downloads, failed-job cleanup, read-error hashing and isolated INF entries.

The native Windows CI job uses MinGW and the pinned base sources. It does not provide a Windows 11 reference baseline or boot THYFOS.

## Linux

The Goose language tests, native Goose ABI host and portable miniport simulation run without Windows headers or a LiberNT checkout (set `GOOSE_COMPILER` first):

```sh
cmake -S tests -B outputs/native-tests -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-Wall -Wextra -Werror -fsanitize=address,undefined"
cmake --build outputs/native-tests --parallel 2
ctest --test-dir outputs/native-tests --output-on-failure
```

To run the complete suite on Linux, cross-compile with MinGW and pass `-DCMAKE_SYSTEM_NAME=Windows`, the cross C/C++ compilers, and `-DCMAKE_CROSSCOMPILING_EMULATOR=/path/to/wine-wrapper` when configuring a separate build directory. The Wine wrapper must provide a display for hidden Win32 controls. Tests are serial to support a shared Wine prefix. Wine results establish behavior in Wine, not in Windows 11 or THYFOS.
