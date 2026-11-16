$ENV:Path += ";D:\Windows\Git;D:\Windows\Git\bin;D:\Windows\mingw64\bin;D:\Windows\nasm;D:\Windows\QEMU"

Set-Alias -Scope Global -Name make -Value mingw32-make.exe

git config --global user.email "ggponti74@hotmail.com"
git config --global user.name "ggponti74"
