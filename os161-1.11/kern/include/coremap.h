#include <types.h>

#ifndef __COREMAP_H__
#define __COREMAP_H__

struct cm_detail;

struct cm_detail {
        int id;
        int kern;
        struct page_detail *pd;
        struct cm_detail *next_free;
        struct cm_detail *prev_free;
        int free;

};

struct cm {
        int init;
        int size;
        int clock_pointer;
        int lowest_frame;
        struct cm_detail *core_details;
        struct cm_detail *free_frame_list;
        struct cm_detail *last_free;
};


void cm_bootstrap();
int cm_getppage();
void cm_release_frame(int frame_number);
void cm_free_core(struct cm_detail *cd, int spl);
void cm_finish_paging(int frame, struct page_detail* pd);
int cm_push_to_swap();
vaddr_t cm_request_kframes(int num);
void cm_release_kframes(int frame_number);



#endif
