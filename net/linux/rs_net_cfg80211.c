// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Copyright (C) [2022-2025] Renesas Electronics Corporation and/or its
 * affiliates.
 */

////////////////////////////////////////////////////////////////////////////////
/// INCLUDE

#include <linux/version.h>
#include <linux/module.h>
#include <linux/firmware.h>
#include <linux/rtnetlink.h>
// #include <linux/etherdevice.h>
#include <linux/inetdevice.h>
#include <net/cfg80211.h>
#include <net/ip.h>

#include "rs_type.h"
#include "rs_c_dbg.h"
#include "rs_k_mem.h"
#include "rs_c_cmd.h"
#include "rs_c_if.h"
// #include "rs_c_status.h"
#include "rs_c_ctrl.h"

#include "rs_core.h"
#include "rs_net_params.h"

#include "rs_net_cfg80211.h"
#include "rs_net_priv.h"
#include "rs_net_dev.h"
#include "rs_net_ctrl.h"

#include "rs_net_tx_data.h"

#ifdef CONFIG_DEBUG_FS
#include "rs_net_dbgfs.h"
#endif

#ifdef CONFIG_NL80211_TESTMODE
#include "rs_net_testmode.h"
#endif
#include "rs_net_dfs.h"
#include "rs_net_stats.h"

////////////////////////////////////////////////////////////////////////////////
/// MACRO DEFINITION

#define NET_RESERVED_CIPHER_NUM (5)
#define NET_DEFAULT_MAC_ADDR	"\xd4\x3d\x39"
#define NET_MAC_ADDR_LEN	(17)

#define RATE(_bitrate, _hw_rate, _flags) \
	{                                \
		.bitrate = (_bitrate),   \
		.flags = (_flags),       \
		.hw_value = (_hw_rate),  \
	}

#define CHAN2G(_channel, _freq)            \
	{                                  \
		.band = NL80211_BAND_2GHZ, \
		.hw_value = (_channel),    \
		.center_freq = (_freq),    \
		.max_antenna_gain = 0,     \
		.max_power = 30,           \
	}

#define CHAN5G(_channel)                                \
	{                                               \
		.band = NL80211_BAND_5GHZ,              \
		.hw_value = (_channel),                 \
		.center_freq = 5000 + (5 * (_channel)), \
		.max_antenna_gain = 0,                  \
		.max_power = 30,                        \
	}

#define RS_CAP_HT \
	{                                                                                                    \
		.ht_supported = true, .cap = 0, .ampdu_factor = IEEE80211_HT_MAX_AMPDU_64K,                \
	.ampdu_density = IEEE80211_HT_MPDU_DENSITY_16,                                             \
	.mcs = {                                                                                   \
	    .rx_mask =                                                                             \
		{                                                                                  \
		    0xff,                                                                          \
		    0,                                                                             \
		    0,                                                                             \
		    0,                                                                             \
		    0,                                                                             \
		    0,                                                                             \
		    0,                                                                             \
		    0,                                                                             \
		    0,                                                                             \
		    0,                                                                             \
		},                                                                                 \
	    .rx_highest = cpu_to_le16(65),                                                         \
	    .tx_params = IEEE80211_HT_MCS_TX_DEFINED,                                              \
	}, \
	}

#define RS_CAP_VHT                                                                                        \
	{                                                                                                 \
		.vht_supported = false, .cap = (7 << IEEE80211_VHT_CAP_MAX_A_MPDU_LENGTH_EXPONENT_SHIFT), \
		.vht_mcs = {                                                                              \
			.rx_mcs_map = cpu_to_le16(IEEE80211_VHT_MCS_SUPPORT_0_9 << 0 |                    \
						  IEEE80211_VHT_MCS_NOT_SUPPORTED << 2 |                  \
						  IEEE80211_VHT_MCS_NOT_SUPPORTED << 4 |                  \
						  IEEE80211_VHT_MCS_NOT_SUPPORTED << 6 |                  \
						  IEEE80211_VHT_MCS_NOT_SUPPORTED << 8 |                  \
						  IEEE80211_VHT_MCS_NOT_SUPPORTED << 10 |                 \
						  IEEE80211_VHT_MCS_NOT_SUPPORTED << 12 |                 \
						  IEEE80211_VHT_MCS_NOT_SUPPORTED << 14),                 \
			.tx_mcs_map = cpu_to_le16(IEEE80211_VHT_MCS_SUPPORT_0_9 << 0 |                    \
						  IEEE80211_VHT_MCS_NOT_SUPPORTED << 2 |                  \
						  IEEE80211_VHT_MCS_NOT_SUPPORTED << 4 |                  \
						  IEEE80211_VHT_MCS_NOT_SUPPORTED << 6 |                  \
						  IEEE80211_VHT_MCS_NOT_SUPPORTED << 8 |                  \
						  IEEE80211_VHT_MCS_NOT_SUPPORTED << 10 |                 \
						  IEEE80211_VHT_MCS_NOT_SUPPORTED << 12 |                 \
						  IEEE80211_VHT_MCS_NOT_SUPPORTED << 14),                 \
		}                                                                                         \
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 20, 0)
#define RS_CAP_HE \
	{                                                          \
		.has_he = false,                                 \
	.he_cap_elem =                                   \
	    {                                            \
		.mac_cap_info[0] = 0,                    \
		.mac_cap_info[1] = 0,                    \
		.mac_cap_info[2] = 0,                    \
		.mac_cap_info[3] = 0,                    \
		.mac_cap_info[4] = 0,                    \
		.mac_cap_info[5] = 0,                    \
		.phy_cap_info[0] = 0,                    \
		.phy_cap_info[1] = 0,                    \
		.phy_cap_info[2] = 0,                    \
		.phy_cap_info[3] = 0,                    \
		.phy_cap_info[4] = 0,                    \
		.phy_cap_info[5] = 0,                    \
		.phy_cap_info[6] = 0,                    \
		.phy_cap_info[7] = 0,                    \
		.phy_cap_info[8] = 0,                    \
		.phy_cap_info[9] = 0,                    \
		.phy_cap_info[10] = 0,                   \
	    },                                           \
	.he_mcs_nss_supp =                               \
	    {                                            \
		.rx_mcs_80 = cpu_to_le16(0xfffa),        \
		.tx_mcs_80 = cpu_to_le16(0xfffa),        \
		.rx_mcs_160 = cpu_to_le16(0xffff),       \
		.tx_mcs_160 = cpu_to_le16(0xffff),       \
		.rx_mcs_80p80 = cpu_to_le16(0xffff),     \
		.tx_mcs_80p80 = cpu_to_le16(0xffff),     \
	    },                                           \
	.ppe_thres = {0x00}, \
	}
#endif

static const s32 net_ac2hwq[IEEE80211_NUM_ACS] = { [NL80211_TXQ_Q_VO] = RS_AC_VO,
						   [NL80211_TXQ_Q_VI] = RS_AC_VI,
						   [NL80211_TXQ_Q_BE] = RS_AC_BE,
						   [NL80211_TXQ_Q_BK] = RS_AC_BK };

#if __has_attribute(__fallthrough__)
#define fallthrough __attribute__((__fallthrough__))
#else
#define fallthrough \
	do {        \
	} while (0) /* fallthrough */
#endif

#ifdef CONFIG_NL80211_TESTMODE
static const struct nla_policy testmode_attr_policy[TESTMODE_ATTR_MAX] = {
	[TESTMODE_ATTR_CMD] = { .type = NLA_U32 },
	[TESTMODE_ATTR_REG_OFFSET] = { .type = NLA_U32 },
	[TESTMODE_ATTR_REG_VALUE32] = { .type = NLA_U32 },
	[TESTMODE_ATTR_REG_FILTER] = { .type = NLA_U32 },

	/* RF commands */
	[TESTMODE_ATTR_START] = { .type = NLA_U8 },
	[TESTMODE_ATTR_CH] = { .type = NLA_U16 },
	[TESTMODE_ATTR_FRAMES_NUM] = { .type = NLA_U16 },
	[TESTMODE_ATTR_FRAMES_LEN] = { .type = NLA_U16 },
	[TESTMODE_ATTR_RATE] = { .type = NLA_U32 },
	[TESTMODE_ATTR_POWER] = { .type = NLA_U32 },
	[TESTMODE_ATTR_ADDR_DEST] = { .type = NLA_U64 },
	[TESTMODE_ATTR_BSSID] = { .type = NLA_U64 },
	[TESTMODE_ATTR_GI] = { .type = NLA_U8 },
	[TESTMODE_ATTR_GREEN] = { .type = NLA_U8 },
	[TESTMODE_ATTR_PREAMBLE] = { .type = NLA_U8 },
	[TESTMODE_ATTR_QOS] = { .type = NLA_U8 },
	[TESTMODE_ATTR_ACK] = { .type = NLA_U8 },
	[TESTMODE_ATTR_AIFSN] = { .type = NLA_U8 },
	[TESTMODE_ATTR_PER_PASS] = { .type = NLA_U32 },
	[TESTMODE_ATTR_PER_FCS] = { .type = NLA_U32 },
	[TESTMODE_ATTR_PER_PHY] = { .type = NLA_U32 },
	[TESTMODE_ATTR_PER_OVERFLOW] = { .type = NLA_U32 },
	[TESTMODE_ATTR_HOST_LOG_LEVEL] = { .type = NLA_U8 },

	[TESTMODE_ATTR_STATS_RSSI] = { .type = NLA_U32 },
	[TESTMODE_ATTR_STATS_BSSID] = { .type = NLA_STRING },
	[TESTMODE_ATTR_STATS_STAIDX] = { .type = NLA_U8 },
	[TESTMODE_ATTR_STATS_OWN_MAC] = { .type = NLA_STRING },
	[TESTMODE_ATTR_STATS_FLAG] = { .type = NLA_U8 },
	[TESTMODE_ATTR_STATS_RX_OFDM] = { .type = NLA_U32, .len = 8 },
	[TESTMODE_ATTR_STATS_READY] = { .type = NLA_U8 },
};
#endif /* CONFIG_NL80211_TESTMODE */

////////////////////////////////////////////////////////////////////////////////
/// TYPE DEFINITION

static struct wireless_dev *net_cfg80211_add_virtual_iface(struct wiphy *wiphy, const char *name,
							   unsigned char name_assign_type,
							   enum nl80211_iftype type,
							   struct vif_params *params);

static int net_cfg80211_del_virtual_iface(struct wiphy *wiphy, struct wireless_dev *wdev);

static int net_cfg80211_change_virtual_iface(struct wiphy *wiphy, struct net_device *ndev,
					     enum nl80211_iftype type, struct vif_params *params);

static int net_cfg80211_scan(struct wiphy *wiphy, struct cfg80211_scan_request *request);

static void net_cfg80211_scan_abort(struct wiphy *wiphy, struct wireless_dev *wdev);

static int net_cfg80211_connect(struct wiphy *wiphy, struct net_device *ndev,
				struct cfg80211_connect_params *sme);

static int net_cfg80211_disconnect(struct wiphy *wiphy, struct net_device *ndev, u16 reason_code);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
static int net_cfg80211_add_key(struct wiphy *wiphy, struct net_device *ndev, int link_id, u8 key_index,
				bool pairwise, const u8 *mac_addr, struct key_params *params);

static int net_cfg80211_get_key(struct wiphy *wiphy, struct net_device *ndev, int link_id, u8 key_index,
				bool pairwise, const u8 *mac_addr, void *cookie,
				void (*callback)(void *cookie, struct key_params *));

static int net_cfg80211_del_key(struct wiphy *wiphy, struct net_device *ndev, int link_id, u8 key_index,
				bool pairwise, const u8 *mac_addr);

static int net_cfg80211_set_default_key(struct wiphy *wiphy, struct net_device *ndev, int link_id,
					u8 key_index, bool unicast, bool multicast);

static int net_cfg80211_set_default_mgmt_key(struct wiphy *wiphy, struct net_device *ndev, int link_id,
					     u8 key_index);
#else
static int net_cfg80211_add_key(struct wiphy *wiphy, struct net_device *ndev, u8 key_index, bool pairwise,
				const u8 *mac_addr, struct key_params *params);

static int net_cfg80211_get_key(struct wiphy *wiphy, struct net_device *ndev, u8 key_index, bool pairwise,
				const u8 *mac_addr, void *cookie,
				void (*callback)(void *cookie, struct key_params *));

static int net_cfg80211_del_key(struct wiphy *wiphy, struct net_device *ndev, u8 key_index, bool pairwise,
				const u8 *mac_addr);

static int net_cfg80211_set_default_key(struct wiphy *wiphy, struct net_device *ndev, u8 key_index,
					bool unicast, bool multicast);

static int net_cfg80211_set_default_mgmt_key(struct wiphy *wiphy, struct net_device *ndev, u8 key_index);
#endif

static int net_cfg80211_add_station(struct wiphy *wiphy, struct net_device *ndev, const u8 *mac,
				    struct station_parameters *params);

static int net_cfg80211_del_station(struct wiphy *wiphy, struct net_device *ndev,
				    struct station_del_parameters *params);

static int net_cfg80211_change_station(struct wiphy *wiphy, struct net_device *ndev, const u8 *mac,
				       struct station_parameters *params);

static int net_cfg80211_mgmt_tx(struct wiphy *wiphy, struct wireless_dev *wdev,
				struct cfg80211_mgmt_tx_params *params, u64 *cookie);

static int net_cfg80211_mgmt_tx_cancel_wait(struct wiphy *wiphy, struct wireless_dev *wdev, u64 cookie);

static int net_cfg80211_start_ap(struct wiphy *wiphy, struct net_device *ndev,
				 struct cfg80211_ap_settings *settings);

static int net_cfg80211_change_beacon(struct wiphy *wiphy, struct net_device *ndev,
				      struct cfg80211_beacon_data *info);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0)
static int net_cfg80211_stop_ap(struct wiphy *wiphy, struct net_device *ndev, unsigned int link_id);
#else
static int net_cfg80211_stop_ap(struct wiphy *wiphy, struct net_device *ndev);
#endif

static int net_cfg80211_set_monitor_channel(struct wiphy *wiphy, struct cfg80211_chan_def *chandef);

static int net_cfg80211_probe_client(struct wiphy *wiphy, struct net_device *ndev, const u8 *peer,
				     u64 *cookie);

static int net_cfg80211_set_wiphy_params(struct wiphy *wiphy, u32 changed);

static int net_cfg80211_set_txq_params(struct wiphy *wiphy, struct net_device *ndev,
				       struct ieee80211_txq_params *params);

static int net_cfg80211_set_tx_power(struct wiphy *wiphy, struct wireless_dev *wdev,
				     enum nl80211_tx_power_setting type, int mbm);

static int net_cfg80211_set_power_mgmt(struct wiphy *wiphy, struct net_device *ndev, bool enabled,
				       int timeout);

static int net_cfg80211_get_station(struct wiphy *wiphy, struct net_device *ndev, const u8 *mac,
				    struct station_info *sinfo);

static int net_cfg80211_dump_station(struct wiphy *wiphy, struct net_device *ndev, int idx, u8 *mac,
				     struct station_info *sinfo);

static int net_cfg80211_remain_on_channel(struct wiphy *wiphy, struct wireless_dev *wdev,
					  struct ieee80211_channel *chan, unsigned int duration, u64 *cookie);

static int net_cfg80211_cancel_remain_on_channel(struct wiphy *wiphy, struct wireless_dev *wdev, u64 cookie);

static int net_cfg80211_channel_switch(struct wiphy *wiphy, struct net_device *ndev,
				       struct cfg80211_csa_settings *params);

static int net_cfg80211_dump_survey(struct wiphy *wiphy, struct net_device *ndev, int idx,
				    struct survey_info *info);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0)
static int net_cfg80211_get_channel(struct wiphy *wiphy, struct wireless_dev *wdev, unsigned int link_id,
				    struct cfg80211_chan_def *chandef);
#else
static int net_cfg80211_get_channel(struct wiphy *wiphy, struct wireless_dev *wdev,
				    struct cfg80211_chan_def *chandef);
#endif

static int net_cfg80211_start_radar_detection(struct wiphy *wiphy, struct net_device *ndev,
					      struct cfg80211_chan_def *chandef, u32 cac_time_ms);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 1, 0)
static int net_cfg80211_update_ft_ies(struct wiphy *wiphy, struct net_device *ndev,
				      struct cfg80211_update_ft_ies_params *ftie);
#endif

static int net_cfg80211_set_cqm_rssi_config(struct wiphy *wiphy, struct net_device *ndev, int32_t rssi_thold,
					    uint32_t rssi_hyst);

static int net_cfg80211_change_bss(struct wiphy *wiphy, struct net_device *ndev,
				   struct bss_parameters *params);

static struct rs_net_sta_priv *net_get_sta_from_frame_control(struct rs_net_cfg80211_priv *net_priv,
							      struct rs_net_vif_priv *vif_priv, u8 *addr,
							      __le16 fc, bool ap);
#ifdef CONFIG_NL80211_TESTMODE
static s32 net_cfg80211_testmode_cmd(struct wiphy *wiphy, struct wireless_dev *wdev, void *data, int len);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 17, 0)
static int net_cfg80211_external_auth(struct wiphy *wiphy, struct net_device *ndev,
				      struct cfg80211_external_auth_params *params);
#endif

////////////////////////////////////////////////////////////////////////////////
/// LOCAL VARIABLE

