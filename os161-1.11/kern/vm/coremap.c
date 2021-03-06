#include <types.h>
#include <kern/errno.h>
#include <kern/unistd.h>
#include <kern/limits.h>
#include <machine/spl.h>
#include <lib.h>
#include <vfs.h>
#include <vnode.h>
#include <thread.h>
#include <vm.h>
#include <swapfile.h>
#include <coremap.h>
#include <pt.h>
#include <vm_tlb.h>
#include <swapfile.h>


struct cm core_map;

void cm_bootstrap() {
    assert(curspl > 0);
    //we should call core_map_init only once
    assert(core_map.init == 0); 
    core_map.init = 1;
    core_map.size = mips_ramsize() / PAGE_SIZE;

    core_map.core_details = (struct cm_detail*) kmalloc(sizeof (struct cm_detail) * core_map.size);

    paddr_t low;
    paddr_t high;
    ram_getsize(&low, &high);
    assert(high != 0); 

    core_map.lowest_frame = low / PAGE_SIZE;

    core_map.free_frame_list = &(core_map.core_details[core_map.size - 1]);
 
    int i;
    for (i = core_map.size - 1; i >= core_map.lowest_frame; i--) {
        core_map.core_details[i].id = i;
        core_map.core_details[i].kern = 0;
        core_map.core_details[i].pd = NULL;
        core_map.core_details[i].next_free = &(core_map.core_details[i - 1]);
        core_map.core_details[i].prev_free = &(core_map.core_details[i + 1]);
        core_map.core_details[i].free = 1;
    }
    core_map.core_details[core_map.lowest_frame].next_free = NULL;
    core_map.core_details[core_map.size - 1].prev_free = NULL; 
    
    core_map.last_free = &core_map.core_details[core_map.lowest_frame];
}

void free_frame_list_add(struct cm_detail *new) {
    int spl = splhigh();
    if (core_map.free_frame_list == NULL) {
        core_map.free_frame_list = new;
        core_map.last_free = new;
        new->next_free = NULL;
        new->prev_free = NULL;
    } else {
        new->next_free = core_map.free_frame_list;
        new->prev_free = NULL;
        core_map.free_frame_list->prev_free = new;
        core_map.free_frame_list = new;
    }
    new->free = 1;
    new->kern = 0;
    splx(spl);
}

void free_frame_list_add_back(struct cm_detail *new) {
    int spl = splhigh();

    if (core_map.last_free == NULL) {
        core_map.free_frame_list = new;
        core_map.last_free = new;
        new->next_free = NULL;
        new->prev_free = NULL;
    } else {
        new->prev_free = core_map.last_free;
        new->next_free = NULL;
        core_map.last_free->next_free = new;
        core_map.last_free = new;
    }
    new->free = 1;
    new->kern = 0;
    splx(spl);
}

struct cm_detail *free_frame_list_pop() {
    int spl = splhigh();

    if (core_map.free_frame_list == NULL) {
        splx(spl);
        return NULL;
    }

    struct cm_detail *retval;
    retval = core_map.free_frame_list;
    core_map.free_frame_list = core_map.free_frame_list->next_free;
    if (core_map.free_frame_list == NULL) {
        core_map.last_free = NULL;
    } else {
        core_map.free_frame_list->prev_free = NULL;
    }
    
    retval->kern = 1;
    retval->free = 0;

    splx(spl);
    return retval;
}

void free_frame_list_remove(int i) {
    int spl = splhigh();

    assert(core_map.free_frame_list != NULL && core_map.last_free != NULL);
    if (core_map.free_frame_list == core_map.last_free) {
        assert(core_map.free_frame_list == &core_map.core_details[i]);
        core_map.free_frame_list = NULL;
        core_map.last_free = NULL;
    } else if (core_map.free_frame_list == &core_map.core_details[i]) {
        core_map.free_frame_list = core_map.free_frame_list->next_free;
        core_map.free_frame_list->prev_free = NULL;
    } else if (core_map.last_free == &core_map.core_details[i]) {
        core_map.last_free = core_map.last_free->prev_free;
        core_map.last_free->next_free = NULL;
    } else {
        core_map.core_details[i].prev_free->next_free = core_map.core_details[i].next_free;
        core_map.core_details[i].next_free->prev_free = core_map.core_details[i].prev_free;
    }
    core_map.core_details[i].free = 0;
    core_map.core_details[i].kern = 1;
    splx(spl);   
}

