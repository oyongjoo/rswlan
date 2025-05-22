// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Copyright (C) [2022-2025] Renesas Electronics Corporation and/or its
 * affiliates.
 */

////////////////////////////////////////////////////////////////////////////////
/// INCLUDE

#include <linux/version.h>
#include <linux/module.h>
#include <net/cfg80211.h>

#include "rs_type.h"
#include "rs_c_dbg.h"
#include "rs_k_mem.h"
#include "rs_c_dbg.h"
#include "rs_c_if.h"
#include "rs_core.h"
#include "rs_c_cmd.h"
#include "rs_c_recovery.h"
#include "rs_net_params.h"

#include "rs_net_cfg80211.h"
#include "rs_net_priv.h"
#include "rs_net_dev.h"
#include "rs_net_ctrl.h"
#include "rs_net_tx_data.h"

#include "rs_net_rx_indi.h"
#include "rs_net_dfs.h"
#include "rs_net_stats.h"

////////////////////////////////////////////////////////////////////////////////
/// MACRO DEFINITION

////////////////////////////////////////////////////////////////////////////////
/// TYPE DEFINITION

const s32 chnl2bw[] = {
	[PHY_CHNL_BW_20] = NL80211_CHAN_WIDTH_20,	[PHY_CHNL_BW_40] = NL80211_CHAN_WIDTH_40,
	[PHY_CHNL_BW_80] = NL80211_CHAN_WIDTH_80,	[PHY_CHNL_BW_160] = NL80211_CHAN_WIDTH_160,
	[PHY_CHNL_BW_80P80] = NL80211_CHAN_WIDTH_80P80,
};

////////////////////////////////////////////////////////////////////////////////
/// LOCAL VARIABLE

////////////////////////////////////////////////////////////////////////////////
/// LOCAL FUNCTION

static inline s32 net_freq_to_idx(struct rs_net_cfg80211_priv *net_priv, s32 freq)
{
	struct ieee80211_supported_band *sband;
	s32 band, ch, idx = 0;

	for (band = NL80211_BAND_2GHZ; band < NUM_NL80211_BANDS; band++) {
		sband = net_priv->wiphy->bands[band];
		if (!sband) {
			continue;
		}

		for (ch = 0; ch < sband->n_channels; ch++, idx++) {
			if (sband->channels[ch].center_freq == freq) {
				goto exit;
			}
		}
	}

	// BUG_ON(1);

exit:
	return idx;
}

static inline s32 net_channel_survey(struct rs_c_if *c_if, struct rs_c_indi *indi_data)
{
	struct rs_c_sc_survey_info *ind = (struct rs_c_sc_survey_info *)indi_data->data;
	// Get the channel index
	s32 idx = 0;
	// Get the survey
	struct rs_c_survey_info *survey = NULL;
	struct rs_net_cfg80211_priv *net_priv = NULL;

	RS_DBG(RS_FN_ENTRY_STR);

	net_priv = (struct rs_net_cfg80211_priv *)rs_c_if_get_net_priv(c_if);
	if (!net_priv)
		return -1;

	idx = net_freq_to_idx(net_priv, ind->freq);
	survey = &net_priv->survey_table[idx];

	// Store the received parameters
	survey->chan_dwell_ms = ind->chan_dwell_ms;
	survey->chan_busy_ms = ind->chan_busy_ms;
	survey->chan_noise_dbm = ind->chan_noise_dbm;
	survey->filled = (SURVEY_INFO_TIME | SURVEY_INFO_TIME_BUSY);

	// #if 1
	//     pr_info("[+ %s/%d] freq % d, time % d ms, busy % d ms,nnoise %d\n",  __func__, __LINE__,
	//             ind->freq, ind->chan_dwell_ms, ind->chan_busy_ms, ind->noise_dbm);
	// #endif
	if (ind->chan_noise_dbm != 0) {
		survey->filled |= SURVEY_INFO_NOISE_DBM;
	}

	return 0;
}

static inline int net_scan_result(struct rs_c_if *c_if, struct rs_c_indi *indi_data)
{
	struct cfg80211_bss *bss = NULL;
	struct ieee80211_channel *chan;
	struct rs_c_sc_result_ind *ind = (struct rs_c_sc_result_ind *)(indi_data->data);
	struct rs_net_cfg80211_priv *net_priv = NULL;

	RS_TRACE(RS_FN_ENTRY_STR);

	net_priv = (struct rs_net_cfg80211_priv *)rs_c_if_get_net_priv(c_if);
	if (!net_priv || !net_priv->wiphy)
		return -1;

	chan = ieee80211_get_channel(net_priv->wiphy, ind->center_freq);

	if (chan)
		bss = cfg80211_inform_bss_frame(net_priv->wiphy, chan, (struct ieee80211_mgmt *)ind->payload,
						ind->length, ind->rssi * 100, GFP_ATOMIC);

	if (bss)
		cfg80211_put_bss(net_priv->wiphy, bss);

	return 0;
}

