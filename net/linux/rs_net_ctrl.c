// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Copyright (C) [2022-2025] Renesas Electronics Corporation and/or its
 * affiliates.
 */

////////////////////////////////////////////////////////////////////////////////
/// INCLUDE

#include <linux/version.h>
#include <linux/module.h>
#include <linux/sched/clock.h>
#include <net/cfg80211.h>

#include "rs_type.h"
#include "rs_k_mem.h"
#include "rs_c_dbg.h"
#include "rs_c_if.h"
#include "rs_c_cmd.h"
#include "rs_c_data.h"
#include "rs_c_ctrl.h"
#include "rs_core.h"

#include "rs_net_cfg80211.h"
#include "rs_net_priv.h"
#include "rs_net_dev.h"
#include "rs_net_params.h"

#include "rs_net_ctrl.h"

////////////////////////////////////////////////////////////////////////////////
/// MACRO DEFINITION

// Assume that rate higher that 54 Mbps are BSS membership
#define IS_BASIC_RATE(r) (r & 0x80) && ((r & ~0x80) <= (54 * 2))

////////////////////////////////////////////////////////////////////////////////
/// TYPE DEFINITION

////////////////////////////////////////////////////////////////////////////////
/// LOCAL VARIABLE

static const s32 bw2chnl[] = {
	[NL80211_CHAN_WIDTH_20_NOHT] = PHY_CHNL_BW_20, [NL80211_CHAN_WIDTH_20] = PHY_CHNL_BW_20,
	[NL80211_CHAN_WIDTH_40] = PHY_CHNL_BW_40,      [NL80211_CHAN_WIDTH_80] = PHY_CHNL_BW_80,
	[NL80211_CHAN_WIDTH_160] = PHY_CHNL_BW_160,    [NL80211_CHAN_WIDTH_80P80] = PHY_CHNL_BW_80P80,
};

////////////////////////////////////////////////////////////////////////////////
/// LOCAL FUNCTION

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL FUNCTION

//////////////////
/// Common Command

static void net_set_channel(const struct cfg80211_chan_def *chandef, struct rs_c_oper_ch_info *chan)
{
	chan->ch_band = chandef->chan->band;
	chan->ch_bw = bw2chnl[chandef->width];
	chan->freq_prim20 = chandef->chan->center_freq;
	chan->freq_cen1 = chandef->center_freq1;
	chan->freq_cen2 = chandef->center_freq2;
	chan->ch_flags = net_ctrl_get_chan_flags(chandef->chan->flags);
	chan->tx_max_pwr = net_get_max_power(chandef->chan->max_power);
}

rs_ret rs_net_ctrl_dev_hw_reset(struct rs_c_if *c_if)
{
	rs_ret ret = RS_FAIL;
	u8 cmd_id = RS_DEV_RESET_CMD;

	RS_TRACE(RS_FN_ENTRY_STR);

	if (c_if) {
		ret = rs_c_ctrl_set(c_if, cmd_id, sizeof(struct rs_c_reset_req), NULL);
	}

	RS_INFO("device reset\n");

	return ret;
}

rs_ret rs_net_ctrl_dev_reset(struct rs_c_if *c_if)
{
	rs_ret ret = RS_FAIL;
	struct rs_c_reset_req req_data = { 0 };
	u8 cmd_id = RS_MM_RESET_CMD;
	u32 bt_coex = 0;
	u64 ts = local_clock();

	RS_TRACE(RS_FN_ENTRY_STR);

	if (c_if) {
		// u64 nsec time to u32 usec time (usec)
		req_data.time = (do_div(ts, 1000000000)) / 1000 + ts * 1000000;
		bt_coex = rs_net_params_get_bt_coex(c_if);
		// Set BT coexistence
		if (bt_coex <= RS_C_BT_COEX_MAX) {
			req_data.bt_coex = bt_coex;
		} else {
			req_data.bt_coex = 0;
			RS_ERR("bt_coex parameter force to 0 !!! (Must 0 to 3)\n");
		}

		ret = rs_c_ctrl_set_and_wait(c_if, cmd_id, sizeof(struct rs_c_reset_req), (u8 *)&req_data,
					     NULL);
		RS_DBG("P:%s[%d]:r[%d]\n", __func__, __LINE__, ret);
	}

	return ret;
}

rs_ret rs_net_ctrl_time_sync_with_dev(struct rs_c_if *c_if)
{
	rs_ret ret = RS_FAIL;
	struct rs_c_time_sync_req req_data = { 0 };
	u8 cmd_id = RS_MM_TIME_SYNC_CMD;
	u64 ts = local_clock();

	RS_TRACE(RS_FN_ENTRY_STR);

	if (c_if) {
		req_data.time = (do_div(ts, 1000000000)) / 1000 + ts * 1000000;
		ret = rs_c_ctrl_set_and_wait(c_if, cmd_id, sizeof(struct rs_c_time_sync_req), (u8 *)&req_data,
					     0);
	}

	return ret;
}

rs_ret rs_net_ctrl_dev_start(struct rs_c_if *c_if)
{
	rs_ret ret = RS_FAIL;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct rs_c_dev_start_req req_data = { 0 };
	u8 cmd_id = RS_MM_START_CMD;

	RS_TRACE(RS_FN_ENTRY_STR);

	net_priv = rs_c_if_get_net_priv(c_if);

	if ((c_if) && (net_priv)) {
		(void)rs_k_memcpy(&req_data.phy_cfg, &net_priv->phy_config, sizeof(struct rs_c_phy_cfg));

		req_data.uapsd_timeout = rs_net_params_get_uapsd_threshold(c_if);
		req_data.lp_clk_accuracy = RS_NET_LPCA_PPM;
		req_data.tx_timeout[0] = 0;
		req_data.tx_timeout[1] = 0;
		req_data.tx_timeout[2] = 0;
		req_data.tx_timeout[3] = 0;
		req_data.rx_hostbuf_size = RS_C_DATA_SIZE;

		ret = rs_c_ctrl_set_and_wait(c_if, cmd_id, sizeof(struct rs_c_dev_start_req), (u8 *)&req_data,
					     NULL);
	}

	return ret;
}

rs_ret rs_net_ctrl_get_fw_mac_addr(struct rs_c_if *c_if, u8 *mac_addr)
{
	rs_ret ret = RS_FAIL;
	struct rs_c_ctrl_rsp *ctrl_rsp_data = NULL;
	u8 cmd_id = RS_DEV_GET_MAC_ADDR_CMD;

	RS_TRACE(RS_FN_ENTRY_STR);

	ctrl_rsp_data = rs_k_calloc(sizeof(struct rs_c_ctrl_rsp));

	if (ctrl_rsp_data) {
		ret = rs_c_ctrl_set_and_wait(c_if, cmd_id, 0, NULL, ctrl_rsp_data);

		if (ret == RS_SUCCESS) {
			if ((ctrl_rsp_data->data[0] == 0) && (ctrl_rsp_data->data[1] == 0) &&
			    (ctrl_rsp_data->data[2] == 0)) {
				ret = RS_FAIL;
			} else {
				if (mac_addr) {
					(void)rs_k_memcpy(mac_addr, ctrl_rsp_data->data, ETH_ADDR_LEN);
				}
			}
		}
	}

	if (ctrl_rsp_data) {
		rs_k_free(ctrl_rsp_data);
	}

	return ret;
}

rs_ret rs_net_ctrl_get_fw_ver(struct rs_c_if *c_if, struct rs_c_fw_ver_rsp *rsp_data)
{
	rs_ret ret = RS_FAIL;
	struct rs_c_ctrl_rsp *ctrl_rsp_data = NULL;
	u8 cmd_id = RS_MM_GET_VER_CMD;

	RS_TRACE(RS_FN_ENTRY_STR);

	ctrl_rsp_data = rs_k_calloc(sizeof(struct rs_c_ctrl_rsp));

	if ((ctrl_rsp_data) && (c_if)) {
		ret = rs_c_ctrl_set_and_wait(c_if, cmd_id, 0, NULL, ctrl_rsp_data);

		if (ret == RS_SUCCESS) {
			if (ctrl_rsp_data->cmd == cmd_id) {
				if (rsp_data) {
					(void)rs_k_memcpy(rsp_data, ctrl_rsp_data->data,
							  sizeof(struct rs_c_fw_ver_rsp));
				} else {
					ret = RS_FAIL;
				}
			}
		}
	}

	if (ctrl_rsp_data) {
		rs_k_free(ctrl_rsp_data);
	}

	return ret;
}

rs_ret rs_net_ctrl_if_add(struct rs_c_if *c_if, const u8 *mac_addr, u8 iftype, bool p2p,
			  struct rs_c_add_if_rsp *rsp_data)
{
	rs_ret ret = RS_FAIL;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct rs_c_add_if_req req_data = { 0 };
	struct rs_c_ctrl_rsp *ctrl_rsp_data = NULL;
	u8 cmd_id = RS_MM_ADD_IF_CMD;

	RS_TRACE(RS_FN_ENTRY_STR);

	net_priv = rs_c_if_get_net_priv(c_if);

	ctrl_rsp_data = rs_k_calloc(sizeof(struct rs_c_ctrl_rsp));

