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
#include "rs_c_recovery.h"

#include "rs_c_ctrl.h"

////////////////////////////////////////////////////////////////////////////////
/// MACRO DEFINITION

#define C_CTRL_MUTEX_INIT(c_if)	     (void)rs_k_mutex_create(&c_if->core->ctrl.mutex)
#define C_CTRL_MUTEX_DEINIT(c_if)    (void)rs_k_mutex_destroy(&c_if->core->ctrl.mutex)
#define C_CTRL_MUTEX_LOCK(c_if)	     (void)rs_k_mutex_lock(&c_if->core->ctrl.mutex)
#define C_CTRL_MUTEX_UNLOCK(c_if)    (void)rs_k_mutex_unlock(&c_if->core->ctrl.mutex)

#define CTRL_EVENT_WAIT_DEFAULT_TIME (5 * 1000 * 1000) // 2000ms

////////////////////////////////////////////////////////////////////////////////
/// TYPE DEFINITION

////////////////////////////////////////////////////////////////////////////////
/// LOCAL VARIABLE

////////////////////////////////////////////////////////////////////////////////
/// LOCAL FUNCTION

// Set control command and wait response
static rs_ret c_ctrl_set(struct rs_c_if *c_if, u8 cmd_id, u16 req_data_len, u8 *req_data)
{
	rs_ret ret = RS_FAIL;
	struct rs_c_ctrl_req *ctrl_req_data = NULL;

	if ((c_if) && (c_if->core)) {
		ret = rs_k_event_reset(c_if->core->ctrl.event);

		if (ret == RS_SUCCESS) {
			ctrl_req_data = rs_k_calloc(sizeof(struct rs_c_ctrl_req));

			if (ctrl_req_data) {
				ctrl_req_data->cmd = cmd_id;
				ctrl_req_data->reserved = 0;
				if ((req_data) &&
				    ((req_data_len > 0) && (req_data_len <= RS_C_CTRL_DATA_LEN))) {
					ctrl_req_data->data_len = req_data_len;
					(void)rs_k_memcpy(ctrl_req_data->data, req_data,
							  ctrl_req_data->data_len);
				}

				// TX Control to FW
				ret = rs_c_if_write(c_if, RS_C_IF_WRITE_CMD, (u8 *)ctrl_req_data,
						    RS_C_GET_DATA_SIZE(RS_C_CTRL_REQ_EXT_LEN,
								       ctrl_req_data->data_len));
			}

			if (ctrl_req_data) {
				rs_k_free(ctrl_req_data);
			}
		}
	}

	return ret;
}

static rs_ret c_ctrl_wait(struct rs_c_if *c_if, u8 cmd_id)
{
	rs_ret ret = RS_FAIL;
	rs_k_event_t ret_event = 0;

	if (c_if && c_if->core) {
		c_if->core->ctrl.buf.cmd = RS_CMD_DATA_START; // clear

		ret_event =
			rs_k_event_timed_wait(c_if->core->ctrl.event, cmd_id, CTRL_EVENT_WAIT_DEFAULT_TIME);
		if (ret_event == cmd_id) {
			ret = RS_SUCCESS;
		}
		rs_k_event_reset(c_if->core->ctrl.event);
	}

	return ret;
}

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL FUNCTION

// Initialize control core
rs_ret rs_c_ctrl_init(struct rs_c_if *c_if)
{
	rs_ret ret = RS_FAIL;

	RS_TRACE(RS_FN_ENTRY_STR);

	if (c_if && c_if->core) {
		C_CTRL_MUTEX_INIT(c_if);

		c_if->core->ctrl.event = rs_k_calloc(sizeof(struct rs_k_event));
		if (c_if->core->ctrl.event) {
			ret = rs_k_event_create(c_if->core->ctrl.event);
		} else {
			ret = RS_MEMORY_FAIL;
		}
	}

	return ret;
}

