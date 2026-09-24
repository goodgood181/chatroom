#======================================================================
#  smoke-test.ps1 —— 自动化冒烟测试驱动（仅用于开发验证）
#  用法：
#    .\smoke-test.ps1 -Nick alice -StartDelaySec 1 -Messages @("hello 1","hello 2")
#    .\smoke-test.ps1 -Nick bob -StartDelaySec 3 -Messages @("hi 1")
#  脚本会启动一个 chat-client，按时间间隔逐条写入消息，最后发送 /quit 退出，
#  并把客户端收到的全部内容打印出来。多个实例可并行，用于验证消息广播。
#======================================================================
param(
    [string]$Nick        = "user",
    [double]$StartDelaySec = 0,
    [string]$MessagesJoined = "",   # 分号分隔的消息列表，例如 "m1;m2;m3"
    [double]$GapMs       = 800,
    [string]$HostAddr    = "127.0.0.1",
    [int]$Port           = 9999,
    [string]$ClientPath  = ""       # 指定客户端 exe 路径（默认 build\Release\chat-client.exe）
)
$ErrorActionPreference = 'Stop'
Start-Sleep -Seconds $StartDelaySec
$Messages = @($MessagesJoined -split ';' | ForEach-Object { $_.Trim() } | Where-Object { $_ -ne '' })

$client = $ClientPath
if (-not $client -or -not (Test-Path $client)) {
    $client = (Join-Path $PSScriptRoot "build\Release\chat-client.exe")
    if (-not (Test-Path $client)) { $client = (Join-Path $PSScriptRoot "chat-client.exe") }
}

$psi = New-Object System.Diagnostics.ProcessStartInfo
$psi.FileName = (Resolve-Path $client).Path
$psi.Arguments = "$HostAddr $Port"
$psi.RedirectStandardInput = $true
$psi.RedirectStandardOutput = $true
$psi.RedirectStandardError  = $true
$psi.StandardOutputEncoding = [System.Text.Encoding]::UTF8
$psi.UseShellExecute = $false

$p = [System.Diagnostics.Process]::Start($psi)

# 昵称用原始 ASCII 字节写入，避免 .NET StreamWriter 首次写入附带 BOM（测试伪影）
$nameBytes = [System.Text.Encoding]::ASCII.GetBytes($Nick + "`n")
$p.StandardInput.BaseStream.Write($nameBytes, 0, $nameBytes.Length)
$p.StandardInput.BaseStream.Flush()

foreach ($m in $Messages) {
    Start-Sleep -Milliseconds $GapMs
    $p.StandardInput.WriteLine($m)
    $p.StandardInput.Flush()
}
Start-Sleep -Milliseconds $GapMs
$p.StandardInput.WriteLine("/quit")
$p.StandardInput.Flush()
$p.StandardInput.Close()

if (-not $p.WaitForExit(15000)) {
    $p.Kill()
    throw "$Nick timed out"
}
$out = $p.StandardOutput.ReadToEnd()
$err = $p.StandardError.ReadToEnd()
$p.WaitForExit()

Write-Output "===== [$Nick] exit=$($p.ExitCode) ====="
Write-Output $out
if ($err) { Write-Output "----- stderr -----"; Write-Output $err }