static struct cfg80211_ops net_cfg80211_ops = {
	.add_virtual_intf = net_cfg80211_add_virtual_iface,
	.del_virtual_intf = net_cfg80211_del_virtual_iface,
	.change_virtual_intf = net_cfg80211_change_virtual_iface,
	.scan = net_cfg80211_scan,
	.abort_scan = net_cfg80211_scan_abort,
	.connect = net_cfg80211_connect,
	.disconnect = net_cfg80211_disconnect,
	.add_key = net_cfg80211_add_key,
	.get_key = net_cfg80211_get_key,
	.del_key = net_cfg80211_del_key,
	.set_default_key = net_cfg80211_set_default_key,
	.set_default_mgmt_key = net_cfg80211_set_default_mgmt_key,
	.add_station = net_cfg80211_add_station,
	.del_station = net_cfg80211_del_station,
	.change_station = net_cfg80211_change_station,
	.mgmt_tx = net_cfg80211_mgmt_tx,
	.mgmt_tx_cancel_wait = net_cfg80211_mgmt_tx_cancel_wait,
	.start_ap = net_cfg80211_start_ap,
	.change_beacon = net_cfg80211_change_beacon,
	.stop_ap = net_cfg80211_stop_ap,
	.set_monitor_channel = net_cfg80211_set_monitor_channel,
	.probe_client = net_cfg80211_probe_client,
	.set_wiphy_params = net_cfg80211_set_wiphy_params,
	.set_txq_params = net_cfg80211_set_txq_params,
	.set_tx_power = net_cfg80211_set_tx_power,
	.set_power_mgmt = net_cfg80211_set_power_mgmt,
	.get_station = net_cfg80211_get_station,
	.dump_station = net_cfg80211_dump_station,
	.remain_on_channel = net_cfg80211_remain_on_channel,
	.cancel_remain_on_channel = net_cfg80211_cancel_remain_on_channel,
	.channel_switch = net_cfg80211_channel_switch,
	.dump_survey = net_cfg80211_dump_survey,
	.get_channel = net_cfg80211_get_channel,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 1, 0)
	.update_ft_ies = net_cfg80211_update_ft_ies,
#endif
	.set_cqm_rssi_config = net_cfg80211_set_cqm_rssi_config,
	.change_bss = net_cfg80211_change_bss,
	.start_radar_detection = net_cfg80211_start_radar_detection,
	CFG80211_TESTMODE_CMD(net_cfg80211_testmode_cmd)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 17, 0)
		.external_auth = net_cfg80211_external_auth,
#endif
};

static struct ieee80211_iface_limit if_limits[] = {
	{ .max = RS_NET_VIF_DEV_MAX, .types = RS_BIT(NL80211_IFTYPE_AP) | RS_BIT(NL80211_IFTYPE_STATION) }
};

static struct ieee80211_iface_limit if_limits_dfs[] = { { .max = RS_NET_VIF_DEV_MAX,
							  .types = RS_BIT(NL80211_IFTYPE_AP) } };

static const struct ieee80211_iface_combination net_if_comb[] = {
	{
		.limits = if_limits,
		.n_limits = ARRAY_SIZE(if_limits),
		.num_different_channels = RS_NET_CHANINFO_MAX,
		.max_interfaces = RS_NET_VIF_DEV_MAX,
	},
	/* Keep this combination as the last one */
	{
		.limits = if_limits_dfs,
		.n_limits = ARRAY_SIZE(if_limits_dfs),
		.num_different_channels = 1,
		.max_interfaces = RS_NET_VIF_DEV_MAX,
		.radar_detect_widths = (RS_BIT(NL80211_CHAN_WIDTH_20_NOHT) | RS_BIT(NL80211_CHAN_WIDTH_20) |
					RS_BIT(NL80211_CHAN_WIDTH_40) | RS_BIT(NL80211_CHAN_WIDTH_80)),
	}
};

/* There isn't a lot of sense in it, but you can transmit anything you like */
static struct ieee80211_txrx_stypes net_macm_stypes[NUM_NL80211_IFTYPES] = {
	[NL80211_IFTYPE_STATION] = { .tx = 0xffff,
				     .rx = RS_BIT(IEEE80211_STYPE_ACTION >> 4) |
					   RS_BIT(IEEE80211_STYPE_PROBE_REQ >> 4) |
					   RS_BIT(IEEE80211_STYPE_AUTH >> 4) },
	[NL80211_IFTYPE_P2P_CLIENT] = { .tx = 0xffff,
					.rx = RS_BIT(IEEE80211_STYPE_ACTION >> 4) |
					      RS_BIT(IEEE80211_STYPE_PROBE_REQ >> 4) },
	[NL80211_IFTYPE_P2P_GO] = { .tx = 0xffff,
				    .rx = RS_BIT(IEEE80211_STYPE_ASSOC_REQ >> 4) |
					  RS_BIT(IEEE80211_STYPE_REASSOC_REQ >> 4) |
					  RS_BIT(IEEE80211_STYPE_PROBE_REQ >> 4) |
					  RS_BIT(IEEE80211_STYPE_DISASSOC >> 4) |
					  RS_BIT(IEEE80211_STYPE_AUTH >> 4) |
					  RS_BIT(IEEE80211_STYPE_DEAUTH >> 4) |
					  RS_BIT(IEEE80211_STYPE_ACTION >> 4) },
	[NL80211_IFTYPE_P2P_DEVICE] = { .tx = 0xffff,
					.rx = RS_BIT(IEEE80211_STYPE_ACTION >> 4) |
					      RS_BIT(IEEE80211_STYPE_PROBE_REQ >> 4) },
	[NL80211_IFTYPE_AP] = { .tx = 0xffff,
				.rx = RS_BIT(IEEE80211_STYPE_ASSOC_REQ >> 4) |
				      RS_BIT(IEEE80211_STYPE_REASSOC_REQ >> 4) |
				      RS_BIT(IEEE80211_STYPE_PROBE_REQ >> 4) |
				      RS_BIT(IEEE80211_STYPE_DISASSOC >> 4) |
				      RS_BIT(IEEE80211_STYPE_AUTH >> 4) |
				      RS_BIT(IEEE80211_STYPE_DEAUTH >> 4) |
				      RS_BIT(IEEE80211_STYPE_ACTION >> 4) },
	[NL80211_IFTYPE_MESH_POINT] = { .tx = 0xffff,
					.rx = RS_BIT(IEEE80211_STYPE_ACTION >> 4) |
					      RS_BIT(IEEE80211_STYPE_AUTH >> 4) |
					      RS_BIT(IEEE80211_STYPE_DEAUTH >> 4) },
};

static u32 net_cipher_suites[] = {
	WLAN_CIPHER_SUITE_WEP40,
	WLAN_CIPHER_SUITE_WEP104,
	WLAN_CIPHER_SUITE_TKIP,
	WLAN_CIPHER_SUITE_CCMP,
	0, // reserved entries to enable AES-CMAC, GCMP-128/256, CCMP-256, SMS4
	0,
	0,
	0,
	0,
};

static struct ieee80211_rate cfg80211_rate_table[] = {
	RATE(10, 0x00, 0),
	RATE(20, 0x01, IEEE80211_RATE_SHORT_PREAMBLE),
	RATE(55, 0x02, IEEE80211_RATE_SHORT_PREAMBLE),
	RATE(110, 0x03, IEEE80211_RATE_SHORT_PREAMBLE),
	RATE(60, 0x04, 0),
	RATE(90, 0x05, 0),
	RATE(120, 0x06, 0),
	RATE(180, 0x07, 0),
	RATE(240, 0x08, 0),
	RATE(360, 0x09, 0),
	RATE(480, 0x0A, 0),
	RATE(540, 0x0B, 0),
};

/* The channels indexes here are not used anymore */
static struct ieee80211_channel cfg80211_chans_2g[] = {
	CHAN2G(1, 2412),  CHAN2G(2, 2417),  CHAN2G(3, 2422),  CHAN2G(4, 2427),	CHAN2G(5, 2432),
	CHAN2G(6, 2437),  CHAN2G(7, 2442),  CHAN2G(8, 2447),  CHAN2G(9, 2452),	CHAN2G(10, 2457),
	CHAN2G(11, 2462), CHAN2G(12, 2467), CHAN2G(13, 2472), CHAN2G(14, 2484),
};

#ifdef CONFIG_RS_SUPPORT_5G
static struct ieee80211_channel cfg80211_chans_5g[] = {
	CHAN5G(36),  CHAN5G(40),  CHAN5G(44),  CHAN5G(48),  CHAN5G(52),	 CHAN5G(56),  CHAN5G(60),
	CHAN5G(64),  CHAN5G(100), CHAN5G(104), CHAN5G(108), CHAN5G(112), CHAN5G(116), CHAN5G(120),
	CHAN5G(124), CHAN5G(128), CHAN5G(132), CHAN5G(136), CHAN5G(140), CHAN5G(144), CHAN5G(149),
	CHAN5G(153), CHAN5G(157), CHAN5G(161), CHAN5G(165), CHAN5G(169), CHAN5G(173),
};
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 20, 0)
static struct ieee80211_sband_iftype_data cfg80211_cap_he = {
	.types_mask = RS_BIT(NL80211_IFTYPE_STATION) | RS_BIT(NL80211_IFTYPE_AP),
	.he_cap = RS_CAP_HE,
};
#endif

static struct ieee80211_supported_band net_cfg80211_band_2g = {
	.channels = cfg80211_chans_2g,
	.n_channels = ARRAY_SIZE(cfg80211_chans_2g),
	.bitrates = cfg80211_rate_table,
	.n_bitrates = ARRAY_SIZE(cfg80211_rate_table),
	.ht_cap = RS_CAP_HT,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 20, 0)
	.n_iftype_data = 1,
#endif
};

#ifdef CONFIG_RS_SUPPORT_5G
static struct ieee80211_supported_band net_cfg80211_band_5g = {
	.channels = cfg80211_chans_5g,
	.n_channels = ARRAY_SIZE(cfg80211_chans_5g),
	.bitrates = &cfg80211_rate_table[4],
	.n_bitrates = ARRAY_SIZE(cfg80211_rate_table) - 4,
	.ht_cap = RS_CAP_HT,
	.vht_cap = RS_CAP_VHT,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 20, 0)
	.n_iftype_data = 1,
#endif
};
#endif

const struct rs_c_legrate legacy_rate_table[] = {
	[0] = { .idx = 0, .rate = 10 },	   [1] = { .idx = 1, .rate = 20 },   [2] = { .idx = 2, .rate = 55 },
	[3] = { .idx = 3, .rate = 110 },   [4] = { .idx = -1, .rate = 0 },   [5] = { .idx = -1, .rate = 0 },
	[6] = { .idx = -1, .rate = 0 },	   [7] = { .idx = -1, .rate = 0 },   [8] = { .idx = 10, .rate = 480 },
	[9] = { .idx = 8, .rate = 240 },   [10] = { .idx = 6, .rate = 120 }, [11] = { .idx = 4, .rate = 60 },
	[12] = { .idx = 11, .rate = 540 }, [13] = { .idx = 9, .rate = 360 }, [14] = { .idx = 7, .rate = 180 },
	[15] = { .idx = 5, .rate = 90 },
};

////////////////////////////////////////////////////////////////////////////////
/// LOCAL FUNCTION

static void net_dev_init(struct net_device *ndev)
{
	rs_net_dev_init(ndev);
}

static void net_set_default_mac_addr(const u8 *default_mac, u8 *mac_addr)
{
	if (!mac_addr)
		return;

	if (default_mac) {
		int n;

		n = sscanf(default_mac, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", mac_addr + 0, mac_addr + 1,
			   mac_addr + 2, mac_addr + 3, mac_addr + 4, mac_addr + 5);
		if (n == ETH_ALEN)
			return;
	}

	mac_addr[0] = NET_DEFAULT_MAC_ADDR[0];
	mac_addr[1] = NET_DEFAULT_MAC_ADDR[1];
	mac_addr[2] = NET_DEFAULT_MAC_ADDR[2];
	get_random_bytes(&mac_addr[3], 1);
	get_random_bytes(&mac_addr[4], 1);
	get_random_bytes(&mac_addr[5], 1);
}

static const u8 *find_tag(const u8 *file_data, u32 file_size, const u8 *tag_name, u32 tag_len)
{
	u32 curr = 0, line_start = 0, line_size = 0;

	RS_TRACE(RS_FN_ENTRY_STR);

	/* Walk through all the lines of the configuration file */
	while (line_start < file_size) {
		/* Search the end of the current line (or the end of the file) */
		for (curr = line_start; curr < file_size; curr++)
			if (file_data[curr] == '\n')
				break;

		/* Compute the line size */
		line_size = curr - line_start;

		/* Check if this line contains the expected tag */
		if ((line_size == (strlen(tag_name) + tag_len)) &&
		    (!strncmp(&file_data[line_start], tag_name, strlen(tag_name))))
			return (&file_data[line_start + strlen(tag_name)]);

		/* Move to next line */
		line_start = curr + 1;
	}

	/* Tag not found */
	return NULL;
}

static rs_ret net_get_mac_addr(struct rs_c_if *c_if, const char *filename, u8 *mac_addr)
{
	rs_ret ret = RS_FAIL;
	struct device *dev_if = NULL;
	const struct firmware *default_mac = NULL;
	const u8 *temp_mac = NULL;

	if (c_if) {
		ret = rs_net_ctrl_get_fw_mac_addr(c_if, mac_addr);

		if (ret == RS_SUCCESS) {
			RS_INFO("OTP MAC Address : " MAC_ADDRESS_STR "\n", MAC_ADDR_ARRAY(mac_addr));
		} else {
			RS_WARN("OTP MAC address not assigned.\n");
			RS_WARN("Using System MAC address from /lib/firmware/default_mac.ini\n");

			dev_if = (struct device *)rs_c_if_get_dev_if(c_if);

			// If MAC address is not provided by OTP,
			// try to get it from config file (default_mac.ini)
			if ((request_firmware(&default_mac, filename, dev_if) == 0)) {
				/* Get MAC Address from default_mac.ini*/
				temp_mac = find_tag(default_mac->data, default_mac->size,
						    "MAC_ADDR=", NET_MAC_ADDR_LEN);
			} else {
				RS_WARN("Failed to get %s%s\n", "/lib/firmware/", filename);
			}

			net_set_default_mac_addr(temp_mac, mac_addr);

			if (!temp_mac) {
				RS_WARN("System MAC Address not found in %s\n", filename);
				RS_INFO("Using Random MAC Address : " MAC_ADDRESS_STR "\n",
					MAC_ADDR_ARRAY(mac_addr));
			} else {
				RS_INFO("System MAC Address : " MAC_ADDRESS_STR "\n",
					MAC_ADDR_ARRAY(mac_addr));
			}

			if (default_mac) {
				release_firmware(default_mac);
			}

			ret = RS_SUCCESS;
		}
	}

	return ret;
}

static void net_reg_notifier(struct wiphy *wiphy, struct regulatory_request *request)
{
	struct rs_net_cfg80211_priv *net_priv = wiphy_priv(wiphy);
	struct rs_c_if *c_if = rs_net_priv_get_c_if(net_priv);

	if (c_if) {		
		rs_k_memcpy(net_priv->phy_config.country_code, request->alpha2, sizeof(request->alpha2));
		rs_net_dfs_set_domain(&net_priv->dfs, request->dfs_region, RS_DFS_RIU);
		(void)rs_net_ctrl_me_chan_config(c_if);
	}
}

u8 *net_create_beacon(struct rs_net_beacon_info *bcn, struct cfg80211_beacon_data *new)
{
	u8 *buf = NULL, *pos = NULL;

	if (new->head) {
		u8 *head = rs_k_calloc(new->head_len);

		if (!head)
			return NULL;

		if (bcn->head)
			rs_k_free(bcn->head);

		bcn->head = head;
		bcn->head_len = new->head_len;
		rs_k_memcpy(bcn->head, new->head, new->head_len);
	}
	if (new->tail) {
		u8 *tail = rs_k_calloc(new->tail_len);

		if (!tail)
			return NULL;

		if (bcn->tail)
			rs_k_free(bcn->tail);

		bcn->tail = tail;
		bcn->tail_len = new->tail_len;
		rs_k_memcpy(bcn->tail, new->tail, new->tail_len);
	}

	if (!bcn->head)
		return NULL;

	bcn->tim_len = 6;
	bcn->len = bcn->head_len + bcn->tail_len + bcn->ies_len + bcn->tim_len;

	buf = rs_k_calloc(bcn->len);
	if (!buf)
		return NULL;

	// Build the beacon buffer
	pos = buf;
	rs_k_memcpy(pos, bcn->head, bcn->head_len);
	pos += bcn->head_len;
	*pos++ = WLAN_EID_TIM;
	*pos++ = 4;
	*pos++ = 0;
	*pos++ = bcn->dtim_period;
	*pos++ = 0;
	*pos++ = 0;
	if (bcn->tail) {
		rs_k_memcpy(pos, bcn->tail, bcn->tail_len);
		pos += bcn->tail_len;
	}
	if (bcn->ies) {
		rs_k_memcpy(pos, bcn->ies, bcn->ies_len);
	}

	return buf;
}

static void net_remove_beacon(struct rs_net_beacon_info *bcn)
{
	if (bcn->head) {
		rs_k_free(bcn->head);
		bcn->head = NULL;
	}
	bcn->head_len = 0;

	if (bcn->tail) {
		rs_k_free(bcn->tail);
		bcn->tail = NULL;
	}
	bcn->tail_len = 0;

	if (bcn->ies) {
		rs_k_free(bcn->ies);
		bcn->ies = NULL;
	}
	bcn->ies_len = 0;
	bcn->tim_len = 0;
	bcn->dtim_period = 0;
	bcn->len = 0;
}

static void net_csa_remove(struct rs_net_vif_priv *vif)
{
	struct rs_net_csa_info *csa = vif->ap.csa;

	if (!csa)
		return;

	net_remove_beacon(&csa->bcn);
	if (csa)
		rs_k_free(csa);
	vif->ap.csa = NULL;
}

