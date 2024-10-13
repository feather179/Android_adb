param (
    [string]$FilePath,
    [string]$VariableName,
    [string]$OutputFile = "",
    [int]$BlockSize = 4096
)

$ResolvedFilePath = Resolve-Path -Path $FilePath
if (-not (Test-Path -Path $ResolvedFilePath)) {
    Write-Error "File not found: $ResolvedFilePath"
    exit
}

# 初始化输出字符串
$output = "unsigned char $VariableName[] = {`n"

# 打开文件并逐块读取
$bytesRead = 0
$totalBytes = (Get-Item $ResolvedFilePath).Length
$buffer = New-Object byte[] $BlockSize

$fs = [System.IO.File]::OpenRead($ResolvedFilePath)
try {
    while ($bytesRead -lt $totalBytes) {
        $read = $fs.Read($buffer, 0, $BlockSize)
        for ($i = 0; $i -lt $read; $i++) {
            $output += "0x{0:X2}, " -f $buffer[$i]
            if (($i + 1) % 16 -eq 0 -or $i -eq $read - 1) {
                # $output = $output.TrimEnd(", ") + "`n"
                $output = $output + "`n"
            }
        }
        $bytesRead += $read
    }
} finally {
    $fs.Close()
}

# 移除最后一个逗号和空格（如果存在）
if ($output[-2] -eq ',') {
    $output = $output.Substring(0, $output.Length - 2) + "`n"
}

# 结束数组定义
$output += "};`n"
# 添加数组长度
# $output += "unsigned int $VariableName`_len = {0};`n" -f $totalBytes

# 输出到控制台或文件
if ([string]::IsNullOrEmpty($OutputFile)) {
    Write-Output $output
} else {
    $output | Out-File -FilePath $OutputFile
}


