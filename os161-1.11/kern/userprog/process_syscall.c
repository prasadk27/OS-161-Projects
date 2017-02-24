
#include <types.h>
#include <clock.h>
#include <lib.h>
#include <kern/limits.h>
#include <thread.h>
#include <curthread.h>
#include <synch.h>
#include <process_syscall.h>
#include <kern/errno.h>
#include <addrspace.h>
#include <vm.h>
#include <vfs.h>
#include <syscall.h>
#include <test.h>
#include <machine/trapframe.h>
#include <machine/pcb.h>
#include <machine/spl.h>
#include <child_table.h>
#include <kern/unistd.h>

#define PID_FREE   0 //the process can be recycled
#define PID_PARENT 1 //the process has not exited, but the parent has exited
#define PID_EXITED 2 //the process has exited, but the parent has not exited or waited on this pid
#define PID_NEW    3 //neither the process nor its parent have exited

struct pid_clist {
    pid_t pid;
    int status;
    struct pid_clist *next;
};

struct pid_list {
    pid_t pid;
    struct pid_list *next;
};

unsigned int unused_pids = MIN_PID;
struct pid_list *recycled_pids;
struct pid_clist *unavailable_pids;


pid_t new_pid() {
    int spl = splhigh();
    if (recycled_pids == NULL) {
        assert(unused_pids < 0x7FFFFFFF); //can't even happen with sys161's available memory
        struct pid_clist *new_entry = kmalloc(sizeof(struct pid_clist));
        new_entry->pid = unused_pids;
        new_entry->status = PID_NEW;
        new_entry->next = unavailable_pids;
        unavailable_pids = new_entry;
        unused_pids += 1;
        splx(spl);

        return (unused_pids - 1);
    } else {
        struct pid_list *first = recycled_pids;
        recycled_pids = recycled_pids->next;
        struct pid_clist *new_entry = kmalloc(sizeof(struct pid_clist));
        new_entry->pid = first->pid;
        new_entry->status = PID_NEW;
        new_entry->next = unavailable_pids;
        unavailable_pids = new_entry;
        kfree(first);
        splx(spl);
        
        return (new_entry->pid);
    }
}

void pid_change_status(pid_t x, int and_mask) {
    int spl = splhigh();
    
    assert(unavailable_pids != NULL);
    if (unavailable_pids->pid == x) {
        unavailable_pids->status &= and_mask;
        
        if (unavailable_pids->status == PID_FREE) {
            //add pid to recycled_pids list
            struct pid_list *new_entry = kmalloc(sizeof(struct pid_list));
            new_entry->pid = x;
            new_entry->next = recycled_pids;
            recycled_pids = new_entry;
            //remove pid from unavailable_pids list
            struct pid_clist *temp = unavailable_pids;
            unavailable_pids = unavailable_pids->next;
            kfree(temp);
            
        }
    } else {
        int found = 0;
        struct pid_clist *p = unavailable_pids;
        while (p->next != NULL) {
            if (p->next->pid == x) {
                found = 1;
                p->next->status &= and_mask;
                
                if (p->next->status == PID_FREE) {
                    //add pid to recycled_pids list
                    struct pid_list *new_entry = kmalloc(sizeof(struct pid_list));
                    new_entry->pid = x;
                    new_entry->next = recycled_pids;
                    recycled_pids = new_entry;
                    //remove pid from unavailable_pids list
                    struct pid_clist *temp = p->next;
                    p->next = p->next->next;
                    kfree(temp);
                    
                }
            }
            p = p->next;
        }
        assert(found);
    }
    splx(spl);
}


void pid_parent_done(pid_t x) {
    pid_change_status(x, PID_PARENT);
}


void pid_process_exit(pid_t x) {
    pid_change_status(x, PID_EXITED);
}


void pid_free(pid_t x) {
    pid_change_status(x, PID_FREE);
}


int pid_claimed(pid_t x) {
    int spl = splhigh();
    struct pid_clist *p;
    for (p = unavailable_pids; p != NULL; p = p->next) {
        if (p->pid == x) {
            splx(spl);
            return 1;
        }
    }
    splx(spl);
    return 0;
}

void
sys___exit(int exitcode)
{
	curthread->exit_status = exitcode;
	thread_exit();
}

pid_t
sys___getpid()
{
	return curthread->pid;
}

int sys___waitpid(pid_t PID, userptr_t *status, int options)
{
	if (as_valid_write_addr(curthread->t_vmspace, (vaddr_t *) status) == 0) {
        return EFAULT;
    }
    if (((int) status) % 4 != 0) {
        return EFAULT;
    }
    int spl = splhigh();
    if (options != 0) {
        
        splx(spl);
        return EINVAL;
    }
    struct child_table *child = NULL;
    struct child_table *p;
    for (p = curthread->children; p != NULL; p = p->next) {
        if (p->pid == PID) {
            child = p;
            break;
        }
    }
    if (child == NULL) { 
        
        splx(spl);
        if (pid_claimed(PID)) {
            return ESRCH; //do not have permission to wait on that pid
        } else {
            return EINVAL; //not a valid pid
        }
    }
    
    
    while (child->finished == 0) {
        thread_sleep((void *) PID);
    }
    
    *status = child->exit_code;
    
    //Adding the child from children list since it has exited and it's PID is no longer needed
    if (curthread->children->pid == PID) {
        struct child_table *temp = curthread->children;
        curthread->children = curthread->children->next;
        kfree(temp);
    } else {
        for (p = curthread->children;; p = p->next) {
            assert(p->next != NULL);
            if (p->next->pid == PID) {
                struct child_table *temp = p->next;
                p->next = p->next->next;
                kfree(temp);
                break;
            }
        }
    }
    
    pid_parent_done(PID);
    splx(spl);
    
    return PID;
}

