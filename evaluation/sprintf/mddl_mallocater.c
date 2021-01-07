/**
 *      Copyright 20l5 TSNｰSHINGENN All Rights Reserved.
 *
 *      Basic Author: Seiichi Takeda  '2015-April-22 Active
 *              Last Alteration $Author: takeda $
 *
 *	Dual License :
 *	non-commercial ... MIT Licence
 *	    commercial ... Requires permission from the author
 */

/**
 * @file mddl_mallocater.c
 * @brief  フラグメント制御がない、軽いメモリーアロケーター
 *	指定されたメモリ領域内での、動的な割り当てと開放を行います。
 */

#if defined(WIN32)
/* Microsoft Windows Series */
#define _CRT_SECURE_NO_WARNINGS
#if _MSC_VER >= 1400            /* VC++2005 */
#pragma warning ( disable:4996 )
#pragma warning ( disable:4819 )
#endif
#endif

#include <sys/types.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* this */
#include "mddl_mallocater.h"

#ifdef DEBUG
#ifdef __GNUC__
__attribute__ ((unused))
#endif
static int debuglevel = 1;
#else
#ifdef __GNUC__
__attribute__ ((unused))
#endif
static const int debuglevel = 0;
#endif
#include "dbms.h"

#define EOL_CRLF "\n\r"

mddl_mallocater_t _mddl_mallocater_heap_obj;
#define ALLOCATER_ALIGN sizeof(void*)
#define ALLOCATED_FLAG ((uint32_t)(0x1))
#define MAGIC_NO (((uint32_t)'M' << 24 ) | ((uint32_t)'e' << 16 ) |((uint32_t) 'm' << 8) | 0)

#define get_allocater_own() &(_mddl_mallocater_heap_obj)

#define SIZEOF_ALLOCATEHEADER (sizeof(mddl_malllocate_header_t))
#define SIZEOF_ALLOCATEFOOTER (sizeof(void*))
#define HEAD_MAGIC_IS_NG(h) (((h)->stamp.magic_no & ~ALLOCATED_FLAG) != MAGIC_NO )
#define AREA_FOOTER_IS_NG(h)  (*(uintptr_t*)((uintptr_t)(h) + (h)->size - SIZEOF_ALLOCATEFOOTER) != (uintptr_t)h) 
#define GET_FOOTER_PTR(h) ((void*)((uintptr_t)(h) + (h)->size - SIZEOF_ALLOCATEFOOTER))
#define GET_PTR2HEAD(p) ((mddl_malllocate_header_t*)((uintptr_t)(p) - SIZEOF_ALLOCATEHEADER))
#define TOTALAREASIZE(z) ((size_t)((((z) + SIZEOF_ALLOCATEHEADER + SIZEOF_ALLOCATEFOOTER) \
	+ (ALLOCATER_ALIGN - 1 )) &  ~(ALLOCATER_ALIGN - 1)))
#define GET_HEAD2PTR(h) (void*)((uintptr_t)(h) + SIZEOF_ALLOCATEHEADER)
#define GET_BUFSIZE(h) (size_t)((h)->size - (SIZEOF_ALLOCATEHEADER + SIZEOF_ALLOCATEFOOTER))
#define AREA_IS_ALLOC(p) ((p)->stamp.occupied & ALLOCATED_FLAG)
#define AREA_IS_FREE(p) (0x1 & ~AREA_IS_ALLOC(p)) 

#ifdef __GNUC__
__attribute__ ((unused))
#endif
static void *own_memmove( void *const dest, const void *const src, const size_t sz);
static int region_pointer_check(mddl_mallocater_t *const, const mddl_malllocate_header_t * const);

/**
 * @fn static __inline size_t own_simply_memcpy(void *const oDst, const void *const iSrc, const size_t len)
 * @brief 1バイト単位でコピーするシンプルなデータコピー
 * @param oDst コピー先
 * @param iSrc コピー元
 * @param len データ長
 * @return コピーされたバイト数
 */
