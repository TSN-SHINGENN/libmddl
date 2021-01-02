#ifndef INC_MDDL_MALLOCATER_H
#define INC_MDDL_MALLOCATER_H

#include <stddef.h>

typedef struct _mddl_malllocate_area_header {
    size_t size;
    union {
	uint32_t magic_no;
	uint32_t occupied;
    } stamp;
    struct _mddl_malllocate_area_header *next_p;
    struct _mddl_malllocate_area_header *prev_p;
} mddl_malllocate_header_t;

typedef struct _mddl_mallocater {
    void *buf;
    size_t bufsiz;
    mddl_malllocate_header_t base;
    uint8_t bufofs;
    union {
	uint8_t flags;
	struct {
	    uint8_t initialized:1;
	} f;
    } init;
} mddl_mallocater_t;

extern mddl_mallocater_t _mddl_mallocater_heap_obj;

#if defined (__cplusplus )
extern "C" {
#endif

void *mddl_mallocater_alloc(const size_t size);
void mddl_mallocater_free(void * const ptr); 
void *mddl_mallocater_realloc(void * const ptr, const size_t size);

int mddl_mallocater_init_obj(mddl_mallocater_t *const self_p, void *const buf, const size_t bufsize);
void *mddl_mallocater_alloc_with_obj(mddl_mallocater_t *const self_p,const size_t size);
void mddl_mallocater_free_with_obj(mddl_mallocater_t *const self_p, void * const ptr); 
void *mddl_mallocater_realloc_with_obj(mddl_mallocater_t *const self_p, void *const ptr, const size_t size);

int64_t mddl_mallocater_avphys_with_obj(mddl_mallocater_t *const self_p);

void _mddl_mallocater_dump_region_list(mddl_mallocater_t const *const self_p);

#if defined (__cplusplus )
}
#endif

#endif /* end of INC_MDDL_MALLOCATER_H */
