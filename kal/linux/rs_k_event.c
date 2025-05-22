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

#include "rs_k_event.h"

////////////////////////////////////////////////////////////////////////////////
/// MACRO DEFINITION

////////////////////////////////////////////////////////////////////////////////
/// TYPE DEFINITION

struct k_event {
	atomic_t condition;
	wait_queue_head_t wait_queue;
};

////////////////////////////////////////////////////////////////////////////////
/// LOCAL VARIABLE

////////////////////////////////////////////////////////////////////////////////
/// LOCAL FUNCTION

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL FUNCTION

// Create event
rs_ret rs_k_event_create(struct rs_k_event *k_event)
{
	rs_ret ret = RS_FAIL;
	struct k_event *temp_event = NULL;

	if (k_event) {
		temp_event = rs_k_calloc(sizeof(struct k_event));
		if (temp_event) {
			atomic_set(&temp_event->condition, K_EVENT_INIT);
			init_waitqueue_head(&temp_event->wait_queue);

			k_event->event = temp_event;

			ret = RS_SUCCESS;
		} else {
			ret = RS_MEMORY_FAIL;
		}
	}

	return ret;
}

// Destroy Event
rs_ret rs_k_event_destroy(struct rs_k_event *k_event)
{
	rs_ret ret = RS_FAIL;

	if (k_event && k_event->event) {
		rs_k_free(k_event->event);
		k_event->event = NULL;
	}

	return ret;
}

// Wait Event
rs_k_event_t rs_k_event_wait(struct rs_k_event *k_event, rs_k_event_t event_mask)
{
	rs_k_event_t ret_event = K_EVENT_INIT;
	s32 status = 0;
	struct k_event *temp_event = NULL;

	if (k_event && k_event->event) {
		temp_event = k_event->event;
		status = wait_event_interruptible(temp_event->wait_queue,
						  ((ret_event = atomic_read(&temp_event->condition)) &
						   event_mask));

		// RS_DBG("P:%s[%d]:s[%d]:re[0x%X]:e[%d]\n", __func__, __LINE__, status, ret_event, event_mask);

		// if (status != 0) {
		//	ret_event = K_EVENT_INIT;
		// }
	}

	return ret_event;
}

// Try to wait Event once
rs_k_event_t rs_k_event_trywait(struct rs_k_event *k_event)
{
	rs_k_event_t ret_event = K_EVENT_INIT;
	struct k_event *temp_event = NULL;

	if (k_event && k_event->event) {
		temp_event = k_event->event;
		ret_event = atomic_read(&temp_event->condition);
	}

	return ret_event;
}

// Wait Event until up or timeout
rs_k_event_t rs_k_event_timed_wait(struct rs_k_event *k_event, rs_k_event_t event_mask, u32 usec)
{
	rs_k_event_t ret_event = K_EVENT_INIT;
	s32 status = 0;
	struct k_event *temp_event = NULL;

	if (k_event && k_event->event) {
		temp_event = k_event->event;
		if (usec == 0) {
			ret_event = rs_k_event_wait(k_event, event_mask);
		} else {
			status = wait_event_timeout(temp_event->wait_queue,
						    ((ret_event = atomic_read(&temp_event->condition)) &
						     event_mask),
						    usecs_to_jiffies(usec));
		}
	}

	return ret_event;
}

// Set Event
rs_ret rs_k_event_post(struct rs_k_event *k_event, rs_k_event_t event_mask)
{
	rs_ret ret = RS_FAIL;
	struct k_event *temp_event = NULL;

	if (k_event && k_event->event) {
		temp_event = k_event->event;
		atomic_set(&temp_event->condition, event_mask);
		wake_up(&temp_event->wait_queue);

		ret = RS_SUCCESS;
	}

	return ret;
}

// Reset Event
rs_ret rs_k_event_reset(struct rs_k_event *k_event)
{
	rs_ret ret = RS_FAIL;
	struct k_event *temp_event = NULL;

	if (k_event && k_event->event) {
		temp_event = k_event->event;
		atomic_set(&temp_event->condition, K_EVENT_INIT);

		ret = RS_SUCCESS;
	}

	return ret;
}
