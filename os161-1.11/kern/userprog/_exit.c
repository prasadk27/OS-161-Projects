#include <types.h>
#include <kern/errno.h>
#include <kern/unistd.h>
#include <thread.h>
#include <curthread.h>
#include <lib.h>

void sys__exit(int exitcode){
    

	curthread->exit_status = exitcode;
	thread_exit();
}



