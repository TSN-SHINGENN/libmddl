/**
 *      Copyright 2011 TSN-SHINGENN All Rights Reserved.
 *
 *      Basic Author: Seiichi Takeda  '2011-january-01 Active
 *              Last Alteration $Author: takeda $
 *
 *	Dual License :
 *	non-commercial ... MIT Licence
 *	    commercial ... Requires permission from the author
 **/

/**
 * @file mddl_malloc.c
 * @brief 1.アライメンを考慮したmallocのmddl版
 *	2.MMUの無いCPU用でもlibmddlを使用できるようにします。
 *　	これはmddl_lite_mallocaterを使用し、STATICバッファを擬似的にHEAPにします。
 *	標準APIによるメモリ消費を軽減します。また、マイコン搭載時には、メモリ管理を見える化します。
 *	切り替えた場合はmddl_lite_mallocater_init()で必ず初期化してください。
 **/

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "mddl_mallocater.h"
#include "mddl_malloc.h"

#ifdef DEBUG
static int debuglevel = 2;
#else
static const int debuglevel = 0;
#endif
#include "dbms.h"

#define EOL_CRLF "\n\r"

/**
 * @fn int mddl_malloc_align(void **memptr, const size_t alignment, const size_t size)
 * @brief アライメントを考慮したメモリ割り当てを行います。
 *	確保したメモリの解放にはmddl_mfree()を使ってください
 * @param memptr 割り当てられたメモリを格納するメモリブロックポインタ
 * @param alignment アライメント(配置)の値。2 の累乗値を指定する必要があります。 
 *	もし0が指定された場合にはsizeof(int)が設定されます
 * @param size メモリブロックのサイズ
 * @retval 0 成功
 * @retval EINVAL 引数エラー
 * @retval ENOMEM メモリ確保失敗
 * @retval -1 原因不明のエラー(コードが知らないだけかも?)
 **/
int mddl_malloc_align(void **memptr, const size_t alignment,
			 const size_t size)
{
    int result;
    const size_t a = (alignment == 0) ? sizeof(uint64_t) : alignment;
    void *mem = NULL;
    uint8_t *aligned_mem = NULL;

    DBMS5("%s : execute" EOL_CRLF, __func__);
    if ((a % 2) || (size == 0) || (NULL == memptr)) {
	result = errno = EINVAL;
    } else {
	const size_t total = size + sizeof(void *) + (a - 1);
	mem = mddl_malloc(total);
	if (NULL == mem) {
	    result = errno;
	} else {
	    uintptr_t offs;

	    IFDBG5THEN {
		DBMS5("%s : mddl_malloc = 0x%p total=%llu  alignment=%llu size=%llu" EOL_CRLF,
		    __func__, mem, (unsigned long long)total,(unsigned long long)alignment, (unsigned long long)size);
	    }

	    /**
	     * @note 確保されたメモリのアライメントが整ったアドレスを計算する
	     */
	    aligned_mem = ((uint8_t *) mem + sizeof(void *));
	    offs = (uintptr_t) a - ((uintptr_t) aligned_mem & (a - 1));

	    IFDBG5THEN {
		DBMS5(
		  "%s : a-1=0x%llx offs=0x%llu" EOL_CRLF,
		    __func__, (uintptr_t)a-1, offs);
	    }

	    aligned_mem += offs;

	    IFDBG5THEN {
		DBMS5( "%s : aligned_mem = 0x%p" EOL_CRLF, __func__, aligned_mem);
	    }

	    /* 返すアドレスの一つ前に本来mallocで返されたポインタアドレスを保存しておく */
	    ((void **) aligned_mem)[-1] = mem;

	    *memptr = aligned_mem;
	    result = 0;
	}
    }

    return result;
}

/**
 * @fn int mddl_realloc_align(void *memblk, const size_t alignment, const size_t size)
 * @brief アライメントを考慮したメモリの再割り当てを行います。
 * @param memblk 現在のメモリブロックポインタ
 * @param size 割り当てするメモリのサイズ
 * @param alignment アライメントサイズ。2の累乗値で最大ページサイズ
 * @retval NULL 失敗
 * @retval NULL以外 再割り当てされたポインタ
 */
void *mddl_realloc_align(void *memblk, const size_t alignment,
			    const size_t size)
{
    int result;
    void *mem;
    result = mddl_malloc_align(&mem, alignment, size);
    if (result) {
	return NULL;
    }
    memcpy(mem, memblk, size);
    mddl_mfree(memblk);
    return mem;
}

/**
 * @fn int mddl_mfree(void *memptr)
 * @brief mddl_malloc_align()で確保したメモリを解放します
 *   OS毎に mddl_malloc_align()の挙動が違うので Cランタイムのfree()を使わないでください。
 * @param memptr メモリブロックポインタ
 * @retval 0 成功
 * @retval -1 このOSではサポートされていません
 */
int mddl_mfree(void *memptr)
{
    /**
     * @note 保存していたポインタをfreeに渡してメモリ解放 
     */
    mddl_free(((void **) memptr)[-1]);
    return 0;
}

