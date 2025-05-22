// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Copyright (C) [2022-2025] Renesas Electronics Corporation and/or its
 * affiliates.
 */

#ifndef RS_K_EVENT_H
#define RS_K_EVENT_H

////////////////////////////////////////////////////////////////////////////////
/// INCLUDE

#include "rs_type.h"

////////////////////////////////////////////////////////////////////////////////
/// MACRO DEFINITION

#define K_EVENT_INIT (0)
#define K_EVENT_EXIT (0xFFFF)

////////////////////////////////////////////////////////////////////////////////
/// TYPE DEFINITION

struct rs_k_event {
	void *event;
};

typedef u32 rs_k_event_t;

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL VARIABLE

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL FUNCTION

// Create event
rs_ret rs_k_event_create(struct rs_k_event *k_event);

// Destroy Event
rs_ret rs_k_event_destroy(struct rs_k_event *k_event);

// Wait Event
rs_k_event_t rs_k_event_wait(struct rs_k_event *k_event, rs_k_event_t event_mask);

// Try to wait Event once
rs_k_event_t rs_k_event_trywait(struct rs_k_event *k_event);

// Wait Event until up or timeout
rs_k_event_t rs_k_event_timed_wait(struct rs_k_event *k_event, rs_k_event_t event_mask, u32 usec);

// Set Event
rs_ret rs_k_event_post(struct rs_k_event *k_event, rs_k_event_t event_mask);

// Reset Event
rs_ret rs_k_event_reset(struct rs_k_event *k_event);

#endif /* RS_K_EVENT_H */
