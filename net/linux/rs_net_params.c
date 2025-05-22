// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Copyright (C) [2022-2025] Renesas Electronics Corporation and/or its
 * affiliates.
 */

////////////////////////////////////////////////////////////////////////////////
/// INCLUDE

#include <linux/version.h>
#include <linux/module.h>
#include <linux/rtnetlink.h>
#include <net/cfg80211.h>

#include "rs_type.h"
#include "rs_c_dbg.h"
#include "rs_c_if.h"
#include "rs_net_cfg80211.h"
#include "rs_net_priv.h"

#include "rs_net_params.h"
#include "rs_k_mem.h"

////////////////////////////////////////////////////////////////////////////////
/// MACRO DEFINITION

/// CHBW field mask
#define MDM_CHBW_MASK		   ((u32)0x03000000)
/// CHBW field LSB position
#define MDM_CHBW_LSB		   (24)

#define __MDM_MAJOR_VERSION(v)	   (((v) & 0xFF000000) >> 24)

/// NSS field mask
#define MDM_NSS_MASK		   ((u32)0x00000F00)
/// NSS field LSB position
#define MDM_NSS_LSB		   (8)

#define __MDM_MINOR_VERSION(v)	   (((v) & 0x00FF0000) >> 16)
#define __MDM_VERSION(v)	   ((__MDM_MAJOR_VERSION(v) + 2) * 10 + __MDM_MINOR_VERSION(v))

#define MAX_VHT_RATE(map, nss, bw) (net_params_mcs_map_to_rate[bw][map] * (nss))

#define NET_DEV_GCMP_BIT	   RS_BIT(9)

////////////////////////////////////////////////////////////////////////////////
/// TYPE DEFINITION

struct module_param_list {
	bool ps_mode;
	bool agg_tx;
	bool tdls;
	bool stbc_enabled;
	bool ldpc_enabled;
	s32 bt_coex;
	bool fw_sflash;
	bool ant_div;
	bool amsdu_require_spp;
	bool custom_channel;
	bool custom_regulatory;
	bool ht_supported;
	bool vht_supported;
	bool he_enable;
	bool he_ul_on;
	bool bfmee_enabled;
	bool bfmer_enabled;
	bool murx_enabled;
	bool mutx_enabled;
	bool ps_supported;
	bool dpsm_supported;
	u32 uapsd_threshold;
	bool uf_supported;
	u8 rx_amsdu_size;
	bool bw_2040;
	bool bw_80;
	bool bw_160;
	bool rx_gf_enabled;
	u8 nss;
	u8 mcs_map_range;
	u8 he_mcs_map_range;
	bool ap_enable_uapsd;
	bool use_sgi_80;
	bool use_sgi;

	u32 log_level;
};

////////////////////////////////////////////////////////////////////////////////
/// LOCAL VARIABLE

struct module_param_list net_module_param_list = { .he_enable = true, .he_ul_on = true };

/* Regulatory rules */
static struct ieee80211_regdomain net_param_regdom = { .n_reg_rules = 2,
						       .alpha2 = "99",
						       .reg_rules = {
							       REG_RULE(2390 - 10, 2510 + 10, 40, 0, 1000, 0),
							       REG_RULE(5150 - 10, 5970 + 10, 80, 0, 1000, 0),
						       } };

static const int net_params_mcs_map_to_rate[4][3] = {
	[PHY_CHNL_BW_20][IEEE80211_VHT_MCS_SUPPORT_0_7] = 65,
	[PHY_CHNL_BW_20][IEEE80211_VHT_MCS_SUPPORT_0_8] = 78,
	[PHY_CHNL_BW_20][IEEE80211_VHT_MCS_SUPPORT_0_9] = 78,
	[PHY_CHNL_BW_40][IEEE80211_VHT_MCS_SUPPORT_0_7] = 135,
	[PHY_CHNL_BW_40][IEEE80211_VHT_MCS_SUPPORT_0_8] = 162,
	[PHY_CHNL_BW_40][IEEE80211_VHT_MCS_SUPPORT_0_9] = 180,
	[PHY_CHNL_BW_80][IEEE80211_VHT_MCS_SUPPORT_0_7] = 292,
	[PHY_CHNL_BW_80][IEEE80211_VHT_MCS_SUPPORT_0_8] = 351,
	[PHY_CHNL_BW_80][IEEE80211_VHT_MCS_SUPPORT_0_9] = 390,
	[PHY_CHNL_BW_160][IEEE80211_VHT_MCS_SUPPORT_0_7] = 585,
	[PHY_CHNL_BW_160][IEEE80211_VHT_MCS_SUPPORT_0_8] = 702,
	[PHY_CHNL_BW_160][IEEE80211_VHT_MCS_SUPPORT_0_9] = 780,
};

module_param_named(ant_div, net_module_param_list.ant_div, bool, 0444);
MODULE_PARM_DESC(ant_div, "Enable Antenna Diversity (Default: 0)");

module_param_named(amsdu_require_spp, net_module_param_list.amsdu_require_spp, bool, 0444);
MODULE_PARM_DESC(amsdu_require_spp, "Require usage of SPP A-MSDU (Default: 0)");

module_param_named(ps_mode, net_module_param_list.ps_mode, bool, 0444);
MODULE_PARM_DESC(ps_mode, "Use Power Save mode (Default: 1-Enabled)");

module_param_named(tdls, net_module_param_list.tdls, bool, 0444);
MODULE_PARM_DESC(tdls, "Enable TDLS (Default: 0-Diabled)");

