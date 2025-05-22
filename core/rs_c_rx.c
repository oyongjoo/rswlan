// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Copyright (C) [2022-2025] Renesas Electronics Corporation and/or its
 * affiliates.
 */

////////////////////////////////////////////////////////////////////////////////
/// INCLUDE

#include "rs_type.h"
#include "rs_k_event.h"
#include "rs_k_thread.h"
#include "rs_k_mem.h"

#include "rs_c_dbg.h"
#include "rs_c_if.h"
#include "rs_core.h"
#include "rs_c_cmd.h"
#include "rs_c_data.h"
#include "rs_c_status.h"
#include "rs_c_ctrl.h"
#include "rs_c_indi.h"

#include "rs_net_rx_data.h"

#include "rs_c_tx.h"
#include "rs_c_rx.h"

////////////////////////////////////////////////////////////////////////////////
/// MACRO DEFINITION

#define C_RX_THREAD_NAME		 "RSW_RX_THREAD"
#define C_RX_DATA_THREAD_NAME		 "RSW_RX_DATA_THREAD"

#define RS_C_RX_DATA_EVENT		 (1)

////////////////////////////////////////////////////////////////////////////////
/// TYPE DEFINITION

#define C_RX_DATA_SPIN_LOCK_INIT(c_if)	 (void)rs_k_spin_lock_create(&c_if->core->rx_data.spin_lock)
#define C_RX_DATA_SPIN_LOCK_DEINIT(c_if) (void)rs_k_spin_lock_destroy(&c_if->core->rx_data.spin_lock)
#define C_RX_DATA_SPIN_LOCK(c_if)	 (void)rs_k_spin_lock(&c_if->core->rx_data.spin_lock)
#define C_RX_DATA_SPIN_TRYLOCK(c_if)	 rs_k_spin_trylock(&c_if->core->rx_data.spin_lock)
#define C_RX_DATA_SPIN_UNLOCK(c_if)	 (void)rs_k_spin_unlock(&c_if->core->rx_data.spin_lock)

////////////////////////////////////////////////////////////////////////////////
/// LOCAL VARIABLE

////////////////////////////////////////////////////////////////////////////////
/// LOCAL FUNCTION

// RX Status

static rs_ret c_rx_status_set(struct rs_c_if *c_if, struct rs_c_rx_status *rx_status)
{
	rs_ret ret = RS_SUCCESS;

	if (rx_status->ext_len == RS_C_RX_STATUS_EXT_LEN) {
		rs_c_set_status(c_if, rx_status->ext_hdr.status);
	}

	return ret;
}

// RX DATA

static rs_ret c_rx_data_push(struct rs_c_if *c_if, struct rs_c_rx_data *rx_data)
{
	rs_ret ret = RS_FAIL;
	s32 free_idx = RS_FAIL;

	if (c_if && c_if->core && rx_data) {
		C_RX_DATA_SPIN_LOCK(c_if);

		free_idx = rs_c_q_push(&c_if->core->rx_data.buf_q);

		if (free_idx != RS_FULL) {
			if (!c_if->core->rx_data.buf[free_idx]) {
				c_if->core->rx_data.buf[free_idx] = rx_data;
			} else {
				RS_ERR("rx q push err : uidx[%d]:ucnt[%d]:fidx[%d]\n",
				       c_if->core->rx_data.buf_q.used_idx,
				       c_if->core->rx_data.buf_q.used_count,
				       c_if->core->rx_data.buf_q.free_idx);
			}
			// check full
			// free_idx = rs_c_q_full(&(c_if->core->rx_data.buf_q));
		} else {
			RS_ERR("rx full[%d]\n", free_idx);
		}

		C_RX_DATA_SPIN_UNLOCK(c_if);
	}

	if (free_idx >= 0) {
		ret = RS_SUCCESS;
	} else {
		ret = free_idx;
	}

	return ret;
}

