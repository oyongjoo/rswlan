// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Copyright (C) [2022-2025] Renesas Electronics Corporation and/or its
 * affiliates.
 */

#ifndef RS_C_TX_H
#define RS_C_TX_H

////////////////////////////////////////////////////////////////////////////////
/// INCLUDE

#include "rs_type.h"

////////////////////////////////////////////////////////////////////////////////
/// MACRO DEFINITION

#define RS_C_TX_EVENT	    (RS_C_TX_AC_EVENT | RS_C_TX_POWER_EVENT)
#define RS_C_TX_AC_EVENT    (0x01)
#define RS_C_TX_POWER_EVENT (0x02)

////////////////////////////////////////////////////////////////////////////////
/// TYPE DEFINITION

enum rs_c_tx_data_ac
{
	IF_DATA_AC = 0,
	IF_DATA_AC_POWER = 1,

	RS_IF_DATA_MAX,
};

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL VARIABLE

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL FUNCTION

// Initialize TX handler
rs_ret rs_c_tx_init(struct rs_c_if *c_if, u16 tx_buf_num, u16 tx_buf_power_num);

// Deinitialize TX handler
rs_ret rs_c_tx_deinit(struct rs_c_if *c_if);

// Post TX event
rs_ret rs_c_tx_event_post(struct rs_c_if *c_if, u8 ac, s8 vif_idx, u8 *tx_skb);

#endif /* RS_C_TX_H */
