#ifndef INC_MDDL_STL_LIST_H
#define INC_MDDL_STL_LIST_H

typedef struct _mddl_stl_list {
   size_t sizeof_element;
   void *ext;
} mddl_stl_list_t;

#if defined (__cplusplus )
extern "C" {
#endif

int mddl_stl_list_init( mddl_stl_list_t *const self_p, const size_t sizeof_element);
int mddl_stl_list_destroy( mddl_stl_list_t *const self_p);
int mddl_stl_list_push_back( mddl_stl_list_t *const self_p, const void *const el_p, const size_t sizeof_element);
int mddl_stl_list_push_front( mddl_stl_list_t *const self_p, const void *const el_p, const size_t sizof_element );
int mddl_stl_list_pop_front( mddl_stl_list_t *const self_p);
int mddl_stl_list_pop_back( mddl_stl_list_t *const self_p);
int mddl_stl_list_insert( mddl_stl_list_t *const self_p, const size_t num, const void *const el_p, const size_t sizof_element );
int mddl_stl_list_get_element_at( mddl_stl_list_t *const self_p, const size_t num, void *const el_p, const size_t sizof_element);
int mddl_stl_list_clear( mddl_stl_list_t *const self_p);
size_t mddl_stl_list_get_pool_cnt( mddl_stl_list_t *const self_p);
int mddl_stl_list_is_empty( mddl_stl_list_t *const self_p);
int mddl_stl_list_remove_at( mddl_stl_list_t *const self_p,  const size_t num);
int mddl_stl_list_overwrite_element_at( mddl_stl_list_t *const self_p,  const size_t num, const void *const el_p, const size_t sizof_element);

int mddl_stl_list_front( mddl_stl_list_t *const self_p, void *const el_p, const size_t sizof_element);
int mddl_stl_list_back( mddl_stl_list_t *const self_p, void *const el_p, const size_t sizof_element);

void mddl_stl_list_dump_element_region_list( mddl_stl_list_t *const self_p);

#if defined (__cplusplus )
}
#endif

#endif /* end of INC_MDDL_STL_LIST_H */