static void net_csa_work(struct work_struct *ws)
{
	struct rs_net_csa_info *csa = container_of(ws, struct rs_net_csa_info, work);
	struct rs_net_vif_priv *vif_priv = csa->vif;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct net_device *ndev = NULL;
	struct wireless_dev *wdev = NULL;
	struct rs_c_if *c_if = NULL;

	int error = csa->status;

	net_priv = rs_vif_priv_get_net_priv(vif_priv);
	ndev = rs_vif_priv_get_ndev(vif_priv);
	wdev = rs_vif_priv_get_wdev(vif_priv);
	c_if = rs_net_priv_get_c_if(net_priv);

	if (!error) {
		error = rs_net_ctrl_change_beacon_req(c_if, vif_priv->vif_index, (u8 *)&csa->buf[0],
						      csa->bcn.len, csa->bcn.head_len, csa->bcn.tim_len,
						      NULL);
	}

	if (error) {
		cfg80211_stop_iface(net_priv->wiphy, wdev, GFP_KERNEL);
	} else {
		mutex_lock(&wdev->mtx);
		__acquire(&wdev->mtx);
		(void)rs_net_cfg80211_unset_chaninfo(vif_priv);
		(void)rs_net_cfg80211_set_chaninfo(vif_priv, csa->ch_idx, &csa->chandef);
		if (net_priv->chaninfo_index == csa->ch_idx) {
			rs_net_dfs_detection_enabled_on_cur_channel(net_priv);
		}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 3, 0)
		cfg80211_ch_switch_notify(ndev, &csa->chandef, 0 /*link_id*/, 0 /*punct_bitmap*/);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0)
		cfg80211_ch_switch_notify(ndev, &csa->chandef, 0 /*link_id*/);
#else
		cfg80211_ch_switch_notify(ndev, &csa->chandef);
#endif
		mutex_unlock(&wdev->mtx);
		__release(&wdev->mtx);
	}

	net_csa_remove(vif_priv);
}

void rs_net_cac_stop(struct rs_net_dfs_priv *dfs)
{
	struct rs_net_cfg80211_priv *net_priv = container_of(dfs, struct rs_net_cfg80211_priv, dfs);
	struct rs_c_if *c_if = NULL;
	struct rs_net_chan_info *chan_info = NULL;

	if (!dfs->cac_vif) {
		return;
	}

	if (!net_priv) {
		return;
	}

	c_if = rs_net_priv_get_c_if(net_priv);

	if (cancel_delayed_work(&dfs->cac_work)) {
		chan_info = rs_net_cfg80211_get_chaninfo(dfs->cac_vif);
		if (chan_info != NULL) {
			cfg80211_cac_event(dfs->cac_vif->ndev, &(chan_info->chan_def),
					   NL80211_RADAR_CAC_ABORTED, GFP_KERNEL);
		}
		rs_net_ctrl_cac_stop_req(c_if, dfs->cac_vif);
		(void)rs_net_cfg80211_unset_chaninfo(dfs->cac_vif);
	}

	dfs->cac_vif = NULL;
}

static int net_station_info_fill(struct rs_net_sta_priv *sta, struct station_info *sinfo,
				 struct rs_net_vif_priv *vif_priv)
{
	struct rs_net_sta_stats *stats = &sta->stats;
	struct net_device *ndev = rs_vif_priv_get_ndev(vif_priv);
	u16 format_mod = 0;

	// Generic info
	sinfo->generation = vif_priv->generation;

	sinfo->inactive_time = jiffies_to_msecs(jiffies - stats->last_acttive_time);
	sinfo->rx_bytes = ndev->stats.rx_bytes;
	sinfo->rx_packets = ndev->stats.rx_packets;
	sinfo->tx_bytes = ndev->stats.tx_bytes;
	sinfo->tx_packets = ndev->stats.tx_packets;

	sinfo->signal = stats->last_rx_data_ext.rssi1;

	switch (stats->last_rx_data_ext.ch_bw) {
	case 0:
		sinfo->rxrate.bw = RATE_INFO_BW_20;
		break;
	case 1:
		sinfo->rxrate.bw = RATE_INFO_BW_40;
		break;
	case 2:
		sinfo->rxrate.bw = RATE_INFO_BW_80;
		break;
	case 3:
		sinfo->rxrate.bw = RATE_INFO_BW_160;
		break;
	default:
		sinfo->rxrate.bw = RATE_INFO_BW_HE_RU;
		break;
	}

	if (stats->last_rx_data_ext.pre_type) {
		format_mod = stats->last_rx_data_ext.format_mod + 1;
	} else
		format_mod = stats->last_rx_data_ext.format_mod;

	if (stats->last_stats.format_mod > 1 && format_mod < 2)
		format_mod = stats->last_stats.format_mod;

	switch (format_mod) {
	case FORMATMOD_NON_HT:
	case FORMATMOD_NON_HT_DUP_OFDM:
		sinfo->rxrate.flags = 0;
		sinfo->rxrate.legacy = legacy_rate_table[stats->last_rx_data_ext.leg_rate].rate;
		break;
	case FORMATMOD_HT_MF:
	case FORMATMOD_HT_GF:
		sinfo->rxrate.flags = RATE_INFO_FLAGS_MCS;
		if (stats->last_stats.ht.short_gi)
			sinfo->rxrate.flags |= RATE_INFO_FLAGS_SHORT_GI;
		sinfo->rxrate.mcs = stats->last_stats.ht.mcs;
		break;
	case FORMATMOD_VHT:
		sinfo->rxrate.flags = RATE_INFO_FLAGS_VHT_MCS;
		if (stats->last_stats.vht.short_gi)
			sinfo->rxrate.flags |= RATE_INFO_FLAGS_SHORT_GI;
		sinfo->rxrate.mcs = stats->last_stats.vht.mcs;
		sinfo->rxrate.nss = stats->last_stats.vht.nss + 1;
		break;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0)
	case FORMATMOD_HE_MU:
		// sinfo->rxrate.he_ru_alloc = stats->last_rx_data_ext.he.ru_size;
		sinfo->rxrate.he_ru_alloc = stats->last_stats.he.ru_size;
		fallthrough;
	case FORMATMOD_HE_SU:
	case FORMATMOD_HE_ER:
	case FORMATMOD_HE_TB:
		sinfo->rxrate.flags = RATE_INFO_FLAGS_HE_MCS;
		sinfo->rxrate.mcs = stats->last_stats.he.mcs;
		sinfo->rxrate.nss = stats->last_stats.he.nss;
		sinfo->rxrate.he_gi = stats->last_stats.he.gi_type;
		sinfo->rxrate.he_dcm = stats->last_stats.he.dcm;
		if (!sinfo->rxrate.nss)
			sinfo->rxrate.nss = 1;
		break;
#endif
	default:
		return -EINVAL;
	}

	sinfo->filled = (BIT(NL80211_STA_INFO_INACTIVE_TIME) | BIT(NL80211_STA_INFO_RX_BYTES64) |
			 BIT(NL80211_STA_INFO_TX_BYTES64) | BIT(NL80211_STA_INFO_RX_PACKETS) |
			 BIT(NL80211_STA_INFO_TX_PACKETS) | BIT(NL80211_STA_INFO_SIGNAL) |
			 BIT(NL80211_STA_INFO_RX_BITRATE));

	return 0;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 1, 0)
static int net_connect_param_fill_rsn(const struct element *rsne, struct cfg80211_connect_params *sme)
{
	int len = rsne->datalen;
	int clen;
	const u8 *pos = rsne->data;

	if (len < 8)
		return 1;

	sme->crypto.control_port_no_encrypt = false;
	sme->crypto.control_port = true;
	sme->crypto.control_port_ethertype = cpu_to_be16(ETH_P_PAE);

	pos += 2;
	sme->crypto.cipher_group = ntohl(*((u32 *)pos));
	pos += 4;
	clen = le16_to_cpu(*((u16 *)pos)) * 4;
	pos += 2;
	len -= 8;
	if (len < clen + 2)
		return 1;
	// only need one cipher suite
	sme->crypto.n_ciphers_pairwise = 1;
	sme->crypto.ciphers_pairwise[0] = ntohl(*((u32 *)pos));
	pos += clen;
	len -= clen;

	// no need for AKM
	clen = le16_to_cpu(*((u16 *)pos)) * 4;
	pos += 2;
	len -= 2;
	if (len < clen)
		return 1;
	pos += clen;
	len -= clen;

	if (len < 4)
		return 0;

	pos += 2;
	clen = le16_to_cpu(*((u16 *)pos)) * 16;
	len -= 4;
	if (len > clen)
		sme->mfp = NL80211_MFP_REQUIRED;

	return 0;
}
#endif

static s32 net_del_station(struct rs_c_if *c_if, struct rs_net_cfg80211_priv *net_priv,
			   struct rs_net_vif_priv *vif_priv, const u8 *mac)
{
	rs_ret ret = RS_FAIL;
	struct rs_net_vif_priv *vlan_vif = NULL;
	struct rs_net_sta_priv *cur = NULL, *tmp = NULL;
	bool all_sta = FALSE;
	s32 found = 0;

	RS_TRACE(RS_FN_ENTRY_STR);

	if ((mac == NULL) || ((mac != NULL) && is_broadcast_ether_addr(mac))) {
		all_sta = TRUE;
	}

	list_for_each_entry_safe(cur, tmp, &vif_priv->ap.sta_list, list) {
		if ((all_sta == TRUE) || ((mac != NULL) && ether_addr_equal(cur->mac_addr, mac))) {
			cur->valid = FALSE;

			if (cur->vif_idx != cur->vlan_idx) {
				RS_DBG("P:%s[%d]:vif[%d]:vlan_idx[%d]\n", __func__, __LINE__, cur->vif_idx,
				       cur->vlan_idx);

				vlan_vif = net_priv->vif_table[cur->vlan_idx];
				if (vlan_vif && vlan_vif->up) {
					if ((RS_NET_WDEV_IF_TYPE(vlan_vif) == NL80211_IFTYPE_AP_VLAN) &&
					    vlan_vif->use_4addr) {
						vlan_vif->ap_vlan.sta_4a = NULL;
					} else {
						RS_WARN("Attempt to delete sta belonging to a VLAN other than AP_VLAN 4addr");
					}
				}
			}

			ret = rs_net_ctrl_del_station_req(c_if, cur->sta_idx, FALSE);
			if (ret != RS_SUCCESS) {
				found = -EPIPE;
				break;
			}

			list_del(&cur->list);
			vif_priv->generation++;
			found++;

			if (all_sta == FALSE) {
				break;
			}
		}
	}

	return found;
}

static struct wireless_dev *net_add_virtual_iface(struct rs_net_cfg80211_priv *net_priv, const u8 *name,
						  u8 name_assign_type, enum nl80211_iftype type,
						  struct vif_params *params, bool locked)
{
	rs_ret ret = RS_FAIL;
	struct wireless_dev *wdev = NULL;
	struct net_device *ndev = NULL;
	struct rs_net_vif_priv *vif_priv = NULL;
	s32 min_idx = 0, max_idx = 0;
	s32 vif_idx = -1;
	s32 i = 0;
	u8 temp_mac[ETH_ADDR_LEN] = { 0 };
	s32 err;

	RS_TRACE(RS_FN_ENTRY_STR);

	// Look for an available VIF
	if (type == NL80211_IFTYPE_AP_VLAN) {
		min_idx = RS_NET_VIF_DEV_MAX;
		max_idx = RS_NET_VIF_MAX;
	} else {
		min_idx = 0;
		max_idx = RS_NET_VIF_DEV_MAX;
	}

	for (i = min_idx; i < max_idx; i++) {
		if (net_priv->avail_idx_map & RS_BIT(i)) {
			vif_idx = i;
			break;
		}
	}

	if (vif_idx >= 0) {
		ret = RS_SUCCESS;

		list_for_each_entry(vif_priv, &net_priv->vifs, list) {
			// Check if monitor interface already exists or type is monitor
			if ((RS_NET_WDEV_IF_TYPE(vif_priv) == NL80211_IFTYPE_MONITOR) ||
			    (type == NL80211_IFTYPE_MONITOR)) {
				RS_ERR("Monitor+Data interface support (MON_DATA) disabled\n");
				ret = RS_FAIL;
			}
		}
	}

	RS_DBG("P:%s[%d]:vif_idx[%d]:name[%s][%d]:r[%d]\n", __func__, __LINE__, vif_idx, name,
	       name_assign_type, ret);

	if (ret == RS_SUCCESS) {
		ndev = alloc_netdev_mqs(sizeof(struct rs_net_vif_priv), name, name_assign_type, net_dev_init,
					RS_NET_NDEV_TXQ, 1);

		if (ndev) {
			vif_priv = netdev_priv(ndev);
			ndev->ieee80211_ptr = rs_vif_priv_get_wdev(vif_priv);
			vif_priv->wdev.wiphy = rs_net_priv_get_wiphy(net_priv);
			ret = rs_vif_priv_set_net_priv(vif_priv, net_priv);

			(void)rs_vif_priv_set_ndev(vif_priv, ndev);

			vif_priv->drv_vif_index = vif_idx;
			SET_NETDEV_DEV(ndev, wiphy_dev(vif_priv->wdev.wiphy));
			vif_priv->wdev.netdev = ndev;
			vif_priv->wdev.iftype = type;
			vif_priv->up = FALSE;
			vif_priv->ch_index = RS_NET_CH_INIT_VALUE;
			vif_priv->generation = 0;

			switch (type) {
			case NL80211_IFTYPE_STATION:
			case NL80211_IFTYPE_P2P_CLIENT:
				vif_priv->sta.flags = 0;
				vif_priv->sta.ap = NULL;
				vif_priv->sta.tdls_sta = NULL;
				vif_priv->sta.ft_assoc_ies = NULL;
				vif_priv->sta.ft_assoc_ies_len = 0;
				break;
			case NL80211_IFTYPE_MESH_POINT:
				INIT_LIST_HEAD(&vif_priv->ap.mpath_list);
				INIT_LIST_HEAD(&vif_priv->ap.proxy_list);
				vif_priv->ap.mesh_pm = NL80211_MESH_POWER_ACTIVE;
				vif_priv->ap.next_mesh_pm = NL80211_MESH_POWER_ACTIVE;
				fallthrough;
			case NL80211_IFTYPE_AP:
			case NL80211_IFTYPE_P2P_GO:
				INIT_LIST_HEAD(&vif_priv->ap.sta_list);
				rs_k_memset(&vif_priv->ap.bcn, 0, sizeof(vif_priv->ap.bcn));
				vif_priv->ap.flags = 0;
				vif_priv->ap.ap_isolate = 0;
				break;
			case NL80211_IFTYPE_AP_VLAN: {
				struct rs_net_vif_priv *master_vif;
				bool found = false;

				list_for_each_entry(master_vif, &net_priv->vifs, list) {
					if ((RS_NET_WDEV_IF_TYPE(master_vif) == NL80211_IFTYPE_AP) &&
					    !(!rs_k_memcmp(master_vif->ndev->dev_addr, params->macaddr,
							   ETH_ADDR_LEN))) {
						found = true;
						break;
					}
				}

				if (found == false) {
					ret = RS_FAIL;
					break;
				}

				vif_priv->ap_vlan.master = master_vif;
				vif_priv->ap_vlan.sta_4a = NULL;
				break;
			}
			case NL80211_IFTYPE_MONITOR:
				ndev->type = ARPHRD_IEEE80211_RADIOTAP;
				ret = rs_net_dev_set_monitor_ops(ndev);
				break;
			default:
				break;
			}

			(void)rs_k_memcpy(temp_mac, net_priv->wiphy->perm_addr, ETH_ADDR_LEN);

			if (type != NL80211_IFTYPE_AP_VLAN) {
				temp_mac[5] ^= vif_idx;
			}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 188) && LINUX_VERSION_CODE < KERNEL_VERSION(5, 11, 0)) || \
	LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
			dev_addr_set(ndev, temp_mac);
#else
			(void)rs_k_memcpy(ndev->dev_addr, temp_mac, ETH_ADDR_LEN);
#endif

			if (params) {
				vif_priv->use_4addr = params->use_4addr;
				ndev->ieee80211_ptr->use_4addr = params->use_4addr;
			} else {
				vif_priv->use_4addr = false;
			}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 12, 0)
			if (locked)
				err = cfg80211_register_netdevice(ndev);
			else
#endif
				err = register_netdevice(ndev);

			if (err == 0) {
				// spin_lock_bh(&rs_hw->cb_lock);
				list_add_tail(&vif_priv->list, &net_priv->vifs);
				// spin_unlock_bh(&rs_hw->cb_lock);
				net_priv->avail_idx_map &= (~(RS_BIT(vif_idx)));

				wdev = rs_vif_priv_get_wdev(vif_priv);
			} else {
				ret = RS_FAIL;
			}
		}
	}

	if (ret != RS_SUCCESS) {
		if (ndev) {
			free_netdev(ndev);
		}
		wdev = NULL;
	}

	return wdev;
}

static struct wireless_dev *net_cfg80211_add_virtual_iface(struct wiphy *wiphy, const char *name,
							   unsigned char name_assign_type,
							   enum nl80211_iftype type,
							   struct vif_params *params)
{
	struct rs_net_cfg80211_priv *net_priv = wiphy_priv(wiphy);
	struct wireless_dev *wdev = NULL;

	RS_TRACE(RS_FN_ENTRY_STR);

	wdev = net_add_virtual_iface(net_priv, name, name_assign_type, type, params, true);

	if (!wdev) {
		return ERR_PTR(-EINVAL);
	}

	return wdev;
}

