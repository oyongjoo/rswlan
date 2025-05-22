// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Copyright (C) [2022-2025] Renesas Electronics Corporation and/or its
 * affiliates.
 */

#ifndef _RS_NET_DFS_H_
#define _RS_NET_DFS_H_

#include <linux/nl80211.h>
#include "rs_net_priv.h"

struct rs_net_vif_priv;
struct rs_net_cfg80211_priv;

enum rwnx_radar_detector
{
	RS_DFS_DETECT_DISABLE = 0,
	RS_DFS_DETECT_ENABLE = 1,
	RS_DFS_DETECT_REPORT = 2
};

#ifdef CONFIG_RS_5G_DFS
#include <linux/workqueue.h>
#include <linux/spinlock.h>

bool rs_net_dfs_detection_init(struct rs_net_dfs_priv *dfs);
void rs_net_dfs_detection_deinit(struct rs_net_dfs_priv *dfs);
bool rs_net_dfs_set_domain(struct rs_net_dfs_priv *dfs, enum nl80211_dfs_regions region, u8 chain);
void rs_net_dfs_detection_enable(struct rs_net_dfs_priv *dfs, u8 enable, u8 chain);
bool rs_net_dfs_detection_enbaled(struct rs_net_dfs_priv *dfs, u8 chain);
void rs_net_dfs_cac_start(struct rs_net_dfs_priv *dfs, u32 cac_time_ms, struct rs_net_vif_priv *vif);
void rs_net_dfs_detection_enabled_on_cur_channel(struct rs_net_cfg80211_priv *net_priv);
int rs_net_dfs_dump_pattern_detector(char *buf, size_t len, struct rs_net_dfs_priv *dfs, u8 chain);
int rs_net_dfs_dump_detected(char *buf, size_t len, struct rs_net_dfs_priv *dfs, u8 chain);

#else

static inline bool rs_net_dfs_detection_init(struct rs_net_dfs_priv *dfs)
{
	return true;
}

static inline void rs_net_dfs_detection_deinit(struct rs_net_dfs_priv *dfs)
{
}

static inline bool rs_net_dfs_set_domain(struct rs_net_dfs_priv *dfs, enum nl80211_dfs_regions region,
					 u8 chain)
{
	return true;
}

static inline void rs_net_dfs_detection_enable(struct rs_net_dfs_priv *dfs, u8 enable, u8 chain)
{
}

static inline bool rs_net_dfs_detection_enbaled(struct rs_net_dfs_priv *dfs, u8 chain)
{
	return false;
}

static inline void rs_net_dfs_cac_start(struct rs_net_dfs_priv *dfs, u32 cac_time_ms,
					struct rs_net_vif_priv *vif)
{
}

static inline void rs_net_dfs_detection_enabled_on_cur_channel(struct rs_net_cfg80211_priv *net_priv)
{
}

static inline int rs_net_dfs_dump_pattern_detector(char *buf, size_t len, struct rs_net_dfs_priv *dfs,
						   u8 chain)
{
	return 0;
}

static inline int rs_net_dfs_dump_detected(char *buf, size_t len, struct rs_net_dfs_priv *dfs, u8 chain)
{
	return 0;
}

#endif /* CONFIG_RS_5G_DFS */

#endif // _RS_NET_DFS_H_
