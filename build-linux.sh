#!/bin/sh
set -eu
CONFIG_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
WORK_ROOT=${WORK_ROOT:-/opt/codex/work}
SOURCE_DIR=${SOURCE_DIR:-$WORK_ROOT/LiberNT}
TOOLCHAIN_ROOT=${TOOLCHAIN_ROOT:-$WORK_ROOT/rosbe}
OUTPUT_DIR=${OUTPUT_DIR:-$CONFIG_DIR/outputs}
SOURCE_COMMIT=346eba3d855d70214105670f859e9dc0bb1c8615
for tool in git cmake ninja flex bison curl python3 tar gcc g++; do
    command -v "$tool" >/dev/null || { echo "Missing build prerequisite: $tool" >&2; exit 1; }
done
mkdir -p "$WORK_ROOT" "$OUTPUT_DIR"
if [ ! -f "$SOURCE_DIR/CMakeLists.txt" ]; then
    git clone --depth 1 https://github.com/LiberNT/LiberNT.git "$SOURCE_DIR"
    git -C "$SOURCE_DIR" fetch --depth 1 origin "$SOURCE_COMMIT"
    git -C "$SOURCE_DIR" checkout --detach "$SOURCE_COMMIT"
fi
if [ -d "$SOURCE_DIR/.git" ]; then
    ACTUAL_COMMIT=$(git -C "$SOURCE_DIR" rev-parse HEAD)
else
    ACTUAL_COMMIT=$(cat "$SOURCE_DIR/.thyfos-source-commit" 2>/dev/null || true)
fi
if [ "$ACTUAL_COMMIT" != "$SOURCE_COMMIT" ]; then
    echo "Source revision does not match $SOURCE_COMMIT. Use a fresh SOURCE_DIR." >&2
    exit 1
fi
if [ ! -f "$TOOLCHAIN_ROOT/rosbe-env.sh" ]; then
    curl -fL https://raw.githubusercontent.com/LiberNT/libernt-build-toolchain/main/rosbe-unix-bootstrap.sh -o "$WORK_ROOT/rosbe-bootstrap.sh"
    INSTALL_ROOT="$TOOLCHAIN_ROOT" BIN_DIR="$WORK_ROOT/bin" sh "$WORK_ROOT/rosbe-bootstrap.sh"
fi
python3 "$CONFIG_DIR/prepare-custom.py" "$SOURCE_DIR" "$CONFIG_DIR"
python3 "$CONFIG_DIR/prepare-wifi.py" "$SOURCE_DIR" "$CONFIG_DIR"
python3 "$CONFIG_DIR/prepare-usb-wifi.py" "$SOURCE_DIR" "$CONFIG_DIR"
export PATH="$TOOLCHAIN_ROOT/mingw-gcc/x86_64-w64-mingw32/bin:$TOOLCHAIN_ROOT/mingw-gcc/i686-w64-mingw32/bin:$PATH"
export CCACHE_DISABLE=1
export REACTOS_NESTED_BUILD_MIRROR=1
cmake -S "$SOURCE_DIR" -B "$SOURCE_DIR/build" -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE=toolchain-gcc.cmake -DARCH=amd64 \
    -DCMAKE_BUILD_TYPE=MinSizeRel -DOPTIMIZE=1 -DENABLE_CCACHE=OFF \
    -DPREINSTALL_IMAGE_SIZE_MB=8192
cmake --build "$SOURCE_DIR/build" --target bootcd --parallel "${BUILD_JOBS:-6}"
cp "$SOURCE_DIR/build/bootcd.iso" "$OUTPUT_DIR/THYFOS-amd64.iso"
(cd "$OUTPUT_DIR" && sha256sum THYFOS-amd64.iso > SHA256SUMS)