static int net_cfg80211_del_virtual_iface(struct wiphy *wiphy, struct wireless_dev *wdev)
{
	s32 ret = 0;
	struct rs_c_if *c_if = NULL;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct rs_net_vif_priv *vif_priv = NULL;
	struct net_device *ndev = wdev->netdev;

	RS_TRACE(RS_FN_ENTRY_STR);

	net_priv = wiphy_priv(wiphy);
	c_if = rs_net_priv_get_c_if(net_priv);
	vif_priv = netdev_priv(ndev);

	if (ndev->reg_state == NETREG_REGISTERED) {
		/* Will call ndo_close if interface is UP */
		(void)unregister_netdevice(ndev);
	}

	// TODO : spin_lock_bh(&(net_priv->cb_lock));
	list_del(&vif_priv->list);
	// TODO : spin_unlock_bh(&(net_priv->cb_lock));
	net_priv->avail_idx_map |= RS_BIT(vif_priv->vif_index);
	(void)rs_vif_priv_set_ndev(vif_priv, NULL);

	/* Clear the priv in adapter */
	ndev->ieee80211_ptr = NULL;

	return ret;
}

static int net_cfg80211_change_virtual_iface(struct wiphy *wiphy, struct net_device *ndev,
					     enum nl80211_iftype type, struct vif_params *params)
{
	s32 ret = 0;
	struct rs_c_if *c_if = NULL;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct rs_net_vif_priv *vif_priv = NULL;

	RS_TRACE(RS_FN_ENTRY_STR);

	net_priv = wiphy_priv(wiphy);
	c_if = rs_net_priv_get_c_if(net_priv);
	vif_priv = netdev_priv(ndev);

	if (vif_priv->up) {
		if ((vif_priv->wdev.iftype == type) && (type == NL80211_IFTYPE_STATION)) {
			if (params->use_4addr != -1) {
				vif_priv->use_4addr = params->use_4addr;
			}

			if (!is_zero_ether_addr(params->macaddr) &&
			    !ether_addr_equal(vif_priv->wdev.address, params->macaddr)) {
				return -EBUSY;
			}
		} else {
			return -EBUSY;
		}
	}

	if (type == NL80211_IFTYPE_MONITOR && (RS_NET_WDEV_IF_TYPE(vif_priv) != NL80211_IFTYPE_MONITOR)) {
		struct rs_net_vif_priv *vif_el = NULL;
		struct rs_net_vif_priv *tmp_vif_priv = NULL;

		rtnl_lock();
		list_for_each_entry_safe(vif_el, tmp_vif_priv, &net_priv->vifs, list) {
			// Check if data interface already exists
			if (vif_el != vif_priv && (RS_NET_WDEV_IF_TYPE(vif_priv) != NL80211_IFTYPE_MONITOR)) {
				wiphy_err(net_priv->wiphy,
					  "Monitor+Data interface support (MON_DATA) disabled\n");
				return -EIO;
			}
		}
		rtnl_unlock();
	} else if ((RS_NET_WDEV_IF_TYPE(vif_priv) == NL80211_IFTYPE_AP) && (type != NL80211_IFTYPE_AP)) {
		// Remove all stations
		(void)net_del_station(c_if, net_priv, vif_priv, NULL);
	}

	// Reset to default case (i.e. not monitor)
	ndev->type = ARPHRD_ETHER;
	ret = rs_net_dev_set_ops(ndev);
	if (ret != RS_SUCCESS)
		RS_ERR("Failed to set netdev ops\n");

	switch (type) {
	case NL80211_IFTYPE_STATION:
	case NL80211_IFTYPE_P2P_CLIENT:
		vif_priv->sta.flags = 0;
		vif_priv->sta.ap = NULL;
		vif_priv->sta.tdls_sta = NULL;
		vif_priv->sta.ft_assoc_ies = NULL;
		vif_priv->sta.ft_assoc_ies_len = 0;
		break;
	case NL80211_IFTYPE_AP:
	case NL80211_IFTYPE_P2P_GO:
		INIT_LIST_HEAD(&vif_priv->ap.sta_list);
		rs_k_memset(&vif_priv->ap.bcn, 0, sizeof(vif_priv->ap.bcn));
		vif_priv->ap.flags = 0;
		vif_priv->ap.ap_isolate = 0;
		break;
	case NL80211_IFTYPE_AP_VLAN:
		return -EPERM;
	case NL80211_IFTYPE_MONITOR:
		ndev->type = ARPHRD_IEEE80211_RADIOTAP;
		ret = rs_net_dev_set_monitor_ops(ndev);
		if (ret != RS_SUCCESS)
			RS_ERR("Failed to set monitor netdev ops\n");
		break;
	default:
		break;
	}

	vif_priv->generation = 0;
	vif_priv->wdev.iftype = type;
	if (params->use_4addr != -1) {
		vif_priv->use_4addr = params->use_4addr;
	}

	return ret;
}

static int net_cfg80211_scan(struct wiphy *wiphy, struct cfg80211_scan_request *request)
{
	s32 ret = 0;
	struct rs_c_if *c_if = NULL;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct rs_net_vif_priv *vif_priv = NULL;

	RS_TRACE(RS_FN_ENTRY_STR);

	net_priv = wiphy_priv(wiphy);
	c_if = rs_net_priv_get_c_if(net_priv);
	vif_priv = container_of(request->wdev, struct rs_net_vif_priv, wdev);

	if (rs_net_vif_is_up(vif_priv) != RS_SUCCESS) {
		return -EIO;
	}

	if (RS_NET_WDEV_IF_TYPE(vif_priv) == NL80211_IFTYPE_AP) {
		return -EOPNOTSUPP;
	}

	if (net_priv->scan_request != NULL) {
		return -EAGAIN;
	}

	ret = rs_net_ctrl_scan_start(c_if, request);

	if (ret == RS_SUCCESS) {
		net_priv->scan_request = request;
		c_if->core->scan = 1;
	}

	return ret;
}

static void net_cfg80211_scan_abort(struct wiphy *wiphy, struct wireless_dev *wdev)
{
	struct rs_c_if *c_if = NULL;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct rs_net_vif_priv *vif_priv = NULL;

	RS_TRACE(RS_FN_ENTRY_STR);

	net_priv = wiphy_priv(wiphy);
	c_if = rs_net_priv_get_c_if(net_priv);

	if ((wdev) && (c_if) && (net_priv) && (net_priv->scan_request)) {
		vif_priv = container_of(wdev, struct rs_net_vif_priv, wdev);
		if (vif_priv->up) {
			(void)rs_net_ctrl_scan_cancel(c_if, vif_priv);
		}
		c_if->core->scan = 0;
	}
}

static void net_save_ft_assoc_info(struct rs_net_vif_priv *vif_priv, struct cfg80211_connect_params *sme)
{
	int ies_len = sme->ie_len + sme->ssid_len + 2;
	u8 *pos;

	if (!vif_priv->sta.ft_assoc_ies) {
		if (!cfg80211_find_ie(WLAN_EID_MOBILITY_DOMAIN, sme->ie, sme->ie_len)) {
			return;
		}

		vif_priv->sta.ft_assoc_ies_len = ies_len;
		vif_priv->sta.ft_assoc_ies = rs_k_calloc(ies_len);
	} else if (vif_priv->sta.ft_assoc_ies_len < ies_len) {
		rs_k_free(vif_priv->sta.ft_assoc_ies);
		vif_priv->sta.ft_assoc_ies = rs_k_calloc(ies_len);
	}

	if (!vif_priv->sta.ft_assoc_ies)
		return;

	// Also save SSID (as an element) in the buffer
	pos = vif_priv->sta.ft_assoc_ies;
	*pos++ = WLAN_EID_SSID;
	*pos++ = sme->ssid_len;
	rs_k_memcpy(pos, sme->ssid, sme->ssid_len);
	pos += sme->ssid_len;
	rs_k_memcpy(pos, sme->ie, sme->ie_len);
	vif_priv->sta.ft_assoc_ies_len = ies_len;
}

static int net_cfg80211_connect(struct wiphy *wiphy, struct net_device *ndev,
				struct cfg80211_connect_params *sme)
{
	s32 ret = 0;
	struct rs_c_if *c_if = NULL;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct rs_net_vif_priv *vif_priv = NULL;
	struct rs_c_sm_connect_rsp *connect_rsp;

	RS_TRACE(RS_FN_ENTRY_STR);

	net_priv = wiphy_priv(wiphy);
	if (!net_priv)
		return -ENOMEM;
	c_if = rs_net_priv_get_c_if(net_priv);
	if (!c_if)
		return -ENOMEM;
	vif_priv = netdev_priv(ndev);
	if (!vif_priv)
		return -ENOMEM;

	if (rs_net_vif_is_up(vif_priv) != RS_SUCCESS) {
		return -EIO;
	}

	connect_rsp = &net_priv->cmd_rsp.connect_rsp;

	if ((sme->crypto.cipher_group == WLAN_CIPHER_SUITE_WEP40 ||
	     sme->crypto.cipher_group == WLAN_CIPHER_SUITE_WEP104) &&
	    (sme->key && sme->key_len > 0)) {
		struct key_params key_params;

		key_params.key = sme->key;
		key_params.seq = NULL;
		key_params.key_len = sme->key_len;
		key_params.seq_len = 0;
		key_params.cipher = sme->crypto.cipher_group;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
		net_cfg80211_add_key(wiphy, ndev, 0 /*link_id*/, sme->key_idx, false, NULL, &key_params);
#else
		net_cfg80211_add_key(wiphy, ndev, sme->key_idx, false, NULL, &key_params);
#endif
	}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 17, 0)
	else if ((sme->auth_type == NL80211_AUTHTYPE_SAE) &&
		 !(sme->flags & CONNECT_REQ_EXTERNAL_AUTH_SUPPORT)) {
		netdev_err(ndev, "Doesn't support SAE without external authentication\n");
		return -EINVAL;
	}
#endif

	ret = rs_net_ctrl_connect(c_if, ndev, sme, connect_rsp);

	if (connect_rsp->status == RS_SUCCESS) {
		net_save_ft_assoc_info(vif_priv, sme);
	}

	return ret;
}

