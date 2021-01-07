/**
 *	Copyright 2014 TSN-SHINGENN All Rights Reserved.
 *
 *	Basic Author: Seiichi Takeda  '2020-December-01 Active
 *		Last Alteration $Author: takeda $
 *
 *	Dual License :
 *	non-commercial ... MIT Licence
 *	    commercial ... Requires permission from the author
 */

#include <stddef.h>
#include <stdint.h>

#include "mddl_vsprintf.h"
#include "mddl_printf.h"

static int (*_this_out_method_cb)(int) = NULL;

void mddl_printf_init( int (*putchar_cb)(int))
{
    _this_out_method_cb = putchar_cb;
}


int mddl_printf(const char *const fmt, ...)
{
    int retval;
    va_list ap;    

    if( _this_out_method_cb == NULL ) {
	return -1;
    }

    va_start(ap, fmt); 
    retval = mddl_vsprintf_putchar( _this_out_method_cb, fmt, ap);
    va_end(ap);

    return retval;
}


