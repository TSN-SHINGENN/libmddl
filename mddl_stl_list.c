/**
 *	Copyright 2014 TSN-SHINGENN All Rights Reserved.
 *
 *	Basic Author: Seiichi Takeda  '2014-March-01 Active
 *		Last Alteration $Author: takeda $
 *	Dual License :
 *	non-commercial ... MIT Licence
 *	    commercial ... Requires permission from the author
 */

/**
 * @file mddl_stl_list.c
 * @brief 双方向リンクリストライブラリ STLのlistクラス互換です。
 *	なのでスレッドセーフではありません。上位層で処理を行ってください。
 *	STL互換と言ってもCの実装です、オブジェクトの破棄の処理はしっかり実装してください
 *  ※ 現状はmallocを乱発します。機会があれば、メモリ管理マネージャに切り替えます。
 */

/* POSIX */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

//#include "mddl_sprintf.h"
/* this */
#include "mddl_stl_list.h"

/* dbms */
#ifdef DEBUG
static int debuglevel = 4;
#else
static const int debuglevel = 0;
#endif
#include "dbms.h"

#define EOL_CRLF "\n\r"

/* 弱いアロケータの定義 */
void __attribute__((weak)) *mddl_malloc(const size_t size)
{
    return malloc(size);
}

void __attribute__((weak)) mddl_free( void *const ptr )
{
    free(ptr);
}


typedef struct _queitem {
    struct _queitem *prev;
    struct _queitem *next;
    int item_id;
    unsigned char data[];
} queitem_t;

typedef struct _mddl_stl_list_ext {
    volatile size_t sizof_element;
    volatile size_t cnt;
    int start_id;
    queitem_t base;		/* エレメントの基点。配列0の構造体があるので必ず最後にする */
} mddl_stl_list_ext_t;

#define get_stl_list_ext(s) (mddl_stl_list_ext_t*)((s)->ext)
#define get_const_stl_list_ext(s) (const mddl_stl_list_ext_t*)((s)->ext)

/**
 * @fn int mddl_stl_list_init( mddl_stl_list_t *const self_p, const size_t sizof_element)
 * @brief 双方向キューオブジェクトを初期化します
 * @param self_p mddl_stl_list_t構造体インスタンスポインタ
 * @param sizof_element 1以上のエレメントのサイズ
 * @retval 0 成功
 * @retval EINVAL sizof_element値が不正
 * @retval EAGAIN リソースの獲得に失敗
 * @retval -1 それ以外の致命的な失敗
 */
int mddl_stl_list_init(mddl_stl_list_t *const self_p,
			   const size_t sizof_element)
{
    mddl_stl_list_ext_t *e = NULL;
    memset(self_p, 0x0, sizeof(mddl_stl_list_t));

    e = (mddl_stl_list_ext_t *)
	mddl_malloc(sizeof(mddl_stl_list_ext_t));
    if (NULL == e) {
	DBMS1("%s : multios_malloc(ext) fail" EOL_CRLF, __func__);
	return EAGAIN;
    }
    memset(e, 0x0, sizeof(mddl_stl_list_ext_t));

    self_p->ext = e;
    self_p->sizeof_element = e->sizof_element = sizof_element;
    e->cnt = 0;
    e->start_id = 0;

    e->base.prev = e->base.next = &e->base;

    return 0;
}

/**
 * @fn int mddl_stl_list_destroy( mddl_stl_list_t *const self_p )
 * @brief 双方向キューオブジェクトを破棄します
 * @param self_p mddl_stl_list_t構造体インスタンスポインタ
 * @retval EBUSY リソースの解放に失敗
 * @retval -1 errno.h以外の致命的な失敗
 * @retval 0 成功
 */
int mddl_stl_list_destroy(mddl_stl_list_t *const self_p)
{
    int result;

    result = mddl_stl_list_clear(self_p);
    if (result) {
	DBMS1("multios_stl_list_destroy : que_clear fail" EOL_CRLF);
	return EBUSY;
    }

    mddl_free(self_p->ext);
    self_p->ext = NULL;

    return 0;
}

