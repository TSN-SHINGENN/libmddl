#ifndef INC_MDDL_STL_SLIST_H
#define INC_MDDL_STL_SLIST_H

#pragma once

#include <stddef.h>

typedef struct _mddl_stl_slist {
   size_t sizof_element;
   void *ext;
} mddl_stl_slist_t;

#if defined (__cplusplus )
extern "C" {
#endif

int mddl_stl_slist_init( mddl_stl_slist_t *const self_p, const size_t sizof_element);
int mddl_stl_slist_destroy( mddl_stl_slist_t *const self_p);
int mddl_stl_slist_push( mddl_stl_slist_t *const self_p, const void *const el_p, const size_t sizof_element);
int mddl_stl_slist_pop( mddl_stl_slist_t *const self_p);
int mddl_stl_slist_front( mddl_stl_slist_t *const self_p, void *const el_p, const size_t  sizof_element );
int mddl_stl_slist_get_element_at( mddl_stl_slist_t *const self_p, const size_t num, void *const el_p, const size_t sizof_element);
int mddl_stl_slist_clear( mddl_stl_slist_t *const self_p);
size_t mddl_stl_slist_get_pool_cnt( mddl_stl_slist_t *const self_p);
int mddl_stl_slist_is_empty( mddl_stl_slist_t *const self_p);

int mddl_stl_slist_back( mddl_stl_slist_t *const self_p, void *const el_p, const size_t  sizof_element );
int mddl_stl_slist_insert_at( mddl_stl_slist_t *const self_p, const size_t no, void *const el_p, const size_t  sizof_element );
int mddl_stl_slist_erase_at( mddl_stl_slist_t *const self_p, const size_t no);

#if defined (__cplusplus )
}
#endif

#endif /* end of INC_MDDL_STL_SLIST_H */
