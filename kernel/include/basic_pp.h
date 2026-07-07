#ifndef BASIC_PP_H
#define BASIC_PP_H

#include <stddef.h>

/* Expand #IF / #ELSE / #ENDIF and #INCLUDE "path" from GFS. */
int basic_preprocess(const char *src, char *out, size_t out_cap);

#endif
