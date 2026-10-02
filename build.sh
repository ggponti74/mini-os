# 1. Create target directories
mkdir -p build dist

# 2. Assemble Sector 1 bootloader (flat binary)
nasm -f bin boot/boot.asm -o build/boot.bin

# 3. Assemble Stage 2 wrapper (32-bit ELF object)
nasm -f elf32 boot/kernel.asm -o build/kernel_entry.o

# 4. Compile C kernel (32-bit freestanding object)
gcc -m32 -ffreestanding -fno-pie -fno-stack-protector -c kernel/main.c -o build/main.o

# 5. Link kernel objects into flat binary starting at 0x7E00
ld -m elf_i386 -e stage2_entry -Ttext 0x7e00 --oformat binary -o build/kernel.bin build/kernel_entry.o build/main.o