static inline int net_scan_complete(struct rs_c_if *c_if)
{
	struct rs_net_cfg80211_priv *net_priv = NULL;

	RS_TRACE(RS_FN_ENTRY_STR);

	net_priv = (struct rs_net_cfg80211_priv *)rs_c_if_get_net_priv(c_if);
	if (net_priv != NULL) {
		(void)rs_net_cfg80211_scan_done(c_if);
	}

	return 0;
}

static int net_sm_connect_ind(struct rs_c_if *c_if, struct rs_c_indi *indi_data)
{
	struct rs_c_sm_connect_ind *ind = (struct rs_c_sm_connect_ind *)indi_data->data;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct rs_net_vif_priv *vif_priv = NULL;
	struct net_device *ndev = NULL;
	const u8 *req_ie = NULL, *rsp_ie = NULL;
	const u8 *extcap_ie = NULL;
	const struct ieee_types_extcap *extcap = NULL;
	struct rs_net_chan_info *chan_info = NULL;
	struct cfg80211_roam_info info;

	RS_TRACE(RS_FN_ENTRY_STR);

	net_priv = (struct rs_net_cfg80211_priv *)rs_c_if_get_net_priv(c_if);
	if (!net_priv)
		return -1;

	vif_priv = rs_net_priv_get_vif_priv(net_priv, ind->vif_idx);
	if (rs_net_vif_is_up(vif_priv) != RS_SUCCESS) {
		return -EIO;
	}

	ndev = rs_vif_priv_get_ndev(vif_priv);
	if (rs_net_if_is_up(ndev) != RS_SUCCESS) {
		return -EIO;
	}

	req_ie = (const u8 *)ind->assoc_ie_buf;
	rsp_ie = req_ie + ind->assoc_req_ie_len;

	if (ind->conn_status == RS_SUCCESS) {
		struct rs_net_sta_priv *sta = rs_net_priv_get_sta_info(net_priv, ind->ap_idx);
		struct ieee80211_channel *chan;
		struct cfg80211_chan_def chandef;

		sta->valid = true;
		sta->sta_idx = ind->ap_idx;
		sta->ch_idx = ind->ch_idx;
		sta->vif_idx = ind->vif_idx;
		sta->vlan_idx = sta->vif_idx;
		sta->qos = ind->flag_qos;
		sta->acm = ind->acm_bits;
		sta->aid = ind->assoc_id;
		sta->band = ind->oper_chan.ch_band;
		sta->width = ind->oper_chan.ch_bw;
		sta->center_freq = ind->oper_chan.freq_prim20;
		sta->center_freq1 = ind->oper_chan.freq_cen1;
		sta->center_freq2 = ind->oper_chan.freq_cen2;
		vif_priv->sta.ap = sta;
		vif_priv->generation++;
		chan = ieee80211_get_channel(net_priv->wiphy, ind->oper_chan.freq_prim20);
		cfg80211_chandef_create(&chandef, chan, NL80211_CHAN_NO_HT);
		if (!rs_net_params_get_ht_supported(c_if))
			chandef.width = NL80211_CHAN_WIDTH_20_NOHT;
		else
			chandef.width = chnl2bw[ind->oper_chan.ch_bw];
		chandef.center_freq1 = ind->oper_chan.freq_cen1;
		chandef.center_freq2 = ind->oper_chan.freq_cen2;
		(void)rs_net_cfg80211_set_chaninfo(vif_priv, ind->ch_idx, &chandef);
		rs_k_memcpy(sta->mac_addr, ind->bssid.addr, ETH_ALEN);
		rs_k_memcpy(sta->ac_param, ind->edca_param, sizeof(sta->ac_param));
		extcap_ie = (u8 *)cfg80211_find_ie(WLAN_EID_EXT_CAPABILITY, rsp_ie, ind->assoc_rsp_ie_len);
		if (extcap_ie && extcap_ie[1] >= 5) {
			extcap = (void *)(extcap_ie);
		}
	}

	if (ind->is_roam) {
		rs_k_memset(&info, 0, sizeof(info));

		chan_info = rs_net_cfg80211_get_chaninfo(vif_priv);
		if (chan_info != NULL) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0)
			info.links[0].channel = chan_info->chan_def.chan;
			info.links[0].bssid = (const u8 *)ind->bssid.addr;
#else
			info.channel = chan_info->chan_def.chan;
			info.bssid = (const u8 *)ind->bssid.addr;
#endif
		}
		info.req_ie = req_ie;
		info.req_ie_len = ind->assoc_req_ie_len;
		info.resp_ie = rsp_ie;
		info.resp_ie_len = ind->assoc_rsp_ie_len;
		cfg80211_roamed(ndev, &info, GFP_ATOMIC);
	} else {
		cfg80211_connect_result(ndev, (const u8 *)ind->bssid.addr, req_ie, ind->assoc_req_ie_len,
					rsp_ie, ind->assoc_rsp_ie_len, ind->conn_status, GFP_ATOMIC);
	}

	(void)rs_net_if_tx_stop(c_if, ind->vif_idx, FALSE);
	netif_carrier_on(ndev);

	return 0;
}

