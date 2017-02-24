#ifndef _PT_H_
#define _PT_H_
#include <swapfile.h>
#include <addrspace.h>
#include <segments.h>
#include <thread.h>

struct page_detail {
	vaddr_t vaddr; 
	int pfn; 
	swap_index_t sfn; 
	int valid; 
	int dirty; 
	int use; 
};

struct page_table {
	int size;
	struct segment* seg;
	struct page_detail *page_details;
};


struct page_table* pt_create(struct segment *segments);

void pt_page_in(vaddr_t vaddr, struct segment *s);
void pt_destroy(struct page_table *pt);

#endif
