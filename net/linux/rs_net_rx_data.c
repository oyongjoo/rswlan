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
#include "rs_k_mem.h"
#include "rs_c_dbg.h"
#include "rs_c_if.h"
#include "rs_core.h"
#include "rs_c_status.h"

#include "rs_net_cfg80211.h"
#include "rs_net_priv.h"
#include "rs_net_dev.h"
#include "rs_net_skb.h"

#include "rs_net_rx_data.h"
#include "rs_net_stats.h"

////////////////////////////////////////////////////////////////////////////////
/// MACRO DEFINITION

////////////////////////////////////////////////////////////////////////////////
/// TYPE DEFINITION

////////////////////////////////////////////////////////////////////////////////
/// LOCAL VARIABLE

////////////////////////////////////////////////////////////////////////////////
/// LOCAL FUNCTION

static rs_ret net_rx_update_stats(struct rs_net_cfg80211_priv *net_priv, struct rs_net_vif_priv *vif_priv,
				  u8 sta_idx, u8 pkt_t, struct rs_c_rx_data *rx_data, rs_ret ret_succ)
{
	rs_ret ret = RS_FAIL;
	struct net_device *ndev = NULL;
	struct rs_net_sta_priv *sta = NULL;

	ndev = rs_vif_priv_get_ndev(vif_priv);
	if ((net_priv) && (ndev) && (rx_data)) {
		sta = rs_net_priv_get_sta_info(net_priv, sta_idx);
		if (sta) {
			sta->stats.last_acttive_time = jiffies;
			sta->stats.last_rx_data_ext = rx_data->ext_hdr;

			if (sta->stats.last_rx_data_ext.format_mod > 1) {
				sta->stats.last_stats = rx_data->ext_hdr;
			}
		}

		if (ret_succ == RS_SUCCESS) {
			ndev->stats.rx_bytes += rx_data->data_len;
			ndev->stats.rx_packets++;
			if (pkt_t == PACKET_MULTICAST) {
				ndev->stats.multicast++;
			}
		} else {
			ndev->stats.rx_dropped++;
		}

#ifdef CONFIG_DBG_STATS
		rs_net_rx_status_update(net_priv, &sta->stats);
#endif

		ret = RS_SUCCESS;
	}

	return ret;
}

