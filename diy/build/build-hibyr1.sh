#!/bin/bash
# 编译 HiBy R1 (hibyr1) 的 Rockbox 插件/固件
# 用法: ./build-hibyr1.sh [源码根目录]   （默认当前目录的上一级）
set -euo pipefail
SRC="${1:-$(cd "$(dirname "$0")/../.." && pwd)}"
TOOLCHAIN="${TOOLCHAIN:-/toolchain-r1}"
export PATH="$TOOLCHAIN/bin:$PATH"
command -v mipsel-rockbox-linux-gnu-gcc >/dev/null || {
    echo "找不到 mipsel-rockbox-linux-gnu-gcc，先跑 tools/rockboxdev.sh --target=y 或设置 TOOLCHAIN"; exit 1; }

cd "$SRC"
mkdir -p build-hibyr1 && cd build-hibyr1
[ -f Makefile ] || ../tools/configure --target=hibyr1 --type=normal
make -j"${JOBS:-10}"

echo "--- 产物 ---"
find . -name 'mpegplayer.rock' -o -name 'pictureflow.rock' | xargs -r ls -l