/**
 * @fn int mddl_stl_list_push_back(mddl_stl_list_t *const self_p, const void *const el_p, const int sizof_element)
 * @brief 双方向キューの後方にエレメントを追加します
 * @param self_p mddl_stl_list_t構造体インスタンスポインタ
 * @param el_p エレメントポインタ
 * @param sizof_element エレメントサイズ(mddl_stl_list_initで指定した以外のサイズはエラーとします)
 * @retval 0 成功
 * @retval EINVAL エレメントサイズが異なる
 * @retval EAGAIN リソースの獲得に失敗
 * @retval -1 それ以外の致命的な失敗
 */
int mddl_stl_list_push_back(mddl_stl_list_t *const self_p,
				const void *const el_p,
				const size_t sizof_element)
{
    int status;
    mddl_stl_list_ext_t *const e = get_stl_list_ext(self_p);
    queitem_t *__restrict f = NULL;

    DBMS3("multios_stl_list_push_back : execute" EOL_CRLF);

    if (e->cnt == 0) {
	return mddl_stl_list_insert(self_p, 0, el_p, sizof_element);
    }

    if (e->sizof_element != sizof_element) {
	return EINVAL;
    }

    f = (queitem_t *)mddl_malloc(sizeof(queitem_t) + e->sizof_element);
    if (NULL == f) {
	DBMS1("%s : mddl_malloc(queitem_t) fail" EOL_CRLF, __func__);
	status = EAGAIN;
	goto out;
    }

    memset(f, 0x0, sizeof(queitem_t));
    memcpy(f->data, el_p, e->sizof_element);
    f->prev = e->base.prev;
    f->next = &e->base;
    f->item_id = (e->base.prev->item_id) + 1;

    /* キューに追加 */
    e->base.prev->next = f;
    e->base.prev = f;
    e->cnt++;

    status = 0;

  out:
    if (status) {
	if (NULL != f) {
	    mddl_free(f);
	}
    }
    return status;
}


/**
 * @fn int mddl_stl_list_push_front(mddl_stl_list_t *const self_p, const void *const el_p, const size_t sizof_element)
 * @brief 双方向キューの前方にエレメントを追加します
 * @param self_p mddl_stl_list_t構造体インスタンスポインタ
 * @param el_p エレメントポインタ
 * @param sizof_element エレメントサイズ(mddl_stl_list_initで指定した以外のサイズはエラーとします)
 * @retval 0 成功
 * @retval EINVAL エレメントサイズが異なる
 * @retval EAGAIN リソースの獲得に失敗
 * @retval -1 それ以外の致命的な失敗
 */
int mddl_stl_list_push_front(mddl_stl_list_t *const self_p,
				 const void *const el_p,
				 const size_t sizof_element)
{
    return mddl_stl_list_insert(self_p, 0, el_p, sizof_element);
}

/**
 * @fn int mddl_stl_list_pop_front(mddl_stl_list_t *self_p)
 * @brief 双方向キューの先頭の要素を削除します
 * @param self_p mddl_stl_list_t構造体インスタンスポインタ
 * @retval EACCES 削除するエレメントが存在しない
 * @retval -1 それ以外の致命的な失敗
 * @retval 0 成功
 */
int mddl_stl_list_pop_front(mddl_stl_list_t *const self_p)
{
    mddl_stl_list_ext_t *const e = get_stl_list_ext(self_p);
    queitem_t *__restrict tmp = NULL;

    /* エレメントがあるかどうか二重チェック */
    if (e->cnt == 0) {
	return EACCES;
    }

    if (e->base.next == &e->base) {
	return -1;
    }

    /* エレメントを外す */
    tmp = e->base.next;
    tmp->prev->next = tmp->next;
    tmp->next->prev = tmp->prev;
    --e->cnt;
    ++e->start_id;

    if (tmp != &e->base) {
	mddl_free(tmp);
    }

    if (e->cnt == 0) {
	e->start_id = 0;
    }

    return 0;
}


