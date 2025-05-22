// SPDX-License-Identifier: GPL-2.0-or-later
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
#include "rs_k_spin_lock.h"

#include "rs_c_dbg.h"
#include "rs_c_if.h"
#include "rs_core.h"
#include "rs_c_cmd.h"
#include "rs_c_data.h"
#include "rs_c_status.h"

#include "rs_net_dev.h"
#include "rs_net_skb.h"

#include "rs_c_tx.h"

////////////////////////////////////////////////////////////////////////////////
/// MACRO DEFINITION

#define C_TX_BUFFER_THRESHOLD (4)

#ifdef C_TX_THREAD
#define C_TX_THREAD_NAME "RSW_TX_THREAD"
#endif

////////////////////////////////////////////////////////////////////////////////
/// TYPE DEFINITION

#define C_TX_SPIN_LOCK_INIT(c_if)   (void)rs_k_spin_lock_create(&c_if->core->tx_data.spin_lock)
#define C_TX_SPIN_LOCK_DEINIT(c_if) (void)rs_k_spin_lock_destroy(&c_if->core->tx_data.spin_lock)
#define C_TX_SPIN_LOCK(c_if)	    (void)rs_k_spin_lock(&c_if->core->tx_data.spin_lock)
#define C_TX_SPIN_TRYLOCK(c_if)	    rs_k_spin_trylock(&c_if->core->tx_data.spin_lock)
#define C_TX_SPIN_UNLOCK(c_if)	    (void)rs_k_spin_unlock(&c_if->core->tx_data.spin_lock)

////////////////////////////////////////////////////////////////////////////////
/// LOCAL VARIABLE

////////////////////////////////////////////////////////////////////////////////
/// LOCAL FUNCTION

static rs_ret c_tx_push(struct rs_c_if *c_if, u8 ac, s8 vif_idx, u8 *tx_skb)
{
	rs_ret ret = RS_FAIL;
	s32 free_idx = RS_FAIL;
	struct rs_q *temp_q = NULL;
	struct rs_c_q_buf *temp_buf = NULL;

	if (c_if && c_if->core && tx_skb) {
		switch (ac) {
		case IF_DATA_AC:
			temp_q = &c_if->core->tx_data.buf_q;
			temp_buf = c_if->core->tx_data.buf;
			break;
		case IF_DATA_AC_POWER:
			temp_q = &c_if->core->tx_data.buf_power_q;
			temp_buf = c_if->core->tx_data.buf_power;
			break;
		default:
			break;
		}

		if (temp_q && temp_buf) {
			C_TX_SPIN_LOCK(c_if);

			free_idx = rs_c_q_push(temp_q);

			if (free_idx != RS_FULL) {
				if (!temp_buf[free_idx].data) {
					temp_buf[free_idx].vif_idx = vif_idx;
					temp_buf[free_idx].data = tx_skb;
				} else {
					RS_DBG("tx [%d] q push err : vi[%d]:uidx[%d]:ucnt[%d]:fidx[%d]\n", ac,
					       vif_idx, temp_q->used_idx, temp_q->used_count,
					       temp_q->free_idx);
				}
			} else {
				RS_DBG("tx [%d] full : [%d]\n", ac, free_idx);
			}

			// check full
			ret = rs_c_q_full(temp_q);
			if (ret == RS_FULL) {
				// Stop net_if
				(void)rs_net_if_tx_stop(c_if, vif_idx, TRUE);
			}

			C_TX_SPIN_UNLOCK(c_if);
		}
	}

	if (free_idx >= 0) {
		ret = RS_SUCCESS;
	} else {
		ret = free_idx;
	}

	return ret;
}