static int net_cfg80211_disconnect(struct wiphy *wiphy, struct net_device *ndev, u16 reason_code)
{
	s32 ret = 0;
	struct rs_c_if *c_if = NULL;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct rs_net_vif_priv *vif_priv = NULL;

	RS_TRACE(RS_FN_ENTRY_STR);

	net_priv = wiphy_priv(wiphy);
	c_if = rs_net_priv_get_c_if(net_priv);
	vif_priv = netdev_priv(ndev);

	if (rs_net_vif_is_up(vif_priv) != RS_SUCCESS) {
		return -EIO;
	}

	if (net_priv->scan_request != NULL) {
		(void)rs_net_cfg80211_scan_stop_and_wait(c_if, vif_priv);
	}

#if CONFIG_DBG_STATS
	if (net_priv->dbg_stats.dbg_stats_enabled)
		rs_net_stats_deinit(net_priv);
#endif

	ret = rs_net_ctrl_disconnect_req(c_if, vif_priv, reason_code);

	return ret;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
static int net_cfg80211_add_key(struct wiphy *wiphy, struct net_device *ndev, int link_id, u8 key_index,
				bool pairwise, const u8 *mac_addr, struct key_params *params)
#else
static int net_cfg80211_add_key(struct wiphy *wiphy, struct net_device *ndev, u8 key_index, bool pairwise,
				const u8 *mac_addr, struct key_params *params)
#endif
{
	s32 ret = 0;
	struct rs_c_if *c_if = NULL;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct rs_net_vif_priv *vif_priv = NULL;
	struct rs_net_sta_priv *sta = NULL;
	struct rs_net_key_info *key = NULL;
	struct rs_c_key_add_rsp *key_add_rsp = NULL;
	u8 cipher_type = 0;

	RS_TRACE(RS_FN_ENTRY_STR);

	if (key_index >= RS_NET_KEYS_MAX) {
		RS_ERR("Invalid key index\n");
		return -EINVAL;
	}

	net_priv = wiphy_priv(wiphy);
	if (!net_priv)
		return -ENOMEM;
	c_if = rs_net_priv_get_c_if(net_priv);
	if (!c_if)
		return -ENOMEM;
	vif_priv = netdev_priv(ndev);
	if (!vif_priv)
		return -ENOMEM;

	if (rs_net_vif_is_up(vif_priv) != RS_SUCCESS) {
		return -EIO;
	}

	if (mac_addr) {
		sta = rs_net_priv_get_sta_info_from_mac(net_priv, mac_addr);
		if (!sta) {
			RS_ERR("No STA found with MAC address\n");
			return -EINVAL;
		}
		key = &sta->key;
	} else {
		key = &vif_priv->key[key_index];
	}

	switch (params->cipher) {
	case WLAN_CIPHER_SUITE_WEP40:
		cipher_type = RS_CIPHER_WEP40;
		break;
	case WLAN_CIPHER_SUITE_WEP104:
		cipher_type = RS_CIPHER_WEP104;
		break;
	case WLAN_CIPHER_SUITE_TKIP:
		cipher_type = RS_CIPHER_TKIP;
		break;
	case WLAN_CIPHER_SUITE_CCMP:
		cipher_type = RS_CIPHER_CCMP;
		break;
	case WLAN_CIPHER_SUITE_AES_CMAC:
		cipher_type = RS_CIPHER_AES_CMAC_128;
		break;
	case WLAN_CIPHER_SUITE_SMS4: {
		u8 tmp, *key = (u8 *)params->key;
		int i = 0;

		cipher_type = RS_CIPHER_SMS4;

		for (i = 0; i < RS_NET_WPI_SUBKEY_LEN / 2; i++) {
			tmp = key[i];
			key[i] = key[RS_NET_WPI_SUBKEY_LEN - 1 - i];
			key[RS_NET_WPI_SUBKEY_LEN - 1 - i] = tmp;
		}
		for (i = 0; i < RS_NET_WPI_SUBKEY_LEN / 2; i++) {
			tmp = key[i + RS_NET_WPI_SUBKEY_LEN];
			key[i + RS_NET_WPI_SUBKEY_LEN] = key[RS_NET_WPI_KEY_LEN - 1 - i];
			key[RS_NET_WPI_KEY_LEN - 1 - i] = tmp;
		}
		break;
	}
	case WLAN_CIPHER_SUITE_GCMP:
		cipher_type = RS_CIPHER_GCMP_128;
		break;
	case WLAN_CIPHER_SUITE_GCMP_256:
		cipher_type = RS_CIPHER_GCMP_256;
		break;
	case WLAN_CIPHER_SUITE_CCMP_256:
		cipher_type = RS_CIPHER_CCMP_256;
		break;
	default:
		RS_ERR("Unsupported cipher\n");
		return -EINVAL;
	}

	key_add_rsp = &net_priv->cmd_rsp.key_add_rsp;

	ret = rs_net_ctrl_add_key_req(c_if, vif_priv->vif_index, (sta ? sta->sta_idx : 0xFF), pairwise,
				      (u8 *)params->key, params->key_len, key_index, cipher_type,
				      key_add_rsp);

	if (ret == RS_SUCCESS) {
		key->key_index = key_add_rsp->hw_key_index;
	}

	return ret;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
static int net_cfg80211_get_key(struct wiphy *wiphy, struct net_device *ndev, int link_id, u8 key_index,
				bool pairwise, const u8 *mac_addr, void *cookie,
				void (*callback)(void *cookie, struct key_params *))
#else
static int net_cfg80211_get_key(struct wiphy *wiphy, struct net_device *ndev, u8 key_index, bool pairwise,
				const u8 *mac_addr, void *cookie,
				void (*callback)(void *cookie, struct key_params *))
#endif
{
	s32 ret = -1;
	struct rs_c_if *c_if = NULL;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct rs_net_vif_priv *vif_priv = NULL;

	RS_TRACE(RS_FN_ENTRY_STR);

	net_priv = wiphy_priv(wiphy);
	c_if = rs_net_priv_get_c_if(net_priv);
	vif_priv = netdev_priv(ndev);

	if (rs_net_vif_is_up(vif_priv) != RS_SUCCESS) {
		ret = -EIO;
	}

	return ret;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
static int net_cfg80211_del_key(struct wiphy *wiphy, struct net_device *ndev, int link_id, u8 key_index,
				bool pairwise, const u8 *mac_addr)
#else
static int net_cfg80211_del_key(struct wiphy *wiphy, struct net_device *ndev, u8 key_index, bool pairwise,
				const u8 *mac_addr)
#endif
{
	s32 ret = 0;
	struct rs_c_if *c_if = NULL;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct rs_net_vif_priv *vif_priv = NULL;
	struct rs_net_sta_priv *sta = NULL;
	struct rs_net_key_info *key = NULL;

	RS_TRACE(RS_FN_ENTRY_STR);

	if (key_index >= RS_NET_KEYS_MAX) {
		RS_ERR("Invalid key index\n");
		return -EINVAL;
	}

	net_priv = wiphy_priv(wiphy);
	if (!net_priv)
		return -ENOMEM;
	c_if = rs_net_priv_get_c_if(net_priv);
	if (!c_if)
		return -ENOMEM;
	vif_priv = netdev_priv(ndev);
	if (!vif_priv)
		return -ENOMEM;

	if (rs_net_vif_is_up(vif_priv) != RS_SUCCESS) {
		return -EIO;
	}

	if (mac_addr) {
		sta = rs_net_priv_get_sta_info_from_mac(net_priv, mac_addr);
		if (!sta)
			return -EINVAL;
		key = &sta->key;
	} else
		key = &vif_priv->key[key_index];

	ret = rs_net_ctrl_del_key_req(c_if, key->key_index);

	return ret;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
static int net_cfg80211_set_default_key(struct wiphy *wiphy, struct net_device *ndev, int link_id,
					u8 key_index, bool unicast, bool multicast)
#else
static int net_cfg80211_set_default_key(struct wiphy *wiphy, struct net_device *ndev, u8 key_index,
					bool unicast, bool multicast)
#endif
{
	s32 ret = 0;
	struct rs_c_if *c_if = NULL;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct rs_net_vif_priv *vif_priv = NULL;

	net_priv = wiphy_priv(wiphy);
	c_if = rs_net_priv_get_c_if(net_priv);
	vif_priv = netdev_priv(ndev);

	if (rs_net_vif_is_up(vif_priv) != RS_SUCCESS) {
		return -EIO;
	}

	RS_TRACE(RS_FN_ENTRY_STR);

	return ret;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
static int net_cfg80211_set_default_mgmt_key(struct wiphy *wiphy, struct net_device *ndev, int link_id,
					     u8 key_index)
#else
static int net_cfg80211_set_default_mgmt_key(struct wiphy *wiphy, struct net_device *ndev, u8 key_index)
#endif
{
	s32 ret = 0;

	RS_TRACE(RS_FN_ENTRY_STR);

	return ret;
}

static int net_cfg80211_add_station(struct wiphy *wiphy, struct net_device *ndev, const u8 *mac,
				    struct station_parameters *params)
{
	s32 ret = 0;
	struct rs_c_if *c_if = NULL;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct rs_net_vif_priv *vif_priv = NULL;
	struct rs_c_sta_add_rsp *sta_add_rsp = NULL;

	RS_TRACE(RS_FN_ENTRY_STR);

	net_priv = wiphy_priv(wiphy);
	c_if = rs_net_priv_get_c_if(net_priv);
	vif_priv = netdev_priv(ndev);

	if (params->sta_flags_set & BIT(NL80211_STA_FLAG_TDLS_PEER))
		return 0;

	sta_add_rsp = &net_priv->cmd_rsp.sta_add_rsp;

	ret = rs_c_ctrl_add_station_req(c_if, params, mac, vif_priv->vif_index, sta_add_rsp);

	if (ret == RS_SUCCESS) {
		switch (RS_GET_DEV_RET(sta_add_rsp->status)) {
		case RS_SUCCESS: {
			struct rs_net_sta_priv *sta =
				rs_net_priv_get_sta_info(net_priv, sta_add_rsp->sta_idx);

			sta->aid = params->aid;

			sta->sta_idx = sta_add_rsp->sta_idx;
			sta->ch_idx = vif_priv->ch_index;
			sta->vif_idx = vif_priv->vif_index;
			sta->vlan_idx = sta->vif_idx;
			sta->qos = (params->sta_flags_set & BIT(NL80211_STA_FLAG_WME)) != 0;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0)
			sta->ht = params->link_sta_params.ht_capa ? 1 : 0;
			sta->vht = params->link_sta_params.vht_capa ? 1 : 0;
			sta->he = params->link_sta_params.he_capa ? 1 : 0;
#else
			sta->ht = params->ht_capa ? 1 : 0;
			sta->vht = params->vht_capa ? 1 : 0;
			sta->he = params->he_capa ? 1 : 0;
#endif
			sta->acm = 0;
			sta->listen_interval = params->listen_interval;
			rs_k_memcpy(sta->mac_addr, mac, ETH_ADDR_LEN);
			// spin_lock_bh(&net_priv->cb_lock); //TODO
			list_add_tail(&sta->list, &vif_priv->ap.sta_list);
			vif_priv->generation++;
			sta->valid = true;
			// spin_unlock_bh(&net_priv->cb_lock); //TODO
		} break;
		default:
			ret = -EBUSY;
			break;
		}
	}

	return ret;
}

static int net_cfg80211_change_station(struct wiphy *wiphy, struct net_device *ndev, const u8 *mac,
				       struct station_parameters *params)
{
	s32 ret = 0;
	struct rs_c_if *c_if = NULL;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct rs_net_vif_priv *vif_priv = NULL;
	struct rs_net_sta_priv *sta = NULL;

	RS_TRACE(RS_FN_ENTRY_STR);

	net_priv = wiphy_priv(wiphy);
	c_if = rs_net_priv_get_c_if(net_priv);
	vif_priv = netdev_priv(ndev);

	sta = rs_net_priv_get_sta_info_from_mac(net_priv, mac);
	if (!sta) {
		return -EINVAL; // TODO : tdls case
	}

	if (params->sta_flags_mask & BIT(NL80211_STA_FLAG_AUTHORIZED)) {
		bool authorized = ((params->sta_flags_set & BIT(NL80211_STA_FLAG_AUTHORIZED)) != 0);

		rs_net_ctrl_port_control_req(c_if, authorized, sta->sta_idx);
	}

	// TODO : mesh case

	if (params->vlan) {
		u8 vif_idx;

		vif_idx = vif_priv->vif_index;

		if (sta->vlan_idx != vif_idx) {
			struct rs_net_vif_priv *old_vif_priv = NULL;

			old_vif_priv = net_priv->vif_table[sta->vlan_idx];
			sta->vlan_idx = vif_idx;

			if ((RS_NET_WDEV_IF_TYPE(vif_priv) == NL80211_IFTYPE_AP_VLAN) &&
			    vif_priv->use_4addr) {
				WARN((vif_priv->ap_vlan.sta_4a),
				     "4A AP_VLAN interface with more than one sta");
				vif_priv->ap_vlan.sta_4a = sta;
			}

			if ((RS_NET_WDEV_IF_TYPE(old_vif_priv) == NL80211_IFTYPE_AP_VLAN) &&
			    old_vif_priv->use_4addr) {
				old_vif_priv->ap_vlan.sta_4a = NULL;
			}
		}
	}

	return ret;
}

static int net_cfg80211_mgmt_tx(struct wiphy *wiphy, struct wireless_dev *wdev,
				struct cfg80211_mgmt_tx_params *params, u64 *cookie)
{
	s32 ret = 0;
	struct rs_c_if *c_if = NULL;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct rs_net_vif_priv *vif_priv = NULL;
	struct rs_net_sta_priv *sta = NULL;
	struct net_device *ndev = NULL;
	struct ieee80211_mgmt *mgmt = NULL;
	struct rs_net_chan_info *chan_info = NULL;
	bool ap = false;
	bool offchan = false;

	RS_TRACE(RS_FN_ENTRY_STR);

	if (!wdev)
		return -EINVAL;

	ndev = wdev->netdev;
	if (!ndev)
		return -EINVAL;

	if (!params)
		return -EINVAL;

	mgmt = (struct ieee80211_mgmt *)params->buf;

	if (!mgmt)
		return -EINVAL;

	net_priv = wiphy_priv(wiphy);
	c_if = rs_net_priv_get_c_if(net_priv);
	if (!c_if)
		return -EINVAL;
	vif_priv = netdev_priv(ndev);
	if (!vif_priv)
		return -EINVAL;

	switch (RS_NET_WDEV_IF_TYPE(vif_priv)) {
	case NL80211_IFTYPE_AP_VLAN:
		vif_priv = vif_priv->ap_vlan.master;
		fallthrough;
	case NL80211_IFTYPE_AP:
	case NL80211_IFTYPE_P2P_GO:
		ap = true;
		break;
	case NL80211_IFTYPE_STATION:
	case NL80211_IFTYPE_P2P_CLIENT:
	default:
		break;
	}

	if (ap) {
		if (ieee80211_is_assoc_resp(mgmt->frame_control) || ieee80211_is_auth(mgmt->frame_control)) {
			cfg80211_mgmt_tx_status(wdev, *cookie, params->buf, params->len, true, GFP_ATOMIC);
		}
	}

	sta = net_get_sta_from_frame_control(net_priv, vif_priv, mgmt->da, mgmt->frame_control, ap);

	if (params->offchan) {
		if (!params->chan)
			return -EINVAL;

		offchan = true;
		chan_info = rs_net_cfg80211_get_chaninfo(vif_priv);
		if ((chan_info) && (chan_info->chan_def.chan->center_freq == params->chan->center_freq)) {
			offchan = false;
		}
	}

	if (offchan) {
		// Offchannel transmission, need to start a RoC
		if (net_priv->roc) {
			// Test if current RoC can be re-used
			if (net_priv->roc->vif != vif_priv ||
			    net_priv->roc->chan->center_freq != params->chan->center_freq)
				return -EINVAL;
			// TODO: inform FW to increase RoC duration
		} else {
			int error;
			unsigned int duration = 30;

			/* Start a new ROC procedure */
			if (params->wait)
				duration = params->wait;

			error = net_cfg80211_remain_on_channel(wiphy, wdev, params->chan, duration, NULL);
			if (error)
				return error;

			// internal RoC, no need to inform user space about it
			net_priv->roc->internal = true;
		}
	}

	ret = rs_net_tx_mgmt(c_if, vif_priv, sta, params, offchan, cookie);
	if (offchan) {
		if (net_priv->roc->tx_cnt < RS_NET_ROC_TX)
			net_priv->roc->tx_cookie[net_priv->roc->tx_cnt] = *cookie;
		else
			wiphy_warn(wiphy, "%d frames sent within the same Roc (> RS_NET_ROC_TX)",
				   net_priv->roc->tx_cnt + 1);
		net_priv->roc->tx_cnt++;
	}

	return ret;
}

static int net_cfg80211_mgmt_tx_cancel_wait(struct wiphy *wiphy, struct wireless_dev *wdev, u64 cookie)
{
	s32 ret = 0;
	struct rs_c_if *c_if = NULL;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct rs_net_vif_priv *vif_priv = NULL;
	struct net_device *ndev = wdev->netdev;
	int i, n_tx_cookie = 0;

	RS_TRACE(RS_FN_ENTRY_STR);

	net_priv = wiphy_priv(wiphy);
	c_if = rs_net_priv_get_c_if(net_priv);
	vif_priv = netdev_priv(ndev);

	if (!net_priv->roc || !net_priv->roc->tx_cnt)
		return 0;

	for (i = 0; i < RS_NET_ROC_TX; i++) {
		if (!net_priv->roc->tx_cookie[i])
			continue;

		n_tx_cookie++;
		if (net_priv->roc->tx_cookie[i] == cookie) {
			net_priv->roc->tx_cookie[i] = 0;
			net_priv->roc->tx_cnt--;
			break;
		}
	}

	if (i == RS_NET_ROC_TX) {
		if (n_tx_cookie != net_priv->roc->tx_cnt)
			net_priv->roc->tx_cnt--;
		else
			return 0;
	}

	if (!net_priv->roc->internal || net_priv->roc->tx_cnt > 0)
		return 0;

	ret = net_cfg80211_cancel_remain_on_channel(wiphy, wdev, (u64)net_priv->roc);

	return ret;
}

static int net_cfg80211_start_ap(struct wiphy *wiphy, struct net_device *ndev,
				 struct cfg80211_ap_settings *settings)
{
	s32 ret = 0;
	struct rs_c_if *c_if = NULL;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct rs_net_vif_priv *vif_priv = NULL;
	struct rs_net_sta_priv *sta = NULL;
	struct rs_c_ap_start_rsp *ap_start_rsp = NULL;

	RS_TRACE(RS_FN_ENTRY_STR);

	net_priv = wiphy_priv(wiphy);
	c_if = rs_net_priv_get_c_if(net_priv);
	vif_priv = netdev_priv(ndev);

	ap_start_rsp = &net_priv->cmd_rsp.ap_start_rsp;

	ret = rs_net_ctrl_ap_start_req(c_if, vif_priv, settings, ap_start_rsp);
	if (ret != RS_SUCCESS) {
		goto end;
	}

	switch (RS_GET_DEV_RET(ap_start_rsp->status)) {
	case RS_SUCCESS:
		vif_priv->ap.bcmc_index = ap_start_rsp->bcmc_idx;
		vif_priv->ap.flags = 0;
		vif_priv->ap.ap_isolate = 0;
		vif_priv->ap.bcn_interval = settings->beacon_interval;
		sta = rs_net_priv_get_sta_info(net_priv, ap_start_rsp->bcmc_idx);
		sta->valid = true;
		sta->aid = 0;
		sta->sta_idx = ap_start_rsp->bcmc_idx;
		sta->ch_idx = ap_start_rsp->ch_idx;
		sta->vif_idx = vif_priv->vif_index;
		sta->qos = false;
		sta->acm = 0;
		sta->listen_interval = 5;
		sta->ht = 0;
		sta->vht = 0;
		sta->he = 0;
		(void)rs_net_cfg80211_set_chaninfo(vif_priv, ap_start_rsp->ch_idx, &settings->chandef);

		netif_tx_start_all_queues(ndev);
		netif_carrier_on(ndev);

#ifdef CONFIG_RS_5G_DFS
		rs_net_dfs_detection_enabled_on_cur_channel(net_priv);
#endif		
		break;
	case RS_BUSY:
		ret = -EINPROGRESS;
		break;
	case RS_IN_PROGRESS:
		ret = -EALREADY;
		break;
	default:
		ret = -EIO;
		break;
	}

end:
	if (ret != RS_SUCCESS) {
		netdev_err(ndev, "Failed to start AP (%d)\n", ret);
	} else {
		netdev_info(ndev, "AP started: ch=%d, bcmc_idx: %d\n", vif_priv->ch_index,
			    vif_priv->ap.bcmc_index);
	}

	return ret;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0)
static int net_cfg80211_stop_ap(struct wiphy *wiphy, struct net_device *ndev, unsigned int link_id)
#else
static int net_cfg80211_stop_ap(struct wiphy *wiphy, struct net_device *ndev)
#endif
{
	s32 ret = 0;
	struct rs_c_if *c_if = NULL;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct rs_net_vif_priv *vif_priv = NULL;

	RS_TRACE(RS_FN_ENTRY_STR);

	net_priv = wiphy_priv(wiphy);
	c_if = rs_net_priv_get_c_if(net_priv);
	vif_priv = netdev_priv(ndev);

	// Remove all stations
	(void)net_del_station(c_if, net_priv, vif_priv, NULL);

	rs_net_cac_stop(&net_priv->dfs);
	rs_net_ctrl_ap_stop_req(c_if, vif_priv);
	(void)rs_net_cfg80211_unset_chaninfo(vif_priv);

	return ret;
}

static int net_cfg80211_change_beacon(struct wiphy *wiphy, struct net_device *ndev,
				      struct cfg80211_beacon_data *info)
{
	s32 ret = 0;
	struct rs_c_if *c_if = NULL;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct rs_net_vif_priv *vif_priv = NULL;
	struct rs_net_beacon_info *bcn = NULL;
	u8 *bcn_buf;

	RS_TRACE(RS_FN_ENTRY_STR);

	net_priv = wiphy_priv(wiphy);
	c_if = rs_net_priv_get_c_if(net_priv);
	vif_priv = netdev_priv(ndev);

	bcn = &vif_priv->ap.bcn;

	bcn_buf = net_create_beacon(bcn, info);
	if (!bcn_buf) {
		return -ENOMEM;
	}

	ret = rs_net_ctrl_change_beacon_req(c_if, vif_priv->vif_index, bcn_buf, bcn->len, bcn->head_len,
					    bcn->tim_len, NULL);

	if (bcn_buf) {
		rs_k_free(bcn_buf);
	}

	return ret;
}

static int net_cfg80211_set_monitor_channel(struct wiphy *wiphy, struct cfg80211_chan_def *chandef)
{
	s32 ret = 0;
	struct rs_c_if *c_if = NULL;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct rs_net_vif_priv *vif_priv = NULL;
	struct rs_c_mon_mode_rsp *mon_mode_rsp = NULL;
	struct rs_net_chan_info *chan_info = NULL;

	RS_TRACE(RS_FN_ENTRY_STR);

	net_priv = wiphy_priv(wiphy);
	c_if = rs_net_priv_get_c_if(net_priv);

	if (net_priv->mon_vif_index == RS_NET_INVALID_IFTYPE) {
		return -EINVAL;
	}

	mon_mode_rsp = &net_priv->cmd_rsp.mon_mode_rsp;

	vif_priv = net_priv->vif_table[net_priv->mon_vif_index];

	chan_info = rs_net_cfg80211_get_chaninfo(vif_priv);
	if ((chan_info) && (chandef) && cfg80211_chandef_identical(&(chan_info->chan_def), chandef)) {
		return 0;
	}

	ret = rs_net_ctrl_monitor_mode_req(c_if, chandef, mon_mode_rsp);

	if (ret != RS_SUCCESS) {
		return -EIO;
	}

	(void)rs_net_cfg80211_unset_chaninfo(vif_priv);

	if (mon_mode_rsp->chan_index != RS_NET_CH_NOT_SET) {
		struct cfg80211_chan_def chandef_mon;

		if (net_priv->vif_started > 1) {
			(void)rs_net_cfg80211_set_chaninfo(vif_priv, mon_mode_rsp->chan_index, NULL);
			return -EBUSY;
		}

		rs_k_memset(&chandef_mon, 0, sizeof(chandef_mon));
		chandef_mon.chan = ieee80211_get_channel(wiphy, mon_mode_rsp->chan.freq_prim20);
		chandef_mon.center_freq1 = mon_mode_rsp->chan.freq_cen1;
		chandef_mon.center_freq2 = mon_mode_rsp->chan.freq_cen2;
		chandef_mon.width = chnl2bw[mon_mode_rsp->chan.ch_bw];
		(void)rs_net_cfg80211_set_chaninfo(vif_priv, mon_mode_rsp->chan_index, &chandef_mon);
	}

	return ret;
}

static int net_cfg80211_probe_client(struct wiphy *wiphy, struct net_device *ndev, const u8 *peer,
				     u64 *cookie)
{
	s32 ret = 0;
	struct rs_c_if *c_if = NULL;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct rs_net_vif_priv *vif_priv = NULL;
	struct rs_net_sta_priv *sta = NULL;
	struct rs_c_probe_client_rsp *probe_client_rsp = NULL;

	RS_TRACE(RS_FN_ENTRY_STR);

	net_priv = wiphy_priv(wiphy);
	c_if = rs_net_priv_get_c_if(net_priv);
	vif_priv = netdev_priv(ndev);

	if ((RS_NET_WDEV_IF_TYPE(vif_priv) != NL80211_IFTYPE_AP) &&
	    (RS_NET_WDEV_IF_TYPE(vif_priv) != NL80211_IFTYPE_AP_VLAN) &&
	    (RS_NET_WDEV_IF_TYPE(vif_priv) != NL80211_IFTYPE_P2P_GO))
		return -EINVAL;

	list_for_each_entry(sta, &vif_priv->ap.sta_list, list) {
		if (sta->valid && ether_addr_equal(sta->mac_addr, peer))
			break;
	}

	if (!sta) {
		return -EINVAL;
	}

	probe_client_rsp = &net_priv->cmd_rsp.probe_client_rsp;

	ret = rs_net_ctrl_probe_client_req(c_if, vif_priv->vif_index, sta->sta_idx, probe_client_rsp);

	if (probe_client_rsp->status != RS_SUCCESS) {
		return -EINVAL;
	}

	*cookie = (u64)probe_client_rsp->probe_id;

	return ret;
}

static int net_cfg80211_set_wiphy_params(struct wiphy *wiphy, u32 changed)
{
	return 0;
}

static int net_cfg80211_set_txq_params(struct wiphy *wiphy, struct net_device *ndev,
				       struct ieee80211_txq_params *params)
{
	s32 ret = 0;
	struct rs_c_if *c_if = NULL;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct rs_net_vif_priv *vif_priv = NULL;
	u8 ac, aifs, cwmin, cwmax;
	u32 param;

	RS_TRACE(RS_FN_ENTRY_STR);

	net_priv = wiphy_priv(wiphy);
	c_if = rs_net_priv_get_c_if(net_priv);
	vif_priv = netdev_priv(ndev);

	ac = net_ac2hwq[params->ac];
	aifs = params->aifs;
	cwmin = fls(params->cwmin);
	cwmax = fls(params->cwmax);

	param = (u32)(aifs << 0);
	param |= (u32)(cwmin << 4);
	param |= (u32)(cwmax << 8);
	param |= (u32)(params->txop << 12);

	ret = rs_net_ctrl_edca_req(c_if, ac, param, false, vif_priv->vif_index);

	return ret;
}

static int net_cfg80211_set_tx_power(struct wiphy *wiphy, struct wireless_dev *wdev,
				     enum nl80211_tx_power_setting type, int mbm)
{
	s32 ret = 0;
	struct rs_c_if *c_if = NULL;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct rs_net_vif_priv *vif_priv = NULL;
	s8 tx_pwr = 0;

	RS_TRACE(RS_FN_ENTRY_STR);

	net_priv = wiphy_priv(wiphy);
	c_if = rs_net_priv_get_c_if(net_priv);

	if (type == NL80211_TX_POWER_AUTOMATIC) {
		tx_pwr = 0x7F;
	} else {
		tx_pwr = MBM_TO_DBM(mbm);
	}

	if (wdev) {
		vif_priv = container_of(wdev, struct rs_net_vif_priv, wdev);
	}

	// TODO:
	// if (rs_net_vif_is_up(vif_priv) != RS_SUCCESS) {
	// 	return -EIO;
	// }

	if (wdev) {
		ret = rs_net_ctrl_set_tx_power_req(c_if, vif_priv->vif_index, tx_pwr);
	} else {
		list_for_each_entry(vif_priv, &net_priv->vifs, list) {
			ret = rs_net_ctrl_set_tx_power_req(c_if, vif_priv->vif_index, tx_pwr);
			if (ret != RS_SUCCESS) {
				break;
			}
		}
	}

	return ret;
}

static int net_cfg80211_set_power_mgmt(struct wiphy *wiphy, struct net_device *ndev, bool enabled,
				       int timeout)
{
	s32 ret = 0;
	struct rs_c_if *c_if = NULL;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct rs_net_vif_priv *vif_priv = NULL;
	u8 ps_mode;

	RS_TRACE(RS_FN_ENTRY_STR);

	net_priv = wiphy_priv(wiphy);
	c_if = rs_net_priv_get_c_if(net_priv);
	vif_priv = netdev_priv(ndev);

	if (rs_net_vif_is_up(vif_priv) != RS_SUCCESS) {
		return -EIO;
	}

	if (!(rs_net_chk_dev_feat(net_priv->cmd_rsp.fw_ver.mac_feat, DEV_FEAT_PS_BIT)))
		return -ENOTSUPP;

	if (enabled) {
		/* Switch to Dynamic Power Save */
		ps_mode = RS_NET_PS_MODE_ON_DYN;
	} else {
		/* Exit Power Save */
		ps_mode = RS_NET_PS_MODE_OFF;
	}

	ret = rs_net_ctrl_set_power_mgmt_req(c_if, ps_mode);

	return ret;
}

static int net_cfg80211_get_station(struct wiphy *wiphy, struct net_device *ndev, const u8 *mac,
				    struct station_info *sinfo)
{
	s32 ret = 0;
	struct rs_c_if *c_if = NULL;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct rs_net_vif_priv *vif_priv = NULL;
	struct rs_net_sta_priv *sta = NULL;

	RS_TRACE(RS_FN_ENTRY_STR);

	net_priv = wiphy_priv(wiphy);
	c_if = rs_net_priv_get_c_if(net_priv);
	vif_priv = netdev_priv(ndev);

	if (rs_net_vif_is_up(vif_priv) != RS_SUCCESS) {
		return -EIO;
	}

	if (RS_NET_WDEV_IF_TYPE(vif_priv) == NL80211_IFTYPE_MONITOR)
		return -EINVAL;

	if (RS_NET_WDEV_IF_TYPE(vif_priv) == NL80211_IFTYPE_STATION ||
	    RS_NET_WDEV_IF_TYPE(vif_priv) == NL80211_IFTYPE_P2P_CLIENT) {
		if (vif_priv->sta.ap && ether_addr_equal(mac, vif_priv->sta.ap->mac_addr)) {
			sta = vif_priv->sta.ap;
		}
	} else {
		struct rs_net_sta_priv *iter = NULL;

		list_for_each_entry(iter, &vif_priv->ap.sta_list, list) {
			if (iter->valid && ether_addr_equal(iter->mac_addr, mac)) {
				sta = iter;
				break;
			}
		}
	}

	if (sta) {
		ret = net_station_info_fill(sta, sinfo, vif_priv);
	}

	return ret;
}

static int net_cfg80211_dump_station(struct wiphy *wiphy, struct net_device *ndev, int idx, u8 *mac,
				     struct station_info *sinfo)
{
	s32 ret = -ENOENT;
	struct rs_c_if *c_if = NULL;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct rs_net_vif_priv *vif_priv = NULL;
	struct rs_net_sta_priv *sta = NULL;

	RS_TRACE(RS_FN_ENTRY_STR);

	net_priv = wiphy_priv(wiphy);
	c_if = rs_net_priv_get_c_if(net_priv);
	vif_priv = netdev_priv(ndev);

	if (RS_NET_WDEV_IF_TYPE(vif_priv) == NL80211_IFTYPE_MONITOR)
		return -EINVAL;

	if (RS_NET_WDEV_IF_TYPE(vif_priv) == NL80211_IFTYPE_STATION ||
	    RS_NET_WDEV_IF_TYPE(vif_priv) == NL80211_IFTYPE_P2P_CLIENT) {
		if (!idx && vif_priv->sta.ap && vif_priv->sta.ap->valid) {
			sta = vif_priv->sta.ap;
			ret = RS_SUCCESS;
		}
	} else {
		struct rs_net_sta_priv *sta_iter = NULL;
		int i = 0;
		list_for_each_entry(sta_iter, &vif_priv->ap.sta_list, list) {
			if (i == idx) {
				sta = sta_iter;
				ret = RS_SUCCESS;
				break;
			}
			i++;
		}
	}

	if (sta == NULL)
		return ret;

	/* Copy peer MAC address */
	rs_k_memcpy(mac, &sta->mac_addr, ETH_ALEN);

	net_station_info_fill(sta, sinfo, vif_priv);
	return ret;
}

static int net_cfg80211_remain_on_channel(struct wiphy *wiphy, struct wireless_dev *wdev,
					  struct ieee80211_channel *chan, unsigned int duration, u64 *cookie)
{
	s32 ret = 0;
	struct rs_c_if *c_if = NULL;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct rs_net_vif_priv *vif_priv = NULL;
	struct rs_net_remain_on_channel *roc = NULL;

	RS_TRACE(RS_FN_ENTRY_STR);

	net_priv = wiphy_priv(wiphy);
	c_if = rs_net_priv_get_c_if(net_priv);
	vif_priv = netdev_priv(wdev->netdev);

	if (!net_priv || !c_if || !vif_priv) {
		return -ENOMEM;
	}

	if (net_priv->roc) {
		return -EBUSY;
	}

	roc = rs_k_calloc(sizeof(struct rs_net_remain_on_channel));
	if (!roc) {
		return -ENOMEM;
	}

	roc->vif = vif_priv;
	roc->chan = chan;
	roc->duration = duration;
	roc->internal = false;
	roc->on_chan = false;
	roc->tx_cnt = 0;
	rs_k_memset(roc->tx_cookie, 0, sizeof(roc->tx_cookie));

	net_priv->roc = roc;
	ret = rs_net_ctrl_remain_on_channel_req(c_if, vif_priv, chan, duration);

	if (ret != RS_SUCCESS) {
		rs_k_free(roc);
		net_priv->roc = NULL;
	} else if (cookie) {
		*cookie = (u64)roc;
	}

	return ret;
}

static int net_cfg80211_cancel_remain_on_channel(struct wiphy *wiphy, struct wireless_dev *wdev, u64 cookie)
{
	s32 ret = 0;
	struct rs_c_if *c_if = NULL;
	struct rs_net_cfg80211_priv *net_priv = NULL;

	RS_TRACE(RS_FN_ENTRY_STR);

	net_priv = wiphy_priv(wiphy);
	c_if = rs_net_priv_get_c_if(net_priv);

	if (!net_priv->roc) {
		return ret;
	}

	if (cookie != (u64)net_priv->roc) {
		return -EINVAL;
	}

	ret = rs_net_ctrl_cancel_remain_on_channel_req(c_if);

	return ret;
}

static int net_cfg80211_channel_switch(struct wiphy *wiphy, struct net_device *ndev,
				       struct cfg80211_csa_settings *params)
{
	s32 ret = 0;
	struct rs_c_if *c_if = NULL;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct rs_net_vif_priv *vif_priv = NULL;
	struct rs_net_beacon_info *bcn, *bcn_after;
	struct rs_net_csa_info *csa;
	u16 csa_oft[RS_C_BCN_MAX_CSA_CPT];
	u8 *bcn_buf;
	u8 *bcn_after_buf;
	int i;

	RS_TRACE(RS_FN_ENTRY_STR);

	net_priv = wiphy_priv(wiphy);
	if (!net_priv)
		return -ENOMEM;
	c_if = rs_net_priv_get_c_if(net_priv);
	if (!c_if)
		return -ENOMEM;
	vif_priv = netdev_priv(ndev);
	if (!vif_priv)
		return -ENOMEM;

	if (vif_priv->ap.csa)
		return -EBUSY;

	if (params->n_counter_offsets_beacon > RS_C_BCN_MAX_CSA_CPT)
		return -EINVAL;

	bcn = &vif_priv->ap.bcn;
	bcn_buf = net_create_beacon(bcn, &params->beacon_csa);
	if (!bcn_buf)
		return -ENOMEM;

	rs_k_memset(csa_oft, 0, sizeof(csa_oft));
	for (i = 0; i < params->n_counter_offsets_beacon; i++) {
		csa_oft[i] = params->counter_offsets_beacon[i] + bcn->head_len + bcn->tim_len;
	}

	if (params->count == 0) {
		params->count = 2;
		for (i = 0; i < params->n_counter_offsets_beacon; i++) {
			bcn_buf[csa_oft[i]] = 2;
		}
	}

	csa = kzalloc(sizeof(struct rs_net_csa_info), GFP_KERNEL);
	if (!csa) {
		ret = -ENOMEM;
		goto end;
	}

	bcn_after = &csa->bcn;
	bcn_after_buf = net_create_beacon(bcn_after, &params->beacon_after);
	if (!bcn_after_buf) {
		ret = -ENOMEM;
		net_csa_remove(vif_priv);
		goto end;
	}

	vif_priv->ap.csa = csa;
	csa->vif = vif_priv;
	csa->chandef = params->chandef;
	rs_k_memcpy(csa->buf, bcn_after_buf, bcn_after->len);
	rs_k_free(bcn_after_buf);

	ret = rs_net_ctrl_change_beacon_req(c_if, vif_priv->vif_index, bcn_buf, bcn->len, bcn->head_len,
					    bcn->tim_len, csa_oft);
	rs_k_free(bcn_buf);

	if (ret != RS_SUCCESS) {
		net_csa_remove(vif_priv);
	} else {
		INIT_WORK(&csa->work, net_csa_work);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 3, 0)
		cfg80211_ch_switch_started_notify(ndev, &csa->chandef, 0 /*link_id*/, params->count,
						  true /*quiet*/, 0 /*punct_bitmap*/);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0)
		cfg80211_ch_switch_started_notify(ndev, &csa->chandef, 0 /*link_id*/, params->count,
						  true /*quiet*/);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 11, 0)
		cfg80211_ch_switch_started_notify(ndev, &csa->chandef, params->count, true /*quiet*/);
#else
		cfg80211_ch_switch_started_notify(ndev, &csa->chandef, params->count);
#endif
	}

end:
	return ret;
}

static int net_cfg80211_dump_survey(struct wiphy *wiphy, struct net_device *ndev, int idx,
				    struct survey_info *info)
{
	s32 ret = 0;
	struct rs_c_if *c_if = NULL;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct rs_net_vif_priv *vif_priv = NULL;
	struct rs_c_survey_info *survey_info = NULL;
	struct ieee80211_supported_band *sband;

	RS_TRACE(RS_FN_ENTRY_STR);

	net_priv = wiphy_priv(wiphy);
	if (!net_priv)
		return -ENOMEM;
	c_if = rs_net_priv_get_c_if(net_priv);
	if (!c_if)
		return -ENOMEM;
	vif_priv = netdev_priv(ndev);
	if (!vif_priv)
		return -ENOMEM;

	if (idx >= ARRAY_SIZE(net_priv->survey_table))
		return -ENONET;

	survey_info = &net_priv->survey_table[idx];

	sband = wiphy->bands[NL80211_BAND_2GHZ];
	if (!sband)
		return -ENONET;

	if (idx >= sband->n_channels) {
		idx -= sband->n_channels;
		sband = NULL;
	}

	if (!sband) {
		sband = wiphy->bands[NL80211_BAND_5GHZ];

		if (!sband || idx >= sband->n_channels)
			return -ENONET;
	}

	info->channel = &sband->channels[idx];
	info->filled = survey_info->filled;

	if (info->filled != 0) {
		info->time = (u64)survey_info->chan_dwell_ms;
		info->time_busy = (u64)survey_info->chan_busy_ms;
		info->noise = survey_info->chan_noise_dbm;
	}

	return ret;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0)
static int net_cfg80211_get_channel(struct wiphy *wiphy, struct wireless_dev *wdev, unsigned int link_id,
				    struct cfg80211_chan_def *chandef)
#else
static int net_cfg80211_get_channel(struct wiphy *wiphy, struct wireless_dev *wdev,
				    struct cfg80211_chan_def *chandef)
#endif
{
	s32 ret = 0;
	struct rs_c_if *c_if = NULL;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct rs_net_vif_priv *vif_priv = NULL;
	struct rs_net_chan_info *chan_info = NULL;

	RS_TRACE(RS_FN_ENTRY_STR);

	net_priv = wiphy_priv(wiphy);
	c_if = rs_net_priv_get_c_if(net_priv);
	vif_priv = container_of(wdev, struct rs_net_vif_priv, wdev);

	if ((net_priv) && (vif_priv->up == TRUE)) {
		if (vif_priv->vif_index == net_priv->mon_vif_index) {
			// retrieve channel from firmware
			// TODO : rs_cfg80211_set_monitor_channel(wiphy, NULL);
		}

		// Check if channel context is valid
		chan_info = rs_net_cfg80211_get_chaninfo(vif_priv);
		if ((chandef) && (chan_info)) {
			*chandef = chan_info->chan_def;
		} else {
			ret = -ENODATA;
		}

	} else {
		ret = -ENODATA;
	}

	RS_DBG("P:%s[%d]:up[%d]:r[%d]:chan[%d]:width[%d]:freq1[%d]:freq2[%d]\n", __func__, __LINE__,
	       vif_priv->up, ret, chandef->chan, chandef->width, chandef->center_freq1,
	       chandef->center_freq2);

	return ret;
}

static int net_cfg80211_start_radar_detection(struct wiphy *wiphy, struct net_device *ndev,
					      struct cfg80211_chan_def *chandef, u32 cac_time_ms)
{
	s32 ret = RS_SUCCESS;
	struct rs_c_if *c_if = NULL;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct rs_net_vif_priv *vif_priv = NULL;
	struct rs_c_cas_start_rsp *cac_start_rsp;

	RS_TRACE(RS_FN_ENTRY_STR);

	net_priv = wiphy_priv(wiphy);
	c_if = rs_net_priv_get_c_if(net_priv);
	vif_priv = netdev_priv(ndev);

	cac_start_rsp = &net_priv->cmd_rsp.cac_start_rsp;

	rs_net_dfs_cac_start(&net_priv->dfs, cac_time_ms, vif_priv);
	ret = rs_net_ctrl_cac_start_req(c_if, vif_priv, chandef, cac_start_rsp);
	if (cac_start_rsp->status == RS_SUCCESS) {
		rs_net_cfg80211_set_chaninfo(vif_priv, cac_start_rsp->ch_idx, chandef);
		if (net_priv->chaninfo_index == cac_start_rsp->ch_idx) {
			rs_net_dfs_detection_enable(&net_priv->dfs, RS_DFS_DETECT_REPORT, RS_DFS_RIU);
		}
	} else
		ret = -EIO;

	return ret;
}

static int net_cfg80211_set_cqm_rssi_config(struct wiphy *wiphy, struct net_device *ndev, int32_t rssi_thold,
					    uint32_t rssi_hyst)
{
	s32 ret = 0;
	struct rs_c_if *c_if = NULL;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct rs_net_vif_priv *vif_priv = NULL;

	RS_TRACE(RS_FN_ENTRY_STR);

	net_priv = wiphy_priv(wiphy);
	c_if = rs_net_priv_get_c_if(net_priv);
	vif_priv = netdev_priv(ndev);

	ret = rs_net_ctrl_cqm_rssi_config_req(c_if, vif_priv->vif_index, rssi_thold, rssi_hyst);

	return ret;
}

static int net_cfg80211_change_bss(struct wiphy *wiphy, struct net_device *ndev,
				   struct bss_parameters *params)
{
	s32 ret = -EIO;
	struct rs_c_if *c_if = NULL;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct rs_net_vif_priv *vif_priv = NULL;

	RS_TRACE(RS_FN_ENTRY_STR);

	net_priv = wiphy_priv(wiphy);
	c_if = rs_net_priv_get_c_if(net_priv);
	vif_priv = netdev_priv(ndev);

	if (((RS_NET_WDEV_IF_TYPE(vif_priv) == NL80211_IFTYPE_AP ||
	      RS_NET_WDEV_IF_TYPE(vif_priv) == NL80211_IFTYPE_P2P_GO)) &&
	    (params->ap_isolate >= 0)) {
		// (0 = no, 1 = yes, -1 = do not change)
		if (params->ap_isolate == 0) {
			vif_priv->ap.ap_isolate = 0;
		} else if (params->ap_isolate > 0) {
			vif_priv->ap.ap_isolate = 1;
		}

		if (rs_net_ctrl_set_ap_isolate(c_if, vif_priv->ap.ap_isolate) == RS_SUCCESS) {
			ret = 0;
		}
	}

	return ret;
}

static rs_ret net_print_fw_ver(struct rs_net_cfg80211_priv *net_priv, struct rs_c_fw_ver_rsp *fw_ver)
{
	rs_ret ret = RS_FAIL;

	u32 vers = fw_ver->fw_version;
	struct wiphy *wiphy = rs_net_priv_get_wiphy(net_priv);

	RS_TRACE(RS_FN_ENTRY_STR);

	if ((net_priv) && (wiphy)) {
		(void)snprintf(wiphy->fw_version, sizeof(wiphy->fw_version), "%d.%d.%d.%d.%d",
			       (vers & (0xf << 28)) >> 28, (vers & (0xf << 24)) >> 24,
			       (vers & (0xff << 16)) >> 16, (vers & (0xff << 8)) >> 8, vers & 0xff);

		RS_INFO("FW version %s\n", wiphy->fw_version);

		net_priv->machw_version = rs_net_get_hw_ver(fw_ver->machw_version);

		RS_DBG("machw_features[0x%X]", fw_ver->machw_features);
		RS_DBG("machw_version[0x%X]", fw_ver->machw_version);
		RS_DBG("phy_feature[0x%X]", fw_ver->phy_feature);
		RS_DBG("phy_version[0x%X]", fw_ver->phy_version);
		RS_DBG("features[0x%X]", fw_ver->mac_feat);
		RS_DBG("sta_max_count[%d]", fw_ver->sta_max_count);
		RS_DBG("sta_max_count[%d]", fw_ver->vif_max_count);

		ret = RS_SUCCESS;
	}

	return ret;
}

static rs_ret net_cfg80211_set_default_wiphy(struct rs_c_if *c_if, struct wiphy *wiphy)
{
	rs_ret ret = RS_FAIL;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	u8 mac_addr[ETH_ADDR_LEN] = { 0 };

	RS_TRACE(RS_FN_ENTRY_STR);

	net_priv = rs_c_if_get_net_priv(c_if);

	if (net_priv) {
		ret = net_get_mac_addr(c_if, RS_DEFAULT_MAC_PATH, mac_addr);
		if (ret != RS_SUCCESS) {
			RS_ERR("MAC Address get failed\n");
		}
	}

	if (ret == RS_SUCCESS) {
		// ether_addr_copy(wiphy->perm_addr, mac_addr);
		(void)rs_k_memcpy(wiphy->perm_addr, mac_addr, ETH_ADDR_LEN);

		wiphy->mgmt_stypes = net_macm_stypes;

		wiphy->bands[NL80211_BAND_2GHZ] = &net_cfg80211_band_2g;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 20, 0)
		if (rs_net_params_get_he_enabled(c_if)) {
			net_cfg80211_band_2g.iftype_data = &cfg80211_cap_he;
		}
#endif

#ifdef CONFIG_RS_SUPPORT_5G
		wiphy->bands[NL80211_BAND_5GHZ] = &net_cfg80211_band_5g;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 20, 0)
		if (rs_net_params_get_he_enabled(c_if)) {
			net_cfg80211_band_5g.iftype_data = &cfg80211_cap_he;
		}
#endif
#else
		wiphy->bands[NL80211_BAND_5GHZ] = NULL;
#endif
		wiphy->interface_modes = RS_BIT(NL80211_IFTYPE_STATION) | RS_BIT(NL80211_IFTYPE_AP) |
					 RS_BIT(NL80211_IFTYPE_AP_VLAN) | RS_BIT(NL80211_IFTYPE_P2P_CLIENT) |
					 RS_BIT(NL80211_IFTYPE_P2P_GO) | RS_BIT(NL80211_IFTYPE_MONITOR);
		wiphy->flags |= WIPHY_FLAG_HAS_REMAIN_ON_CHANNEL | WIPHY_FLAG_HAS_CHANNEL_SWITCH |
				WIPHY_FLAG_4ADDR_STATION | WIPHY_FLAG_4ADDR_AP | WIPHY_FLAG_REPORTS_OBSS |
				WIPHY_FLAG_OFFCHAN_TX;

		wiphy->max_scan_ssids = RS_C_SCAN_SSID_MAX;
		wiphy->max_scan_ie_len = RS_C_SCAN_IE_LEN;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 1, 0)
		wiphy->support_mbssid = 1;
