#include <types.h>
#include <kern/unistd.h>
#include <kern/errno.h>
#include <lib.h>
#include <addrspace.h>
#include <thread.h>
#include <curthread.h>
#include <vm.h>
#include <vfs.h>
#include <test.h>

int sys_execv(char *progname, char ** args) {

    assert(curthread->t_vmspace != NULL);


    if (!(as_valid_read_addr(curthread->t_vmspace, (vaddr_t*) progname) && as_valid_read_addr(curthread->t_vmspace, (vaddr_t*) args))) {
        return EFAULT;
    }
    if (strlen(progname) < 1) {
        return EINVAL;
    }



    int nargs = 0;
    int i = 0;
    while (args[i] != NULL) {
        nargs++;
        i++;
    }

    
    if (nargs > 16) { 
        return E2BIG;
    }

    
    char* kern_progname = kmalloc(sizeof (char) * (strlen(progname) + 1));
    
    copyinstr((const_userptr_t) progname, kern_progname, (strlen(progname) + 1), NULL);

    
    char** kern_argumentPointer = kmalloc(sizeof (char*) * nargs);
    
    for (i = 0; i < nargs; i++) {
        if (!as_valid_read_addr(curthread->t_vmspace, (vaddr_t*) args[i])) {
            return EFAULT;
        }
        if (strlen(args[i]) < 1) {
            return EINVAL;
        }
        int arg_size = strlen(args[i]);
        kern_argumentPointer[i] = kmalloc(sizeof (char) * (arg_size + 1));
        copyinstr((const_userptr_t) args[i], kern_argumentPointer[i], (arg_size + 1), NULL);
    }

    vaddr_t entrypoint, stackptr;
    int result;


    DEBUG(DB_EXEC, "EXECV[%d] Before AS_DESTROY - progname: [%s] - Address space: [%d]\n", (int) curthread->pid, kern_progname, (int) curthread->t_vmspace);
    
    as_destroy(curthread->t_vmspace);
    curthread->t_vmspace = NULL;
        assert(curthread->t_vmspace == NULL);
    
    curthread->t_vmspace = as_create();
    if (curthread->t_vmspace == NULL) {

        return ENOMEM;
    }

    
    as_activate(curthread->t_vmspace);

    assert(curthread->t_vmspace != NULL);

    
    result = load_elf(kern_progname, &entrypoint);
    if (result) {
        
        return result;
    }
    


    
    result = as_define_stack(curthread->t_vmspace, &stackptr);
    if (result) {
        return result;
    }

    
    int stackFrameSize = 8;
    for (i = 0; i < nargs; i++) {
        stackFrameSize += strlen(kern_argumentPointer[i]) + 1 + 4; 
    }

    
    stackptr -= stackFrameSize;
    
    for (; (stackptr % 8) > 0; stackptr--) {
    }

    
    int argumentStringLocation = (int) stackptr + 4 + ((nargs + 1) * 4); 
    copyout((void *) & nargs, (userptr_t) stackptr, (size_t) 4); 

    for (i = 0; i < nargs; i++) {
        copyout((void *) & argumentStringLocation, (userptr_t) (stackptr + 4 + (4 * i)), (size_t) 4); 
        copyoutstr(kern_argumentPointer[i], (userptr_t) argumentStringLocation, (size_t) strlen(kern_argumentPointer[i]), NULL); 
        argumentStringLocation += strlen(kern_argumentPointer[i]) + 1; 
    }
    int *nullValue = NULL;
    copyout((void *) & nullValue, (userptr_t) (stackptr + 4 + (4 * i)), (size_t) 4);

    kfree(kern_progname);

    
    for (i = 0; i < nargs; i++) {
        kfree(kern_argumentPointer[i]);
    }
    kfree(kern_argumentPointer);

   
    panic("md_usermode returned\n");
    return EINVAL;
}

