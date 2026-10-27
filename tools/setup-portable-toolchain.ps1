[CmdletBinding()]
param(
    [string]$InstallPath = (Join-Path $env:LOCALAPPDATA "mini-os-toolchains\i686-elf-15.2.0")
)

$ErrorActionPreference = "Stop"

$downloadUrl = "https://github.com/lordmilko/i686-elf-tools/releases/download/15.2.0/i686-elf-tools-windows.zip"
$expectedSha256 = "83558a1064413a2f0b82769b6e4c94aa09bb7ba31ffbae7a791d8ee35f16b312"

if (Test-Path -LiteralPath $InstallPath) {
    $existingCompiler = Get-ChildItem -LiteralPath $InstallPath -Filter "i686-elf-gcc.exe" -File -Recurse |
        Select-Object -First 1
    if ($existingCompiler) {
        Write-Host "Portable toolchain already exists at $InstallPath"
        return
    }

    throw "Install path exists but contains no i686-elf-gcc.exe: $InstallPath"
}

$parentPath = Split-Path -Parent $InstallPath
$workPath = Join-Path ([System.IO.Path]::GetTempPath()) ("mini-os-toolchain-" + [guid]::NewGuid().ToString("N"))
$archivePath = Join-Path $workPath "toolchain.zip"
$extractPath = Join-Path $workPath "extracted"

New-Item -ItemType Directory -Path $parentPath -Force | Out-Null
New-Item -ItemType Directory -Path $workPath -Force | Out-Null

try {
    Write-Host "Downloading the Windows i686-elf toolchain (about 888 MB)..."
    Invoke-WebRequest -Uri $downloadUrl -OutFile $archivePath

    $actualSha256 = (Get-FileHash -LiteralPath $archivePath -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($actualSha256 -ne $expectedSha256) {
        throw "Toolchain archive SHA-256 mismatch. Expected $expectedSha256; got $actualSha256."
    }

    Write-Host "Archive verified. Extracting..."
    Expand-Archive -LiteralPath $archivePath -DestinationPath $extractPath

    $compiler = Get-ChildItem -LiteralPath $extractPath -Filter "i686-elf-gcc.exe" -File -Recurse |
        Select-Object -First 1
    if (-not $compiler) {
        throw "The verified archive did not contain i686-elf-gcc.exe."
    }

    Move-Item -LiteralPath $extractPath -Destination $InstallPath
    Write-Host "Portable toolchain installed at $InstallPath"
}
finally {
    if (Test-Path -LiteralPath $workPath) {
        Remove-Item -LiteralPath $workPath -Recurse -Force
    }
}