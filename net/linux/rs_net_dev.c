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
#include "rs_c_ctrl.h"

#include "rs_net_cfg80211.h"
#include "rs_net_priv.h"
#include "rs_net_ctrl.h"
#include "rs_net_tx_data.h"
#include "rs_net_skb.h"

#include "rs_net_dev.h"
#include "rs_net_dfs.h"

////////////////////////////////////////////////////////////////////////////////
/// MACRO DEFINITION

////////////////////////////////////////////////////////////////////////////////
/// TYPE DEFINITION

static int ndo_open(struct net_device *ndev);
static int ndo_close(struct net_device *ndev);
static netdev_tx_t ndo_start_xmit(struct sk_buff *skb, struct net_device *ndev);
static u16 ndo_select_queue(struct net_device *dev, struct sk_buff *skb, struct net_device *sb_dev);
static int ndo_set_mac_address(struct net_device *ndev, void *addr);

////////////////////////////////////////////////////////////////////////////////
/// LOCAL VARIABLE

static const struct net_device_ops rs_net_dev_ops = { .ndo_open = ndo_open,
						      .ndo_stop = ndo_close,
						      .ndo_start_xmit = ndo_start_xmit,
						      .ndo_select_queue = ndo_select_queue,
						      .ndo_set_mac_address = ndo_set_mac_address };

static const struct net_device_ops rs_net_dev_monitor_ops = {
	.ndo_open = ndo_open,
	.ndo_stop = ndo_close,
	.ndo_set_mac_address = ndo_set_mac_address,
};

////////////////////////////////////////////////////////////////////////////////
/// LOCAL FUNCTION

static int ndo_open(struct net_device *ndev)
{
	rs_ret ret = RS_FAIL;
	struct rs_net_vif_priv *vif_priv = netdev_priv(ndev);
	struct rs_net_cfg80211_priv *net_priv = rs_vif_priv_get_net_priv(vif_priv);
	struct rs_c_if *c_if = rs_net_priv_get_c_if(net_priv);
	struct rs_c_add_if_rsp *add_if_rsp = &net_priv->cmd_rsp.add_if;

	RS_TRACE(RS_FN_ENTRY_STR);

	if (vif_priv && net_priv && c_if) {
		ret = RS_SUCCESS;
	}

	// Check if it is the first opened VIF
	if (ret == RS_SUCCESS && net_priv->vif_started == 0) {
		// Start Device
		ret = rs_net_ctrl_dev_start(c_if);
	}

	if (ret == RS_SUCCESS) {
		if (RS_NET_WDEV_IF_TYPE(vif_priv) == NL80211_IFTYPE_AP_VLAN) {
			/* For AP_vlan use same fw and drv indexes. We ensure that this index
	   will not be used by fw for another vif by taking index >= NX_VIRT_DEV_MAX */
			netif_tx_stop_all_queues(ndev);
		} else {
			/* Forward the information to the LMAC,
	 *     p2p value not used in FMAC configuration, iftype is sufficient */
			ret = rs_net_ctrl_if_add(c_if, ndev->dev_addr, RS_NET_WDEV_IF_TYPE(vif_priv), false,
						 add_if_rsp);

			if (ret == RS_SUCCESS) {
				if (add_if_rsp->status == 0) {
					vif_priv->vif_index = add_if_rsp->vif_index;
				} else {
					ret = RS_FAIL;
				}
			} else {
				RS_DBG_ERR_RSP(add_if_rsp);
			}
		}
	}

	if (ret == RS_SUCCESS) {
		vif_priv->up = TRUE;
		net_priv->vif_started++;
		ret = rs_net_priv_set_vif_priv(net_priv, vif_priv->vif_index, vif_priv);
		netif_carrier_off(ndev);
	}

	return ret;
}