static rs_ret c_tx_pop(struct rs_c_if *c_if, u8 ac, s8 *vif_idx, u8 **tx_skb)
{
	rs_ret ret = RS_FAIL;
	s32 used_idx = RS_FAIL;
	struct rs_q *temp_q = NULL;
	struct rs_c_q_buf *temp_buf = NULL;
	s8 temp_vif_idx = -1;

	if (c_if && c_if->core) {
		switch (ac) {
		case IF_DATA_AC:
			temp_q = &c_if->core->tx_data.buf_q;
			temp_buf = c_if->core->tx_data.buf;
			break;
		case IF_DATA_AC_POWER:
			temp_q = &c_if->core->tx_data.buf_power_q;
			temp_buf = c_if->core->tx_data.buf_power;
			break;
		default:
			break;
		}

		if (temp_q && temp_buf) {
			C_TX_SPIN_LOCK(c_if);

			used_idx = rs_c_q_pop(temp_q);

			if (used_idx != RS_EMPTY) {
				if (temp_buf[used_idx].data) {
					if (tx_skb) {
						*tx_skb = temp_buf[used_idx].data;
					}
					temp_vif_idx = temp_buf[used_idx].vif_idx;
					if (vif_idx) {
						*vif_idx = temp_vif_idx;
					}
					temp_buf[used_idx].data = NULL;
					temp_buf[used_idx].vif_idx = -1;

				} else {
					RS_DBG("tx [%d] q pop err : vi[%d]:uidx[%d]:ucnt[%d]:fidx[%d]\n", ac,
					       vif_idx, temp_q->used_idx, temp_q->used_count,
					       temp_q->free_idx);
				}
			}

			C_TX_SPIN_UNLOCK(c_if);
		}
	}

	if (used_idx >= 0) {
		ret = RS_SUCCESS;
	} else {
		// RS_DBG("P:%s[%d]:skb NULL!![%d]\n", __func__, __LINE__, used_idx);
		ret = used_idx;
	}

	return ret;
}

static rs_ret c_tx_q_free(struct rs_c_if *c_if)
{
	rs_ret ret = RS_FAIL;
	u8 *tx_skb = NULL;

	while ((ret = c_tx_pop(c_if, IF_DATA_AC, NULL, &tx_skb)) >= RS_SUCCESS) {
		if (tx_skb) {
			(void)rs_net_skb_free(tx_skb);
			tx_skb = NULL;
		}
	}

	while ((ret = c_tx_pop(c_if, IF_DATA_AC_POWER, NULL, &tx_skb)) >= RS_SUCCESS) {
		if (tx_skb) {
			(void)rs_net_skb_free(tx_skb);
			tx_skb = NULL;
		}
	}

	return ret;
}

static rs_ret c_tx_data_send(struct rs_c_if *c_if, u8 *tx_skb)
{
	rs_ret ret = RS_FAIL;
	struct rs_c_tx_data *tx_data = NULL;
	s32 len = 0;

	if (tx_skb) {
		(void)rs_net_skb_get_data(tx_skb, (u8 **)&tx_data);
		if (tx_data) {
			if (tx_data->ext_len == RS_C_TX_EXT_LEN && tx_data->data_len <= RS_C_DATA_SIZE &&
			    tx_data->data_len) {
				len = RS_C_GET_DATA_SIZE(tx_data->ext_len, tx_data->data_len);
				ret = rs_c_if_write(c_if, RS_C_IF_WRITE_CMD, (u8 *)tx_data, len);
				if (ret == RS_SUCCESS) {
					rs_c_dbg_stat.tx.nb_sent++;
				} else {
					rs_c_dbg_stat.tx.nb_if_err++;
				}
			} else {
				RS_DBG("P:%s[%d]:len[%d]:ext_len[%d]:data_len[%d]\n", __func__, __LINE__, len,
				       tx_data->ext_len, tx_data->data_len);
			}

			tx_data = NULL;
		} else {
			RS_DBG("P:%s[%d]:tx_data[%p]\n", __func__, __LINE__, tx_data);
		}
	}

	return ret;
}

