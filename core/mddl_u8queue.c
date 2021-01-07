/**
 *	Copyright 2020 TSN-SHINGENN All Rights Reserved.
 *	Basic Author: Seiichi Takeda  '2020-November-03 Active
 *
 *	Dual License :
 *	non-commercial ... MIT Licence
 *	    commercial ... Requires permission from the author
 */

/**
 * @file mddl_u8queue.c
 * @brief u8�L���[�i�����O�o�b�t�@�j���C�u����
 **/

#include <stddef.h>
#include <stdio.h>
#include <errno.h>

#include "mddl_u8queue.h"

void __attribute__((weak)) *mddl_malloc(const size_t size)
{
    return malloc(size);
}

void __attribute__((weak)) mddl_free( void *const ptr )
{
    free(ptr);
}

/* �L���[�̎��̑}���ʒu�����߂� */
#define queue_next(s, n) (((n) + 1) % (s)->queue_size)

int mddl_u8queue_init(mddl_u8queue_t *const self_p, const size_t size)
{
    self_p->head = 0;
    self_p->tail = 0;
    self_p->buf = (uint8_t*)mddl_malloc(sizeof(uint8_t)*size);
    if (NULL == self_p->buf) {
	return ERANGE;
    }
    self_p->queue_size = size;

    return 0; 
}

int mddl_u8queue_is_empty(mddl_u8queue_t *const self_p)
{
    return (self_p->head == self_p->tail);
}

int mddl_u8queue_push(mddl_u8queue_t *const self_p, const uint8_t u8)
{
    /* �L���[�����t�ł��邩�m�F���� */
    if(queue_next(self_p, self_p->tail) == self_p->head) {
	return -1;
    }

    /* �L���[�̖����Ƀf�[�^��}������ */
    self_p->buf[self_p->tail] = u8;

    /* �L���[�̎���}���ʒu������ɂ��� */
    self_p->tail = queue_next(self_p, self_p->tail);

    return 0;
}

int mddl_u8queue_pop(mddl_u8queue_t *const self_p, uint8_t *const u8_p)
{
    /* �L���[�Ɏ��o���f�[�^�����݂��邩�m�F���� */
    if(mddl_u8queue_is_empty(self_p)) {
	return -1;
    }

    /* �L���[����f�[�^���擾���� */
    *u8_p = self_p->buf[self_p->head];

    /* ���̃f�[�^�擾�ʒu�����肷�� */
    self_p->head = queue_next(self_p, self_p->head);

    return 0;
}

int mddl_u8queue_destroy(mddl_u8queue_t *const self_p)
{
    mddl_free(self_p->buf);
    return 0;
}