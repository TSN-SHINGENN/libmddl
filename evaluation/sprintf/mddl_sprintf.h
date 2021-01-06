#ifndef INC_MDDL_SPRINTF_H
#define INC_MDDL_SPRINTF_H

#pragma once

#include <stddef.h>

#if defined(__cplusplus)
extern "C" {
#endif

int mddl_sprintf(char *const buf, const char *const fmt, ...);
int mddl_snprintf(char *const buf, const size_t max_length, const char *const format, ...);

int mddl_sprintf_putchar(int (*putchar_cbfunc)(int), const char *const fmt, ...);
int mddl_snprintf_putchar(int (*putchar_cbfunc)(int), const size_t max_length, const char *const fmt, ...);

#if defined(__cplusplus)
}
#endif

#endif /* end of INC_MDDL_SPRINTF_H */
