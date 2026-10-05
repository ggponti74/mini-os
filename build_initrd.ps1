New-Item -ItemType Directory -Force -Path "initrd_root" | Out-Null
Set-Content -Path "initrd_root\readme.txt" -Value "Hello from mini-os initrd!"
Set-Content -Path "initrd_root\system.cfg" -Value "timezone -4"

# Using tar.exe available natively in modern Windows
tar.exe -cvf initrd.tar -C initrd_root .
Remove-Item -Recurse -Force "initrd_root"