winget install --id Microsoft.PowerShell --source winget

$ENV:Path += ";D:\Windows\Git"
$ENV:Path += ";D:\Windows\Git\bin"
$ENV:Path += ";D:\Windows\mingw64\bin"
$ENV:Path += ";D:\Windows\nasm"
$ENV:Path += ";D:\Windows\QEMU"

Set-Alias -Name make -Value mingw32-make.exe

git config --global user.email "ggponti74@hotmail.com"
git config --global user.name "ggponti74"