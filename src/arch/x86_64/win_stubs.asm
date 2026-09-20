[BITS 64]

; Los exports de kernel32 son stubs en user space que hacen int 0x81.
; Convencion Windows x64:
;   arg1 = RCX, arg2 = RDX, arg3 = R8, arg4 = R9
;   numero de syscall en RAX
;   int 0x81
;   return en RAX

global stub_GetStdHandle
global stub_WriteFile
global stub_ReadFile
global stub_ExitProcess
global stub_Sleep

%macro WINSTUB 2
global stub_%1
stub_%1:
    mov rax, %2
    int 0x81
    ret
%endmacro

WINSTUB GetStdHandle, 0x1004
WINSTUB WriteFile,    0x1001
WINSTUB ReadFile,     0x1002
WINSTUB ExitProcess,  0x1003
WINSTUB Sleep,        0x1006
