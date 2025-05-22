// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Copyright (C) [2022-2025] Renesas Electronics Corporation and/or its
 * affiliates.
 */

#ifndef RS_C_CTRL_H
#define RS_C_CTRL_H

////////////////////////////////////////////////////////////////////////////////
/// INCLUDE

#include "rs_type.h"
#include "rs_c_data.h"

////////////////////////////////////////////////////////////////////////////////
/// MACRO DEFINITION

////////////////////////////////////////////////////////////////////////////////
/// TYPE DEFINITION

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL VARIABLE

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL FUNCTION

// Initialize control core
rs_ret rs_c_ctrl_init(struct rs_c_if *c_if);

// Deinitialize control core
rs_ret rs_c_ctrl_deinit(struct rs_c_if *c_if);

// Set control command and no wait
rs_ret rs_c_ctrl_set(struct rs_c_if *c_if, u8 cmd_id, u16 req_data_len, u8 *req_data);

// Set control command and wait response
rs_ret rs_c_ctrl_set_and_wait(struct rs_c_if *c_if, u8 cmd_id, u16 req_data_len, u8 *req_data,
			      struct rs_c_ctrl_rsp *ctrl_rsp_data);

// Post control response event
rs_ret rs_c_ctrl_event_post(struct rs_c_if *c_if, struct rs_c_ctrl_rsp *ctrl_rsp_data);

#endif /* RS_C_CTRL_H */