const char *connection_loss_reason[3] = { "BEACON_MISS", "PS_TX_MAX_ERR", "FAILED_CHANNEL_CHANGE" };

static int net_sm_disconnect_ind(struct rs_c_if *c_if, struct rs_c_indi *indi_data)
{
	struct rs_c_sm_disconnect_ind *ind = (struct rs_c_sm_disconnect_ind *)indi_data->data;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct rs_net_vif_priv *vif_priv = NULL;

	RS_TRACE(RS_FN_ENTRY_STR);

	net_priv = (struct rs_net_cfg80211_priv *)rs_c_if_get_net_priv(c_if);
	if (!net_priv)
		return -1;

	vif_priv = rs_net_priv_get_vif_priv(net_priv, ind->vif_idx);
	if (rs_net_vif_is_up(vif_priv) == RS_SUCCESS) {
		struct net_device *ndev = rs_vif_priv_get_ndev(vif_priv);

		if (!ind->reassoc) {
			if (ind->reason_code > 100 && ind->reason_code < 104) {
				RS_INFO("[+ %s/%d] connection loss reasn is %s\n", __func__, __LINE__,
					connection_loss_reason[ind->reason_code - 100]);
				ind->reason_code = 1;
			}

#if CONFIG_DBG_STATS
			if (net_priv->dbg_stats.dbg_stats_enabled)
				rs_net_stats_deinit(net_priv);
#endif

			cfg80211_disconnected(ndev, ind->reason_code, NULL, 0, (ind->reason_code <= 1),
					      GFP_ATOMIC);

			if (vif_priv->sta.ft_assoc_ies) {
				rs_k_free(vif_priv->sta.ft_assoc_ies);
				vif_priv->sta.ft_assoc_ies = NULL;
				vif_priv->sta.ft_assoc_ies_len = 0;
			}
		}

		(void)rs_net_if_tx_stop(c_if, ind->vif_idx, TRUE);
		netif_carrier_off(ndev);
	} else {
		return -EIO;
	}

	if (vif_priv->sta.ap) {
		vif_priv->sta.ap->valid = false;
		vif_priv->sta.ap = NULL;
	}
	vif_priv->generation++;

	(void)rs_net_cfg80211_unset_chaninfo(vif_priv);

	return 0;
}

static int net_me_mic_failure_ind(struct rs_c_if *c_if, struct rs_c_indi *indi_data)
{
	struct rs_c_me_mic_failure_ind *ind = (struct rs_c_me_mic_failure_ind *)indi_data->data;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct rs_net_vif_priv *vif_priv = NULL;
	struct net_device *ndev = NULL;

	RS_TRACE(RS_FN_ENTRY_STR);

	net_priv = (struct rs_net_cfg80211_priv *)rs_c_if_get_net_priv(c_if);
	if (!net_priv)
		return -1;

	vif_priv = rs_net_priv_get_vif_priv(net_priv, ind->vif_idx);
	if (rs_net_vif_is_up(vif_priv) != RS_SUCCESS) {
		return -EIO;
	}

	ndev = rs_vif_priv_get_ndev(vif_priv);
	if (rs_net_if_is_up(ndev) != RS_SUCCESS) {
		return -EIO;
	}

	if (vif_priv->up) {
		cfg80211_michael_mic_failure(ndev, (u8 *)&ind->mac_addr,
					     (ind->key_type ? NL80211_KEYTYPE_GROUP :
							      NL80211_KEYTYPE_PAIRWISE),
					     ind->keyid, (u8 *)&ind->tsc, GFP_ATOMIC);
	}

	return 0;
}

