CC := gcc
AR := ar
LD := ld
OBJCOPY := objcopy

ifeq ($(OS),Windows_NT)
    FIX_PATH = $(subst /,\,$(1))
    MKDIR_P = if not exist $(call FIX_PATH,$(1)) mkdir $(call FIX_PATH,$(1))
    WIN_CFLAGS = -fno-leading-underscore -mno-stack-arg-probe
else
    MKDIR_P = mkdir -p $(1)
    WIN_CFLAGS =
endif

GCC_INC := $(shell $(CC) -print-file-name=include)
CFLAGS := -m32 -ffreestanding -fPIC -fno-stack-protector \
	-fno-builtin -O2 $(WIN_CFLAGS)

SRCS := libsys/stubs.c libsys/apps/editor.c
OBJS := $(SRCS:.c=.o)
TARGET := libsys/libsys.a
EDIT_BIN := ../build/edit
EDITOR_OBJ_DIR := ../build/userland
EDITOR_OBJS := $(EDITOR_OBJ_DIR)/editor_start.o \
	$(EDITOR_OBJ_DIR)/editor_entry.o $(EDITOR_OBJ_DIR)/editor.o \
	$(EDITOR_OBJ_DIR)/editor_syscall.o

ifeq ($(OS),Windows_NT)
    CLEAN_FILES = -del /q $(call FIX_PATH,$(OBJS) $(TARGET) $(EDITOR_OBJS) $(EDIT_BIN) $(EDITOR_OBJ_DIR)\edit.elf) >nul 2>&1
else
    CLEAN_FILES = rm -f $(OBJS) $(TARGET) $(EDITOR_OBJS) $(EDIT_BIN) $(EDITOR_OBJ_DIR)/edit.elf
endif

.PHONY: all clean edit

all: $(TARGET) edit

libsys/%.o: libsys/%.c
	@$(call MKDIR_P,$(dir $@))
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(AR) rcs $@ $(OBJS)

edit: $(EDIT_BIN)

$(EDITOR_OBJ_DIR):
	@$(call MKDIR_P,$@)

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

ifeq ($(OS),Windows_NT)
$(EDIT_BIN): $(EDITOR_OBJS) flat.ld | ../build
	$(CC) -m32 -nostdlib -Wl,--defsym=_GLOBAL_OFFSET_TABLE_=0 -Wl,-T,flat.ld -o $(EDITOR_OBJ_DIR)/edit.elf $(EDITOR_OBJS)
	$(OBJCOPY) -O binary $(EDITOR_OBJ_DIR)/edit.elf $@
else
$(EDIT_BIN): $(EDITOR_OBJS) flat.ld | ../build
	$(LD) -m elf_i386 --defsym=_GLOBAL_OFFSET_TABLE_=0 -T flat.ld \
		--oformat binary \
		-o $@ $(EDITOR_OBJS)
endif

../build:
	@$(call MKDIR_P,$@)

clean:
	$(CLEAN_FILES)
	