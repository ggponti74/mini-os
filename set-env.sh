sudo apt update
sudo apt install -y nasm build-essential gcc qemu-system-x86 xorriso
sudo apt update && sudo apt install -y novnc websockify
websockify --web /usr/share/novnc 8080 localhost:5900
