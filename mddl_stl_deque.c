/**
 *	Copyright 2014 TSN-SHINGENN All Rights Reserved.
 *
 *	Basic Author: Seiichi Takeda  '2014-March-01 Active
 *		Last Alteration $Author: takeda $
 *
 *	Dual License :
 *	non-commercial ... MIT Licence
 *	    commercial ... Requires permission from the author
 */

/**
 * @file mddl_stl_deque.c
 * @brief 両端待ち行列ライブラリ STLのdequeクラス互換です。
 *	なのでスレッドセーフではありません。上位層で処理を行ってください。
 *	STL互換と言ってもCの実装です、オブジェクトの破棄の処理はしっかり実装してください
 *  ※ 現状はmallocを乱発します。機会があれば、メモリ管理マネージャに切り替えます。
 */

/* POSIX */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

/* this */
#include "mddl_stl_vector.h"
#include "mddl_stl_deque.h"

/* dbms */
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

typedef struct _mddl_stl_deque_ext {
    size_t sizof_element;
    mddl_stl_vector_t vect;

    union {
	unsigned int flags;
	struct {
	    unsigned int vect:1;
	} f;
    } init;
} mddl_stl_deque_ext_t;

#define get_stl_deque_ext(s) (mddl_stl_deque_ext_t*)((s)->ext)
#define get_const_stl_deque_ext(s) (const mddl_stl_deque_ext_t*)((s)->ext)

/**
 * @fn int mddl_stl_deque_init( mddl_stl_deque_t *const self_p, const size_t sizof_element)
 * @brief インスタンスを初期化します
 * @param self_p mddl_stl_deque_t構造体インスタンスポインタ
 * @param sizof_element 1以上のエレメントのサイズ
 * @retval 0 成功
 * @retval EINVAL sizof_element値が不正
 * @retval EAGAIN リソースの獲得に失敗
 * @retval -1 それ以外の致命的な失敗
 */
int mddl_stl_deque_init(mddl_stl_deque_t *const self_p,
			   const size_t sizof_element)
{
    int result, status;
    mddl_stl_deque_ext_t * __restrict e = NULL;
    memset(self_p, 0x0, sizeof(mddl_stl_deque_t));

    e = (mddl_stl_deque_ext_t *)
	own_malloc(sizeof(mddl_stl_deque_ext_t));
    if (NULL == e) {
	DBMS1(stderr, "%s : mddl_malloc(ext) fail" EOL_CRLF, __func__);
	return EAGAIN;
    }
    memset(e, 0x0, sizeof(mddl_stl_deque_ext_t));

    self_p->ext = e;
    self_p->sizeof_element = e->sizof_element = sizof_element;

    result = mddl_stl_vector_init( &e->vect, sizeof(void*));
    if( result ) {
	DBMS1(stderr, "%s : mddl_stl_vector_init fail, strerror:%s" EOL_CRLF, __func__, strerror(result));
	status = result;
	goto out;
    } else {
	e->init.f.vect = 1;
    }

    status = 0;

out:
    if(status) {
	mddl_stl_deque_destroy(self_p);
    }

    return status;
}

/**
 * @fn int mddl_stl_deque_destroy( mddl_stl_deque_t *const self_p )
 * @brief 双方向キューオブジェクトを破棄します
 * @param self_p mddl_stl_deque_t構造体インスタンスポインタ
 * @retval EBUSY リソースの解放に失敗
 * @retval -1 errno.h以外の致命的な失敗
 * @retval 0 成功
 */
