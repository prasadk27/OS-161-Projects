#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <addrspace.h>
#include <vm.h>
#include <thread.h>
#include <curthread.h>
#include <segments.h>
#include <vm_tlb.h>
#include <pt.h>
#include <machine/spl.h>
#include <machine/tlb.h>
#include <coremap.h>
#include <vnode.h>
#include <vfs.h>
#include <kern/unistd.h>

#include <elf.h>
#define DUMBVM_STACKPAGES    12

struct addrspace* last_addrspace = NULL;

void vm_bootstrap(void) {
 	kprintf(" ");
}

void vm_shutdown(void) {
    int spl = splhigh();
    splx(spl);
}

paddr_t getppages(unsigned long npages) {
    assert(0);
    int spl;
    paddr_t addr;

    spl = splhigh();

    addr = ram_stealmem(npages);

    splx(spl);
    return addr;
}
vaddr_t alloc_kpages(int npages) {
    return cm_request_kframes(npages);
}

void free_kpages(vaddr_t addr) {
    int paddr = (int) addr - MIPS_KSEG0;
    assert(paddr % PAGE_SIZE == 0);
    cm_release_kframes(paddr / PAGE_SIZE);
}

int vm_fault(int faulttype, vaddr_t faultaddress) {
    struct addrspace *as;
    int spl;

    spl = splhigh();

    faultaddress &= PAGE_FRAME;

    as = curthread->t_vmspace;
    if (as == NULL) {
        
        return EFAULT;
    }

    switch (faulttype) {
        case VM_FAULT_READONLY:
            thread_exit();
            return EFAULT;
        case VM_FAULT_WRITE:
            if (!as_valid_write_addr(as, (void *) faultaddress)) {
                thread_exit();
                return EFAULT;
            }
            break;
        case VM_FAULT_READ:
            if (!as_valid_read_addr(as, (void *) faultaddress)) {
                thread_exit();
                return EFAULT;
            }
            break;
        default:
            splx(spl);
            return EINVAL;
    }

    struct segment * s = as_get_segment(as, faultaddress);
    assert(s != NULL);
    splx(spl);
    pt_page_in(faultaddress, s);
    return 0;
}


struct addrspace * as_create(void) {
    struct addrspace *as = kmalloc(sizeof (struct addrspace));
    if (as == NULL) {
        return NULL;
    }

    int i;
    for (i = 0; i < AS_NUM_SEG - 1; i++) {
        as->segments[i].active = 0;
        as->segments[i].vbase = 0;
        as->segments[i].size = 0;
        as->segments[i].writeable = 0;
        as->segments[i].pt = NULL;
        as->segments[i].p_offset = 0;
        as->segments[i].p_filesz = 0;
        as->segments[i].p_memsz = 0;
        as->segments[i].p_flags = 0;

    }

    as->file = NULL;
    as->num_segments = 0;
    return as;
}

void as_destroy(struct addrspace *as) {
    as_free_segments(as);
    if (as->file != NULL) {
        VOP_DECOPEN(as->file); 
    }
    kfree(as);
}

void as_free_segments(struct addrspace *as){
    assert(as != NULL);
    int i;
    for(i=0;i < AS_NUM_SEG; i++){
        if(as->segments[i].active){
            if(as->segments[i].pt != NULL){
                //freeing all the frames
                int j;
                for(j = 0; j < as->segments[i].pt->size; j++){
                    if(as->segments[i].pt->page_details[j].pfn != -1){
                        cm_release_frame(as->segments[i].pt->page_details[j].pfn);
                    }
                    if(as->segments[i].pt->page_details[j].sfn != -1){
                        swap_free_page(as->segments[i].pt->page_details[j].sfn);
                    }
                }
                
                pt_destroy(as->segments[i].pt);
            }
        }
    }
    
}

void as_activate(struct addrspace *as) {
    if(last_addrspace != NULL && last_addrspace != as){
        tlb_context_switch();
    }
    last_addrspace = as;
}

int as_define_region(struct addrspace *as, vaddr_t vaddr, size_t sz, int flags, u_int32_t offset, u_int32_t filesz) {
    size_t npages;
    size_t memsz = sz;
    /* Base and page aligning */
    sz += vaddr & ~(vaddr_t) PAGE_FRAME;
    vaddr &= PAGE_FRAME;
    sz = (sz + PAGE_SIZE - 1) & PAGE_FRAME;

    npages = sz / PAGE_SIZE;


    if (as->num_segments < AS_NUM_SEG - 1) {
        assert(as->segments[as->num_segments].active == 0);
        as->segments[as->num_segments].active = 1;
        as->segments[as->num_segments].vbase = vaddr;
        as->segments[as->num_segments].size = npages;
        as->segments[as->num_segments].writeable = flags & PF_W;
        as->segments[as->num_segments].p_offset = offset;
        as->segments[as->num_segments].p_memsz = memsz;
        as->segments[as->num_segments].p_filesz = filesz;
        as->segments[as->num_segments].p_flags = flags & PF_X;
        as->segments[as->num_segments].pt = pt_create(&(as->segments[as->num_segments]));
        assert(as->segments[as->num_segments].pt != NULL);
        as->num_segments++;
        return 0;
    } else {
        kprintf("dumbvm: Warning: too many regions\n");

        return EUNIMP;
    }
    return 0;

}

