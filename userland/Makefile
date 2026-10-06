# userland/libsys/Makefile

CC := gcc
AR := ar
# userland/libsys/Makefile

# Add GCC's internal header directory to CFLAGS
GCC_INC := $(shell $(CC) -print-file-name=include)
CFLAGS := -m32 -ffreestanding -nostdinc -O2 -isystem $(GCC_INC) -I../kernel

SRCS := libsys/stubs.c
OBJS := $(SRCS:.c=.o)
TARGET := libsys.a

all: $(TARGET)

$(BUILD)/%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(AR) rcs $@ $(OBJS)

clean:
	rm -f *.o *.a

.PHONY: all clean