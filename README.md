# AetherOS

Sistema operativo de 64 bits escrito desde cero, con el objetivo de
ejecutar archivos `.exe` de Windows de forma **nativa** (compatibilidad
binaria, sin capas de emulación ni Wine).

## Estado actual

- [x] Bootloader MBR (16 bits)
- [x] Modo protegido 32 bits
- [x] Long mode 64 bits
- [x] Paginación básica (32 MB)
- [x] IDT + excepciones
- [x] PIC remapeado
- [x] PIT (100 Hz)
- [x] Driver teclado PS/2
- [x] PMM (bitmap de páginas físicas)
- [x] Heap / kmalloc / kfree
- [ ] Multitarea (planificador)
- [ ] Syscalls + ring 3
- [ ] Sistema de archivos
- [ ] Cargador PE
- [ ] Subsistema NT (Object Manager, Executive)
- [ ] Compatibilidad .exe nativa

## Requisitos

- nasm
- gcc (con soporte -m64 -ffreestanding)
- ld (binutils)
- qemu-system-x86_64

## Compilar y ejecutar

    make
    make run

## Estructura

    src/
      boot.asm           bootloader MBR
      kernel_entry.asm   paso a modo protegido + long mode
      kernel.c           kernel principal
      idt.c / idt_asm.asm  tabla de interrupciones
      pic.c / pit.c      controladores de hardware
      keyboard.c         driver PS/2
      pmm.c              gestor de memoria física
      heap.c             kmalloc / kfree
