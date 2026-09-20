#ifndef WIN_ABI_H
#define WIN_ABI_H

#include <stdint.h>

/* Convencion Windows x64:
 *   arg1 = RCX
 *   arg2 = RDX
 *   arg3 = R8
 *   arg4 = R9
 *   resto en pila
 *   retorno = RAX
 *
 * Usamos int 0x81 como puerta de entrada (equivalente al 'syscall'
 * de Windows real, que requiere MSR_LSTAR y sysretq).
 */

/* Numeros de syscall estilo NT (base 0x1000 para no chocar con los nuestros) */
#define WIN_SYS_WRITEFILE       0x1001
#define WIN_SYS_READFILE        0x1002
#define WIN_SYS_EXITPROCESS     0x1003
#define WIN_SYS_GETSTDHANDLE    0x1004
#define WIN_SYS_GETCMDLINEA     0x1005
#define WIN_SYS_SLEEP           0x1006

/* Handles estandar de Windows */
#define STD_INPUT_HANDLE   ((uint64_t)-10)
#define STD_OUTPUT_HANDLE  ((uint64_t)-11)
#define STD_ERROR_HANDLE   ((uint64_t)-12)

/* Prototipos con la convencion Windows x64 (los ponemos a mano en ASM de todos modos,
 * esto es solo documentacion) */
typedef void *HANDLE;
typedef uint32_t DWORD;
typedef int BOOL;

uint64_t win_dispatch(uint64_t nr, uint64_t a1, uint64_t a2,
                      uint64_t a3, uint64_t a4);
void win_init(void);

#endif
