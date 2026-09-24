#======================================================================
#  draw-topology.ps1 —— 生成聊天室网络拓扑结构图 topology.png
#  用法：powershell -ExecutionPolicy Bypass -File .\draw-topology.ps1
#  依赖：Windows + .NET Framework（System.Drawing，即 GDI+）
#  输出：chatroom\topology.png（980x740，2x 缩放渲染）
#======================================================================
param(
    [string]$Out = "topology.png"
)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

$W = 980; $H = 740; $SCALE = 2
$bmp = New-Object System.Drawing.Bitmap ($W * $SCALE), ($H * $SCALE)
$g   = [System.Drawing.Graphics]::FromImage($bmp)
$g.SmoothingMode        = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
$g.TextRenderingHint    = [System.Drawing.Text.TextRenderingHint]::AntiAliasGridFit
$g.InterpolationMode    = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
$g.ScaleTransform($SCALE, $SCALE)
$g.Clear([System.Drawing.Color]::White)

function New-Brush([string]$hex) { [System.Drawing.SolidBrush]::new([System.Drawing.ColorTranslator]::FromHtml($hex)) }
function New-Pen([string]$hex, [single]$w, [switch]$dash) {
    $p = [System.Drawing.Pen]::new([System.Drawing.ColorTranslator]::FromHtml($hex), $w)
    if ($dash) { $p.DashStyle = [System.Drawing.Drawing2D.DashStyle]::Dash }
    $p
}
function New-Font([single]$size, [System.Drawing.FontStyle]$style = [System.Drawing.FontStyle]::Regular) {
    [System.Drawing.Font]::new('Microsoft YaHei', $size, $style, [System.Drawing.GraphicsUnit]::Pixel)
}
function Add-RoundRectPath([single]$x, [single]$y, [single]$w, [single]$h, [single]$r) {
    $d = $r * 2
    $path = [System.Drawing.Drawing2D.GraphicsPath]::new()
    $path.AddArc($x, $y, $d, $d, 180, 90)
    $path.AddArc($x + $w - $d, $y, $d, $d, 270, 90)
    $path.AddArc($x + $w - $d, $y + $h - $d, $d, $d, 0, 90)
    $path.AddArc($x, $y + $h - $d, $d, $d, 90, 90)
    $path.CloseFigure()
    $path
}
function Draw-Rounded([single]$x, [single]$y, [single]$w, [single]$h, [single]$r, $fill, $pen) {
    $p = Add-RoundRectPath $x $y $w $h $r
    if ($fill) { $g.FillPath($fill, $p) }
    if ($pen)  { $g.DrawPath($pen, $p) }
    $p.Dispose()
}
function Draw-Text([string]$text, [single]$x, [single]$y, [single]$w, [single]$h,
                   $font, $brush, [bool]$center = $true) {
    $fmt = [System.Drawing.StringFormat]::new()
    if ($center) { $fmt.Alignment = 'Center'; $fmt.LineAlignment = 'Center' }
    else         { $fmt.Alignment = 'Near';   $fmt.LineAlignment = 'Center' }
    $rect = [System.Drawing.RectangleF]::new($x, $y, $w, $h)
    $g.DrawString($text, $font, $brush, $rect, $fmt)
    $fmt.Dispose()
}

# ---------------------- 配色 ----------------------
$cTitle       = "#1f2430"
$cSub         = "#5a6472"
$cLanLine     = "#43a047"; $cLanText = "#2e7d32"; $cLanFill = "#f2faf0"
$cServerFill  = "#2f6fbf"; $cServerLine = "#1d4f8f"
$cClientFill  = "#eaf1f9"; $cClientLine = "#2f6fbf"
$cClientTitle = "#1d4f8f"; $cClientText = "#22303f"; $cClientDim = "#6a7688"
$cMsgLine     = "#e6a23c"; $cMsgFill = "#fff7e0";   $cMsgText = "#7a5b00"
$cWarn        = "#c0392b"

# ---------------------- 标题区 ----------------------
Draw-Text "简易在线聊天室 —— 网络拓扑结构图" 0 8 980 36 (New-Font 22 ([System.Drawing.FontStyle]::Bold)) (New-Brush $cTitle)
Draw-Text "C++ 简易聊天室 ｜ 服务器 + 多个客户端（星型拓扑）｜ TCP 长连接（双向）｜ 消息经服务器广播" 0 44 980 24 (New-Font 12) (New-Brush $cSub)

