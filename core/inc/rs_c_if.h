// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Copyright (C) [2022-2025] Renesas Electronics Corporation and/or its
 * affiliates.
 */

#ifndef RS_C_IF_H
#define RS_C_IF_H

////////////////////////////////////////////////////////////////////////////////
/// INCLUDE

#include "rs_type.h"

////////////////////////////////////////////////////////////////////////////////
/// MACRO DEFINITION

// default fullmac firmware name
#define RS_FMAC_FW_BASE_NAME "fmacfw"

#define RS_C_IF_READ_CMD     (0x0001)
#define RS_C_IF_WRITE_CMD    (0x0002)
#define RS_C_IF_STATUS_CMD   (0x0010)

////////////////////////////////////////////////////////////////////////////////
/// TYPE DEFINITION

struct rs_c_if;
typedef rs_ret(rs_c_callback_t)(struct rs_c_if *c_if);

struct rs_c_if_ops {
	rs_ret (*read)(struct rs_c_if *c_if, u32 addr, u8 *data, u32 len);
	rs_ret (*write)(struct rs_c_if *c_if, u32 addr, u8 *data, u32 len);
	rs_ret (*read_status)(struct rs_c_if *c_if, u8 *data, u32 len);
	rs_ret (*reload)(struct rs_c_if *c_if);
};

struct rs_c_if_dev {
	u16 vendor;
	u16 device;
	void *dev;
	void *dev_if;
	void *dev_if_priv;
};

struct rs_c_if_cb {
	rs_c_callback_t *core_init_cb;
	rs_c_callback_t *core_deinit_cb;
	rs_c_callback_t *recv_cb;
};

struct rs_c_if {
	struct rs_core *core;

	struct rs_c_if_cb *if_cb;

	struct rs_c_if_dev if_dev;
	struct rs_c_if_ops if_ops;
};

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL VARIABLE

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL FUNCTION

// Initialize Core Interface
rs_ret rs_c_if_init(rs_c_callback_t core_init_cb, rs_c_callback_t core_deinit_cb, rs_c_callback_t recv_cb);

// Deitialize Core Interface
rs_ret rs_c_if_deinit(struct rs_c_if *c_if);

// Read from I/F
rs_ret rs_c_if_read(struct rs_c_if *c_if, u32 addr, u8 *buf, u32 len);

// Write to I/F
rs_ret rs_c_if_write(struct rs_c_if *c_if, u32 addr, u8 *buf, u32 len);

// Read Status from I/F
rs_ret rs_c_if_read_status(struct rs_c_if *c_if, u8 *buf, u32 len);

// Reload I/F device
rs_ret rs_c_if_reload(struct rs_c_if *c_if);

// Get Driver Core
struct rs_core *rs_c_if_get_core(struct rs_c_if *c_if);

// Set Driver Core
rs_ret rs_c_if_set_core(struct rs_c_if *c_if, struct rs_core *core);

// Get I/F device
void *rs_c_if_get_dev(struct rs_c_if *c_if);

// Set I/F device
rs_ret rs_c_if_set_dev(struct rs_c_if *c_if, void *dev);

// Get system device of I/F device
void *rs_c_if_get_dev_if(struct rs_c_if *c_if);

// Set system device of I/F device
rs_ret rs_c_if_set_dev_if(struct rs_c_if *c_if, void *dev_if);

// Get Network Private
void *rs_c_if_get_net_priv(struct rs_c_if *c_if);

// Set Network Private
rs_ret rs_c_if_set_net_priv(struct rs_c_if *c_if, void *net_priv);

#endif /* RS_C_IF_H */