module_param_named(ldpc_enabled, net_module_param_list.ldpc_enabled, bool, 0444);
MODULE_PARM_DESC(ldpc_enabled, "Enable LDPC (Default: 1)");

module_param_named(custom_channel, net_module_param_list.custom_channel, bool, 0444);
MODULE_PARM_DESC(custom_channel,
		 "Extend channel set to non-standard channels (for testing ONLY) (Default: 0)");

module_param_named(custom_regulatory, net_module_param_list.custom_regulatory, bool, 0444);
MODULE_PARM_DESC(custom_regulatory, "Use permissive custom regulatory rules (for testing ONLY) (Default: 0)");

module_param_named(bt_coex, net_module_param_list.bt_coex, int, 0644);
MODULE_PARM_DESC(bt_coex, "BT-COEX. (Default: 0: off)");

module_param_named(ps_supported, net_module_param_list.ps_supported, bool, 0444);
MODULE_PARM_DESC(ps_supported, "Power Save supported (Default: 1-Enabled)");

module_param_named(dpsm_supported, net_module_param_list.dpsm_supported, bool, 0444);
MODULE_PARM_DESC(dpsm_supported, "Dynamic Power Save Mode supported (Default: 1-Enabled)");

module_param_named(log_level, net_module_param_list.log_level, int, 0644);
MODULE_PARM_DESC(log_level, "log_level (Default: 7: error + warning + info)");

module_param_named(he_enable, net_module_param_list.he_enable, bool, 0444);
MODULE_PARM_DESC(he_enable, "Enable HE (Default: 1-Enabled)");

////////////////////////////////////////////////////////////////////////////////
/// LOCAL FUNCTION

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 20, 0)
static void net_params_threshold_set(struct ieee80211_sta_he_cap *he_cap)
{
	const u8 PPE_THRES_INFO_OFT = 7;
	const u8 PPE_THRES_INFO_BIT_LEN = 6;
	struct ppe_thres_info_tag {
		u8 ppet16 : 3;
		u8 ppet8 : 3;
	} __packed;

	struct ppe_thres_field_tag {
		u8 nsts : 3;
		u8 ru_idx_bmp : 4;
	};
	int nss = net_module_param_list.nss;
	struct ppe_thres_field_tag *ppe_thres_field = (struct ppe_thres_field_tag *)he_cap->ppe_thres;
	struct ppe_thres_info_tag ppe_thres_info = {
		.ppet16 = 0, // BSPK
		.ppet8 = 7 // None
	};

	u8 *ppe_thres_info_ptr = (u8 *)&ppe_thres_info;
	u16 *ppe_thres_ptr = (u16 *)he_cap->ppe_thres;
	u8 i, j, cnt, offset;

	if (net_module_param_list.bw_80) {
		ppe_thres_field->ru_idx_bmp = 7;
		cnt = 3;
	} else if (net_module_param_list.bw_2040) {
		ppe_thres_field->ru_idx_bmp = 3;
		cnt = 2;
	} else {
		ppe_thres_field->ru_idx_bmp = 1;
		cnt = 1;
	}
	ppe_thres_field->nsts = nss - 1;
	for (i = 0; i < nss; i++) {
		for (j = 0; j < cnt; j++) {
			offset = (i * cnt + j) * PPE_THRES_INFO_BIT_LEN + PPE_THRES_INFO_OFT;
			ppe_thres_ptr = (u16 *)&he_cap->ppe_thres[offset / 8];
			*ppe_thres_ptr |= *ppe_thres_info_ptr << (offset % 8);
		}
	}
}
#endif // LINUX_VERSION_CODE >= 4.20

static rs_ret net_params_wiphy_feat_set(struct wiphy *wiphy)
{
	rs_ret ret = RS_SUCCESS;

	if (net_module_param_list.tdls) {
		/* TDLS support */
		wiphy->flags |= WIPHY_FLAG_SUPPORTS_TDLS;
		wiphy->features |= NL80211_FEATURE_TDLS_CHANNEL_SWITCH;
#ifdef CONFIG_RS_FULLMAC
		/* TDLS external setup support */
		wiphy->flags |= WIPHY_FLAG_TDLS_EXTERNAL_SETUP;
#endif
	}

	if (net_module_param_list.ap_enable_uapsd) {
		wiphy->flags |= WIPHY_FLAG_AP_UAPSD;
	}

	if (net_module_param_list.ps_supported) {
		wiphy->flags |= WIPHY_FLAG_PS_ON_BY_DEFAULT;
	} else {
		wiphy->flags &= ~WIPHY_FLAG_PS_ON_BY_DEFAULT;
	}

	if (net_module_param_list.custom_regulatory) {
		if (net_module_param_list.custom_channel) {
			wiphy->interface_modes = RS_BIT(NL80211_IFTYPE_MONITOR);
			wiphy->bands[NL80211_BAND_2GHZ]->n_channels += 13;

			// Enable "extra" channels
			if (wiphy->bands[NL80211_BAND_5GHZ]) {
				wiphy->bands[NL80211_BAND_5GHZ]->n_channels += 59;
			}
		}
	}

	return ret;
}

