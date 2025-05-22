// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Copyright (C) [2022-2025] Renesas Electronics Corporation and/or its
 * affiliates.
 */

#ifndef RS_K_MUTEX_H
#define RS_K_MUTEX_H

////////////////////////////////////////////////////////////////////////////////
/// INCLUDE

#include "rs_type.h"

////////////////////////////////////////////////////////////////////////////////
/// MACRO DEFINITION

#define __RS_K_MUTEX_INIT { 0, NULL }

////////////////////////////////////////////////////////////////////////////////
/// TYPE DEFINITION

struct rs_k_mutex {
	bool init;
	void *mutex;
};

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL VARIABLE

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL FUNCTION

// Create Mutex
rs_ret rs_k_mutex_create(struct rs_k_mutex *k_mutex);

// Destroy Mutex
rs_ret rs_k_mutex_destroy(struct rs_k_mutex *k_mutex);

// Lock Mutex
rs_ret rs_k_mutex_lock(struct rs_k_mutex *k_mutex);

// Try to lock
rs_ret rs_k_mutex_trylock(struct rs_k_mutex *k_mutex);

// Unlock Mutex
rs_ret rs_k_mutex_unlock(struct rs_k_mutex *k_mutex);

#endif /* RS_K_MUTEX_H */
