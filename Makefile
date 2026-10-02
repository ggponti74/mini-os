# Detect Operating System
ifeq ($(OS),Windows_NT)
    SHELL := cmd.exe
    MKDIR = if not exist $(call FIX_PATH,$(1)) mkdir $(call FIX_PATH,$(1))
    RM = if exist $(call FIX_PATH,$(1)) rmdir /s /q $(call FIX_PATH,$(1))
    NASM_FMT = win32
    FIX_PATH = $(subst /,\,$(1))
    COPY = copy /y
    CONCAT = copy /b $(call FIX_PATH,$(1)) + $(call FIX_PATH,$(2)) $(call FIX_PATH,$(3))
    PAD_IMAGE = powershell -Command "$$f = [System.IO.File]::OpenWrite('$(1)');$$f.SetLength(1474560);$$f.Close()"
    QEMU = qemu-system-x86_64
else
    SHELL := /bin/sh
    MKDIR = mkdir -p $(1)
    RM = rm -rf $(1)
    NASM_FMT = elf32
    FIX_PATH = $(1)
    COPY = cp
    CONCAT = cat $(1) $(2) > $(3)
    PAD_IMAGE = truncate -s 1474560 $(1)
    QEMU = qemu-system-x86_64
endif

BUILD := build
DIST := dist

BOOT_SRC := boot/boot.asm
KERNEL_ENTRY_SRC := boot/kernel.asm

BOOT_BIN := $(BUILD)/boot.bin
KERNEL_ENTRY_OBJ := $(BUILD)/kernel_entry.o
KERNEL_ELF := $(BUILD)/kernel.elf
KERNEL_BIN := $(BUILD)/kernel.bin
IMAGE := $(DIST)/mini-os.img
ISO := $(DIST)/mini-os.iso

# Recursive wildcard function definition
rwildcard = $(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

# Discover C and ASM sources in kernel/ and any subfolders
C_SRCS   := $(call rwildcard,kernel,*.c)
ASM_SRCS := $(call rwildcard,kernel,*.asm)

C_OBJS   := $(patsubst kernel/%.c,$(BUILD)/%.o,$(C_SRCS))
ASM_OBJS := $(patsubst kernel/%.asm,$(BUILD)/%.o,$(ASM_SRCS))

NASM := nasm
CC := gcc
OBJCOPY := objcopy
CFLAGS = -m32 -ffreestanding -fno-pic -fno-pie -fno-stack-protector -nostdlib -c

.PHONY: all boot kernel image iso run run-iso clean

all: image

### Directory creation rules
$(BUILD):
	@$(call MKDIR,$(BUILD))

$(DIST):
	@$(call MKDIR,$(DIST))

### 1. Assemble 16-bit bootloader (force exact 512-byte size)
boot: | $(BUILD)
	$(NASM) -f bin $(BOOT_SRC) -o $(BOOT_BIN)

### 2. Assemble Kernel Stage Entry (boot/kernel.asm)
$(KERNEL_ENTRY_OBJ): $(KERNEL_ENTRY_SRC) | $(BUILD)
	$(NASM) -f $(NASM_FMT) $(KERNEL_ENTRY_SRC) -o $(KERNEL_ENTRY_OBJ)

### 3. Assemble all kernel/*.asm files across subdirectories
$(BUILD)/%.o: kernel/%.asm | $(BUILD)
	@$(call MKDIR,$(dir $@))
	$(NASM) -f $(NASM_FMT) $< -o $@

### 4. Compile all kernel/*.c files across subdirectories
$(BUILD)/%.o: kernel/%.c | $(BUILD)
	@$(call MKDIR,$(dir $@))
	$(CC) $(CFLAGS) $< -o $@

### 5. Link kernel objects into flat binary using linker script
kernel: $(KERNEL_ENTRY_OBJ) $(ASM_OBJS) $(C_OBJS) | $(BUILD)
	$(CC) -m32 -nostdlib -Wl,-T,linker.ld -o $(KERNEL_ELF) $(KERNEL_ENTRY_OBJ) $(ASM_OBJS) $(C_OBJS)
	$(OBJCOPY) -O binary $(KERNEL_ELF) $(KERNEL_BIN)

### 6. Concatenate boot sector and kernel binary into floppy image
image: boot kernel | $(DIST)
	@$(call CONCAT,$(BOOT_BIN),$(KERNEL_BIN),$(IMAGE))
	@$(call PAD_IMAGE,$(IMAGE))

# Step to construct a bootable ISO image using xorriso
iso: image | $(DIST)
	@$(call MKDIR,$(BUILD)/iso_root)
	$(COPY) $(call FIX_PATH,$(IMAGE)) $(call FIX_PATH,$(BUILD)/iso_root/mini-os.img)
	xorriso -as mkisofs -b mini-os.img -boot-load-size 4 -o $(ISO) $(BUILD)/iso_root

# Run raw floppy image explicitly with active VGA window and debug port output
run: image
	$(QEMU) -drive file=$(IMAGE),format=raw,if=floppy -boot a -vga std -debugcon file:qemu_img.log

# Run CD-ROM ISO properly attached to QEMU
run-iso: iso
	$(QEMU) -cdrom $(ISO) -vga std -debugcon file:qemu_iso.log

clean:
	@$(call RM,$(BUILD))
	@$(call RM,$(DIST))