int mddl_stl_deque_destroy(mddl_stl_deque_t *const  self_p)
{
    mddl_stl_deque_ext_t *const e = get_stl_deque_ext(self_p);
    int result;

    if( NULL == e ) {
	return 0;
    }

    result = mddl_stl_deque_clear(self_p);
    if (result) {
	DBMS1(stderr, "%s : que_clear fail" EOL_CRLF, __func__);
	return EBUSY;
    }

    if(e->init.f.vect) {
	result = mddl_stl_vector_destroy( &e->vect);
	if(result) {
	    DBMS1(stderr, "%s : mddl_stl_vector_init fail, strerror:%s" EOL_CRLF, __func__, strerror(result));
	} else {
	    e->init.f.vect = 0;
	}
    }

    if(e->init.flags) {
	DBMS1(stderr, "%s : init.flags = 0x%08s" EOL_CRLF, __func__, e->init.flags);
	return e->init.flags;
    }

    if( NULL != self_p->ext ) {
	own_free(self_p->ext);
    }

    return 0;
}

/**
 * @fn int mddl_stl_deque_push_back(mddl_stl_deque_t *const self_p, const void *const el_p, const int sizof_element)
 * @brief 双方向キューの後方にエレメントを追加します
 * @param self_p mddl_stl_deque_t構造体インスタンスポインタ
 * @param el_p エレメントポインタ
 * @param sizof_element エレメントサイズ(mddl_stl_deque_initで指定した以外のサイズはエラーとします)
 * @retval 0 成功
 * @retval EINVAL エレメントサイズが異なる
 * @retval EAGAIN リソースの獲得に失敗
 * @retval -1 それ以外の致命的な失敗
 */
int mddl_stl_deque_push_back(mddl_stl_deque_t *const self_p,
				const void *const el_p,
				const size_t sizof_element)
{
    int status, result;
    mddl_stl_deque_ext_t *const e = get_stl_deque_ext(self_p);
    void *page = NULL;

    DBMS3(stderr, "%s : execute" EOL_CRLF, __func__);

    if( NULL == el_p ) {
	return EFAULT;
    }
    if (e->sizof_element != sizof_element) {
	return EINVAL;
    }

    page = own_malloc(e->sizof_element);
    if (NULL == page) {
	DBMS1(stderr, "%s : own_malloc(queitem_t) fail" EOL_CRLF, __func__);
	status = EAGAIN;
	goto out;
    }

    memcpy(page, el_p, e->sizof_element);

    /* キューに追加 */
    result = mddl_stl_vector_push_back( &e->vect, &page, sizeof(page));
    if(result) {
	DBMS1(stderr, "%s : mddl_vector_push_back fail, strerror:%s" EOL_CRLF, __func__, strerror(result));
	status = result;
	goto out;
    }
    status = 0;

  out:
    if (status) {
	if (NULL != page) {
	    own_mfree(page);
	}
    }
    return status;
}


/**
 * @fn int mddl_stl_deque_push_front(mddl_stl_deque_t *const self_p, const void *const el_p, const size_t sizof_element)
 * @brief 双方向キューの前方にエレメントを追加します
 * @param self_p mddl_stl_deque_t構造体インスタンスポインタ
 * @param el_p エレメントポインタ
 * @param sizof_element エレメントサイズ(mddl_stl_deque_initで指定した以外のサイズはエラーとします)
 * @retval 0 成功
 * @retval EINVAL エレメントサイズが異なる
 * @retval EAGAIN リソースの獲得に失敗
 * @retval -1 それ以外の致命的な失敗
 */
int mddl_stl_deque_push_front(mddl_stl_deque_t *const self_p,
				 const void *const el_p,
				 const size_t sizof_element)
{
    return mddl_stl_deque_insert(self_p, 0, el_p, sizof_element);
}

/**
 * @fn int mddl_stl_deque_pop_front(mddl_stl_deque_t *const self_p)
 * @brief 双方向キューの先頭の要素を削除します
 * @param self_p mddl_stl_deque_t構造体インスタンスポインタ
 * @retval EACCES 削除するエレメントが存在しない
 * @retval -1 それ以外の致命的な失敗
 * @retval 0 成功
 */
