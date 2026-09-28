# Detect Operating System
ifeq ($(OS),Windows_NT)
    SHELL := cmd.exe
    MKDIR = if not exist $(1) mkdir $(1)
    RM = if exist $(1) rmdir /s /q $(1)
    NASM_FMT = win32
    FIX_PATH = $(subst /,\,$(1))
    CONCAT = copy /b $(call FIX_PATH,$(1)) + $(call FIX_PATH,$(2)) $(call FIX_PATH,$(3))
    PAD_IMAGE = powershell -Command "$$f = [System.IO.File]::OpenWrite('$(1)'); $$f.SetLength(1474560); $$f.Close()"
	QEMU = qemu-system-x86_64
else
    SHELL := /bin/sh
    MKDIR = mkdir -p $(1)
    RM = rm -rf $(1)
    NASM_FMT = elf32
    FIX_PATH = $(1)
    CONCAT = cat $(1) $(2) > $(3)
    PAD_IMAGE = truncate -s 1474560 $(1)
	QEMU = qemu-system-x86_64
endif

BUILD := build
DIST := dist

C_SRCS := $(wildcard kernel/*.c)
C_OBJS := $(patsubst kernel/%.c,$(BUILD)/%.o,$(C_SRCS))


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

.PHONY: all boot kernel image run clean

all: image

# Ensure build directory rule exists
$(BUILD):
	mkdir -p $(BUILD)

# Ensure dist directory rule exists
$(DIST):
	mkdir -p $(DIST)
	
# Rule to compile any kernel C source file into an object file
$(BUILD)/%.o: kernel/%.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@


$(DIST):
	@$(call MKDIR,$(DIST))

# 1. Assemble 16-bit bootloader
boot: $(BUILD)
	$(NASM) -f bin $(BOOT_SRC) -o $(BOOT_BIN)

# 2. Assemble Stage 2 Entry (win32 for Windows COFF, elf32 for Linux ELF)
$(KERNEL_ENTRY_OBJ): $(KERNEL_ENTRY_SRC) | $(BUILD)
	$(NASM) -f $(NASM_FMT) $(KERNEL_ENTRY_SRC) -o $(KERNEL_ENTRY_OBJ)

# 3. Compile C kernel into 32-bit object file
$(KERNEL_C_OBJ): $(KERNEL_C_SRC) | $(BUILD)
	$(CC) $(CFLAGS) $(KERNEL_C_SRC) -o $(KERNEL_C_OBJ)

# Update the kernel linking rule to pass all C objects
kernel: $(KERNEL_ENTRY_OBJ) $(C_OBJS)
	$(CC) -m32 -nostdlib -T linker.ld -o $(KERNEL_ELF) $(KERNEL_ENTRY_OBJ) $(C_OBJS)
	$(OBJCOPY) -O binary $(KERNEL_ELF) $(KERNEL_BIN)
	
	
# 5. Concatenate boot sector and kernel binary, then pad image to 1.44MB
image: boot kernel $(DIST)
	$(call CONCAT,$(BOOT_BIN),$(KERNEL_BIN),$(IMAGE))
	$(call PAD_IMAGE,$(IMAGE))

run: image
	$(QEMU) -drive file=$(IMAGE),format=raw,index=0,media=disk -nographic -audiodev id=audio0,driver=none -device sb16,audiodev=audio0

clean:
	$(call RM,$(BUILD))
	$(call RM,$(DIST))