// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Copyright (C) [2022-2025] Renesas Electronics Corporation and/or its
 * affiliates.
 */

#ifndef RS_K_SEMA_H
#define RS_K_SEMA_H

////////////////////////////////////////////////////////////////////////////////
/// INCLUDE

#include "rs_type.h"

////////////////////////////////////////////////////////////////////////////////
/// MACRO DEFINITION

////////////////////////////////////////////////////////////////////////////////
/// TYPE DEFINITION

struct rs_k_sema {
	void *sema;
};

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL VARIABLE

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL FUNCTION

// Create Semaphore
rs_ret rs_k_sema_create(struct rs_k_sema *k_sema, u8 up);

// Destroy Semaphore
rs_ret rs_k_sema_destroy(struct rs_k_sema *k_sema);

// Wait semaphore
rs_ret rs_k_sema_wait(struct rs_k_sema *k_sema);

// Try to wait semaphore once
rs_ret rs_k_sema_trywait(struct rs_k_sema *k_sema);

// Wait semaphore until up or timeout
rs_ret rs_k_sema_timed_wait(struct rs_k_sema *k_sema, u32 usec);

// Set semaphore
rs_ret rs_k_sema_post(struct rs_k_sema *k_sema);

#endif /* RS_K_SEMA_H */