static rs_ret net_rx_mgmt_set(struct rs_net_cfg80211_priv *net_priv, struct rs_c_rx_data *rx_data, s16 index)
{
	rs_ret ret = RS_SUCCESS;
	struct net_device *ndev = NULL;
	struct wireless_dev *wdev = NULL;
	struct wiphy *wiphy = NULL;
	struct ieee80211_mgmt *mgmt_frame = NULL;
	struct rs_net_vif_priv *vif_priv = NULL;
	s16 vif_index = -1;

	mgmt_frame = (struct ieee80211_mgmt *)(rx_data->data);
	if (!mgmt_frame) {
		RS_ERR("mgmt_frame is null!!\n");
		return RS_FAIL;
	}

	vif_priv = rs_net_priv_get_vif_priv(net_priv, index);
	ndev = rs_vif_priv_get_ndev(vif_priv);
	wdev = rs_vif_priv_get_wdev(vif_priv);
	vif_index = index;
	wiphy = rs_net_priv_get_wiphy(net_priv);

	if ((vif_priv) && (ndev) && (wdev)) {
		s16 sta_index = -1;
		s16 priority = 0;

		sta_index = rx_data->ext_hdr.sta_idx;
		priority = rx_data->ext_hdr.priority;

		if ((RS_NET_WDEV_IF_TYPE(vif_priv) == NL80211_IFTYPE_AP) ||
		    (RS_NET_WDEV_IF_TYPE(vif_priv) == NL80211_IFTYPE_AP_VLAN) ||
		    (RS_NET_WDEV_IF_TYPE(vif_priv) == NL80211_IFTYPE_P2P_CLIENT) ||
		    (RS_NET_WDEV_IF_TYPE(vif_priv) == NL80211_IFTYPE_P2P_GO)) {
			if (ieee80211_is_beacon(mgmt_frame->frame_control) != 0) {
				RS_DBG("P:%s[%d]:beacon:frame_control[0x%X]:vif[%d][%p]\n", __func__,
				       __LINE__, mgmt_frame->frame_control, vif_index, vif_priv);

				cfg80211_report_obss_beacon(wiphy, rx_data->data, rx_data->data_len,
							    rx_data->ext_hdr.prim20_freq,
							    rx_data->ext_hdr.rssi1);

			} else if ((ieee80211_is_deauth(mgmt_frame->frame_control) ||
				    ieee80211_is_disassoc(mgmt_frame->frame_control)) &&
				   (mgmt_frame->u.deauth.reason_code ==
					    WLAN_REASON_CLASS2_FRAME_FROM_NONAUTH_STA ||
				    mgmt_frame->u.deauth.reason_code ==
					    WLAN_REASON_CLASS3_FRAME_FROM_NONASSOC_STA)) {
				RS_DBG("P:%s[%d]:deauth:frame_control[0x%X]:vif[%d][%p]\n", __func__,
				       __LINE__, mgmt_frame->frame_control, vif_index, vif_priv);

				cfg80211_rx_unprot_mlme_mgmt(ndev, rx_data->data, rx_data->data_len);

			} else {
				RS_DBG("P:%s[%d]:frame_control[0x%X]:vif[%d][%p]:acat[%d]\n", __func__,
				       __LINE__, mgmt_frame->frame_control, vif_index, vif_priv,
				       mgmt_frame->u.action.category);

				cfg80211_rx_mgmt(wdev, rx_data->ext_hdr.prim20_freq, rx_data->ext_hdr.rssi1,
						 rx_data->data, rx_data->data_len, 0);
			}
		} else if (RS_NET_WDEV_IF_TYPE(vif_priv) == NL80211_IFTYPE_STATION) {
			if (ieee80211_is_action(mgmt_frame->frame_control) &&
			    mgmt_frame->u.action.category == 6) {
				struct cfg80211_ft_event_params ft_event;
				u8 *action_frame = (u8 *)&mgmt_frame->u.action;
				u8 action_code = action_frame[1];
				u16 status_code = *((u16 *)&action_frame[2 + 2 * ETH_ALEN]);

				RS_DBG("P:%s[%d]:STA:ieee80211_is_action[0x%X]:vif[%d][%p]\n", __func__,
				       __LINE__, mgmt_frame->frame_control, vif_index, vif_priv);

				if ((action_code == 2) && (status_code == 0)) {
					ft_event.target_ap = action_frame + 2 + ETH_ALEN;
					ft_event.ies = action_frame + 2 + 2 * ETH_ALEN + 2;
					ft_event.ies_len =
						rx_data->data_len - (ft_event.ies - (u8 *)mgmt_frame);
					ft_event.ric_ies = NULL;
					ft_event.ric_ies_len = 0;
					cfg80211_ft_event(vif_priv->ndev, &ft_event);
					vif_priv->sta.flags |= FT_OVER_DS;
					rs_k_memcpy(vif_priv->sta.ft_target_ap, ft_event.target_ap, ETH_ALEN);
				}
			} else if ((ieee80211_is_deauth(mgmt_frame->frame_control) ||
				    ieee80211_is_disassoc(mgmt_frame->frame_control)) &&
				   ((mgmt_frame->u.deauth.reason_code ==
				     WLAN_REASON_CLASS2_FRAME_FROM_NONAUTH_STA) ||
				    (mgmt_frame->u.deauth.reason_code ==
				     WLAN_REASON_CLASS3_FRAME_FROM_NONASSOC_STA))) {
				RS_DBG("P:%s[%d]:STA:ieee80211_is_deauth[0x%X]:vif[%d]:reason[%d]:freq[%d]:rssi[%d]\n",
				       __func__, __LINE__, mgmt_frame->frame_control, vif_index,
				       mgmt_frame->u.deauth.reason_code, rx_data->ext_hdr.prim20_freq,
				       rx_data->ext_hdr.rssi1);

				cfg80211_rx_unprot_mlme_mgmt(ndev, rx_data->data, rx_data->data_len);
			} else {
				RS_DBG("P:%s[%d]:STA:frame_ctrl[0x%X]:vif[%d][%p]:freq[%d]:rssi[%d]\n",
				       __func__, __LINE__, mgmt_frame->frame_control, vif_index, vif_priv,
				       rx_data->ext_hdr.prim20_freq, rx_data->ext_hdr.rssi1);

				cfg80211_rx_mgmt(wdev, rx_data->ext_hdr.prim20_freq, rx_data->ext_hdr.rssi1,
						 rx_data->data, rx_data->data_len, 0);
			}
		}
	} else {
		RS_DBG("P:%s[%d]:data_len[%d]:frame_control[0x%X]:vif[%d][%p]\n", __func__, __LINE__,
		       rx_data->data_len, mgmt_frame->frame_control, vif_index, vif_priv);

		ret = RS_FAIL;
	}

	return ret;
}

