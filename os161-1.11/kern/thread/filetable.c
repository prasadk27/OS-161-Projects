#include <types.h>
#include <kern/errno.h>
#include <kern/unistd.h>
#include <kern/limits.h>
#include <machine/spl.h>
#include <lib.h>
#include <array.h>
#include <queue.h>
#include <vfs.h>
#include <vnode.h>
#include <filetable.h>

struct filetable *ft_create() {

    struct filetable *ft = kmalloc(sizeof (struct filetable));
    if (ft == NULL) {
        return NULL;
    }
    ft->size = 0;

    ft->filedescriptor = array_create();
    if (ft->filedescriptor == NULL) {
        ft_destroy(ft);
        return NULL;
    }

    array_preallocate(ft->filedescriptor, 20);

    array_add(ft->filedescriptor, NULL);
    array_add(ft->filedescriptor, NULL);
    array_add(ft->filedescriptor, NULL);
    
    return ft;
}


int ft_attachstds(struct filetable *ft) {
    char *console = NULL;
    int mode;
    int result = 0;
    
    struct vnode *vn_stdin;
    mode = O_RDONLY;
    struct filedescriptor *fd_stdin = NULL;
    fd_stdin = (struct filedescriptor *) kmalloc(sizeof ( struct filedescriptor));
    if (fd_stdin == NULL) {
        ft_destroy(ft);
        return 0;
    }
    console = kstrdup("con:");
    result = vfs_open(console, mode, &vn_stdin);
    if (result) {
        vfs_close(vn_stdin);
        ft_destroy(ft);
        return 0;
    }
    kfree(console);
    fd_stdin->mode = mode;
    fd_stdin->offset = 0;
    fd_stdin->fdvnode = vn_stdin;
    fd_stdin->numOwners = 1;
    ft_set(ft, fd_stdin, STDIN_FILENO);
    
    struct vnode *vn_stdout;
    mode = O_WRONLY;
    struct filedescriptor *fd_stdout = NULL;
    fd_stdout = (struct filedescriptor *) kmalloc(sizeof (struct filedescriptor));
    if (fd_stdout == NULL) {
        ft_destroy(ft);
        return 0;
    }
    console = kstrdup("con:");
    result = vfs_open(console, mode, &vn_stdout);
    if (result) {
        vfs_close(vn_stdout);
        ft_destroy(ft);
        return 0;
    }
    kfree(console);
    fd_stdout->mode = mode;
    fd_stdout->offset = 0;
    fd_stdout->fdvnode = vn_stdout;
    fd_stdout->numOwners = 1;
    ft_set(ft, fd_stdout, STDOUT_FILENO);
    struct vnode *vn_stderr;
    mode = O_WRONLY;
    struct filedescriptor *fd_stderr = NULL;
    fd_stderr = (struct filedescriptor *) kmalloc(sizeof (struct filedescriptor));
    if (fd_stderr == NULL) {
        ft_destroy(ft);
        return 0;
    }
    console = kstrdup("con:");
    result = vfs_open(console, mode, &vn_stderr);
    if (result) {
        vfs_close(vn_stderr);
        ft_destroy(ft);
        return 0;
    }
    kfree(console);
    fd_stderr->mode = mode;
    fd_stderr->offset = 0;
    fd_stderr->fdvnode = vn_stderr;
    fd_stderr->numOwners = 1;
    ft_set(ft, fd_stderr, STDERR_FILENO);
    return 1;
}


int ft_array_size(struct filetable *ft) {
    assert(ft != NULL);
    return (array_getnum(ft->filedescriptor));
}

int ft_size(struct filetable *ft) {
    assert(ft != NULL);
    int total = array_getnum(ft->filedescriptor);
    int i = 0;
    for (i = 0; i < ft_array_size(ft); i++) {
        if (ft_get(ft, i) == NULL) {
            total--;
        }
    }
    return total;
}

struct filedescriptor *ft_get(struct filetable *ft, int fti) {
    if (fti < 0) {
        return NULL;
    }
    