static rs_ret c_rx_data_pop(struct rs_c_if *c_if, struct rs_c_rx_data **rx_data)
{
	rs_ret ret = RS_FAIL;
	s32 used_idx = RS_FAIL;

	if (c_if && c_if->core && rx_data) {
		C_RX_DATA_SPIN_LOCK(c_if);

		used_idx = rs_c_q_pop(&c_if->core->rx_data.buf_q);

		if (used_idx != RS_EMPTY) {
			if (c_if->core->rx_data.buf[used_idx]) {
				*rx_data = c_if->core->rx_data.buf[used_idx];
				c_if->core->rx_data.buf[used_idx] = NULL;
			} else {
				RS_ERR("rx q pop err : uidx[%d]:ucnt[%d]:fidx[%d]\n",
				       c_if->core->rx_data.buf_q.used_idx,
				       c_if->core->rx_data.buf_q.used_count,
				       c_if->core->rx_data.buf_q.free_idx);
			}
		}

		C_RX_DATA_SPIN_UNLOCK(c_if);
	}

	if (used_idx >= 0) {
		ret = RS_SUCCESS;
	} else {
		// RS_DBG("P:%s[%d]:skb NULL!![%d]\n", __func__, __LINE__, used_idx);
		ret = used_idx;
	}

	return ret;
}

static rs_ret c_rx_data_q_free(struct rs_c_if *c_if)
{
	rs_ret ret = RS_FAIL;
	struct rs_c_rx_data *temp_rx_data = NULL;

	while ((ret = c_rx_data_pop(c_if, &temp_rx_data)) >= RS_SUCCESS) {
		if (temp_rx_data) {
			rs_k_free(temp_rx_data);
			temp_rx_data = NULL;
		}
	}

	return ret;
}

static rs_ret c_rx_data(struct rs_c_if *c_if)
{
	rs_ret ret = RS_FAIL;
	struct rs_c_rx_data *temp_rx_data = NULL;

	while ((rs_c_q_empty(&c_if->core->rx_data.buf_q) != RS_EMPTY)
#ifdef C_RX_THREAD
	       && (rs_k_thread_is_running() == RS_SUCCESS)
#endif
	) {
		ret = c_rx_data_pop(c_if, &temp_rx_data);
		if (ret == RS_SUCCESS) {
			ret = rs_net_rx_data(c_if, temp_rx_data);
		}

		if (temp_rx_data) {
			rs_k_free(temp_rx_data);
			temp_rx_data = NULL;
		}
	}

	if (temp_rx_data) {
		RS_DBG("P:%s[%d]:rx_data memory leak!!:[%d]\n", __func__, __LINE__, ret);
	}

	return ret;
}

#ifdef C_RX_THREAD
static s32 c_rx_data_thread(void *param)
{
	rs_ret ret = RS_FAIL;
	struct rs_c_if *c_if = param;
	rs_k_event_t ret_event = 0;

	if (c_if && c_if->core) {
		do {
			ret_event = rs_k_event_wait(c_if->core->rx_data.event, RS_C_RX_DATA_EVENT);

			if (ret_event == RS_C_RX_DATA_EVENT) {
				ret = c_rx_data(c_if);
			}

			(void)rs_k_event_reset(c_if->core->rx_data.event);

		} while ((rs_k_thread_is_running() == RS_SUCCESS) && (ret_event != K_EVENT_EXIT));

		(void)rs_k_event_destroy(c_if->core->rx_data.event);
		if (c_if->core->rx_data.event) {
			rs_k_free(c_if->core->rx_data.event);
		}
		c_if->core->rx_data.event = NULL;

		(void)rs_k_thead_destroy_wait(&c_if->core->rx_data.thread);
	}

	return 0;
}
#else

static void c_rx_data_work_handler(void *param)
{
	struct rs_c_if *c_if = param;

	if (c_if && c_if->core) {
		(void)c_rx_data(c_if);
	}
}

#endif