static int ndo_close(struct net_device *ndev)
{
	rs_ret ret = RS_FAIL;

	struct rs_net_vif_priv *vif_priv = netdev_priv(ndev);
	struct rs_net_cfg80211_priv *net_priv = rs_vif_priv_get_net_priv(vif_priv);
	struct wireless_dev *wdev = rs_vif_priv_get_wdev(vif_priv);
	struct rs_c_if *c_if = rs_net_priv_get_c_if(net_priv);

	RS_TRACE(RS_FN_ENTRY_STR);

	netdev_info(ndev, "CLOSE");

	if (vif_priv && net_priv && wdev && c_if) {
		rs_net_cac_stop(&net_priv->dfs);

		/* Abort scan request on the vif */
		(void)rs_net_cfg80211_scan_stop_and_wait(c_if, vif_priv);

		ret = rs_net_ctrl_if_remove(c_if, vif_priv->vif_index);

		if (net_priv->roc && net_priv->roc->vif == vif_priv) {
			rs_k_free(net_priv->roc);
			net_priv->roc = NULL;
		}

		vif_priv->up = FALSE;

		if (netif_carrier_ok(ndev)) {
			if (RS_NET_WDEV_IF_TYPE(vif_priv) == NL80211_IFTYPE_STATION ||
			    RS_NET_WDEV_IF_TYPE(vif_priv) == NL80211_IFTYPE_P2P_CLIENT) {
				if (vif_priv->sta.ft_assoc_ies) {
					rs_k_free(vif_priv->sta.ft_assoc_ies);
					vif_priv->sta.ft_assoc_ies = NULL;
					vif_priv->sta.ft_assoc_ies_len = 0;
				}
				cfg80211_disconnected(ndev, WLAN_REASON_DEAUTH_LEAVING, NULL, 0, true,
						      GFP_ATOMIC);
				(void)rs_net_if_tx_stop(c_if, vif_priv->vif_index, TRUE);
				netif_carrier_off(ndev);
			} else if (RS_NET_WDEV_IF_TYPE(vif_priv) == NL80211_IFTYPE_AP) {
				netif_carrier_off(ndev);
			} else {
				RS_WARN("AP not stopped when disabling interface\n");
			}
		}

		ret = rs_net_priv_set_vif_priv(net_priv, vif_priv->vif_index, NULL);
		vif_priv->vif_index = RS_NET_INVALID_VIF_IDX;

		(void)rs_net_cfg80211_unset_chaninfo(vif_priv);

		if (RS_NET_WDEV_IF_TYPE(vif_priv) == NL80211_IFTYPE_MONITOR)
			net_priv->mon_vif_index = VIF_MAX;

		net_priv->vif_started--;
		if (net_priv->vif_started == 0) {
			(void)rs_net_ctrl_dev_reset(c_if);

			// Set parameters to firmware
			(void)rs_net_ctrl_me_config(c_if);

			// Set channel parameters to firmware
			(void)rs_net_ctrl_me_chan_config(c_if);
		}
	}

	return 0;
}

static netdev_tx_t ndo_start_xmit(struct sk_buff *skb, struct net_device *ndev)
{
	rs_ret ret = RS_FAIL;
	netdev_tx_t netdev_ret = NETDEV_TX_OK;
	struct rs_net_vif_priv *vif_priv = netdev_priv(ndev);
	struct rs_net_cfg80211_priv *net_priv = rs_vif_priv_get_net_priv(vif_priv);
	struct rs_c_if *c_if = rs_net_priv_get_c_if(net_priv);
	u32 tx_len = skb->len;

	ret = rs_net_tx_data(c_if, vif_priv, (u8 *)skb);
	if (tx_len == 0) {
		RS_DBG("P:%s[%d]:tx_packets[%d]:tx_bytes[%d][%d]:tx_dropped[%d]\n", __func__, __LINE__,
		       ndev->stats.tx_packets, tx_len, ndev->stats.tx_bytes, ndev->stats.tx_dropped);
	}

	if (ret == RS_SUCCESS) {
		ndev->stats.tx_packets++;
		ndev->stats.tx_bytes += tx_len;
	} else {
		// netdev_ret = NETDEV_TX_BUSY;
		ndev->stats.tx_dropped++;
	}

	return netdev_ret;
}

