// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Copyright (C) [2022-2025] Renesas Electronics Corporation and/or its
 * affiliates.
 */

#ifndef RS_K_WORKQUEUE_H
#define RS_K_WORKQUEUE_H

////////////////////////////////////////////////////////////////////////////////
/// INCLUDE

#include "rs_type.h"

////////////////////////////////////////////////////////////////////////////////
/// MACRO DEFINITION

////////////////////////////////////////////////////////////////////////////////
/// TYPE DEFINITION

struct rs_k_workqueue {
	void *wq;
};

struct rs_k_work {
	void *work;
};

typedef void(rs_k_workqueue_cb)(void *);

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL VARIABLE

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL FUNCTION

// Create Workqueue
rs_ret rs_k_workqueue_create(struct rs_k_workqueue *k_workqueue);

// Destroy Workqueue
rs_ret rs_k_workqueue_destroy(struct rs_k_workqueue *k_workqueue);

// Initiailze work struct
rs_ret rs_k_workqueue_init_work(struct rs_k_work *k_work, rs_k_workqueue_cb *cb_func, void *arg);

// Add Work to Workqueue
rs_ret rs_k_workqueue_add_work(struct rs_k_workqueue *k_workqueue, struct rs_k_work *k_work);

// Remove work struct
rs_ret rs_k_workqueue_free_work(struct rs_k_work *k_work);

#endif /* RS_K_WORKQUEUE_H */
