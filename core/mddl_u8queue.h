#ifndef INC_MDDL_U8QUEUE_H
#define INC_MDDL_U8QUEUE_H

#pragma once

#include <stddef.h>
#include <stdint.h>

typedef struct _mddl_u8queue_ {
    size_t queue_size;
    unsigned int head; /* キューの先頭位置 */
    unsigned int tail; /* キューの末尾位置 */
    uint8_t *buf;
} mddl_u8queue_t;

#if defined (__cplusplus )
extern "C" {
#endif

int mddl_u8queue_init(mddl_u8queue_t *const self_p, const size_t size);
int mddl_u8queue_is_empty(mddl_u8queue_t *const self_p);
int mddl_u8queue_push(mddl_u8queue_t *const self_p, const uint8_t u8);
int mddl_u8queue_pop(mddl_u8queue_t *const self_p, uint8_t *const u8_p);
int mddl_u8queue_destroy(mddl_u8queue_t *const self_p);

#if defined (__cplusplus )
}
#endif

#endif /* INC_MDDL_U8QUEUE_H */
