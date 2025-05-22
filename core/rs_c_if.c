// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Copyright (C) [2022-2025] Renesas Electronics Corporation and/or its
 * affiliates.
 */

////////////////////////////////////////////////////////////////////////////////
/// INCLUDE

#include "rs_type.h"
#include "rs_k_mem.h"
#include "rs_core.h"
#include "rs_c_dbg.h"
#include "rs_c_if.h"

#include "rs_k_if.h"

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

rs_ret rs_c_if_init(rs_c_callback_t core_init_cb, rs_c_callback_t core_deinit_cb, rs_c_callback_t recv_cb)
{
	rs_ret ret = RS_FAIL;

	ret = rs_k_if_init(core_init_cb, core_deinit_cb, recv_cb);

	return ret;
}

rs_ret rs_c_if_deinit(struct rs_c_if *c_if)
{
	rs_ret ret = RS_SUCCESS;

	ret = rs_k_if_deinit(c_if);

	return ret;
}

rs_ret rs_c_if_read(struct rs_c_if *c_if, u32 addr, u8 *buf, u32 len)
{
	rs_ret ret = RS_FAIL;

	if (c_if && c_if->if_ops.read && c_if->core->recovery.in_recovery == FALSE) {
		ret = c_if->if_ops.read(c_if, addr, buf, len);
	}

	return ret;
}

rs_ret rs_c_if_write(struct rs_c_if *c_if, u32 addr, u8 *buf, u32 len)
{
	rs_ret ret = RS_FAIL;

	if (c_if && c_if->if_ops.write && c_if->core->recovery.in_recovery == FALSE) {
		ret = c_if->if_ops.write(c_if, addr, buf, len);
	}

	return ret;
}

rs_ret rs_c_if_read_status(struct rs_c_if *c_if, u8 *buf, u32 len)
{
	rs_ret ret = RS_FAIL;

	if (c_if && c_if->if_ops.read_status && c_if->core->recovery.in_recovery == FALSE) {
		ret = c_if->if_ops.read_status(c_if, buf, len);
	}

	return ret;
}

rs_ret rs_c_if_reload(struct rs_c_if *c_if)
{
	rs_ret ret = RS_FAIL;

	if (c_if && c_if->if_ops.reload) {
		ret = c_if->if_ops.reload(c_if);
	}

	return ret;
}

struct rs_core *rs_c_if_get_core(struct rs_c_if *c_if)
{
	struct rs_core *core = NULL;

	if (c_if && c_if->core) {
		core = (c_if->core);
	}

	return core;
}

rs_ret rs_c_if_set_core(struct rs_c_if *c_if, struct rs_core *core)
{
	rs_ret ret = RS_FAIL;

	if (c_if) {
		c_if->core = core;
		ret = RS_SUCCESS;
	}

	return ret;
}

void *rs_c_if_get_dev(struct rs_c_if *c_if)
{
	void *dev = NULL;

	if (c_if) {
		dev = c_if->if_dev.dev;
	}

	return dev;
}

rs_ret rs_c_if_set_dev(struct rs_c_if *c_if, void *dev)
{
	rs_ret ret = RS_FAIL;

	if (c_if) {
		(c_if->if_dev).dev = dev;
		ret = RS_SUCCESS;
	}

	return ret;
}

void *rs_c_if_get_dev_if(struct rs_c_if *c_if)
{
	void *dev = NULL;

	if (c_if) {
		dev = ((c_if->if_dev).dev_if);
	}

	return dev;
}

rs_ret rs_c_if_set_dev_if(struct rs_c_if *c_if, void *dev_if)
{
	rs_ret ret = RS_FAIL;

	if (c_if) {
		(c_if->if_dev).dev_if = dev_if;
		ret = RS_SUCCESS;
	}

	return ret;
}

void *rs_c_if_get_net_priv(struct rs_c_if *c_if)
{
	void *net_priv = NULL;

	if (c_if && c_if->core) {
		net_priv = ((c_if->core)->net_priv);
	}

	return net_priv;
}

rs_ret rs_c_if_set_net_priv(struct rs_c_if *c_if, void *net_priv)
{
	rs_ret ret = RS_FAIL;

	RS_DBG("P:%s[%d]:c_if[0x%p]:core[0x%p]\n", __func__, __LINE__, c_if, c_if->core);

	if (c_if && c_if->core) {
		c_if->core->net_priv = net_priv;
		ret = RS_SUCCESS;
	}

	return ret;
}