static rs_ret net_params_set_vht(struct wiphy *wiphy)
{
	rs_ret ret = RS_FAIL;

	struct ieee80211_supported_band *chan_5G = wiphy->bands[NL80211_BAND_5GHZ];
	u32 nss = net_module_param_list.nss;
	u32 mcs_map, mcs_map_max;
	u32 mcs_map_max_2ss_rx = IEEE80211_VHT_MCS_SUPPORT_0_9;
	int mcs_map_max_2ss_tx = IEEE80211_VHT_MCS_SUPPORT_0_9;
	u32 bw_max;
	u32 i;

	if (net_module_param_list.vht_supported != FALSE && wiphy->bands[NL80211_BAND_5GHZ]) {
		ret = RS_SUCCESS;

		chan_5G->vht_cap.vht_supported = TRUE;
		if (net_module_param_list.use_sgi_80)
			chan_5G->vht_cap.cap |= IEEE80211_VHT_CAP_SHORT_GI_80;
		if (net_module_param_list.stbc_enabled)
			chan_5G->vht_cap.cap |= IEEE80211_VHT_CAP_RXSTBC_1;
		if (net_module_param_list.ldpc_enabled)
			chan_5G->vht_cap.cap |= IEEE80211_VHT_CAP_RXLDPC;
		if (net_module_param_list.bfmee_enabled) {
			chan_5G->vht_cap.cap |= IEEE80211_VHT_CAP_SU_BEAMFORMEE_CAPABLE;
			chan_5G->vht_cap.cap |= 3 << IEEE80211_VHT_CAP_BEAMFORMEE_STS_SHIFT;
		}
		if (nss > 1)
			chan_5G->vht_cap.cap |= IEEE80211_VHT_CAP_TXSTBC;

		chan_5G->vht_cap.cap |= net_module_param_list.rx_amsdu_size;

		if (net_module_param_list.bfmer_enabled) {
			chan_5G->vht_cap.cap |= IEEE80211_VHT_CAP_SU_BEAMFORMER_CAPABLE;
			/* Set number of sounding dimensions */
			chan_5G->vht_cap.cap |= (nss - 1) << IEEE80211_VHT_CAP_SOUNDING_DIMENSIONS_SHIFT;
		}

		if (net_module_param_list.murx_enabled)
			chan_5G->vht_cap.cap |= IEEE80211_VHT_CAP_MU_BEAMFORMEE_CAPABLE;
		if (net_module_param_list.mutx_enabled)
			chan_5G->vht_cap.cap |= IEEE80211_VHT_CAP_MU_BEAMFORMER_CAPABLE;

		if (!net_module_param_list.bw_80 && !net_module_param_list.bw_2040) {
			bw_max = PHY_CHNL_BW_20;
			mcs_map_max = IEEE80211_VHT_MCS_SUPPORT_0_8;
		}
		mcs_map = min_t(int, net_module_param_list.mcs_map_range, mcs_map_max);
		chan_5G->vht_cap.vht_mcs.rx_mcs_map = cpu_to_le16(0);
		for (i = 0; i < nss; i++) {
			chan_5G->vht_cap.vht_mcs.rx_mcs_map |= cpu_to_le16(mcs_map << (i * 2));
			chan_5G->vht_cap.vht_mcs.rx_highest = MAX_VHT_RATE(mcs_map, nss, bw_max);
			mcs_map = min_t(int, mcs_map, mcs_map_max_2ss_rx);
		}
		for (; i < 8; i++) {
			chan_5G->vht_cap.vht_mcs.rx_mcs_map |=
				cpu_to_le16(IEEE80211_VHT_MCS_NOT_SUPPORTED << (i * 2));
		}

		mcs_map = min_t(int, net_module_param_list.mcs_map_range, mcs_map_max);
		chan_5G->vht_cap.vht_mcs.tx_mcs_map = cpu_to_le16(0);
		for (i = 0; i < nss; i++) {
			chan_5G->vht_cap.vht_mcs.tx_mcs_map |= cpu_to_le16(mcs_map << (i * 2));
			chan_5G->vht_cap.vht_mcs.tx_highest = MAX_VHT_RATE(mcs_map, nss, bw_max);
			mcs_map = min_t(int, mcs_map, mcs_map_max_2ss_tx);
		}
		for (; i < 8; i++) {
			chan_5G->vht_cap.vht_mcs.tx_mcs_map |=
				cpu_to_le16(IEEE80211_VHT_MCS_NOT_SUPPORTED << (i * 2));
		}

		if (!net_module_param_list.bw_80) {
			chan_5G->vht_cap.cap &= ~IEEE80211_VHT_CAP_SHORT_GI_80;
		}
	}

	return ret;
}

static rs_ret net_params_set_he(struct rs_net_cfg80211_priv *net_priv, struct wiphy *wiphy)
{
	rs_ret ret = RS_FAIL;

#ifdef CONFIG_RS_SUPPORT_5G
	struct ieee80211_supported_band *chan_5G = wiphy->bands[NL80211_BAND_5GHZ];
#endif
	struct ieee80211_supported_band *chan_2G = wiphy->bands[NL80211_BAND_2GHZ];
	int i;
	int nss = net_module_param_list.nss;
	struct ieee80211_sta_he_cap *he_cap;
	int mcs_map, mcs_map_max_2ss = IEEE80211_HE_MCS_SUPPORT_0_11;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 1, 0)
	u8 dcm_max_ru = IEEE80211_HE_PHY_CAP8_DCM_MAX_RU_242;
