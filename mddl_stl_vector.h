#ifndef INC_MDDL_STL_VECTOR_H
#define INC_MDDL_STL_VECTOR_H

#include <stddef.h>
#include <stdint.h>

typedef struct _mddl_stl_vector {
    size_t sizof_element;
    void *ext;
} mddl_stl_vector_t;

#if defined (__cplusplus )
extern "C" {
#endif

int mddl_stl_vector_init( mddl_stl_vector_t *const self_p, const size_t sizof_element);
int mddl_stl_vector_attach_memory_fixed( mddl_stl_vector_t *const self_p, void *const mem_ptr, const size_t len);

int mddl_stl_vector_destroy( mddl_stl_vector_t *const self_p);

int mddl_stl_vector_push_back( mddl_stl_vector_t *const self_p, const void *const el_p, const size_t sizof_element);
int mddl_stl_vector_pop_back( mddl_stl_vector_t *const self_p);

int mddl_stl_vector_resize( mddl_stl_vector_t *const self_p, const size_t num_elements, const void *const el_p, const size_t sizof_element);
int mddl_stl_vector_reserve( mddl_stl_vector_t *const self_p, const size_t num_elements);
int mddl_stl_vector_clear( mddl_stl_vector_t *const self_p);

size_t mddl_stl_vector_capacity( mddl_stl_vector_t *const self_p);
void *mddl_stl_vector_ptr_at( mddl_stl_vector_t *const self_p, const size_t num);
int mddl_stl_vector_is_empty( mddl_stl_vector_t *const self_p);

size_t mddl_stl_vector_size( mddl_stl_vector_t *const self_p);
size_t mddl_stl_vector_get_pool_cnt( mddl_stl_vector_t *const self_p);

int mddl_stl_vector_front( mddl_stl_vector_t *const self_p, void *const el_p, const size_t sizof_element);
int mddl_stl_vector_back( mddl_stl_vector_t *const self_p, void *const el_p, const size_t sizof_element);

int mddl_stl_vector_insert( mddl_stl_vector_t *const self_p, const size_t num, const void *const el_p, const size_t sizof_element);

int mddl_stl_vector_remove_at( mddl_stl_vector_t *const self_p, const size_t num);

int mddl_stl_vector_get_element_at( mddl_stl_vector_t *const self_p, const size_t num, void *const el_p, const size_t sizof_element);
int mddl_stl_vector_overwrite_element_at( mddl_stl_vector_t *const self_p, const size_t num, const void *const el_p, const size_t sizof_element);

int mddl_stl_vector_element_swap_at( mddl_stl_vector_t *const self_p, const size_t at1, const size_t at2);

int mddl_stl_vector_shrink( mddl_stl_vector_t *const self_p, const size_t num_elements);

#if defined (__cplusplus )
}
#endif

#endif /* end of INC_MDDL_STL_VECTOR_H */