static u16 ndo_select_queue(struct net_device *dev, struct sk_buff *skb, struct net_device *sb_dev)
{
	struct rs_net_vif_priv *vif_priv = netdev_priv(dev);
	struct rs_net_cfg80211_priv *net_priv = rs_vif_priv_get_net_priv(vif_priv);
	struct wireless_dev *wdev = rs_vif_priv_get_wdev(vif_priv);
	struct rs_c_if *c_if = rs_net_priv_get_c_if(net_priv);
	struct rs_net_sta_priv *sta_info = NULL;
	struct ethhdr *eth = NULL;

	u16 sta_idx = RS_NET_NDEV_INVALID_TXQ;

	RS_TRACE(RS_FN_ENTRY_STR);

	if ((c_if != NULL) && (wdev != NULL) && (net_priv != NULL) && (vif_priv != NULL)) {
		switch (wdev->iftype) {
		case NL80211_IFTYPE_STATION:
		case NL80211_IFTYPE_P2P_CLIENT:
			// TODO : TDLS

			sta_info = vif_priv->sta.ap;

			if ((sta_info != NULL) && (sta_info->valid == true)) {
				sta_idx = sta_info->sta_idx;
			}
			RS_DBG("P:%s[%d]:sta_idx[%d]\n", __func__, __LINE__, sta_idx);
			break;

		case NL80211_IFTYPE_AP_VLAN:
			sta_info = vif_priv->ap_vlan.sta_4a;
			if ((sta_info != NULL) && (sta_info->valid == true)) {
				sta_idx = sta_info->sta_idx;
				break;
			}

			// for searching sta amongs all AP's clients
			vif_priv = vif_priv->ap_vlan.master;
			fallthrough;

		case NL80211_IFTYPE_AP:
		case NL80211_IFTYPE_P2P_GO:
			eth = eth_hdr(skb);

			if (is_multicast_ether_addr(eth->h_dest)) {
				sta_info = rs_net_priv_get_sta_info(net_priv, vif_priv->ap.bcmc_index);

				if ((sta_info != NULL) && (sta_info->valid == true)) {
					sta_idx = sta_info->sta_idx;
				}

				RS_DBG("P:%s[%d]:multi:sta_idx[%d]:src[" MAC_ADDRESS_STR
				       "]:desc[" MAC_ADDRESS_STR "],prot[0x%X]\n",
				       __func__, __LINE__, sta_idx, MAC_ADDR_ARRAY(eth->h_source),
				       MAC_ADDR_ARRAY(eth->h_dest), ntohs(eth->h_proto));
			} else {
				list_for_each_entry(sta_info, &vif_priv->ap.sta_list, list) {
					if ((sta_info->valid == true) &&
					    ether_addr_equal(sta_info->mac_addr, eth->h_dest)) {
						sta_idx = sta_info->sta_idx;
						break;
					}
				}
				if (sta_idx < RS_NET_NDEV_INVALID_TXQ) {
					RS_DBG("P:%s[%d]:local:sta_idx[%d]:src[" MAC_ADDRESS_STR
					       "]:mac[" MAC_ADDRESS_STR " ],prot[0x%X]\n",
					       __func__, __LINE__, sta_idx, MAC_ADDR_ARRAY(eth->h_source),
					       MAC_ADDR_ARRAY(eth->h_dest), ntohs(eth->h_proto));
				} else {
					RS_DBG("P:%s[%d]:global:sta_idx[%d]:src[" MAC_ADDRESS_STR
					       "]:desc[" MAC_ADDRESS_STR "],prot[0x%X]\n",
					       __func__, __LINE__, sta_idx, MAC_ADDR_ARRAY(eth->h_source),
					       MAC_ADDR_ARRAY(eth->h_dest), ntohs(eth->h_proto));
				}
			}
			break;

		case NL80211_IFTYPE_MESH_POINT:
			// TODO : MESH_POINT
		default:
			// invalid packet : drop
			break;
		}
	}

	return sta_idx;
}