#endif

		wiphy->max_num_csa_counters = RS_C_BCN_MAX_CSA_CPT;
		wiphy->max_remain_on_channel_duration = 5000;

		wiphy->features |= NL80211_FEATURE_NEED_OBSS_SCAN | NL80211_FEATURE_SK_TX_STATUS |
				   NL80211_FEATURE_VIF_TXPOWER | NL80211_FEATURE_ACTIVE_MONITOR |
				   NL80211_FEATURE_AP_MODE_CHAN_WIDTH_CHANGE;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 17, 0)
		wiphy->features |= NL80211_FEATURE_SAE;
#endif

		wiphy->iface_combinations = net_if_comb;
		wiphy->n_iface_combinations = ARRAY_SIZE(net_if_comb) - 1;
		wiphy->reg_notifier = net_reg_notifier;

		wiphy->signal_type = CFG80211_SIGNAL_TYPE_MBM;

		wiphy->cipher_suites = net_cipher_suites;
		wiphy->n_cipher_suites = ARRAY_SIZE(net_cipher_suites) - NET_RESERVED_CIPHER_NUM;

		wiphy->extended_capabilities = net_priv->ext_capa;
		wiphy->extended_capabilities_mask = net_priv->ext_capa;
		wiphy->extended_capabilities_len = ARRAY_SIZE(net_priv->ext_capa);
	}

	return ret;
}

