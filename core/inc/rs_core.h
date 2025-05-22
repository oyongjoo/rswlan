// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Copyright (C) [2022-2025] Renesas Electronics Corporation and/or its
 * affiliates.
 */

#ifndef RS_CORE_H
#define RS_CORE_H

////////////////////////////////////////////////////////////////////////////////
/// INCLUDE

#include "rs_type.h"
#include "rs_k_event.h"
#include "rs_k_workqueue.h"
#include "rs_k_mutex.h"
#include "rs_k_spin_lock.h"
#include "rs_k_thread.h"
#include "rs_c_q.h"
#include "rs_c_if.h"
#include "rs_c_data.h"
#include "rs_c_indi.h"

////////////////////////////////////////////////////////////////////////////////
/// MACRO DEFINITION

#define RS_CORE_STATUS_SIZE (4)

#define C_RX_THREAD
#define C_TX_THREAD
#define C_REC_THREAD

////////////////////////////////////////////////////////////////////////////////
/// TYPE DEFINITION

struct rs_c_q_buf {
	s8 vif_idx;
	u8 *data;
};

struct rs_core {
	struct rs_c_if *c_if;

	void *net_priv;

	struct rs_k_workqueue *wq;

	u8 scan;

	struct {
		u8 *value;
		struct rs_k_mutex mutex;
	} status;

	struct {
		struct rs_k_event *event;
		struct rs_k_mutex mutex;

		struct rs_c_ctrl_rsp buf;
	} ctrl;

	struct {
#ifdef C_RX_THREAD
		struct rs_k_event *event;
		struct rs_k_thread thread;
#else
		struct rs_k_work work;
#endif
		struct rs_k_mutex mutex;

		struct rs_q buf_q;
		struct rs_c_indi **buf;
		u16 buf_num;
	} indi;

	struct {
#ifdef C_RX_THREAD
		struct rs_k_event *event;
		struct rs_k_thread thread;
#else
		struct rs_k_work work;
#endif
	} rx;

	struct {
#ifdef C_RX_THREAD
		struct rs_k_event *event;
		struct rs_k_thread thread;
#else
		struct rs_k_work work;
#endif
		struct rs_k_spin_lock spin_lock;

		struct rs_q buf_q;
		struct rs_c_rx_data **buf;
		u16 buf_num;
	} rx_data;

	struct {
#ifdef C_TX_THREAD
		struct rs_k_event *event;
		struct rs_k_thread thread;
#else
		struct rs_k_work power_work;
		struct rs_k_work work;
#endif

		struct rs_k_spin_lock spin_lock;

		struct rs_q buf_q;
		struct rs_c_q_buf *buf;
		u16 buf_num;

		struct rs_q buf_power_q;
		struct rs_c_q_buf *buf_power;
		u16 buf_power_num;
	} tx_data;

	struct {
#ifdef C_REC_THREAD
		struct rs_k_event *event;
		struct rs_k_thread thread;
#else
		struct rs_k_work work;
#endif
		u8 in_recovery;
	} recovery;
};

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL VARIABLE

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL FUNCTION

rs_ret rs_core_init(struct rs_c_if *c_if);
rs_ret rs_core_deinit(struct rs_c_if *c_if);

#endif /* RS_CORE_H */
