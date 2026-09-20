[BITS 64]

; Todos los stubs son triviales: cargan el numero de syscall en RAX
; y hacen int 0x81. Los argumentos ya estan en RCX, RDX, R8, R9.

%macro WINSTUB 2
global stub_%1
stub_%1:
    mov rax, %2
    int 0x81
    ret
%endmacro

; ---- kernel32 ----
WINSTUB GetStdHandle,   0x1004
WINSTUB WriteFile,      0x1001
WINSTUB ReadFile,       0x1002
WINSTUB ExitProcess,    0x1003
WINSTUB Sleep,          0x1006
WINSTUB GetLastError,   0x1007
WINSTUB SetLastError,   0x1008
WINSTUB GetTickCount,   0x1009
WINSTUB VirtualAlloc,   0x100A
WINSTUB VirtualFree,    0x100B
WINSTUB GetProcessHeap, 0x100C
WINSTUB CloseHandle,    0x100D
WINSTUB GetCurrentProcessId, 0x100E
WINSTUB GetCommandLineA, 0x1005
WINSTUB CreateFileA,       0x100F
WINSTUB WriteConsoleA,     0x1010
WINSTUB ReadConsoleA,      0x1011
WINSTUB SetConsoleTitleA,  0x1012
WINSTUB GetModuleHandleA,  0x1013
WINSTUB GetProcAddress,    0x1014
WINSTUB LoadLibraryA,      0x1015
WINSTUB FreeLibrary,       0x1016
WINSTUB WriteConsoleW,     0x1017

; ---- msvcrt ----
WINSTUB strlen,         0x2001
WINSTUB strcmp,         0x2002
WINSTUB strcpy,         0x2003
WINSTUB memcpy,         0x2004
WINSTUB memset,         0x2005
WINSTUB puts,           0x2006
WINSTUB malloc,         0x2007
WINSTUB free,           0x2008
