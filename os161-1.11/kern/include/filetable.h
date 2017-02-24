#ifndef _FILETABLE_H_
#define _FILETABLE_H_


struct filedescriptor;

struct filedescriptor {
	int fdn;
	int mode;
	int numOwners;
	int offset;
	struct vnode* fdvnode;
};


struct filetable;

struct filetable {
	struct array *filedescriptor;
	int size;
};

struct filetable *ft_create();
int ft_attachstds(struct filetable *ft);
int ft_array_size(struct filetable *ft);
int ft_size(struct filetable *ft);
struct filedescriptor *ft_get(struct filetable *ft, int fti);
int ft_set(struct filetable* ft, struct filedescriptor* fdn, int fti);
int ft_add(struct filetable* ft, struct filedescriptor* fdn);
int ft_remove(struct filetable* ft, int fti);
int ft_destroy(struct filetable* ft);
void ft_test_list(struct filetable* ft);
void ft_test(struct filetable* ft);

#endif /* _FILETABLE_H_ */