static rs_ret c_tx_data(struct rs_c_if *c_if, u8 ac)
{
	rs_ret ret = RS_FAIL;
	u8 *tx_skb = NULL;
	u8 status = 0;
	s32 tx_avail_cnt = 0;
	s32 tx_cnt = 0;
	s8 vif_idx = -1;
	struct rs_q *temp_q = NULL;
	struct rs_c_q_buf *temp_buf = NULL;

	switch (ac) {
	case IF_DATA_AC:
		temp_q = &c_if->core->tx_data.buf_q;
		temp_buf = c_if->core->tx_data.buf;
		break;
	case IF_DATA_AC_POWER:
		temp_q = &c_if->core->tx_data.buf_power_q;
		temp_buf = c_if->core->tx_data.buf_power;
		break;
	default:
		RS_DBG("P:%s[%d]:fail!!:ac[%d]\n", __func__, __LINE__, ac);
		break;
	}

	tx_avail_cnt = rs_c_get_status_tx_avail_cnt(c_if, ac);

	if ((temp_q) && (temp_buf)) {
		while (
#ifdef C_TX_THREAD
			(rs_k_thread_is_running() == RS_SUCCESS) &&
#endif
			(rs_c_q_empty(temp_q) != RS_EMPTY) && (c_if->core->scan == 0) &&
			((status = rs_c_get_status_tx(c_if, ac)) == 0) && (tx_cnt < tx_avail_cnt)) {
			ret = c_tx_pop(c_if, ac, &vif_idx, &tx_skb);
			if (tx_skb) {
				if (rs_net_vif_idx_is_up(c_if, vif_idx) == RS_SUCCESS) {
					ret = c_tx_data_send(c_if, tx_skb);
				} else {
					RS_DBG("P:%s[%d]:skip!!:vif[%d]\n", __func__, __LINE__, vif_idx);
				}
				(void)rs_net_skb_free(tx_skb);
				tx_skb = NULL;
			}
			tx_cnt++;

			if ((tx_cnt > 1) && ((tx_cnt % C_TX_BUFFER_THRESHOLD) == 0)) {
				tx_avail_cnt = rs_c_get_status_tx_avail_cnt(c_if, ac);
				tx_cnt = 0;
			}
		}

		if (tx_skb) {
			RS_DBG("P:%s[%d]:tx_skb memory leak!!:[%d]\n", __func__, __LINE__, ret);
		}

		if (vif_idx >= 0) {
			// Start net_if
			(void)rs_net_if_tx_stop(c_if, vif_idx, FALSE);
		}
	}

	return ret;
}

#ifdef C_TX_THREAD
static s32 c_tx_thread(void *param)
{
	rs_ret ret = RS_SUCCESS;
	struct rs_c_if *c_if = param;
	rs_k_event_t ret_event = 0;

	if (c_if && c_if->core) {
		do {
			ret_event = rs_k_event_wait(c_if->core->tx_data.event, RS_C_TX_EVENT);

			if ((ret_event & RS_C_TX_AC_EVENT) != 0) {
				ret = c_tx_data(c_if, IF_DATA_AC);
			}
			if ((ret_event & RS_C_TX_POWER_EVENT) != 0) {
				ret = c_tx_data(c_if, IF_DATA_AC_POWER);
			}

			(void)rs_k_event_reset(c_if->core->tx_data.event);

			(void)rs_c_tx_event_post(c_if, RS_IF_DATA_MAX, 0, NULL);

		} while ((rs_k_thread_is_running() == RS_SUCCESS) && (ret_event != K_EVENT_EXIT));

		(void)rs_k_event_destroy(c_if->core->tx_data.event);
		if (c_if->core->tx_data.event) {
			rs_k_free(c_if->core->tx_data.event);
		}
		c_if->core->tx_data.event = NULL;

		(void)rs_k_thead_destroy_wait(&c_if->core->tx_data.thread);
	}

	return 0;
}
#else
static void c_tx_power_work_handler(void *param)
{
	rs_ret ret = RS_SUCCESS;
	struct rs_c_if *c_if = param;

	if (c_if && c_if->core) {
		ret = c_tx_data(c_if, IF_DATA_AC_POWER);

		(void)rs_c_tx_event_post(c_if, RS_IF_DATA_MAX, 0, NULL);
	}
}

static void c_tx_work_handler(void *param)
{
	rs_ret ret = RS_SUCCESS;
	struct rs_c_if *c_if = param;

	if (c_if && c_if->core) {
		ret = c_tx_data(c_if, IF_DATA_AC);

		(void)rs_c_tx_event_post(c_if, RS_IF_DATA_MAX, 0, NULL);
	}
}
#endif

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL FUNCTION

