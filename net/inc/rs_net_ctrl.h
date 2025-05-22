// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Copyright (C) [2022-2025] Renesas Electronics Corporation and/or its
 * affiliates.
 */

#ifndef RS_NET_CTRL_H
#define RS_NET_CTRL_H

////////////////////////////////////////////////////////////////////////////////
/// INCLUDE

#include "rs_type.h"

#include "rs_c_cmd.h"
#include "rs_c_if.h"
#include "rs_net_cfg80211.h"

////////////////////////////////////////////////////////////////////////////////
/// MACRO DEFINITION

#define RS_DBG_ERR_RSP(rsp) RS_ERR(KERN_CRIT "%s: Status Error(%d)\n", #rsp, (rsp)->status)

#define NET_MAX_POWER	    (127)
#define TIME_SYNC_INTERVAL  (60000)

#ifdef CONFIG_DBG_STATS
#define REQ_STATS_TX	RS_BIT(2)
#define REQ_STATS_START RS_BIT(1)
#define REQ_STATS_STOP	RS_BIT(0)
#endif

#define RS_UAPSD_QUEUES                                                            \
	(IEEE80211_WMM_IE_STA_QOSINFO_AC_VO | IEEE80211_WMM_IE_STA_QOSINFO_AC_VI | \
	 IEEE80211_WMM_IE_STA_QOSINFO_AC_BK | IEEE80211_WMM_IE_STA_QOSINFO_AC_BE)
////////////////////////////////////////////////////////////////////////////////
/// TYPE DEFINITION

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL VARIABLE
extern const int chnl2bw[];

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL FUNCTION

static inline u16 net_ctrl_get_chan_flags(u32 flags)
{
	u16 chan_flags = 0;

	if (flags & IEEE80211_CHAN_NO_IR) {
		chan_flags |= CHAN_NO_IR;
	}

	if (flags & IEEE80211_CHAN_RADAR) {
		chan_flags |= (CHAN_RADAR | CHAN_NO_IR);
	}

	return chan_flags;
}

static inline s8 net_get_max_power(s16 power)
{
	return power > NET_MAX_POWER ? NET_MAX_POWER : (s8)power;
}

// Reset H/W device
rs_ret rs_net_ctrl_dev_hw_reset(struct rs_c_if *c_if);

// Reset device
rs_ret rs_net_ctrl_dev_reset(struct rs_c_if *c_if);

// Start device
rs_ret rs_net_ctrl_dev_start(struct rs_c_if *c_if);

// Get MAC Address
rs_ret rs_net_ctrl_get_fw_mac_addr(struct rs_c_if *c_if, u8 *mac_addr);

// Get F/W version
rs_ret rs_net_ctrl_get_fw_ver(struct rs_c_if *c_if, struct rs_c_fw_ver_rsp *rsp_data);

// Add network interface
rs_ret rs_net_ctrl_if_add(struct rs_c_if *c_if, const u8 *mac_addr, u8 iftype, bool p2p,
			  struct rs_c_add_if_rsp *rsp_data);

// Remove network interface
rs_ret rs_net_ctrl_if_remove(struct rs_c_if *c_if, u8 vif_index);

// Set MLME Config
rs_ret rs_net_ctrl_me_config(struct rs_c_if *c_if);

// Set MLME Channel Config
rs_ret rs_net_ctrl_me_chan_config(struct rs_c_if *c_if);

// Start scan
rs_ret rs_net_ctrl_scan_start(struct rs_c_if *c_if, struct cfg80211_scan_request *param);

// Cancel scan
rs_ret rs_net_ctrl_scan_cancel(struct rs_c_if *c_if, struct rs_net_vif_priv *vif_priv);

// connect
rs_ret rs_net_ctrl_connect(struct rs_c_if *c_if, struct net_device *ndev, struct cfg80211_connect_params *sme,
			   struct rs_c_sm_connect_rsp *rsp_data);

rs_ret rs_net_ctrl_disconnect_req(struct rs_c_if *c_if, struct rs_net_vif_priv *vif_priv, u16 reason);

rs_ret rs_net_ctrl_add_key_req(struct rs_c_if *c_if, u8 vif_id, u8 sta_id, bool pairwise, u8 *key,
			       u32 key_len, u8 key_index, u8 cipher_type, struct rs_c_key_add_rsp *rsp_data);

rs_ret rs_net_ctrl_del_key_req(struct rs_c_if *c_if, u8 key_index);

rs_ret rs_c_ctrl_add_station_req(struct rs_c_if *c_if, struct station_parameters *params, const u8 *mac,
				 u8 vif_idx, struct rs_c_sta_add_rsp *rsp_data);

