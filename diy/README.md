# DIY 分支说明（HiBy R1 / AIGO EROS Q Rockbox 修改）

这个分支（`diy/r1-erosq`）是 **基于上游 Rockbox `master` 的本地修改**，不向上游提 PR。
以后上游更新时，拉取并合并即可继续编译：

```bash
git fetch origin
git merge origin/master      # 或者 git rebase origin/master
```

## 分支里有什么（4 个提交）

| 提交 | 内容 | 影响设备 |
|---|---|---|
| `mpegplayer: take the libmpeg2 video pool from the C heap` | 视频解码池改用 `malloc`（4 MiB，失败降 2/1 MiB，64 字节对齐，退出时 free） | R1 / EROS Q（hosted 端口） |
| `mpegplayer: optional explicit LCD update after the YUV blit` | `MPEGPLAYER_BLIT_UPDATE` 宏控制的显式 `lcd_update_rect()` | 只给 EROS Q 开；**R1 必须关**，否则频闪+斜向撕裂 |
| `pictureflow: R1 volume-key scrolling, 32bpp colour fix, validate cached slides` | ① R1 音量键翻封面（触摸 3×3 只在中间/右下有效）② `scale_subpixel_lcd()` 改成 `LCD_DEPTH < 24` 才量化（修 aigoerosq 32bpp 偏绿）③ `read_pfraw()` 校验文件尺寸，`CACHE_VERSION` 4→5 | R1（①）、EROS Q（②③） |
| `hosted/sim: local development patches` | 模拟器自动播放钩子、SDL 输入/窗口调整、skinned list、configure 选工具链 | 仅本地开发 |

## 编译

两个设备都是 **hosted Linux** 目标，使用同一套工具链 `mipsel-rockbox-linux-gnu`
（`tools/rockboxdev.sh --target=y` 生成，本机缓存在 Docker 镜像 `rb-r1-final:/toolchain-r1`）。

```bash
# R1
export PATH=/toolchain-r1/bin:$PATH
mkdir build-r1 && cd build-r1
../tools/configure --target=hibyr1 --type=normal
make -j10
# 产物：apps/plugins/mpegplayer/mpegplayer.rock 等

# EROS Q（注意：额外定义 MPEGPLAYER_BLIT_UPDATE）
export PATH=/toolchain-r1/bin:$PATH
mkdir build-erosq && cd build-erosq
../tools/configure --target=aigoerosq --type=normal
make -j10 CFLAGS_EXTRA=-DMPEGPLAYER_BLIT_UPDATE   # 或直接改 video_out 里的宏默认值
```

> 注意：`aigoerosq` 是 hosted Linux 32bpp（XRGB8888），`erosqnative` 才是裸机 16bpp 的变体，
> 两者都用 `rockbox.erosq` 作文件名，别搞混。卡上 `rockbox-info.txt` 的 `Target:` 字段可以确认。

## 部署（把 .rock 拷到卡上）

| 设备 | 插件路径 |
|---|---|
| R1 | `.rockbox/rocks/viewers/mpegplayer.rock`、`.rockbox/rocks/demos/pictureflow.rock` |
| EROS Q | `.rockbox/rocks/viewers/mpegplayer.rock`、`.rockbox/rocks/demos/pictureflow.rock` |

覆盖前先备份成 `.bak`。

## 已知坑（血泪）

1. **R1 不要在 `video_out_rockbox.c` 里加显式 `lcd_update_rect()`**：`lcd-linuxfb.c` 在 YUV blit 内部已经上屏，重复整屏拷贝会造成高频频闪 + 黑色三角形（斜向撕裂）。EROS Q 相反，不加就只显示第一帧。
2. `mpegplayer` 不做缩放，视频编码分辨率 = 屏幕显示尺寸。R1 满屏 = 800×480。
3. `mpegplayer` 音频解码是 libmad，只吃 **MP3**（MP2/AAC/AC3 都不行）。
4. MPEG-1 帧率只能是标准值（23.976/24/25/29.97/30/50/59.94/60）。
5. R1 上 `config.cfg` 的“显示文件”默认是“音乐”，必须改成“支持的格式”才能看到 `.mpg`。
