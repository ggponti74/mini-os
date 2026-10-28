New-Item -ItemType Directory -Force -Path "initrd_root" | Out-Null
Set-Content -Encoding Ascii -Path "initrd_root\readme.txt" -Value "Hello from mini-os initrd!"
Set-Content -Encoding Ascii -Path "initrd_root\system.cfg" -Value "timezone -4"
Add-Content -Encoding Ascii -Path "initrd_root\system.cfg" -Value "alias copy=cp"
Add-Content -Encoding Ascii -Path "initrd_root\system.cfg" -Value "alias move=mv"
Add-Content -Encoding Ascii -Path "initrd_root\system.cfg" -Value "alias del=rm"
Add-Content -Encoding Ascii -Path "initrd_root\system.cfg" -Value "sound on"
Add-Content -Encoding Ascii -Path "initrd_root\system.cfg" -Value "set os=mini-os 1.0"
Copy-Item -Path "bin\demo" -Destination "initrd_root\demo"
Copy-Item -Path "music.cfg" -Destination "initrd_root\ringtone"
Copy-Item -Path "build\edit" -Destination "initrd_root\edit"
Copy-Item -Path "LICENSE" -Destination "initrd_root\LICENSE"

# Using tar.exe available natively in modern Windows
tar.exe -cvf initrd.tar -C initrd_root .
Remove-Item -Recurse -Force "initrd_root"