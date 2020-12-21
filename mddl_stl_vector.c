/**
 *      Copyright 2012 TSN-SHINGENN All Rights Reserved.
 *
 *      Basic Author: Seiichi Takeda  '2012-November-17 Active
 *              Last Alteration $Author: takeda $
 *
 *	Dual License :
 *	non-commercial ... MIT Licence
 *	    commercial ... Requires permission from the author
 */

/**
 * @file mddl_stl_vector.c
 * @brief 配列ライブラリ STLのvectorクラス互換です。
 *      なのでスレッドセーフではありません。上位層で処理を行ってください。
 *      STL互換と言ってもCの実装です、オブジェクトの破棄の処理はしっかり実装してください
 */

/* CRL */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* this */
#include "mddl_stl_vector.h"

/* Debug */
#if defined(__GNUC__)
__attribute__ ((unused))
#endif
#ifdef DEBUG
static int debuglevel = 4;
#else
static const int debuglevel = 0;
#endif
#include "dbms.h"

#if defined(_MDDL_DMSG_IS_UART)
#define EOL_CRLF "\n\r"
#else
#define EOL_CRLF "\n"
#endif

typedef union _mddl_stl_vector_stat {
    unsigned int flags;
    struct {
	unsigned int mem_fixed:1; /* mem_fixedを使った際の制限 */
    } f;
} mddl_stl_vector_stat_t;

typedef struct _mddl_stl_vector_ext {
    void *buf;
    size_t reserved_bytes;
    size_t num_elements;
    size_t sizof_element;

    mddl_stl_vector_stat_t stat;
} mddl_stl_vector_ext_t;

#define get_vector_ext(s) (mddl_stl_vector_ext_t*)((s)->ext)
#define get_const_vector_ext(s) (const mddl_stl_vector_ext_t*)((s)->ext)

static void *own_malloc(const size_t size);
static void *own_mrealloc( void *const ptr, const size_t size);
static void own_mfree( void *const ptr );

static void *own_malloc(const size_t size)
{
    return malloc(size);
}

static void *own_mrealloc( void *const ptr, const size_t size)
{
    return realloc( ptr, size);
}

static void own_mfree( void *const ptr )
{
    free(ptr);
    return;
}

/**
 * @fn int mddl_stl_vector_init( mddl_stl_vector_t *const self_p, const size_t sizof_element)
 * @brief vectorオブジェクトを初期化します。
 * @param self_p mddl_stl_vector_t構造体インスタンスポインタ
 * @param sizof_element 要素サイズ
 * @retval 0 成功
 * @retval EAGAIN リソース獲得に失敗
 * @retval -1 エラー定義以外の致命的な失敗
 */
int mddl_stl_vector_init( mddl_stl_vector_t *const self_p, const size_t sizof_element)
{
    mddl_stl_vector_ext_t *e = NULL;
    memset(self_p, 0x0, sizeof(mddl_stl_vector_t));

    e = (mddl_stl_vector_ext_t *)
	own_malloc(sizeof(mddl_stl_vector_ext_t));
    if (NULL == e) {
	DBMS1("%s : mddl_malloc(ext) fail" EOL_CRLF, __func__);
	return EAGAIN;
    }
    memset(e, 0x0, sizeof(mddl_stl_vector_ext_t));

    e->buf = NULL;
    e->reserved_bytes = 0;
    e->num_elements = 0;
    self_p->sizof_element = e->sizof_element = sizof_element;

    self_p->ext = e;

    return 0;
}

/**
 * @fn int mddl_stl_vector_destroy( mddl_stl_vector_t *const self_p)
 * @brief vectorオブジェクトを破棄します
 * @param self_p mddl_stl_vector_t構造体インスタンスポインタ
 * @return 0固定
 */
int mddl_stl_vector_destroy(mddl_stl_vector_t *const self_p)
{
    mddl_stl_vector_ext_t *const e =
	get_vector_ext(self_p);

    if( NULL == e ) {
	return 0;
    }

    if (NULL != e->buf) {
	own_mfree(e->buf);
	e->buf = NULL;
    }

    own_mfree(self_p->ext);
    self_p->ext = NULL;

    return 0;
}

