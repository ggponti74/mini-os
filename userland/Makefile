CC := gcc
AR := ar

GCC_INC := $(shell $(CC) -print-file-name=include)
CFLAGS := -m32 -ffreestanding -nostdinc -O2 -isystem $(GCC_INC) -I/path/to/newlib/include

SRCS := libsys/stubs.c
OBJS := $(SRCS:.c=.o)
TARGET := libsys/libsys.a

.PHONY: all clean

all: $(TARGET)

libsys/%.o: libsys/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(AR) rcs $@ $(OBJS)

clean:
	rm -f libsys/*.o libsys/*.a
	