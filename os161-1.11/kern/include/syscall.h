#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#include <types.h>
#include <machine/trapframe.h>

/*
 * Prototypes for IN-KERNEL entry points for system call implementations.
 */

int sys_reboot(int code);

void sys__exit(int exitcode);
int sys_write(int *retval, int filehandle, const void *buf, size_t size);

int sys_execv(char *progname, char ** args);





#endif /* _SYSCALL_H_ */