#endif
	u32 phy_vers = net_priv->cmd_rsp.fw_ver.phy_version;
	u16 unsup_for_ss = 0;

	if (net_module_param_list.he_enable == TRUE) {
		ret = RS_SUCCESS;

		if (chan_2G->iftype_data) {
			he_cap = (struct ieee80211_sta_he_cap *)&(chan_2G->iftype_data->he_cap);

			he_cap->has_he = TRUE;

			if (net_priv->cmd_rsp.fw_ver.mac_feat & RS_BIT(DEV_FEAT_TWT_BIT)) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 0, 0)
				net_priv->ext_capa[9] = WLAN_EXT_CAPA10_TWT_REQUESTER_SUPPORT;
#endif
				he_cap->he_cap_elem.mac_cap_info[0] |= IEEE80211_HE_MAC_CAP0_TWT_REQ;
			}

			he_cap->he_cap_elem.mac_cap_info[2] |= IEEE80211_HE_MAC_CAP2_ALL_ACK;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 20, 0)
			net_params_threshold_set(he_cap);
#endif

			if (net_module_param_list.bw_2040) {
				he_cap->he_cap_elem.phy_cap_info[0] |=
					IEEE80211_HE_PHY_CAP0_CHANNEL_WIDTH_SET_40MHZ_IN_2G;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 1, 0)
				dcm_max_ru = IEEE80211_HE_PHY_CAP8_DCM_MAX_RU_484;
#endif
			}
			if (net_module_param_list.bw_80) {
				he_cap->he_cap_elem.phy_cap_info[0] |=
					IEEE80211_HE_PHY_CAP0_CHANNEL_WIDTH_SET_40MHZ_80MHZ_IN_5G;
				mcs_map_max_2ss = IEEE80211_HE_MCS_SUPPORT_0_7;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 1, 0)
				dcm_max_ru = IEEE80211_HE_PHY_CAP8_DCM_MAX_RU_996;
#endif
			}
			if (net_module_param_list.ldpc_enabled) {
				he_cap->he_cap_elem.phy_cap_info[1] |=
					IEEE80211_HE_PHY_CAP1_LDPC_CODING_IN_PAYLOAD;
			} else {
				// If no LDPC is supported, we have to limit to MCS0_9, as LDPC is mandatory
				// for MCS 10 and 11
				net_module_param_list.he_mcs_map_range =
					min_t(int, net_module_param_list.he_mcs_map_range,
					      IEEE80211_HE_MCS_SUPPORT_0_9);
			}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 20, 0)
			he_cap->he_cap_elem.phy_cap_info[1] |=
				IEEE80211_HE_PHY_CAP1_HE_LTF_AND_GI_FOR_HE_PPDUS_0_8US |
				IEEE80211_HE_PHY_CAP1_MIDAMBLE_RX_TX_MAX_NSTS;
			he_cap->he_cap_elem.phy_cap_info[2] |= IEEE80211_HE_PHY_CAP2_MIDAMBLE_RX_TX_MAX_NSTS |
							       IEEE80211_HE_PHY_CAP2_NDP_4x_LTF_AND_3_2US |
							       IEEE80211_HE_PHY_CAP2_DOPPLER_RX;
#else
			he_cap->he_cap_elem.phy_cap_info[1] |=
				IEEE80211_HE_PHY_CAP1_HE_LTF_AND_GI_FOR_HE_PPDUS_0_8US;
			he_cap->he_cap_elem.phy_cap_info[2] |= IEEE80211_HE_PHY_CAP2_NDP_4x_LTF_AND_3_2US |
							       IEEE80211_HE_PHY_CAP2_DOPPLER_RX;
#endif
			if (net_module_param_list.stbc_enabled) {
				he_cap->he_cap_elem.phy_cap_info[2] |=
					IEEE80211_HE_PHY_CAP2_STBC_RX_UNDER_80MHZ;
			}

			he_cap->he_cap_elem.phy_cap_info[3] |=
				IEEE80211_HE_PHY_CAP3_DCM_MAX_CONST_RX_16_QAM
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 14, 0)
				| IEEE80211_HE_PHY_CAP3_RX_PARTIAL_BW_SU_IN_20MHZ_MU
#endif
				;
			if (nss > 0) {
				he_cap->he_cap_elem.phy_cap_info[3] |= IEEE80211_HE_PHY_CAP3_DCM_MAX_RX_NSS_2;
			} else {
				he_cap->he_cap_elem.phy_cap_info[3] |= IEEE80211_HE_PHY_CAP3_DCM_MAX_RX_NSS_1;
			}

			if (net_module_param_list.bfmee_enabled) {
				he_cap->he_cap_elem.phy_cap_info[4] |= IEEE80211_HE_PHY_CAP4_SU_BEAMFORMEE;
				he_cap->he_cap_elem.phy_cap_info[4] |=
					IEEE80211_HE_PHY_CAP4_BEAMFORMEE_MAX_STS_UNDER_80MHZ_4;
			}
			he_cap->he_cap_elem.phy_cap_info[5] |= IEEE80211_HE_PHY_CAP5_NG16_SU_FEEDBACK |
							       IEEE80211_HE_PHY_CAP5_NG16_MU_FEEDBACK;
			he_cap->he_cap_elem.phy_cap_info[6] |=
				IEEE80211_HE_PHY_CAP6_CODEBOOK_SIZE_42_SU |
				IEEE80211_HE_PHY_CAP6_CODEBOOK_SIZE_75_MU |
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 14, 0)
				IEEE80211_HE_PHY_CAP6_TRIG_SU_BEAMFORMING_FB |
				IEEE80211_HE_PHY_CAP6_TRIG_MU_BEAMFORMING_PARTIAL_BW_FB |
