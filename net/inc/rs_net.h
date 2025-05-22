// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Copyright (C) [2022-2025] Renesas Electronics Corporation and/or its
 * affiliates.
 */

#ifndef RS_NET_H
#define RS_NET_H

////////////////////////////////////////////////////////////////////////////////
/// INCLUDE

#include "rs_type.h"
#include "rs_c_if.h"

////////////////////////////////////////////////////////////////////////////////
/// MACRO DEFINITION

#define RS_NET_CH_INIT_VALUE	  (0xFF)

#define RS_NET_INID_BUF_Q_MAX	  (64)
#define RS_NET_RX_BUF_Q_MAX	  (64)
#define RS_NET_TX_BUF_Q_MAX	  (128)
#define RS_NET_TX_POWER_BUF_Q_MAX (32)

////////////////////////////////////////////////////////////////////////////////
/// TYPE DEFINITION

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL VARIABLE

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL FUNCTION

// Initialize Network Stack
rs_ret rs_net_init(struct rs_c_if *c_if);

// Deinitialize Network Stack
rs_ret rs_net_deinit(struct rs_c_if *c_if);

#endif /* RS_NET_H */