/**
 * @fn void *mddl_mrealloc_align( void *memblock, const size_t alignment, const size_t size)
 * @brief メモリの再割り当てを行います。
 *	差異割り当てされたメモリへのデータコピーは行われません(C1xで未定義と決まったため）
 * @param memblock 現在メモリのブロックポインた
 * @param alignment アライメント(配置)の値。2 の累乗値を指定する必要があります。 
 *	もし0が指定された場合にはsizeof(uint64_t)が設定されます
 * @param size メモリブロックのサイズ
 * @retval NULL 失敗(errno参照)
 * @retval NULL以外 新しいメモリブロックポインタ
 */
void *mddl_mrealloc_align(void *memblock, const size_t alignment,
			     const size_t size)
{
    size_t a = (alignment == 0) ? sizeof(uint64_t) : alignment;
    int result;
    void *mem = NULL;
    uint8_t *aligned_mem = NULL;

    DBMS5( "%s : execute" EOL_CRLF, __func__);
    if (NULL == memblock) {
	void *memptr = NULL;
	result = mddl_malloc_align(&memptr, a, size);
	return (result == 0) ? memptr : NULL;
    } else if ((a % 2) || (size == 0)) {
	errno = EINVAL;
	return NULL;
    } else {
	mem =
	    mddl_realloc(((void **) memblock)[-1],
		    size + (a - 1) + sizeof(void *));
	if (NULL == mem) {
	    return NULL;
	} else {
	    aligned_mem = ((uint8_t *) mem + sizeof(void *));
	    aligned_mem += ((uintptr_t) aligned_mem & (a - 1));
	    ((void **) aligned_mem)[-1] = mem;
	}
    }

    return aligned_mem;
}

/**
 * @fn mddl_mem_alignment_partition_t mddl_mem_alignment_partition(int *result_p, const void *p, size_t size, size_t alignment)
 * @brief 指定されたメモリ空間のアライメントを確認し、前方・中間・後方に分割します
 * @param result_p 結果返却用変数ポインタ(0:成功 0以外:失敗)
 * @param p メモリの先頭ポインタ
 * @param size メモリのサイズ
 * @param alignment アライメントあわせの為のサイズ
 * @return mddl_mem_alignment_partition_t構造体
 */
mddl_mem_alignment_partition_t mddl_mem_alignment_partition(int
								  *const
								  result_p,
								  const
								  void
								  *const p,
								  size_t
								  size,
								  size_t
								  alignment)
{
    mddl_mem_alignment_partition_t part;
    uintptr_t a = (uintptr_t) p;
    const size_t align_mask = (alignment - 1);

    memset(&part, 0x0, sizeof(mddl_mem_alignment_partition_t));
    part.total_sz = size;
    part.alignment_sz = alignment;

    if (size < alignment) {
	part.foward_sz = size;
    } else {
	part.ext.f.size_is_multiple_alignment =
	    (size & align_mask) ? 0 : 1;
	part.foward_sz = (a & align_mask);
	if (!part.foward_sz) {
	    part.ext.f.is_aligned = 1;
	} else {
	    part.foward_sz = alignment - part.foward_sz;
	    size -= part.foward_sz;
	    part.ext.f.is_aligned = 0;
	}

	if (size != 0) {
	    part.bottom_sz = (size & align_mask);
	    part.middle_sz = size - part.bottom_sz;
	}
    }

    if (NULL != result_p) {
	*result_p = 0;
    }

    return part;
}

/**
 * @fn void *mddl_malloc(size_t size);
 * @breif malloc()のラッパーです。
 *	size バイトを割り当て、 割り当てられたメモリーに対する ポインターを返す。
 *	メモリーの内容は初期化されない。 size が 0 の場合、NULL)を返す。 
 * @param size 割り当ててほしいメモリバイトサイズ
 * @retval NULL 割り当て失敗。errnoにENOMEMを設定する
 **/
void *mddl_malloc(const size_t size)
{
    return mddl_mallocater_alloc(size);
}

/**
 * @fn void mddl_free( void *const ptr)
 * @brief free()のラッパーです。mddl_malloc()で取得したメモリを開放します。
 * @param ptr mddl_malloc()で取得したメモリのポインタを指定。NULLを指定すると。何も処理せず抜けます。
 **/
void mddl_free( void *const ptr)
{
    mddl_mallocater_free(ptr);
}


/**
 * @fn void *mddl_realloc( void *ptr, size_t size)
 * @breif mddl_mallocater_realloc()のラッパーです。
 *	size バイトを再割り当てを行います。
 *	元のメモリエリアで処理できない場合は戻り値はptrと異なるポインタを戻します。
 *	このとき、内部のデータは保障されます。
 * @param size 割り当ててほしいメモリバイトサイズ
 * @retval NULL 割り当て失敗。errnoにENOMEMを設定する
 * @retval NULL以外 再割り当て成功。ptr指定と異なるポインタの場合はptrは開放済
 **/
void *mddl_realloc( void *const ptr, const size_t size)
{
    return mddl_mallocater_realloc(ptr, size);
}

/**
 * @fn void mddl_malloc_init(void *const buf, const size_t bufsize)
 * @brief mddl_mallocaterライブラリを使用して初期化します。
 * @param buf 割り当てるバッファ
 * @param bufsize バッファのサイズ
 */
void mddl_malloc_init(void *const buf, const size_t bufsize)
{
    mddl_mallocater_init_obj(&_mddl_mallocater_heap_obj, buf, bufsize);
}

