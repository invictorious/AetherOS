# AetherOS

Sistema operativo de 64 bits escrito desde cero, con el objetivo de
ejecutar archivos `.exe` de Windows de forma **nativa** (compatibilidad
binaria, sin capas de emulación ni Wine).

## Estado actual (v0.8.0)

- [x] Bootloader MBR + E820
- [x] Modo protegido 32 bits
- [x] Long mode 64 bits
- [x] Paginacion (identity + higher-half)
- [x] IDT + excepciones + trap gates
- [x] PIC remapeado
- [x] PIT (100 Hz)
- [x] Driver teclado PS/2
- [x] PMM (bitmap de paginas fisicas)
- [x] Heap / kmalloc / kfree
- [x] Multitarea preemptiva
- [x] Syscalls (int 0x80)
- [x] Ring 3 (modo usuario)
- [x] Driver ATA PIO
- [x] FAT32 read-only
- [x] Shell interactivo
- [x] Cargador ELF64
- [x] Win64 ABI (int 0x81)
- [x] Cargador PE (.exe de Windows)
- [x] Relocations PE
- [x] kernel32 + msvcrt (31 funciones)
- [x] user32 + MessageBoxA
- [x] **Framebuffer VESA 1024x768x32**
- [x] **Console sobre pixeles (fuente 8x16)**
- [ ] Ventanas reales (GUI)
- [ ] Dibujo GDI (Rectangle, Ellipse...)
- [ ] DLLs dinamicas
- [ ] Subsystema NT completo

## Requisitos

- nasm
- gcc (con soporte -m64 -ffreestanding)
- ld (binutils)
- qemu-system-x86_64
- mingw-w64 (para compilar .exe de prueba)

## Compilar y ejecutar

    make
    make run

## Estructura

    src/
      arch/x86_64/       bootloader, modo protegido, IDT, GDT, long mode
      kernel/            kernel principal, procesos, scheduler, console
      mm/                PMM + heap
      drivers/           pic, pit, keyboard, ATA, framebuffer
      fs/                FAT32
      win/               PE loader, kernel32, msvcrt, user32
    userspace/           programas de prueba (compilados con MinGW)
