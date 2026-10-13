include nvmed.conf

ifndef USERLIB_PATH
$(error USERLIB_PATH is not defined (see nvmed.conf))
endif

ifndef NPCID_LIB_PATH
$(error NPCID_LIB_PATH is not defined (see nvmed.conf))
endif

ifndef NYVD_LIB_PATH
$(error NYVD_LIB_PATH is not defined (see nvmed.conf))
endif

CC := gcc
LD := ld
OBJCOPY := objcopy

BUILD := build

TARGET := $(BUILD)/nvmed
ELF := $(TARGET).elf
BIN := $(TARGET).bin

CFLAGS := -ffreestanding -m64 -mno-red-zone \
          -mgeneral-regs-only -mpreferred-stack-boundary=3 \
          -fno-stack-protector -fno-pie -fno-pic \
          -ffunction-sections -fdata-sections \
          -Wall -Wextra \
          -Iinclude -I$(USERLIB_PATH) -I$(NPCID_LIB_PATH) -I$(NYVD_LIB_PATH)

LDFLAGS := -nostdlib -static -Wl,--gc-sections -T linker.ld
SRC := $(shell find src -type f -name '*.c')

.PHONY: all clean

all: $(ELF) $(BIN)

$(ELF): $(SRC) linker.ld
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(LDFLAGS) $(SRC) -o $@

$(BIN): $(ELF)
	$(OBJCOPY) -O binary --set-section-flags .bss=alloc,load,contents $< $@

clean:
	rm -rf $(BUILD)