	if ((ctrl_rsp_data) && (c_if) && (net_priv)) {
		(void)rs_k_memcpy(req_data.addr.addr, mac_addr, ETH_ADDR_LEN);

		ret = RS_SUCCESS;

		switch (iftype) {
		case NL80211_IFTYPE_P2P_CLIENT:
			p2p = true;
			req_data.iftype = IF_STA;
			break;
		case NL80211_IFTYPE_STATION:
			req_data.iftype = IF_STA;
			break;
		case NL80211_IFTYPE_ADHOC:
			req_data.iftype = IF_IBSS;
			break;
		case NL80211_IFTYPE_P2P_GO:
			p2p = true;
			req_data.iftype = IF_AP;
			break;
		case NL80211_IFTYPE_AP:
			req_data.iftype = IF_AP;
			break;
		case NL80211_IFTYPE_MESH_POINT:
			req_data.iftype = IF_MESH_POINT;
			break;
		case NL80211_IFTYPE_AP_VLAN:
			ret = RS_FAIL;
			break;
		case NL80211_IFTYPE_MONITOR:
			req_data.iftype = IF_MONITOR;
			req_data.uf = false;
			break;
		default:
			req_data.iftype = IF_STA;
			break;
		}

		if (ret == RS_SUCCESS) {
			req_data.p2p = p2p;

			ret = rs_c_ctrl_set_and_wait(c_if, cmd_id, sizeof(struct rs_c_add_if_req),
						     (u8 *)&req_data, ctrl_rsp_data);

			if (ret == RS_SUCCESS) {
				if (ctrl_rsp_data->cmd == cmd_id) {
					if (rsp_data) {
						(void)rs_k_memcpy(rsp_data, ctrl_rsp_data->data,
								  sizeof(struct rs_c_add_if_rsp));
					} else {
						ret = RS_FAIL;
					}
				}
			}
		}
	}

	if (ctrl_rsp_data) {
		rs_k_free(ctrl_rsp_data);
	}

	return ret;
}

rs_ret rs_net_ctrl_if_remove(struct rs_c_if *c_if, u8 vif_index)
{
	rs_ret ret = RS_FAIL;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct rs_c_remove_if_req req_data = { 0 };
	u8 cmd_id = RS_MM_RM_IF_CMD;

	RS_TRACE(RS_FN_ENTRY_STR);

	net_priv = rs_c_if_get_net_priv(c_if);

	if ((c_if) && (net_priv)) {
		req_data.vif_index = vif_index;

		ret = rs_c_ctrl_set_and_wait(c_if, cmd_id, sizeof(struct rs_c_remove_if_req), (u8 *)&req_data,
					     NULL);
	}

	return ret;
}

///////////////////
/// FullMAC Command

rs_ret rs_net_ctrl_me_config(struct rs_c_if *c_if)
{
	rs_ret ret = RS_FAIL;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct wiphy *wiphy = NULL;
	struct ieee80211_sta_ht_cap *ht_cap = NULL;
	struct ieee80211_sta_vht_cap *vht_cap = NULL;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 20, 0)
	struct ieee80211_sta_he_cap const *he_cap = NULL;
#endif
	u8 *ht_mcs = NULL;
	s32 i = 0;

	struct rs_c_me_config_req req_data = { 0 };
	u8 cmd_id = RS_ME_CONFIG_CMD;

	RS_TRACE(RS_FN_ENTRY_STR);

	net_priv = rs_c_if_get_net_priv(c_if);
	wiphy = rs_net_priv_get_wiphy(net_priv);

	if ((c_if) && (net_priv) && (wiphy)) {
		ret = RS_SUCCESS;

		if (wiphy->bands[NL80211_BAND_5GHZ]) {
			ht_cap = &wiphy->bands[NL80211_BAND_5GHZ]->ht_cap;
			vht_cap = &wiphy->bands[NL80211_BAND_5GHZ]->vht_cap;
		} else if (wiphy->bands[NL80211_BAND_2GHZ]) {
			ht_cap = &wiphy->bands[NL80211_BAND_2GHZ]->ht_cap;
			vht_cap = NULL;
		} else {
			ret = RS_FAIL;
		}
		if (ret == RS_SUCCESS) {
			ht_mcs = (u8 *)&ht_cap->mcs;

			req_data.ht_supp = ht_cap->ht_supported;
			if (vht_cap) {
				req_data.vht_supp = vht_cap->vht_supported;
			} else {
				req_data.vht_supp = FALSE;
			}
			req_data.ht_cap.ht_capa_info = cpu_to_le16(ht_cap->cap);
			req_data.ht_cap.a_mpdu_param =
				ht_cap->ampdu_factor |
				(ht_cap->ampdu_density << IEEE80211_HT_AMPDU_PARM_DENSITY_SHIFT);
			for (i = 0; i < sizeof(ht_cap->mcs); i++)
				req_data.ht_cap.mcs_rate[i] = ht_mcs[i];
			req_data.ht_cap.ht_extended_capa = 0;
			req_data.ht_cap.tx_beamforming_capa = 0;
			req_data.ht_cap.asel_capa = 0;

			if (vht_cap) {
				req_data.vht_cap.vht_capa_info = cpu_to_le32(vht_cap->cap);
				req_data.vht_cap.rx_highest = cpu_to_le16(vht_cap->vht_mcs.rx_highest);
				req_data.vht_cap.rx_mcs_map = cpu_to_le16(vht_cap->vht_mcs.rx_mcs_map);
				req_data.vht_cap.tx_highest = cpu_to_le16(vht_cap->vht_mcs.tx_highest);
				req_data.vht_cap.tx_mcs_map = cpu_to_le16(vht_cap->vht_mcs.tx_mcs_map);
			}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 20, 0)
			if (wiphy->bands[NL80211_BAND_5GHZ] && wiphy->bands[NL80211_BAND_5GHZ]->iftype_data) {
				he_cap = &wiphy->bands[NL80211_BAND_5GHZ]->iftype_data->he_cap;

				req_data.he_supp = he_cap->has_he;
				for (i = 0; i < ARRAY_SIZE(he_cap->he_cap_elem.mac_cap_info); i++) {
					req_data.he_cap.mac_cap_info[i] = he_cap->he_cap_elem.mac_cap_info[i];
				}
				for (i = 0; i < ARRAY_SIZE(he_cap->he_cap_elem.phy_cap_info); i++) {
					req_data.he_cap.phy_cap_info[i] = he_cap->he_cap_elem.phy_cap_info[i];
				}
				req_data.he_cap.mcs_supp.rx_mcs_80 =
					cpu_to_le16(he_cap->he_mcs_nss_supp.rx_mcs_80);
				req_data.he_cap.mcs_supp.tx_mcs_80 =
					cpu_to_le16(he_cap->he_mcs_nss_supp.tx_mcs_80);
				req_data.he_cap.mcs_supp.rx_mcs_160 =
					cpu_to_le16(he_cap->he_mcs_nss_supp.rx_mcs_160);
				req_data.he_cap.mcs_supp.tx_mcs_160 =
					cpu_to_le16(he_cap->he_mcs_nss_supp.tx_mcs_160);
				req_data.he_cap.mcs_supp.rx_mcs_80p80 =
					cpu_to_le16(he_cap->he_mcs_nss_supp.rx_mcs_80p80);
				req_data.he_cap.mcs_supp.tx_mcs_80p80 =
					cpu_to_le16(he_cap->he_mcs_nss_supp.tx_mcs_80p80);
				for (i = 0; i < HE_PPE_THRES_MAX; i++) {
					req_data.he_cap.ppe_thres[i] = he_cap->ppe_thres[i];
				}
				req_data.he_ul_on = rs_net_params_get_he_ul_on(c_if);
			} else if (wiphy->bands[NL80211_BAND_2GHZ] &&
				   wiphy->bands[NL80211_BAND_2GHZ]->iftype_data) {
				he_cap = &wiphy->bands[NL80211_BAND_2GHZ]->iftype_data->he_cap;

				req_data.he_supp = he_cap->has_he;
				for (i = 0; i < ARRAY_SIZE(he_cap->he_cap_elem.mac_cap_info); i++) {
					req_data.he_cap.mac_cap_info[i] = he_cap->he_cap_elem.mac_cap_info[i];
				}
				for (i = 0; i < ARRAY_SIZE(he_cap->he_cap_elem.phy_cap_info); i++) {
					req_data.he_cap.phy_cap_info[i] = he_cap->he_cap_elem.phy_cap_info[i];
				}
				req_data.he_cap.mcs_supp.rx_mcs_80 =
					cpu_to_le16(he_cap->he_mcs_nss_supp.rx_mcs_80);
				req_data.he_cap.mcs_supp.tx_mcs_80 =
					cpu_to_le16(he_cap->he_mcs_nss_supp.tx_mcs_80);
				req_data.he_cap.mcs_supp.rx_mcs_160 =
					cpu_to_le16(he_cap->he_mcs_nss_supp.rx_mcs_160);
				req_data.he_cap.mcs_supp.tx_mcs_160 =
					cpu_to_le16(he_cap->he_mcs_nss_supp.tx_mcs_160);
				req_data.he_cap.mcs_supp.rx_mcs_80p80 =
					cpu_to_le16(he_cap->he_mcs_nss_supp.rx_mcs_80p80);
				req_data.he_cap.mcs_supp.tx_mcs_80p80 =
					cpu_to_le16(he_cap->he_mcs_nss_supp.tx_mcs_80p80);
				for (i = 0; i < HE_PPE_THRES_MAX; i++) {
					req_data.he_cap.ppe_thres[i] = he_cap->ppe_thres[i];
				}
				req_data.he_ul_on = rs_net_params_get_he_ul_on(c_if);
			}
#endif
			req_data.ps_on = rs_net_params_get_ps(c_if);
			req_data.dpsm = rs_net_params_get_dpsm(c_if);
			req_data.tx_lft = RS_C_TX_LIFETIME_MS;
			req_data.amsdu_tx = 0;
			req_data.ant_div_on = false;
			req_data.phy_bw_max = PHY_CHNL_BW_20;

			// wiphy_info(wiphy,
			// 	   "HT supp %d, VHT supp %d, HE supp %d\n",
			// 	   req_data.ht_supp, req_data.vht_supp,
			// 	   req_data.he_supp);
		}

		if (ret == RS_SUCCESS) {
			ret = rs_c_ctrl_set_and_wait(c_if, cmd_id, sizeof(struct rs_c_me_config_req),
						     (u8 *)&req_data, NULL);
		}
	}

	return ret;
}

