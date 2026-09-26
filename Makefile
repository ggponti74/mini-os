BUILD := build
DIST := dist

BOOT_SRC := boot/boot.asm
BOOT_BIN := $(BUILD)/boot.bin
IMAGE := $(DIST)/mini-os.img

NASM := nasm
QEMU := qemu-system-i386

.PHONY: all boot image run clean

all: image

$(BUILD):
	mkdir -p $(BUILD)

$(DIST):
	mkdir -p $(DIST)

boot: $(BUILD)
	$(NASM) -f bin $(BOOT_SRC) -o $(BOOT_BIN)

image: boot $(DIST)
	cp $(BOOT_BIN) $(IMAGE)

run: image
	$(QEMU) -drive format=raw,file=$(IMAGE)

clean:
	rm -rf $(BUILD) $(DIST)
	