# ---------------------- 局域网边界 ----------------------
Draw-Rounded 40 80 900 580 16 (New-Brush $cLanFill) (New-Pen $cLanLine 2)
Draw-Text "局域网（同一路由器 / Wi-Fi 之下，如 192.168.1.x 网段）" 58 96 400 20 (New-Font 13 ([System.Drawing.FontStyle]::Bold)) (New-Brush $cLanText)

# ---------------------- 服务器 ----------------------
Draw-Rounded 345 145 290 120 12 (New-Brush $cServerFill) (New-Pen $cServerLine 2.5)
Draw-Text "① 服务器（先启动）" 345 152 290 30 (New-Font 15 ([System.Drawing.FontStyle]::Bold)) (New-Brush "#ffffff")
Draw-Text "chat-server" 345 184 290 24 (New-Font 13) (New-Brush "#ffffff")
Draw-Text "监听端口 8888（可用参数修改）" 345 208 290 22 (New-Font 12) (New-Brush "#e3edf7")
Draw-Text "运行主机 IP：192.168.1.100（示例）" 345 230 290 20 (New-Font 12) (New-Brush "#e3edf7")

# ---------------------- TCP 连接线（带箭头） ----------------------
$edgePen = New-Pen $cClientLine 2.5
$cap = [System.Drawing.Drawing2D.AdjustableArrowCap]::new(7, 7)
$edgePen.CustomEndCap = $cap
$g.DrawLine($edgePen, 490, 265, 217, 455)   # 服务器 -> 客户端A
$g.DrawLine($edgePen, 490, 265, 490, 455)   # 服务器 -> 客户端B
$g.DrawLine($edgePen, 490, 265, 763, 455)   # 服务器 -> 客户端C
$edgePen.Dispose(); $cap.Dispose()
Draw-Text "TCP 长连接（双向）· 端口 8888" 510 342 260 22 (New-Font 12.5) (New-Brush $cClientTitle) $false

# ---------------------- 客户端 ----------------------
foreach ($box in @(
    @{ x = 92;  y = 455; w = 250; t1 = "② 客户端 A"; t2 = "昵称：小明";     t3 = "本机 127.0.0.1";        t4 = "运行：chat-client";            t5 = "（服务器同机，另开窗口）" },
    @{ x = 365; y = 455; w = 250; t1 = "③ 客户端 B"; t2 = "昵称：小红";     t3 = "本机 127.0.0.1";        t4 = "运行：chat-client";            t5 = "（服务器同机，另开窗口）" },
    @{ x = 638; y = 455; w = 250; t1 = "④ 客户端 C"; t2 = "昵称：小刚";     t3 = "局域网其他设备";        t4 = "chat-client 192.168.1.100";    t5 = "（连服务器 IP，端口默认 8888）" }
)) {
    $x = $box.x; $w = $box.w
    Draw-Rounded $x 455 $w 130 10 (New-Brush $cClientFill) (New-Pen $cClientLine 1.5)
    Draw-Text $box.t1 $x 460 $w 26 (New-Font 14 ([System.Drawing.FontStyle]::Bold)) (New-Brush $cClientTitle)
    Draw-Text $box.t2 $x 486 $w 22 (New-Font 12.5) (New-Brush $cClientText)
    Draw-Text $box.t3 $x 508 $w 22 (New-Font 12.5) (New-Brush $cClientText)
    Draw-Text $box.t4 $x 530 $w 20 (New-Font 12)   (New-Brush $cClientText)
    Draw-Text $box.t5 $x 550 $w 20 (New-Font 11)   (New-Brush $cClientDim)
}

# ---------------------- 消息流转说明 ----------------------
Draw-Rounded 140 615 700 38 8 (New-Brush $cMsgFill) (New-Pen $cMsgLine 1.5)
Draw-Text "消息流转：客户端 → 服务器 → 广播给所有在线客户端（包括发送者自己）" 140 615 700 38 (New-Font 13) (New-Brush $cMsgText)

# ---------------------- 图例 / 注意事项 ----------------------
Draw-Text "图例：实线 ＝ TCP 长连接（双向）｜星型拓扑：客户端之间不直连，消息一律经服务器转发" 0 676 980 20 (New-Font 11.5) (New-Brush $cSub)
Draw-Text "注意：跨设备连接前，请在运行服务器的主机上用防火墙放行 TCP 8888 端口" 0 698 980 20 (New-Font 11.5) (New-Brush $cWarn)

# ---------------------- 保存 ----------------------
$outPath = Join-Path $PSScriptRoot $Out
$bmp.Save($outPath, [System.Drawing.Imaging.ImageFormat]::Png)
$g.Dispose(); $bmp.Dispose()
Write-Host "已生成拓扑图: $outPath"