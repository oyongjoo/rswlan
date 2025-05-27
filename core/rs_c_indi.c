// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Copyright (C) [2022-2025] Renesas Electronics Corporation and/or its
 * affiliates.
 */

////////////////////////////////////////////////////////////////////////////////
/// INCLUDE

#include "rs_type.h"
#include "rs_k_event.h"
#include "rs_k_mem.h"
#include "rs_c_dbg.h"
#include "rs_c_if.h"
#include "rs_core.h"
#include "rs_c_status.h"
#include "rs_c_cmd.h"
#include "rs_net_rx_indi.h"

#include "rs_c_indi.h"

////////////////////////////////////////////////////////////////////////////////
/// MACRO DEFINITION

#define C_INDI_SPIN_INIT(c_if)    (void)rs_k_spinlock_create(&c_if->core->indi.lock)
#define C_INDI_SPIN_DEINIT(c_if) (void)rs_k_spinlock_destroy(&c_if->core->indi.lock)
#define C_INDI_SPIN_LOCK(c_if)   (void)rs_k_spinlock_lock(&c_if->core->indi.lock)
#define C_INDI_SPIN_UNLOCK(c_if) (void)rs_k_spinlock_unlock(&c_if->core->indi.lock)

#define C_IF_INDI_ADDR          (0)
#define C_INDI_THREAD_NAME      "RSW_INDI_THREAD"

////////////////////////////////////////////////////////////////////////////////
/// TYPE DEFINITION

////////////////////////////////////////////////////////////////////////////////
/// LOCAL VARIABLE

////////////////////////////////////////////////////////////////////////////////
/// LOCAL FUNCTION

static rs_ret c_indi_push(struct rs_c_if *c_if, struct rs_c_indi *indi_data)
{
	rs_ret ret = RS_FAIL;
	s32 free_idx = RS_FAIL;

	if (c_if && c_if->core && indi_data) {
		C_INDI_SPIN_LOCK(c_if);

		free_idx = rs_c_q_push(&c_if->core->indi.buf_q);

		if (free_idx != RS_FULL) {
			if (!c_if->core->indi.buf[free_idx]) {
				c_if->core->indi.buf[free_idx] = indi_data;
			} else {
				RS_ERR("indi q push err : uidx[%d]:ucnt[%d]:fidx[%d]\n",
				       c_if->core->indi.buf_q.used_idx, c_if->core->indi.buf_q.used_count,
				       c_if->core->indi.buf_q.free_idx);
			}
		} else {
			RS_ERR("indi full[%d]\n", free_idx);
		}

		C_INDI_SPIN_UNLOCK(c_if);
	}

	if (free_idx >= 0) {
		ret = RS_SUCCESS;
	} else {
		ret = free_idx;
	}

	return ret;
}

static rs_ret c_indi_pop(struct rs_c_if *c_if, struct rs_c_indi **indi_data)
{
	rs_ret ret = RS_FAIL;
	s32 used_idx = RS_FAIL;

	if (c_if && c_if->core && indi_data) {
		C_INDI_MUTEX_LOCK(c_if);

		used_idx = rs_c_q_pop(&c_if->core->indi.buf_q);

		if (used_idx != RS_EMPTY) {
			if (c_if->core->indi.buf[used_idx]) {
				*indi_data = c_if->core->indi.buf[used_idx];
				c_if->core->indi.buf[used_idx] = NULL;
			} else {
				RS_ERR("indi q pop err : uidx[%d]:ucnt[%d]:fidx[%d]\n",
				       c_if->core->indi.buf_q.used_idx, c_if->core->indi.buf_q.used_count,
				       c_if->core->indi.buf_q.free_idx);
			}
		}

		C_INDI_MUTEX_UNLOCK(c_if);
	}

	if (used_idx >= 0) {
		ret = RS_SUCCESS;
	} else {
		ret = used_idx;
	}

	return ret;
}

static rs_ret c_indi_q_free(struct rs_c_if *c_if)
{
	rs_ret ret = RS_FAIL;
	struct rs_c_indi *temp_indi_data = NULL;

	while ((ret = c_indi_pop(c_if, &temp_indi_data)) >= RS_SUCCESS) {
		if (temp_indi_data) {
			rs_k_free(temp_indi_data);
			temp_indi_data = NULL;
		}
	}

	return ret;
}