/**
 * @fn int mddl_stl_list_pop_back(mddl_stl_list_t *const self_p)
 * @brief 双方向キューの後方の要素を削除します
 * @param self_p mddl_stl_list_t構造体インスタンスポインタ
 * @retval EACCES 削除するエレメントが存在しない
 * @retval -1 それ以外の致命的な失敗
 * @retval 0 成功
 */
int mddl_stl_list_pop_back(mddl_stl_list_t *const self_p)
{
    mddl_stl_list_ext_t *const e = get_stl_list_ext(self_p);
    queitem_t *__restrict tmp = NULL;

    /* エレメントがあるかどうか二重チェック */
    if (e->cnt == 0) {
	return EACCES;
    }

    if (e->base.prev == &e->base) {
	return -1;
    }

    /* エレメントを外す */
    tmp = e->base.prev;
    tmp->prev->next = tmp->next;
    tmp->next->prev = tmp->prev;
    (void) e->start_id;		/* 何もしない */
    e->cnt--;

    if (tmp != &e->base) {
	mddl_free(tmp);
    }

    if (e->cnt == 0) {
	e->start_id = 0;
    }

    return 0;
}

/**
 * @fn int mddl_stl_list_clear( mddl_stl_list_t *const self_p )
 * @brief キューに貯まっているエレメントデータを全て破棄します
 * @param self_p mddl_stl_list_t構造体インスタンスポインタ
 * @retval -1 致命的な失敗
 * @retval 0 成功
 */
int mddl_stl_list_clear(mddl_stl_list_t *const self_p)
{
    mddl_stl_list_ext_t *const e = get_stl_list_ext(self_p);
    size_t n;
    int result;

    if (e->cnt == 0) {
	return 0;
    }

    for (n = e->cnt; n != 0; --n) {
	result = mddl_stl_list_pop_front(self_p);
	if (result) {
	    DBMS1(
		  "%s : mddl_stl_list_pop_front[%llu] fail" EOL_CRLF, __func__, (unsigned long long)n);
	    return -1;
	}
    }

    return 0;
}

/**
 * @fn int mddl_stl_list_get_pool_cnt( mddl_stl_list_t *const self_p );
 * @brief キューにプールされているエレメント数を返します
 * @param self_p mddl_stl_list_t構造体インスタンスポインタ
 * @retval -1 失敗
 * @retval 0以上 要素数
 */
size_t mddl_stl_list_get_pool_cnt(mddl_stl_list_t *const self_p)
{
    const mddl_stl_list_ext_t *const e = get_const_stl_list_ext(self_p);

    return e->cnt;
}

/**
 * @fn int mddl_stl_list_is_empty( mddl_stl_list_t *self_p)
 * @brief キューが空かどうかを判定します
 * @param self_p mddl_stl_list_t構造体インスタンスポインタ
 * @retval -1 失敗
 * @retval 0 キューは空ではない
 * @retval 1 キューは空である
 */
int mddl_stl_list_is_empty(mddl_stl_list_t *const self_p)
{
    const mddl_stl_list_ext_t *const e = get_const_stl_list_ext(self_p);

    return (e->cnt == 0) ? 1 : 0;
}

/**
 * @fn static queitem_t *list_search_item( mddl_stl_list_ext_t *const e, const size_t num)
 * @brief numからqueitemのポインタを検索します
 * @param e mddl_stl_list_ext_t構造体ポインタ
 * @param 0から始まるエレメント配列番号
 */
static queitem_t *list_search_item(mddl_stl_list_ext_t *const e,
				    const size_t num)
{
    const int target_id = e->start_id + (int) num;
    queitem_t *__restrict item_p;

    if ((e->cnt / 2) < num) {
	/* 後方から前方検索 */
	item_p = e->base.prev;
	while (item_p != &e->base) {
	    if (item_p->item_id != target_id) {
		item_p = item_p->prev;
		continue;
	    }
	    break;
	}
    } else {
	/* 前方から後方検索 */
	item_p = e->base.next;
	while (item_p != &e->base) {
	    if (item_p->item_id != target_id) {
		item_p = item_p->next;
		continue;
	    }
	    break;
	}
    }

    if (item_p == &e->base) {
	return NULL;
    }

    return item_p;
}