rs_ret rs_net_ctrl_del_station_req(struct rs_c_if *c_if, u8 sta_idx, bool is_tdls_sta);

rs_ret rs_net_ctrl_port_control_req(struct rs_c_if *c_if, bool authorized, u8 sta_idx);

rs_ret rs_net_ctrl_remain_on_channel_req(struct rs_c_if *c_if, struct rs_net_vif_priv *vif_priv,
					 struct ieee80211_channel *chan, int duration);

rs_ret rs_net_ctrl_cancel_remain_on_channel_req(struct rs_c_if *c_if);

rs_ret rs_net_ctrl_ap_start_req(struct rs_c_if *c_if, struct rs_net_vif_priv *vif_priv,
				struct cfg80211_ap_settings *settings, struct rs_c_ap_start_rsp *rsp_data);

rs_ret rs_net_ctrl_ap_stop_req(struct rs_c_if *c_if, struct rs_net_vif_priv *vif_priv);

rs_ret rs_net_ctrl_cac_start_req(struct rs_c_if *c_if, struct rs_net_vif_priv *vif_priv,
				 struct cfg80211_chan_def *chandef, struct rs_c_cas_start_rsp *rsp_data);
rs_ret rs_net_ctrl_cac_stop_req(struct rs_c_if *c_if, struct rs_net_vif_priv *vif_priv);

rs_ret rs_net_ctrl_change_beacon_req(struct rs_c_if *c_if, u8 vif_idx, u8 *bcn, u16 bcn_len, u16 tim_oft,
				     u16 tim_len, u16 *csa_oft);

rs_ret rs_net_ctrl_monitor_mode_req(struct rs_c_if *c_if, struct cfg80211_chan_def *chandef,
				    struct rs_c_mon_mode_rsp *rsp_data);

rs_ret rs_net_ctrl_probe_client_req(struct rs_c_if *c_if, u8 vif_idx, u8 sta_idx,
				    struct rs_c_probe_client_rsp *rsp_data);

rs_ret rs_net_ctrl_set_ap_isolate(struct rs_c_if *c_if, u8 ap_isolate);

rs_ret rs_net_ctrl_edca_req(struct rs_c_if *c_if, u8 ac, u32 param, bool uapsd, u8 vif_idx);

rs_ret rs_net_ctrl_set_tx_power_req(struct rs_c_if *c_if, u8 vif_idx, s8 tx_power);

rs_ret rs_net_ctrl_set_power_mgmt_req(struct rs_c_if *c_if, u8 ps_mode);

rs_ret rs_net_ctrl_ft_auth_req(struct rs_c_if *c_if, struct net_device *ndev, u8 *ie, int ie_len);

rs_ret rs_net_ctrl_cqm_rssi_config_req(struct rs_c_if *c_if, u8 vif_idx, s32 rssi_thold, u32 rssi_hyst);

u8 *net_create_beacon(struct rs_net_beacon_info *bcn, struct cfg80211_beacon_data *new);

rs_ret rs_net_ctrl_mem_read_req(struct rs_c_if *c_if, u32 addr, struct rs_c_mem_read_rsp *rsp_data);
rs_ret rs_net_ctrl_mem_write_req(struct rs_c_if *c_if, u32 addr, u32 value);
rs_ret rs_net_ctrl_dbg_mode_filter_req(struct rs_c_if *c_if, u32 mode);
rs_ret rs_net_ctrl_dbg_level_filter_req(struct rs_c_if *c_if, u32 level);
rs_ret rs_net_ctrl_rf_tx_req(struct rs_c_if *c_if, struct rs_c_rf_tx_req *data);
rs_ret rs_net_ctrl_rf_cw_req(struct rs_c_if *c_if, struct rs_c_rf_cw_req *data);
rs_ret rs_net_ctrl_rf_cont_req(struct rs_c_if *c_if, struct rs_c_rf_cont_req *data);
rs_ret rs_net_ctrl_rf_ch_req(struct rs_c_if *c_if, u16 frequency);
rs_ret rs_net_ctrl_rf_per_req(struct rs_c_if *c_if, u8 start, struct rs_c_rf_per_rsp *rsp_data);

rs_ret rs_net_ctrl_sm_ext_auth_req_rsp(struct rs_c_if *c_if, u8 vif_index, u16 status);

rs_ret rs_net_ctrl_time_sync_with_dev(struct rs_c_if *c_if);
#ifdef CONFIG_DBG_STATS
rs_ret rs_net_ctrl_dbg_stats_tx_req(struct rs_c_if *c_if, u8 req, struct rs_c_dbg_stats_tx_rsp *rsp_data);
#endif
#endif /* RS_NET_CTRL_H */