int mddl_stl_deque_pop_front(mddl_stl_deque_t *const self_p)
{
    mddl_stl_deque_ext_t *const e = get_stl_deque_ext(self_p);
    void *__restrict page = NULL;
    int result;

    /* エレメントがあるかどうかチェック */
    if (mddl_stl_vector_is_empty(&e->vect)) {
	return EACCES;
    }

    /* エレメントを外す */
    page = *(void**)mddl_stl_vector_ptr_at( &e->vect, 0 );
    mddl_mfree(page);

    result = mddl_stl_vector_remove_at( &e->vect, 0);
    if(result) {
	DBMS(stderr, "%s : mddl_stl_vector_erase fail, strerror:%s" EOL_CRLF, __func__, strerror(result));
	return result;
    }

    return 0;
}

/**
 * @fn int mddl_stl_deque_pop_back(mddl_stl_deque_t *const self_p)
 * @brief 双方向キューの後方の要素を削除します
 * @param self_p mddl_stl_deque_t構造体インスタンスポインタ
 * @retval EACCES 削除するエレメントが存在しない
 * @retval -1 それ以外の致命的な失敗
 * @retval 0 成功
 */
int mddl_stl_deque_pop_back(mddl_stl_deque_t *const self_p)
{
    mddl_stl_deque_ext_t *const e = get_stl_deque_ext(self_p);
    size_t num;
    int result;
    void * __restrict page;

    /* エレメントがあるかどうかチェック */
    if (mddl_stl_vector_is_empty(&e->vect)) {
	return EACCES;
    }

    /* エレメントを外す */
    num = mddl_stl_vector_get_pool_cnt( &e->vect ) -1;

    /* エレメントを外す */
    page = *(void**)mddl_stl_vector_ptr_at( &e->vect, num );
    mddl_free(page);

    result = mddl_stl_vector_pop_back( &e->vect);
    if(result) {
	DBMS("%s : mddl_stl_vector_pop_back fail, strerror:%s" EOL_CRLF, __func__, strerror(result));
	return result;
    }

    return 0;
}

/**
 * @fn int mddl_stl_deque_clear( mddl_stl_deque_t *const self_p )
 * @brief キューに貯まっているエレメントデータを全て破棄します
 * @param self_p mddl_stl_deque_t構造体インスタンスポインタ
 * @retval -1 致命的な失敗
 * @retval 0 成功
 */
int mddl_stl_deque_clear(mddl_stl_deque_t *const self_p)
{
    mddl_stl_deque_ext_t *const e = get_stl_deque_ext(self_p);
    int result;
    size_t n, total;

    /* エレメントがあるかどうかチェック */
    if (mddl_stl_vector_is_empty(&e->vect)) {
	return 0;
    }

    /* エレメントを外す */
    total = mddl_stl_vector_get_pool_cnt( &e->vect );
    for (n=0; n<total; ++n) {
	void *page = *(void**)mddl_stl_vector_ptr_at( &e->vect, n);
	mddl_free(page);
    }

    /* ベクタテーブルをクリア */
    result = mddl_stl_vector_clear( &e->vect );
    if(result) {
	DBMS1( "%s : mddl_stl_vector_clear fail, strerror:%s" EOL_CRLF, __func__, strerror(result));
	return result;
    }

    return 0;
}

/**
 * @fn int mddl_stl_deque_get_pool_cnt( mddl_stl_deque_t *const self_p );
 * @brief キューにプールされているエレメント数を返します
 * @param self_p mddl_stl_deque_t構造体インスタンスポインタ
 * @retval -1 失敗
 * @retval 0以上 要素数
 */
size_t mddl_stl_deque_get_pool_cnt(mddl_stl_deque_t *const self_p)
{
    mddl_stl_deque_ext_t *const e = get_stl_deque_ext(self_p);

    return mddl_stl_vector_get_pool_cnt( &e->vect );
}

/**
 * @fn int mddl_stl_deque_is_empty( mddl_stl_deque_t *const self_p)
 * @brief キューが空かどうかを判定します
 * @param self_p mddl_stl_deque_t構造体インスタンスポインタ
 * @retval -1 失敗
 * @retval 0 キューは空ではない
 * @retval 1 キューは空である
 */
