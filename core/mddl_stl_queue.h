#ifndef INC_MDDL_STL_QUEUE_H
#define INC_MDDL_STL_QUEUE_H

#include <stddef.h>

typedef enum _mddl_stl_queue_implement_type {
    MDDL_STL_QUEUE_TYPE_IS_DEFAULT = 11,
    MDDL_STL_QUEUE_TYPE_IS_SLIST,
    MDDL_STL_QUEUE_TYPE_IS_DEQUE,
    MDDL_STL_QUEUE_TYPE_IS_LIST,
    MDDL_STL_QUEUE_TYPE_IS_OTHERS
} enum_mddl_stl_queue_implement_type_t;

typedef struct _mddl_stl_queue {
   size_t sizof_element;
   void *ext;
} mddl_stl_queue_t;

#if defined (__cplusplus )
extern "C" {
#endif

int mddl_stl_queue_init( mddl_stl_queue_t *const self_p, const size_t sizof_element);
int mddl_stl_queue_init_ex( mddl_stl_queue_t *const self_p, const size_t sizof_element, const enum_mddl_stl_queue_implement_type_t implement_type, void *const attr_p);

int mddl_stl_queue_destroy( mddl_stl_queue_t *const self_p);
int mddl_stl_queue_push( mddl_stl_queue_t *const self_p, const void *const el_p, const size_t sizof_element);
int mddl_stl_queue_pop( mddl_stl_queue_t *const self_p);
int mddl_stl_queue_front( mddl_stl_queue_t *const self_p, void *const el_p, const size_t sizof_element );
int mddl_stl_queue_get_element_at( mddl_stl_queue_t *const self_p, const size_t num, void *const el_p, const size_t sizof_element);
int mddl_stl_queue_clear( mddl_stl_queue_t *const self_p);
size_t mddl_stl_queue_get_pool_cnt( mddl_stl_queue_t *const self_p);
int mddl_stl_queue_is_empty( mddl_stl_queue_t *const self_p);

int mddl_stl_queue_back( mddl_stl_queue_t *const self_p, void *const el_p, const size_t  sizof_element );

#if defined (__cplusplus )
}
#endif

#endif /* end of INC_MDDL_STL_QUEUE_H */
