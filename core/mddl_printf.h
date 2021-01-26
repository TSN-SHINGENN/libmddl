#ifndef INC_MDDL_PRINTF_H
#define INC_MDDL_PRINTF_H

#pragma once

#if defined (__cplusplus )
extern "C" {
#endif

void mddl_printf_init( int (*putchar_cb)(int));

#if defined(__GNUC__)
__attribute__((format(printf, 1, 2)))
#endif
int mddl_printf(const char *const fmt, ...);

#if defined (__cplusplus )
}
#endif

#endif /* end of INC_MDDL_PRINTF_H */



