#ifndef INC_MDDL_SPRINTF_H
#define INC_MDDL_SPRINTF_H

#pragma once

#include <stddef.h>

#if defined(__cplusplus)
extern "C" {
#endif

int mddl_sprintf(char *const buf, const char *const fmt, ...);
int mddl_snprintf(char *const buf, const size_t max_length, const char *const format, ...);

#if defined(__cplusplus)
}
#endif

#endif /* end of INC_MDDL_SPRINTF_H */