rs_ret rs_net_ctrl_me_chan_config(struct rs_c_if *c_if)
{
	rs_ret ret = RS_FAIL;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct wiphy *wiphy = NULL;
	struct ieee80211_supported_band *band = NULL;
	s32 i = 0;

	struct rs_c_me_chan_config_req req_data = { 0 };
	u8 cmd_id = RS_ME_CHAN_CONFIG_CMD;

	RS_TRACE(RS_FN_ENTRY_STR);

	net_priv = rs_c_if_get_net_priv(c_if);
	wiphy = rs_net_priv_get_wiphy(net_priv);

	if (c_if && net_priv && wiphy) {
		ret = RS_SUCCESS;

		req_data.chan2G4_cnt = 0;
		if (wiphy->bands[NL80211_BAND_2GHZ]) {
			band = wiphy->bands[NL80211_BAND_2GHZ];
			for (i = 0; i < band->n_channels; i++) {
				req_data.chan2G4[req_data.chan2G4_cnt].ch_flags = 0;
				if (band->channels[i].flags & IEEE80211_CHAN_DISABLED)
					req_data.chan2G4[req_data.chan2G4_cnt].ch_flags |= CHAN_DISABLED;

				req_data.chan2G4[req_data.chan2G4_cnt].ch_flags |=
					net_ctrl_get_chan_flags(band->channels[i].flags);
				req_data.chan2G4[req_data.chan2G4_cnt].ch_band = NL80211_BAND_2GHZ;
				req_data.chan2G4[req_data.chan2G4_cnt].ch_freq =
					band->channels[i].center_freq;
				req_data.chan2G4[req_data.chan2G4_cnt].tx_max_pwr =
					net_get_max_power(band->channels[i].max_power);
				req_data.chan2G4_cnt++;
				if (req_data.chan2G4_cnt == RS_C_SCAN_CHANNEL_2G)
					break;
			}
		}

		req_data.chan5G_cnt = 0;
		if (wiphy->bands[NL80211_BAND_5GHZ]) {
			band = wiphy->bands[NL80211_BAND_5GHZ];
			for (i = 0; i < band->n_channels; i++) {
				req_data.chan5G[req_data.chan5G_cnt].ch_flags = 0;
				if (band->channels[i].flags & IEEE80211_CHAN_DISABLED)
					req_data.chan5G[req_data.chan5G_cnt].ch_flags |= CHAN_DISABLED;

				req_data.chan5G[req_data.chan5G_cnt].ch_flags |=
					net_ctrl_get_chan_flags(band->channels[i].flags);
				req_data.chan5G[req_data.chan5G_cnt].ch_band = NL80211_BAND_5GHZ;
				req_data.chan5G[req_data.chan5G_cnt].ch_freq = band->channels[i].center_freq;
				req_data.chan5G[req_data.chan5G_cnt].tx_max_pwr =
					net_get_max_power(band->channels[i].max_power);
				req_data.chan5G_cnt++;
				if (req_data.chan5G_cnt == RS_C_SCAN_CHANNEL_5G)
					break;
			}
		}

		if (ret == RS_SUCCESS) {
			ret = rs_c_ctrl_set_and_wait(c_if, cmd_id, sizeof(struct rs_c_me_chan_config_req),
						     (u8 *)&req_data, NULL);
		}
	}

	return ret;
}

rs_ret rs_net_ctrl_scan_start(struct rs_c_if *c_if, struct cfg80211_scan_request *param)
{
	rs_ret ret = RS_FAIL;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct rs_net_vif_priv *vif_priv = NULL;
	struct rs_c_sc_start_req *req_data = NULL;
	u16 req_data_len = 0;
	u16 chan_flags = 0;
	u16 i = 0, j = 0;
	u8 cmd_id = RS_SC_START_CMD;
	u8 mac_addr[ETH_ALEN] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

	RS_TRACE(RS_FN_ENTRY_STR);

	req_data_len = sizeof(struct rs_c_sc_start_req) + param->ie_len;
	net_priv = rs_c_if_get_net_priv(c_if);
	vif_priv = container_of(param->wdev, struct rs_net_vif_priv, wdev);
	req_data = rs_k_calloc(req_data_len);

	if ((c_if) && (net_priv)) {
		(void)rs_k_memcpy(req_data->bssid, mac_addr, ETH_ALEN);

		req_data->vif_idx = vif_priv->vif_index;
		req_data->n_channels = (u8)min_t(int, RS_C_SCAN_CHANNEL_MAX, param->n_channels);
		req_data->n_ssids = (u8)min_t(int, RS_C_SCAN_SSID_MAX, param->n_ssids);
		req_data->no_cck = param->no_cck;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)
		if (param->duration_mandatory)
			req_data->duration = ieee80211_tu_to_usec(param->duration);
#endif

		if (req_data->n_ssids == 0)
			chan_flags |= CHAN_NO_IR;
		for (i = 0; i < req_data->n_ssids; i++) {
			for (j = 0; j < param->ssids[i].ssid_len; j++)
				req_data->ssid[i].ssid[j] = param->ssids[i].ssid[j];
			req_data->ssid[i].ssid_len = param->ssids[i].ssid_len;
		}

		if ((param->ie) && (param->ie_len > 0)) {
			req_data->ie_len = param->ie_len;
			req_data->ie_addr = 0;
			rs_k_memcpy(req_data->ie, param->ie, param->ie_len);
		} else {
			req_data->ie_len = 0;
			req_data->ie_addr = 0;
		}

		for (i = 0; i < req_data->n_channels; i++) {
			struct ieee80211_channel *chan = param->channels[i];

			req_data->chan[i].ch_band = chan->band;
			req_data->chan[i].ch_freq = chan->center_freq;
			req_data->chan[i].ch_flags = chan_flags | net_ctrl_get_chan_flags(chan->flags);
			req_data->chan[i].tx_max_pwr = net_get_max_power(chan->max_reg_power);
		}

		ret = rs_c_ctrl_set_and_wait(c_if, cmd_id, req_data_len, (u8 *)req_data, NULL);
	}

	if (req_data) {
		rs_k_free(req_data);
	}

	return ret;
}

rs_ret rs_net_ctrl_scan_cancel(struct rs_c_if *c_if, struct rs_net_vif_priv *vif_priv)
{
	rs_ret ret = RS_FAIL;
	struct rs_c_sc_cancel_req *req_data = NULL;
	u16 req_data_len = 0;
	u8 cmd_id = RS_SC_CANCEL_CMD;

	req_data_len = sizeof(struct rs_c_sc_cancel_req);
	req_data = rs_k_calloc(req_data_len);

	if ((c_if) && (vif_priv) && (req_data)) {
		req_data->vif_idx = vif_priv->vif_index;

		ret = rs_c_ctrl_set_and_wait(c_if, cmd_id, req_data_len, (u8 *)req_data, NULL);
	}

	if (req_data) {
		rs_k_free(req_data);
	}

	return ret;
}

/**
 * net_update_connect_req -- Return the length of the association request IEs
 *
 * @vif: Vif that received the connection request
 * @sme: Connection info
 *
 * Return the ft_ie_len in case of FT.
 * FT over the air is possible if:
 * - auth_type = AUTOMATIC (if already set to FT then it means FT over DS)
 * - already associated to a FT BSS
 * - Target Mobility domain is the same as the curent one
 *
 * If FT is not possible return ie length of the connection info
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 1, 0)
static int net_update_connect_req(struct rs_net_vif_priv *vif_priv, struct cfg80211_connect_params *sme)
{
	if (vif_priv->sta.ap && vif_priv->sta.ft_assoc_ies && sme->auth_type == NL80211_AUTHTYPE_AUTOMATIC) {
		const struct element *rsne, *fte, *mde, *mde_req;
		int ft_ie_len = 0;

		mde_req = cfg80211_find_elem(WLAN_EID_MOBILITY_DOMAIN, sme->ie, sme->ie_len);
		mde = cfg80211_find_elem(WLAN_EID_MOBILITY_DOMAIN, vif_priv->sta.ft_assoc_ies,
					 vif_priv->sta.ft_assoc_ies_len);
		if (!mde || !mde_req || rs_k_memcmp(mde, mde_req, sizeof(struct element) + mde->datalen)) {
			return sme->ie_len;
		}

		ft_ie_len += sizeof(struct element) + mde->datalen;

		rsne = cfg80211_find_elem(WLAN_EID_RSN, vif_priv->sta.ft_assoc_ies,
					  vif_priv->sta.ft_assoc_ies_len);
		fte = cfg80211_find_elem(WLAN_EID_FAST_BSS_TRANSITION, vif_priv->sta.ft_assoc_ies,
					 vif_priv->sta.ft_assoc_ies_len);

		if (rsne && fte) {
			ft_ie_len += 2 * sizeof(struct element) + rsne->datalen + fte->datalen;
			sme->auth_type = NL80211_AUTHTYPE_FT;
			return ft_ie_len;
		} else if (rsne || fte) {
			RS_WARN("Missing RSNE or FTE element, skip FT over air\n");
		} else {
			sme->auth_type = NL80211_AUTHTYPE_FT;
			return ft_ie_len;
		}
	}
	return sme->ie_len;
}

static void net_copy_connect_ies(struct rs_net_vif_priv *vif_priv, struct rs_c_sm_connect_req *req_data,
				 struct cfg80211_connect_params *sme)
{
	if (sme->auth_type == NL80211_AUTHTYPE_FT && !(vif_priv->sta.flags & RS_STA_AUTH_FT_OVER_DS)) {
		const struct element *rsne, *fte, *mde;
		u8 *pos;

		rsne = cfg80211_find_elem(WLAN_EID_RSN, vif_priv->sta.ft_assoc_ies,
					  vif_priv->sta.ft_assoc_ies_len);
		fte = cfg80211_find_elem(WLAN_EID_FAST_BSS_TRANSITION, vif_priv->sta.ft_assoc_ies,
					 vif_priv->sta.ft_assoc_ies_len);
		mde = cfg80211_find_elem(WLAN_EID_MOBILITY_DOMAIN, vif_priv->sta.ft_assoc_ies,
					 vif_priv->sta.ft_assoc_ies_len);
		pos = (u8 *)req_data->ie_buf;

		// We can use FT over the air
		rs_k_memcpy(&vif_priv->sta.ft_target_ap, sme->bssid, ETH_ALEN);

		if (rsne) {
			rs_k_memcpy(pos, rsne, sizeof(struct element) + rsne->datalen);
			pos += sizeof(struct element) + rsne->datalen;
		}
		rs_k_memcpy(pos, mde, sizeof(struct element) + mde->datalen);
		pos += sizeof(struct element) + mde->datalen;
		if (fte) {
			rs_k_memcpy(pos, fte, sizeof(struct element) + fte->datalen);
			pos += sizeof(struct element) + fte->datalen;
		}

		req_data->ie_len = pos - (uint8_t *)req_data->ie_buf;
	} else {
		rs_k_memcpy(req_data->ie_buf, sme->ie, sme->ie_len);
		req_data->ie_len = sme->ie_len;
	}
}
#else
static int net_update_connect_req(struct rs_net_vif_priv *vif_priv, struct cfg80211_connect_params *sme)
{
	if ((vif_priv->sta.ap) && (vif_priv->sta.ft_assoc_ies) &&
	    (sme->auth_type == NL80211_AUTHTYPE_AUTOMATIC)) {
		const u8 *rsne, *fte, *mde, *mde_req;
		int ft_ie_len = 0;

		mde_req = cfg80211_find_ie(WLAN_EID_MOBILITY_DOMAIN, sme->ie, sme->ie_len);
		mde = cfg80211_find_ie(WLAN_EID_MOBILITY_DOMAIN, vif_priv->sta.ft_assoc_ies,
				       vif_priv->sta.ft_assoc_ies_len);
		if (!mde || !mde_req || rs_k_memcmp(mde, mde_req, *(mde + 1) /*ie_len*/)) {
			return sme->ie_len;
		}

		ft_ie_len += *(mde + 1);

		rsne = cfg80211_find_ie(WLAN_EID_RSN, vif_priv->sta.ft_assoc_ies,
					vif_priv->sta.ft_assoc_ies_len);
		fte = cfg80211_find_ie(WLAN_EID_FAST_BSS_TRANSITION, vif_priv->sta.ft_assoc_ies,
				       vif_priv->sta.ft_assoc_ies_len);

		if (rsne && fte) {
			ft_ie_len += *(rsne + 1) + *(fte + 1);
			sme->auth_type = NL80211_AUTHTYPE_FT;
			return ft_ie_len;
		} else if (rsne || fte) {
			RS_WARN("Missing RSNE or FTE element, skip FT over air\n");
		} else {
			sme->auth_type = NL80211_AUTHTYPE_FT;
			return ft_ie_len;
		}
	}
	return sme->ie_len;
}

