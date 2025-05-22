// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Copyright (C) [2022-2025] Renesas Electronics Corporation and/or its
 * affiliates.
 */

#ifndef RS_C_Q_H
#define RS_C_Q_H

////////////////////////////////////////////////////////////////////////////////
/// INCLUDE

#include "rs_type.h"

////////////////////////////////////////////////////////////////////////////////
/// MACRO DEFINITION

////////////////////////////////////////////////////////////////////////////////
/// TYPE DEFINITION

// Q status
struct rs_q {
	u8 free_idx;
	u8 used_idx;
	u8 used_count;
	u8 max_count;
};

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL VARIABLE

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL FUNCTION

// Initialize Q
rs_ret rs_c_q_init(struct rs_q *rs_q, u8 max_count);

// Push Q
s32 rs_c_q_push(struct rs_q *rs_q);

// Pop Q
s32 rs_c_q_pop(struct rs_q *rs_q);

// Pick Q
s32 rs_c_q_pick(struct rs_q *rs_q);

// Check Q empty
rs_ret rs_c_q_empty(struct rs_q *rs_q);

// Check Q full
rs_ret rs_c_q_full(struct rs_q *rs_q);

#endif /* RS_C_Q_H */