static rs_ret net_vif_del_all(struct rs_net_cfg80211_priv *net_priv)
{
	rs_ret ret = RS_SUCCESS;

	struct rs_net_vif_priv *vif_priv = NULL, *tmp_vif_priv = NULL;
	struct wiphy *wiphy = NULL;
	struct wireless_dev *wdev = NULL;
	s16 i = 0;

	wiphy = rs_net_priv_get_wiphy(net_priv);
	if ((net_priv) && (wiphy)) {
		rtnl_lock();
		list_for_each_entry_safe(vif_priv, tmp_vif_priv, &net_priv->vifs, list) {
			wdev = rs_vif_priv_get_wdev(vif_priv);
			if (wdev) {
				(void)net_cfg80211_del_virtual_iface(wiphy, wdev);
			}
		}

		for (i = 0; i < RS_NET_PRIV_VIF_TABLE_MAX; i++) {
			(void)rs_net_priv_set_vif_priv(net_priv, i, NULL);
		}

		rtnl_unlock();
	}

	return ret;
}

static struct rs_net_sta_priv *net_get_sta_from_frame_control(struct rs_net_cfg80211_priv *net_priv,
							      struct rs_net_vif_priv *vif_priv, u8 *addr,
							      __le16 fc, bool ap)
{
	if (ap) {
		/* only deauth, disassoc and action are bufferable MMPDUs */
		bool bufferable = ieee80211_is_deauth(fc) || ieee80211_is_disassoc(fc) ||
				  ieee80211_is_action(fc);

		/* Check if the packet is bufferable or not */
		if (bufferable) {
			/* Check if address is a broadcast or a multicast address */
			if (is_broadcast_ether_addr(addr) || is_multicast_ether_addr(addr)) {
				/* Returned STA pointer */
				struct rs_net_sta_priv *sta =
					rs_net_priv_get_sta_info(net_priv, vif_priv->ap.bcmc_index);

				if (sta->valid)
					return sta;
			} else {
				/* Returned STA pointer */
				struct rs_net_sta_priv *sta = NULL;

				/* Go through list of STAs linked with the provided VIF */
				list_for_each_entry(sta, &vif_priv->ap.sta_list, list) {
					if (sta->valid && ether_addr_equal(sta->mac_addr, addr)) {
						/* Return the found STA */
						return sta;
					}
				}
			}
		}
	} else {
		if (vif_priv->sta.ap && vif_priv->sta.ap->valid &&
		    ether_addr_equal(vif_priv->sta.ap->mac_addr, addr))
			return vif_priv->sta.ap;
	}

	return NULL;
}

static int net_cfg80211_del_station(struct wiphy *wiphy, struct net_device *ndev,
				    struct station_del_parameters *params)
{
	s32 ret = 0;
	struct rs_c_if *c_if = NULL;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct rs_net_vif_priv *vif_priv = NULL;
	const u8 *mac = NULL;
	u8 found = 0;

	RS_TRACE(RS_FN_ENTRY_STR);

	net_priv = wiphy_priv(wiphy);
	c_if = rs_net_priv_get_c_if(net_priv);
	vif_priv = netdev_priv(ndev);

	if (rs_net_vif_is_up(vif_priv) != RS_SUCCESS) {
		return -EIO;
	}

	if (params) {
		mac = params->mac;
	}

	found = net_del_station(c_if, net_priv, vif_priv, mac);

	if (found == 0) {
		ret = -ENOENT;
	}

	return ret;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 17, 0)
static int net_cfg80211_external_auth(struct wiphy *wiphy, struct net_device *ndev,
				      struct cfg80211_external_auth_params *params)
{
	struct rs_c_if *c_if = NULL;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct rs_net_vif_priv *vif_priv = NULL;

	RS_TRACE(RS_FN_ENTRY_STR);

	net_priv = wiphy_priv(wiphy);
	c_if = rs_net_priv_get_c_if(net_priv);
	vif_priv = netdev_priv(ndev);

	if (!(vif_priv->sta.flags & RS_STA_AUTH_EXT))
		return -EINVAL;

	vif_priv->sta.flags &= ~RS_STA_AUTH_EXT;
	return rs_net_ctrl_sm_ext_auth_req_rsp(c_if, vif_priv->vif_index, params->status);
}
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 1, 0)
static int net_cfg80211_update_ft_ies(struct wiphy *wiphy, struct net_device *ndev,
				      struct cfg80211_update_ft_ies_params *ftie)
{
	s32 ret = 0;
	struct rs_c_if *c_if = NULL;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct rs_net_vif_priv *vif_priv = NULL;
	bool ft_in_none_rsn = false;
	int ft_ies_len = 0;
	u8 *ft_assoc_ies = NULL, *pos = NULL;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 1, 0)
	const struct element *rsne = NULL, *mde = NULL, *fte = NULL, *elem = NULL;
#else
	const u8 *rsne = NULL, *fte = NULL, *elem = NULL;
#endif

	RS_TRACE(RS_FN_ENTRY_STR);

	net_priv = wiphy_priv(wiphy);
	c_if = rs_net_priv_get_c_if(net_priv);
	vif_priv = netdev_priv(ndev);

	if (RS_NET_WDEV_IF_TYPE(vif_priv) != NL80211_IFTYPE_STATION || !vif_priv->sta.ft_assoc_ies)
		return -EINVAL;

	for_each_element(elem, ftie->ie, ftie->ie_len) {
		switch (elem->id) {
		case WLAN_EID_RSN:
			rsne = elem;
			break;
		case WLAN_EID_MOBILITY_DOMAIN:
			mde = elem;
			break;
		case WLAN_EID_FAST_BSS_TRANSITION:
			fte = elem;
			break;
		default:
			netdev_warn(ndev, "Unexpected FT element %d\n", elem->id);
			break;
		}
	}

	if (!mde) {
		netdev_warn(ndev, "FT: No Mobility Domain Element in FT IE\n");
		return -EINVAL;
	} else if (!rsne && !fte) {
		ft_in_none_rsn = true;
	} else if (!rsne || !fte) {
		netdev_warn(ndev, "FT: Incomplete FT IE\n");
		return -EINVAL;
	}

	for_each_element(elem, vif_priv->sta.ft_assoc_ies, vif_priv->sta.ft_assoc_ies_len) {
		if (elem->id == WLAN_EID_RSN || elem->id == WLAN_EID_MOBILITY_DOMAIN ||
		    elem->id == WLAN_EID_FAST_BSS_TRANSITION) {
			ft_ies_len += elem->datalen + sizeof(struct element);
		}
	}

	ft_assoc_ies = rs_k_malloc(vif_priv->sta.ft_assoc_ies_len - ft_ies_len);
	if (!ft_assoc_ies)
		return -ENOMEM;

	pos = ft_assoc_ies;
	for_each_element(elem, vif_priv->sta.ft_assoc_ies, vif_priv->sta.ft_assoc_ies_len) {
		if (elem->id == WLAN_EID_RSN) {
			if (ft_in_none_rsn) {
				netdev_warn(ndev, "Found RSN element in non RSN FT");
				goto abort;
			} else if (!rsne) {
				netdev_warn(ndev, "Found several RSN element");
				goto abort;
			} else {
				rs_k_memcpy(pos, rsne, sizeof(*rsne) + rsne->datalen);
				pos += sizeof(*rsne) + rsne->datalen;
				rsne = NULL;
			}
		} else if (elem->id == WLAN_EID_MOBILITY_DOMAIN) {
			if (!mde) {
				netdev_warn(ndev, "Found several Mobility Domain element");
				goto abort;
			} else {
				rs_k_memcpy(pos, mde, sizeof(*mde) + mde->datalen);
				pos += sizeof(*mde) + mde->datalen;
				mde = NULL;
			}
		} else if (elem->id == WLAN_EID_FAST_BSS_TRANSITION) {
			if (ft_in_none_rsn) {
				netdev_warn(ndev, "Found Fast Transition element in non RSN FT");
				goto abort;
			} else if (!fte) {
				netdev_warn(ndev, "found several Fast Transition element");
				goto abort;
			} else {
				rs_k_memcpy(pos, fte, sizeof(*fte) + fte->datalen);
				pos += sizeof(*fte) + fte->datalen;
				fte = NULL;
			}
		} else {
			// Put FTE after MDE if non present in Association Element
			if (fte && !mde) {
				rs_k_memcpy(pos, fte, sizeof(*fte) + fte->datalen);
				pos += sizeof(*fte) + fte->datalen;
				fte = NULL;
			}
			rs_k_memcpy(pos, elem, sizeof(*elem) + elem->datalen);
			pos += sizeof(*elem) + elem->datalen;
		}
	}

	if (fte) {
		rs_k_memcpy(pos, fte, sizeof(*fte) + fte->datalen);
		pos += sizeof(*fte) + fte->datalen;
		fte = NULL;
	}

	rs_k_free(vif_priv->sta.ft_assoc_ies);

	vif_priv->sta.ft_assoc_ies = ft_assoc_ies;
	vif_priv->sta.ft_assoc_ies_len = pos - ft_assoc_ies;

	if (vif_priv->sta.flags & RS_STA_AUTH_FT_OVER_DS) {
		struct rs_c_sm_connect_rsp *connect_rsp = &net_priv->cmd_rsp.connect_rsp;
		struct cfg80211_connect_params sme;

		rs_k_memset(&sme, 0, sizeof(sme));
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 1, 0)
		rsne = cfg80211_find_elem(WLAN_EID_RSN, vif_priv->sta.ft_assoc_ies,
					  vif_priv->sta.ft_assoc_ies_len);