static void net_copy_connect_ies(struct rs_net_vif_priv *vif_priv, struct rs_c_sm_connect_req *req_data,
				 struct cfg80211_connect_params *sme)
{
	if ((sme->auth_type == NL80211_AUTHTYPE_FT) && !(vif_priv->sta.flags & RS_STA_AUTH_FT_OVER_DS)) {
		const u8 *rsne, *fte, *mde;
		u8 *pos;

		rsne = cfg80211_find_ie(WLAN_EID_RSN, vif_priv->sta.ft_assoc_ies,
					vif_priv->sta.ft_assoc_ies_len);
		fte = cfg80211_find_ie(WLAN_EID_FAST_BSS_TRANSITION, vif_priv->sta.ft_assoc_ies,
				       vif_priv->sta.ft_assoc_ies_len);
		mde = cfg80211_find_ie(WLAN_EID_MOBILITY_DOMAIN, vif_priv->sta.ft_assoc_ies,
				       vif_priv->sta.ft_assoc_ies_len);
		pos = (u8 *)req_data->ie_buf;

		// We can use FT over the air
		rs_k_memcpy(&vif_priv->sta.ft_target_ap, sme->bssid, ETH_ALEN);

		if (rsne) {
			rs_k_memcpy(pos, rsne, *(rsne + 1));
			pos += *(rsne + 1);
		}
		rs_k_memcpy(pos, mde, *(mde + 1));
		pos += *(mde + 1);
		if (fte) {
			rs_k_memcpy(pos, fte, *(fte + 1));
			pos += *(fte + 1);
		}

		req_data->ie_len = pos - (uint8_t *)req_data->ie_buf;
	} else {
		rs_k_memcpy(req_data->ie_buf, sme->ie, sme->ie_len);
		req_data->ie_len = sme->ie_len;
	}
}
#endif

rs_ret rs_net_ctrl_connect(struct rs_c_if *c_if, struct net_device *ndev, struct cfg80211_connect_params *sme,
			   struct rs_c_sm_connect_rsp *rsp_data)
{
	rs_ret ret = RS_FAIL;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct rs_net_vif_priv *vif_priv = NULL;
	struct rs_c_sm_connect_req *req_data = NULL;
	struct rs_c_ctrl_rsp *ctrl_rsp_data = NULL;
	u8 cmd_id = RS_SM_CONNECT_CMD;
	int i, ie_len;
	u32 req_len = sizeof(struct rs_c_sm_connect_req);
	const struct rs_c_mac_addr mac_addr_bcst = { { 0xFFFF, 0xFFFF, 0xFFFF } };

	RS_TRACE(RS_FN_ENTRY_STR);

	net_priv = rs_c_if_get_net_priv(c_if);
	vif_priv = netdev_priv(ndev);
	ctrl_rsp_data = rs_k_calloc(sizeof(struct rs_c_ctrl_rsp));
	if (!(net_priv && vif_priv && ctrl_rsp_data))
		goto out;

	ie_len = net_update_connect_req(vif_priv, sme);
	req_len += ie_len;
	req_data = rs_k_calloc(req_len);
	if (!req_data) {
		goto alloc_fail;
	}

	/* Set parameters for the USTAM_CONNECT_SEND message */
	if (sme->crypto.n_ciphers_pairwise && (sme->crypto.ciphers_pairwise[0] == WLAN_CIPHER_SUITE_WEP40 ||
					       sme->crypto.ciphers_pairwise[0] == WLAN_CIPHER_SUITE_TKIP ||
					       sme->crypto.ciphers_pairwise[0] == WLAN_CIPHER_SUITE_WEP104)) {
		req_data->flags |= DISABLE_HT;
	}

	if (sme->crypto.cipher_group != 0)
		req_data->flags |= USE_PRIVACY;

	if (sme->crypto.control_port != 0)
		req_data->flags |= CONTROL_PORT_HOST;

	if (sme->crypto.control_port_no_encrypt != 0)
		req_data->flags |= CONTROL_PORT_NO_ENC;

	if (sme->crypto.cipher_group != 0 && sme->crypto.cipher_group != WLAN_CIPHER_SUITE_WEP40 &&
	    sme->crypto.cipher_group != WLAN_CIPHER_SUITE_WEP104) {
		req_data->flags |= USE_PAIRWISE_KEY;
	}

	if (sme->mfp == NL80211_MFP_REQUIRED)
		req_data->flags |= MFP_IN_USE;

	if (rs_net_params_get_amsdu_require_spp(c_if) != 0)
		req_data->flags |= REQUIRE_SPP_AMSDU;

	req_data->ctrl_port_ethertype = sme->crypto.control_port_ethertype;

	if (sme->bssid)
		rs_k_memcpy(&req_data->bssid, sme->bssid, ETH_ALEN);
	else
		req_data->bssid = mac_addr_bcst;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 7, 0)
	if (sme->prev_bssid)
		req_data->flags |= REASSOCIATION;
#else
	if (vif_priv->sta.ap)
		req_data->flags |= REASSOCIATION;
#endif

	if (sme->auth_type == NL80211_AUTHTYPE_FT && (vif_priv->sta.flags & RS_STA_AUTH_FT_OVER_DS)) {
		req_data->flags |= (REASSOCIATION | FT_OVER_DS);
	}

	req_data->vif_idx = vif_priv->vif_index;
	if (sme->channel) {
		req_data->chan.ch_band = sme->channel->band;
		req_data->chan.ch_freq = sme->channel->center_freq;
		req_data->chan.ch_flags = net_ctrl_get_chan_flags(sme->channel->flags);
	} else {
		req_data->chan.ch_freq = (u16)-1;
	}

	for (i = 0; i < sme->ssid_len; i++)
		req_data->ssid.ssid[i] = sme->ssid[i];

	req_data->ssid.ssid_len = sme->ssid_len;
	req_data->listen_interval = 0;
	req_data->dont_wait_bcmc = !(true);

	/* Set auth_type */
	if (sme->auth_type == NL80211_AUTHTYPE_AUTOMATIC)
		req_data->auth_type = WLAN_AUTH_OPEN;
	else if (sme->auth_type == NL80211_AUTHTYPE_OPEN_SYSTEM)
		req_data->auth_type = WLAN_AUTH_OPEN;
	else if (sme->auth_type == NL80211_AUTHTYPE_SHARED_KEY)
		req_data->auth_type = WLAN_AUTH_SHARED_KEY;
	else if (sme->auth_type == NL80211_AUTHTYPE_FT)
		req_data->auth_type = WLAN_AUTH_FT;
	else if (sme->auth_type == NL80211_AUTHTYPE_SAE)
		req_data->auth_type = WLAN_AUTH_SAE;
	else {
		ret = -EINVAL;
		goto invalid;
	}

	net_copy_connect_ies(vif_priv, req_data, sme);

	RS_DBG("P:%s[%d]:ie_len[%d][%d]\n", __func__, __LINE__, ie_len, req_data->ie_len);

	req_data->uapsd_queues = RS_UAPSD_QUEUES;

	ret = rs_c_ctrl_set_and_wait(c_if, cmd_id, req_len, (u8 *)req_data, ctrl_rsp_data);

	if (ret == RS_SUCCESS) {
		if (ctrl_rsp_data->cmd == cmd_id) {
			(void)rs_k_memcpy(rsp_data, ctrl_rsp_data->data, sizeof(struct rs_c_sm_connect_rsp));
		}
	}