int mddl_stl_deque_is_empty(mddl_stl_deque_t *const self_p)
{
    mddl_stl_deque_ext_t *const e = get_stl_deque_ext(self_p);

    return mddl_stl_vector_is_empty(&e->vect);
}

/**
 * @fn void *mddl_stl_deque_ptr_at( mddl_stl_deque_t *const self_p, const size_t num)
 * @brief 要素の割当ポインタを得る
 * @param self_p mddl_stl_deque_t構造体インスタンスポインタ
 * @param num 割当の要素番号
 * @retval NULL 不正な要素番号
 * @retval NULL以外 要素のポインタ
 */
void *mddl_stl_deque_ptr_at( mddl_stl_deque_t *const self_p, const size_t num)
{
    mddl_stl_deque_ext_t *const e = get_stl_deque_ext(self_p);

    /* エレメントがあるかどうかチェック */
    if (mddl_stl_vector_is_empty(&e->vect)) {
	return NULL;
    }

    if( !(num < mddl_stl_vector_get_pool_cnt( &e->vect ))) {
	return NULL;
    }

    /* ページを取得 */
    return *(void**)mddl_stl_vector_ptr_at( &e->vect, num );
}

/**
 * @fn int mddl_stl_deque_get_element_at( mddl_stl_deque_t *const self_p, const int num, void *const el_p, const size_t sizof_element);
 * @brief キューに保存されているエレメントを返します
 * @param self_p mddl_stl_deque_t構造体インスタンスポインタ
 * @param num 0から始まるキュー先頭からのエレメント配列番号
 * @param el_p エレメントデータポインタ
 * @param sizof_element エレメントサイズ(主にエレメントサイズ検証向け)
 * @retval 0 成功
 * @retval EFAULT el_pに指定されたアドレスがNULLだった
 * @retval EINVAL 不正な引数
 */
int mddl_stl_deque_get_element_at(mddl_stl_deque_t *const self_p,
				     const size_t num, void *const el_p,
				     const size_t sizof_element)
{
    mddl_stl_deque_ext_t *const e = get_stl_deque_ext(self_p);
    void *const page = mddl_stl_deque_ptr_at( self_p, num);

    if (NULL == el_p) {
	return EFAULT;
    }

    if(NULL == page) {
	return EINVAL;
    }

    if (sizof_element < e->sizof_element) {
	return EINVAL;
    }

    /* ページを取得 */
    mddl_memcpy(el_p, page, e->sizof_element);

    return 0;
}

/**
 * @fn int mddl_stl_deque_remove_at(mddl_stl_deque_t *const self_p, const size_t num)
 * @brief 双方向キューに保存されている要素を消去します
 * @param self_p mddl_stl_deque_t構造体インスタンスポインタ
 * @param num 0から始まるキュー先頭からのエレメント配列番号
 * @retval 0 成功
 * @retval EACCES 双方向キューが空
 * @retval ENOENT 指定されたエレメントが無い
 * @retval EINVAL sizof_elementのサイズが異なる(小さい）
 */
int mddl_stl_deque_remove_at(mddl_stl_deque_t *const self_p,
			       const size_t num)
{
    mddl_stl_deque_ext_t *const e = get_stl_deque_ext(self_p);
    int result;
    void *page;

    /* エレメントがあるかどうかチェック */
    if (mddl_stl_vector_is_empty(&e->vect)) {
	return EACCES;
    }

    if( !(num < mddl_stl_vector_get_pool_cnt( &e->vect ))) {
	return ENOENT;
    }

    page = *(void**)mddl_stl_vector_ptr_at( &e->vect, num );
    mddl_free(page);

    result = mddl_stl_vector_remove_at( &e->vect, num);
    if(result) {
	DBMS1( stderr, "%s : mddl_stl_vector_remove_at fail, strerror:%s" EOL_CRLF, __func__, strerror(result));
	return result;
    }
    return 0;
}