static __inline size_t own_simply_memcpy(void *const oDst, const void *const iSrc,
				     const size_t len)
{
    size_t i = len;

    if( oDst < iSrc ) {
	const uint8_t *__restrict s_p = (const uint8_t *)(iSrc);
	uint8_t *__restrict d_p = (uint8_t *)(oDst);
        for (; i; --i) {
	    *d_p++ = *s_p++;
        }
    } else if( oDst >iSrc ) {
	const uint8_t *__restrict s_p = (const uint8_t *)((uintptr_t)iSrc + len -1);
	uint8_t *__restrict d_p = (uint8_t *)((uintptr_t)oDst + len -1);
        for (; i; --i) {
	    *d_p-- = *s_p--;
        }
    } else {
	return 0;
    }

    return len;
}

/**
 * @fn static __inline size_t own_uint32_memcpy(void *oDst, const void *iSrc, const size_t len)
 * @brief uint32_tのサイズコピーするシンプルなデータコピー
 * @param oDst コピー先
 * @param iSrc コピー元
 * @param len データ長
 * @return コピーされたバイト数
 */
static __inline size_t own_uint32_memcpy(void *const oDst, const void *const iSrc,
				     const size_t len)
{
    size_t i = (len / sizeof(uint32_t));
    const size_t rlen = i * sizeof(uint32_t);

    if( oDst < iSrc ) {
	const uint32_t *__restrict s_p = (const uint32_t *) (iSrc);
	uint32_t *__restrict d_p = (uint32_t *) oDst;
	for (; i; --i) {
	    *d_p++ = *s_p++;
        }
    } else if( oDst > iSrc ) {
	const uint32_t *__restrict s_p = (const uint32_t *) (((uintptr_t)iSrc)+rlen-sizeof(uint32_t));
	uint32_t *__restrict d_p = (uint32_t *)((uintptr_t)oDst+rlen-sizeof(uint32_t));
	for (; i; --i) {
	    *d_p-- = *s_p--;
        }
    } else {
	return 0;
    }

    return rlen;
}

/** 
 * @fn static void *own_memmove( void *const dest, const void *const src, const size_t sz)
 * @brief データを移動します
 *   バッファ領域は重なる場合、destが前方にあるときのみ保障します。
 * @param dest 転送先バッファ
 * @param src 転送元バッファ
 * @param sz 転送サイズ
 **/
static void *own_memmove( void *const dest, const void *const src, const size_t sz)
{
    size_t remain = sz, rlen;
    const uint8_t * __restrict s_p = (const uint8_t*)src;
    uint8_t * __restrict d_p = (uint8_t*)dest; 

    if( src == dest ) {
	return NULL;
    }

    rlen = own_uint32_memcpy( d_p, s_p, remain);
    if( (remain -= rlen) != 0  ) {
	s_p += rlen;
	d_p += rlen;
	own_simply_memcpy( d_p, s_p, remain);
    }

    return dest;
}

/**
 * @fn int mddl_mallocater_init_obj(mddl_mallocater_t *const self_p, void * const buf, const size_t bufsiz )
 * @brief メモリアロケータオブジェクトインスタンスを初期化します。
 * @param self_p オブジェクトインスタンスポインタ
 * @param buf アロケータで制御するメモリ領域。
 * @param bufsiz bufのサイズ
 * @retval EBUSY 初期化済みで使用中
 * @retval 0 成功
 **/