invalid:
	if (req_data)
		rs_k_free(req_data);

alloc_fail:
	if (ctrl_rsp_data) {
		rs_k_free(ctrl_rsp_data);
	}

out:
	return ret;
}

rs_ret rs_net_ctrl_disconnect_req(struct rs_c_if *c_if, struct rs_net_vif_priv *vif_priv, u16 reason)
{
	rs_ret ret = RS_FAIL;
	struct rs_c_disconnect_req req_data = { 0 };
	u8 cmd_id = RS_SM_DISCONNECT_CMD;

	RS_TRACE(RS_FN_ENTRY_STR);

	if (c_if) {
		req_data.vif_idx = vif_priv->vif_index;
		req_data.deauth_reason = reason;

		ret = rs_c_ctrl_set_and_wait(c_if, cmd_id, sizeof(struct rs_c_disconnect_req),
					     (u8 *)&req_data, NULL);
	}

	return ret;
}

rs_ret rs_net_ctrl_add_key_req(struct rs_c_if *c_if, u8 vif_id, u8 sta_id, bool pairwise, u8 *key,
			       u32 key_len, u8 key_index, u8 cipher_type, struct rs_c_key_add_rsp *rsp_data)
{
	rs_ret ret = RS_FAIL;
	struct rs_c_ctrl_rsp *ctrl_rsp_data = NULL;
	struct rs_c_key_add_req req_data = { 0 };
	u8 cmd_id = RS_MM_ADD_KEY_CMD;

	RS_TRACE(RS_FN_ENTRY_STR);

	ctrl_rsp_data = rs_k_calloc(sizeof(struct rs_c_ctrl_rsp));

	if ((ctrl_rsp_data) && (c_if)) {
		if (sta_id != 0xFF) {
			req_data.sta_id = sta_id;
		} else {
			req_data.sta_id = sta_id;
			req_data.key_idx = key_index;
		}

		req_data.pairwise = pairwise;
		req_data.vif_id = vif_id;
		req_data.key.length = key_len;
		req_data.cipher_suite = cipher_type;

		rs_k_memcpy(&req_data.key.array[0], key, key_len);

		ret = rs_c_ctrl_set_and_wait(c_if, cmd_id, sizeof(struct rs_c_key_add_req), (u8 *)&req_data,
					     ctrl_rsp_data);

		if (ret == RS_SUCCESS) {
			if (ctrl_rsp_data->cmd == cmd_id) {
				(void)rs_k_memcpy(rsp_data, ctrl_rsp_data->data,
						  sizeof(struct rs_c_key_add_rsp));
			}
		}
	}

	if (ctrl_rsp_data) {
		rs_k_free(ctrl_rsp_data);
	}

	return ret;
}

rs_ret rs_net_ctrl_del_key_req(struct rs_c_if *c_if, u8 key_index)
{
	rs_ret ret = RS_FAIL;
	struct rs_c_key_del_req req_data = { 0 };
	u8 cmd_id = RS_MM_DEL_KEY_CMD;

	RS_TRACE(RS_FN_ENTRY_STR);

	if (c_if) {
		req_data.key_index = key_index;
		ret = rs_c_ctrl_set_and_wait(c_if, cmd_id, sizeof(struct rs_c_key_del_req), (u8 *)&req_data,
					     NULL);
	}

	return ret;
}

rs_ret rs_c_ctrl_add_station_req(struct rs_c_if *c_if, struct station_parameters *params, const u8 *mac,
				 u8 vif_idx, struct rs_c_sta_add_rsp *rsp_data)
{
	rs_ret ret = RS_FAIL;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct rs_c_ctrl_rsp *ctrl_rsp_data = NULL;
	struct rs_c_sta_add_req req_data = { 0 };
	u8 cmd_id = RS_ME_ADD_STA_CMD;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0)
	u8 *ht_mcs = (u8 *)&params->link_sta_params.ht_capa->mcs;
#else
	u8 *ht_mcs = (u8 *)&params->ht_capa->mcs;
#endif
	int i;

	RS_TRACE(RS_FN_ENTRY_STR);

	net_priv = rs_c_if_get_net_priv(c_if);
	ctrl_rsp_data = rs_k_calloc(sizeof(struct rs_c_ctrl_rsp));

	if ((ctrl_rsp_data) && (c_if)) {
		rs_k_memcpy(&req_data.mac_addr.addr[0], mac, ETH_ADDR_LEN);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0)
		req_data.rate_set.length = params->link_sta_params.supported_rates_len;
		for (i = 0; i < params->link_sta_params.supported_rates_len; i++) {
			req_data.rate_set.array[i] = params->link_sta_params.supported_rates[i];
		}
#else
		req_data.rate_set.length = params->supported_rates_len;
		for (i = 0; i < params->supported_rates_len; i++) {
			req_data.rate_set.array[i] = params->supported_rates[i];
		}
#endif

		req_data.flags = 0;
		if (params->capability & WLAN_CAPABILITY_SHORT_PREAMBLE)
			req_data.flags |= STA_CAP_SHORT_PREAMBLE;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0)
		if (params->link_sta_params.ht_capa) {
			const struct ieee80211_ht_cap *ht_cap = params->link_sta_params.ht_capa;
#else
		if (params->ht_capa) {
			const struct ieee80211_ht_cap *ht_cap = params->ht_capa;
#endif

			req_data.flags |= STA_CAP_HT;
			req_data.ht_cap.ht_capa_info = cpu_to_le16(ht_cap->cap_info);
			req_data.ht_cap.a_mpdu_param = ht_cap->ampdu_params_info;
			for (i = 0; i < sizeof(ht_cap->mcs); i++)
				req_data.ht_cap.mcs_rate[i] = ht_mcs[i];
			req_data.ht_cap.ht_extended_capa = cpu_to_le16(ht_cap->extended_ht_cap_info);
			req_data.ht_cap.tx_beamforming_capa = cpu_to_le32(ht_cap->tx_BF_cap_info);
			req_data.ht_cap.asel_capa = ht_cap->antenna_selection_info;
		}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0)
		if (params->link_sta_params.vht_capa) {
			const struct ieee80211_vht_cap *vht_cap = params->link_sta_params.vht_capa;
#else
		if (params->vht_capa) {
			const struct ieee80211_vht_cap *vht_cap = params->vht_capa;
#endif

			req_data.flags |= STA_CAP_VHT;
			req_data.vht_cap.vht_capa_info = cpu_to_le32(vht_cap->vht_cap_info);
			req_data.vht_cap.rx_highest = cpu_to_le16(vht_cap->supp_mcs.rx_highest);
			req_data.vht_cap.rx_mcs_map = cpu_to_le16(vht_cap->supp_mcs.rx_mcs_map);
			req_data.vht_cap.tx_highest = cpu_to_le16(vht_cap->supp_mcs.tx_highest);
			req_data.vht_cap.tx_mcs_map = cpu_to_le16(vht_cap->supp_mcs.tx_mcs_map);
		}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 20, 0)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0)
		if (params->link_sta_params.he_capa) {
			const struct ieee80211_he_cap_elem *he_cap = params->link_sta_params.he_capa;
#else
		if (params->he_capa) {
			const struct ieee80211_he_cap_elem *he_cap = params->he_capa;
#endif
			struct ieee80211_he_mcs_nss_supp *mcs_nss_supp =
				(struct ieee80211_he_mcs_nss_supp *)(he_cap + 1);

			req_data.flags |= STA_CAP_HE;
			for (i = 0; i < ARRAY_SIZE(he_cap->mac_cap_info); i++) {
				req_data.he_cap.mac_cap_info[i] = he_cap->mac_cap_info[i];
			}
			for (i = 0; i < ARRAY_SIZE(he_cap->phy_cap_info); i++) {
				req_data.he_cap.phy_cap_info[i] = he_cap->phy_cap_info[i];
			}
			req_data.he_cap.mcs_supp.rx_mcs_80 = mcs_nss_supp->rx_mcs_80;
			req_data.he_cap.mcs_supp.tx_mcs_80 = mcs_nss_supp->tx_mcs_80;
			req_data.he_cap.mcs_supp.rx_mcs_160 = mcs_nss_supp->rx_mcs_160;
			req_data.he_cap.mcs_supp.tx_mcs_160 = mcs_nss_supp->tx_mcs_160;
			req_data.he_cap.mcs_supp.rx_mcs_80p80 = mcs_nss_supp->rx_mcs_80p80;
			req_data.he_cap.mcs_supp.tx_mcs_80p80 = mcs_nss_supp->tx_mcs_80p80;
		}
#endif

		if (params->sta_flags_set & BIT(NL80211_STA_FLAG_WME))
			req_data.flags |= STA_CAP_QOS;

		if (params->sta_flags_set & BIT(NL80211_STA_FLAG_MFP))
			req_data.flags |= STA_CAP_MFP;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0)
		if (params->link_sta_params.opmode_notif_used) {
			req_data.flags |= STA_OP_NOT_IE;
			req_data.opmode_notif = params->link_sta_params.opmode_notif;
		}
#else
		if (params->opmode_notif_used) {
			req_data.flags |= STA_OP_NOT_IE;
			req_data.opmode_notif = params->opmode_notif;
		}
#endif

		req_data.aid = cpu_to_le16(params->aid);
		req_data.uapsd_queues = params->uapsd_queues;
		req_data.max_sp = params->max_sp * 2;
		req_data.vif_idx = vif_idx;

		if (params->sta_flags_set & BIT(NL80211_STA_FLAG_TDLS_PEER)) {
			struct rs_net_vif_priv *vif = net_priv->vif_table[vif_idx];

			req_data.tdls_sta = true;
			if ((params->ext_capab[3] & WLAN_EXT_CAPA4_TDLS_CHAN_SWITCH) &&
			    !vif->tdls_chsw_prohibited)
				req_data.tdls_chsw_allowed = true;
			if (vif->tdls_status == RS_TDLS_STATE_TX_RSP)
				req_data.tdls_sta_initiator = true;
		}

		ret = rs_c_ctrl_set_and_wait(c_if, cmd_id, sizeof(struct rs_c_sta_add_req), (u8 *)&req_data,
					     ctrl_rsp_data);

		if (ret == RS_SUCCESS) {
			if (ctrl_rsp_data->cmd == cmd_id) {
				(void)rs_k_memcpy(rsp_data, ctrl_rsp_data->data,
						  sizeof(struct rs_c_sta_add_rsp));
			}
		}
	}

	if (ctrl_rsp_data) {
		rs_k_free(ctrl_rsp_data);
	}

	return ret;
}