/**
 * @fn int mddl_stl_list_get_element_at( mddl_stl_list_t *const self_p, const int num, void *const el_p, const size_t sizof_element);
 * @brief キューに保存されているエレメントを返します
 * @param self_p mddl_stl_list_t構造体インスタンスポインタ
 * @param num 0から始まるキュー先頭からのエレメント配列番号
 * @param el_p エレメントデータポインタ
 * @param sizof_element エレメントサイズ(主にエレメントサイズ検証向け)
 * @retval 0 成功
 * @retval EACCES キューにエレメントがない
 * @retval ENOENT 指定されたエレメントが無
 * @retval EFAULT el_pに指定されたアドレスがNULLだった
 * @retval EINVAL sizof_elementのサイズが異なる(小さい）
 */
int mddl_stl_list_get_element_at(mddl_stl_list_t *const self_p,
				     const size_t num, void *const el_p,
				     const size_t sizof_element)
{
    mddl_stl_list_ext_t *const e = get_stl_list_ext(self_p);
    queitem_t *__restrict item_p;

    if (mddl_stl_list_is_empty(self_p)) {
	return EACCES;
    }

    if (NULL == el_p) {
	return EFAULT;
    }

    if (sizof_element < e->sizof_element) {
	return EINVAL;
    }

    if (!(num < e->cnt)) {
	return ENOENT;
    }

    item_p = list_search_item(e, num);
    if (item_p == NULL) {
	return -1;
    }

    memcpy(el_p, item_p->data, e->sizof_element);

    return 0;
}

/**
 * @fn int mddl_stl_list_remove_at(mddl_stl_list_t *const self_p, const size_t num)
 * @brief 双方向キューに保存されている要素を消去します
 * @param self_p mddl_stl_list_t構造体インスタンスポインタ
 * @param num 0から始まるキュー先頭からのエレメント配列番号
 * @retval 0 成功
 * @retval EACCES 双方向キューが空
 * @retval ENOENT 指定されたエレメントが無い
 * @retval EFAULT el_pに指定されたアドレスがNULLだった
 * @retval EINVAL sizof_elementのサイズが異なる(小さい）
 */
int mddl_stl_list_remove_at(mddl_stl_list_t *const self_p,
			       const size_t num)
{
    mddl_stl_list_ext_t *const e = get_stl_list_ext(self_p);
    queitem_t *__restrict item_p = &e->base;

    if (mddl_stl_list_is_empty(self_p)) {
	return EACCES;
    }

    if (!(num < e->cnt)) {
	return ENOENT;
    }

    item_p = list_search_item(e, num);
    if (item_p == &e->base) {
	return -1;
    }

    /* itemのエレメントを排除 */
    item_p->prev->next = item_p->next;
    item_p->next->prev = item_p->prev;
    e->cnt--;

    /* start_idを書き換える */
    if (e->start_id == item_p->item_id) {
	e->start_id = item_p->next->item_id;
    } else if (item_p->next != &e->base) {
	queitem_t *__restrict tmp_p = item_p->next;
	int id = item_p->item_id;
	while (tmp_p != &e->base) {
	    tmp_p->item_id = id;
	    ++id;
	    tmp_p = tmp_p->next;
	}
    };

    /* itemのエレメントを削除 */
    mddl_free(item_p);

    return 0;
}

/**
 * @fn void mddl_stl_list_dump_element_region_list( mddl_stl_list_t *const self_p)
 * @brief 双方向キューに保存されている要素の情報をダンプします
 *	主にこのデバッグ用
 * @param self_p mddl_stl_list_t構造体インスタンスポインタ
 **/
