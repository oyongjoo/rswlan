// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Copyright (C) [2022-2025] Renesas Electronics Corporation and/or its
 * affiliates.
 */

////////////////////////////////////////////////////////////////////////////////
/// INCLUDE

#include <linux/semaphore.h>
#include <linux/sched.h>

#include "rs_type.h"
#include "rs_k_mem.h"

#include "rs_k_sema.h"

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

rs_ret rs_k_sema_create(struct rs_k_sema *k_sema, u8 up)
{
	rs_ret ret = RS_FAIL;
	struct semaphore *temp_sema = NULL;

	if (k_sema) {
		temp_sema = rs_k_calloc(sizeof(struct semaphore));
		if (temp_sema) {
			sema_init(temp_sema, up);

			k_sema->sema = temp_sema;

			ret = RS_SUCCESS;
		} else {
			ret = RS_MEMORY_FAIL;
		}
	}

	return ret;
}

rs_ret rs_k_sema_destroy(struct rs_k_sema *k_sema)
{
	rs_ret ret = RS_SUCCESS;

	if (k_sema) {
		if (k_sema->sema) {
			rs_k_free(k_sema->sema);
			k_sema->sema = NULL;
		}
	}

	return ret;
}

rs_ret rs_k_sema_wait(struct rs_k_sema *k_sema)
{
	rs_ret ret = RS_FAIL;

	if (k_sema) {
		if (down_interruptible(k_sema->sema) != 0) {
			ret = RS_FAIL;
		} else {
			ret = RS_SUCCESS;
		}
	}

	return ret;
}

rs_ret rs_k_sema_trywait(struct rs_k_sema *k_sema)
{
	rs_ret ret = RS_SUCCESS;

	if (k_sema) {
		if (down_trylock(k_sema->sema) != 0) {
			ret = RS_FAIL;
		} else {
			ret = RS_SUCCESS;
		}
	}

	return ret;
}

rs_ret rs_k_sema_timed_wait(struct rs_k_sema *k_sema, u32 usec)
{
	rs_ret ret = RS_SUCCESS;

	if (k_sema) {
		if (down_timeout(k_sema->sema, usecs_to_jiffies(usec)) != 0) {
			ret = RS_FAIL;
		} else {
			ret = RS_SUCCESS;
		}
	}

	return ret;
}

rs_ret rs_k_sema_post(struct rs_k_sema *k_sema)
{
	rs_ret ret = RS_FAIL;

	if (k_sema) {
		up(k_sema->sema);

		ret = RS_SUCCESS;
	}

	return ret;
}