#endif
				IEEE80211_HE_PHY_CAP6_PPE_THRESHOLD_PRESENT |
				IEEE80211_HE_PHY_CAP6_PARTIAL_BANDWIDTH_DL_MUMIMO;
			he_cap->he_cap_elem.phy_cap_info[7] |=
				IEEE80211_HE_PHY_CAP7_HE_SU_MU_PPDU_4XLTF_AND_08_US_GI;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 1, 0)
			he_cap->he_cap_elem.phy_cap_info[8] |=
				IEEE80211_HE_PHY_CAP8_20MHZ_IN_40MHZ_HE_PPDU_IN_2G | dcm_max_ru;
			he_cap->he_cap_elem.phy_cap_info[9] |=
				IEEE80211_HE_PHY_CAP9_RX_FULL_BW_SU_USING_MU_WITH_COMP_SIGB |
				IEEE80211_HE_PHY_CAP9_RX_FULL_BW_SU_USING_MU_WITH_NON_COMP_SIGB |
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 17, 0)
				IEEE80211_HE_PHY_CAP9_NOMINAL_PKT_PADDING_16US;
#else
				IEEE80211_HE_PHY_CAP9_NOMIMAL_PKT_PADDING_16US;
#endif
#else
			he_cap->he_cap_elem.phy_cap_info[8] |=
				IEEE80211_HE_PHY_CAP8_20MHZ_IN_40MHZ_HE_PPDU_IN_2G;
#endif
			// Starting from version v31 more HE_ER_SU modulations is supported
			if (__MDM_VERSION(phy_vers) > 30) { // 31

				he_cap->he_cap_elem.phy_cap_info[6] |=
					IEEE80211_HE_PHY_CAP6_PARTIAL_BW_EXT_RANGE;
				he_cap->he_cap_elem.phy_cap_info[8] |=
					IEEE80211_HE_PHY_CAP8_HE_ER_SU_1XLTF_AND_08_US_GI |
					IEEE80211_HE_PHY_CAP8_HE_ER_SU_PPDU_4XLTF_AND_08_US_GI;
			}

			mcs_map = net_module_param_list.he_mcs_map_range;
			rs_k_memset(&he_cap->he_mcs_nss_supp, 0, sizeof(he_cap->he_mcs_nss_supp));
			for (i = 0; i < nss; i++) {
				unsup_for_ss = cpu_to_le16(IEEE80211_HE_MCS_NOT_SUPPORTED << (i * 2));
				he_cap->he_mcs_nss_supp.rx_mcs_80 |= cpu_to_le16(mcs_map << (i * 2));
				he_cap->he_mcs_nss_supp.rx_mcs_160 |= unsup_for_ss;
				he_cap->he_mcs_nss_supp.rx_mcs_80p80 |= unsup_for_ss;
				mcs_map = min_t(int, net_module_param_list.he_mcs_map_range, mcs_map_max_2ss);
			}
			for (; i < 8; i++) {
				unsup_for_ss = cpu_to_le16(IEEE80211_HE_MCS_NOT_SUPPORTED << (i * 2));
				he_cap->he_mcs_nss_supp.rx_mcs_80 |= unsup_for_ss;
				he_cap->he_mcs_nss_supp.rx_mcs_160 |= unsup_for_ss;
				he_cap->he_mcs_nss_supp.rx_mcs_80p80 |= unsup_for_ss;
			}
			mcs_map = net_module_param_list.he_mcs_map_range;
			for (i = 0; i < nss; i++) {
				unsup_for_ss = cpu_to_le16(IEEE80211_HE_MCS_NOT_SUPPORTED << (i * 2));
				he_cap->he_mcs_nss_supp.tx_mcs_80 |= cpu_to_le16(mcs_map << (i * 2));
				he_cap->he_mcs_nss_supp.tx_mcs_160 |= unsup_for_ss;
				he_cap->he_mcs_nss_supp.tx_mcs_80p80 |= unsup_for_ss;
				mcs_map = min_t(int, net_module_param_list.he_mcs_map_range, mcs_map_max_2ss);
			}
			for (; i < 8; i++) {
				unsup_for_ss = cpu_to_le16(IEEE80211_HE_MCS_NOT_SUPPORTED << (i * 2));
				he_cap->he_mcs_nss_supp.tx_mcs_80 |= unsup_for_ss;
				he_cap->he_mcs_nss_supp.tx_mcs_160 |= unsup_for_ss;
				he_cap->he_mcs_nss_supp.tx_mcs_80p80 |= unsup_for_ss;
			}
		}

	} else {
		chan_2G->iftype_data = NULL;
		chan_2G->n_iftype_data = 0;
#ifdef CONFIG_RS_SUPPORT_5G
		chan_5G->iftype_data = NULL;
		chan_5G->n_iftype_data = 0;
#endif
	}

	return ret;
}

