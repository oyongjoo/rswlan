// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Copyright (C) [2022-2025] Renesas Electronics Corporation and/or its
 * affiliates.
 */

#ifndef RS_C_STATUS_H
#define RS_C_STATUS_H

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

// Initialize Status handler
rs_ret rs_c_status_init(struct rs_c_if *c_if, u8 status_buf_num);

// Deinitialize Status handler
rs_ret rs_c_status_deinit(struct rs_c_if *c_if);

// update status
rs_ret rs_c_update_status(struct rs_c_if *c_if);

// update status callback
rs_ret rs_c_status(struct rs_c_if *c_if);

// set status bits
void rs_c_set_status_bits(struct rs_c_if *c_if, u8 status_bits);

// set status
void rs_c_set_status(struct rs_c_if *c_if, u32 status);

// rx status : 1 == empty
u8 rs_c_get_status_rx(struct rs_c_if *c_if);

// tx ac status : 1 == full
u8 rs_c_get_status_tx_ac(struct rs_c_if *c_if);

// tx ac power status : 1 == full
u8 rs_c_get_status_tx_power(struct rs_c_if *c_if);

// tx status : 1 == full
u8 rs_c_get_status_tx(struct rs_c_if *c_if, u8 ac);

// tx status : tx avail count
u8 rs_c_get_status_tx_avail_cnt(struct rs_c_if *c_if, u8 ac);

#endif /* RS_C_STATUS_H */
