// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Copyright (C) [2022-2025] Renesas Electronics Corporation and/or its
 * affiliates.
 */

#ifndef RS_K_MEM_H
#define RS_K_MEM_H

////////////////////////////////////////////////////////////////////////////////
/// INCLUDE

#include "rs_type.h"

////////////////////////////////////////////////////////////////////////////////
/// MACRO DEFINITION

////////////////////////////////////////////////////////////////////////////////
/// TYPE DEFINITION

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL VARIABLE

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL FUNCTION

// allocates memory
void *rs_k_malloc(u32 size);

// allocates memory initialized 0(NULL).
void *rs_k_calloc(u32 size);

// reallocates memory
void *rs_k_realloc(void *ptr, u32 size);

// allocates memory and copy data of ptr
void *rs_k_memdup(void *ptr, u32 size);

// frees allocated memory
void rs_k_free(void *ptr);

// memory copy
void *rs_k_memcpy(void *dest, const void *src, u32 len);

// memory compare
s32 rs_k_memcmp(const void *cs, const void *ct, u32 len);

// memory set
void *rs_k_memset(void *s, int c, u32 count);

#endif /* RS_K_MEM_H */
