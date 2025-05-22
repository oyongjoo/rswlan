// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Copyright (C) [2022-2025] Renesas Electronics Corporation and/or its
 * affiliates.
 */

#ifndef RS_NET_PARAMS_H
#define RS_NET_PARAMS_H

////////////////////////////////////////////////////////////////////////////////
/// INCLUDE

#include "rs_type.h"

////////////////////////////////////////////////////////////////////////////////
/// MACRO DEFINITION

////////////////////////////////////////////////////////////////////////////////
/// TYPE DEFINITION

enum rs_net_dev_feat
{
	DEV_FEAT_BCN_BIT = 0,
	DEV_FEAT_RADAR_BIT,
	DEV_FEAT_PS_BIT,
	DEV_FEAT_UAPSD_BIT,
	DEV_FEAT_AMPDU_BIT,
	DEV_FEAT_AMSDU_BIT,
	DEV_FEAT_P2P_BIT,
	DEV_FEAT_P2P_GO_BIT,
	DEV_FEAT_UMAC_BIT,
	DEV_FEAT_VHT_BIT,
	DEV_FEAT_BFMEE_BIT,
	DEV_FEAT_BFMER_BIT,
	DEV_FEAT_WAPI_BIT,
	DEV_FEAT_MFP_BIT,
	DEV_FEAT_MU_MIMO_RX_BIT,
	DEV_FEAT_MU_MIMO_TX_BIT,
	DEV_FEAT_MESH_BIT,
	DEV_FEAT_TDLS_BIT,
	DEV_FEAT_ANT_DIV_BIT,
	DEV_FEAT_UF_BIT,
	DEV_AMSDU_MAX_SIZE_BIT0,
	DEV_AMSDU_MAX_SIZE_BIT1,
	/// MON_DATA support
	DEV_FEAT_MON_DATA_BIT,
	/// HE (802.11ax) support
	DEV_FEAT_HE_BIT,
	/// TWT support
	DEV_FEAT_TWT_BIT,
	/// FTM initiator support
	DEV_FEAT_FTM_INIT_BIT,
	/// Fake FTM responder support
	DEV_FEAT_FAKE_FTM_RSP_BIT,
	/// HW-assisted LLC/SNAP insertion support
	DEV_FEAT_HW_LLCSNAP_INS_BIT,
	DEV_FEAT_MAX
};

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL VARIABLE

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL FUNCTION

// Initialize network parameters
rs_ret rs_net_params_init(struct rs_c_if *c_if);

// Set custom regulatory
rs_ret rs_net_params_custom_regd(struct rs_c_if *c_if);

// bool rs_net_params_get_mesh(struct rs_c_if *c_if);

u32 rs_net_params_get_bt_coex(struct rs_c_if *c_if);

bool rs_net_params_get_agg_tx(struct rs_c_if *c_if);

bool rs_net_params_get_ps(struct rs_c_if *c_if);

bool rs_net_params_get_dpsm(struct rs_c_if *c_if);

bool rs_net_params_get_amsdu_require_spp(struct rs_c_if *c_if);

bool rs_net_params_get_ht_supported(struct rs_c_if *c_if);

bool rs_net_params_get_he_enabled(struct rs_c_if *c_if);

// Check device's feature
bool rs_net_chk_dev_feat(u32 dev_feat, enum rs_net_dev_feat id);

bool rs_net_params_get_he_ul_on(struct rs_c_if *c_if);

u32 rs_net_params_get_uapsd_threshold(struct rs_c_if *c_if);

#endif /* RS_NET_PARAMS_H */