/**
 * @fn static int resize_buffer( mddl_stl_vector_ext_t *const e, const size_t num_elements, const void *const el_p)
 * @brief vectorバッファを可変します。拡張時はel_p(NULL以外)に設定された要素で埋めることができます
 * @param e mddl_stl_vector_ext_t構造体ポインタ
 * @param num_elements 要素格納数
 * @param el_p 拡張時に設定する要素データポインタ(NULLで設定しない)
 * @retval 0 成功
 * @retval EAGAIN リソース不足　
 */
static int resize_buffer(mddl_stl_vector_ext_t *const e,
			 const size_t num_elements, const void *const el_p)
{
    const size_t new_reserve = e->sizof_element * num_elements;
    void *new_buf = NULL;

    if (e->reserved_bytes < new_reserve) {
	new_buf = own_mrealloc(e->buf, new_reserve);
	if (NULL == new_buf) {
	    return EAGAIN;
	}
	e->buf = new_buf;
	e->reserved_bytes = new_reserve;
    }

    if (e->num_elements < num_elements) {
	/* expand */
	if (NULL != el_p) {
	    size_t st_pos = e->num_elements;
	    size_t end_pos = num_elements;
	    size_t n;
	    if (e->sizof_element > 1) {
		for (n = st_pos; n < end_pos; n++) {
		    void *at =
			(uint8_t *) e->buf + (n * e->sizof_element);
		    memcpy(at, el_p, e->sizof_element);
		}
		e->num_elements = num_elements;
	    } else {
		void *p =
		    (uint8_t *) e->buf + (st_pos * e->sizof_element);
		size_t dur = end_pos - st_pos;

		memset(p, *(uint8_t *) el_p, dur);
		e->num_elements = num_elements;
	    }
	}
    } else {
	e->num_elements = num_elements;
    }

    return 0;
}

/**
 * @fn int mddl_stl_vector_push_back( mddl_stl_vector_t *const self_p, const void *const el_p, const size_t sizof_element)
 * @brief 末尾に要素を追加します
 * @param self_p mddl_stl_vector_t構造体インスタンスポインタ
 * @param el_p 追加する要素のポインタ
 * @param sizof_element 要素サイズ
 * @retval 0 成功
 * @retval EAGAIN リソース不足
 * @retval EINVAL 引数不正
 */
int mddl_stl_vector_push_back(mddl_stl_vector_t *const self_p,
				 const void *const el_p,
				 const size_t sizof_element)
{
    mddl_stl_vector_ext_t *const e =
	get_vector_ext(self_p);
    int result;

    DBMS3("%s : execute" EOL_CRLF, __func__);

    if (sizof_element != e->sizof_element) {
	return EINVAL;
    }
    if (NULL == el_p) {
	return EINVAL;
    }

    result = resize_buffer(e, e->num_elements + 1, el_p);
    if (result) {
	return result;
    }

    return 0;
}

/**
 * @fn int mddl_stl_vector_pop_back( mddl_stl_vector_t *const self_p)
 * @brief 末尾の要素を削除します
 * @param self_p mddl_stl_vector_t構造体インスタンスポインタ
 * @retval 0 成功
 * @retval ENOENT 削除する要素が存在しない
 */
int mddl_stl_vector_pop_back(mddl_stl_vector_t *const self_p)
{
    mddl_stl_vector_ext_t *const e =
	get_vector_ext(self_p);

    if (e->num_elements == 0) {
	return ENOENT;
    }

    --(e->num_elements);

    return 0;
}

/**
 * @fn int mddl_stl_vector_resize( mddl_stl_vector_t *const self_p, const size_t num_elements, const void *const el_p, const size_t sizof_element)
 * @brief 要素を後方から補充・削除します
 * @param self_p mddl_stl_vector_t構造体インスタンスポインタ
 * @param num_elements 変更する総要素数
 * @param el_p 補充する要素データ（削除の場合はNULL指定）
 * @param sizof_element 要素サイズ（削除の場合は0指定)
 * @retval 0 成功
 * @retval EINVAL 不正な引数（オブジェクトや要素サイズ）
 * @retval EAGAIN リソース不足
 * @retval EPERM 関数の実行に許可がない
 */
int mddl_stl_vector_resize(mddl_stl_vector_t *const self_p,
			      const size_t num_elements, const void *const el_p,
			      const size_t sizof_element)
{
    mddl_stl_vector_ext_t *const e =
	get_vector_ext(self_p);
    int result;

    if( NULL == self_p ) {
	return EINVAL;
    } else if( e->stat.f.mem_fixed ) {
	return EPERM;
    } if (e->num_elements == num_elements) {
	return 0;
    } else if (e->num_elements < num_elements) {
	/* extpand */
	if (sizof_element != e->sizof_element) {
	    return EINVAL;
	}
    }
    result = resize_buffer(e, num_elements, el_p);
    if (result) {
	return result;
    }

    return 0;
}