static int net_twt_setup_ind(struct rs_c_if *c_if, struct rs_c_indi *indi_data)
{
	struct twt_setup_ind *ind = (struct twt_setup_ind *)indi_data->data;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct rs_net_sta_priv *sta = NULL;

	RS_TRACE(RS_FN_ENTRY_STR);

	net_priv = (struct rs_net_cfg80211_priv *)rs_c_if_get_net_priv(c_if);
	if (!net_priv)
		return -1;

	sta = rs_net_priv_get_sta_info(net_priv, ind->sta_idx);
	if (!sta)
		return -1;

	rs_k_memcpy(&sta->twt_ind, ind, sizeof(struct twt_setup_ind));

	return 0;
}

static int net_rssi_status_ind(struct rs_c_if *c_if, struct rs_c_indi *indi_data)
{
	struct rs_c_rssi_status_ind *ind = (struct rs_c_rssi_status_ind *)indi_data->data;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct rs_net_vif_priv *vif_priv = NULL;
	struct net_device *ndev = NULL;

	RS_TRACE(RS_FN_ENTRY_STR);

	net_priv = (struct rs_net_cfg80211_priv *)rs_c_if_get_net_priv(c_if);
	if (!net_priv)
		return -1;

	vif_priv = rs_net_priv_get_vif_priv(net_priv, ind->vif_index);
	if (rs_net_vif_is_up(vif_priv) != RS_SUCCESS) {
		return -EIO;
	}

	ndev = rs_vif_priv_get_ndev(vif_priv);
	if (rs_net_if_is_up(ndev) != RS_SUCCESS) {
		return -EIO;
	}

	if (vif_priv->up) {
		cfg80211_cqm_rssi_notify(ndev,
					 (ind->rssi_status ? NL80211_CQM_RSSI_THRESHOLD_EVENT_LOW :
							     NL80211_CQM_RSSI_THRESHOLD_EVENT_HIGH),
					 ind->rssi, GFP_ATOMIC);
	}

	return 0;
}

static inline int net_sm_ext_auth_req_ind(struct rs_c_if *c_if, struct rs_c_indi *indi_data)
{
	struct rs_c_sm_ext_auth_req_ind *ind = (struct rs_c_sm_ext_auth_req_ind *)indi_data->data;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct rs_net_vif_priv *vif_priv = NULL;
	struct cfg80211_external_auth_params params;

	RS_TRACE(RS_FN_ENTRY_STR);

	net_priv = (struct rs_net_cfg80211_priv *)rs_c_if_get_net_priv(c_if);
	if (!net_priv)
		return -1;

	vif_priv = rs_net_priv_get_vif_priv(net_priv, ind->vif_idx);
	if (rs_net_vif_is_up(vif_priv) != RS_SUCCESS) {
		return -EIO;
	}

	params.action = NL80211_EXTERNAL_AUTH_START;
	rs_k_memcpy(params.bssid, ind->bssid.addr, ETH_ALEN);
	params.ssid.ssid_len = ind->ssid.ssid_len;
	rs_k_memcpy(params.ssid.ssid, ind->ssid.ssid,
		    min_t(size_t, ind->ssid.ssid_len, sizeof(params.ssid.ssid)));
	params.key_mgmt_suite = ind->key_mgmt_suite;

	if ((RS_NET_WDEV_IF_TYPE(vif_priv) != NL80211_IFTYPE_STATION) ||
	    cfg80211_external_auth_request(vif_priv->ndev, &params, GFP_ATOMIC)) {
		wiphy_err(net_priv->wiphy, "Failed to start external auth on vif %d", ind->vif_idx);
		rs_net_ctrl_sm_ext_auth_req_rsp(c_if, ind->vif_idx, WLAN_STATUS_UNSPECIFIED_FAILURE);
		return 0;
	}

	vif_priv->sta.flags |= RS_STA_AUTH_EXT;
	return 0;
}

static inline int net_chan_pre_switch_ind(struct rs_c_if *c_if, struct rs_c_indi *indi_data)
{
	struct rs_net_cfg80211_priv *net_priv = NULL;

	RS_TRACE(RS_FN_ENTRY_STR);

	net_priv = (struct rs_net_cfg80211_priv *)rs_c_if_get_net_priv(c_if);
	if (net_priv && indi_data) {
		// TODO: tx traffic stop
	}

	return 0;
}