void mddl_stl_list_dump_element_region_list(mddl_stl_list_t *const
						self_p)
{
    const mddl_stl_list_ext_t *const e = get_const_stl_list_ext(self_p);
#ifdef __GNUC__
__attribute__ ((unused))
#endif
	size_t p;

    DMSG("%s : execute\n", __func__);
    DMSG("sizof_element = %llu" EOL_CRLF, (unsigned long long)e->sizof_element);
    DMSG("numof_elements = %llu" EOL_CRLF, (unsigned long long)e->cnt);
    DMSG("start_id = %llu" EOL_CRLF, (long long)e->start_id);

#if 0
    const queitem_t *__restrict item_p = &e->base;
    item_p = e->base.next;
    size_t n;
    for (n = 0; n < e->cnt; ++n) {
	p += mddl_sprintf(&buf[p], "[%03d(%03d)]: ",
		     (item_p->item_id - e->start_id), item_p->item_id);
	p += mddl_sprintf(&buf[p], "%p : prev[%03d]=%p next[%03d]=%p ", item_p,
		     item_p->prev->item_id, item_p->prev,
		     item_p->next->item_id, item_p->next);
	if (item_p->prev == &e->base) {
	    p += mddl_sprintf(&buf[p], "(prev is base) ");
	}
	if (item_p->next == &e->base) {
	    p += mddl_sprintf(&buf[p], "(next is base) ");
	}
	DMSG(stdout, "%s" EOL_CRLF, buf);
	}
	if (n > 16) {
	    return;
	}
	item_p = item_p->next;
    }
#endif

    return;
}

/**
 * @fn int mddl_stl_list_insert( mddl_stl_list_t *const self_p, const size_t num, const void *const el_p, const size_t sizof_element )
 * @brief 双方向キューの指定されたエレメント番号の直前に、データを挿入します.
 *	numに0を指定した場合は、mddl_listue_push_front()と同等の動作をします。
 * @param self_p mddl_stl_list_t構造体インスタンスポインタ
 * @param num 0から始まるキュー先頭からのエレメント配列番号
 * @param el_p エレメントデータポインタ
 * @param sizof_element エレメントサイズ(主にエレメントサイズ検証向け)
 * @retval 0 成功
 * @retval ENOENT 指定されたエレメントが無い
 * @retval EFAULT el_pに指定されたアドレスがNULLだった
 * @retval EINVAL sizof_elementのサイズが異なる(小さい）
 * @retval EAGAIN リソースの獲得に失敗
 */
int mddl_stl_list_insert(mddl_stl_list_t *const self_p,
			     const size_t num, const void *const el_p,
			     const size_t sizof_element)
{
    int status;
    mddl_stl_list_ext_t *const e = get_stl_list_ext(self_p);
    queitem_t *__restrict f, *__restrict front;

    IFDBG3THEN {
	DBMS3("%s : execute" EOL_CRLF, __func__);
	DBMS3("%s : num=%llu e->cnt=%llu" EOL_CRLF,
	    __func__, (unsigned long long)num, (unsigned long long)e->cnt);
    }

    if (e->sizof_element != sizof_element) {
	DBMS3("%s : e->sizof_element=%s sizof_element=%s" EOL_CRLF, __func__, e->sizof_element, sizof_element);
	return EINVAL;
    }

    front = list_search_item(e, num);
    if (NULL == front) {
	if ((num == 0) && (e->cnt == 0)) {
	    front = &e->base;
	} else {
	    return ENOENT;
	}
    }

    DBMS3("%s : front=0x%p e->base=0x%p" EOL_CRLF,
	  __func__, front, &e->base);

    f = (queitem_t *) mddl_malloc(sizeof(queitem_t) + e->sizof_element);
    if (NULL == f) {
	DBMS1("%s : mddl_malloc(queitem_t) fail" EOL_CRLF, __func__);
	status = EAGAIN;
	goto out;
    }

    memset(f, 0x0, sizeof(queitem_t));
    memcpy(f->data, el_p, e->sizof_element);

    f->prev = front->prev;
    f->next = front;

    if ((num == 0) && (e->cnt != 0)) {
	f->item_id = (front->item_id) - 1;
	e->start_id = f->item_id;
    } else {
	queitem_t *__restrict p = front;
	f->item_id = front->item_id;
	/* 挿入したエレメント以降のidの再割当 */
	while (p != &e->base) {
	    ++(p->item_id);
	    p = p->next;
	}
    }

    /* 挿入処理 */
    f->prev->next = f;
    f->next->prev = f;
    e->cnt++;

    status = 0;

  out:
    if (status) {
	if (NULL != f) {
	    mddl_free(f);
	}
    }
    return status;
}


