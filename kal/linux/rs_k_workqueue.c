// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Copyright (C) [2022-2025] Renesas Electronics Corporation and/or its
 * affiliates.
 */

////////////////////////////////////////////////////////////////////////////////
/// INCLUDE

#include <linux/kthread.h>

#include "rs_type.h"
#include "rs_k_mem.h"
#include "rs_c_dbg.h"

#include "rs_k_workqueue.h"

////////////////////////////////////////////////////////////////////////////////
/// MACRO DEFINITION

////////////////////////////////////////////////////////////////////////////////
/// TYPE DEFINITION

struct k_work {
	struct work_struct work;

	rs_k_workqueue_cb *cb_func;
	void *arg;
};

////////////////////////////////////////////////////////////////////////////////
/// LOCAL VARIABLE

////////////////////////////////////////////////////////////////////////////////
/// LOCAL FUNCTION

void k_workqueue_handler(struct work_struct *work)
{
	struct k_work *temp_work = NULL;

	//RS_INFO("P:%s[%d]\n", __func__, __LINE__);

	if (work != NULL) {
		temp_work = container_of(work, struct k_work, work);

		if ((temp_work != NULL) && (temp_work->cb_func != NULL)) {
			(void)(temp_work->cb_func)(temp_work->arg);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL FUNCTION

// Create Workqueue
rs_ret rs_k_workqueue_create(struct rs_k_workqueue *k_workqueue)
{
	rs_ret ret = RS_FAIL;

	//RS_INFO("P:%s[%d]\n", __func__, __LINE__);

	if ((k_workqueue != NULL) && (k_workqueue->wq == NULL)) {
		k_workqueue->wq =
			alloc_workqueue("rswlan_wq", WQ_UNBOUND | WQ_HIGHPRI, WQ_UNBOUND_MAX_ACTIVE);
		if (k_workqueue->wq) {
			ret = RS_SUCCESS;
		} else {
			ret = RS_MEMORY_FAIL;
		}
	}

	//RS_INFO("P:%s[%d]:r[%d]\n", __func__, __LINE__, ret);

	return ret;
}

// Destroy Workqueue
rs_ret rs_k_workqueue_destroy(struct rs_k_workqueue *k_workqueue)
{
	rs_ret ret = RS_FAIL;

	if ((k_workqueue != NULL) && (k_workqueue->wq != NULL)) {
		// flush_work(&(temp_work->work));

		flush_workqueue(k_workqueue->wq);
		destroy_workqueue(k_workqueue->wq);

		k_workqueue->wq = NULL;

		ret = RS_SUCCESS;
	}

	//RS_INFO("P:%s[%d]:r[%d]\n", __func__, __LINE__, ret);

	return ret;
}

rs_ret rs_k_workqueue_init_work(struct rs_k_work *k_work, rs_k_workqueue_cb *cb_func, void *arg)
{
	rs_ret ret = RS_FAIL;
	struct k_work *temp_work = NULL;

	if ((k_work != NULL) && (cb_func != NULL)) {
		temp_work = rs_k_calloc(sizeof(struct k_work));
		if (temp_work) {
			temp_work->cb_func = cb_func;
			temp_work->arg = arg;

			INIT_WORK(&(temp_work->work), k_workqueue_handler);

			k_work->work = temp_work;

			ret = RS_SUCCESS;
		} else {
			ret = RS_MEMORY_FAIL;
		}
	}

	//RS_INFO("P:%s[%d]:r[%d]\n", __func__, __LINE__, ret);

	return ret;
}

rs_ret rs_k_workqueue_add_work(struct rs_k_workqueue *k_workqueue, struct rs_k_work *k_work)
{
	rs_ret ret = RS_FAIL;
	struct k_work *temp_work = NULL;

	if ((k_workqueue != NULL) && ((k_workqueue->wq != NULL)) && (k_work != NULL) &&
	    (k_work->work != NULL)) {
		temp_work = k_work->work;

		queue_work(k_workqueue->wq, &(temp_work->work));

		ret = RS_SUCCESS;
	}

	//RS_INFO("P:%s[%d]:r[%d]\n", __func__, __LINE__, ret);

	return ret;
}

rs_ret rs_k_workqueue_free_work(struct rs_k_work *k_work)
{
	rs_ret ret = RS_FAIL;

	if ((k_work != NULL) && (k_work->work != NULL)) {
		rs_k_free(k_work->work);
		k_work->work = NULL;

		ret = RS_SUCCESS;
	}

	//RS_INFO("P:%s[%d]:r[%d]\n", __func__, __LINE__, ret);

	return ret;
}
