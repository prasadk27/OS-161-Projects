#ifndef PROCESS_SYSCALL_H
#define PROCESS_SYSCALL_H

#include <types.h>
#include <thread.h>
#include <kern/limits.h>

#define MIN_PID 100
pid_t new_pid();
void pid_process_exit(pid_t x);
void pid_parent_done(pid_t x);
void pid_free(pid_t x);
int pid_claimed(pid_t x);


pid_t sys___getpid(void);

void sys___exit(int);

int sys___waitpid(pid_t PID, userptr_t *status, int options);

pid_t sys___fork(struct trapframe *tf);

int sys_execv(char *progname, char ** args);



#endif