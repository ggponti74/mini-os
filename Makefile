BUILD := build
DIST := dist
ISO_ROOT := $(BUILD)/iso
KERNEL := $(BUILD)/kernel.bin
ISO := $(DIST)/mini-os.iso

NASM := nasm
LD := ld
GRUB_MKRESCUE := grub-mkrescue
QEMU := qemu-system-i386

NASM_FLAGS := -f elf32 -g -F dwarf
LD_FLAGS := -m elf_i386 -T linker.ld -nostdlib

.PHONY: all clean kernel iso smoke-test

all: iso

$(BUILD):
	mkdir -p $(BUILD)

$(DIST):
	mkdir -p $(DIST)

kernel: $(BUILD)
	$(NASM) $(NASM_FLAGS) kernel/kernel.asm -o $(BUILD)/kernel.o
	$(LD) $(LD_FLAGS) -o $(KERNEL) $(BUILD)/kernel.o

iso: kernel $(DIST)
	rm -rf $(ISO_ROOT)
	mkdir -p $(ISO_ROOT)/boot/grub
	cp $(KERNEL) $(ISO_ROOT)/boot/kernel.bin
	cp iso/boot/grub/grub.cfg $(ISO_ROOT)/boot/grub/grub.cfg
	$(GRUB_MKRESCUE) -o $(ISO) $(ISO_ROOT)

smoke-test: iso
	timeout 15s $(QEMU) \
		-cdrom $(ISO) \
		-display none \
		-monitor none \
		-serial stdio \
		-no-reboot \
		-no-shutdown \
		-d \
		int,cpu_reset \
		-D $(BUILD)/qemu.log \
		2>&1 | tee $(BUILD)/serial.log || test $$? -eq 124

clean:
	rm -rf $(BUILD) $(DIST)