int mddl_mallocater_init_obj(mddl_mallocater_t *const self_p, void * const buf, const size_t bufsiz )
{
    int result;
    mddl_mallocater_t * const o = self_p;
    mddl_malllocate_header_t *h;
    const uint8_t align = (uintptr_t)buf & (ALLOCATER_ALIGN -1);

    DBMS5( "%s : execute buf=0x%p siz=%llu" EOL_CRLF, __func__,
	    buf, (unsigned long long)bufsiz);

    if( bufsiz < (TOTALAREASIZE(1) * 3)) {
	return ENOMEM;
    }

    memset(  o, 0x0, sizeof(mddl_malllocate_header_t));
    memset(buf, 0x0, bufsiz);

    if( align ) {
	o->bufofs = ALLOCATER_ALIGN - align;
	o->buf = (void*)((uintptr_t)buf + o->bufofs);
	o->bufsiz = bufsiz - o->bufofs;
    } else {
	o->bufofs = 0;
	o->buf = buf;
	o->bufsiz = bufsiz;
    }
    o->bufsiz = TOTALAREASIZE(o->bufsiz - TOTALAREASIZE(0) - ALLOCATER_ALIGN);

    o->base.size = 0;
    o->base.stamp.magic_no = MAGIC_NO;
    o->base.stamp.occupied |= ALLOCATED_FLAG;

    h = o->base.next_p = o->base.prev_p =
	(mddl_malllocate_header_t*)o->buf;

    h->size = o->bufsiz;
    h->stamp.magic_no = MAGIC_NO;
    h->stamp.occupied &= ~ALLOCATED_FLAG;
    h->next_p = h->prev_p = &o->base;
    *(uintptr_t*)GET_FOOTER_PTR(h) = (uintptr_t)h;

    IFDBG5THEN {
	    DMSG( "%s : self_p=%p buf=%p bufsiz=%llu, &self_p->base=%p" EOL_CRLF,
		__func__, self_p, buf, (unsigned long long)bufsiz, &(self_p->base));
    }

    result = region_pointer_check(self_p, h);
    if(result) {
	//DMSG("%s : post(0x%08x) check is error" EOL_CRLF, __FUNCTION__, (uintptr_t)h);
	// _mddl_mallocater_dump_region_list(self_p);
	 abort();
    }
    self_p->init.f.initialized = 1;

    return 0;
}

/**
 * @fn int mddl_mallocater_destroy(mddl_mallocater_t *const self_p)
 * @brief メモリアロケータオブジェクトインスタンスを破棄します。
 *	メモリマッピングが後続の処理に影響しないように管理エリアを0クリアします
 * @param self_p オブジェクトインスタンスポインタ
 */
int mddl_mallocater_destroy(mddl_mallocater_t *const self_p)
{
    mddl_mallocater_t * const o = self_p;

    if(self_p->init.f.initialized) {
	return EINVAL;
    }

    memset( o->buf, 0x0, o->bufsiz); 
    self_p->init.f.initialized = 0;

    return 0;
}


/**
 * @fn void *mddl_mallocater_alloc_with_obj(mddl_mallocater_t *const self_p, const size_t sz)
 * @brief メモリの空き領域から線形領域を確保します。
 * @param self_p オブジェクトインスタンスポインタ
 * @param sz 確保領域
 * @retval NULL 確保できない
 * @retval NULL以外 確保したメモリのポインタ
 **/