// Post RX DATA event
static rs_ret rs_c_rx_data_event_post(struct rs_c_if *c_if, struct rs_c_rx_data *rx_data)
{
	rs_ret ret = RS_FAIL;

	if (c_if && c_if->core) {
		if (rx_data) {
			// update data status
			if (rx_data->ext_len == RS_C_RX_EXT_LEN) {
				rs_c_set_status(c_if, rx_data->ext_hdr.status);
			}

			// push rx data
			ret = c_rx_data_push(c_if, rx_data);
		}
		if (rs_c_q_empty(&c_if->core->rx_data.buf_q) != RS_EMPTY) {
#ifdef C_RX_THREAD
			(void)rs_k_event_post(c_if->core->rx_data.event, RS_C_RX_DATA_EVENT);
#else
			ret = rs_k_workqueue_add_work(c_if->core->wq, &(c_if->core->rx_data.work));
#endif
		}
	}

	return ret;
}

////////////////////////////////////////////////////////////////////////////////

// RX
static rs_ret c_rx_push(struct rs_c_if *c_if)
{
	rs_ret ret = RS_SUCCESS;
	u8 *temp_rx_buf = NULL;
	u8 cmd_id = 0;

	while (
#ifdef C_RX_THREAD
		(rs_k_thread_is_running() == RS_SUCCESS)
#else
		TRUE
#endif
	) {
		if (!temp_rx_buf) {
			temp_rx_buf = rs_k_calloc(sizeof(struct rs_c_rx_data));
		}
		if (temp_rx_buf) {
			ret = rs_c_if_read(c_if, RS_C_IF_READ_CMD, (u8 *)temp_rx_buf,
					   sizeof(struct rs_c_rx_data));

			if (ret >= RS_SUCCESS) {
				cmd_id = ((struct rs_c_rx_data *)temp_rx_buf)->cmd;

				if (RS_C_IS_DATA_RX(cmd_id)) {
					// RX DATA
					ret = rs_c_rx_data_event_post(c_if,
								      (struct rs_c_rx_data *)temp_rx_buf);
					if (ret == RS_SUCCESS) {
						temp_rx_buf = NULL;
					}
				} else if (RS_C_IS_STATUS_RX(cmd_id)) {
					ret = c_rx_status_set(c_if, (struct rs_c_rx_status *)temp_rx_buf);
				} else if (RS_C_IS_COMMON_CMD(cmd_id) || RS_C_IS_FMAC_CTRL_CMD(cmd_id) ||
					   RS_C_IS_DBG_CMD(cmd_id)) {
					// Response
					ret = rs_c_ctrl_event_post(c_if, (struct rs_c_ctrl_rsp *)temp_rx_buf);
				} else if (RS_C_IS_FMAC_INDI_CMD(cmd_id)) {
					// Indication
					ret = rs_c_indi_event_post(c_if, (struct rs_c_indi *)temp_rx_buf);
					if (ret == RS_SUCCESS) {
						temp_rx_buf = NULL;
					}
				} else {
					// TODO
					RS_INFO("P:%s[%d]:cmd_id[%d]\n", __func__, __LINE__, cmd_id);
					break;
				}
			}
		}

		if (rs_c_get_status_rx(c_if)) {
			break;
		}
	}

	if (temp_rx_buf) {
		rs_k_free(temp_rx_buf);
		temp_rx_buf = NULL;
	}

	return ret;
}

#ifdef C_RX_THREAD
static s32 c_rx_thread(void *param)
{
	struct rs_c_if *c_if = param;
	rs_k_event_t ret_event = 0;

	if (c_if && c_if->core) {
		do {
			ret_event = rs_k_event_wait(c_if->core->rx.event, RS_C_RX_EVENT);

			if (ret_event == RS_C_RX_EVENT) {
				(void)c_rx_push(c_if);
			}

			(void)rs_k_event_reset(c_if->core->rx.event);

		} while ((rs_k_thread_is_running() == RS_SUCCESS) && (ret_event != K_EVENT_EXIT));

		(void)rs_k_event_destroy(c_if->core->rx.event);
		if (c_if->core->rx.event) {
			rs_k_free(c_if->core->rx.event);
		}
		c_if->core->rx.event = NULL;

		(void)rs_k_thead_destroy_wait(&c_if->core->rx.thread);
	}

	return 0;
}
#else

static void c_rx_work_handler(void *param)
{
	struct rs_c_if *c_if = param;

	if (c_if && c_if->core) {
		(void)c_rx_push(c_if);
	}
}

