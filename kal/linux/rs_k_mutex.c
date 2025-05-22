// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Copyright (C) [2022-2025] Renesas Electronics Corporation and/or its
 * affiliates.
 */

////////////////////////////////////////////////////////////////////////////////
/// INCLUDE

#include <linux/mutex.h>

#include "rs_type.h"
#include "rs_k_mem.h"

#include "rs_k_mutex.h"

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

rs_ret rs_k_mutex_create(struct rs_k_mutex *k_mutex)
{
	rs_ret ret = RS_FAIL;
	struct mutex *temp_mutex = NULL;

	if (k_mutex) {
		if (k_mutex->init != TRUE) {
			temp_mutex = rs_k_calloc(sizeof(struct mutex));
			if (temp_mutex) {
				mutex_init(temp_mutex);

				k_mutex->mutex = temp_mutex;
				k_mutex->init = TRUE;

				ret = RS_SUCCESS;
			} else {
				ret = RS_MEMORY_FAIL;
			}
		} else {
			ret = RS_EXIST;
		}
	}

	return ret;
}

rs_ret rs_k_mutex_destroy(struct rs_k_mutex *k_mutex)
{
	rs_ret ret = RS_FAIL;

	if (k_mutex) {
		if (k_mutex->init == TRUE) {
			k_mutex->init = FALSE;

			mutex_destroy(k_mutex->mutex);
		}
		if (k_mutex->mutex) {
			rs_k_free(k_mutex->mutex);
			k_mutex->mutex = NULL;
		}

		ret = RS_SUCCESS;
	}

	return ret;
}

rs_ret rs_k_mutex_lock(struct rs_k_mutex *k_mutex)
{
	rs_ret ret = RS_FAIL;

	if (k_mutex) {
		if (k_mutex->init == FALSE) {
			ret = rs_k_mutex_create(k_mutex);
		}

		if (k_mutex->init == TRUE) {
			mutex_lock(k_mutex->mutex);
			ret = RS_SUCCESS;
		}
	}

	return ret;
}

rs_ret rs_k_mutex_trylock(struct rs_k_mutex *k_mutex)
{
	rs_ret ret = RS_FAIL;

	if (k_mutex) {
		if (k_mutex->init == FALSE) {
			ret = rs_k_mutex_create(k_mutex);
		}

		if (k_mutex->init == TRUE) {
			if (mutex_trylock(k_mutex->mutex) != 0) {
				ret = RS_SUCCESS;
			}
		}
	}

	return ret;
}

rs_ret rs_k_mutex_unlock(struct rs_k_mutex *k_mutex)
{
	rs_ret ret = RS_FAIL;

	if (k_mutex) {
		if (k_mutex->init == FALSE) {
			ret = rs_k_mutex_create(k_mutex);
		}
		if (k_mutex->init == TRUE) {
			mutex_unlock(k_mutex->mutex);
			ret = RS_SUCCESS;
		}
	}

	return ret;
}

#ifndef CONFIG_RS_COMBINE_DRIVER
EXPORT_SYMBOL(rs_k_mutex_create);
EXPORT_SYMBOL(rs_k_mutex_destroy);
EXPORT_SYMBOL(rs_k_mutex_lock);
EXPORT_SYMBOL(rs_k_mutex_unlock);
#endif