void *mddl_mallocater_alloc_with_obj(mddl_mallocater_t *const self_p, const size_t sz)
{
    mddl_malllocate_header_t *p;
    int result;
    const size_t totalsz = TOTALAREASIZE(sz); // Ceilling
    void *retptr = NULL;

    DBMS5( "mddl_mallocater_alloc_with_obj : execute" EOL_CRLF);

    if(!self_p->init.f.initialized ) {
	errno = EPERM;
	return NULL;
    } else if ( totalsz == 0 ) {
	// DMSG( "mddl_mallocater_alloc_with_obj : totalsz=0:sz=%d" EOL_CRLF, sz);
	errno = EINVAL;
	return NULL;
    }

    IFDBG3THEN {
	DBMS3( "%s : header size = %llu : malloc size = %llu, totalsize = %llu" EOL_CRLF,
	    __func__, (unsigned long long)SIZEOF_ALLOCATEHEADER, (unsigned long long)sz, (unsigned long long)totalsz);
    }

    /* 指定サイズと同等があるか探す */
    DBMS3( "%s : search free area of same size" EOL_CRLF, __func__);
    for( p=self_p->base.next_p; p != &self_p->base; p=p->next_p ) {
	if( AREA_IS_FREE(p) && (p->size == totalsz)) {
	    DBMS3( "%s : Found same size area" EOL_CRLF, __func__);
	    /* ヘッダを使用中にしてバッファエリアのポインタを戻す */
	    p->stamp.magic_no =  MAGIC_NO;
	    p->stamp.occupied |= ALLOCATED_FLAG;
	    *(uintptr_t*)GET_FOOTER_PTR(p) = (uintptr_t)p;
	    retptr = GET_HEAD2PTR(p);
	    break;
        }
    }

    IFDBG3THEN {
	if( NULL == retptr ) {
	    DBMS3( "%s : Not found same size area" EOL_CRLF, __func__);
	}
    }

    /* 指定サイズよりも大きいものを探す */
    DBMS3( "%s : Search free area of bigger area" EOL_CRLF, __func__);
    for( p=self_p->base.next_p; (NULL == retptr) && (p != &self_p->base); p=p->next_p ) {
	if( AREA_IS_FREE(p) && (p->size > totalsz)) {
	    DBMS3( "Found area" EOL_CRLF);
	    if((p->size - totalsz) < TOTALAREASIZE(1) ) {
		/**
		 * @note 後半ブロックを使うにはサイズが小さすぎるので、
		 * そのままのサイズを割り当てる
		 **/

		/* ヘッダの再構成　*/
		p->stamp.magic_no = MAGIC_NO;
		p->stamp.occupied |= ALLOCATED_FLAG;
		*(uintptr_t*)GET_FOOTER_PTR(p) = (uintptr_t)p;
		retptr = GET_HEAD2PTR(p);
	    } else {
		/* 後半を空き領域にする */
		mddl_malllocate_header_t * const s =
			(mddl_malllocate_header_t*)((uintptr_t)p+totalsz);
		s->size = p->size - totalsz;
		s->stamp.magic_no = MAGIC_NO;
		s->stamp.occupied &= ~ALLOCATED_FLAG;
		*(uintptr_t*)GET_FOOTER_PTR(s) = (uintptr_t)s;

		/* sを双方向リンクに追加 */
		(p->next_p)->prev_p = s;
		s->next_p = p->next_p;
		p->next_p = s;
		s->prev_p = p;

		IFDBG3THEN {
		    DBMS3( "%s : p=0x%p s=0x%p" EOL_CRLF, __func__, p, s);
		    DBMS3( "%s : p->prev_p=0x%p p->next_p=0x%p s->prev_p=0x%p s->next_p=0x%p" EOL_CRLF,
			__func__, p->prev_p, p->next_p, s->prev_p, s->next_p);
		}

		/* 前半の長さを調整して使用中のマークをつける */
		p->size = totalsz;
		p->stamp.magic_no = MAGIC_NO;
		p->stamp.occupied |= ALLOCATED_FLAG;
		*(uintptr_t*)GET_FOOTER_PTR(p) = (uintptr_t)p;
		retptr = GET_HEAD2PTR(p);
	    }
	}
    }
	   
    if(NULL != retptr) {
	p = GET_PTR2HEAD(retptr); 
	IFDBG3THEN {
	    DBMS3( "%s : retptr=0x%p" EOL_CRLF, __func__, retptr);
	}

	IFDBG4THEN {
	    _mddl_mallocater_dump_region_list(self_p);
	}

	result = region_pointer_check(self_p, p);
	if(result) {
	    // DMSG("%s : post(0x%08x) check is error" EOL_CRLF, myfunc, (uintptr_t)p);
	    // _mddl_mallocater_dump_region_list(self_p);
	    abort();
	}
    	return retptr;
    }

    // DMSG("%s : ENOMEM" EOL_CRLF, myfunc);

    /* 空き領域を用意できなかった */
    errno = ENOMEM;
    return NULL;
}

