// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Copyright (C) [2022-2025] Renesas Electronics Corporation and/or its
 * affiliates.
 */

////////////////////////////////////////////////////////////////////////////////
/// INCLUDE

#include <linux/printk.h>

#include "rs_type.h"
#include "rs_c_dbg.h"
#include "rs_k_dbg.h"

////////////////////////////////////////////////////////////////////////////////
/// MACRO DEFINITION

////////////////////////////////////////////////////////////////////////////////
/// TYPE DEFINITION

////////////////////////////////////////////////////////////////////////////////
/// LOCAL VARIABLE

u32 rs_log_level = RS_DEFAULT_LOG_LEVEL;

////////////////////////////////////////////////////////////////////////////////
/// LOCAL FUNCTION

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL FUNCTION

u32 rs_k_dbg_set_level(u32 level)
{
	if (level >= RS_DBG_OFF && level <= RS_DBG_VERBOSE) {
		rs_log_level = level;
	}

	return rs_log_level;
}

void rs_k_dbg_print(const u8 *fmt, ...)
{
	va_list args = { 0 };

	va_start(args, fmt);
	(void)vprintk(fmt, args);
	va_end(args);
}

#ifndef CONFIG_RS_COMBINE_DRIVER
EXPORT_SYMBOL(rs_log_level);
EXPORT_SYMBOL(rs_k_dbg_print);
#endif