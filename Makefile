# Detect Operating System
ifeq ($(OS),Windows_NT)
    SHELL := cmd.exe
    MKDIR = if not exist $(call FIX_PATH,$(1)) mkdir $(call FIX_PATH,$(1))
    RM = if exist $(call FIX_PATH,$(1)) rmdir /s /q $(call FIX_PATH,$(1))
    NASM_FMT = elf32
    FIX_PATH = $(subst /,\,$(1))
    COPY = copy /y
    CONCAT = copy /b $(call FIX_PATH,$(1)) + $(call FIX_PATH,$(2)) $(call FIX_PATH,$(3))
    PAD_IMAGE = powershell -Command "$$f = [System.IO.File]::OpenWrite('$(1)'); $$f.SetLength(1474560); $$f.Close()"
    QEMU = qemu-system-i386
    QEMU_ISO_FLAGS = -cdrom $(call FIX_PATH,$(ISO)) -vga std -m 16M
    QEMU_NET_FLAGS = -nic user,model=e1000
    QEMU_AUDIO_FLAGS ?= -audiodev dsound,id=pcspk -machine pcspk-audiodev=pcspk
    INITRD_SOURCE = build_initrd.ps1
    INITRD_COMMAND = powershell -ExecutionPolicy Bypass -File build_initrd.ps1
else
    SHELL := /bin/sh
    MKDIR = mkdir -p $(1)
    RM = rm -rf $(1)
    NASM_FMT = elf32
    FIX_PATH = $(1)
    COPY = cp
    CONCAT = cat $(1) $(2) > $(3)
    PAD_IMAGE = truncate -s 1474560 $(1)
    QEMU = qemu-system-i386
    QEMU_ISO_FLAGS = -cdrom $(ISO) -nographic
    QEMU_NET_FLAGS = -nic user,model=e1000
    QEMU_AUDIO_FLAGS ?= -audiodev pa,id=pcspk -machine pcspk-audiodev=pcspk
    INITRD_SOURCE = build_initrd.sh
    INITRD_COMMAND = bash build_initrd.sh
endif

BUILD := build
DIST := dist

QEMU_DEBUG_FLAGS = -d int,guest_errors,cpu_reset -D $(call FIX_PATH,$(BUILD)/qemu.log) -debugcon file:$(call FIX_PATH,$(BUILD)/guest.log) -global isa-debugcon.iobase=0xe9 -no-reboot

BOOT_SRC := boot/boot.asm
KERNEL_ENTRY_SRC := boot/kernel.asm

BOOT_BIN := $(BUILD)/boot.bin
KERNEL_ENTRY_OBJ := $(BUILD)/kernel_entry.o
KERNEL_ELF := $(BUILD)/kernel.elf
KERNEL_BIN := $(BUILD)/kernel.bin
IMAGE := $(DIST)/mini-os.img
ISO := $(DIST)/mini-os.iso