// Deinitialize control core
rs_ret rs_c_ctrl_deinit(struct rs_c_if *c_if)
{
	rs_ret ret = RS_FAIL;

	RS_TRACE(RS_FN_ENTRY_STR);

	if (c_if && c_if->core) {
		ret = rs_k_event_destroy(c_if->core->ctrl.event);
		if (c_if->core->ctrl.event) {
			rs_k_free(c_if->core->ctrl.event);
		}
		c_if->core->ctrl.event = NULL;

		C_CTRL_MUTEX_DEINIT(c_if);
	}

	return ret;
}

// Set control command and no wait
rs_ret rs_c_ctrl_set(struct rs_c_if *c_if, u8 cmd_id, u16 req_data_len, u8 *req_data)
{
	rs_ret ret = RS_FAIL;

	if ((c_if) && (c_if->core)) {
		C_CTRL_MUTEX_LOCK(c_if);

		ret = c_ctrl_set(c_if, cmd_id, req_data_len, req_data);

		C_CTRL_MUTEX_UNLOCK(c_if);
	}

	return ret;
}

// Set control command and wait response
rs_ret rs_c_ctrl_set_and_wait(struct rs_c_if *c_if, u8 cmd_id, u16 req_data_len, u8 *req_data,
			      struct rs_c_ctrl_rsp *ctrl_rsp_data)
{
	rs_ret ret = RS_FAIL;

	if (c_if && c_if->core && c_if->core->recovery.in_recovery == FALSE) {
		C_CTRL_MUTEX_LOCK(c_if);

		ret = c_ctrl_set(c_if, cmd_id, req_data_len, req_data);

		if (ret == RS_SUCCESS) {
			// wait response
			ret = c_ctrl_wait(c_if, cmd_id);
		}

		RS_DBG("P:%s[%d]:r[%d]:cmd[%d %d]:len[%d]:ctrl_rsp_data[%p]\n", __func__, __LINE__, ret,
		       cmd_id, c_if->core->ctrl.buf.cmd, c_if->core->ctrl.buf.data_len, ctrl_rsp_data);

		if (ret != RS_SUCCESS) {
			RS_ERR("Timeout waiting for rsp %u r[%d]\n", cmd_id, ret);

			// try to recieve message again before recovery
			ret = c_ctrl_wait(c_if, cmd_id);
			if (ret == RS_SUCCESS)
				RS_ERR("managed to recover from timeout\n");
			else {
				RS_ERR("Failed to recover from timeout\n");
				C_CTRL_MUTEX_UNLOCK(c_if);
				rs_c_recovery_event_post(c_if);
			}
		}

		if (ret == RS_SUCCESS) {
			if (ctrl_rsp_data) {
				ctrl_rsp_data->cmd = c_if->core->ctrl.buf.cmd;
				ctrl_rsp_data->data_len = c_if->core->ctrl.buf.data_len;
				if (ctrl_rsp_data->data_len > RS_C_CTRL_DATA_LEN) {
					ctrl_rsp_data->data_len = RS_C_CTRL_DATA_LEN;
				}
				(void)rs_k_memcpy(ctrl_rsp_data->data, c_if->core->ctrl.buf.data,
						  ctrl_rsp_data->data_len);
			}
		}

		C_CTRL_MUTEX_UNLOCK(c_if);
	}

	return ret;
}

// Post control response event
rs_ret rs_c_ctrl_event_post(struct rs_c_if *c_if, struct rs_c_ctrl_rsp *ctrl_rsp_data)
{
	rs_ret ret = RS_FAIL;

	if (c_if && c_if->core && c_if->core->ctrl.event && ctrl_rsp_data) {
		// update data status
		if (ctrl_rsp_data->ext_len == RS_C_CTRL_RSP_EXT_LEN) {
			rs_c_set_status(c_if, ctrl_rsp_data->ext_hdr.status);
		}

		(void)rs_k_memcpy(&c_if->core->ctrl.buf, ctrl_rsp_data, sizeof(struct rs_c_ctrl_rsp));

		// Post Control event
		ret = rs_k_event_post(c_if->core->ctrl.event, ctrl_rsp_data->cmd);
	}

	return ret;
}