static rs_ret net_params_set_ht(struct wiphy *wiphy)
{
	rs_ret ret = RS_FAIL;

	struct ieee80211_supported_band *chan_5G = wiphy->bands[NL80211_BAND_5GHZ];
	struct ieee80211_supported_band *chan_2G = wiphy->bands[NL80211_BAND_2GHZ];
	int i;
	int nss = net_module_param_list.nss;

	if (net_module_param_list.ht_supported == TRUE) {
		ret = RS_SUCCESS;

		if (net_module_param_list.stbc_enabled) {
			chan_2G->ht_cap.cap |= 1 << IEEE80211_HT_CAP_RX_STBC_SHIFT;
		}
		if (net_module_param_list.ldpc_enabled) {
			chan_2G->ht_cap.cap |= IEEE80211_HT_CAP_LDPC_CODING;
		}
		if (net_module_param_list.bw_2040) {
			chan_2G->ht_cap.mcs.rx_mask[4] = 0x1; /* MCS32 */
			chan_2G->ht_cap.cap |= IEEE80211_HT_CAP_SUP_WIDTH_20_40;
			chan_2G->ht_cap.mcs.rx_highest = cpu_to_le16(135 * nss);
		} else {
			chan_2G->ht_cap.mcs.rx_highest = cpu_to_le16(65 * nss);
		}
		if (nss > 1) {
			chan_2G->ht_cap.cap |= IEEE80211_HT_CAP_TX_STBC;
		}

		// Update the AMSDU max RX size
		if (net_module_param_list.rx_amsdu_size) {
			chan_2G->ht_cap.cap |= IEEE80211_HT_CAP_MAX_AMSDU;
		}

		if (net_module_param_list.use_sgi) {
			chan_2G->ht_cap.cap |= IEEE80211_HT_CAP_SGI_20;
			if (net_module_param_list.bw_2040) {
				chan_2G->ht_cap.cap |= IEEE80211_HT_CAP_SGI_40;
				chan_2G->ht_cap.mcs.rx_highest = cpu_to_le16(150 * nss);
			} else
				chan_2G->ht_cap.mcs.rx_highest = cpu_to_le16(72 * nss);
		}
		if (net_module_param_list.rx_gf_enabled) {
			chan_2G->ht_cap.cap |= IEEE80211_HT_CAP_GRN_FLD;
		}

		for (i = 0; i < nss; i++) {
			chan_2G->ht_cap.mcs.rx_mask[i] = 0xFF;
		}

		if (chan_5G) {
			chan_5G->ht_cap = chan_2G->ht_cap;
		}
	} else {
		chan_2G->ht_cap.ht_supported = FALSE;
		if (chan_5G) {
			chan_5G->ht_cap.ht_supported = FALSE;
		}
	}

	return ret;
}

static rs_ret net_params_dev_feat(struct rs_net_cfg80211_priv *net_priv, struct wiphy *wiphy)
{
	rs_ret ret = RS_FAIL;

	u32 sys_feat = net_priv->cmd_rsp.fw_ver.mac_feat;
	u32 mac_feat = net_priv->cmd_rsp.fw_ver.machw_features;
	u32 phy_feat = net_priv->cmd_rsp.fw_ver.phy_feature;
	u32 phy_vers = net_priv->cmd_rsp.fw_ver.phy_version;
	u16 sta_max_count = net_priv->cmd_rsp.fw_ver.sta_max_count;
	u8 vif_max_count = net_priv->cmd_rsp.fw_ver.vif_max_count;
	u32 bw = 0;
	u8 amsdu_rx = 0;

	if ((wiphy) && (rs_net_chk_dev_feat(sys_feat, DEV_FEAT_UMAC_BIT) != 0)) {
		ret = RS_SUCCESS;

		if (rs_net_chk_dev_feat(sys_feat, DEV_FEAT_VHT_BIT) == 0) {
			net_module_param_list.vht_supported = FALSE;
		}

		if (rs_net_chk_dev_feat(sys_feat, DEV_FEAT_PS_BIT) == 0) {
			net_module_param_list.ps_supported = FALSE;
		}

		if (rs_net_chk_dev_feat(sys_feat, DEV_FEAT_AMSDU_BIT) != 0) {
			RS_INFO("AMSDU supported!!!\n");
		}

		if (rs_net_chk_dev_feat(sys_feat, DEV_FEAT_UAPSD_BIT) == 0) {
			net_module_param_list.uapsd_threshold = 0;
		}

		if (rs_net_chk_dev_feat(sys_feat, DEV_FEAT_BFMEE_BIT) == 0) {
			net_module_param_list.bfmee_enabled = FALSE;
		}

		// if (rs_net_chk_dev_feat(sys_feat, DEV_FEAT_MESH_BIT) == 0) {
		//	net_module_param_list.mesh_supported = FALSE;
		// }

		if (rs_net_chk_dev_feat(sys_feat, DEV_FEAT_TDLS_BIT) == 0) {
			net_module_param_list.tdls = FALSE;
		}

		if (rs_net_chk_dev_feat(sys_feat, DEV_FEAT_UF_BIT) == 0) {
			net_module_param_list.uf_supported = FALSE;
		}

		// Check supported AMSDU RX size
		amsdu_rx = (sys_feat >> DEV_AMSDU_MAX_SIZE_BIT0) & 0x03;
		if (amsdu_rx < net_module_param_list.rx_amsdu_size) {
			net_module_param_list.rx_amsdu_size = amsdu_rx;
		}

		// Check supported BW
		bw = (phy_feat & MDM_CHBW_MASK) >> MDM_CHBW_LSB;
		if (bw < 1) {
			net_module_param_list.bw_2040 = FALSE;
			net_module_param_list.bw_80 = FALSE;
		}

		if (net_module_param_list.ht_supported == FALSE) {
			net_module_param_list.vht_supported = FALSE;
			net_module_param_list.he_enable = FALSE;
		}

		// LDPC is mandatory for HE40 and above, so if LDPC is not supported, then disable
		// support for 40 and 80MHz
		if (net_module_param_list.he_enable == TRUE && net_module_param_list.ldpc_enabled == FALSE) {
			net_module_param_list.bw_80 = FALSE;
			net_module_param_list.bw_2040 = FALSE;
		}

		if (__MDM_MAJOR_VERSION(phy_vers) > 0) {
			net_module_param_list.rx_gf_enabled = FALSE;
		}

		if ((rs_net_chk_dev_feat(sys_feat, DEV_FEAT_MU_MIMO_RX_BIT) == 0) ||
		    !net_module_param_list.bfmee_enabled) {
			net_module_param_list.murx_enabled = FALSE;
		}

		if (rs_net_chk_dev_feat(sys_feat, DEV_FEAT_WAPI_BIT) != 0) {
			rs_net_cfg80211_wapi_enable(wiphy);
		}

		if (rs_net_chk_dev_feat(sys_feat, DEV_FEAT_MFP_BIT) != 0) {
			rs_net_cfg80211_mfp_enable(wiphy);
		}

		if (net_priv->machw_version >= RS_MACHW_SUPPORT_HE) {
			rs_net_cfg80211_ccmp256_enable(wiphy);
		}

		if (mac_feat & NET_DEV_GCMP_BIT) {
			rs_net_cfg80211_gcmp_enable(wiphy);
		}

#ifdef CONFIG_RS_SUPPORT_5G
		if (rs_net_chk_dev_feat(sys_feat, DEV_FEAT_RADAR_BIT) != 0) {
			wiphy->n_iface_combinations++;
		}
#endif

		net_module_param_list.nss = (phy_feat & MDM_NSS_MASK) >> MDM_NSS_LSB;

		if (sta_max_count != RS_NET_STA_MAX) {
			RS_ERR("Different number of supported stations between driver and FW (%d != %d)\n",
			       RS_NET_STA_MAX, sta_max_count);
			ret = RS_FAIL;
		}

		if (vif_max_count != RS_NET_VIF_DEV_MAX) {
			RS_ERR("Different number of supported virtual interfaces between driver and FW (%d != %d)\n",
			       RS_NET_VIF_DEV_MAX, vif_max_count);
			ret = RS_FAIL;
		}

	} else {
		RS_ERR("UMAC is not supported\n");
	}

	return ret;
}

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL FUNCTION

