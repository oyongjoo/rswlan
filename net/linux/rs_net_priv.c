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
#include "rs_c_if.h"
#include "rs_core.h"
#include "rs_c_cmd.h"

#include "rs_net_cfg80211.h"
#include "rs_net_priv.h"
#include "rs_net_dev.h"

#include "rs_net_ctrl.h"

////////////////////////////////////////////////////////////////////////////////
/// MACRO DEFINITION

#define MACHW_VERSION_GET(field, val) BF_GET(MACHW_VER, field, val)

////////////////////////////////////////////////////////////////////////////////
/// TYPE DEFINITION

////////////////////////////////////////////////////////////////////////////////
/// LOCAL VARIABLE

////////////////////////////////////////////////////////////////////////////////
/// LOCAL FUNCTION

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL FUNCTION

struct rs_c_if *rs_net_priv_get_c_if(struct rs_net_cfg80211_priv *net_priv)
{
	struct rs_c_if *c_if = NULL;

	if (net_priv) {
		c_if = net_priv->core->c_if;
	}

	return c_if;
}

rs_ret rs_net_priv_set_wiphy(struct rs_net_cfg80211_priv *net_priv, struct wiphy *wiphy)
{
	rs_ret ret = RS_FAIL;

	if ((net_priv) && (wiphy)) {
		net_priv->wiphy = wiphy;
		ret = RS_SUCCESS;
	}

	return ret;
}

struct wiphy *rs_net_priv_get_wiphy(struct rs_net_cfg80211_priv *net_priv)
{
	struct wiphy *wiphy = NULL;

	if (net_priv) {
		wiphy = net_priv->wiphy;
	}

	return wiphy;
}

struct net_device *rs_vif_priv_get_ndev(struct rs_net_vif_priv *vif_priv)
{
	struct net_device *ndev = NULL;

	if (vif_priv) {
		ndev = vif_priv->ndev;
	}

	return ndev;
}

rs_ret rs_vif_priv_set_ndev(struct rs_net_vif_priv *vif_priv, struct net_device *ndev)
{
	rs_ret ret = RS_FAIL;

	if ((vif_priv) && (ndev)) {
		vif_priv->ndev = ndev;
		ret = RS_SUCCESS;
	}

	return ret;
}

struct wireless_dev *rs_vif_priv_get_wdev(struct rs_net_vif_priv *vif_priv)
{
	struct wireless_dev *wdev = NULL;

	if (vif_priv) {
		wdev = &vif_priv->wdev;
	}

	return wdev;
}

// rs_ret rs_net_priv_set_sta_info(struct rs_net_cfg80211_priv *net_priv, s16 sta_index,
//				struct rs_net_sta_priv *sta_info)
// {
//	rs_ret ret = RS_FAIL;

//	if ((net_priv != NULL) && (sta_info != NULL)) {
//		if ((sta_index >= 0) && (sta_index < RS_NET_PRIV_STA_TABLE_MAX)) {
//			net_priv->sta_table[sta_index] = sta_info;
//			ret = RS_SUCCESS;
//		}
//	}

//	return ret;
// }

struct rs_net_sta_priv *rs_net_priv_get_sta_info(struct rs_net_cfg80211_priv *net_priv, s16 sta_index)
{
	struct rs_net_sta_priv *sta_info = NULL;

	if (net_priv) {
		if (sta_index >= 0 && sta_index < RS_NET_PRIV_STA_TABLE_MAX) {
			sta_info = &net_priv->sta_table[sta_index];
		}
	}

	return sta_info;
}

struct rs_net_sta_priv *rs_net_priv_get_sta_info_from_mac(struct rs_net_cfg80211_priv *net_priv,
							  const u8 *mac_addr)
{
	struct rs_net_sta_priv *sta_info = NULL;
	u8 i;

	for (i = 0; i < RS_NET_STA_MAX; i++) {
		sta_info = rs_net_priv_get_sta_info(net_priv, i);

		if ((sta_info) && (sta_info->valid == true) &&
		    ether_addr_equal(sta_info->mac_addr, mac_addr)) {
			return sta_info;
		}
	}

	return NULL;
}

rs_ret rs_net_priv_set_vif_priv(struct rs_net_cfg80211_priv *net_priv, s16 vif_index,
				struct rs_net_vif_priv *vif_priv)
{
	rs_ret ret = RS_FAIL;

	if (net_priv && vif_priv) {
		if (vif_index >= 0 && vif_index < RS_NET_PRIV_VIF_TABLE_MAX) {
			net_priv->vif_table[vif_index] = vif_priv;
			ret = RS_SUCCESS;
		}
	}

	return ret;
}

struct rs_net_vif_priv *rs_net_priv_get_vif_priv(struct rs_net_cfg80211_priv *net_priv, s16 vif_index)
{
	struct rs_net_vif_priv *vif_priv = NULL;

	if (net_priv) {
		// if (vif_index == 0xFF) {
		//	// default VIF
		//	vif_priv = net_priv->vif_table[0];
		// } else
		if (vif_index >= 0 && vif_index < RS_NET_PRIV_VIF_TABLE_MAX) {
			vif_priv = net_priv->vif_table[vif_index];
			if (vif_priv && vif_priv->up == FALSE) {
				vif_priv = NULL;
			}
		}
	}

	return vif_priv;
}

rs_ret rs_vif_priv_set_net_priv(struct rs_net_vif_priv *vif_priv, struct rs_net_cfg80211_priv *net_priv)
{
	rs_ret ret = RS_FAIL;

	if (vif_priv && net_priv) {
		vif_priv->net_priv = net_priv;
		ret = RS_SUCCESS;
	}

	return ret;
}

struct rs_net_cfg80211_priv *rs_vif_priv_get_net_priv(struct rs_net_vif_priv *vif_priv)
{
	struct rs_net_cfg80211_priv *net_priv = NULL;

	if (vif_priv) {
		net_priv = vif_priv->net_priv;
	}

	return net_priv;
}

s32 rs_net_get_hw_ver(u32 ver)
{
#if 0
	u32 um_ver = MACHW_VERSION_GET(UM_VERSION, ver);
	u32 um_type = MACHW_VERSION_GET(UM_TYPE, ver);

	if (um_type < 0) {
		if (um_ver < 0x40)
			return RS_MACHW_DEFAULT;
		else
			return RS_MACHW_HE;
	} else {
		if (um_ver < 0x1D)
			return RS_MACHW_HE_AP;
		else
			return RS_MACHW_HE_AP + 1;
	}
#else
	return RS_MACHW_HE;
#endif
}
