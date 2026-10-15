#!/usr/bin/env bash
mkdir -p initrd_root
echo "Hello from mini-os initrd!" > initrd_root/readme.txt
echo "timezone -4" > initrd_root/system.cfg
echo "alias copy=cp" >> initrd_root/system.cfg
echo "alias move=mv" >> initrd_root/system.cfg
echo "alias del=rm" >> initrd_root/system.cfg
cp bin/demo initrd_root/

# Create uncompressed tar archive
tar -cvf initrd.tar -C initrd_root .
rm -rf initrd_root