# Discover C and ASM sources in kernel/ and subfolders
rwildcard = $(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

# Discover kernel C, ASM, and GAS files across subdirectories
C_SRCS     := $(wildcard kernel/*.c kernel/*/*.c kernel/*/*/*.c)
ASM_SRCS   := $(wildcard kernel/*.asm kernel/*/*.asm kernel/*/*/*.asm)
GAS_SRCS   := $(wildcard kernel/*.S kernel/*/*.S kernel/*/*/*.S)

# Generate object paths inside $(BUILD)/
C_OBJS     := $(patsubst kernel/%.c,$(BUILD)/%.o,$(C_SRCS))
ASM_OBJS   := $(patsubst kernel/%.asm,$(BUILD)/%.o,$(ASM_SRCS))
GAS_OBJS   := $(patsubst kernel/%.S,$(BUILD)/%.o,$(GAS_SRCS))

# Combine all kernel objects
ALL_OBJS   := $(ASM_OBJS) $(GAS_OBJS) $(C_OBJS)
NASM := nasm
CC := gcc
OBJCOPY := objcopy
CFLAGS := -m32 -ffreestanding -fno-pie -fno-pic -fno-stack-protector -c -O2 -Ikernel

.PHONY: all clean run run-iso iso boot kernel libsys image

all: image

$(BOOT_BIN): $(BOOT_SRC) | $(BUILD)
	$(NASM) -f bin $(BOOT_SRC) -o $(BOOT_BIN)

# Alias for manual 'make boot' calls
boot: $(BOOT_BIN)

$(BUILD):
	@$(call MKDIR,$(BUILD))

$(DIST):
	@$(call MKDIR,$(DIST))

# 2. Assemble Kernel Stage Entry
$(KERNEL_ENTRY_OBJ): $(KERNEL_ENTRY_SRC) | $(BUILD)
	$(NASM) -f $(NASM_FMT) $(KERNEL_ENTRY_SRC) -o $(KERNEL_ENTRY_OBJ)

# 3. Assemble all kernel/*.asm files
$(BUILD)/%.o: kernel/%.asm | $(BUILD)
	@$(call MKDIR,$(dir $@))
	$(NASM) -f $(NASM_FMT) $< -o $@

# Make initrd_data.o depend on initrd.tar if applicable
$(BUILD)/fs/initrd_data.o: kernel/fs/initrd_data.S initrd.tar | $(BUILD)
	@$(call MKDIR,$(dir $@))
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/edit: userland/Makefile userland/libsys/apps/editor.c \
                 userland/libsys/apps/editor.h \
                 userland/libsys/apps/editor_entry.c \
                 userland/libsys/apps/editor_start.S \
                 userland/libsys/apps/editor_syscall.S \
                 userland/flat.ld | $(BUILD)
	$(MAKE) -C userland edit

initrd.tar: $(BUILD)/edit $(INITRD_SOURCE) bin/demo music.cfg
	$(INITRD_COMMAND)
	
# GCC GNU assembly (.S)
$(BUILD)/%.o: kernel/%.S | $(BUILD)
	@$(call MKDIR,$(dir $@))
	$(CC) $(CFLAGS) $< -o $@

# C source (.c)
$(BUILD)/%.o: kernel/%.c | $(BUILD)
	@$(call MKDIR,$(dir $@))
	$(CC) $(CFLAGS) $< -o $@

# Explicitly place KERNEL_ENTRY_OBJ FIRST in the linker command line
kernel: $(KERNEL_ENTRY_OBJ) $(ALL_OBJS) | $(BUILD)
	$(CC) -m32 -nostdlib -no-pie -Wl,--build-id=none -Wl,-e,stage2_entry -Wl,-Map=$(BUILD)/linker.map -T linker.ld -o $(KERNEL_ELF) $(KERNEL_ENTRY_OBJ) $(ALL_OBJS)
	$(OBJCOPY) -O binary $(KERNEL_ELF) $(KERNEL_BIN)

libsys:
	$(MAKE) -C userland

# Make image depend on the actual binary file
image: boot kernel | $(DIST)
	$(call CONCAT,$(BOOT_BIN),$(KERNEL_BIN),$(IMAGE))
	$(call PAD_IMAGE,$(IMAGE))
	
# Generate bootable ISO using xorriso
iso: image | $(DIST)
	@$(call MKDIR,$(BUILD)/iso_root)
	$(COPY) $(call FIX_PATH,$(IMAGE)) $(call FIX_PATH,$(BUILD)/iso_root/mini-os.img)
	xorriso -as mkisofs -b mini-os.img -o $(ISO) $(BUILD)/iso_root

# Run floppy image directly
run: image
	$(QEMU) -fda $(IMAGE) $(QEMU_NET_FLAGS) $(QEMU_AUDIO_FLAGS) -nographic -serial stdio

# Run ISO image in CD-ROM mode
run-iso: iso
	$(QEMU) $(QEMU_ISO_FLAGS) $(QEMU_NET_FLAGS) $(QEMU_AUDIO_FLAGS) $(QEMU_DEBUG_FLAGS)
	
clean:
	$(call RM,$(BUILD))
	$(call RM,$(DIST))
	$(MAKE) -C userland clean