    if (fti < 3) {
        if (array_getguy(ft->filedescriptor, fti) == NULL) {
            ft_attachstds(ft);
        }
    }
    
    if (fti >= ft_array_size(ft)) { 
        return NULL;
    }
    struct filedescriptor *ret = array_getguy(ft->filedescriptor, fti);
    return ret;
}

int ft_set(struct filetable* ft, struct filedescriptor* fd, int fti) {
    if (fti >= ft_array_size(ft)) {
        return 1;
    }
    array_setguy(ft->filedescriptor, fti, fd);
    if (ft_get(ft, fti) == fd) {
        return 1;
    }
    return 0;
}

int ft_add(struct filetable* ft, struct filedescriptor* fd) {
    int fdn = 0;
    for (fdn = 0; fdn < ft_array_size(ft) && fdn < OPEN_MAX; fdn++) {
        if (ft_get(ft, fdn) == NULL) {
            array_setguy(ft->filedescriptor, fdn, fd);
            return fdn;
        }
    }
    if (fdn == OPEN_MAX) {
        return -1;
    }
    if (array_add(ft->filedescriptor, fd) != 0) { 
        return -1;
    }
    fd->numOwners++;
    assert(fdn != 0);
    return fdn;
}

int ft_remove(struct filetable* ft, int fti) {
    struct filedescriptor * fd = ft_get(ft, fti);
    if (fd != NULL) {

        int spl = splhigh();
        fd->numOwners--;
        if (fd->numOwners == 0) {
            vfs_close(fd->fdvnode);
            kfree(fd);
        }
        splx(spl);
        array_setguy(ft->filedescriptor, fti, NULL);
    }
    return 1;
}

int ft_destroy(struct filetable* ft) {
    int i;
    for (i = ft_array_size(ft) - 1; i >= 0; i--) {
        ft_remove(ft, i);
    }
    kfree(ft);
    return 1;
}

void ft_test_list(struct filetable* ft) {
    kprintf("printing file descriptors\n");
    int i = 0;
    for (i = 0; i < ft_array_size(ft); i++) {
        struct filedescriptor *fd;
        fd = ft_get(ft, i);
        kprintf("filetable index: %d         ", i);

        if (fd != NULL) {
            kprintf("fdn: %d   ", fd->fdn);
        } else {
            kprintf("fdn: NULL   ");
        }
        kprintf("\n");
    }
}


void ft_test(struct filetable* ft) {
    kprintf("filetable test begin\n");
    kprintf("inserting file descriptors\n");
    ft_attachstds(ft);
    kprintf("printing file descriptors\n");
    int i = 0;
    for (i = 0; i < ft_array_size(ft); i++) {
        struct filedescriptor *fd;
        fd = ft_get(ft, i);
        kprintf("filetable index: %d\n", i);
        kprintf("fdn: %d   ", fd->fdn);
        kprintf("\n");
    }
    kprintf("inserting more file descriptors\n");
    ft_attachstds(ft);
    kprintf("printing file descriptors\n");
    for (i = 0; i < ft_array_size(ft); i++) {
        struct filedescriptor *fd;
        fd = ft_get(ft, i);
        kprintf("filetable index: %d ", i);
        kprintf("fdn: %d ", fd->fdn);
        kprintf("\n");
    }
    kprintf("removing file descriptor 4\n");
    ft_remove(ft, 4);
    kprintf("inserting more file descriptors\n");
    ft_attachstds(ft);
    kprintf("printing file descriptors\n");
    for (i = 0; i < ft_array_size(ft); i++) {
        struct filedescriptor *fd;
        fd = ft_get(ft, i);
        kprintf("filetable index: %d ", i);
        kprintf("fdn: %d ", fd->fdn);
        kprintf("\n");
    }
    kprintf("inserting infinite number of file descriptors\n");
    for (i = 0; i < 2147483647; i++) {
        kprintf("ft_array_size: %d\n", ft_array_size(ft));
        ft_attachstds(ft);
    }   
    panic("test end");
}

