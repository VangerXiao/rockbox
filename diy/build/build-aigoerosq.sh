#!/bin/bash
# 编译 AIGO EROS Q (aigoerosq, hosted Linux 32bpp) 的 Rockbox
# 与 R1 的唯一区别：EROS Q 的 lcd_blit_yuv() 不推屏，必须打开 MPEGPLAYER_BLIT_UPDATE
set -euo pipefail
SRC="${1:-$(cd "$(dirname "$0")/../.." && pwd)}"
TOOLCHAIN="${TOOLCHAIN:-/toolchain-r1}"
export PATH="$TOOLCHAIN/bin:$PATH"
command -v mipsel-rockbox-linux-gnu-gcc >/dev/null || {
    echo "找不到 mipsel-rockbox-linux-gnu-gcc，先跑 tools/rockboxdev.sh --target=y 或设置 TOOLCHAIN"; exit 1; }

cd "$SRC"
# 打开显式刷屏宏（幂等）
VOUT="apps/plugins/mpegplayer/video_out_rockbox.c"
grep -q '^#define MPEGPLAYER_BLIT_UPDATE 1' "$VOUT" || \
    sed -i '1i #define MPEGPLAYER_BLIT_UPDATE 1' "$VOUT"

mkdir -p build-erosq && cd build-erosq
[ -f Makefile ] || ../tools/configure --target=aigoerosq --type=normal
make -j"${JOBS:-10}"

echo "--- 产物 ---"
find . -name 'mpegplayer.rock' -o -name 'pictureflow.rock' | xargs -r ls -l
