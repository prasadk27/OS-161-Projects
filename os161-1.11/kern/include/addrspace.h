#ifndef _ADDRSPACE_H_
#define _ADDRSPACE_H_

#include <vm.h>


struct vnode;

#include <segments.h>
#include <pt.h>
#define AS_NUM_SEG 3

/* 
 * Address space - data structure associated with the virtual memory
 * space of a process.
 *
 * You write this.
 */

struct addrspace {
	struct segment segments[AS_NUM_SEG];
	struct vnode *file;
	int num_segments;
};

/*
 * Functions in addrspace.c:
 *
 *    as_create - create a new empty address space. You need to make 
 *                sure this gets called in all the right places. You
 *                may find you want to change the argument list. May
 *                return NULL on out-of-memory error.
 *
 *    as_copy   - create a new address space that is an exact copy of
 *                an old one. Probably calls as_create to get a new
 *                empty address space and fill it in, but that's up to
 *                you.
 *
 *    as_activate - make the specified address space the one currently
 *                "seen" by the processor. Argument might be NULL, 
 *		  meaning "no particular address space".
 *
 *    as_destroy - dispose of an address space. You may need to change
 *                the way this works if implementing user-level threads.
 *
 *    as_define_region - set up a region of memory within the address
 *                space.
 *
 *    as_prepare_load - this is called before actually loading from an
 *                executable into the address space.
 *
 *    as_complete_load - this is called when loading from an executable
 *                is complete.
 *
 *    as_define_stack - set up the stack region in the address space.
 *                (Normally called *after* as_complete_load().) Hands
 *                back the initial stack pointer for the new process.
 *    
 *    as_valid_read_addr - Check an address for valid user reads
 *
 *    as_valid_write_addr - Check an address for valid user writes
 */

struct addrspace *as_create(void);
int as_copy(struct addrspace *src, struct addrspace **ret);
int as_copy_segments(struct addrspace *old, struct addrspace *new);
void as_activate(struct addrspace *);
void as_destroy(struct addrspace *);
void as_free_segments(struct addrspace *as);

int as_define_region(struct addrspace *as, vaddr_t vaddr, size_t sz, int flags, u_int32_t offset, u_int32_t filesz);
int as_define_stack(struct addrspace *as, vaddr_t *initstackptr);
struct segment * as_get_segment(struct addrspace * as, vaddr_t v);
int as_valid_read_addr(struct addrspace *as, vaddr_t *check_addr);
int as_valid_write_addr(struct addrspace *as, vaddr_t *check_addr);

int load_elf(char* progname, vaddr_t *entrypoint);
int load_segment_page(struct vnode *v, vaddr_t vaddr, struct segment *s, paddr_t paddr);
paddr_t getppages(unsigned long npages);













#endif /* _ADDRSPACE_H_ */
