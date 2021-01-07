#ifndef INC_MDDL_SPRINTF_H
#define INC_MDDL_SPRINTF_H

#pragma once

#include <stddef.h>
#include <stdarg.h>

#if defined(__cplusplus)
extern "C" {
#endif

int mddl_vprintf(const char *const fmt, va_list ap);
int mddl_vsprintf(char *const buf, const char *const fmt, va_list ap);
int mddl_vsnprintf(char *const buf, const size_t max_length, const char *const fmt, va_list ap);

int mddl_vsprintf_putchar(int (*putc_cbfunc)(int), const char *const fmt, va_list ap);
int mddl_vsnprintf_putchar(int (*putc_cbfunc)(int), const size_t max_len, const char *const fmt, va_list ap);

#if defined(__cplusplus)
}
#endif

#endif /* end of INC_MDDL_SPRINTF_H */
