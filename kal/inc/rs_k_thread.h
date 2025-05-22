// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Copyright (C) [2022-2025] Renesas Electronics Corporation and/or its
 * affiliates.
 */

#ifndef RS_K_THREAD_H
#define RS_K_THREAD_H

////////////////////////////////////////////////////////////////////////////////
/// INCLUDE

#include "rs_type.h"

////////////////////////////////////////////////////////////////////////////////
/// MACRO DEFINITION

////////////////////////////////////////////////////////////////////////////////
/// TYPE DEFINITION

// Thread Status
enum rs_k_thread_status
{
	RS_K_THREAD_DESTROY_WAIT = 1,
	RS_K_THREAD_NONE = 2,
	RS_K_THREAD_RUN = 3,
};

struct rs_k_thread {
	void *thread;
	enum rs_k_thread_status status;
};

typedef s32(rs_k_thread_cb)(void *);

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL VARIABLE

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL FUNCTION

// Create Thread
rs_ret rs_k_thead_create(struct rs_k_thread *k_thread, rs_k_thread_cb *cb_func, void *arg, u8 *name);

// Is running?
rs_ret rs_k_thread_is_running(void);

// Destroy Thread
rs_ret rs_k_thread_destroy(struct rs_k_thread *k_thread);

// Wait for Thread to destroy
rs_ret rs_k_thead_destroy_wait(struct rs_k_thread *k_thread);

// Detach Thread
rs_ret rs_k_thread_detach(struct rs_k_thread *k_thread);

#endif /* RS_K_THREAD_H */