static rs_ret net_rx_mgmt(struct rs_net_cfg80211_priv *net_priv, struct rs_c_rx_data *rx_data)
{
	rs_ret ret = RS_SUCCESS;
	struct rs_net_vif_priv *vif_priv = NULL;
	s16 sta_index = -1;
	s16 vif_index = -1;
	u16 i = 0;

	sta_index = rx_data->ext_hdr.sta_idx;
	vif_index = rx_data->ext_hdr.vif_idx;

	if (vif_index == RS_NET_INVALID_VIF_IDX) {
		for (i = 0; i < RS_NET_PRIV_VIF_TABLE_MAX; i++) {
			vif_priv = net_priv->vif_table[i];
			if (vif_priv && vif_priv->up) {
				vif_index = vif_priv->vif_index;
				(void)net_rx_mgmt_set(net_priv, rx_data, vif_index);
			}
		}
	} else {
		ret = net_rx_mgmt_set(net_priv, rx_data, vif_index);
	}

	(void)net_rx_update_stats(net_priv, vif_priv, sta_index, PACKET_HOST, rx_data, ret);

	return ret;
}

static rs_ret net_rx_sta(struct rs_net_cfg80211_priv *net_priv, struct rs_c_rx_data *rx_data)
{
	rs_ret ret = RS_FAIL;
	struct rs_net_vif_priv *vif_priv = NULL;
	struct net_device *ndev = NULL;
	struct sk_buff *skb = NULL;
	u8 *skb_data_buff = NULL;
	s16 sta_index = -1;
	s16 vif_index = -1;
	s16 priority = 0;
	s16 pkt_type = 0;

	sta_index = rx_data->ext_hdr.sta_idx;
	vif_index = rx_data->ext_hdr.vif_idx;
	priority = rx_data->ext_hdr.priority;

	vif_priv = rs_net_priv_get_vif_priv(net_priv, vif_index);
	ndev = rs_vif_priv_get_ndev(vif_priv);

	if (vif_priv) {
		skb = (struct sk_buff *)rs_net_skb_alloc(rx_data->data_len);
		if (skb) {
			// set skb
			skb->dev = ndev;
			(void)rs_net_skb_get_data((u8 *)skb, &skb_data_buff);
			if (skb->dev && skb_data_buff) {
				(void)rs_k_memcpy(skb_data_buff, rx_data->data, rx_data->data_len);
				skb->priority = priority;
				skb->protocol = eth_type_trans(skb, skb->dev);
				pkt_type = skb->pkt_type;

				ret = rs_net_dev_rx(ndev, (u8 *)skb);

				(void)net_rx_update_stats(net_priv, vif_priv, sta_index, pkt_type, rx_data,
							  ret);
			}
		}
	} else {
		RS_DBG("P:%s[%d]:sta[%d]:vif[%d]:data_len[%d]\n", __func__, __LINE__, sta_index, vif_index,
		       rx_data->data_len);
	}

	return ret;
}

// static rs_ret net_rx_mon(struct rs_c_if *c_if, struct rs_c_data *rx_data)
// {
//	rs_ret ret = RS_FAIL;

//	return ret;
// }

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL FUNCTION

// RX Data handler
rs_ret rs_net_rx_data(struct rs_c_if *c_if, struct rs_c_rx_data *rx_data)
{
	rs_ret ret = RS_FAIL;
	struct rs_net_cfg80211_priv *net_priv = NULL;

	RS_TRACE(RS_FN_ENTRY_STR);

	net_priv = rs_c_if_get_net_priv(c_if);

	if (net_priv && rx_data) {
		if (rx_data->ext_len == RS_C_RX_EXT_LEN && rx_data->data_len > 0) {
			if (rx_data->ext_hdr.mpdu == 1) {
				ret = net_rx_mgmt(net_priv, rx_data);
			} else {
				ret = net_rx_sta(net_priv, rx_data);
			}
			rs_c_dbg_stat.rx.nb_recv++;
		} else {
			RS_DBG("P:%s[%d]:[%d]:elen[%d]:dlen[%d]\n", __func__, __LINE__, ret, rx_data->ext_len,
			       rx_data->data_len);
			rs_c_dbg_stat.rx.nb_err_len++;
		}
	}

	return ret;
}
