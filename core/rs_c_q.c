// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Copyright (C) [2022-2025] Renesas Electronics Corporation and/or its
 * affiliates.
 */

////////////////////////////////////////////////////////////////////////////////
/// INCLUDE

#include "rs_type.h"

#include "rs_c_q.h"

////////////////////////////////////////////////////////////////////////////////
/// MACRO DEFINITION

////////////////////////////////////////////////////////////////////////////////
/// TYPE DEFINITION

////////////////////////////////////////////////////////////////////////////////
/// LOCAL VARIABLE

////////////////////////////////////////////////////////////////////////////////
/// LOCAL FUNCTION

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL FUNCTION

// Initialize Q
rs_ret rs_c_q_init(struct rs_q *rs_q, u8 max_count)
{
	rs_ret ret = RS_FAIL;

	if (rs_q && max_count > 0) {
		rs_q->free_idx = 0;
		rs_q->used_idx = 0;
		rs_q->used_count = 0;
		rs_q->max_count = max_count;

		ret = RS_SUCCESS;
	}

	return ret;
}

// Push Q
s32 rs_c_q_push(struct rs_q *rs_q)
{
	s32 free_idx = RS_FAIL;

	if (rs_q) {
		if (rs_q->used_count == rs_q->max_count) {
			free_idx = RS_FULL;
		} else {
			rs_q->free_idx++;
			if (rs_q->free_idx == rs_q->max_count) {
				rs_q->free_idx = 0;
			}

			rs_q->used_count++;

			free_idx = rs_q->free_idx;
		}
	}

	return free_idx;
}

// Pop Q
s32 rs_c_q_pop(struct rs_q *rs_q)
{
	s32 used_idx = RS_FAIL;

	if (rs_q) {
		if (rs_q->used_count == 0) {
			used_idx = RS_EMPTY;
		} else {
			rs_q->used_idx++;
			if (rs_q->used_idx == rs_q->max_count) {
				rs_q->used_idx = 0;
			}

			rs_q->used_count--;

			used_idx = rs_q->used_idx;
		}
	}

	return used_idx;
}

s32 rs_c_q_pick(struct rs_q *rs_q)
{
	s32 used_idx = RS_FAIL;

	if (rs_q) {
		if (rs_q->used_count == 0) {
			used_idx = RS_EMPTY;
		} else {
			if ((rs_q->used_idx + 1) >= rs_q->max_count) {
				used_idx = 0;
			} else {
				used_idx = (rs_q->used_idx + 1);
			}
		}
	}

	return used_idx;
}

// Check Q empty
rs_ret rs_c_q_empty(struct rs_q *rs_q)
{
	rs_ret ret = RS_FAIL;

	if (rs_q) {
		if (rs_q->used_count == 0) {
			ret = RS_EMPTY;
		} else {
			ret = RS_SUCCESS;
		}
	}

	return ret;
}

// Check Q full
rs_ret rs_c_q_full(struct rs_q *rs_q)
{
	rs_ret ret = RS_FAIL;

	if (rs_q) {
		if (rs_q->used_count >= rs_q->max_count) {
			ret = RS_FULL;
		} else {
			ret = RS_SUCCESS;
		}
	}

	return ret;
}
