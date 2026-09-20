#!/bin/bash
# Compila programas de usuario y los mete en disk.img (FAT32)
set -e

cd "$(dirname "$0")"

if [ ! -f disk.img ]; then
    echo "[!] No existe disk.img. Creando..."
    dd if=/dev/zero of=disk.img bs=1M count=64 2>/dev/null
    mkfs.fat -F 32 disk.img > /dev/null
fi

for src in userspace/*.c; do
    name=$(basename "$src" .c)
    upper=$(echo "$name" | tr 'a-z' 'A-Z')
    echo "[*] Compilando $src -> $upper.BIN"

    gcc -m64 -ffreestanding -fno-pie -fno-stack-protector -nostdlib \
        -Ttext=0x500000 -e _start \
        -o "build/$name.elf" "$src"

    objcopy -O binary "build/$name.elf" "build/$name.bin"

    echo "[*] Copiando a disk.img como $upper.BIN"
    mcopy -o -i disk.img "build/$name.bin" "::$upper.BIN"
done

echo "[OK] Userspace actualizado en disk.img"
mdir -i disk.img ::