static int region_pointer_check(mddl_mallocater_t *const self_p, const mddl_malllocate_header_t * const cur)
{
    const mddl_malllocate_header_t *p = cur->prev_p, *n = cur->next_p;
    int result = 0;

    /* 指定位置の周囲のポインタがあっているかチェックする */

    /* 現在地 */
    if( HEAD_MAGIC_IS_NG(cur) ) {
	// DMSG(  "cur(%08x)  MAGIC no is ignore, or invalid" EOL_CRLF, c);
	result |= ~0;
    }

    if( AREA_IS_ALLOC(cur) && AREA_FOOTER_IS_NG(cur) ) {
	// DMSG(  "cur(%08x) area was overflow" EOL_CRLF, (uintptr_t)c);
	result |= ~0;
    }

    /* 前の領域 */
    if( &self_p->base != p ) {
	if ( HEAD_MAGIC_IS_NG(p) || (AREA_IS_ALLOC(p) && AREA_FOOTER_IS_NG(p)) ) {
	    // DMSG(  "cur(%08x)->prev_p is NG(%08x)" EOL_CRLF, (uintptr_t)c, (uintptr_t)p);
	    result |= ~0;
	} 
    } 

    /* 後の領域 */
    if( &self_p->base != n && AREA_IS_ALLOC(n)) {
	if ( HEAD_MAGIC_IS_NG(p) || (AREA_IS_ALLOC(p) && AREA_FOOTER_IS_NG(p)) ) {
	    // DMSG(  "cur(%08x)->next_p is NG(%08x)" EOL_CRLF, (uintptr_t)c, (uintptr_t)n);
	    result |= ~0;
	}
    } 
    return result;
}

/**
 * @fn void mddl_mallocater_free_with_obj(mddl_mallocater_t *const self_p, void * const ptr)
 * @brief 確保した線形領域を開放します
 * @param self_p オブジェクトインスタンスポインタ
 * @param ptr 開放するバッファのポインタ
 */
void mddl_mallocater_free_with_obj(mddl_mallocater_t *const self_p, void * const ptr)
{
    mddl_malllocate_header_t *cur;
    int result;
    const char *const myfunc=__FUNCTION__;
    DBMS5(  "mddl_mallocater_free_with_obj : execute" EOL_CRLF);

    if(!self_p->init.f.initialized ) {
	errno = EPERM;
	abort();
    }
    if(!ptr) return;

    /* dealloc するヘッダを得る */
    cur = GET_PTR2HEAD(ptr);

    /* ヘッダー フッターのチェック */
    result = region_pointer_check(self_p, cur);
    if(result) {
	DBMS("%s : pre chk err" EOL_CRLF, myfunc);
	// _mddl_mallocater_dump_region_list(self_p);
	abort();
    }

    /* もし直前が空きブロックだったら、 併合して1つの領域にする */
//    if (!(cur->prev_p->stamp.occupied & ALLOCATED_FLAG) ) {
    if (AREA_IS_FREE(cur->prev_p)) {
	(cur->next_p)->prev_p = cur->prev_p;
	(cur->prev_p)->next_p = cur->next_p;
	(cur->prev_p)->size += cur->size;
	cur = cur->prev_p;
    }

    /* もし、 直後が空きブロックだったら、 併合して1つの領域にする */
//    if (!(cur->next_p->stamp.occupied & ALLOCATED_FLAG)) {
    if (AREA_IS_FREE(cur->next_p)) {
	((cur->next_p)->next_p)->prev_p = cur;
	cur->size  += (cur->next_p)->size;
	cur->next_p = (cur->next_p)->next_p;
    }

    /* 空きブロックのマークをつける */
    cur->stamp.occupied = MAGIC_NO;
    cur->stamp.occupied &= ~ALLOCATED_FLAG;
    *(uintptr_t*)GET_FOOTER_PTR(cur) = (uintptr_t)cur;

    /* ヘッダー フッターの再チェック */
    result = region_pointer_check(self_p, cur);
    if(result) {
	DBMS("%s : post chk err" EOL_CRLF, myfunc);
	// _mddl_mallocater_dump_region_list(self_p);
	abort();
    }

    return;
}

/**
 * @fn void _mddl_mallocater_dump_region_list(void)
 * @brief 現在の領域確保マップ（リージョンテーブル）をダンプします。
 * @param self_p オブジェクトインスタンスポインタ
 **/
