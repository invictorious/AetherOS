CC      := gcc
NASM    := nasm
LD      := ld
OBJCOPY := objcopy
QEMU    := qemu-system-x86_64

CFLAGS  := -m64 -ffreestanding -fno-pie -fno-stack-protector -Wall -Wextra -c
LDFLAGS := -m elf_x86_64 -nostdlib -Ttext=0x10000 -e _start

BUILD := build

ARCH_ASM := src/arch/x86_64/kernel_entry.asm \
            src/arch/x86_64/idt_asm.asm \
            src/arch/x86_64/switch.asm \
            src/arch/x86_64/gdt_asm.asm \
            src/arch/x86_64/usermode.asm

BOOT_ASM := src/arch/x86_64/boot.asm

C_SRCS   := src/arch/x86_64/idt.c \
            src/arch/x86_64/gdt.c \
            src/kernel/kernel.c \
            src/kernel/thread.c \
            src/kernel/scheduler.c \
            src/kernel/syscall.c \
            src/kernel/user_program.c \
            src/mm/pmm.c \
            src/mm/heap.c \
            src/drivers/pic.c \
            src/drivers/pit.c \
            src/drivers/keyboard.c

ARCH_OBJS := $(patsubst src/arch/x86_64/%.asm,$(BUILD)/%.o,$(ARCH_ASM))
C_OBJS    := $(patsubst src/%.c,$(BUILD)/%.o,$(C_SRCS))
OBJS      := $(ARCH_OBJS) $(C_OBJS)

all: $(BUILD)/aetheros.img

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/boot.bin: $(BOOT_ASM) | $(BUILD)
	$(NASM) -f bin $< -o $@

$(BUILD)/%.o: src/arch/x86_64/%.asm | $(BUILD)
	$(NASM) -f elf64 $< -o $@

$(BUILD)/%.o: src/%.c | $(BUILD)
	@mkdir -p $(dir $@)
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

# --- Target de depuracion: ejecuta QEMU con log y muestra resultado ---
debug: $(BUILD)/aetheros.img
	@echo ""
	@echo "=== Ejecutando QEMU con log (5 segundos)... ==="
	@timeout 5 $(QEMU) -fda $< -boot a -display gtk -no-reboot -d int,cpu_reset -D /tmp/qemu.log || true
	@echo ""
	@echo "=== Ultimas 60 lineas del log ==="
	@tail -60 /tmp/qemu.log
	@echo ""
	@echo "=== Si ves 'Triple fault', el fallo esta arriba ==="
