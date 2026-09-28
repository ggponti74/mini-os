# Detect Operating System
ifeq ($(OS),Windows_NT)
    SHELL := cmd.exe
    MKDIR = if not exist $(call FIX_PATH,$(1)) mkdir $(call FIX_PATH,$(1))
    RM = if exist $(call FIX_PATH,$(1)) rmdir /s /q $(call FIX_PATH,$(1))
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

BOOT_SRC := boot/boot.asm
KERNEL_ENTRY_SRC := boot/kernel.asm

BOOT_BIN := $(BUILD)/boot.bin
KERNEL_ENTRY_OBJ := $(BUILD)/kernel_entry.o
KERNEL_ELF := $(BUILD)/kernel.elf
KERNEL_BIN := $(BUILD)/kernel.bin
IMAGE := $(DIST)/mini-os.img

# Automatically gather all C sources and ASM files in kernel/
C_SRCS := $(wildcard kernel/*.c)
C_OBJS := $(patsubst kernel/%.c,$(BUILD)/%.o,$(C_SRCS))

ASM_SRCS := $(wildcard kernel/*.asm)
ASM_OBJS := $(patsubst kernel/%.asm,$(BUILD)/%.o,$(ASM_SRCS))

NASM := nasm
CC := gcc
OBJCOPY := objcopy
CFLAGS := -m32 -ffreestanding -c -O2

.PHONY: all boot kernel image run clean

all: image

# Directory creation rules
$(BUILD):
	@$(call MKDIR,$(BUILD))

$(DIST):
	@$(call MKDIR,$(DIST))

# 1. Assemble 16-bit bootloader
boot: | $(BUILD)
	$(NASM) -f bin $(BOOT_SRC) -o $(BOOT_BIN)

# 2. Assemble Kernel Stage Entry (boot/kernel.asm)
$(KERNEL_ENTRY_OBJ): $(KERNEL_ENTRY_SRC) | $(BUILD)
	$(NASM) -f $(NASM_FMT) $(KERNEL_ENTRY_SRC) -o $(KERNEL_ENTRY_OBJ)

# 3. Assemble all kernel/*.asm files (e.g., interrupts.asm)
$(BUILD)/%.o: kernel/%.asm | $(BUILD)
	$(NASM) -f $(NASM_FMT) $< -o $@

# 4. Compile all kernel/*.c files (e.g., main.c, idt.c, pic.c, keyboard.c)
$(BUILD)/%.o: kernel/%.c | $(BUILD)
	$(CC) $(CFLAGS) $< -o $@

# If building on Linux / ELF32:
kernel: $(KERNEL_ENTRY_OBJ) $(ASM_OBJS) $(C_OBJS) | $(BUILD)
	$(CC) -m32 -nostdlib -Wl,-e,stage2_entry -Wl,-Ttext,0x7e00 -o $(KERNEL_ELF) $(KERNEL_ENTRY_OBJ) $(ASM_OBJS) $(C_OBJS)
	$(OBJCOPY) -O binary -j .text -j .data -j .rodata -j .bss $(KERNEL_ELF) $(KERNEL_BIN)
    
# 6. Concatenate boot sector and kernel binary, then pad image to 1.44MB
image: boot kernel | $(DIST)
	$(call CONCAT,$(BOOT_BIN),$(KERNEL_BIN),$(IMAGE))
	$(call PAD_IMAGE,$(IMAGE))

run: image
	$(QEMU) -drive file=$(IMAGE),format=raw,index=0,media=disk -nographic -audiodev id=audio0,driver=none -device sb16,audiodev=audio0

clean:
	$(call RM,$(BUILD))
	$(call RM,$(DIST))