#else
		rsne = cfg80211_find_ie(WLAN_EID_RSN, vif_priv->sta.ft_assoc_ies,
					vif_priv->sta.ft_assoc_ies_len);
#endif
		if (rsne && net_connect_param_fill_rsn(rsne, &sme)) {
			netdev_warn(ndev, "FT RSN parsing failed\n");
			return 0;
		}

		sme.ssid_len = vif_priv->sta.ft_assoc_ies[1];
		sme.ssid = &vif_priv->sta.ft_assoc_ies[2];
		sme.bssid = vif_priv->sta.ft_target_ap;
		sme.ie = &vif_priv->sta.ft_assoc_ies[2 + sme.ssid_len];
		sme.ie_len = vif_priv->sta.ft_assoc_ies_len - (2 + sme.ssid_len);
		sme.auth_type = NL80211_AUTHTYPE_FT;
		rs_net_ctrl_connect(c_if, ndev, &sme, connect_rsp);
		vif_priv->sta.flags &= ~RS_STA_AUTH_FT_OVER_DS;

	} else if (vif_priv->sta.flags & RS_STA_AUTH_FT_OVER_AIR) {
		u8 ssid_len;

		vif_priv->sta.flags &= ~RS_STA_AUTH_FT_OVER_AIR;

		// Skip the first element (SSID)
		ssid_len = vif_priv->sta.ft_assoc_ies[1] + 2;
		ret = rs_net_ctrl_ft_auth_req(c_if, ndev, &vif_priv->sta.ft_assoc_ies[ssid_len],
					      (vif_priv->sta.ft_assoc_ies_len - ssid_len));

		if (ret != RS_SUCCESS)
			netdev_err(ndev, "FT Over Air: Failed to send updated assoc elem\n");
	}

	return ret;

abort:
	rs_k_free(ft_assoc_ies);
	return ret;
}
#endif

/**
 * @testmode_cmd: Implement a cfg80211 test mode command. The passed @vif may
 *	be %NULL. The callback can sleep.
 */
#ifdef CONFIG_NL80211_TESTMODE
static s32 net_cfg80211_testmode_cmd(struct wiphy *wiphy, struct wireless_dev *wdev, void *data, int len)
{
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct rs_c_if *c_if = NULL;
	struct nlattr *tb[TESTMODE_ATTR_MAX];
	s32 ret = RS_FAIL;
	struct net_device *ndev = NULL;
	struct rs_net_vif_priv *vif_priv = NULL;

	RS_DBG(RS_FN_ENTRY_STR);

	net_priv = wiphy_priv(wiphy);
	c_if = rs_net_priv_get_c_if(net_priv);
	ndev = wdev->netdev;
	vif_priv = netdev_priv(ndev);

	if (!c_if)
		return -1;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
	ret = nla_parse(tb, TESTMODE_ATTR_MAX - 1, data, len, testmode_attr_policy, NULL);
	if (ret != 0) {
		RS_ERR("Error parsing the gnl message : %d\n", ret);
		return ret;
	}
#endif
	if (!tb[TESTMODE_ATTR_CMD]) {
		RS_ERR("Error finding testmode command type\n");
		return -ENOMSG;
	}

	switch (nla_get_u32(tb[TESTMODE_ATTR_CMD])) {
	case TESTMODE_CMD_READ_REG:
	case TESTMODE_CMD_WRITE_REG:
		ret = rs_net_testmode_reg(c_if, tb);
		break;
	case TESTMODE_CMD_LOGMODEFILTER_SET:
	case TESTMODE_CMD_DBGLEVELFILTER_SET:
		ret = rs_net_testmode_dbg_filter(c_if, tb);
		break;
	case TESTMODE_CMD_TX:
		ret = rs_net_testmode_rf_tx(c_if, tb);
		break;
	case TESTMODE_CMD_CW:
		ret = rs_net_testmode_rf_cw(c_if, tb);
		break;
	case TESTMODE_CMD_CONT:
		ret = rs_net_testmode_rf_cont(c_if, tb);
		break;
	case TESTMODE_CMD_CHANNEL:
		ret = rs_net_testmode_rf_ch(c_if, tb);
		break;
	case TESTMODE_CMD_PER:
		ret = rs_net_testmode_rf_per(c_if, tb);
		break;
	case TESTMODE_CMD_HOST_LOG_LEVEL:
		ret = rs_net_testmode_host_log_level(c_if, tb);
		break;
	case TESTMODE_CMD_TX_POWER:
		ret = rs_net_testmode_tx_power(c_if, tb, vif_priv);
		break;
	case TESTMODE_CMD_STATS_START:
		ret = rs_net_testmod_stats_start(c_if, tb, vif_priv);
		break;
	case TESTMODE_CMD_STATS_STOP:
		ret = rs_net_testmod_stats_stop(c_if, tb, vif_priv);
		break;
	case TESTMODE_CMD_STATS_TX:
		ret = rs_net_testmod_stats_tx(c_if, tb, vif_priv);
		break;
	case TESTMODE_CMD_STATS_RX:
		ret = rs_net_testmod_stats_rx(c_if, tb, vif_priv);
		break;
	case TESTMODE_CMD_STATS_RSSI:
		ret = rs_net_testmod_stats_rssi(c_if, tb, vif_priv);
		break;
	default:
		RS_ERR("Unknown testmode command\n");
		ret = -EINVAL;
		break;
	}

	return ret;
}
#endif

void wq_ts_cb(struct work_struct *work)
{
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct rs_c_if *c_if = NULL;

	net_priv = container_of(work, struct rs_net_cfg80211_priv, wq_ts.work);
	c_if = rs_net_priv_get_c_if(net_priv);

	rs_net_ctrl_time_sync_with_dev(c_if);

	schedule_delayed_work(&net_priv->wq_ts, msecs_to_jiffies(TIME_SYNC_INTERVAL));
}

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL FUNCTION

rs_ret rs_net_cfg80211_init(struct rs_c_if *c_if)
{
	rs_ret ret = RS_FAIL;
	struct wiphy *wiphy = NULL;
	struct wireless_dev *wdev = NULL;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	s32 i = 0;

	RS_TRACE(RS_FN_ENTRY_STR);

	wiphy = wiphy_new(&net_cfg80211_ops, sizeof(struct rs_net_cfg80211_priv));
	if (wiphy) {
		net_priv = wiphy_priv(wiphy);
		if (net_priv) {
			net_priv->core = rs_c_if_get_core(c_if);
			net_priv->dev_if = rs_c_if_get_dev_if(c_if);

			(void)set_wiphy_dev(wiphy, net_priv->dev_if);
			(void)rs_net_priv_set_wiphy(net_priv, wiphy);
			ret = rs_c_if_set_net_priv(c_if, net_priv);
			RS_DBG("P:%s[%d]:c_if[0x%p]:core[0x%p]:core[0x%p]\n", __func__, __LINE__, c_if,
			       c_if->core, net_priv->core);
		}

		if (ret == RS_SUCCESS) {
			/// Set net_priv
			net_priv->vif_started = 0;
			net_priv->mon_vif_index = RS_NET_INVALID_IFTYPE;

			for (i = 0; i < RS_NET_VIF_DEV_MAX; i++) {
				net_priv->avail_idx_map |= RS_BIT(i);
			}

			INIT_LIST_HEAD(&net_priv->vifs);

			net_priv->roc = NULL;

			net_priv->ext_capa[0] = WLAN_EXT_CAPA1_EXT_CHANNEL_SWITCHING;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 1, 0)
			net_priv->ext_capa[2] = WLAN_EXT_CAPA3_MULTI_BSSID_SUPPORT;
#endif
			net_priv->ext_capa[7] = WLAN_EXT_CAPA8_OPMODE_NOTIF;
			// Max number of MSDUs in A-MSDU = 3 (=> 8 subframes max)
			net_priv->ext_capa[7] |= WLAN_EXT_CAPA8_MAX_MSDU_IN_AMSDU_LSB;
			net_priv->ext_capa[8] = WLAN_EXT_CAPA9_MAX_MSDU_IN_AMSDU_MSB;
		}

		if (ret == RS_SUCCESS) {
			/// Set parameter to F/W
			ret = rs_net_ctrl_dev_reset(c_if);
		}

		if (ret == RS_SUCCESS) {
			ret = rs_net_ctrl_get_fw_ver(c_if, &net_priv->cmd_rsp.fw_ver);
		}

		if (ret == RS_SUCCESS) {
			/// Set wiphy
			ret = net_cfg80211_set_default_wiphy(c_if, wiphy);
		}

		if (ret == RS_SUCCESS) {
			ret = net_print_fw_ver(net_priv, &net_priv->cmd_rsp.fw_ver);
		}

		if (ret == RS_SUCCESS) {
			ret = rs_net_params_init(c_if);
		}

		rs_net_dfs_detection_init(&net_priv->dfs);

		if (ret == RS_SUCCESS) {
			ret = rs_net_ctrl_me_config(c_if);
		}

		if (ret == RS_SUCCESS) {
			if (wiphy_register(wiphy) != 0) {
				ret = RS_FAIL;
				RS_ERR("Could not register wiphy device\n");
			}
		}

		if (ret == RS_SUCCESS) {
			ret = rs_net_params_custom_regd(c_if);
		}

		if (ret == RS_SUCCESS) {
			ret = rs_net_ctrl_me_chan_config(c_if);
		}

#ifdef CONFIG_DEBUG_FS
		if (ret == RS_SUCCESS) {
			ret = rs_net_dbgfs_register(net_priv);
			if (ret) {
				RS_ERR("Failed to register debugfs entries\n");
			}
		}
#endif

		if (ret == RS_SUCCESS) {
			rtnl_lock();

			wdev = net_add_virtual_iface(net_priv, "wlan%d", NET_NAME_UNKNOWN,
						     NL80211_IFTYPE_STATION, NULL, false);

			rtnl_unlock();

			if (!wdev) {
				RS_INFO("Failed to instantiate a network device\n");
				ret = RS_FAIL;
			} else {
				RS_INFO("New interface create %s\n", wdev->netdev->name);
			}
#if TIME_SYNC_DEBUG
			INIT_DELAYED_WORK(&net_priv->wq_ts, wq_ts_cb);
			schedule_delayed_work(&net_priv->wq_ts, msecs_to_jiffies(TIME_SYNC_INTERVAL));
#endif
		}
	} else {
		RS_ERR("Failed to create new wiphy\n");
		ret = RS_FAIL;
	}

	if (ret != RS_SUCCESS) {
		wiphy_free(wiphy);
		(void)rs_net_priv_set_wiphy(net_priv, NULL);
		(void)rs_c_if_set_net_priv(c_if, NULL);
	}

	return ret;
}

rs_ret rs_net_cfg80211_deinit(struct rs_c_if *c_if)
{
	rs_ret ret = RS_FAIL;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct wiphy *wiphy = NULL;

	RS_TRACE(RS_FN_ENTRY_STR);

	net_priv = rs_c_if_get_net_priv(c_if);
	wiphy = rs_net_priv_get_wiphy(net_priv);

	if ((net_priv) && (wiphy)) {
		(void)net_vif_del_all(net_priv);

#ifdef CONFIG_DEBUG_FS
		(void)rs_net_dbgfs_deregister();
#endif

		(void)wiphy_unregister(wiphy);
		(void)wiphy_free(wiphy);

		rs_net_dfs_detection_deinit(&net_priv->dfs);

		(void)rs_net_priv_set_wiphy(net_priv, NULL);
		(void)rs_c_if_set_net_priv(c_if, NULL);

#if TIME_SYNC_DEBUG
		(void)cancel_delayed_work_sync(&net_priv->wq_ts);
#endif
	}

	return ret;
}

rs_ret rs_net_cfg80211_set_chaninfo(struct rs_net_vif_priv *vif_priv, u8 ch_idx,
				    struct cfg80211_chan_def *chandef)
{
	rs_ret ret = RS_FAIL;
	struct rs_net_cfg80211_priv *net_priv = rs_vif_priv_get_net_priv(vif_priv);
	struct rs_net_chan_info *chan_info = NULL;

	RS_TRACE(RS_FN_ENTRY_STR);

	if ((vif_priv != NULL) && (net_priv != NULL) && (ch_idx < RS_NET_CHANINFO_MAX)) {
		chan_info = &(net_priv->chaninfo_table[ch_idx]);
		chan_info->count++;

		// For now chandef is NULL for STATION interface
		if (chandef != NULL) {
			if (chan_info->chan_def.chan == NULL) {
				chan_info->chan_def = *chandef;

				vif_priv->ch_index = ch_idx;

				ret = RS_SUCCESS;
			} else {
				struct ieee80211_channel *m_chan = chan_info->chan_def.chan;
				struct ieee80211_channel *s_chan = chandef->chan;
				if (m_chan->band == s_chan->band &&
				    m_chan->center_freq == s_chan->center_freq &&
				    m_chan->dfs_state == s_chan->dfs_state) {
					ret = RS_EXIST;
				} else {
					chan_info->chan_def = *chandef;
					vif_priv->ch_index = ch_idx;
					ret = RS_SUCCESS;
				}
			}
		}
	}

	return ret;
}

rs_ret rs_net_cfg80211_unset_chaninfo(struct rs_net_vif_priv *vif_priv)
{
	rs_ret ret = RS_FAIL;
	struct rs_net_cfg80211_priv *net_priv = rs_vif_priv_get_net_priv(vif_priv);
	struct rs_net_chan_info *chan_info = NULL;

	RS_TRACE(RS_FN_ENTRY_STR);

	if ((vif_priv != NULL) && (net_priv != NULL)) {
		if (vif_priv->ch_index < RS_NET_CHANINFO_MAX) {
			chan_info = &(net_priv->chaninfo_table[vif_priv->ch_index]);

			if (chan_info->count == 0) {
				RS_DBG("P:%s[%d]:Chaninfo ref == 0\n", __func__, __LINE__);
			} else {
				chan_info->count--;
			}

			if (chan_info->count == 0) {
				if (vif_priv->ch_index == net_priv->chaninfo_index) {
					rs_net_dfs_detection_enable(&net_priv->dfs, RS_DFS_DETECT_DISABLE,
								    RS_DFS_RIU);
				}

				chan_info->chan_def.chan = NULL;
			}

			vif_priv->ch_index = RS_NET_CH_INIT_VALUE;

			ret = RS_SUCCESS;
		}
	}

	return ret;
}

struct rs_net_chan_info *rs_net_cfg80211_get_chaninfo(struct rs_net_vif_priv *vif_priv)
{
	struct rs_net_cfg80211_priv *net_priv = rs_vif_priv_get_net_priv(vif_priv);
	struct rs_net_chan_info *chan_info = NULL;

	RS_TRACE(RS_FN_ENTRY_STR);

	if ((vif_priv != NULL) && (net_priv != NULL)) {
		if ((vif_priv->ch_index < RS_NET_CHANINFO_MAX) &&
		    (net_priv->chaninfo_table[vif_priv->ch_index].chan_def.chan != NULL)) {
			chan_info = &(net_priv->chaninfo_table[vif_priv->ch_index]);
		}
	}

	return chan_info;
}

void rs_net_cfg80211_wapi_enable(struct wiphy *wiphy)
{
	net_cipher_suites[wiphy->n_cipher_suites] = WLAN_CIPHER_SUITE_SMS4;
	wiphy->n_cipher_suites++;
	wiphy->flags |= WIPHY_FLAG_CONTROL_PORT_PROTOCOL;
}

void rs_net_cfg80211_mfp_enable(struct wiphy *wiphy)
{
	net_cipher_suites[wiphy->n_cipher_suites] = WLAN_CIPHER_SUITE_AES_CMAC;
	wiphy->n_cipher_suites++;
}

void rs_net_cfg80211_ccmp256_enable(struct wiphy *wiphy)
{
	net_cipher_suites[wiphy->n_cipher_suites++] = WLAN_CIPHER_SUITE_CCMP_256;
}

void rs_net_cfg80211_gcmp_enable(struct wiphy *wiphy)
{
	net_cipher_suites[wiphy->n_cipher_suites++] = WLAN_CIPHER_SUITE_GCMP;
	net_cipher_suites[wiphy->n_cipher_suites++] = WLAN_CIPHER_SUITE_GCMP_256;
}

rs_ret rs_net_cfg80211_scan_done(struct rs_c_if *c_if)
{
	struct rs_net_cfg80211_priv *net_priv = NULL;

	RS_TRACE(RS_FN_ENTRY_STR);

	net_priv = (struct rs_net_cfg80211_priv *)rs_c_if_get_net_priv(c_if);
	if (net_priv != NULL) {
		if (net_priv->scan_request) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)
			struct cfg80211_scan_info info = {
				.aborted = false,
			};

			cfg80211_scan_done(net_priv->scan_request, &info);
#else
			cfg80211_scan_done(net_priv->scan_request, false);
#endif
		}

		net_priv->scan_request = NULL;
	}

	c_if->core->scan = 0;

	return 0;
}

rs_ret rs_net_cfg80211_scan_stop_and_wait(struct rs_c_if *c_if, struct rs_net_vif_priv *vif_priv)
{
	rs_ret ret = RS_FAIL;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	s32 count = 100; // 1 sec

	net_priv = (struct rs_net_cfg80211_priv *)rs_c_if_get_net_priv(c_if);

	if ((c_if != NULL) && (net_priv != NULL) && (vif_priv != NULL) && (net_priv->scan_request != NULL) &&
	    (rs_net_vif_is_up(vif_priv) == RS_SUCCESS)) {
		(void)rs_net_ctrl_scan_cancel(c_if, vif_priv);
		while ((net_priv->scan_request != NULL) && (count > 0)) {
			msleep(10);
			count--;
		}

		(void)rs_net_cfg80211_scan_done(c_if);
	}

	return ret;
}
