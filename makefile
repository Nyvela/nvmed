include nvmed.conf

ifndef USERLIB_PATH
$(error USERLIB_PATH is not defined (see nvmed.conf))
endif

CC := gcc
LD := ld
OBJCOPY := objcopy

BUILD := build

TARGET := $(BUILD)/nvmed
ELF := $(TARGET).elf
BIN := $(TARGET).bin

CFLAGS := -ffreestanding -m64 -mno-red-zone \
          -fno-stack-protector -fno-pie -fno-pic \
          -ffunction-sections -fdata-sections \
          -Wall -Wextra \
          -Iinclude -I$(USERLIB_PATH)

LDFLAGS := -nostdlib -static -Wl,--gc-sections -T linker.ld
SRC := $(shell find src -type f -name '*.c')

.PHONY: all clean

all: $(ELF) $(BIN)

$(ELF): $(SRC) linker.ld
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(LDFLAGS) $(SRC) -o $@

$(BIN): $(ELF)
	$(OBJCOPY) -O binary $< $@

clean:
	rm -rf $(BUILD)