rs_ret rs_net_params_init(struct rs_c_if *c_if)
{
	rs_ret ret = RS_FAIL;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct wiphy *wiphy = NULL;

	RS_TRACE(RS_FN_ENTRY_STR);

	net_priv = rs_c_if_get_net_priv(c_if);
	wiphy = rs_net_priv_get_wiphy(net_priv);

	if ((net_priv) && (wiphy)) {
		net_module_param_list.mutx_enabled = FALSE;

#ifdef RS_TIN_AA_RELEASE_LIMIT
		net_module_param_list.ps_mode = TRUE;
#else

		net_module_param_list.ps_mode = TRUE;
#endif

		net_module_param_list.tdls = FALSE;
		net_module_param_list.stbc_enabled = TRUE;
		net_module_param_list.ldpc_enabled = TRUE;
		net_module_param_list.bt_coex = 0;
		net_module_param_list.fw_sflash = FALSE;
		net_module_param_list.custom_channel = FALSE;
		net_module_param_list.custom_regulatory = FALSE;
#ifdef RS_TIN_AA_RELEASE_LIMIT
		net_module_param_list.ps_supported = TRUE;
#else
		net_module_param_list.ps_supported = TRUE;
#endif

		net_module_param_list.dpsm_supported = TRUE;
		net_module_param_list.uapsd_threshold = RS_NET_UAPSD_TIMEOUT;
		net_module_param_list.rx_amsdu_size = 2;
		net_module_param_list.ht_supported = TRUE;
		net_module_param_list.bw_2040 = FALSE;
		net_module_param_list.bw_80 = FALSE;
		net_module_param_list.bw_160 = FALSE;
		net_module_param_list.nss = 1;
		net_module_param_list.mcs_map_range = IEEE80211_VHT_MCS_SUPPORT_0_8;
		net_module_param_list.use_sgi = TRUE;

		net_module_param_list.agg_tx = TRUE;

		net_module_param_list.ant_div = FALSE;
		net_module_param_list.amsdu_require_spp = FALSE;
		net_module_param_list.vht_supported = TRUE;
		net_module_param_list.bfmee_enabled = TRUE;
		net_module_param_list.bfmer_enabled = TRUE;
		// net_module_param_list.mesh_supported = FALSE;
		net_module_param_list.uf_supported = TRUE;
		net_module_param_list.murx_enabled = TRUE;
		net_module_param_list.he_mcs_map_range = IEEE80211_HE_MCS_SUPPORT_0_9;
		net_module_param_list.ap_enable_uapsd = TRUE;
		net_module_param_list.use_sgi_80 = FALSE;
		net_module_param_list.mutx_enabled = FALSE;

		ret = net_params_dev_feat(net_priv, wiphy);

		if (ret == RS_SUCCESS) {
			ret = net_params_wiphy_feat_set(wiphy);
		}

		(void)net_params_set_vht(wiphy);

		(void)net_params_set_he(net_priv, wiphy);

		(void)net_params_set_ht(wiphy);

		net_module_param_list.log_level = RS_DEFAULT_LOG_LEVEL;
		rs_log_level = net_module_param_list.log_level;
	}

	return ret;
}