/**
 * @fn int mddl_stl_vector_reserve( mddl_stl_vector_t *const self_p, const size_t num_elements);
 * @brief メモリの予約サイズを増やします
 * @param self_p mddl_stl_vector_t構造体インスタンスポインタ
 * @param num_elements メモリ割当済み総エレメント数
 * @retval 0 成功
 * @retval EINVAL 不正な引数（オブジェクトや要素サイズ）
 * @retval EBUSY 要素が使用中である
 * @retval EAGAIN リソースが獲得できなかった
 */
int mddl_stl_vector_reserve(mddl_stl_vector_t *const self_p,
			       const size_t num_elements)
{
    mddl_stl_vector_ext_t *const e =
	get_vector_ext(self_p);

    if( NULL == self_p ) {
	return EINVAL;
    } else if( e->stat.f.mem_fixed ) {
	return EPERM;
    } else if (e->num_elements >= num_elements) {
	return EBUSY;
    }

    return resize_buffer(e, num_elements, NULL);
}

/**
 * @fn size_t mddl_stl_vector_capacity( mddl_stl_vector_t *const self_p)
 * @brief メモリの確保済みサイズを得る
 * @param self_p mddl_stl_vector_t構造体インスタンスポインタ
 * @return  確保（予約済み）要素数
 */
size_t mddl_stl_vector_capacity(mddl_stl_vector_t *const self_p)
{
    const mddl_stl_vector_ext_t *const e =
	get_const_vector_ext(self_p);

    return e->reserved_bytes / e->sizof_element;
}

/**
 * @fn void *mddl_stl_vector_ptr_at( mddl_stl_vector_t *const self_p, const size_t num)
 * @brief 要素の割当ポインタを得る
 *	※ capacityサイズを超えてコンテナが収容されるとポインタアドレスが変更されることがあります。
 *	スレッドセーフではないので、上位で排他制御してください。
 * @param self_p mddl_stl_vector_t構造体インスタンスポインタ
 * @param num 割当の要素番号
 * @retval NULL 不正な要素番号
 * @retval NULL以外 要素のポインタ
 */
void *mddl_stl_vector_ptr_at(mddl_stl_vector_t *const self_p, const size_t num)
{
    mddl_stl_vector_ext_t *const e =
	get_vector_ext(self_p);

    if (e->num_elements > num) {
	return (void *) ((uint8_t *) e->buf + (num * e->sizof_element));
    }

    return NULL;
}

/**
 * @fn int mddl_stl_vector_is_empty( mddl_stl_vector_t *const self_p)
 * @brief コンテナに要素が空かどうかを返す
 * @param self_p mddl_stl_vector_t構造体インスタンスポインタ
 * @retval 0以外 空
 * @retval 0 要素あり
 */
int mddl_stl_vector_is_empty(mddl_stl_vector_t *const self_p)
{
    mddl_stl_vector_ext_t *const e =
	get_vector_ext(self_p);

    if (e->num_elements == 0) {
	return 1;
    }

    return 0;
}

/**
 * @fn size_t mddl_stl_vector_size( mddl_stl_vector_t *const self_p)
 * @brief コンテナに入っている要素数を得る
 * @param self_p mddl_stl_vector_t構造体インスタンスポインタ
 * @return 要素数
 */
size_t mddl_stl_vector_size(mddl_stl_vector_t *const  self_p)
{
    mddl_stl_vector_ext_t *const e =
	get_vector_ext(self_p);

    return e->num_elements;
}

/**
 * @fn int mddl_stl_vector_clear( mddl_stl_vector_t *const self_p)
 * @brief コンテナの要素および内部バッファを削除します。
 * @param self_p mddl_stl_vector_t構造体インスタンスポインタ
 * @retval 0 成功
 */
int mddl_stl_vector_clear(mddl_stl_vector_t *const self_p)
{
    mddl_stl_vector_ext_t *const e =
	get_vector_ext(self_p);

    if(NULL == self_p) {
	return EINVAL;
    } else if( e->stat.f.mem_fixed ) {
	e->num_elements = 0;
	return 0;
    }

    if (NULL != e->buf) {
	own_mfree(e->buf);
	e->buf = NULL;
    }
    e->num_elements = 0;
    e->reserved_bytes = 0;

    return 0;
}


