CC      := gcc
NASM    := nasm
LD      := ld
OBJCOPY := objcopy
QEMU    := qemu-system-x86_64

CFLAGS  := -m64 -ffreestanding -fno-pie -fno-stack-protector -Wall -Wextra -c
LDFLAGS := -m elf_x86_64 -nostdlib -Ttext=0x1000 -e _start

BUILD := build
OBJS  := $(BUILD)/kernel_entry.o \
         $(BUILD)/idt_asm.o \
         $(BUILD)/idt.o \
         $(BUILD)/pic.o \
         $(BUILD)/pit.o \
         $(BUILD)/keyboard.o \
         $(BUILD)/pmm.o \
         $(BUILD)/heap.o \
         $(BUILD)/kernel.o

all: $(BUILD)/aetheros.img

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/boot.bin: src/boot.asm | $(BUILD)
	$(NASM) -f bin $< -o $@

$(BUILD)/kernel_entry.o: src/kernel_entry.asm | $(BUILD)
	$(NASM) -f elf64 $< -o $@

$(BUILD)/idt_asm.o: src/idt_asm.asm | $(BUILD)
	$(NASM) -f elf64 $< -o $@

$(BUILD)/%.o: src/%.c | $(BUILD)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/kernel.elf: $(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@

$(BUILD)/kernel.bin: $(BUILD)/kernel.elf
	$(OBJCOPY) -O binary $< $@

$(BUILD)/aetheros.img: $(BUILD)/boot.bin $(BUILD)/kernel.bin
	dd if=/dev/zero of=$@ bs=512 count=2880 2>/dev/null
	dd if=$(BUILD)/boot.bin   of=$@ conv=notrunc 2>/dev/null
	dd if=$(BUILD)/kernel.bin of=$@ bs=512 seek=1 conv=notrunc 2>/dev/null

run: $(BUILD)/aetheros.img
	$(QEMU) -fda $< -boot a -display gtk

clean:
	rm -rf $(BUILD)

.PHONY: all run clean
