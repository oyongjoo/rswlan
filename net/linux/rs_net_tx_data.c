// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Copyright (C) [2022-2025] Renesas Electronics Corporation and/or its
 * affiliates.
 */

////////////////////////////////////////////////////////////////////////////////
/// INCLUDE

#include <linux/version.h>
#include <linux/module.h>
#include <linux/if_ether.h>
#include <net/cfg80211.h>

#include "rs_type.h"
#include "rs_k_mem.h"
#include "rs_c_dbg.h"
#include "rs_c_if.h"
#include "rs_core.h"
#include "rs_c_status.h"
#include "rs_c_tx.h"

#include "rs_net_cfg80211.h"
#include "rs_net_priv.h"
#include "rs_net_skb.h"
#include "rs_net_tx_data.h"

////////////////////////////////////////////////////////////////////////////////
/// MACRO DEFINITION

////////////////////////////////////////////////////////////////////////////////
/// TYPE DEFINITION

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL VARIABLE

////////////////////////////////////////////////////////////////////////////////
/// LOCAL FUNCTION

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL FUNCTION

rs_ret rs_net_if_tx_stop(struct rs_c_if *c_if, s8 vif_idx, bool stop)
{
	rs_ret ret = RS_FAIL;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct net_device *ndev = NULL;
	struct rs_net_vif_priv *vif_priv = NULL;

	net_priv = rs_c_if_get_net_priv(c_if);
	vif_priv = rs_net_priv_get_vif_priv(net_priv, vif_idx);
	ndev = rs_vif_priv_get_ndev(vif_priv);
	if (ndev) {
		if (stop == TRUE) {
			if (vif_priv->net_tx_stopped == FALSE) {
				vif_priv->net_tx_stopped = TRUE;
				netif_tx_stop_all_queues(ndev);
				// netif_stop_queue(ndev);
			}
		} else {
			if (vif_priv->net_tx_stopped == TRUE) {
				vif_priv->net_tx_stopped = FALSE;
				netif_tx_start_all_queues(ndev);
				// netif_wake_queue(ndev);
			}
		}
		ret = RS_SUCCESS;
	}

	return ret;
}

rs_ret rs_net_tx_mgmt(struct rs_c_if *c_if, struct rs_net_vif_priv *vif_priv, struct rs_net_sta_priv *sta,
		      void *params, bool offchan, u64 *cookie)
{
	rs_ret ret = RS_FAIL;
	struct cfg80211_mgmt_tx_params *temp_params = NULL;
	struct sk_buff *temp_skb = NULL;
	struct rs_c_tx_data *tx_data = NULL;
	struct ethhdr *temp_eth_hdr = NULL;
	struct ethhdr *temp_eth_hdr2 = NULL;
	s32 i = 0;
	s32 tx_len = 0;
	u8 tx_buf_ac = IF_DATA_AC_POWER;

	temp_params = (struct cfg80211_mgmt_tx_params *)params;

	if (c_if && temp_params && temp_params->len > 0 && temp_params->buf) {
		temp_skb = (struct sk_buff *)rs_net_skb_alloc(
			RS_C_GET_DATA_SIZE(RS_C_TX_EXT_LEN, temp_params->len));
		if (temp_skb) {
			tx_len = rs_net_skb_get_data((u8 *)temp_skb, (u8 **)&tx_data);
		}

		if (tx_data && tx_len == RS_C_GET_DATA_SIZE(RS_C_TX_EXT_LEN, temp_params->len)) {
			*cookie = (unsigned long)temp_skb;

			temp_eth_hdr = eth_hdr(temp_skb);
			temp_eth_hdr2 = (struct ethhdr *)(tx_data->data);

			(void)rs_k_memcpy(tx_data->data, temp_params->buf, temp_params->len);

			if (unlikely(temp_params->n_csa_offsets > 0) &&
			    (RS_NET_WDEV_IF_TYPE(vif_priv) == NL80211_IFTYPE_AP) && (vif_priv->ap.csa)) {
				for (i = 0; i < temp_params->n_csa_offsets; i++) {
					tx_data->data[temp_params->csa_offsets[i]] = vif_priv->ap.csa->count;
				}
			}

			tx_buf_ac = IF_DATA_AC_POWER;

			tx_data->cmd = RS_CMD_DATA_TX;
			tx_data->ext_len = RS_C_TX_EXT_LEN;
			tx_data->ext_hdr.buf_idx = tx_buf_ac;
			tx_data->ext_hdr.tid = RS_NET_INVALID_TID;
			tx_data->ext_hdr.vif_idx = vif_priv->vif_index;
			tx_data->ext_hdr.sta_idx = (sta) ? sta->sta_idx : RS_NET_INVALID_STA_IDX;

			tx_data->ext_hdr.flags = RS_C_EXT_FLAGS_MGMT;

			if (ieee80211_is_robust_mgmt_frame(temp_skb) != 0) {
				tx_data->ext_hdr.flags |= RS_C_EXT_FLAGS_MGMT_ROBUST;
			}
			if (temp_params->no_cck != 0) {
				tx_data->ext_hdr.flags |= RS_C_EXT_FLAGS_MGMT_NO_CCK;
			}

			tx_data->ext_hdr.sn = 0;
			tx_data->data_len = temp_params->len;

			RS_DBG("P:net_tx_mgmt[%d]:vif[%d]:sta[%d]:skb_data src[" MAC_ADDRESS_STR
			       " " MAC_ADDRESS_STR "],dest[" MAC_ADDRESS_STR " " MAC_ADDRESS_STR
			       "],prot[0x%X 0x%X]\n",
			       __LINE__, tx_data->ext_hdr.vif_idx, tx_data->ext_hdr.sta_idx,
			       MAC_ADDR_ARRAY(temp_eth_hdr->h_source),
			       MAC_ADDR_ARRAY(temp_eth_hdr2->h_source), MAC_ADDR_ARRAY(temp_eth_hdr->h_dest),
			       MAC_ADDR_ARRAY(temp_eth_hdr2->h_dest), ntohs(temp_eth_hdr->h_proto),
			       ntohs(temp_eth_hdr2->h_proto));

			ret = rs_c_tx_event_post(c_if, tx_buf_ac, vif_priv->vif_index, (u8 *)temp_skb);
		} else {
			rs_net_skb_free((u8 *)temp_skb);
			RS_ERR("SKB alloc failed!![%d][%d]\n", tx_len,
			       RS_C_GET_DATA_SIZE(RS_C_TX_EXT_LEN, temp_params->len));
		}
	}

	return ret;
}