/**
 * @fn size_t mddl_stl_vector_get_pool_cnt( mddl_stl_vector_t *const self_p)
 * @brief 内包しているエレメント数を返します．push_backで保存されたエレメントに限ります。
 * @param self_p mddl_stl_vector_t構造体インスタンスポインタ
 * @return 0以上 要素数
 */
size_t mddl_stl_vector_get_pool_cnt( mddl_stl_vector_t *const self_p)
{
    mddl_stl_vector_ext_t *const e =
	get_vector_ext(self_p);

    return e->num_elements;
}

/**
 * @fn int mddl_stl_vector_front( mddl_stl_vector_t *const self_p, void *const el_p, const size_t sizof_element)
 * @brief 先頭に保存されたエレメントを返します
 * @param self_p mddl_stl_vector_t構造体インスタンスポインタ
 * @param el_p 取得する要素のバッファポインタ
 * @param sizof_element 要素サイズ
 * @retval 0 成功
 * @retval EINVAL 要素サイズが異なる
 * @retval ENOENT 先頭に保存された要素がない
 **/
int mddl_stl_vector_front( mddl_stl_vector_t *const self_p, void *const el_p, const size_t sizof_element)
{
    mddl_stl_vector_ext_t *const e =
	get_vector_ext(self_p);

    if( e->num_elements == 0 ) {
	return ENOENT;
    } else if(e->sizof_element != sizof_element) {
	return EINVAL;
    }
    memcpy( el_p, e->buf, e->sizof_element);

    return 0;
}

/**
 * @fn int mddl_stl_vector_back( mddl_stl_vector_t *const self_p, void *const el_p, const size_t sizof_element)
 * @brief 最後に保存されたエレメントを返します
 * @param self_p mddl_stl_vector_t構造体インスタンスポインタ
 * 	*** 検証中です ***
 * @param el_p 取得する要素のバッファポインタ
 * @param sizof_element 要素サイズ
 * @retval 0 成功
 * @retval EINVAL 要素サイズが異なる
 * @retval ENOENT 先頭に保存された要素がない
 **/
int mddl_stl_vector_back( mddl_stl_vector_t *const self_p, void *const el_p, const size_t sizof_element)
{
    mddl_stl_vector_ext_t *const e =
	get_vector_ext(self_p);

    if( e->num_elements == 0 ) {
	return ENOENT;
    } else if(e->sizof_element != sizof_element) {
	return EINVAL;
    }
    memcpy( el_p, (uint8_t*)e->buf + (e->sizof_element * (e->num_elements -1)), e->sizof_element);

    return 0;
}

/**
 * @fn int mddl_stl_vector_insert( mddl_stl_vector_t *const self_p, const size_t num, const void *const el_p, const size_t sizof_element)
 * @brief 要素を挿入します。
 * @param self_p mddl_stl_vector_t構造体インスタンスポインタ
 * @param num 要素番号
 * @param el_p 挿入する要素のデータバッファポインタ
 * @param sizof_element 要素サイズ
 * @retval EINVAL 要素サイズが異なる
 * @retval ENOENT 不正な要素番号
 * @retval EINVAL 関数実行の不許可
 */ 
int mddl_stl_vector_insert( mddl_stl_vector_t *const self_p, const size_t num, const void *const el_p, const size_t sizof_element)
{
    mddl_stl_vector_ext_t *const e =
	get_vector_ext(self_p);
    int result;
    uint8_t *ptr;
    size_t move_size;

    if( NULL == self_p ) {
	return EINVAL;
    } else if( e->stat.f.mem_fixed ) {
	return EPERM;
    } else if( sizof_element != e->sizof_element) {
	return EINVAL;
    } else if (!(e->num_elements >= num)) {
	return ENOENT;
    }

    result = resize_buffer(e, e->num_elements + 1, NULL);
    if(result) {
	DBMS1( "%s : resize_buffer fail, strerror:%s" EOL_CRLF, __func__, strerror(result));
	return result;
    }

    ptr = ((uint8_t *) e->buf + (num * e->sizof_element));
    move_size = (e->num_elements - num) * e->sizof_element;

    if( move_size ) {
	memmove( ptr + e->sizof_element, ptr, move_size);
    }
    memcpy( ptr, el_p, e->sizof_element);
    ++(e->num_elements);

    return 0;
}


