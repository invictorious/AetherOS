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

; ---- user32 ----
WINSTUB MessageBoxA,       0x3001
WINSTUB MessageBoxW,       0x3002
WINSTUB MessageBeep,       0x3003
WINSTUB ShowWindow,        0x3005
WINSTUB DestroyWindow,     0x3006
WINSTUB SetWindowTextA,    0x3007
WINSTUB GetMessageA,       0x3008
WINSTUB DefWindowProcA,    0x3009
WINSTUB GetDC,             0x300A
WINSTUB ReleaseDC,         0x300B
WINSTUB TextOutA,          0x300C
WINSTUB UpdateWindow,      0x300D

; ---- gdi32 ----
WINSTUB SetPixel,          0x4001
WINSTUB MoveToEx,          0x4002
WINSTUB LineTo,            0x4003
WINSTUB FillRect,          0x4006
WINSTUB SetTextColor,      0x4007
WINSTUB SetBkColor,        0x4008
WINSTUB CreateSolidBrush,  0x4009
WINSTUB DeleteObject,      0x400A
WINSTUB FillSolidRect,     0x400B

; ---- msvcrt ----
WINSTUB strlen,         0x2001
WINSTUB strcmp,         0x2002
WINSTUB strcpy,         0x2003
WINSTUB memcpy,         0x2004
WINSTUB memset,         0x2005
WINSTUB puts,           0x2006
WINSTUB malloc,         0x2007
WINSTUB free,           0x2008


; --- CreateWindowExA custom ---
; Recibe 12 args segun el ABI Windows x64:
;   args 1-4 en RCX, RDX, R8, R9
;   args 5-8 en la pila + shadow space de 32 bytes
;   args 9-12 en la pila (mas arriba)
;
; Queremos extraer X, Y, W, H (args 5-8) y pasarselos al kernel
; como si fueran args 1-4 (RCX, RDX, R8, R9).
;
; Layout de la pila al entrar:
;   [rsp + 0]   return address
;   [rsp + 8]   shadow[0]
;   [rsp + 16]  shadow[1]
;   [rsp + 24]  shadow[2]
;   [rsp + 32]  shadow[3]
;   [rsp + 40]  arg5 = X
;   [rsp + 48]  arg6 = Y
;   [rsp + 56]  arg7 = W
;   [rsp + 64]  arg8 = H
global stub_CreateWindowExA
stub_CreateWindowExA:
    mov rcx, [rsp + 40]    ; X
    mov rdx, [rsp + 48]    ; Y
    mov r8,  [rsp + 56]    ; W
    mov r9,  [rsp + 64]    ; H
    mov rax, 0x3004        ; WIN_CREATEWINDOWEXA
    int 0x81
    ret


; --- Rectangle custom (5 args: hdc, l, t, r, b) ---
; El 5o argumento (bottom) va en la pila con shadow space
global stub_Rectangle
stub_Rectangle:
    ; [rsp+0]  return
    ; [rsp+8..32] shadow
    ; [rsp+40] bottom
    ; Pasamos al kernel: hdc, l, t, r (4 args) + bottom
    ; El kernel recibira en RCX, RDX, R8, R9 = hdc, l, t, r
    ; Y el bottom se lo pasamos codificado en el numero de syscall + 0x100
    ; ... Demasiado complejo. Mejor: leer bottom y guardar en R10
    mov r10, [rsp + 40]        ; bottom
    mov rax, 0x4004
    int 0x81
    ret

; --- Ellipse custom (5 args: hdc, l, t, r, b) ---
global stub_Ellipse
stub_Ellipse:
    mov r10, [rsp + 40]
    mov rax, 0x4005
    int 0x81
    ret
