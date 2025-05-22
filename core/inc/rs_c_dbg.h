// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Copyright (C) [2022-2025] Renesas Electronics Corporation and/or its
 * affiliates.
 */

#ifndef RS_C_DBG_H
#define RS_C_DBG_H

////////////////////////////////////////////////////////////////////////////////
/// INCLUDE

#include "rs_type.h"
#include "rs_k_dbg.h"

////////////////////////////////////////////////////////////////////////////////
/// MACRO DEFINITION

#define RS_DBG_ENABLE

#define RS_DEFAULT_LOG_LEVEL (RS_DBG_INFO)

#define RS_DRV_NAME	     "rswlan"
#define RS_ERR_NAME	     " E"
#define RS_WARN_NAME	     " W"
#define RS_INFO_NAME	     " I"
#define RS_TRACE_NAME	     " T"
#define RS_DBG_NAME	     " D"
#define RS_VERB_NAME	     " V"

#ifdef RS_DBG_ENABLE
#define RS_DBG_LEVEL(level, fmt, arg...)            \
	do {                                        \
		if (level <= rs_log_level) {        \
			rs_k_dbg_print(fmt, ##arg); \
		}                                   \
	} while (0)
#endif

#define RS_ERR(fmt, arg...)   RS_DBG_LEVEL(RS_DBG_ERROR, RS_DRV_NAME RS_ERR_NAME ": " fmt, ##arg)
#define RS_WARN(fmt, arg...)  RS_DBG_LEVEL(RS_DBG_WARN, RS_DRV_NAME RS_WARN_NAME ": " fmt, ##arg)
#define RS_INFO(fmt, arg...)  RS_DBG_LEVEL(RS_DBG_INFO, RS_DRV_NAME RS_INFO_NAME ": " fmt, ##arg)
#define RS_TRACE(fmt, arg...) RS_DBG_LEVEL(RS_DBG_TRACE, RS_DRV_NAME RS_TRACE_NAME ": " fmt, ##arg)
#define RS_DBG(fmt, arg...)   RS_DBG_LEVEL(RS_DBG_DEBUG, RS_DRV_NAME RS_DBG_NAME ": " fmt, ##arg)
#define RS_VERB(fmt, arg...)  RS_DBG_LEVEL(RS_DBG_VERBOSE, RS_DRV_NAME RS_VERB_NAME ": " fmt, ##arg)

#define RS_SET_DBG_LEVEL(level)            \
	{                                  \
		rs_k_dbg_set_level(level); \
	}

#define RS_FN_ENTRY_STR_ ">>> %s()"
#define RS_FN_ENTRY_STR	 ">>> %s()\n", __func__

////////////////////////////////////////////////////////////////////////////////
/// TYPE DEFINITION

enum rs_c_dbg_level
{
	RS_DBG_OFF = 0x00,
	RS_DBG_ERROR = 0x01,
	RS_DBG_WARN = 0x02,
	RS_DBG_INFO = 0x04,
	RS_DBG_DEBUG = 0x08,
	RS_DBG_TRACE = 0x10,
	RS_DBG_VERBOSE = 0x20
};

struct rs_c_dbg_stat_t {
	/// Tx statistics
	struct {
		u32 nb_sent;
		u32 nb_if_err;
	} tx;
	/// Rx statistics
	struct {
		u32 nb_recv;
		u32 nb_err_len;
	} rx;
};

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL VARIABLE

extern u32 rs_log_level;

extern struct rs_c_dbg_stat_t rs_c_dbg_stat;

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL FUNCTION

#endif /* RS_C_DBG_H */