/**
 * @fn int mddl_stl_vector_remove_at( mddl_stl_vector_t *const self_p, const size_t num)
 * @brief 要素を消去します。
 * @param self_p mddl_stl_vector_t構造体インスタンスポインタ
 * @param num 要素番号
 * @retval ENOENT 不正な要素番号
 * @retval EINVAL 関数実行の不許可
 */ 
int mddl_stl_vector_remove_at( mddl_stl_vector_t *const self_p, const size_t num)
{

    mddl_stl_vector_ext_t *const e =
	get_vector_ext(self_p);
    int result;
    uint8_t *ptr;
    size_t move_size;

    if(NULL == self_p) {
	return EINVAL;
    } else if( e->stat.f.mem_fixed ) {
	return EPERM;
    } else if (!(e->num_elements > num)) {
	return ENOENT;
    }

    ptr = ((uint8_t *) e->buf + (num * e->sizof_element));
    move_size = (e->num_elements - (num + 1)) * e->sizof_element;

    result = resize_buffer(e, e->num_elements -1, NULL);
    if(result) {
	DBMS1("%s : resize_buffer fail, strerror:%s" EOL_CRLF, __func__, strerror(result));
	return result;
    }

    if( move_size != 0 ) {
	memmove( ptr, ptr + e->sizof_element, move_size);
    }

    return 0;
}

/**
 * @fn int mddl_stl_vector_get_element_at( mddl_stl_vector_t *const self_p, const size_t num, void *const el_p, const size_t sizof_element)
 * @brief 保存された要素を返します
 * @param self_p mddl_stl_vector_t構造体インスタンスポインタ
 * @param num 0空始まるキュー先頭からのエレメント配列番号
 * @param el_p エレメントデータコピー用エレメント構造体ポインタ
 * @param sizof_element エレメントサイズ(主にエレメントサイズ検証向け)
 * @retval 0 成功
 * @retval EINVAL 不正な引数 
 **/
int mddl_stl_vector_get_element_at( mddl_stl_vector_t *const self_p, const size_t num, void *const el_p, const size_t sizof_element)
{
    mddl_stl_vector_ext_t *const e =
	get_vector_ext(self_p);
    const uint8_t *const ptr = (const uint8_t*)mddl_stl_vector_ptr_at(self_p, num);
  
    if((NULL == self_p) || (NULL == ptr) || (NULL == el_p)) {
   	return EINVAL;
    }

    if (sizof_element != e->sizof_element) {
	return EINVAL;
    }

    memcpy( el_p, ptr, sizof_element);

    return 0;
}



/**
 * @fn int mddl_stl_vector_get_element_at( mddl_stl_vector_t *const self_p, const size_t num, void *const el_p, const size_t sizof_element)
 * @brief 保存された要素を返します
 * @param self_p mddl_stl_vector_t構造体インスタンスポインタ
 * @param num 0空始まるキュー先頭からのエレメント配列番号
 * @param el_p エレメントデータコピー用エレメント構造体ポインタ
 * @param sizof_element エレメントサイズ(主にエレメントサイズ検証向け)
 * @retval 0 成功
 * @retval EINVAL 不正な引数 
 **/
int mddl_stl_vector_overwrite_element_at( mddl_stl_vector_t *const self_p, const size_t num, const void *const el_p, const size_t sizof_element)
{
    mddl_stl_vector_ext_t *const e =
	get_vector_ext(self_p);
    uint8_t *const ptr = (uint8_t*)mddl_stl_vector_ptr_at(self_p, num);
  
    if((NULL == self_p) || (NULL == ptr) || (NULL == el_p)) {
   	return EINVAL;
    }

    if (sizof_element != e->sizof_element) {
	return EINVAL;
    }

    memcpy( ptr, el_p, sizof_element);

    return 0;
}



/**
 * @fn int mddl_stl_vector_element_swap_at( mddl_stl_vector_t *const self_p, const size_t at1, const size_t at2)
 * @brief 指定された二つの要素を入れ替えます。
 * @param self_p mddl_stl_vector_t構造体インスタンスポインタ
 * @param at1 要素1
 * @param at2 要素2
 * @retval 0 成功
 * @retval EINVAL 不正な要素番号
 * @retval EAGAIN リソースを獲得できなかった
 **/
