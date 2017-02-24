#include <types.h>
#include <vnode.h>
#include <vfs.h>
#include <uio.h>
#include <synch.h>
#include <vm.h>
#include <kern/unistd.h>
#include <fs.h>
#include <swapfile.h>
#include <lib.h>
#include <machine/spl.h>

//Page file size
#define SWAP_SIZE 4194304

struct vnode *swapfile;
int SWAP_PAGES = SWAP_SIZE / PAGE_SIZE;

struct lock *swapLock;

struct free_list {
    swap_index_t index;
    struct free_list *next;
};

struct free_list *freePages; 
struct free_list *pageList; 

void swap_bootstrap() {   
    freePages = (struct free_list *) kmalloc((int) sizeof(struct free_list) * SWAP_PAGES);
    assert(freePages != NULL);
    pageList = freePages;
    int i = 0;
    for (i = 0; i < SWAP_PAGES - 1 ; i++) {
        freePages[i].index = i;
        freePages[i].next = &freePages[i + 1];
    }
    freePages[SWAP_PAGES - 1].index = SWAP_PAGES - 1;
    freePages[SWAP_PAGES - 1].next = NULL;
    
    
    swapfile = kmalloc(sizeof(struct vnode));
    assert(swapfile != NULL);
    char *path = kstrdup("SWAPFILE\0");
    assert(path != NULL);
    vfs_open(path, O_RDWR | O_CREAT, &swapfile);
    kfree(path);
}


int swap_full() {
    return (freePages == NULL);
}


void swap_free_page(swap_index_t n) {
    pageList[(int) n].next = freePages;
    freePages = &pageList[(int) n];
}

void swap_write_page(void *data, swap_index_t n) {
    struct uio u;
    mk_kuio(&u, data, PAGE_SIZE, (int) n * PAGE_SIZE, UIO_WRITE);
    VOP_WRITE(swapfile, &u);
}


swap_index_t swap_write(int phys_frame_num) {
    swap_index_t pagenum;
    void *data = (void *) PADDR_TO_KVADDR(phys_frame_num * PAGE_SIZE);
    int spl = splhigh();
    if (freePages == NULL) {
        panic("Out of swap space");
    } else {
        pagenum = freePages->index;
        freePages = freePages->next;
    }
    splx(spl);
    swap_write_page(data, pagenum);
    return pagenum;
}


void swap_read(int phys_frame_num, swap_index_t n) {
    void *write_addr = (void *) PADDR_TO_KVADDR(phys_frame_num * PAGE_SIZE);
    struct uio u;
    mk_kuio(&u, write_addr, PAGE_SIZE, (int) n * PAGE_SIZE, UIO_READ);
    VOP_READ(swapfile, &u);
    pageList[(int) n].next = freePages;
    freePages = &pageList[(int) n];
    int spl = splhigh();
    splx(spl);
}
