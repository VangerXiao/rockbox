# 拉取上游 Rockbox 并合并到当前 DIY 分支
# 用法：右键"使用 PowerShell 运行"，或 pwsh -File diy\sync-upstream.ps1
$ErrorActionPreference = 'Stop'
$repo = Resolve-Path (Join-Path $PSScriptRoot '..\..')
Set-Location $repo

"仓库: $repo"
"当前分支: " + (git rev-parse --abbrev-ref HEAD)

git fetch origin
$behind = [int](git rev-list --count 'HEAD..origin/master')
$ahead  = [int](git rev-list --count 'origin/master..HEAD')
"上游领先 $behind 个提交，本地领先 $ahead 个提交"

if ($behind -eq 0) { "已经是最新，无需合并。"; exit 0 }

git merge --no-edit origin/master
if ($LASTEXITCODE -ne 0) {
    "合并出现冲突，请手动解决后再运行：git commit"
    exit 1
}

"合并完成，最近提交："
git log --oneline -5
""
"下一步：重新编译"
"  bash diy/build/build-hibyr1.sh      # R1"
"  bash diy/build/build-aigoerosq.sh   # EROS Q"