static inline int net_chan_switch_ind(struct rs_c_if *c_if, struct rs_c_indi *indi_data)
{
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct rs_net_vif_priv *vif_priv = NULL;
	struct rs_c_channel_switch_ind *ind = NULL;
	struct rs_net_remain_on_channel *roc = NULL;
	struct wireless_dev *wdev = NULL;

	RS_TRACE(RS_FN_ENTRY_STR);

	net_priv = (struct rs_net_cfg80211_priv *)rs_c_if_get_net_priv(c_if);

	if (net_priv && indi_data) {
		ind = (struct rs_c_channel_switch_ind *)indi_data->data;

		if (ind->roc_tdls) {
			list_for_each_entry(vif_priv, &net_priv->vifs, list) {
				if (vif_priv->vif_index == ind->vif_index) {
					vif_priv->roc_tdls = true;

					// TODO: tdls start
				}
			}
		} else if (ind->roc_req) {
			roc = net_priv->roc;

			if (roc) {
				wdev = rs_vif_priv_get_wdev(roc->vif);
				if (wdev) {
					if (!roc->internal) {
						cfg80211_ready_on_channel(wdev, (u64)(roc), roc->chan,
									  roc->duration, GFP_ATOMIC);
					}
					roc->on_chan = true;
				}
			} else {
				return -1;
			}

			// TODO: tx traffic start on OFF channel
		} else {
			// TODO: tx traffic start
		}

		net_priv->chaninfo_index = ind->chan_index;
		rs_net_dfs_detection_enabled_on_cur_channel(net_priv);
	}

	return 0;
}

static int net_sm_ft_auth_ind(struct rs_c_if *c_if, struct rs_c_indi *indi_data)
{
	struct rs_c_sm_ft_auth_ind *ind = (struct rs_c_sm_ft_auth_ind *)indi_data->data;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct rs_net_vif_priv *vif_priv = NULL;
	struct cfg80211_ft_event_params ft_event;

	RS_TRACE(RS_FN_ENTRY_STR);

	net_priv = (struct rs_net_cfg80211_priv *)rs_c_if_get_net_priv(c_if);
	if (!net_priv)
		return -1;

	vif_priv = rs_net_priv_get_vif_priv(net_priv, ind->vif_idx);
	if (rs_net_vif_is_up(vif_priv) != RS_SUCCESS) {
		return -EIO;
	}

	ft_event.target_ap = vif_priv->sta.ft_target_ap;
	ft_event.ies = (u8 *)&ind->ft_ie_buf;
	ft_event.ies_len = ind->ft_ie_len;
	ft_event.ric_ies = NULL;
	ft_event.ric_ies_len = 0;
	cfg80211_ft_event(vif_priv->ndev, &ft_event);
	vif_priv->sta.flags |= RS_STA_AUTH_FT_OVER_AIR;

	return 0;
}

static rs_ret net_ap_probe_client_ind(struct rs_c_if *c_if, struct rs_c_indi *indi_data)
{
	struct rs_c_ap_probe_client_ind *ind = (struct rs_c_ap_probe_client_ind *)indi_data->data;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct rs_net_vif_priv *vif_priv = NULL;
	struct rs_net_sta_priv *sta = NULL;

	RS_TRACE(RS_FN_ENTRY_STR);

	net_priv = (struct rs_net_cfg80211_priv *)rs_c_if_get_net_priv(c_if);
	if (!net_priv)
		return -1;

	vif_priv = rs_net_priv_get_vif_priv(net_priv, ind->vif_idx);
	if (rs_net_vif_is_up(vif_priv) != RS_SUCCESS) {
		return -EIO;
	}
	sta = rs_net_priv_get_sta_info(net_priv, ind->sta_idx);

	sta->stats.last_acttive_time = jiffies;
	cfg80211_probe_status(vif_priv->ndev, sta->mac_addr, (u64)ind->probe_id, ind->client_present, 0,
			      false, GFP_ATOMIC);

	return 0;
}

static inline int net_roc_exp_ind(struct rs_c_if *c_if, struct rs_c_indi *indi_data)
{
	struct rs_net_remain_on_channel *roc = NULL;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct rs_net_vif_priv *vif_priv = NULL;

	RS_TRACE(RS_FN_ENTRY_STR);

	net_priv = (struct rs_net_cfg80211_priv *)rs_c_if_get_net_priv(c_if);
	if (!net_priv)
		return -1;
	roc = net_priv->roc;
	if (!roc)
		return -1;

	vif_priv = roc->vif;

	if (!roc->internal && roc->on_chan) {
		cfg80211_remain_on_channel_expired(&vif_priv->wdev, (u64)(roc), roc->chan, GFP_ATOMIC);
	}

	// TODO: offchan deinit

	rs_k_free(roc);
	net_priv->roc = NULL;

	return 0;
}

