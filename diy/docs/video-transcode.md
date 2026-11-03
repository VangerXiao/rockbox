# HiBy R1 视频转码规范

目标：转出来的 `.mpg` 拷进卡上 `/Videos/` 就能在 R1 上满屏播放（800×480，实测流畅、无撕裂）。

## 设备事实

- 固件：Rockbox，target `hibyr1`（screen 480×800 竖屏，逻辑横屏 800×480，LCD 16bpp RGB565）
- 播放器：插件 `mpegplayer` → 视频 libmpeg2 / 音频 libmad
- 因此只能播 **MPEG-1/2 视频 + MP3 音频**，封装 MPEG-PS（`.mpg`）
- `mpegplayer` 不做缩放：编码多大就显示多大 → 满屏 = 800×480

## 参数

| 项目 | 值 |
|---|---|
| 容器 | MPEG-PS（`.mpg`） |
| 视频 | `mpeg1video`，800×480，24 fps，yuv420p，GOP 12，**无 B 帧** |
| 视频码率 | 2.2 Mbps（上限 2.6 Mbps） |
| 音频 | `libmp3lame`，128 kbps，44.1 kHz，立体声 |

## 命令

```bash
ffmpeg -y -i "输入.mp4" \
  -vf "scale=800:480:force_original_aspect_ratio=decrease,pad=800:480:(ow-iw)/2:(oh-ih)/2:black,setsar=1" \
  -c:v mpeg1video -b:v 2200k -maxrate 2600k -bufsize 2M -r 24 -g 12 -bf 0 \
  -c:a libmp3lame -b:a 128k -ar 44100 -ac 2 \
  -f mpeg "输出 (800x480).mpg"
```

## 硬性限制

1. 帧率只能是 23.976 / 24 / 25 / 29.97 / 30 / 50 / 59.94 / 60；12、15 fps 会让 ffmpeg 直接报错。
2. 音频必须是 MP3（libmad）；MP2 / AAC / AC3 都不行。
3. 必须 `-bf 0`。
4. 分辨率 ≤ 800×480。
5. 不要加 rotate/transpose：竖屏旋转由 LCD 驱动完成。
6. 卡上 `设置 → 通用设置 → 文件视图 → 显示文件` 要改成“支持的格式”，否则看不到 `.mpg`。

## 自检

```bash
ffmpeg -hide_banner -i 输出.mpg 2>&1 | grep -E 'Duration|Stream'
ffmpeg -hide_banner -v error -i 输出.mpg -f null -     # 无输出 = 解码无错
```