/**
 * @fn int mddl_stl_deque_insert( mddl_stl_deque_t *const self_p, const size_t num, const void *const el_p, const size_t sizof_element )
 * @brief 双方向キューの指定されたエレメント番号の直前に、データを挿入します.
 *	numに0を指定した場合は、mddl_dequeue_push_front()と同等の動作をします。
 * @param self_p mddl_stl_deque_t構造体インスタンスポインタ
 * @param num 0から始まるキュー先頭からのエレメント配列番号
 * @param el_p エレメントデータポインタ
 * @param sizof_element エレメントサイズ(主にエレメントサイズ検証向け)
 * @retval 0 成功
 * @retval ENOENT 指定されたエレメントが無い
 * @retval EFAULT el_pに指定されたアドレスがNULLだった
 * @retval EINVAL sizof_elementのサイズが異なる(小さい）
 * @retval EAGAIN リソースの獲得に失敗
 */
int mddl_stl_deque_insert(mddl_stl_deque_t *const self_p,
			     const size_t num, const void *const el_p,
			     const size_t sizof_element)
{
    int status, result;
    mddl_stl_deque_ext_t *const e = get_stl_deque_ext(self_p);
    void *page;

    if (e->sizof_element != sizof_element) {
	DBMS3( "%s  : e->sizof_element=%s sizof_element=%s" EOL_CRLF, __func__, e->sizof_element, sizof_element);
	return EINVAL;
    }

    page = own_malloc(e->sizof_element);
    if (NULL == page) {
	DBMS1("%s : own_malloc(page) fail" EOL_CRLF, __func__);
	status = EAGAIN;
	goto out;
    }
    memcpy(page, el_p, e->sizof_element);

    /* 挿入処理 */
    result = mddl_stl_vector_insert( &e->vect, num, &page, sizeof(page));
    if(result) {
	DBMS1(stderr, "%s : mddl_stl_vector_insert fail, strerror:%s" EOL_CRLF, __func__, strerror(result));
	return result;
    }

    status = 0;

  out:
    if (status) {
	if (NULL != page) {
	    own_free(page);
	}
    }
    return status;
}


/**
 * @fn int mddl_stl_deque_overwrite_element_at( mddl_stl_deque_t *const self_p,  const size_t num, const void *const el_p, const size_t sizof_element)
 * @brief キューに保存されているエレメントを上書きしますす
 * @param self_p mddl_stl_deque_t構造体インスタンスポインタ
 * @param num 0から始まるキュー先頭からのエレメント配列番号
 * @param el_p エレメントデータポインタ
 * @param sizof_element エレメントサイズ(主にエレメントサイズ検証向け)
 * @retval 0 成功
 * @retval EACCES キューにエレメントがない
 * @retval ENOENT 指定されたエレメントが無い
 * @retval EINVAL 引数が不正
 */

int mddl_stl_deque_overwrite_element_at( mddl_stl_deque_t *const self_p,  const size_t num, const void *const el_p, const size_t sizof_element)
{
    mddl_stl_deque_ext_t *const e = get_stl_deque_ext(self_p);
    void *page;

    /* エレメントがあるかどうかチェック */
    if (mddl_stl_vector_is_empty(&e->vect)) {
	return EACCES;
    }
    if (NULL == el_p) {
	return EFAULT;
    }

    if (sizof_element < e->sizof_element) {
	return EINVAL;
    }

    if( !(num < mddl_stl_vector_get_pool_cnt( &e->vect ))) {
	return ENOENT;
    }

    /* ページを取得 */
    page = *(void**)mddl_stl_vector_ptr_at( &e->vect, num );

    memcpy(page, el_p, e->sizof_element);

    return 0;
}