static inline int net_pktloss_notify_ind(struct rs_c_if *c_if, struct rs_c_indi *indi_data)
{
	struct rs_c_pktloss_ind *ind = (struct rs_c_pktloss_ind *)indi_data->data;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct rs_net_vif_priv *vif_priv = NULL;

	RS_TRACE(RS_FN_ENTRY_STR);

	net_priv = (struct rs_net_cfg80211_priv *)rs_c_if_get_net_priv(c_if);
	if (!net_priv)
		return 1;
	vif_priv = rs_net_priv_get_vif_priv(net_priv, ind->vif_index);
	if (rs_net_vif_is_up(vif_priv) != RS_SUCCESS) {
		return -EIO;
	}

	cfg80211_cqm_pktloss_notify(vif_priv->ndev, (const u8 *)ind->mac_addr.addr, ind->num_packets,
				    GFP_ATOMIC);
	return 0;
}

static inline int net_csa_counter_ind(struct rs_c_if *c_if, struct rs_c_indi *indi_data)
{
	struct rs_c_csa_counter_ind *ind = (struct rs_c_csa_counter_ind *)indi_data->data;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct rs_net_vif_priv *vif_priv = NULL;

	RS_TRACE(RS_FN_ENTRY_STR);

	net_priv = (struct rs_net_cfg80211_priv *)rs_c_if_get_net_priv(c_if);
	if (!net_priv)
		return 1;

	vif_priv = rs_net_priv_get_vif_priv(net_priv, ind->vif_index);
	if (rs_net_vif_is_up(vif_priv) != RS_SUCCESS) {
		return -EIO;
	}

	if (vif_priv->ap.csa)
		vif_priv->ap.csa->count = ind->csa_count;
	else
		netdev_err(vif_priv->ndev, "CSA counter update but no active CSA");

	return 0;
}

static inline int net_csa_finish_ind(struct rs_c_if *c_if, struct rs_c_indi *indi_data)
{
	struct rs_c_csa_finish_ind *ind = (struct rs_c_csa_finish_ind *)indi_data->data;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct rs_net_vif_priv *vif_priv = NULL;

	RS_TRACE(RS_FN_ENTRY_STR);

	net_priv = (struct rs_net_cfg80211_priv *)rs_c_if_get_net_priv(c_if);
	if (!net_priv)
		return 1;

	vif_priv = rs_net_priv_get_vif_priv(net_priv, ind->vif_index);
	if (rs_net_vif_is_up(vif_priv) != RS_SUCCESS) {
		return -EIO;
	}

	if (RS_NET_WDEV_IF_TYPE(vif_priv) == NL80211_IFTYPE_AP ||
	    RS_NET_WDEV_IF_TYPE(vif_priv) == NL80211_IFTYPE_P2P_GO) {
		if (vif_priv->ap.csa) {
			vif_priv->ap.csa->status = ind->status;
			vif_priv->ap.csa->ch_idx = ind->chan_idx;
			schedule_work(&vif_priv->ap.csa->work);
		} else
			netdev_err(vif_priv->ndev, "CSA finish indication but no active CSA");
	} else {
		if (ind->status == 0) {
			(void)rs_net_cfg80211_unset_chaninfo(vif_priv);
			(void)rs_net_cfg80211_set_chaninfo(vif_priv, ind->chan_idx, NULL);
			if (net_priv->chaninfo_index == ind->chan_idx) {
				rs_net_dfs_detection_enabled_on_cur_channel(net_priv);
				// TODO : tx restart
			} else {
				// TODO : tx stop
			}
		}
	}

	return 0;
}

static inline int net_csa_traffic_ind(struct rs_c_if *c_if, struct rs_c_indi *indi_data)
{
	struct rs_c_csa_traffic_ind *ind = (struct rs_c_csa_traffic_ind *)indi_data->data;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct rs_net_vif_priv *vif_priv = NULL;

	RS_TRACE(RS_FN_ENTRY_STR);

	net_priv = (struct rs_net_cfg80211_priv *)rs_c_if_get_net_priv(c_if);
	if (!net_priv)
		return 1;

	vif_priv = rs_net_priv_get_vif_priv(net_priv, ind->vif_index);
	if (rs_net_vif_is_up(vif_priv) != RS_SUCCESS) {
		return -EIO;
	}

	if (ind->enable) {
		// TODO: tx restart
	} else {
		// TODO: tx stop
	}
	return 0;
}