int as_copy_segments(struct addrspace *old, struct addrspace *new){
    //set the number of segments
    new->num_segments = old->num_segments;
    
    int i;
    for (i = 0; i < AS_NUM_SEG; i++) {
        if (old->segments[i].active) {
            new->segments[i].active = 1;
            new->segments[i].vbase = old->segments[i].vbase;
            new->segments[i].size = old->segments[i].size;
            new->segments[i].writeable = old->segments[i].writeable;
            new->segments[i].p_offset = old->segments[i].p_offset;
            new->segments[i].p_filesz = old->segments[i].p_filesz;
            new->segments[i].p_memsz = old->segments[i].p_memsz;
            new->segments[i].p_flags = old->segments[i].p_flags;
	        new->segments[i].pt = pt_create(&new->segments[i]);
	        int j;
	        for(j=0;j < new->segments[i].pt->size; j++){
 
                struct page_detail *pd = &(new->segments[i].pt->page_details[j]);
                if(old->segments[i].pt->page_details[j].dirty){
                    
                    pd->use = 0;
                    pd->valid = 1;
                    pd->pfn = cm_getppage();
                    vaddr_t copy_location = PADDR_TO_KVADDR(pd->pfn*PAGE_SIZE);
		    //making sure the old address space is saved.
                    assert(curthread->t_vmspace == old); 
                    copyin((void *)pd->vaddr, (void *) copy_location, PAGE_SIZE);
                    
                    cm_finish_paging(pd->pfn, pd);
                }else{
                  
                    pd->use = 0;
                    pd->pfn = -1;
                    pd->valid = 0;
                    pd->sfn = -1;
                }
	        }
        }
    }
    
    return 0;
}

int as_copy(struct addrspace *old, struct addrspace **ret) {
    struct addrspace *new;

    new = as_create();
    if (new == NULL) {
        return ENOMEM;
    }
    
    //copy the vnode and open it.
    new->file = old->file;
    VOP_OPEN(new->file, O_RDONLY);
    VOP_INCOPEN(new->file);
    int result = as_copy_segments(old, new);
    if(result){
        return result;
    }
    *ret = new;
    return 0;
}

int as_define_stack(struct addrspace *as, vaddr_t *stackptr) {
    as->segments[AS_NUM_SEG - 1].active = 1;
    as->segments[AS_NUM_SEG - 1].vbase = USERTOP - DUMBVM_STACKPAGES*PAGE_SIZE;
    as->segments[AS_NUM_SEG - 1].size = DUMBVM_STACKPAGES;
    as->segments[AS_NUM_SEG - 1].writeable = 1;
    as->segments[AS_NUM_SEG - 1].p_offset = 0;
    as->segments[AS_NUM_SEG - 1].p_filesz = 0;
    as->segments[AS_NUM_SEG - 1].p_memsz = 0;
    as->segments[AS_NUM_SEG - 1].p_flags = 0;
    as->segments[AS_NUM_SEG - 1].pt = pt_create(&(as->segments[AS_NUM_SEG - 1]));
    *stackptr = USERTOP;
    return 0;
}

struct segment * as_get_segment(struct addrspace * as, vaddr_t v) {
    int i = 0;
    for (i = 0; i < AS_NUM_SEG; i++) {
        if (as->segments[i].active == 1 && v >= as->segments[i].vbase && v < as->segments[i].vbase + as->segments[i].size * PAGE_SIZE) {
            return &as->segments[i];
        }
    }
    return NULL;
}

int as_valid_read_addr(struct addrspace *as, vaddr_t *check_addr) {
    int i = 0;
    if (!(check_addr < (vaddr_t *) USERTOP)) {
        return 0;
    }
    for (i = 0; i < AS_NUM_SEG; i++) {
        if (as->segments[i].active) {
            if ((check_addr >= (vaddr_t *) as->segments[i].vbase) && (check_addr < (vaddr_t *) as->segments[i].vbase + PAGE_SIZE * as->segments[i].size)) {
                return 1;
            }
        }

    }
    return 0;
}

int as_valid_write_addr(struct addrspace *as, vaddr_t *check_addr) {
    int i = 0;
    if (!(check_addr < (vaddr_t *) USERTOP)) {
        return 0;
    }
    for (i = 0; i < AS_NUM_SEG; i++) {
        if (as->segments[i].active) {
            if ((check_addr >= (vaddr_t *) as->segments[i].vbase) && (check_addr < (vaddr_t *) as->segments[i].vbase + PAGE_SIZE * as->segments[i].size)) {
                return 1;
            }
        }

    }
    return 0;
}
