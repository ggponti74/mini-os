SHELL := cmd.exe

BUILD := build
DIST := dist

BOOT_SRC := boot/boot.asm
KERNEL_ENTRY_SRC := boot/kernel.asm
KERNEL_C_SRC := kernel/main.c

BOOT_BIN := $(BUILD)/boot.bin
KERNEL_ENTRY_OBJ := $(BUILD)/kernel_entry.o
KERNEL_C_OBJ := $(BUILD)/main.o
KERNEL_ELF := $(BUILD)/kernel.elf
KERNEL_BIN := $(BUILD)/kernel.bin
IMAGE := $(DIST)/mini-os.img

NASM := nasm
CC := gcc
OBJCOPY := objcopy
CFLAGS := -m32 -ffreestanding -c -O2
QEMU := qemu-system-x86_64

# Windows CMD-compatible path conversion
FIX_PATH = $(subst /,\,$(1))

.PHONY: all boot kernel image run clean

all: image

$(BUILD):
	if not exist $(BUILD) mkdir $(BUILD)

$(DIST):
	if not exist $(DIST) mkdir $(DIST)

# 1. Assemble 16-bit bootloader
boot: $(BUILD)
	$(NASM) -f bin $(BOOT_SRC) -o $(BOOT_BIN)

# 2. Assemble Stage 2 Entry into COFF format (Win32 compatible)
$(KERNEL_ENTRY_OBJ): $(KERNEL_ENTRY_SRC) | $(BUILD)
	$(NASM) -f win32 $(KERNEL_ENTRY_SRC) -o $(KERNEL_ENTRY_OBJ)

# 3. Compile C kernel into 32-bit object file
$(KERNEL_C_OBJ): $(KERNEL_C_SRC) | $(BUILD)
	$(CC) $(CFLAGS) $(KERNEL_C_SRC) -o $(KERNEL_C_OBJ)

# 4. Link with MinGW GCC and extract raw binary with objcopy
kernel: $(KERNEL_ENTRY_OBJ) $(KERNEL_C_OBJ)
	$(CC) -m32 -nostdlib -Wl,-Ttext,0x7e00 -o $(KERNEL_ELF) $(KERNEL_ENTRY_OBJ) $(KERNEL_C_OBJ)
	$(OBJCOPY) -O binary -j .text -j .data -j .rodata -j .bss $(KERNEL_ELF) $(KERNEL_BIN)

# 5. Concatenate boot sector and kernel binary into final OS image
# 5. Concatenate boot sector and kernel binary, then pad image
# 5. Concatenate boot sector + kernel binary, then pad image to 1.44MB
image: boot kernel $(DIST)
	copy /b $(call FIX_PATH,$(BOOT_BIN)) + $(call FIX_PATH,$(KERNEL_BIN)) $(call FIX_PATH,$(IMAGE))
	powershell -Command "$$f = [System.IO.File]::OpenWrite('$(IMAGE)'); $$f.SetLength(1474560); $$f.Close()"

run: image
	$(QEMU) -fda $(IMAGE) -audiodev id=audio0,driver=none -device sb16,audiodev=audio0	

clean:
	if exist $(BUILD) rmdir /s /q $(BUILD)
	if exist $(DIST) rmdir /s /q $(DIST)