rs_ret rs_net_ctrl_del_station_req(struct rs_c_if *c_if, u8 sta_idx, bool is_tdls_sta)
{
	rs_ret ret = RS_FAIL;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct rs_c_sta_del_req req_data = { 0 };
	u8 cmd_id = RS_ME_DEL_STA_CMD;

	RS_TRACE(RS_FN_ENTRY_STR);

	if (c_if) {
		net_priv = rs_c_if_get_net_priv(c_if);

		req_data.sta_idx = sta_idx;
		req_data.is_tdls_sta = is_tdls_sta;

		ret = rs_c_ctrl_set_and_wait(c_if, cmd_id, sizeof(struct rs_c_sta_del_req), (u8 *)&req_data,
					     NULL);
	}

	return ret;
}

rs_ret rs_net_ctrl_port_control_req(struct rs_c_if *c_if, bool authorized, u8 sta_idx)
{
	rs_ret ret = 0;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct rs_c_port_control_req req_data = { 0 };
	u8 cmd_id = RS_ME_SET_CONTROL_PORT_CMD;

	RS_TRACE(RS_FN_ENTRY_STR);

	if (c_if) {
		net_priv = rs_c_if_get_net_priv(c_if);

		req_data.port_control_state = authorized;
		req_data.sta_idx = sta_idx;
		ret = rs_c_ctrl_set_and_wait(c_if, cmd_id, sizeof(struct rs_c_port_control_req),
					     (u8 *)&req_data, NULL);
	}

	return ret;
}

rs_ret rs_net_ctrl_remain_on_channel_req(struct rs_c_if *c_if, struct rs_net_vif_priv *vif_priv,
					 struct ieee80211_channel *chan, int duration)
{
	rs_ret ret = RS_FAIL;
	struct rs_c_roc_req req_data = { 0 };
	u8 cmd_id = RS_MM_REMAIN_ON_CHANNEL_CMD;
	struct cfg80211_chan_def chandef;

	RS_TRACE(RS_FN_ENTRY_STR);

	if (c_if) {
		cfg80211_chandef_create(&chandef, chan, NL80211_CHAN_NO_HT);

		req_data.op_code = RS_ROC_OP_START;
		req_data.vif_index = vif_priv->vif_index;
		req_data.duration_ms = duration;
		net_set_channel(&chandef, &req_data.chan);

		ret = rs_c_ctrl_set_and_wait(c_if, cmd_id, sizeof(struct rs_c_roc_req), (u8 *)&req_data,
					     NULL);
	}

	return ret;
}

rs_ret rs_net_ctrl_cancel_remain_on_channel_req(struct rs_c_if *c_if)
{
	rs_ret ret = RS_FAIL;
	struct rs_c_roc_req req_data = { 0 };
	u8 cmd_id = RS_MM_REMAIN_ON_CHANNEL_CMD;

	RS_TRACE(RS_FN_ENTRY_STR);

	if (c_if) {
		req_data.op_code = RS_ROC_OP_STOP;
		ret = rs_c_ctrl_set_and_wait(c_if, cmd_id, sizeof(struct rs_c_roc_req), (u8 *)&req_data,
					     NULL);
	}

	return ret;
}

rs_ret rs_net_ctrl_ap_start_req(struct rs_c_if *c_if, struct rs_net_vif_priv *vif_priv,
				struct cfg80211_ap_settings *settings, struct rs_c_ap_start_rsp *rsp_data)
{
	rs_ret ret = RS_FAIL;
	struct rs_c_ap_start_req req_data = { 0 };
	struct rs_c_ctrl_rsp *ctrl_rsp_data = NULL;
	struct rs_net_beacon_info *beacon_info = NULL;
	int var_offset = offsetof(struct ieee80211_mgmt, u.beacon.variable);
	u8 cmd_id = RS_AM_START_CMD;
	int len = 0, i;
	u8 *var_pos = NULL, *beacon_buf = NULL;
	const u8 *rate_ie = NULL;
	u8 rate_len = 0;
	u32 flags = 0;

	RS_TRACE(RS_FN_ENTRY_STR);

	ctrl_rsp_data = rs_k_calloc(sizeof(struct rs_c_ctrl_rsp));

	if (c_if && ctrl_rsp_data) {
		beacon_info = &vif_priv->ap.bcn;
		beacon_info->dtim_period = settings->dtim_period;
		beacon_buf = net_create_beacon(beacon_info, &settings->beacon);
		if (!beacon_buf) {
			rs_k_free(ctrl_rsp_data);
			return -ENOMEM;
		}

		len = beacon_info->len - var_offset;
		var_pos = beacon_buf + var_offset;

		rate_ie = cfg80211_find_ie(WLAN_EID_SUPP_RATES, var_pos, len);
		if (rate_ie) {
			const u8 *rates = rate_ie + 2;

			for (i = 0; (i < rate_ie[1]) && (rate_len < SUPP_RATESET_SIZE); i++) {
				if (IS_BASIC_RATE(rates[i]))
					req_data.basic_rates.array[rate_len++] = rates[i];
			}
		}
		rate_ie = cfg80211_find_ie(WLAN_EID_EXT_SUPP_RATES, var_pos, len);
		if (rate_ie) {
			const u8 *rates = rate_ie + 2;

			for (i = 0; (i < rate_ie[1]) && (rate_len < SUPP_RATESET_SIZE); i++) {
				if (IS_BASIC_RATE(rates[i]))
					req_data.basic_rates.array[rate_len++] = rates[i];
			}
		}
		req_data.basic_rates.length = rate_len;

		req_data.vif_idx = vif_priv->vif_index;
		rs_k_memcpy(req_data.beacon, beacon_buf, beacon_info->len);
		req_data.bcn_len = beacon_info->len;
		req_data.tim_oft = beacon_info->head_len;
		req_data.tim_len = beacon_info->tim_len;
		net_set_channel(&settings->chandef, &req_data.chan);
		req_data.bcn_int = settings->beacon_interval;

		if (settings->crypto.cipher_group != 0) {
			flags |= USE_PRIVACY;

			if (settings->crypto.cipher_group != WLAN_CIPHER_SUITE_WEP40 &&
			    settings->crypto.cipher_group != WLAN_CIPHER_SUITE_WEP104)
				flags |= USE_PAIRWISE_KEY;
		}

		if (settings->crypto.control_port)
			flags |= CONTROL_PORT_HOST;

		if (settings->crypto.control_port_no_encrypt)
			flags |= CONTROL_PORT_NO_ENC;

		if (settings->crypto.control_port_ethertype)
			req_data.ctrl_port_ethertype = settings->crypto.control_port_ethertype;
		else
			req_data.ctrl_port_ethertype = ETH_P_PAE;
		req_data.flags = flags;

		ret = rs_c_ctrl_set_and_wait(c_if, cmd_id, sizeof(struct rs_c_ap_start_req), (u8 *)&req_data,
					     ctrl_rsp_data);

		if (ret == RS_SUCCESS) {
			if (ctrl_rsp_data->cmd == cmd_id) {
				(void)rs_k_memcpy(rsp_data, ctrl_rsp_data->data,
						  sizeof(struct rs_c_ap_start_rsp));
			}
		}
	}

	if (ctrl_rsp_data) {
		rs_k_free(ctrl_rsp_data);
	}

	return ret;
}

rs_ret rs_net_ctrl_ap_stop_req(struct rs_c_if *c_if, struct rs_net_vif_priv *vif_priv)
{
	rs_ret ret = RS_FAIL;
	struct rs_c_ap_stop_req req_data = { 0 };
	u8 cmd_id = RS_AM_STOP_CMD;

	RS_TRACE(RS_FN_ENTRY_STR);

	if (c_if) {
		req_data.vif_idx = vif_priv->vif_index;
		ret = rs_c_ctrl_set_and_wait(c_if, cmd_id, sizeof(struct rs_c_ap_stop_req), (u8 *)&req_data,
					     NULL);
	}

	return ret;
}

rs_ret rs_net_ctrl_cac_start_req(struct rs_c_if *c_if, struct rs_net_vif_priv *vif_priv,
				 struct cfg80211_chan_def *chandef, struct rs_c_cas_start_rsp *rsp_data)
{
	rs_ret ret = RS_FAIL;
	struct rs_c_cac_start_req req_data = { 0 };
	struct rs_c_ctrl_rsp *ctrl_rsp_data = NULL;
	u8 cmd_id = RS_AM_START_CHAN_AVAIL_CMD;

	RS_TRACE(RS_FN_ENTRY_STR);

	ctrl_rsp_data = rs_k_calloc(sizeof(struct rs_c_ctrl_rsp));
	if ((ctrl_rsp_data) && (c_if)) {
		net_set_channel(chandef, &req_data.chan);
		req_data.vif_idx = vif_priv->vif_index;

		ret = rs_c_ctrl_set_and_wait(c_if, cmd_id, sizeof(struct rs_c_cac_start_req), (u8 *)&req_data,
					     ctrl_rsp_data);

		if (ret == RS_SUCCESS) {
			if (ctrl_rsp_data->cmd == cmd_id) {
				(void)rs_k_memcpy(rsp_data, ctrl_rsp_data->data,
						  sizeof(struct rs_c_cas_start_rsp));
			}
		}
	}

	if (ctrl_rsp_data) {
		rs_k_free(ctrl_rsp_data);
	}

