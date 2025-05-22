// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Copyright (C) [2022-2025] Renesas Electronics Corporation and/or its
 * affiliates.
 */

////////////////////////////////////////////////////////////////////////////////
/// INCLUDE

#include <linux/version.h>
#include <linux/kthread.h>
#include <linux/completion.h>

#include "rs_type.h"
#include "rs_k_mem.h"

#include "rs_k_thread.h"

////////////////////////////////////////////////////////////////////////////////
/// MACRO DEFINITION

////////////////////////////////////////////////////////////////////////////////
/// TYPE DEFINITION

typedef struct {
	struct completion completion;
	struct task_struct *task;
} k_thread_t;

////////////////////////////////////////////////////////////////////////////////
/// LOCAL VARIABLE

////////////////////////////////////////////////////////////////////////////////
/// LOCAL FUNCTION

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL FUNCTION

rs_ret rs_k_thead_create(struct rs_k_thread *k_thread, rs_k_thread_cb *cb_func, void *arg, u8 *name)
{
	rs_ret ret = RS_FAIL;
	k_thread_t *temp_thread = NULL;

	if (k_thread && cb_func) {
		temp_thread = rs_k_calloc(sizeof(k_thread_t));
		if (temp_thread) {
			temp_thread->task = kthread_run(cb_func, arg, name);
			if (temp_thread->task) {
				init_completion(&temp_thread->completion);

				k_thread->thread = temp_thread;
				k_thread->status = RS_K_THREAD_NONE;

				ret = RS_SUCCESS;
			}
		} else {
			ret = RS_MEMORY_FAIL;
		}
	}

	return ret;
}

rs_ret rs_k_thread_is_running(void)
{
	rs_ret ret = RS_SUCCESS;

	if (kthread_should_stop() == true) {
		ret = RS_FAIL;
	}

	return ret;
}

rs_ret rs_k_thread_destroy(struct rs_k_thread *k_thread)
{
	rs_ret ret = RS_FAIL;
	k_thread_t *temp_thread = NULL;

	if (k_thread && k_thread->thread) {
		temp_thread = k_thread->thread;
		if (temp_thread->task) {
			// thread stop and wait completion(rs_k_thead_destroy_wait)
			kthread_stop(temp_thread->task);
		}

		temp_thread->task = NULL;
		if (temp_thread) {
			rs_k_free(temp_thread);
			k_thread->thread = NULL;
		}

		ret = RS_SUCCESS;
	}

	return ret;
}

rs_ret rs_k_thead_destroy_wait(struct rs_k_thread *k_thread)
{
	rs_ret ret = RS_FAIL;
	k_thread_t *temp_thread = NULL;

	if (k_thread && k_thread->thread) {
		temp_thread = k_thread->thread;
		if (temp_thread) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 17, 0)
			kthread_complete_and_exit(&temp_thread->completion, 0);
#else
			complete_and_exit(&temp_thread->completion, 0);
#endif
		}

		ret = RS_SUCCESS;
	}

	return ret;
}

rs_ret rs_k_thread_detach(struct rs_k_thread *k_thread)
{
	rs_ret ret = RS_FAIL;
	k_thread_t *temp_thread = NULL;

	if (k_thread && k_thread->thread) {
		temp_thread = k_thread->thread;
		if (temp_thread->task) {
			ret = RS_SUCCESS;
		}
	}

	return ret;
}