#ifdef C_RX_THREAD
static s32 c_indi_thread(void *param)
{
	struct rs_c_if *c_if = param;
	rs_k_event_t ret_event = 0;
	struct rs_c_indi *temp_indi_data = NULL;

	if (c_if && c_if->core) {
		do {
			ret_event = rs_k_event_wait(c_if->core->indi.event, RS_C_INDI_EVENT);

			if (ret_event == RS_C_INDI_EVENT) {
				while (c_indi_pop(c_if, &temp_indi_data) == RS_SUCCESS) {
					(void)rs_net_rx_indi(c_if, temp_indi_data);
					if (temp_indi_data) {
						rs_k_free(temp_indi_data);
						temp_indi_data = NULL;
					}
				}
			}

			rs_k_event_reset(c_if->core->indi.event);

		} while ((rs_k_thread_is_running() == RS_SUCCESS) && (ret_event != K_EVENT_EXIT));

		(void)rs_k_event_destroy(c_if->core->indi.event);
		if (c_if->core->indi.event) {
			rs_k_free(c_if->core->indi.event);
		}
		c_if->core->indi.event = NULL;

		(void)rs_k_thead_destroy_wait(&c_if->core->indi.thread);
	}

	return 0;
}
#else

static void c_indi_work_handler(void *param)
{
	struct rs_c_if *c_if = param;
	struct rs_c_indi *temp_indi_data = NULL;

	if (c_if && c_if->core) {
		while (c_indi_pop(c_if, &temp_indi_data) == RS_SUCCESS) {
			(void)rs_net_rx_indi(c_if, temp_indi_data);

			if (temp_indi_data) {
				rs_k_free(temp_indi_data);
				temp_indi_data = NULL;
			}
		}
	}
}

#endif

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL FUNCTION

// Initialize indication core
rs_ret rs_c_indi_init(struct rs_c_if *c_if, u16 indi_buf_num)
{
	rs_ret ret = RS_FAIL;

	RS_TRACE(RS_FN_ENTRY_STR);

	if (c_if && c_if->core && indi_buf_num > 0) {
		C_INDI_SPIN_INIT(c_if);

		c_if->core->indi.buf =
			(struct rs_c_indi **)rs_k_calloc(indi_buf_num * sizeof(struct rs_c_indi));
		if (c_if->core->indi.buf) {
			c_if->core->indi.buf_num = indi_buf_num;
			ret = rs_c_q_init(&c_if->core->indi.buf_q, indi_buf_num);

#ifdef C_RX_THREAD
			c_if->core->indi.event = rs_k_calloc(sizeof(struct rs_k_event));
			if (c_if->core->indi.event) {
				ret = rs_k_event_create(c_if->core->indi.event);
			} else {
				ret = RS_MEMORY_FAIL;
			}
			if (ret == RS_SUCCESS) {
				ret = rs_k_thead_create(&c_if->core->indi.thread, c_indi_thread, c_if,
							(u8 *)C_INDI_THREAD_NAME);
			}
#else
			ret = rs_k_workqueue_init_work(&(c_if->core->indi.work), c_indi_work_handler, c_if);
#endif
		} else {
			c_if->core->indi.buf_num = 0;
			ret = RS_MEMORY_FAIL;
		}
	}

	return ret;
}

// Deinitialize indication core
rs_ret rs_c_indi_deinit(struct rs_c_if *c_if)
{
	rs_ret ret = RS_FAIL;

	RS_TRACE(RS_FN_ENTRY_STR);

	if (c_if && c_if->core) {
#ifdef C_RX_THREAD
		(void)rs_k_event_post(c_if->core->indi.event, K_EVENT_EXIT);
		ret = rs_k_thread_destroy(&c_if->core->indi.thread);
#else
		(void)rs_k_workqueue_free_work(&(c_if->core->indi.work));
#endif

		// free Q
		ret = c_indi_q_free(c_if);

		// free buf
		if (c_if->core->indi.buf) {
			rs_k_free(c_if->core->indi.buf);
			c_if->core->indi.buf = NULL;
			c_if->core->indi.buf_num = 0;
		}

		C_INDI_SPIN_DEINIT(c_if);
	}

	return ret;
}

// Post indication event
rs_ret rs_c_indi_event_post(struct rs_c_if *c_if, struct rs_c_indi *indi_data)
{
	rs_ret ret = RS_FAIL;

	if (c_if && c_if->core) {
		if (indi_data) {
			// update data status
			if (indi_data->ext_len == RS_C_INDI_EXT_LEN) {
				rs_c_set_status(c_if, indi_data->ext_hdr.status);
			}

			ret = c_indi_push(c_if, (struct rs_c_indi *)indi_data);
		}
		if (rs_c_q_empty(&c_if->core->indi.buf_q) != RS_EMPTY) {
#ifdef C_RX_THREAD
			(void)rs_k_event_post(c_if->core->indi.event, RS_C_INDI_EVENT);
#else
			ret = rs_k_workqueue_add_work(c_if->core->wq, &(c_if->core->indi.work));
#endif
		}
	}

	return ret;
}