/**
 * @fn int mddl_stl_deque_front( mddl_stl_deque_t *const self_p, const void *const el_p, const size_t sizeof_element)
 * @brief 先頭に保存されたエレメントを返します
 * @param self_p mddl_stl_deque_t構造体インスタンスポインタ
 * @param el_p 取得する要素のバッファポインタ
 * @param sizof_element 要素サイズ
 * @retval 0 成功
 * @retval EINVAL 要素サイズが異なる
 * @retval ENOENT 先頭に保存された要素がない
 * @retval -1 その他の致命的なエラー
 **/
int mddl_stl_deque_front( mddl_stl_deque_t *const self_p, void *const el_p, const size_t sizof_element)
{
    mddl_stl_deque_ext_t *const e = get_stl_deque_ext(self_p);
    int result;
    void *page;

    /* エレメントがあるかどうか二重チェック */
    if (mddl_stl_vector_is_empty(&e->vect)) {
	return ENOENT;
    } else if( e->sizof_element != sizof_element ) {
	return EINVAL;
    }

    result = mddl_stl_vector_front( &e->vect, &page, sizeof(page) );
    if(result) {
	DBMS1( stderr, "%s : mddl_stl_vector_front fail, strerror:%s" EOL_CRLF, __func__, strerror(result));
	return result;
    }

    memcpy(el_p, page, e->sizof_element);

    return 0;
}


/**
 * @fn int mddl_stl_deque_back( mddl_stl_deque_t *const self_p, void *const el_p, const size_t sizof_element)
 * @brief 最後尾に保存されたエレメントを返します
 *	*** 検証中です ***
 * @param self_p mddl_stl_deque_t構造体インスタンスポインタ
 * @param el_p 取得する要素のバッファポインタ
 * @param sizof_element 要素サイズ
 * @retval 0 成功
 * @retval EINVAL 要素サイズが異なる
 * @retval ENOENT 先頭に保存された要素がない
 * @retval -1 その他の致命的なエラー
 **/
int mddl_stl_deque_back( mddl_stl_deque_t *const self_p, void *const el_p, const size_t sizof_element)
{
    mddl_stl_deque_ext_t *const e = get_stl_deque_ext(self_p);
    void *page;
    int result;

    /* エレメントがあるかどうか二重チェック */
    if (mddl_stl_vector_is_empty(&e->vect)) {
	return ENOENT;
    } else if( e->sizof_element != sizof_element ) {
	return EINVAL;
    }

    result = mddl_stl_vector_back( &e->vect, &page, sizeof(page) );
    if(result) {
	DBMS1( stderr, "%s : mddl_stl_vector_back fail, strerror:%s" EOL_CRLF, __func__, strerror(result));
	return result;
    }

    memcpy(el_p, page, e->sizof_element);
	
    return 0;
}

/**
 * @fn int mddl_stl_deque_element_swap_at( mddl_stl_deque_t *const self_p, const size_t at1, const size_t at2)
 * @brief　指定された二つの要素を入れ替えます
 * @param self_p mddl_stl_deque_t構造体インスタンスポインタ
 * @param at1 要素番号1
 * @param at2 要素番号2
 * @retval 0 成功
 * @retval EACCES キューに保存されたエレメントがない
 * @retval EINVAL at1,at2で指定された要素番号が不正
 **/
int mddl_stl_deque_element_swap_at( mddl_stl_deque_t *const self_p, const size_t at1, const size_t at2)
{
    mddl_stl_deque_ext_t *const e = get_stl_deque_ext(self_p);
    void **ptr1, **ptr2, *page;

    /* エレメントがあるかどうかチェック */
    if (mddl_stl_vector_is_empty(&e->vect)) {
	return EACCES;
    }

    ptr1 = (void**)mddl_stl_vector_ptr_at( &e->vect, at1 );
    ptr2 = (void**)mddl_stl_vector_ptr_at( &e->vect, at2 );

    if( (NULL == ptr1) || (NULL == ptr2 )) {
	return EINVAL;
    }

    page = *ptr1;
    *ptr1 = *ptr2;
    *ptr2 = page;

    return 0;
}

