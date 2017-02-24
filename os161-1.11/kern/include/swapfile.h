#ifndef __SWAPFILE_H__
#define __SWAPFILE_H__

#include <types.h>
#include <vm.h>

typedef  int swap_index_t;


void swap_bootstrap();
int swap_full();
void swap_free_page(swap_index_t n);
swap_index_t swap_write(int phys_frame_num);
void swap_read(int phys_frame_num, swap_index_t n);

#endif
