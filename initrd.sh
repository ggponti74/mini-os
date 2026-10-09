# 1. Create a temporary staging directory
mkdir initrd_root

# 2. Add placeholder files
echo "Hello from mini-os initrd!" > initrd_root/hello.txt
echo "timezone -4\n" > initrd_root/system.cfg

cat initrd_root/system.cfg

# 3. Create an UNCOMPRESSED tar file (POSIX standard header)
tar -cvf initrd.tar -C initrd_root .

# 4. Clean up temporary directory
rm -rf initrd_root