int mddl_stl_vector_element_swap_at( mddl_stl_vector_t *const self_p, const size_t at1, const size_t at2)
{
    mddl_stl_vector_ext_t *const e =
	get_vector_ext(self_p);
    void *ptr1 = mddl_stl_vector_ptr_at(self_p, at1);
    void *ptr2 = mddl_stl_vector_ptr_at(self_p, at2);
    void *buf;

    if((NULL == self_p) || (NULL == ptr1) || (NULL == ptr2)) {
	return EINVAL;
    }

    buf = own_malloc( e->sizof_element );
    if( NULL == buf ) {
	return EAGAIN;
    }
    memcpy( buf, ptr1, e->sizof_element);
    memcpy( ptr1, ptr2, e->sizof_element);
    memcpy( ptr2, buf, e->sizof_element);

    if( NULL != buf ) {
	own_mfree(buf);
    }

    return 0;
}


/**
 * @fn int mddl_stl_vector_shrink( mddl_stl_vector_t *const self_p, const size_t num_elements)
 * @brief メモリの予約サイズを縮小します。
 * @param self_p mddl_stl_vector_t構造体インスタンスポインタ
 * @param num_elements メモリ割当済み総エレメント数
 * @retval 0 成功
 * @retval EINVAL 引数(self_p)が不正
 * @retval EBUSY 要素が使用中である
 * @retval EAGAIN リソースが獲得できなかった
 */
int mddl_stl_vector_shrink( mddl_stl_vector_t *const self_p, const size_t num_elements)
{
    mddl_stl_vector_ext_t *const e =
	get_vector_ext(self_p);
    const size_t new_reserve = e->sizof_element * num_elements;

    if( NULL == self_p ) {
	return EINVAL;
    } else if( e->stat.f.mem_fixed ) {
	return EPERM;
    } else if (e->num_elements > num_elements) {
	return EBUSY;
    }

    if( num_elements == 0 ) {
	if( NULL != e->buf ) {
	    own_mfree(e->buf);
	    e->buf = NULL;
	}
	e->reserved_bytes = 0;
    } else {
	void *new_buf = own_mrealloc(e->buf, new_reserve);
	if (NULL == new_buf) {
	    return EAGAIN;
	}
	e->buf = new_buf;
	e->reserved_bytes = new_reserve;
    }

    return 0;
}

/**
 * @fn mddl_stl_vector_attach_memory_fixed( mddl_stl_vector_t *const self_p, void *const mem_ptr, const size_t len)
 * @brief エレメントを保存するエリアを固定します。指定されたメモリポインタの領域は、インスタンスが破棄されるまで使用されます。
 * 割り当てた後はインスタンスを破棄するまで指定された領域の中はいじらないでください。
 *
 * 次の関数はエラーになります。エリアの変更・拡張・縮小に関係するものです。
 *  EPERMをエラーコードで戻します。 
 * ・mddl_stl_vector_attach_memory_fixed()
 * ・mddl_stl_vector_resize()
 * ・mddl_stl_vector_reserve()
 * ・mddl_stl_vector_shrink()
 * 次の関数は成功しますが、指定されたメモリを開放することはしません。
 * ・mddl_stl_vector_clear()
 * 
 * @param self_p mddl_stl_vector_t構造体インスタンスポインタ
 * @param mem_ptr メモリ領域の開始ポインタ(仮想・物理エリア対応
 * @param len メモリエリアサイズ
 * @retval 0 成功
 * @retval EPERM すでに割り当て済み、エレメントが存在する
 * @retval EINVAL 引数のどれかが不正
 **/
int mddl_stl_vector_attach_memory_fixed( mddl_stl_vector_t *const self_p, void *const mem_ptr, const size_t len)
{
    mddl_stl_vector_ext_t *const e =
	get_vector_ext(self_p);

    if( NULL == self_p ) {
	return EINVAL;
    } else if( NULL != e->buf ) {
	return EPERM;
    } else if((NULL == mem_ptr) || !(e->sizof_element < len)) {
	return EINVAL;
    } else if( e->stat.f.mem_fixed ) {
	return EPERM;
    }

    e->buf = mem_ptr;
    e->reserved_bytes = len - (len % e->sizof_element);
    e->stat.f.mem_fixed = 1;

    // mbmcs_printf("mddl_stl_vector_attach_memory_fixed : e->reserved_bytes = %u" EOL_CRLF, (unsigned int)e->reserved_bytes);

    return 0;
}