static inline int net_p2p_noa_update_ind(struct rs_c_if *c_if, struct rs_c_indi *indi_data)
{
	struct rs_c_p2p_noa_update_ind *ind = (struct rs_c_p2p_noa_update_ind *)indi_data->data;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct rs_net_vif_priv *vif_priv = NULL;

	RS_TRACE(RS_FN_ENTRY_STR);

	net_priv = (struct rs_net_cfg80211_priv *)rs_c_if_get_net_priv(c_if);
	if (!net_priv)
		return 1;

	vif_priv = rs_net_priv_get_vif_priv(net_priv, ind->vif_index);
	if (rs_net_vif_is_up(vif_priv) != RS_SUCCESS) {
		return -EIO;
	}

	RS_DBG(RS_FN_ENTRY_STR_
	       " noa_idx: %d, type: %d, count: %d, duration: %ld, interval: %ld, start_time: %ld: \n",
	       __func__, ind->noa_ins_idx, ind->noa_type, ind->count, ind->duration_us, ind->interval_us,
	       ind->start_time);

	return 0;
}

#ifdef CONFIG_RS_5G_DFS
static inline int net_radar_detect_ind(struct rs_c_if *c_if, struct rs_c_indi *indi_data)
{
	struct rs_c_dfs_array_ind *ind = (struct rs_c_dfs_array_ind *)indi_data->data;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	int i;

	RS_TRACE(RS_FN_ENTRY_STR);

	net_priv = (struct rs_net_cfg80211_priv *)rs_c_if_get_net_priv(c_if);
	if (!net_priv)
		return -1;
	
	if (ind->cnt == 0) {
		return -1;
	}

	if (rs_net_dfs_detection_enbaled(&net_priv->dfs, ind->idx)) {
		for (i = 0; i < ind->cnt; i++) {
			struct rs_net_dfs_pulse *p = &net_priv->dfs.pulses[ind->idx];

			p->buffer[p->index] = ind->pulse[i];
			p->index = (p->index + 1) % 16;
			if (p->count < 16)
				p->count++;
		}

		if (!work_pending(&net_priv->dfs.dfs_work))
			schedule_work(&net_priv->dfs.dfs_work);
	}

	ind->cnt = 0;

	return 0;
}
#endif 

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL FUNCTION

