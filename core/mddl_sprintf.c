/** 
 *      Copyright 2018 TSN-SHINGENN All Rights Reserved.
 * 
 *      Basic Author: Seiichi Takeda  '2018-May-25 Active 
 *              Last Alteration $Author: takeda $
 */

/**
 * @file mddl_sprintf.c
 * @brief mddl_vsprintfを使用した文字列処理ライブラリ。
 */

#if defined(WIN32)
/* Microsoft Windows Series */
#define _CRT_SECURE_NO_WARNINGS
#if _MSC_VER >= 1400            /* VC++2005 */
#pragma warning ( disable:4996 )
#pragma warning ( disable:4819 )
#endif
#endif

#include <stddef.h>
#include <stdarg.h>

#ifdef __GNUC__
__attribute__ ((unused))
#endif
#ifdef DEBUG
static int debuglevel = 1;
#else
static const int debuglevel = 0;
#endif
#include "dbms.h"

/* this */
#include "mddl_vsprintf.h"
#include "mddl_sprintf.h"

#define EOL_CRLF "\n\r"

int mddl_sprintf(char *const buf, const char *const fmt, ...)
{
    int retval;
    va_list ap;

    DBMS5( "%s : fmt=%s" EOL_CRLF,__func__, fmt);

    va_start(ap, fmt);
    retval = mddl_vsprintf( buf, fmt, ap);
    va_end(ap);

    return retval;
}

int mddl_snprintf(char *const buf, const size_t max_length, const char *const fmt, ...)
{
    int retval;
    va_list ap;

    DBMS5( "%s : fmt=%s" EOL_CRLF,__func__, fmt);

    va_start(ap, fmt);
    retval = mddl_vsnprintf( buf, max_length, fmt, ap);
    va_end(ap);

    return retval;
}

