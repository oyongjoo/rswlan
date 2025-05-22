// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Copyright (C) [2022-2025] Renesas Electronics Corporation and/or its
 * affiliates.
 */

#ifndef RS_C_RX_H
#define RS_C_RX_H

////////////////////////////////////////////////////////////////////////////////
/// INCLUDE

#include "rs_type.h"

////////////////////////////////////////////////////////////////////////////////
/// MACRO DEFINITION

#define RS_C_RX_EVENT (1)

////////////////////////////////////////////////////////////////////////////////
/// TYPE DEFINITION

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL VARIABLE

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL FUNCTION

// Initialize RX handler
rs_ret rs_c_rx_init(struct rs_c_if *c_if);

// Deinitialize RX handler
rs_ret rs_c_rx_deinit(struct rs_c_if *c_if);

// Initialize RX Data handler
rs_ret rs_c_rx_data_init(struct rs_c_if *c_if, u16 rx_buf_num);

// Deinitialize RX Data handler
rs_ret rs_c_rx_data_deinit(struct rs_c_if *c_if);

#endif /* RS_C_RX_H */
