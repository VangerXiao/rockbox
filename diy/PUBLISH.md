# 推送到自己的仓库（公开固件 / 私有主题）

## 0. 现状

| 仓库 | 路径 | 分支 | 说明 |
|---|---|---|---|
| 固件（公开） | `D:\rockbox\_rbsrc` | `diy/r1-erosq` | 上游 Rockbox 的浅克隆，`origin` = `https://github.com/Rockbox/rockbox.git` |
| 主题（私有） | `D:\rockbox` | `master` | 独立仓库，只放主题/CJK 相关脚本与素材 |

`diy/r1-erosq` 有 6 个提交（4 个功能改动 + 文档 + gitattributes），基于上游 `7d090f2`。

## 1. 公开仓库（固件）

先在 GitHub 上点 **Fork**（`Rockbox/rockbox` → 你自己的账号），然后：

```bash
cd D:\rockbox\_rbsrc
git remote add myfork https://github.com/<你的用户名>/rockbox.git
git push -u myfork diy/r1-erosq
```

> **注意**：当前是 `--filter=blob:none` 的浅克隆。推送前必须先解除浅克隆：
> ```bash
> git fetch --unshallow --filter=blob:none origin
> ```
> 否则 GitHub 会拒绝（`shallow update not allowed`）。

以后上游更新：

```bash
git fetch origin
git merge origin/master        # 或 git rebase origin/master
# 解决冲突后重新编译
```

## 2. 私有仓库（主题 / CJK）

在 GitHub 新建一个 **Private** 空仓库（不要勾选 README），然后：

```bash
cd D:\rockbox
git remote add private https://github.com/<你的用户名>/rockbox-diy-themes.git
git push -u private master
```

仓库内容：`*.py` 构建脚本、`*.md` 说明、`fonts_src/`（TTF 源）、`icon_src/`、`ref/`。
`.gitignore` 已排除构建产物（`.rockbox/`、`发布包/`、`out*/`、`_rbsrc/`、日志、音视频）。
当前约 197 MB；如果嫌大，可把 `fonts_src/` 改为 Git LFS 或干脆不入库（脚本能重新下载字体）。

## 3. 认证

本机没有 `gh` CLI，也没有配置 token。两种办法：

- **最简单**：第一次 `git push` 时让 Git 弹出凭据窗口，用 GitHub 账号 + Personal Access Token（推荐 token，不用密码）；
- 或者安装 `gh`：`winget install GitHub.cli` 后 `gh auth login`，再 `gh repo create`。

## 4. 日常流程

```bash
# 拉上游 + 合并
cd D:\rockbox\_rbsrc
git fetch origin
git merge origin/master

# 编译（两个设备）
bash diy/build/build-hibyr1.sh        # R1
bash diy/build/build-aigoerosq.sh     # EROS Q（会自动打开 MPEGPLAYER_BLIT_UPDATE）

# 部署：把 build-*/apps/plugins/**/*.rock 拷到卡的 .rockbox/rocks/ 对应目录（先备份）
```

## 5. 备份

- `D:\rockbox\diy-r1-erosq.bundle`：固件分支的 git bundle（64 MB），可直接 `git clone` 恢复。
- 补丁形式：`D:\rockbox\patches\*.patch`（`git format-patch` 导出），可在任意上游版本上 `git am`。