int clock_to_index(int c) {
    return (c + core_map.lowest_frame);
}

int cm_getppage(){
    int spl = splhigh();    
    struct cm_detail *frame = free_frame_list_pop();
    if (frame == NULL) {
        
        splx(spl);
        return cm_push_to_swap();
    } else {
        
        splx(spl);
        return frame->id;
    }
}

int cm_push_to_swap() {
    int spl = splhigh();
    int i = 0;

    for (i = core_map.lowest_frame; i < core_map.size; i++) {
        struct cm_detail *cd = &core_map.core_details[clock_to_index(core_map.clock_pointer)];
        if (cd->kern == 0) {
            struct page_detail * pd = cd->pd;
            if (pd == NULL) {
                panic("FREE PHYSICAL FRAMES NOT IN THE FREE LIST");
            }
            if (pd->use == 0) {
                cm_free_core(cd, spl);
                return cd->id;
            } else {
                pd->use = 0;
                tlb_invalidate_vaddr(pd->vaddr);
            }
        }
        core_map.clock_pointer = (core_map.clock_pointer + 1) % (core_map.size - core_map.lowest_frame);
    }
    for (i = core_map.lowest_frame; i < core_map.size; i++) {
        struct cm_detail *cd = &core_map.core_details[clock_to_index(core_map.clock_pointer)];
        if (cd->kern == 0) {
            struct page_detail * pd = cd->pd;

            if (pd == NULL) {
                panic("FREE PHYSICAL FRAMES NOT IN THE FREE LIST");
            }
           
            cm_free_core(cd, spl);
            return cd->id;

        }
        core_map.clock_pointer = (core_map.clock_pointer + 1) % (core_map.size - core_map.lowest_frame);
    }
    splx(spl);
    
    panic("Too many kernel pages! No more space in RAM for a new page.");
    return 0;
}

void cm_finish_paging(int frame, struct page_detail* pd) {
    core_map.core_details[frame].pd = pd;
    pd->pfn = frame;
    pd->valid = 1;
    
    core_map.core_details[frame].free = 0;
    core_map.core_details[frame].kern = 0;
}

void cm_free_core(struct cm_detail *cd, int spl) {
    tlb_invalidate_vaddr(cd->pd->vaddr);
    cd->pd->valid = 0;
    cd->pd->use = 0;
    cd->pd->sfn = -2;
    splx(spl);
    if (cd->pd->dirty) {
        cd->pd->sfn = swap_write(cd->id);
    }else{
        cd->pd->sfn = -1;
    }
    cd->pd->pfn = -1;
    cd->pd = NULL;
    
    cd->next_free = NULL;
}

vaddr_t cm_request_kframes(int num) {
    assert(core_map.init); 
    int spl = splhigh();
    int frame = -1;
    int i;
    int j;
    for (i = core_map.lowest_frame; i <= core_map.size - num; i++) {
        frame = i;
        for (j = 0; j < num; j++) {
            if (core_map.core_details[i + j].kern) {
                frame = -1;
                break;
            }
        }
        if (frame != -1) break;
    }
    if (frame == -1) {
        panic("No space to get %d kernel page(s)!", num);
    }
    
    for (i = frame; i < frame + num; i++) {
        core_map.core_details[i].kern = 1;
    }
    
    core_map.core_details[frame].kern = num;

    for (i = frame; i < frame + num; i++) {
        if (core_map.core_details[i].free) {
            free_frame_list_remove(i);
        } else {
            cm_free_core(&core_map.core_details[i], spl);
            spl = splhigh();
        }

    }
    splx(spl);
    return PADDR_TO_KVADDR((paddr_t) (frame * PAGE_SIZE));
}

void cm_release_frame(int frame_number) {
    assert(frame_number >= core_map.lowest_frame);
    free_frame_list_add(&core_map.core_details[frame_number]);
}

void cm_release_kframes(int frame_number) {
    if (core_map.core_details[frame_number].kern == 0) {
        assert(0); 
    }
    int num = core_map.core_details[frame_number].kern;
    assert(num > 0);
    int i;
    for (i = frame_number; i < frame_number + num; i++) {
        assert(core_map.core_details[i].kern);
        assert(frame_number >= core_map.lowest_frame);
        free_frame_list_add_back(&core_map.core_details[i]);
    }
}
  
