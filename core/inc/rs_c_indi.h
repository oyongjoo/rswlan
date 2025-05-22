// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Copyright (C) [2022-2025] Renesas Electronics Corporation and/or its
 * affiliates.
 */

#ifndef RS_C_INDI_H
#define RS_C_INDI_H

////////////////////////////////////////////////////////////////////////////////
/// INCLUDE

#include "rs_type.h"
#include "rs_c_data.h"

////////////////////////////////////////////////////////////////////////////////
/// MACRO DEFINITION

#define RS_C_INDI_EVENT (1)

////////////////////////////////////////////////////////////////////////////////
/// TYPE DEFINITION

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL VARIABLE

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL FUNCTION

// Initialize indication core
rs_ret rs_c_indi_init(struct rs_c_if *c_if, u16 indi_buf_num);

// Deinitialize indication core
rs_ret rs_c_indi_deinit(struct rs_c_if *c_if);

// Post indication event
rs_ret rs_c_indi_event_post(struct rs_c_if *c_if, struct rs_c_indi *indi_data);

#endif /* RS_C_INDI_H */
