// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Copyright (C) [2022-2025] Renesas Electronics Corporation and/or its
 * affiliates.
 */

#ifndef RS_C_RECOVERY_H
#define RS_C_RECOVERY_H

////////////////////////////////////////////////////////////////////////////////
/// INCLUDE

#include "rs_type.h"

////////////////////////////////////////////////////////////////////////////////
/// MACRO DEFINITION

#define RS_C_RECOVERY_EVENT 1

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL FUNCTION

// Initialize RX handler
rs_ret rs_c_recovery_init(struct rs_c_if *c_if);

// Deinitialize RX handler
rs_ret rs_c_recovery_deinit(struct rs_c_if *c_if);

// Post indication event
rs_ret rs_c_recovery_event_post(struct rs_c_if *c_if);
#endif