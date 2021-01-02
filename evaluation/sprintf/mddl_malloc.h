#ifndef INC_MDDL_MALLOC_H
#define INC_MDDL_MALLOC_H
#pragma once

#include <stddef.h>

typedef struct _mddl_mem_alignment_partition {
   size_t foward_sz; /* アライメントされていない前方サイズ */
   size_t middle_sz; /* アライメントされている中間サイズ   */
   size_t bottom_sz; /* アライメントされていない後方サイズ */
   size_t total_sz;  /* 総サイズ */
   size_t alignment_sz; /* 指定されたアライメントサイズ */
   union {
	unsigned int flags;
	struct {
	    unsigned int is_aligned:1;		/* ポインタ先頭がアライメントがととなっている場合を示す */
	    unsigned int size_is_multiple_alignment:1;  /* サイズはアライメントの倍数である場合を示す */
	} f;
   } ext;
} mddl_mem_alignment_partition_t;

#ifdef __cplusplus
extern "C" {
#endif

int mddl_malloc_align(void **memptr, const size_t alignment, const size_t size);
void *mddl_realloc_align( void *memblk, const size_t algnment, const size_t size);
int mddl_mfree(void *memptr);
void *mddl_mrealloc_align( void *memblock, const size_t alignment, const size_t size);

mddl_mem_alignment_partition_t mddl_mem_alignment_partition(int * const resilt_p, const void * const p, size_t size, size_t alignment);

void mddl_malloc_init(void *const buf, const size_t size);
void *mddl_malloc(const size_t size);
void mddl_free( void *const ptr);
void *mddl_realloc( void *const ptr, const size_t size);

#if defined (__cplusplus )
}
#endif

#endif /* end of INC_MDDL_MALLOC_H */
