// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Copyright (C) [2022-2025] Renesas Electronics Corporation and/or its
 * affiliates.
 */

#ifndef RS_NET_CFG80211_H
#define RS_NET_CFG80211_H

////////////////////////////////////////////////////////////////////////////////
/// INCLUDE

#include <net/cfg80211.h>

#include "rs_type.h"
#include "rs_net_priv.h"

////////////////////////////////////////////////////////////////////////////////
/// MACRO DEFINITION

#define RS_NET_CH_INIT_VALUE (0xFF)

////////////////////////////////////////////////////////////////////////////////
/// TYPE DEFINITION

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL VARIABLE

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL FUNCTION

// Initialize CFG80211
rs_ret rs_net_cfg80211_init(struct rs_c_if *c_if);

// Deinitialize CFG80211
rs_ret rs_net_cfg80211_deinit(struct rs_c_if *c_if);

rs_ret rs_net_cfg80211_set_chaninfo(struct rs_net_vif_priv *vif_priv, u8 ch_idx,
				    struct cfg80211_chan_def *chandef);
rs_ret rs_net_cfg80211_unset_chaninfo(struct rs_net_vif_priv *vif_priv);
struct rs_net_chan_info *rs_net_cfg80211_get_chaninfo(struct rs_net_vif_priv *vif_priv);

void rs_net_cfg80211_wapi_enable(struct wiphy *wiphy);

void rs_net_cfg80211_mfp_enable(struct wiphy *wiphy);

void rs_net_cfg80211_ccmp256_enable(struct wiphy *wiphy);

void rs_net_cfg80211_gcmp_enable(struct wiphy *wiphy);

rs_ret rs_net_cfg80211_scan_done(struct rs_c_if *c_if);

rs_ret rs_net_cfg80211_scan_stop_and_wait(struct rs_c_if *c_if, struct rs_net_vif_priv *vif_priv);

void rs_net_cac_stop(struct rs_net_dfs_priv *dfs);

#endif /* RS_NET_CFG80211_H */