/**
 * @fn int mddl_stl_list_overwrite_element_at( mddl_stl_list_t *const self_p,  const size_t num, const void *const el_p, const size_t sizof_element)
 * @brief キューに保存されているエレメントを上書きしますす
 * @param self_p mddl_stl_list_t構造体インスタンスポインタ
 * @param num 0から始まるキュー先頭からのエレメント配列番号
 * @param el_p エレメントデータポインタ
 * @param sizof_element エレメントサイズ(主にエレメントサイズ検証向け)
 * @retval 0 成功
 * @retval EACCES キューにエレメントがない
 * @retval ENOENT 指定されたエレメントが無い
 * @retval EINVAL 引数が不正
 */

int mddl_stl_list_overwrite_element_at( mddl_stl_list_t *const self_p,  const size_t num, const void *const el_p, const size_t sizof_element)
{
    mddl_stl_list_ext_t *const e = get_stl_list_ext(self_p);
    queitem_t *__restrict item_p;

    if (mddl_stl_list_is_empty(self_p)) {
	return EACCES;
    }

    if (NULL == el_p) {
	return EINVAL;
    }

    if (sizof_element < e->sizof_element) {
	return EINVAL;
    }

    if (!(num < e->cnt)) {
	return ENOENT;
    }

    item_p = list_search_item(e, num);
    if (item_p == NULL) {
	return -1;
    }

    memcpy(item_p->data, el_p, e->sizof_element);

    return 0;
}

/**
 * @fn int mddl_stl_list_front( mddl_stl_list_t *const self_p, const void *const el_p, const size_t sizeof_element)
 * @brief 先頭に保存されたエレメントを返します
 * @param self_p mddl_stl_list_t構造体インスタンスポインタ
 * @param el_p 取得する要素のバッファポインタ
 * @param sizof_element 要素サイズ
 * @retval 0 成功
 * @retval EINVAL 要素サイズが異なる
 * @retval ENOENT 先頭に保存された要素がない
 * @retval -1 その他の致命的なエラー
 **/
int mddl_stl_list_front( mddl_stl_list_t *const self_p, void *const el_p, const size_t sizof_element)
{
    mddl_stl_list_ext_t *const e = get_stl_list_ext(self_p);
    queitem_t *__restrict tmp = NULL;

    /* エレメントがあるかどうか二重チェック */
    if (e->cnt == 0) {
	return ENOENT;
    } else if( e->sizof_element != sizof_element ) {
	return EINVAL;
    }
    tmp = e->base.next;

    memcpy(el_p, tmp->data, e->sizof_element);

    return 0;
}


/**
 * @fn int mddl_stl_list_back( mddl_stl_list_t *const self_p, void *const el_p, const size_t sizof_element)
 * @brief 最後尾に保存されたエレメントを返します
 * @param self_p mddl_stl_list_t構造体インスタンスポインタ
 * @param el_p 取得する要素のバッファポインタ
 * @param sizof_element 要素サイズ
 * @retval 0 成功
 * @retval EINVAL 要素サイズが異なる
 * @retval ENOENT 先頭に保存された要素がない
 * @retval -1 その他の致命的なエラー
 **/
int mddl_stl_list_back( mddl_stl_list_t *const self_p, void *const el_p, const size_t sizof_element)
{
    mddl_stl_list_ext_t *const e = get_stl_list_ext(self_p);
    queitem_t *__restrict tmp = NULL;

    /* エレメントがあるかどうか二重チェック */
    if (e->cnt == 0) {
	return ENOENT;
    } else if( e->sizof_element != sizof_element ) {
	return EINVAL;
    }
    tmp = e->base.prev;

    memcpy(el_p, tmp->data, e->sizof_element);

    return 0;
}

