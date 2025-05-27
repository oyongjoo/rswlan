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
#include "rs_k_if.h"

#include "rs_c_status.h"
#include "rs_c_ctrl.h"
#include "rs_c_indi.h"
#include "rs_c_rx.h"
#include "rs_c_tx.h"
#include "rs_c_recovery.h"
#include "rs_net.h"

#include "rs_core.h"

////////////////////////////////////////////////////////////////////////////////
/// MACRO DEFINITION

#define C_STATUS_INIT(c_if)   (void)rs_k_mutex_create(&c_if->core->status.mutex)
#define C_STATUS_DEINIT(c_if) (void)rs_k_mutex_destroy(&c_if->core->status.mutex)

////////////////////////////////////////////////////////////////////////////////
/// TYPE DEFINITION

////////////////////////////////////////////////////////////////////////////////
/// LOCAL VARIABLE

struct rs_c_dbg_stat_t rs_c_dbg_stat = {
	0,
};

////////////////////////////////////////////////////////////////////////////////
/// LOCAL FUNCTION

static rs_ret core_init(struct rs_c_if *c_if)
{
	rs_ret ret;

	RS_TRACE(RS_FN_ENTRY_STR);

	c_if->core->wq = rs_k_calloc(sizeof(struct rs_k_workqueue));
	if (!c_if->core->wq) {
		RS_ERR("Failed to allocate workqueue");
		return RS_FAIL;
	}

	ret = rs_k_workqueue_create(c_if->core->wq);
	if (ret != RS_SUCCESS) {
		RS_ERR("Failed to create workqueue, ret=%d", ret);
		return ret;
	}

	ret = rs_c_ctrl_init(c_if);
	if (ret != RS_SUCCESS) {
		RS_ERR("Failed to initialize control module, ret=%d", ret);
		return ret;
	}

	ret = rs_c_recovery_init(c_if);
	if (ret != RS_SUCCESS) {
		RS_ERR("Failed to initialize recovery module, ret=%d", ret);
		return ret;
	}

	ret = rs_c_indi_init(c_if, RS_NET_INID_BUF_Q_MAX);
	if (ret != RS_SUCCESS) {
		RS_ERR("Failed to initialize indication module, ret=%d", ret);
		return ret;
	}

	ret = rs_c_rx_data_init(c_if, RS_NET_RX_BUF_Q_MAX);
	if (ret != RS_SUCCESS) {
		RS_ERR("Failed to initialize RX data module, ret=%d", ret);
		return ret;
	}

	ret = rs_c_rx_init(c_if);
	if (ret != RS_SUCCESS) {
		RS_ERR("Failed to initialize RX module, ret=%d", ret);
		return ret;
	}

	ret = rs_c_tx_init(c_if, RS_NET_TX_BUF_Q_MAX, RS_NET_TX_POWER_BUF_Q_MAX);
	if (ret != RS_SUCCESS) {
		RS_ERR("Failed to initialize TX module, ret=%d", ret);
		return ret;
	}

	ret = rs_c_status_init(c_if, RS_CORE_STATUS_SIZE);
	if (ret != RS_SUCCESS) {
		RS_ERR("Failed to initialize status module, ret=%d", ret);
		return ret;
	}

	ret = rs_net_init(c_if);
	if (ret != RS_SUCCESS) {
		RS_ERR("Failed to initialize network module, ret=%d", ret);
		return ret;
	}

	c_if->core->scan = 0;

	return ret;
}

static rs_ret core_deinit(struct rs_c_if *c_if)
{
	rs_ret ret = RS_FAIL;
	struct rs_k_workqueue *temp_wq = NULL;

	RS_TRACE(RS_FN_ENTRY_STR);

	(void)rs_net_deinit(c_if);

	if (c_if->core->wq) {
		temp_wq = c_if->core->wq;
		c_if->core->wq = NULL;

		(void)rs_k_workqueue_destroy(temp_wq);
		rs_k_free(temp_wq);
	}

	(void)rs_c_tx_deinit(c_if);

	(void)rs_c_rx_deinit(c_if);

	(void)rs_c_rx_data_deinit(c_if);

	(void)rs_c_indi_deinit(c_if);

	(void)rs_c_recovery_deinit(c_if);

	(void)rs_c_ctrl_deinit(c_if);

	(void)rs_c_status_deinit(c_if);

	return ret;
}

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL FUNCTION

rs_ret rs_core_init(struct rs_c_if *c_if)
{
	rs_ret ret = RS_FAIL;
	struct rs_core *core = NULL;

	RS_TRACE(RS_FN_ENTRY_STR);

	if (c_if) {
		core = rs_k_calloc(sizeof(struct rs_core));
		if (core) {
			core->c_if = c_if;
			(void)rs_c_if_set_core(c_if, core);

			ret = core_init(c_if);

		} else {
			ret = RS_MEMORY_FAIL;
		}
	}

	return ret;
}

rs_ret rs_core_deinit(struct rs_c_if *c_if)
{
	rs_ret ret = RS_SUCCESS;

	RS_TRACE(RS_FN_ENTRY_STR);

	if (c_if) {
		if (c_if->core) {
			ret = core_deinit(c_if);

			rs_k_free(c_if->core);
			(void)rs_c_if_set_core(c_if, NULL);
		}
	}

	return ret;
}
