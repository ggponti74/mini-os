#!/usr/bin/env bash
mkdir -p initrd_root
echo "Hello from mini-os initrd!" > initrd_root/readme.txt
echo "Kernel config placeholder" > initrd_root/system.cfg

# Create uncompressed tar archive
tar -cvf initrd.tar -C initrd_root .
rm -rf initrd_root