// TX Data handler
rs_ret rs_net_tx_data(struct rs_c_if *c_if, struct rs_net_vif_priv *vif_priv, u8 *skb)
{
	rs_ret ret = RS_FAIL;
	u8 hdr_size = RS_C_GET_DATA_SIZE(RS_C_TX_EXT_LEN, 0);
	struct sk_buff *temp_skb = (struct sk_buff *)skb;
	struct rs_c_tx_data *tx_data = NULL;
	u8 prio = 0;
	struct ethhdr *temp_eth_hdr = NULL;
	u8 tx_buf_ac = IF_DATA_AC;
	u16 sta_idx = 0;

	if (c_if) {
		sta_idx = skb_get_queue_mapping(temp_skb);
		if (sta_idx == RS_NET_NDEV_INVALID_TXQ) {
			RS_DBG("P:%s[%d]:invalid frame\n", __func__, __LINE__);
		} else {
			tx_data = (struct rs_c_tx_data *)rs_net_skb_add_tx_hdr((u8 *)temp_skb, hdr_size);
		}

		if (tx_data) {
			temp_eth_hdr = eth_hdr(temp_skb);

			if ((temp_skb->priority == 0) ||
			    (temp_skb->priority > IEEE80211_QOS_CTL_TAG1D_MASK)) {
				prio = cfg80211_classify8021d(temp_skb, NULL);
			} else {
				prio = temp_skb->priority;
			}

			tx_data->cmd = RS_CMD_DATA_TX;
			tx_data->ext_len = RS_C_TX_EXT_LEN;
			tx_data->ext_hdr.buf_idx = tx_buf_ac;
			tx_data->ext_hdr.tid = prio;
			tx_data->ext_hdr.vif_idx = vif_priv->vif_index;
			tx_data->ext_hdr.sta_idx = sta_idx;
			tx_data->ext_hdr.flags = 0;
			tx_data->ext_hdr.sn = 0;
			tx_data->data_len = (temp_skb->len - hdr_size);

			ret = rs_c_tx_event_post(c_if, tx_buf_ac, vif_priv->vif_index, (u8 *)temp_skb);

			RS_DBG("P:net_tx[%d]:skb_data src[" MAC_ADDRESS_STR "],dest[" MAC_ADDRESS_STR
			       "],prot[0x%X]\n",
			       __LINE__, MAC_ADDR_ARRAY(temp_eth_hdr->h_source),
			       MAC_ADDR_ARRAY(temp_eth_hdr->h_dest), ntohs(temp_eth_hdr->h_proto));
		}

		if (ret != RS_SUCCESS) {
			if (temp_skb) {
				(void)rs_net_skb_free((u8 *)temp_skb);
			}
		}
	}

	return ret;
}
