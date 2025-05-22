// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Copyright (C) [2022-2025] Renesas Electronics Corporation and/or its
 * affiliates.
 */

////////////////////////////////////////////////////////////////////////////////
/// INCLUDE

#include "rs_type.h"
#include "rs_k_mem.h"
#include "rs_c_dbg.h"
#include "rs_c_if.h"
#include "rs_core.h"
#include "rs_c_rx.h"
#include "rs_c_tx.h"

#include "rs_c_status.h"
#include "rs_c_recovery.h"

////////////////////////////////////////////////////////////////////////////////
/// MACRO DEFINITION

#define C_BITS_RX_EMPTY		    (0)
#define C_BITS_TX_AC_FULL	    (1)
#define C_BITS_TX_POWER_FULL	    (2)

#define C_STATUS_BITS		    (0)
#define C_STATUS_TX_AVAIL_CNT	    (1)
#define C_STATUS_TX_POWER_AVAIL_CNT (2)
#define C_STATUS_RESERVED	    (3)

#define C_STATUS_INIT(c_if)	    (void)rs_k_mutex_create(&c_if->core->status.mutex)
#define C_STATUS_DEINIT(c_if)	    (void)rs_k_mutex_destroy(&c_if->core->status.mutex)
#define C_STATUS_LOCK(c_if)	    (void)rs_k_mutex_lock(&c_if->core->status.mutex)
#define C_STATUS_TRYLOCK(c_if)	    rs_k_mutex_trylock(&c_if->core->status.mutex)
#define C_STATUS_UNLOCK(c_if)	    (void)rs_k_mutex_unlock(&c_if->core->status.mutex)

////////////////////////////////////////////////////////////////////////////////
/// TYPE DEFINITION

////////////////////////////////////////////////////////////////////////////////
/// LOCAL VARIABLE

////////////////////////////////////////////////////////////////////////////////
/// LOCAL FUNCTION

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL FUNCTION

rs_ret rs_c_status_init(struct rs_c_if *c_if, u8 status_buf_num)
{
	rs_ret ret = RS_FAIL;

	if ((c_if) && (c_if->core) && (status_buf_num > 0)) {
		c_if->core->status.value = rs_k_calloc(status_buf_num);
		if (c_if->core->status.value) {
			C_STATUS_INIT(c_if);

			ret = RS_SUCCESS;
		}
	}

	return ret;
}

rs_ret rs_c_status_deinit(struct rs_c_if *c_if)
{
	rs_ret ret = RS_FAIL;

	if (c_if && c_if->core) {
		C_STATUS_LOCK(c_if);
		if (c_if->core->status.value) {
			rs_k_free(c_if->core->status.value);
			c_if->core->status.value = NULL;
		}
		C_STATUS_UNLOCK(c_if);

		C_STATUS_DEINIT(c_if);
		ret = RS_SUCCESS;
	}

	return ret;
}

rs_ret rs_c_update_status(struct rs_c_if *c_if)
{
	rs_ret ret = RS_FAIL;

	if (c_if && c_if->core && c_if->core->status.value) {
		C_STATUS_LOCK(c_if);
		if (c_if->core->status.value) {
			ret = rs_c_if_read_status(c_if, (u8 *)(c_if->core->status.value),
						  RS_CORE_STATUS_SIZE);
		}
		C_STATUS_UNLOCK(c_if);

		if (ret != RS_SUCCESS)
			rs_c_recovery_event_post(c_if);
	}

	return ret;
}

rs_ret rs_c_status(struct rs_c_if *c_if)
{
	rs_ret ret = RS_SUCCESS;

	if (c_if && c_if->core && c_if->core->status.value) {
#ifdef C_RX_THREAD
		if (c_if->core->rx.event) {
			// call RX Thread
			(void)rs_k_event_post(c_if->core->rx.event, RS_C_RX_EVENT);
		}
#else
		ret = rs_k_workqueue_add_work(c_if->core->wq, &(c_if->core->rx.work));
#endif
	}

	return ret;
}

void rs_c_set_status_bits(struct rs_c_if *c_if, u8 status_bits)
{
	if ((c_if) && (c_if->core) && (c_if->core->status.value)) {
		(c_if->core->status.value[C_STATUS_BITS]) = status_bits;
	}
}

void rs_c_set_status(struct rs_c_if *c_if, u32 status)
{
	if ((c_if) && (c_if->core) && (c_if->core->status.value)) {
		*((u32 *)(c_if->core->status.value)) = status;
	}
}

u8 rs_c_get_status_rx(struct rs_c_if *c_if)
{
	u8 status = 0;

	if (c_if && c_if->core && c_if->core->status.value) {
		status = (((c_if->core->status.value[C_STATUS_BITS]) & RS_BIT(C_BITS_RX_EMPTY)) != 0);
	}

	return status;
}

u8 rs_c_get_status_tx_ac(struct rs_c_if *c_if)
{
	u8 status = 0;

	if (c_if && c_if->core && c_if->core->status.value) {
		status = (((c_if->core->status.value[C_STATUS_BITS]) & RS_BIT(C_BITS_TX_AC_FULL)) != 0);
	}

	return status;
}

u8 rs_c_get_status_tx_power(struct rs_c_if *c_if)
{
	u8 status = 0;

	if (c_if && c_if->core && c_if->core->status.value) {
		status = (((c_if->core->status.value[C_STATUS_BITS]) & RS_BIT(C_BITS_TX_POWER_FULL)) != 0);
	}

	return status;
}

u8 rs_c_get_status_tx(struct rs_c_if *c_if, u8 ac)
{
	u8 status = 0;

	if (ac == IF_DATA_AC) {
		status = rs_c_get_status_tx_ac(c_if);

	} else if (ac == IF_DATA_AC_POWER) {
		status = rs_c_get_status_tx_power(c_if);
	}

	return status;
}

u8 rs_c_get_status_tx_avail_cnt(struct rs_c_if *c_if, u8 ac)
{
	u8 cnt = 0;

	if (c_if && c_if->core && c_if->core->status.value) {
		if (ac == IF_DATA_AC) {
			cnt = c_if->core->status.value[C_STATUS_TX_AVAIL_CNT];
		} else if (ac == IF_DATA_AC_POWER) {
			cnt = c_if->core->status.value[C_STATUS_TX_POWER_AVAIL_CNT];
		}
	}

	return cnt;
}