void _mddl_mallocater_dump_region_list(mddl_mallocater_t const *const self_p)
{
    volatile mddl_malllocate_header_t const *p; 
    unsigned int n;
    const char *flag, *magic_is;
    const char * const dfmt = "%04d:p=0x%p(buf_ptr:%p) magic_is=%s size=%u FLAG=%s next_ptr=%p" EOL_CRLF;

    DBMS5(  "%s : execute" EOL_CRLF, __func__);

    DMSG(  "mallocater_dump_region" EOL_CRLF);
    DMSG(  "self_p=0x%p &self_p->base=0x%p" EOL_CRLF, self_p, &(self_p->base));


    /* 後方確認 */
    DMSG(  "backword list" EOL_CRLF);
    for( p=self_p->base.next_p, n=0; p != &self_p->base; p=p->next_p, ++n ) {
	magic_is = HEAD_MAGIC_IS_NG(p) ? "NG" : "OK";
	flag = AREA_IS_ALLOC(p) ? (AREA_FOOTER_IS_NG(p) ? "allocNG" :"allocOK") : "free";
	// dfmt = "%04d:p=0x%p(buf_ptr:%p) magic_is=%s size=%u FLAG=%s next_ptr=%p" EOL_CRLF;
	DMSG(  dfmt, n, p, GET_HEAD2PTR(p), magic_is, (unsigned)p->size, flag, &(p->next_p));
    }

    /* 前方確認 */
    DMSG(  "forword list" EOL_CRLF);
    for( p=self_p->base.prev_p; p != &self_p->base; p=p->prev_p, --n ) {
	magic_is = HEAD_MAGIC_IS_NG(p) ? "NG" : "OK";
	flag = AREA_IS_ALLOC(p) ? (AREA_FOOTER_IS_NG(p) ? "allocNG" :"allocOK") : "free";
	// dfmt = "%04d:p=0x%p(buf_ptr:%p) magic_is=%s size=%u FLAG=%s next_ptr=%p" EOL_CRLF;
	DMSG(  dfmt, n, p, GET_HEAD2PTR(p), magic_is, (unsigned)p->size, flag, &(p->next_p));
    }

    return;
}

/**
 * @fn void *mddl_mallocater_realloc_with_obj(mddl_mallocater_t *const self_p, void *const ptr, const size_t size)
 * @brief ポインター ptr が示すメモリーブロックのサイズを size バイト に変更する。
 *     開発途上なのでまだ使えません。 * @param ptr 変更前のバッファポインタ
 * @param self_p オブジェクトインスタンスポインタ
 * @param size 変更後のサイズ
 * @retval NULL 失敗
 * @retval NULL以外 成功
 **/
