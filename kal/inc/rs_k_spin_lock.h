// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Copyright (C) [2022-2025] Renesas Electronics Corporation and/or its
 * affiliates.
 */

#ifndef RS_K_SPINLOCK_H
#define RS_K_SPINLOCK_H

////////////////////////////////////////////////////////////////////////////////
/// INCLUDE

#include "rs_type.h"

////////////////////////////////////////////////////////////////////////////////
/// MACRO DEFINITION

#define __RS_K_SPIN_LOCK_INIT { 0, NULL }

////////////////////////////////////////////////////////////////////////////////
/// TYPE DEFINITION

struct rs_k_spin_lock {
	bool init;
	void *lock;
};

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL VARIABLE

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL FUNCTION

// Create Spin Lock
rs_ret rs_k_spin_lock_create(struct rs_k_spin_lock *k_spin_lock);

// Destroy Spin Lock
rs_ret rs_k_spin_lock_destroy(struct rs_k_spin_lock *k_spin_lock);

// Lock Spin Lock
rs_ret rs_k_spin_lock(struct rs_k_spin_lock *k_spin_lock);

// Try to Spin Lock
rs_ret rs_k_spin_trylock(struct rs_k_spin_lock *k_spin_lock);

// Unlock Spin Lock
rs_ret rs_k_spin_unlock(struct rs_k_spin_lock *k_spin_lock);

#endif /* RS_K_SPINLOCK_H */
