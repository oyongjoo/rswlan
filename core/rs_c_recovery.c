// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Copyright (C) [2022-2025] Renesas Electronics Corporation and/or its
 * affiliates.
 */

////////////////////////////////////////////////////////////////////////////////
/// INCLUDE
#include <linux/delay.h>
#include "rs_type.h"
#include "rs_core.h"
#include "rs_c_dbg.h"
#include "rs_c_recovery.h"
#include "rs_k_sdio.h"
#include "rs_c_status.h"
#include "rs_k_mem.h"
#include "rs_net_ctrl.h"

////////////////////////////////////////////////////////////////////////////////
/// MACRO DEFINITION

#define C_RECOVERY_THREAD_NAME "RSW_REC_THREAD"

////////////////////////////////////////////////////////////////////////////////
/// TYPE DEFINITION

////////////////////////////////////////////////////////////////////////////////
/// LOCAL VARIABLE

////////////////////////////////////////////////////////////////////////////////
/// LOCAL FUNCTION

rs_ret rs_c_recovery_process(struct rs_c_if *c_if)
{
	s32 ret = 0;
	RS_INFO("recovery start\n");
	// TODO - add hw reset function, Jira https://jira.global.renesas.com/browse/WIFISWTIN-2313

	ret = rs_c_if_reload(c_if);

	return ret;
}

#ifdef C_REC_THREAD
static s32 c_recovery_thread(void *param)
{
	struct rs_c_if *c_if = param;
	rs_k_event_t ret_event = 0;

	if (c_if && c_if->core) {
		do {
			ret_event = rs_k_event_wait(c_if->core->recovery.event, RS_C_RECOVERY_EVENT);
			if (ret_event == RS_C_RECOVERY_EVENT) {
				(void)rs_c_recovery_process(c_if);
			}

			(void)rs_k_event_reset(c_if->core->recovery.event);

		} while ((rs_k_thread_is_running() == RS_SUCCESS) && (ret_event != K_EVENT_EXIT));

		(void)rs_k_event_destroy(c_if->core->recovery.event);
		if (c_if->core->recovery.event) {
			rs_k_free(c_if->core->recovery.event);
		}
		c_if->core->recovery.event = NULL;

		(void)rs_k_thead_destroy_wait(&c_if->core->recovery.thread);
	}

	return 0;
}
#else

static void c_recovery_work_handler(void *param)
{
	struct rs_c_if *c_if = param;

	if (c_if && c_if->core) {
		(void)rs_c_recovery_process(c_if);
	}
}

#endif

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL FUNCTION

rs_ret rs_c_recovery_init(struct rs_c_if *c_if)
{
	rs_ret ret = RS_FAIL;

	RS_TRACE(RS_FN_ENTRY_STR);

	if (c_if && c_if->core) {
#ifdef C_REC_THREAD
		// RX
		c_if->core->recovery.event = rs_k_calloc(sizeof(struct rs_k_event));
		if (c_if->core->recovery.event) {
			ret = rs_k_event_create(c_if->core->recovery.event);
		} else {
			ret = RS_MEMORY_FAIL;
		}

		if (ret == RS_SUCCESS) {
			ret = rs_k_thead_create(&c_if->core->recovery.thread, c_recovery_thread, c_if,
						(u8 *)C_RECOVERY_THREAD_NAME);
		}
#else
		ret = rs_k_workqueue_init_work(&(c_if->core->recovery.work), c_recovery_work_handler, c_if);
#endif
	}

	return ret;
}

rs_ret rs_c_recovery_deinit(struct rs_c_if *c_if)
{
	rs_ret ret = RS_FAIL;

	RS_TRACE(RS_FN_ENTRY_STR);

	if (c_if && c_if->core) {
#ifdef C_REC_THREAD
		(void)rs_k_event_post(c_if->core->recovery.event, K_EVENT_EXIT);
		ret = rs_k_thread_destroy(&c_if->core->recovery.thread);
#else
		(void)rs_k_workqueue_free_work(&(c_if->core->recovery.work));
#endif
	}

	return ret;
}

// Post indication event
rs_ret rs_c_recovery_event_post(struct rs_c_if *c_if)
{
	rs_ret ret = RS_FAIL;
	if (c_if && c_if->core) {
		if (c_if->core->recovery.in_recovery == FALSE) {
#ifdef C_REC_THREAD
			rs_net_ctrl_dev_hw_reset(c_if);
			c_if->core->recovery.in_recovery = TRUE;
			(void)rs_k_event_post(c_if->core->recovery.event, RS_C_RECOVERY_EVENT);
#else
			ret = rs_k_workqueue_add_work(c_if->core->wq, &(c_if->core->recovery.work));
#endif
		}
	}

	return ret;
}