void *mddl_mallocater_realloc_with_obj(mddl_mallocater_t *const self_p, void *const ptr, const size_t size)
{
     void *retptr = NULL;
	(void)self_p;
    (void)ptr;
    (void)size;
#if 0
    mddl_mallocater_t * const o = self_p;
    mddl_malllocate_header_t *cur;
    int result;
    void *retptr = NULL;
    const char *const myfunc = __FUNCTION__;

    DBMS5(  "%s : execute" EOL_CRLF, myfunc);

    if( NULL == ptr ) {
	return mddl_mallocater_alloc_with_obj(o, size);
    }

    /* 指定された領域がオーバーフローしていないことを確認する */
    cur = GET_PTR2HEAD (ptr);
    result = region_pointer_check(self_p, cur);
    if(result) {
	// DMSG(  "%s : cur prechk err" EOL_CRLF, myfunc);
	// _mddl_mallocater_dump_region_list(self_p);
	abort();
    }
    /* reallocサイズが小さい場合 */
    if( GET_BUFSIZE(cur) >= size ) {
	size_t freearea = GET_BUFSIZE(cur) - size;
	if( freearea > TOTALAREASIZE(1) ) {
	    const size_t totalsz = freearea - GET_BUFSIZE(cur);
	    mddl_malllocate_header_t *new = (mddl_malllocate_header_t*)((intptr_t)cur+cur->size);
	    new->stamp.magic_no = MAGIC_NO;
	    new->stamp.occupied &= ~ALLOCATED_FLAG;
	    new->size = totalsz;
	    *(uintptr_t*)GET_FOOTER_PTR(new) = (uintptr_t)new;
	    /* リンクリストに追加 */
	    (cur->next_p)->prev_p = (void*)new;
	    new->next_p = cur->next_p;
	    cur->next_p = (void*)new;
	    new->prev_p = (void*)cur;
	    retptr = ptr;
	}
    } 
    /* reallocサイズが大きい場合 */
    if ((retptr == NULL) &&(cur->next_p != &o->base)) {
        /* 後方が空き領域だったら合併できるか確かめる */
	mddl_malllocate_header_t *n = (mddl_malllocate_header_t*)cur->next_p;
	/* 後方バッファが空であることを確認する */
	if(AREA_IS_FREE(n)) {
	    const size_t totalsz = TOTALAREASIZE(size);
	    if((cur->size + n->size) <= totalsz ) {
		const size_t freearea = totalsz - (cur->size + n->size);
		if( freearea == 0 || (freearea < TOTALAREASIZE(1))) {
		    /* 後方エリアを破棄してつなげる */
		    cur->size += n->size;
		    (n->next_p)->prev_p = cur;
		    cur->next_p = n->next_p;
		    *(uintptr_t*)GET_FOOTER_PTR(cur) = (uintptr_t)cur;
		    retptr = ptr; /* 完了 */
		} else {
		    /* 後方エリアを再定義する */
		    n = (mddl_malllocate_header_t*)((uintptr_t)cur + totalsz);
		    n->size = freearea;
		    n->stamp.magic_no = MAGIC_NO;
		    n->stamp.occupied &= ~ALLOCATED_FLAG;
		    *(uintptr_t*)GET_FOOTER_PTR(n) = (uintptr_t)n;

		    /* sを双方向リンクに追加 */
		    (cur->next_p)->prev_p = (void*)n;
		    n->next_p = cur->next_p;
		    cur->next_p = (void*)n;
		    n->prev_p = (void*)cur;
		    retptr = ptr; /* 完了 */
		}
	    }
	}
    }

    /* 前方が空き領域だったら合併できるか確かめる */
    if((NULL == retptr) && (cur->prev_p != &o->base) ) {
	mddl_malllocate_header_t *b = (mddl_malllocate_header_t*)cur->prev_p;
	/* 前方バッファが空であることを確認する */
	if(AREA_IS_FREE(b)) {
	    const size_t totalsz = TOTALAREASIZE(size);
	    if((cur->size + b->size) <= totalsz ) {
		const size_t freearea = totalsz - (cur->size + b->size);
		if( freearea > TOTALAREASIZE(0) ) {
		    /* 残りのエリアがヘッダ部よりも大きかったら空エリアとして登録 */
		    mddl_malllocate_header_t *new = (mddl_malllocate_header_t*)((intptr_t)cur + totalsz);
		    /* 最初にデータを移動する */
		    own_memmove( GET_HEAD2PTR(b), ptr, cur->size - SIZEOF_ALLOCATEHEADER);
		    /* ヘッダ作成 */
		    new->size = freearea;
		    new->stamp.magic_no = MAGIC_NO;
		    new->stamp.occupied &= ~ALLOCATED_FLAG;
		    /* newを双方向リンクに追加 */
		    (b->next_p)->prev_p = (void*)new;
		    new->next_p = b->next_p;
		    b->next_p = (void*)new;
		    new->prev_p = b;

		    retptr = GET_HEAD2PTR(b); /* 完了 */
		} else {
		    /* サイズを調整して結合する */
		    b->size += cur->size;
		    b->stamp.occupied |= ALLOCATED_FLAG;
		    *(uintptr_t*)GET_FOOTER_PTR(b) = (uintptr_t)b;
		    /* カレントを破棄する */
		    b->next_p = (void*)cur->next_p;
		    (cur->next_p)->prev_p = (void*)b;

		    /* 最後にデータを移動する */
		    own_memmove( GET_HEAD2PTR(b), ptr, cur->size - SIZEOF_ALLOCATEHEADER);
		    retptr = GET_HEAD2PTR(b); /* 完了 */
		}
	    }
	}
    }

    if( NULL == retptr ) {
	/* 他に空き領域があったら入る場所を探す */
	void *  __restrict new_ptr = NULL;
	new_ptr = mddl_mallocater_alloc(size);
	if( NULL != new_ptr ) {
	    own_memmove( new_ptr, ptr, cur->size);
	    mddl_mallocater_free(ptr);
	    retptr = new_ptr;
	}
    }

    if( NULL != retptr) {
	/* 最後にチェック */
	cur = GET_PTR2HEAD (ptr);
	result = region_pointer_check(self_p, cur);
        if(result) {
	    // DMSG(  "%s : cur postchk err" EOL_CRLF, myfunc);
	    // _mddl_mallocater_dump_region_list(self_p);
	    abort();
	}
    } else {
	/* だめでした */
	errno = ENOMEM;
    }
#else
    errno = ENOSYS;
#endif    
    return retptr;
}

