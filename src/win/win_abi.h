#ifndef WIN_ABI_H
#define WIN_ABI_H

#include <stdint.h>

/* ------------------------------------------------------------
 * Syscalls Win32 (base 0x1000)
 * ------------------------------------------------------------ */
#define WIN_WRITEFILE          0x1001
#define WIN_READFILE           0x1002
#define WIN_EXITPROCESS        0x1003
#define WIN_GETSTDHANDLE       0x1004
#define WIN_GETCMDLINEA        0x1005
#define WIN_SLEEP              0x1006
#define WIN_GETLASTERROR       0x1007
#define WIN_SETLASTERROR       0x1008
#define WIN_GETTICKCOUNT       0x1009
#define WIN_VIRTUALALLOC       0x100A
#define WIN_VIRTUALFREE        0x100B
#define WIN_GETPROCESSHEAP     0x100C
#define WIN_CLOSEHANDLE        0x100D
#define WIN_GETPID             0x100E

/* ------------------------------------------------------------
 * Syscalls msvcrt (base 0x2000) - puros, pero van por syscall
 * para mantener todo uniforme
 * ------------------------------------------------------------ */
#define WIN_STRLEN             0x2001
#define WIN_STRCMP             0x2002
#define WIN_STRCPY             0x2003
#define WIN_MEMCPY             0x2004
#define WIN_MEMSET             0x2005
#define WIN_PUTS               0x2006
#define WIN_MALLOC             0x2007
#define WIN_FREE               0x2008

/* Handles estandar */
#define STD_INPUT_HANDLE   ((uint64_t)-10)
#define STD_OUTPUT_HANDLE  ((uint64_t)-11)
#define STD_ERROR_HANDLE   ((uint64_t)-12)

void     win_init(void);
uint64_t win_dispatch(uint64_t nr, uint64_t a1, uint64_t a2,
                      uint64_t a3, uint64_t a4);

#endif
