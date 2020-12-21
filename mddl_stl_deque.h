#ifndef INC_MDDL_STL_DEQUE_H
#define INC_MDDL_STL_DEQUE_H

#pragma once

#include <stddef.h>

typedef struct _mddl_stl_deque {
   size_t sizeof_element;
   void *ext;
} mddl_stl_deque_t;

#if defined (__cplusplus )
extern "C" {
#endif

int mddl_stl_deque_init( mddl_stl_deque_t *const self_p, const size_t sizeof_element);
int mddl_stl_deque_destroy( mddl_stl_deque_t *const self_p);
int mddl_stl_deque_push_back( mddl_stl_deque_t *const self_p, const void *const el_p, const size_t sizeof_element);
int mddl_stl_deque_push_front( mddl_stl_deque_t *const self_p, const void *const el_p, const size_t sizof_element );
int mddl_stl_deque_pop_front( mddl_stl_deque_t *const self_p);
int mddl_stl_deque_pop_back( mddl_stl_deque_t *const self_p);
int mddl_stl_deque_insert( mddl_stl_deque_t *const self_p, const size_t num, const void *const el_p, const size_t sizof_element );
int mddl_stl_deque_get_element_at( mddl_stl_deque_t *const self_p, const size_t num, void *const el_p, const size_t sizof_element);
int mddl_stl_deque_clear( mddl_stl_deque_t *const self_p);
size_t mddl_stl_deque_get_pool_cnt( mddl_stl_deque_t *const self_p);
int mddl_stl_deque_is_empty( mddl_stl_deque_t *const self_p);
int mddl_stl_deque_remove_at( mddl_stl_deque_t *const self_p,  const size_t num);
int mddl_stl_deque_overwrite_element_at( mddl_stl_deque_t *const self_p,  const size_t num, const void *const el_p, const size_t sizof_element);

int mddl_stl_deque_front( mddl_stl_deque_t *const self_p, void *const el_p, const size_t sizof_element);
int mddl_stl_deque_back( mddl_stl_deque_t *const self_p, void *const el_p, const size_t sizof_element);

int mddl_stl_deque_element_swap_at( mddl_stl_deque_t *const self_p, const size_t at1, const size_t at2);

void mddl_stl_deque_dump_element_region_list( mddl_stl_deque_t *const self_p);

void *mddl_stl_deque_ptr_at( mddl_stl_deque_t *const self_p, const size_t num);

#if defined (__cplusplus )
}
#endif

#endif /* end of INC_MDDL_STL_DEQUEUE_H */