rs_ret rs_c_tx_init(struct rs_c_if *c_if, u16 tx_buf_num, u16 tx_buf_power_num)
{
	rs_ret ret = RS_FAIL;

	RS_TRACE(RS_FN_ENTRY_STR);

	if (c_if && c_if->core && tx_buf_num > 0) {
		C_TX_SPIN_LOCK_INIT(c_if);

		c_if->core->tx_data.buf =
			(struct rs_c_q_buf *)rs_k_calloc(tx_buf_num * sizeof(struct rs_c_q_buf));
		if (c_if->core->tx_data.buf) {
			c_if->core->tx_data.buf_power = (struct rs_c_q_buf *)rs_k_calloc(
				tx_buf_power_num * sizeof(struct rs_c_q_buf));
		}

		if (c_if->core->tx_data.buf && c_if->core->tx_data.buf_power) {
			c_if->core->tx_data.buf_num = tx_buf_num;
			ret = rs_c_q_init(&c_if->core->tx_data.buf_q, tx_buf_num);

			c_if->core->tx_data.buf_power_num = tx_buf_power_num;
			ret = rs_c_q_init(&c_if->core->tx_data.buf_power_q, tx_buf_power_num);

#ifdef C_TX_THREAD
			c_if->core->tx_data.event = rs_k_calloc(sizeof(struct rs_k_event));
			if (c_if->core->tx_data.event) {
				ret = rs_k_event_create(c_if->core->tx_data.event);
			} else {
				ret = RS_MEMORY_FAIL;
			}
			if (ret == RS_SUCCESS) {
				ret = rs_k_thead_create(&c_if->core->tx_data.thread, c_tx_thread, c_if,
							(u8 *)C_TX_THREAD_NAME);
			}
#else
			ret = rs_k_workqueue_init_work(&(c_if->core->tx_data.power_work),
						       c_tx_power_work_handler, c_if);
			ret = rs_k_workqueue_init_work(&(c_if->core->tx_data.work), c_tx_work_handler, c_if);
#endif
		} else {
			c_if->core->tx_data.buf_num = 0;
			ret = RS_MEMORY_FAIL;
		}
	}

	return ret;
}

rs_ret rs_c_tx_deinit(struct rs_c_if *c_if)
{
	rs_ret ret = RS_FAIL;

	RS_TRACE(RS_FN_ENTRY_STR);

	if (c_if && c_if->core) {
#ifdef C_TX_THREAD
		(void)rs_k_event_post(c_if->core->tx_data.event, K_EVENT_EXIT);
		ret = rs_k_thread_destroy(&c_if->core->tx_data.thread);
#else
		(void)rs_k_workqueue_free_work(&(c_if->core->tx_data.power_work));
		(void)rs_k_workqueue_free_work(&(c_if->core->tx_data.work));
#endif

		// free Q
		ret = c_tx_q_free(c_if);

		// free buf
		if (c_if->core->tx_data.buf) {
			rs_k_free(c_if->core->tx_data.buf);
			c_if->core->tx_data.buf = NULL;
			c_if->core->tx_data.buf_num = 0;
		}

		if (c_if->core->tx_data.buf_power) {
			rs_k_free(c_if->core->tx_data.buf_power);
			c_if->core->tx_data.buf_power = NULL;
			c_if->core->tx_data.buf_power_num = 0;
		}

		C_TX_SPIN_LOCK_DEINIT(c_if);
	}

	return ret;
}

// Post tx_data event
rs_ret rs_c_tx_event_post(struct rs_c_if *c_if, u8 ac, s8 vif_idx, u8 *tx_skb)
{
	rs_ret ret = RS_FAIL;
	rs_k_event_t event = 0;

	if (c_if && c_if->core) {
		if ((tx_skb != NULL) && (ac < RS_IF_DATA_MAX)) {
			ret = c_tx_push(c_if, ac, vif_idx, tx_skb);
		}

		if (rs_c_q_empty(&(c_if->core->tx_data.buf_q)) != RS_EMPTY) {
			event |= RS_C_TX_AC_EVENT;
		}

		if (rs_c_q_empty(&(c_if->core->tx_data.buf_power_q)) != RS_EMPTY) {
			event |= RS_C_TX_POWER_EVENT;
		}

#ifdef C_TX_THREAD
		if (c_if->core->tx_data.event) {
			if ((event & RS_C_TX_EVENT) != 0) {
				(void)rs_k_event_post(c_if->core->tx_data.event, event);
			}
		}
#else
		if (event & RS_C_TX_POWER_EVENT) {
			ret = rs_k_workqueue_add_work(c_if->core->wq, &(c_if->core->tx_data.power_work));
		}
		if (event & RS_C_TX_AC_EVENT) {
			ret = rs_k_workqueue_add_work(c_if->core->wq, &(c_if->core->tx_data.work));
		}
#endif
	}

	return ret;
}
