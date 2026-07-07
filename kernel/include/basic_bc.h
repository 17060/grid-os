#ifndef BASIC_BC_H
#define BASIC_BC_H

#include <stddef.h>
#include <stdint.h>

#define BASIC_BC_MAGIC "GRIDBC\x01"

int basic_is_bytecode(const void *data, size_t len);
int basic_compile_tokens(const void *tokbuf, size_t tokbytes, void *out, size_t cap, size_t *out_len);
int basic_load_bytecode(const void *data, size_t len);

#endif
