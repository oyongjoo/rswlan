// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Copyright (C) [2022-2025] Renesas Electronics Corporation and/or its
 * affiliates.
 */

////////////////////////////////////////////////////////////////////////////////
/// INCLUDE

#include <linux/string.h>
#include <linux/slab.h>

#include "rs_type.h"

#include "rs_k_mem.h"

////////////////////////////////////////////////////////////////////////////////
/// MACRO DEFINITION

////////////////////////////////////////////////////////////////////////////////
/// TYPE DEFINITION

////////////////////////////////////////////////////////////////////////////////
/// LOCAL VARIABLE

////////////////////////////////////////////////////////////////////////////////
/// LOCAL FUNCTION

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL FUNCTION

void *rs_k_malloc(u32 size)
{
	return kmalloc(size, GFP_KERNEL);
}

void *rs_k_calloc(u32 size)
{
	return kcalloc(size, sizeof(u8), GFP_KERNEL);
}

void *rs_k_realloc(void *ptr, u32 size)
{
	return krealloc(ptr, size, GFP_KERNEL);
}

void *rs_k_memdup(void *ptr, u32 size)
{
	void *temp_ptr = NULL;

	return (((temp_ptr = (void *)rs_k_calloc(size)) != NULL) ? rs_k_memcpy(temp_ptr, ptr, size) : 0);
}

void rs_k_free(void *ptr)
{
	kfree(ptr);
}

void *rs_k_memcpy(void *dest, const void *src, u32 len)
{
	return memcpy(dest, src, len);
}

s32 rs_k_memcmp(const void *cs, const void *ct, u32 len)
{
	return (s32)memcmp(cs, ct, len);
}

void *rs_k_memset(void *s, int c, u32 count)
{
	return memset(s, c, count);
}

#ifndef CONFIG_RS_COMBINE_DRIVER
EXPORT_SYMBOL(rs_k_malloc);
EXPORT_SYMBOL(rs_k_calloc);
EXPORT_SYMBOL(rs_k_realloc);
EXPORT_SYMBOL(rs_k_memdup);
EXPORT_SYMBOL(rs_k_free);
EXPORT_SYMBOL(rs_k_memcpy);
EXPORT_SYMBOL(rs_k_memcmp);
EXPORT_SYMBOL(rs_k_memset);
#endif