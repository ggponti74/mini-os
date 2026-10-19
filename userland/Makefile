CC := gcc
AR := ar
LD := ld

GCC_INC := $(shell $(CC) -print-file-name=include)
CFLAGS := -m32 -ffreestanding -fPIC -fno-stack-protector \
	-fno-builtin -nostdinc -O2 -isystem $(GCC_INC)

SRCS := libsys/stubs.c libsys/apps/editor.c
OBJS := $(SRCS:.c=.o)
TARGET := libsys/libsys.a
EDITOR_BIN := ../build/editor
EDITOR_OBJ_DIR := ../build/userland
EDITOR_OBJS := $(EDITOR_OBJ_DIR)/editor_start.o \
	$(EDITOR_OBJ_DIR)/editor_entry.o $(EDITOR_OBJ_DIR)/editor.o \
	$(EDITOR_OBJ_DIR)/editor_syscall.o

.PHONY: all clean editor

all: $(TARGET) editor

libsys/%.o: libsys/%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(AR) rcs $@ $(OBJS)

editor: $(EDITOR_BIN)

$(EDITOR_OBJ_DIR):
	mkdir -p $@

$(EDITOR_OBJ_DIR)/editor_entry.o: libsys/apps/editor_entry.c \
                                  libsys/apps/editor.h | $(EDITOR_OBJ_DIR)
	$(CC) $(CFLAGS) -Ilibsys/apps -c $< -o $@

$(EDITOR_OBJ_DIR)/editor_start.o: libsys/apps/editor_start.S | $(EDITOR_OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(EDITOR_OBJ_DIR)/editor.o: libsys/apps/editor.c \
                            libsys/apps/editor.h | $(EDITOR_OBJ_DIR)
	$(CC) $(CFLAGS) -Ilibsys/apps -c $< -o $@

$(EDITOR_OBJ_DIR)/editor_syscall.o: libsys/apps/editor_syscall.S \
                                    | $(EDITOR_OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(EDITOR_BIN): $(EDITOR_OBJS) flat.ld | ../build
	$(LD) -m elf_i386 --defsym=_GLOBAL_OFFSET_TABLE_=0 -T flat.ld \
		--oformat binary \
		-o $@ $(EDITOR_OBJS)

../build:
	mkdir -p $@

clean:
	rm -f $(OBJS) $(TARGET) $(EDITOR_OBJS) $(EDITOR_BIN)
	