#endif

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL FUNCTION

rs_ret rs_c_rx_init(struct rs_c_if *c_if)
{
	rs_ret ret = RS_FAIL;

	RS_TRACE(RS_FN_ENTRY_STR);

	if (c_if && c_if->core) {
#ifdef C_RX_THREAD
		// RX
		c_if->core->rx.event = rs_k_calloc(sizeof(struct rs_k_event));
		if (c_if->core->rx.event) {
			ret = rs_k_event_create(c_if->core->rx.event);
		} else {
			ret = RS_MEMORY_FAIL;
		}

		if (ret == RS_SUCCESS) {
			ret = rs_k_thead_create(&c_if->core->rx.thread, c_rx_thread, c_if,
						(u8 *)C_RX_THREAD_NAME);
		}
#else
		ret = rs_k_workqueue_init_work(&(c_if->core->rx.work), c_rx_work_handler, c_if);
#endif
	}

	return ret;
}

rs_ret rs_c_rx_deinit(struct rs_c_if *c_if)
{
	rs_ret ret = RS_FAIL;

	RS_TRACE(RS_FN_ENTRY_STR);

	if (c_if && c_if->core) {
#ifdef C_RX_THREAD
		(void)rs_k_event_post(c_if->core->rx.event, K_EVENT_EXIT);
		ret = rs_k_thread_destroy(&c_if->core->rx.thread);
#else
		(void)rs_k_workqueue_free_work(&(c_if->core->rx.work));
#endif
	}

	return ret;
}

rs_ret rs_c_rx_data_init(struct rs_c_if *c_if, u16 rx_buf_num)
{
	rs_ret ret = RS_FAIL;

	RS_TRACE(RS_FN_ENTRY_STR);

	// RX DATA
	if (c_if && c_if->core && (rx_buf_num > 0)) {
		C_RX_DATA_SPIN_LOCK_INIT(c_if);

		c_if->core->rx_data.buf =
			(struct rs_c_rx_data **)rs_k_calloc(rx_buf_num * sizeof(struct rs_c_rx_data));
		if (c_if->core->rx_data.buf) {
			c_if->core->rx_data.buf_num = rx_buf_num;
			ret = rs_c_q_init(&c_if->core->rx_data.buf_q, rx_buf_num);

#ifdef C_RX_THREAD
			c_if->core->rx_data.event = rs_k_calloc(sizeof(struct rs_k_event));
			if (c_if->core->rx_data.event) {
				ret = rs_k_event_create(c_if->core->rx_data.event);
			} else {
				ret = RS_MEMORY_FAIL;
			}

			if (ret == RS_SUCCESS) {
				ret = rs_k_thead_create(&c_if->core->rx_data.thread, c_rx_data_thread, c_if,
							(u8 *)C_RX_DATA_THREAD_NAME);
			}
#else
			ret = rs_k_workqueue_init_work(&(c_if->core->rx_data.work), c_rx_data_work_handler,
						       c_if);
#endif
		} else {
			c_if->core->rx_data.buf_num = 0;
			ret = RS_MEMORY_FAIL;
		}
	}

	return ret;
}

rs_ret rs_c_rx_data_deinit(struct rs_c_if *c_if)
{
	rs_ret ret = RS_FAIL;

	RS_TRACE(RS_FN_ENTRY_STR);

	if (c_if && c_if->core) {
#ifdef C_RX_THREAD
		(void)rs_k_event_post(c_if->core->rx_data.event, K_EVENT_EXIT);
		ret = rs_k_thread_destroy(&c_if->core->rx_data.thread);
#else
		(void)rs_k_workqueue_free_work(&(c_if->core->rx_data.work));
#endif

		// free Q
		ret = c_rx_data_q_free(c_if);

		// free buf
		if (c_if->core->rx_data.buf) {
			rs_k_free(c_if->core->rx_data.buf);
			c_if->core->rx_data.buf = NULL;
			c_if->core->rx_data.buf_num = 0;
		}

		C_RX_DATA_SPIN_LOCK_DEINIT(c_if);
	}

	return ret;
}