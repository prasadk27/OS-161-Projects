#ifndef _SEGMENTS_H_
#define _SEGMENTS_H_


struct segment {
	int active;
	vaddr_t vbase; /*Base Virtual Address*/
	size_t size; /*Number of pages*/

	int writeable; /* Writeable */
	
	struct page_table * pt;
	
	u_int32_t p_offset; /* Location of data within file */
	u_int32_t p_filesz; /* Size of data within file */
	u_int32_t p_memsz; /* Size of data to be loaded into memory*/
	u_int32_t p_flags; /* Flags */
};

#endif
