// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Copyright (C) [2022-2025] Renesas Electronics Corporation and/or its
 * affiliates.
 */

#ifndef RS_NET_DEV_H
#define RS_NET_DEV_H

////////////////////////////////////////////////////////////////////////////////
/// INCLUDE

#include "rs_type.h"

////////////////////////////////////////////////////////////////////////////////
/// MACRO DEFINITION

////////////////////////////////////////////////////////////////////////////////
/// TYPE DEFINITION

typedef void rs_net_dev_t;
typedef void rs_net_vif_priv_t;

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL VARIABLE

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL FUNCTION

// Initialize netdev
void rs_net_dev_init(rs_net_dev_t *ndev);

// Set netdev ops
rs_ret rs_net_dev_set_ops(rs_net_dev_t *ndev);

// Set monitor ops
rs_ret rs_net_dev_set_monitor_ops(rs_net_dev_t *ndev);

// Send received packets to net_device.
rs_ret rs_net_dev_rx(rs_net_dev_t *ndev, u8 *skb);

// Check whether interface is up?
rs_ret rs_net_if_is_up(rs_net_dev_t *ndev);

// Check whether interface is up?
rs_ret rs_net_vif_is_up(rs_net_vif_priv_t *vif_priv);

// Check whether interface of index is up?
rs_ret rs_net_vif_idx_is_up(struct rs_c_if *c_if, s16 vif_idx);

// Control network transmittion
rs_ret rs_net_if_tx_stop(struct rs_c_if *c_if, s8 vif_idx, bool stop);

#endif /* RS_NET_DEV_H */
