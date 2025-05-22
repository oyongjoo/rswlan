// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Copyright (C) [2022-2025] Renesas Electronics Corporation and/or its
 * affiliates.
 */

////////////////////////////////////////////////////////////////////////////////
/// INCLUDE

// #include <linux/spinlock_types.h>
#include <linux/spinlock.h>

#include "rs_type.h"
#include "rs_k_mem.h"

#include "rs_k_spin_lock.h"

////////////////////////////////////////////////////////////////////////////////
/// MACRO DEFINITION

////////////////////////////////////////////////////////////////////////////////
/// TYPE DEFINITION

struct spin_lock {
	unsigned long flags;
	spinlock_t lock;
};

////////////////////////////////////////////////////////////////////////////////
/// LOCAL VARIABLE

////////////////////////////////////////////////////////////////////////////////
/// LOCAL FUNCTION

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL FUNCTION
// static DEFINE_SPINLOCK(test_lock);

rs_ret rs_k_spin_lock_create(struct rs_k_spin_lock *k_spin_lock)
{
	rs_ret ret = RS_FAIL;
	struct spin_lock *temp_lock = NULL;

	if (k_spin_lock) {
		if (k_spin_lock->init != TRUE) {
			temp_lock = rs_k_calloc(sizeof(struct spin_lock));
			if (temp_lock) {
#ifdef CONFIG_PREEMPT_RT
				DEFINE_SPINLOCK(temp_lock_rt);
				temp_lock->lock = temp_lock_rt;
#else
				temp_lock->lock = __SPIN_LOCK_UNLOCKED(temp_lock->lock);
#endif

				k_spin_lock->lock = temp_lock;
				k_spin_lock->init = TRUE;

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

rs_ret rs_k_spin_lock_destroy(struct rs_k_spin_lock *k_spin_lock)
{
	rs_ret ret = RS_FAIL;

	if (k_spin_lock) {
		k_spin_lock->init = FALSE;
		if (k_spin_lock->lock) {
			rs_k_free(k_spin_lock->lock);
			k_spin_lock->lock = NULL;
		}

		ret = RS_SUCCESS;
	}

	return ret;
}

rs_ret rs_k_spin_lock(struct rs_k_spin_lock *k_spin_lock)
{
	rs_ret ret = RS_FAIL;
	struct spin_lock *temp_lock = NULL;

	if (k_spin_lock) {
		if (k_spin_lock->init == FALSE) {
			ret = rs_k_spin_lock_create(k_spin_lock);
		}

		if ((k_spin_lock->init == TRUE) && (k_spin_lock->lock)) {
			temp_lock = k_spin_lock->lock;
			spin_lock_irqsave(&temp_lock->lock, temp_lock->flags);
			ret = RS_SUCCESS;
		}
	}

	return ret;
}

rs_ret rs_k_spin_trylock(struct rs_k_spin_lock *k_spin_lock)
{
	rs_ret ret = RS_FAIL;
	struct spin_lock *temp_lock = NULL;

	if (k_spin_lock) {
		if (k_spin_lock->init == FALSE) {
			ret = rs_k_spin_lock_create(k_spin_lock);
		}

		if ((k_spin_lock->init == TRUE) && (k_spin_lock->lock)) {
			temp_lock = k_spin_lock->lock;
			if (spin_trylock_irqsave(&temp_lock->lock, temp_lock->flags) != 0) {
				ret = RS_SUCCESS;
			}
		}
	}

	return ret;
}

rs_ret rs_k_spin_unlock(struct rs_k_spin_lock *k_spin_lock)
{
	rs_ret ret = RS_FAIL;
	struct spin_lock *temp_lock = NULL;

	if (k_spin_lock) {
		if (k_spin_lock->init == FALSE) {
			ret = rs_k_spin_lock_create(k_spin_lock);
		}
		if ((k_spin_lock->init == TRUE) && (k_spin_lock->lock)) {
			temp_lock = k_spin_lock->lock;
			spin_unlock_irqrestore(&temp_lock->lock, temp_lock->flags);
			ret = RS_SUCCESS;
		}
	}

	return ret;
}