static int ndo_set_mac_address(struct net_device *ndev, void *addr)
{
	struct sockaddr *sa = addr;
	int ret;

	ret = eth_mac_addr(ndev, sa);

	return ret;
}

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL FUNCTION

void rs_net_dev_init(rs_net_dev_t *ndev)
{
	struct net_device *temp_ndev = ndev;

	u16 hdr_size = RS_C_GET_DATA_SIZE(RS_C_TX_EXT_LEN, 0) + 32;

	if (temp_ndev) {
		ether_setup(temp_ndev);

		temp_ndev->priv_flags &= ~IFF_TX_SKB_SHARING;

		(void)rs_net_dev_set_ops(temp_ndev);

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 12, 0)
		dev->destructor = free_netdev;
#else
		temp_ndev->needs_free_netdev = true;
#endif
		temp_ndev->watchdog_timeo = RS_C_TX_LIFETIME_MS;
		temp_ndev->needed_headroom = hdr_size;
		temp_ndev->hw_features = 0;
	}
}

rs_ret rs_net_dev_set_ops(rs_net_dev_t *ndev)
{
	rs_ret ret = RS_FAIL;
	struct net_device *temp_ndev = ndev;

	if (temp_ndev) {
		temp_ndev->netdev_ops = &rs_net_dev_ops;

		ret = RS_SUCCESS;
	}

	return ret;
}

rs_ret rs_net_dev_set_monitor_ops(rs_net_dev_t *ndev)
{
	rs_ret ret = RS_FAIL;
	struct net_device *temp_ndev = ndev;

	if (temp_ndev) {
		temp_ndev->netdev_ops = &rs_net_dev_monitor_ops;

		ret = RS_SUCCESS;
	}

	return ret;
}

rs_ret rs_net_dev_rx(rs_net_dev_t *ndev, u8 *skb)
{
	rs_ret ret = RS_FAIL;
	struct net_device *temp_ndev = ndev;
	struct sk_buff *temp_skb = (struct sk_buff *)skb;

	if ((temp_ndev) && (temp_skb)) {
		if (rs_net_if_is_up(ndev) == RS_SUCCESS) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 18, 0)
			if (likely(netif_rx(temp_skb) == NET_RX_SUCCESS)) {
#else
			if (likely(netif_rx_ni(temp_skb) == NET_RX_SUCCESS)) {
#endif
				ret = RS_SUCCESS;
			}
		} else {
			rs_net_skb_free(skb);
			temp_skb = NULL;
		}
	}

	return ret;
}

rs_ret rs_net_if_is_up(rs_net_dev_t *ndev)
{
	rs_ret ret = RS_FAIL;
	struct net_device *temp_ndev = ndev;

	if (temp_ndev) {
		if ((temp_ndev->flags & IFF_UP) != 0) {
			ret = RS_SUCCESS;
		}
	}

	return ret;
}

rs_ret rs_net_vif_is_up(rs_net_vif_priv_t *vif_priv)
{
	rs_ret ret = RS_FAIL;
	struct rs_net_vif_priv *temp_vif_priv = vif_priv;

	if ((temp_vif_priv != NULL) && (temp_vif_priv->up == TRUE)) {
		ret = RS_SUCCESS;
	}

	return ret;
}

rs_ret rs_net_vif_idx_is_up(struct rs_c_if *c_if, s16 vif_idx)
{
	rs_ret ret = RS_FAIL;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct net_device *temp_ndev = NULL;
	struct rs_net_vif_priv *vif_priv = NULL;

	net_priv = rs_c_if_get_net_priv(c_if);
	vif_priv = rs_net_priv_get_vif_priv(net_priv, vif_idx);
	temp_ndev = rs_vif_priv_get_ndev(vif_priv);
	if (temp_ndev) {
		if (((temp_ndev->flags & IFF_UP) != 0) && (vif_priv->up == TRUE)) {
			ret = RS_SUCCESS;
		}
	}

	return ret;
}