pid_t sys___fork(struct trapframe *tf) {
    int spl = splhigh();
    char *child_name = kmalloc(sizeof(char) * (strlen(curthread->t_name)+9));
    if (child_name == NULL) {
        
        splx(spl);
        return ENOMEM;
    }
    child_name = strcpy(child_name, curthread->t_name);
    struct child_table *new_child = kmalloc(sizeof(struct child_table));
    if (new_child == NULL) {
        
        splx(spl);
        return ENOMEM;
    }
    struct thread *child = NULL;

    void (*func_pt)(void *, unsigned long) = &md_forkentry;
    
    int result = thread_fork(strcat(child_name, "'s child"), tf, 0, func_pt, &child);
   
    if (result != 0) {
        kfree(new_child);
        
        splx(spl);
        return result;
    }  
    
    child->parent = curthread;
    //adding new process to list of children
    new_child->pid = child->pid;
    new_child->finished = 0;
    new_child->exit_code = -2; 
    new_child->next = curthread->children;
    curthread->children = new_child;
    
    
    int err = as_copy(curthread->t_vmspace, &child->t_vmspace); 
    if (err != 0) {
        child->t_vmspace = NULL;
        child->parent = NULL; 
        md_initpcb(&child->t_pcb, child->t_stack, 0, 0, &thread_exit); //set new thread to delete itself
        splx(spl);
        return err;
    }
    
    
    
    int retval = child->pid;
    splx(spl);
    return retval; 
}


int sys_execv(char *progname, char ** args) 
{    
    assert(curthread->t_vmspace != NULL);

    //check the arguments are valid pointers
    if (!(as_valid_read_addr(curthread->t_vmspace, (vaddr_t*) progname) && as_valid_read_addr(curthread->t_vmspace, (vaddr_t*) args))) 
    {
        return EFAULT;
    }
    if (strlen(progname) < 1) 
    {
        return EINVAL;
    }

    /* Copying the programname and args into kernel space */
    //Checking the size of the args
    int nargs = 0;
    int i = 0;
    while (args[i] != NULL) 
    {
        nargs++;
        i++;
    }

    
    if (nargs > 16) { //The value 16 is choosen as the menu can take a maximum of 16 arguments
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

    struct vnode *v;
    vaddr_t entrypoint, stackptr;
    int result;

    
    result = vfs_open(kern_progname, O_RDONLY, &v);
    if (result) {
        return result;
    }



    
    
    as_destroy(curthread->t_vmspace);
    curthread->t_vmspace = NULL;
    
    assert(curthread->t_vmspace == NULL);
    
    /* Creating a new address space. */
    curthread->t_vmspace = as_create();
    if (curthread->t_vmspace == NULL) 
    {
        vfs_close(v);
        return ENOMEM;
    }

    
    /* Activating the created addresspace. */
    as_activate(curthread->t_vmspace);

    assert(curthread->t_vmspace != NULL);

    
    /* Loading the executable. */
    result = load_elf(v, &entrypoint);

    if (result) {
        
        vfs_close(v);
        return result;
    }
    


    
    vfs_close(v);

    /* Define the user stack in the address space */
    result = as_define_stack(curthread->t_vmspace, &stackptr);
    if (result) {
        
        return result;
    }

    /* calculate the size of the stack frame for the main() function */
    int stackFrameSize = 8; 
    for (i = 0; i < nargs; i++) {
        stackFrameSize += strlen(kern_argumentPointer[i]) + 1 + 4; 
    }

    /* Decide the stackpointer address */
    stackptr -= stackFrameSize;
    //decrement the stackptr until it is divisible by 8
    for (; (stackptr % 8) > 0; stackptr--) {
    }

    /* copy the arguments to the proper location on the stack */
    int argumentStringLocation = (int) stackptr + 4 + ((nargs + 1) * 4); //begining of where strings will be stored
    copyout((void *) & nargs, (userptr_t) stackptr, (size_t) 4); //copy argc

    for (i = 0; i < nargs; i++) {
        copyout((void *) & argumentStringLocation, (userptr_t) (stackptr + 4 + (4 * i)), (size_t) 4); //copy address of string into argv[i+1]
        copyoutstr(kern_argumentPointer[i], (userptr_t) argumentStringLocation, (size_t) strlen(kern_argumentPointer[i]), NULL); //copy the argument string
        argumentStringLocation += strlen(kern_argumentPointer[i]) + 1; //update the location for the next iteration
    }
    int *nullValue = NULL;
    copyout((void *) & nullValue, (userptr_t) (stackptr + 4 + (4 * i)), (size_t) 4); //copy null into last position of argv

    /* free up the memory we copied our arguments to in the kernel */
    kfree(kern_progname);

    //free up each string in the arguments
    for (i = 0; i < nargs; i++) {
        kfree(kern_argumentPointer[i]);
    }
    kfree(kern_argumentPointer);
    kprintf("Going to user mode");
    md_usermode(nargs , (userptr_t) (stackptr + 4) , stackptr, entrypoint);

    /* md_usermode does not return */
    panic("md_usermode returned\n");
    return EINVAL;
}