rs_ret rs_net_rx_indi(struct rs_c_if *c_if, struct rs_c_indi *indi_data)
{
	rs_ret ret = RS_FAIL;

	if (c_if && c_if->core && indi_data && indi_data->data_len >= 0) {
		switch (indi_data->cmd) {
		/// MAC Entity
		case RS_ME_TKIP_MIC_FAILURE_IND:
			RS_DBG(RS_FN_ENTRY_STR_ " : RS_ME_TKIP_MIC_FAILURE_IND\n", __func__);
			net_me_mic_failure_ind(c_if, indi_data);
			break;

		/// SCan
		case RS_SC_RESULT_IND:
			RS_DBG(RS_FN_ENTRY_STR_ " : RS_SC_RESULT_IND\n", __func__);
			net_scan_result(c_if, indi_data);
			break;

		case RS_SC_COMPLETE_IND:
			RS_DBG(RS_FN_ENTRY_STR_ " : RS_SC_COMPLETE_IND\n", __func__);
			net_scan_complete(c_if);
			break;

		case RS_SC_CHANNEL_SURVEY_IND:
			RS_DBG(RS_FN_ENTRY_STR_ " : RS_SC_CHANNEL_SURVEY_IND\n", __func__);
			net_channel_survey(c_if, indi_data);
			break;

		/// Station Manager
		case RS_SM_CONNECT_IND:
			RS_DBG(RS_FN_ENTRY_STR_ " : RS_SM_CONNECT_IND\n", __func__);
			net_sm_connect_ind(c_if, indi_data);
			break;

		case RS_SM_DISCONNECT_IND:
			RS_DBG(RS_FN_ENTRY_STR_ " : RS_SM_DISCONNECT_IND\n", __func__);
			net_sm_disconnect_ind(c_if, indi_data);
			break;

		case RS_SM_FT_AUTH_IND:
			RS_DBG(RS_FN_ENTRY_STR_ " : RS_SM_FT_AUTH_IND\n", __func__);
			net_sm_ft_auth_ind(c_if, indi_data);
			break;

		case RS_SM_EXTERNAL_AUTH_REQUIRED_IND:
			RS_DBG(RS_FN_ENTRY_STR_ " : RS_SM_EXTERNAL_AUTH_REQUIRED_IND\n", __func__);
			net_sm_ext_auth_req_ind(c_if, indi_data);
			break;

		/// TWT
		case RS_TWT_SETUP_IND:
			RS_DBG(RS_FN_ENTRY_STR_ " : RS_TWT_SETUP_IND\n", __func__);
			net_twt_setup_ind(c_if, indi_data);
			break;

		/// Ap Manager
		case RS_AM_PROBE_CLIENT_IND:
			RS_DBG(RS_FN_ENTRY_STR_ " : RS_AM_PROBE_CLIENT_IND\n", __func__);
			net_ap_probe_client_ind(c_if, indi_data);
			break;

		/// MAC Manager
		case RS_MM_CHANNEL_SWITCH_IND:
			RS_DBG(RS_FN_ENTRY_STR_ " : RS_MM_CHANNEL_SWITCH_IND\n", __func__);
			net_chan_switch_ind(c_if, indi_data);
			break;

		case RS_MM_CHANNEL_PRE_SWITCH_IND:
			RS_DBG(RS_FN_ENTRY_STR_ " : RS_MM_CHANNEL_PRE_SWITCH_IND\n", __func__);
			net_chan_pre_switch_ind(c_if, indi_data);
			break;

		case RS_MM_REMAIN_ON_CHANNEL_EXP_IND:
			RS_DBG(RS_FN_ENTRY_STR_ " : RS_MM_REMAIN_ON_CHANNEL_EXP_IND\n", __func__);
			net_roc_exp_ind(c_if, indi_data);
			break;

		case RS_MM_P2P_NOA_UPDATE_IND:
			RS_DBG(RS_FN_ENTRY_STR_ " : RS_MM_P2P_NOA_UPDATE_IND\n", __func__);
			net_p2p_noa_update_ind(c_if, indi_data);
			break;

		case RS_MM_RSSI_STATUS_IND:
			RS_DBG(RS_FN_ENTRY_STR_ " : RS_MM_RSSI_STATUS_IND\n", __func__);
			net_rssi_status_ind(c_if, indi_data);
			break;

		case RS_MM_CSA_COUNTER_IND:
			RS_DBG(RS_FN_ENTRY_STR_ " : RS_MM_CSA_COUNTER_IND\n", __func__);
			net_csa_counter_ind(c_if, indi_data);
			break;

		case RS_MM_CSA_FINISH_IND:
			RS_DBG(RS_FN_ENTRY_STR_ " : RS_MM_CSA_FINISH_IND\n", __func__);
			net_csa_finish_ind(c_if, indi_data);
			break;

		case RS_MM_CSA_TRAFFIC_IND:
			RS_DBG(RS_FN_ENTRY_STR_ " : RS_MM_CSA_TRAFFIC_IND\n", __func__);
			net_csa_traffic_ind(c_if, indi_data);
			break;

		case RS_MM_PACKET_LOSS_IND:
			RS_DBG(RS_FN_ENTRY_STR_ " : RS_MM_PACKET_LOSS_IND\n", __func__);
			net_pktloss_notify_ind(c_if, indi_data);
			break;

		/// TDLS
		case RS_TD_CHAN_SWITCH_IND:
			RS_DBG(RS_FN_ENTRY_STR_ " : RS_TD_CHAN_SWITCH_IND\n", __func__);
			break;

		case RS_TD_CHAN_SWITCH_BASE_CHAN_IND:
			RS_DBG(RS_FN_ENTRY_STR_ " : RS_TD_CHAN_SWITCH_BASE_CHAN_IND\n", __func__);
			break;

		/// DBG
		case RS_DBG_ERROR_IND:
			RS_ERR(RS_FN_ENTRY_STR_ " : RS_DBG_ERROR_IND\n", __func__);
			rs_c_recovery_event_post(c_if);
			break;

#ifdef CONFIG_RS_5G_DFS
		case RS_RADAR_DETECT_IND:
			RS_DBG(RS_FN_ENTRY_STR_ " : RS_RADAR_DETECT_IND\n", __func__);
			net_radar_detect_ind(c_if, indi_data);
			break;
#endif

		default:
			RS_DBG(RS_FN_ENTRY_STR_ " : unknown indi[%d]\n", __func__, indi_data->cmd);
			break;
		}
	}

	return ret;
}
