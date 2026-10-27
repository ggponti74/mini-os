[CmdletBinding()]
param(
    [string]$ToolchainPath = (Join-Path $env:LOCALAPPDATA "mini-os-toolchains\i686-elf-15.2.0")
)

$ErrorActionPreference = "Stop"
$repositoryRoot = Split-Path -Parent $PSScriptRoot

$compiler = Get-ChildItem -LiteralPath $ToolchainPath -Filter "i686-elf-gcc.exe" -File -Recurse -ErrorAction SilentlyContinue |
    Select-Object -First 1
$objcopy = Get-ChildItem -LiteralPath $ToolchainPath -Filter "i686-elf-objcopy.exe" -File -Recurse -ErrorAction SilentlyContinue |
    Select-Object -First 1
if (-not $compiler -or -not $objcopy) {
    throw "Portable i686-elf compiler not found. Run tools\setup-portable-toolchain.ps1 first."
}

$makeCommand = Get-Command make.exe, mingw32-make.exe -CommandType Application -ErrorAction SilentlyContinue |
    Select-Object -First 1
if (-not $makeCommand) {
    throw "GNU Make is required. Add make.exe or mingw32-make.exe to PATH."
}

$nasmCommand = Get-Command nasm.exe -CommandType Application -ErrorAction SilentlyContinue |
    Select-Object -First 1
if (-not $nasmCommand) {
    throw "NASM is required. Add nasm.exe to PATH."
}

$compilerBin = Split-Path -Parent $compiler.FullName
$nasmBin = Split-Path -Parent $nasmCommand.Source
$env:PATH = "$compilerBin;$nasmBin;$env:PATH"
$makeArguments = @(
    "CC=i686-elf-gcc",
    "OBJCOPY=i686-elf-objcopy",
    "NASM=nasm",
    "WIN_CFLAGS=",
    "WIN_LDFLAGS=",
    "iso"
)

Push-Location $repositoryRoot
try {
    & $makeCommand.Source @makeArguments
    if ($LASTEXITCODE -ne 0) {
        throw "ISO build failed with exit code $LASTEXITCODE."
    }
}
finally {
    Pop-Location
}

Write-Host "ISO created at $(Join-Path $repositoryRoot 'dist\mini-os.iso')"