	return ret;
}

rs_ret rs_net_ctrl_cac_stop_req(struct rs_c_if *c_if, struct rs_net_vif_priv *vif_priv)
{
	rs_ret ret = RS_FAIL;
	struct rs_c_cac_stop_req req_data = { 0 };
	u8 cmd_id = RS_AM_STOP_CHAN_AVAIL_CMD;

	RS_TRACE(RS_FN_ENTRY_STR);

	if (c_if) {
		req_data.vif_idx = vif_priv->vif_index;
		ret = rs_c_ctrl_set_and_wait(c_if, cmd_id, sizeof(struct rs_c_cac_stop_req), (u8 *)&req_data,
					     NULL);
	}

	return ret;
}

rs_ret rs_net_ctrl_change_beacon_req(struct rs_c_if *c_if, u8 vif_idx, u8 *bcn, u16 bcn_len, u16 tim_oft,
				     u16 tim_len, u16 *csa_oft)
{
	rs_ret ret = RS_FAIL;
	struct rs_c_change_bcn_req req_data = { 0 };
	u8 cmd_id = RS_AM_BCN_CHANGE_CMD;

	RS_TRACE(RS_FN_ENTRY_STR);

	if (c_if) {
		/* Set parameters for the MACM_BCN_CHANGE_REQ message */
		req_data.bcn_len = bcn_len;
		req_data.tim_oft = tim_oft;
		req_data.tim_len = tim_len;
		req_data.vif_id = vif_idx;
		rs_k_memcpy(req_data.bcn_ptr, bcn, bcn_len);

		if (csa_oft) {
			int i;

			for (i = 0; i < RS_C_BCN_MAX_CSA_CPT; i++) {
				req_data.csa_oft[i] = csa_oft[i];
			}
		}

		ret = rs_c_ctrl_set_and_wait(c_if, cmd_id, sizeof(struct rs_c_change_bcn_req),
					     (u8 *)&req_data, NULL);
	}

	return ret;
}

rs_ret rs_net_ctrl_monitor_mode_req(struct rs_c_if *c_if, struct cfg80211_chan_def *chandef,
				    struct rs_c_mon_mode_rsp *rsp_data)
{
	rs_ret ret = RS_FAIL;
	struct rs_c_mon_mode_req req_data = { 0 };
	struct rs_c_ctrl_rsp *ctrl_rsp_data = NULL;
	u8 cmd_id = RS_ME_SET_MON_CFG_CMD;

	RS_TRACE(RS_FN_ENTRY_STR);

	ctrl_rsp_data = rs_k_calloc(sizeof(struct rs_c_ctrl_rsp));

	if ((ctrl_rsp_data) && (c_if)) {
		if (chandef) {
			req_data.ch_valid = true;
			net_set_channel(chandef, &req_data.chan);
		} else {
			req_data.ch_valid = false;
		}

		req_data.uf_enable = true;

		ret = rs_c_ctrl_set_and_wait(c_if, cmd_id, sizeof(struct rs_c_mon_mode_req), (u8 *)&req_data,
					     ctrl_rsp_data);

		if (ret == RS_SUCCESS) {
			if (ctrl_rsp_data->cmd == cmd_id) {
				(void)rs_k_memcpy(rsp_data, ctrl_rsp_data->data,
						  sizeof(struct rs_c_mon_mode_rsp));
			}
		}
	}

	if (ctrl_rsp_data) {
		rs_k_free(ctrl_rsp_data);
	}

	return ret;
}

rs_ret rs_net_ctrl_probe_client_req(struct rs_c_if *c_if, u8 vif_idx, u8 sta_idx,
				    struct rs_c_probe_client_rsp *rsp_data)
{
	rs_ret ret = RS_FAIL;
	struct rs_c_probe_client_req req_data = { 0 };
	struct rs_c_ctrl_rsp *ctrl_rsp_data = NULL;
	u8 cmd_id = RS_AM_PROBE_CLIENT_CMD;

	RS_TRACE(RS_FN_ENTRY_STR);

	ctrl_rsp_data = rs_k_calloc(sizeof(struct rs_c_ctrl_rsp));

	if ((ctrl_rsp_data) && (c_if)) {
		req_data.vif_idx = vif_idx;
		req_data.sta_idx = sta_idx;

		ret = rs_c_ctrl_set_and_wait(c_if, cmd_id, sizeof(struct rs_c_probe_client_req),
					     (u8 *)&req_data, ctrl_rsp_data);

		if (ret == RS_SUCCESS) {
			if (ctrl_rsp_data->cmd == cmd_id) {
				(void)rs_k_memcpy(rsp_data, ctrl_rsp_data->data,
						  sizeof(struct rs_c_probe_client_rsp));
			}
		}
	}

	if (ctrl_rsp_data) {
		rs_k_free(ctrl_rsp_data);
	}

	return ret;
}

rs_ret rs_net_ctrl_set_ap_isolate(struct rs_c_if *c_if, u8 ap_isolate)
{
	rs_ret ret = RS_FAIL;
	struct rs_net_ap_isolate_req req_data = { 0 };
	struct rs_c_ctrl_rsp *ctrl_rsp_data = NULL;
	u8 cmd_id = RS_AM_ISOLATE_CMD;

	RS_TRACE(RS_FN_ENTRY_STR);

	ctrl_rsp_data = rs_k_calloc(sizeof(struct rs_c_ctrl_rsp));

	if ((ctrl_rsp_data) && (c_if)) {
		req_data.ap_isolate = ap_isolate;

		ret = rs_c_ctrl_set_and_wait(c_if, cmd_id, sizeof(struct rs_net_ap_isolate_req),
					     (u8 *)&req_data, ctrl_rsp_data);

		if (ret == RS_SUCCESS) {
			if ((ctrl_rsp_data->cmd != cmd_id) ||
			    (((struct rs_net_ap_isolate_req *)(ctrl_rsp_data->data))->ap_isolate !=
			     ap_isolate)) {
				ret = RS_FAIL;
			}
		}
	}

	if (ctrl_rsp_data) {
		rs_k_free(ctrl_rsp_data);
	}

	return ret;
}

rs_ret rs_net_ctrl_edca_req(struct rs_c_if *c_if, u8 ac, u32 param, bool uapsd, u8 vif_idx)
{
	rs_ret ret = RS_FAIL;
	struct rs_c_edca_req req_data = { 0 };
	u8 cmd_id = RS_MM_SET_EDCA_CMD;

	RS_TRACE(RS_FN_ENTRY_STR);

	if (c_if) {
		req_data.ac = ac;
		req_data.edca_param = param;
		req_data.uapsd_enabled = uapsd;
		req_data.vif_idx = vif_idx;

		ret = rs_c_ctrl_set_and_wait(c_if, cmd_id, sizeof(struct rs_c_edca_req), (u8 *)&req_data,
					     NULL);
	}

	return ret;
}

rs_ret rs_net_ctrl_set_tx_power_req(struct rs_c_if *c_if, u8 vif_idx, s8 tx_power)
{
	rs_ret ret = RS_FAIL;
	struct rs_c_set_tx_power_req req_data = { 0 };
	u8 cmd_id = RS_MM_SET_POWER_CMD;

	RS_TRACE(RS_FN_ENTRY_STR);

	if (c_if) {
		req_data.vif_idx = vif_idx;
		req_data.tx_power = tx_power;
		ret = rs_c_ctrl_set_and_wait(c_if, cmd_id, sizeof(struct rs_c_set_tx_power_req),
					     (u8 *)&req_data, NULL);
	}

	return ret;
}

rs_ret rs_net_ctrl_set_power_mgmt_req(struct rs_c_if *c_if, u8 ps_mode)
{
	rs_ret ret = RS_FAIL;
	struct rs_c_set_power_mgmt_req req_data = { 0 };
	u8 cmd_id = RS_ME_SET_PS_MODE_CMD;

	RS_TRACE(RS_FN_ENTRY_STR);

	if (c_if) {
		req_data.ps_mode = ps_mode;
		ret = rs_c_ctrl_set_and_wait(c_if, cmd_id, sizeof(struct rs_c_set_power_mgmt_req),
					     (u8 *)&req_data, NULL);
	}

	return ret;
}

rs_ret rs_net_ctrl_ft_auth_req(struct rs_c_if *c_if, struct net_device *ndev, u8 *ie, int ie_len)
{
	rs_ret ret = RS_FAIL;
	struct rs_c_sm_connect_req *req_data = NULL;
	struct rs_net_vif_priv *vif_priv = NULL;
	u8 cmd_id = RS_SM_FT_AUTH_RSP_CMD;
	u8 req_len = sizeof(struct rs_c_sm_connect_req);

	RS_TRACE(RS_FN_ENTRY_STR);

	vif_priv = netdev_priv(ndev);

	if (c_if) {
		req_len += ie_len;
		req_data = rs_k_calloc(req_len);

		req_data->vif_idx = vif_priv->vif_index;
		req_data->ie_len = ie_len;
		rs_k_memcpy(req_data->ie_buf, ie, ie_len);

		ret = rs_c_ctrl_set_and_wait(c_if, cmd_id, req_len, (u8 *)&req_data, 0);
	}

	if (req_data) {
		rs_k_free(req_data);
	}

	return ret;
}

rs_ret rs_net_ctrl_cqm_rssi_config_req(struct rs_c_if *c_if, u8 vif_idx, s32 rssi_thold, u32 rssi_hyst)
{
	rs_ret ret = RS_FAIL;
	struct rs_c_cqm_rssi_config_req req_data = { 0 };
	u8 cmd_id = RS_MM_GET_RSSI_CMD;

	RS_TRACE(RS_FN_ENTRY_STR);

	if (c_if) {
		req_data.vif_idx = vif_idx;
		req_data.rssi_thold = rssi_thold;
		req_data.rssi_hyst = rssi_hyst;
		ret = rs_c_ctrl_set(c_if, cmd_id, sizeof(struct rs_c_cqm_rssi_config_req), (u8 *)&req_data);
	}

	return ret;
}

