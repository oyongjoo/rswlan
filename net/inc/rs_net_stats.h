// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Copyright (C) [2022-2025] Renesas Electronics Corporation and/or its
 * affiliates.
 */

#ifndef RS_NET_STATS_H
#define RS_NET_STATS_H

enum
{
	RX_STATS_CCK = (1 << 0),
	RX_STATS_OFDM = (1 << 1),
	RX_STATS_HT = (1 << 2),
	RX_STATS_VHT = (1 << 3),
	RX_STATS_HE_SU = (1 << 4),
	RX_STATS_HE_MU = (1 << 5),
	RX_STATS_HE_EXT = (1 << 6),
	RX_STATS_HE_TRIGFLAG_LONG = (1 << 7)
};

#ifdef CONFIG_DBG_STATS
void rs_net_rx_status_update(struct rs_net_cfg80211_priv *net_priv, struct rs_net_sta_stats *stats);
int rs_net_stats_init(struct rs_net_cfg80211_priv *net_priv);
int rs_net_stats_deinit(struct rs_net_cfg80211_priv *net_priv);
#else
static inline void rs_net_rx_status_update(struct rs_net_cfg80211_priv *net_priv,
					   struct rs_net_sta_stats *stats)
{
	return 0;
}
static inline int rs_net_stats_init(struct rs_net_cfg80211_priv *net_priv)
{
	return 0;
}
static inline int rs_net_stats_deinit(struct rs_net_cfg80211_priv *net_priv)
{
	return 0;
}
#endif

#endif // RS_NET_STATS_H