rs_ret rs_net_params_custom_regd(struct rs_c_if *c_if)
{
	rs_ret ret = RS_SUCCESS;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct wiphy *wiphy = NULL;

	RS_TRACE(RS_FN_ENTRY_STR);

	net_priv = rs_c_if_get_net_priv(c_if);
	wiphy = rs_net_priv_get_wiphy(net_priv);

	if (wiphy && net_module_param_list.custom_regulatory == TRUE) {
		rtnl_lock();

#if LINUX_VERSION_CODE <= KERNEL_VERSION(6, 1, 38)
		wiphy->regulatory_flags |= REGULATORY_IGNORE_STALE_KICKOFF;
#endif
		wiphy->regulatory_flags |= REGULATORY_WIPHY_SELF_MANAGED;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 12, 0)
		if (regulatory_set_wiphy_regd_sync(wiphy, &net_param_regdom) != 0) {
#else
		if (regulatory_set_wiphy_regd_sync_rtnl(wiphy, &net_param_regdom) != 0) {
#endif
			RS_ERR("Failed to set custom regdomain\n");
			ret = RS_FAIL;
		} else {
			RS_INFO("\n"
				"*******************************************************\n"
				"** CAUTION: USING PERMISSIVE CUSTOM REGULATORY RULES **\n"
				"*******************************************************\n");
		}

		rtnl_unlock();
	}

	return ret;
}

// bool rs_net_params_get_mesh(struct rs_c_if *c_if)
// {
//	bool mesh = FALSE;
//	struct rs_net_cfg80211_priv *net_priv = NULL;

//	net_priv = rs_c_if_get_net_priv(c_if);
//	if (net_priv != NULL) {
//		mesh = net_module_param_list.mesh_supported;
//	}

//	return mesh;
// }

u32 rs_net_params_get_bt_coex(struct rs_c_if *c_if)
{
	bool bt_coex = FALSE;
	struct rs_net_cfg80211_priv *net_priv = NULL;

	net_priv = rs_c_if_get_net_priv(c_if);
	if (net_priv) {
		bt_coex = net_module_param_list.bt_coex;
	}

	return bt_coex;
}

bool rs_net_params_get_agg_tx(struct rs_c_if *c_if)
{
	bool agg_tx = FALSE;
	struct rs_net_cfg80211_priv *net_priv = NULL;

	net_priv = rs_c_if_get_net_priv(c_if);
	if (net_priv) {
		agg_tx = net_module_param_list.agg_tx;
	}

	return agg_tx;
}

bool rs_net_params_get_ps(struct rs_c_if *c_if)
{
	bool ps_supported = FALSE;
	struct rs_net_cfg80211_priv *net_priv = NULL;

	net_priv = rs_c_if_get_net_priv(c_if);
	if (net_priv) {
		ps_supported = net_module_param_list.ps_supported;
	}

	return ps_supported;
}

bool rs_net_params_get_dpsm(struct rs_c_if *c_if)
{
	bool dpsm_supported = FALSE;
	struct rs_net_cfg80211_priv *net_priv = NULL;

	net_priv = rs_c_if_get_net_priv(c_if);
	if (net_priv) {
		dpsm_supported = net_module_param_list.dpsm_supported;
	}

	return dpsm_supported;
}

bool rs_net_chk_dev_feat(u32 dev_feat, enum rs_net_dev_feat id)
{
	return (dev_feat & RS_BIT(id));
}

bool rs_net_params_get_amsdu_require_spp(struct rs_c_if *c_if)
{
	bool amsdu_require_spp = FALSE;
	struct rs_net_cfg80211_priv *net_priv = NULL;

	net_priv = rs_c_if_get_net_priv(c_if);
	if (net_priv) {
		amsdu_require_spp = net_module_param_list.amsdu_require_spp;
	}

	return amsdu_require_spp;
}

bool rs_net_params_get_ht_supported(struct rs_c_if *c_if)
{
	bool ht_supported = FALSE;
	struct rs_net_cfg80211_priv *net_priv = NULL;

	net_priv = rs_c_if_get_net_priv(c_if);
	if (net_priv) {
		ht_supported = net_module_param_list.ht_supported;
	}

	return ht_supported;
}

bool rs_net_params_get_he_enabled(struct rs_c_if *c_if)
{
	bool he_enabled = FALSE;
	struct rs_net_cfg80211_priv *net_priv = NULL;

	net_priv = rs_c_if_get_net_priv(c_if);
	if (net_priv) {
		u32 sys_feat = net_priv->cmd_rsp.fw_ver.mac_feat;

		if (rs_net_chk_dev_feat(sys_feat, DEV_FEAT_HE_BIT) == 0) {
			net_module_param_list.he_enable = FALSE;
		}
		he_enabled = net_module_param_list.he_enable;
	}

	return he_enabled;
}

bool rs_net_params_get_he_ul_on(struct rs_c_if *c_if)
{
	bool he_ul_on = FALSE;
	struct rs_net_cfg80211_priv *net_priv = NULL;

	net_priv = rs_c_if_get_net_priv(c_if);
	if (net_priv) {
		u32 sys_feat = net_priv->cmd_rsp.fw_ver.mac_feat;

		if (rs_net_chk_dev_feat(sys_feat, DEV_FEAT_HE_BIT) == 0) {
			net_module_param_list.he_ul_on = FALSE;
		}
		he_ul_on = net_module_param_list.he_ul_on;
	}

	return he_ul_on;
}

u32 rs_net_params_get_uapsd_threshold(struct rs_c_if *c_if)
{
	u32 uapsd_threshold = 0;

	struct rs_net_cfg80211_priv *net_priv = NULL;

	net_priv = rs_c_if_get_net_priv(c_if);
	if (net_priv) {
		u32 sys_feat = net_priv->cmd_rsp.fw_ver.mac_feat;

		if (rs_net_chk_dev_feat(sys_feat, DEV_FEAT_UAPSD_BIT) == 0) {
			net_module_param_list.uapsd_threshold = RS_NET_UAPSD_TIMEOUT;
		}
		uapsd_threshold = net_module_param_list.uapsd_threshold;
	}

	return uapsd_threshold;
}