rs_ret rs_net_ctrl_sm_ext_auth_req_rsp(struct rs_c_if *c_if, u8 vif_index, u16 status)
{
	rs_ret ret = RS_FAIL;
	struct rs_c_sm_ext_auth_req_rsp req_data = { 0 };
	u8 cmd_id = RS_SM_EXTERNAL_AUTH_REQUIRED_RSP_CMD;

	RS_TRACE(RS_FN_ENTRY_STR);

	if (c_if) {
		req_data.vif_idx = vif_index;
		req_data.status = status;
		ret = rs_c_ctrl_set_and_wait(c_if, cmd_id, sizeof(struct rs_c_sm_ext_auth_req_rsp),
					     (u8 *)&req_data, NULL);
	}

	return ret;
}

// TESTMODE
rs_ret rs_net_ctrl_mem_read_req(struct rs_c_if *c_if, u32 addr, struct rs_c_mem_read_rsp *rsp_data)
{
	rs_ret ret = RS_FAIL;
	struct rs_c_dbg_mem_read_req req_data = { 0 };
	struct rs_c_ctrl_rsp *ctrl_rsp_data = NULL;
	u8 cmd_id = RS_DBG_MEM_READ_CMD;

	RS_TRACE(RS_FN_ENTRY_STR);

	ctrl_rsp_data = rs_k_calloc(sizeof(struct rs_c_ctrl_rsp));

	if ((ctrl_rsp_data) && (c_if)) {
		req_data.mem_addr = addr;
		ret = rs_c_ctrl_set_and_wait(c_if, cmd_id, sizeof(struct rs_c_dbg_mem_read_req),
					     (u8 *)&req_data, ctrl_rsp_data);

		if (ret == RS_SUCCESS) {
			if (ctrl_rsp_data->cmd == cmd_id) {
				(void)rs_k_memcpy(rsp_data, ctrl_rsp_data->data,
						  sizeof(struct rs_c_mem_read_rsp));
			}
		}
	}

	if (ctrl_rsp_data) {
		rs_k_free(ctrl_rsp_data);
	}

	return ret;
}

rs_ret rs_net_ctrl_mem_write_req(struct rs_c_if *c_if, u32 addr, u32 value)
{
	rs_ret ret = RS_FAIL;
	struct rs_c_dbg_mem_write_req req_data = { 0 };
	u8 cmd_id = RS_DBG_MEM_WRITE_CMD;

	RS_TRACE(RS_FN_ENTRY_STR);

	if (c_if) {
		req_data.mem_addr = addr;
		req_data.mem_value = value;
		ret = rs_c_ctrl_set_and_wait(c_if, cmd_id, sizeof(struct rs_c_dbg_mem_write_req),
					     (u8 *)&req_data, NULL);
	}

	return ret;
}

rs_ret rs_net_ctrl_dbg_mode_filter_req(struct rs_c_if *c_if, u32 mode)
{
	rs_ret ret = RS_FAIL;
	struct rs_c_dbg_mode_filter_req req_data = { 0 };
	u8 cmd_id = RS_DBG_SET_MOD_FILTER_CMD;

	RS_TRACE(RS_FN_ENTRY_STR);

	if (c_if) {
		req_data.mode_filter = mode;
		ret = rs_c_ctrl_set_and_wait(c_if, cmd_id, sizeof(struct rs_c_dbg_mode_filter_req),
					     (u8 *)&req_data, NULL);
	}

	return ret;
}

rs_ret rs_net_ctrl_dbg_level_filter_req(struct rs_c_if *c_if, u32 level)
{
	rs_ret ret = RS_FAIL;
	struct rs_c_dbg_level_filter_req req_data = { 0 };
	u8 cmd_id = RS_DBG_SET_SEV_FILTER_CMD;

	RS_TRACE(RS_FN_ENTRY_STR);

	if (c_if) {
		req_data.level_filter = level;
		ret = rs_c_ctrl_set_and_wait(c_if, cmd_id, sizeof(struct rs_c_dbg_level_filter_req),
					     (u8 *)&req_data, NULL);
	}

	return ret;
}

rs_ret rs_net_ctrl_rf_tx_req(struct rs_c_if *c_if, struct rs_c_rf_tx_req *data)
{
	rs_ret ret = RS_FAIL;
	struct rs_c_rf_tx_req req_data = { 0 };
	u8 cmd_id = RS_DBG_RF_TX_CMD;

	RS_TRACE(RS_FN_ENTRY_STR);

	if (c_if) {
		req_data.start = data->start;
		req_data.frequency = data->frequency;
		req_data.numFrames = data->numFrames;
		req_data.frameLen = data->frameLen;
		req_data.txRate = data->txRate;
		req_data.txPower = data->txPower;
		req_data.destAddr = data->destAddr;
		req_data.bssid = data->bssid;
		req_data.gi = data->gi;
		req_data.greenField = data->greenField;
		req_data.preambleType = data->preambleType;
		req_data.qosEnable = data->qosEnable;
		req_data.ackPolicy = data->ackPolicy;
		req_data.aifsnVal = data->aifsnVal;

		ret = rs_c_ctrl_set_and_wait(c_if, cmd_id, sizeof(struct rs_c_rf_tx_req), (u8 *)&req_data,
					     NULL);
	}

	return ret;
}

rs_ret rs_net_ctrl_rf_cw_req(struct rs_c_if *c_if, struct rs_c_rf_cw_req *data)
{
	rs_ret ret = RS_FAIL;
	struct rs_c_rf_cw_req req_data = { 0 };
	u8 cmd_id = RS_DBG_RF_CW_CMD;

	RS_TRACE(RS_FN_ENTRY_STR);

	if (c_if) {
		req_data.start = data->start;
		req_data.frequency = data->frequency;
		req_data.txPower = data->txPower;

		ret = rs_c_ctrl_set_and_wait(c_if, cmd_id, sizeof(struct rs_c_rf_cw_req), (u8 *)&req_data,
					     NULL);
	}

	return ret;
}

rs_ret rs_net_ctrl_rf_cont_req(struct rs_c_if *c_if, struct rs_c_rf_cont_req *data)
{
	rs_ret ret = RS_FAIL;
	struct rs_c_rf_cont_req req_data = { 0 };
	u8 cmd_id = RS_DBG_RF_CONT_CMD;

	RS_TRACE(RS_FN_ENTRY_STR);

	if (c_if) {
		req_data.start = data->start;
		req_data.frequency = data->frequency;
		req_data.txPower = data->txPower;

		ret = rs_c_ctrl_set_and_wait(c_if, cmd_id, sizeof(struct rs_c_rf_cont_req), (u8 *)&req_data,
					     NULL);
	}

	return ret;
}

rs_ret rs_net_ctrl_rf_ch_req(struct rs_c_if *c_if, u16 frequency)
{
	rs_ret ret = RS_FAIL;
	struct rs_c_rf_ch_req req_data = { 0 };
	u8 cmd_id = RS_DBG_RF_CH_CMD;

	RS_TRACE(RS_FN_ENTRY_STR);

	if (c_if) {
		req_data.frequency = frequency;

		ret = rs_c_ctrl_set_and_wait(c_if, cmd_id, sizeof(struct rs_c_rf_ch_req), (u8 *)&req_data,
					     NULL);
	}

	return ret;
}

rs_ret rs_net_ctrl_rf_per_req(struct rs_c_if *c_if, u8 start, struct rs_c_rf_per_rsp *rsp_data)
{
	rs_ret ret = RS_FAIL;
	struct rs_c_rf_per_req req_data = { 0 };
	struct rs_c_ctrl_rsp *ctrl_rsp_data = NULL;
	u8 cmd_id = RS_DBG_RF_PER_CMD;

	RS_TRACE(RS_FN_ENTRY_STR);

	ctrl_rsp_data = rs_k_calloc(sizeof(struct rs_c_ctrl_rsp));

	if ((ctrl_rsp_data) && (c_if)) {
		req_data.start = start;
		ret = rs_c_ctrl_set_and_wait(c_if, cmd_id, sizeof(struct rs_c_rf_per_req), (u8 *)&req_data,
					     ctrl_rsp_data);

		if (ret == RS_SUCCESS) {
			if (ctrl_rsp_data->cmd == cmd_id) {
				(void)rs_k_memcpy(rsp_data, ctrl_rsp_data->data,
						  sizeof(struct rs_c_rf_per_rsp));
			}
		}
	}

	if (ctrl_rsp_data) {
		rs_k_free(ctrl_rsp_data);
	}

	return ret;
}

#ifdef CONFIG_DBG_STATS
rs_ret rs_net_ctrl_dbg_stats_tx_req(struct rs_c_if *c_if, u8 req, struct rs_c_dbg_stats_tx_rsp *rsp_data)
{
	rs_ret ret = RS_FAIL;
	struct rs_c_dbg_stats_tx_req req_data = { 0 };
	struct rs_c_ctrl_rsp *ctrl_rsp_data = NULL;
	u8 cmd_id = RS_DBG_STATS_TX_CMD;
	u8 req_type = 0;

	RS_TRACE(RS_FN_ENTRY_STR);

	req_type = req & 0x07;

	if (c_if) {
		req_data.req_type = req_type;

		if (req_type == REQ_STATS_TX) {
			ctrl_rsp_data = rs_k_calloc(sizeof(struct rs_c_dbg_stats_tx_rsp));
		}

		ret = rs_c_ctrl_set_and_wait(c_if, cmd_id, sizeof(struct rs_c_dbg_stats_tx_req),
					     (u8 *)&req_data, ctrl_rsp_data);

		if (ret == RS_SUCCESS && ctrl_rsp_data) {
			if (ctrl_rsp_data->cmd == cmd_id) {
				(void)rs_k_memcpy(rsp_data, ctrl_rsp_data->data,
						  sizeof(struct rs_c_dbg_stats_tx_rsp));
			}

			rs_k_free(ctrl_rsp_data);
		}
	}

	return ret;
}
#endif