/**
 * @fn void *mddl_mallocater_alloc(const size_t size)
 * @brief CRL allocをエミュレートするためのラッパ関数
 *     注意） MMUがなくても利用可能なように、簡易な処理になっています。
 *	　　　バッファーオーバランについては mddl_mallocater_free()で一応確認します。
 * @param size 割り当てるメモリバッファサイズ
 * @retval NULL 割り当て失敗（errno番号参照）
 *	EPERM オブジェクトが初期化されていない
 *	EINVAL sizeが不正
 *	ENOMEM メモリが足りない
 * @retval NULL以外 割り当て成功
 **/
void *mddl_mallocater_alloc(const size_t size)
{
    mddl_mallocater_t * const o = get_allocater_own();

    return mddl_mallocater_alloc_with_obj( o, size);
}

/**
 * @fn void mddl_mallocater_free(void *const ptr)
 * @brief CRL freeをエミュレートするためのラッパ関数
 *  ptrがNULL以外で、割り当て管理されていない場合はabort()を実行します
 * @param ptr 割り当てられたバッファのポインタ
 **/
void mddl_mallocater_free(void *const ptr)
{
    mddl_mallocater_t * const o = get_allocater_own();

    mddl_mallocater_free_with_obj(o, ptr);
}

/**
 * @fn void *mddl_mallocater_realloc(void *const ptr, const size_t size)
 * @brief CRL reallocをエミュレートするためのラッパ関数
 *  ptrがNULL指定の場合はmddl_mallocater_alloc()と同等の処理をします。
 *　リサイズ前のバッファがオーバランしていれば，abort()処理を実行します。
 * @param ptr リサイズ前のバッファ
 * @param size リサイズ後のサイズ
 * @retval NULL 割り当て失敗(errno参照)
 * @retval NULL以外 リサイズしたバッファポインタ（ptrと値が異なり場合は、データをコピー済）
 **/
void *mddl_mallocater_realloc(void *const ptr, const size_t size)
{
    mddl_mallocater_t * const o = get_allocater_own();

    return mddl_mallocater_realloc_with_obj( o, ptr, size);
}

/**
 * @fn size_t mddl_mallocater_avphys_with_obj(void *const ptr)
 * @breif 管理しているメモリ領域のFREE領域を戻します。
 * @retval 0以上 FREE領域のトータルサイズ
 **/
size_t mddl_mallocater_avphys_with_obj(mddl_mallocater_t *const self_p)
{
    mddl_mallocater_t * const o = self_p;
    mddl_malllocate_header_t const *p; 
    size_t freearea_sz = 0;

    /* 後方確認 */
    for( p=o->base.next_p; p != &o->base; p=p->next_p ) {
	if(AREA_IS_FREE(p)) { 
	    freearea_sz += (int64_t)p->size;
	}
    }

    return freearea_sz;
}

/**
 * @fn size_t mddl_mallocater_phys_with_obj( mddl_mallocater_t *const self_p)
 * @brief 管理しているメモリ領域の全領域を戻します。
 */
size_t mddl_mallocater_phys_with_obj( mddl_mallocater_t *const self_p)
{
    return self_p->bufsiz;
}
