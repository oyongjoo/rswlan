// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Copyright (C) [2022-2025] Renesas Electronics Corporation and/or its
 * affiliates.
 */
#include <linux/version.h>
#include <net/mac80211.h>
#include <net/netlink.h>

#include "rs_type.h"
#include "rs_c_dbg.h"
#include "rs_k_mem.h"
#include "rs_c_cmd.h"
#include "rs_c_if.h"
#include "rs_net_priv.h"
#include "rs_net_ctrl.h"
#ifdef CONFIG_DBG_STATS
#include "rs_net_stats.h"
#endif

#include "rs_net_testmode.h"

////////////////////////////////////////////////////////////////////////////////
/// MACRO DEFINITION

#define PRINT_CMD_INFO 0
#define TESTMODE_SIZE  1024

#ifdef CONFIG_DBG_STATS
#define BSSID_STR      "%02x:%02x:%02x:%02x:%02x:%02x"
#define BSSID_ARRAY(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#endif

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL FUNCTION

#ifdef CONFIG_NL80211_TESTMODE

s32 rs_net_testmode_reg(struct rs_c_if *c_if, struct nlattr **tb)
{
	u32 memaddr, val32;
	struct sk_buff *skb;
	s32 status = 0;
	struct rs_net_cfg80211_priv *net_priv = NULL;

	RS_TRACE(RS_FN_ENTRY_STR);

	if (!tb[TESTMODE_ATTR_REG_OFFSET]) {
		RS_ERR("Error finding register offset\n");
		return -ENOMSG;
	}

	net_priv = rs_c_if_get_net_priv(c_if);

	memaddr = nla_get_u32(tb[TESTMODE_ATTR_REG_OFFSET]);

	switch (nla_get_u32(tb[TESTMODE_ATTR_CMD])) {
	case TESTMODE_CMD_READ_REG: {
		struct rs_c_mem_read_rsp *mem_read_rsp = &net_priv->cmd_rsp.mem_read_rsp;

		status = rs_net_ctrl_mem_read_req(c_if, memaddr, mem_read_rsp);
		if (status)
			return status;

		skb = cfg80211_testmode_alloc_reply_skb(net_priv->wiphy, TESTMODE_SIZE);
		if (!skb) {
			RS_ERR("Error allocating memory\n");
			return -ENOMEM;
		}

		val32 = mem_read_rsp->memdata;
		if (nla_put_u32(skb, TESTMODE_ATTR_REG_VALUE32, val32))
			goto nla_put_failure;

		status = cfg80211_testmode_reply(skb);
		if (status < 0)
			RS_ERR("Error testmode reply error : %d\n", status);
	} break;

	case TESTMODE_CMD_WRITE_REG:
		if (!tb[TESTMODE_ATTR_REG_VALUE32]) {
			RS_ERR("Error finding value to write\n");
			return -ENOMSG;
		}

		val32 = nla_get_u32(tb[TESTMODE_ATTR_REG_VALUE32]);

		status = rs_net_ctrl_mem_write_req(c_if, memaddr, val32);
		if (status)
			return status;

		break;

	default:
		RS_ERR("Unknown TM reg cmd ID\n");
		return -EINVAL;
	}

	return status;

nla_put_failure:
	kfree_skb(skb);
	return -EMSGSIZE;
}

s32 rs_net_testmode_dbg_filter(struct rs_c_if *c_if, struct nlattr **tb)
{
	u32 filter;
	s32 status = 0;

	RS_TRACE(RS_FN_ENTRY_STR);

	if (!tb[TESTMODE_ATTR_REG_FILTER]) {
		RS_ERR("Error finding filter value\n");
		return -ENOMSG;
	}

	filter = nla_get_u32(tb[TESTMODE_ATTR_REG_FILTER]);
	RS_DBG("TM DBG filter, setting: 0x%x\n", filter);

	switch (nla_get_u32(tb[TESTMODE_ATTR_CMD])) {
	case TESTMODE_CMD_LOGMODEFILTER_SET:
		status = rs_net_ctrl_dbg_mode_filter_req(c_if, filter);
		if (status != 0)
			return status;
		break;
	case TESTMODE_CMD_DBGLEVELFILTER_SET:
		status = rs_net_ctrl_dbg_level_filter_req(c_if, filter);
		if (status != 0)
			return status;
		break;

	default:
		RS_ERR("Unknown testmode register command ID\n");
		return -EINVAL;
	}

	return status;
}

s32 rs_net_testmode_rf_tx(struct rs_c_if *c_if, struct nlattr **tb)
{
	s32 status = 0;
	struct rs_c_rf_tx_req data;

	RS_TRACE(RS_FN_ENTRY_STR);

	if (!tb[TESTMODE_ATTR_START]) {
		RS_ERR("Error finding RF TX parameters\n");
		return -ENOMSG;
	}

	data.start = nla_get_u8(tb[TESTMODE_ATTR_START]);

	if (data.start == TESTMODE_VALUE_START) {
		data.txRate = nla_get_u32(tb[TESTMODE_ATTR_RATE]);
		data.txPower = nla_get_u32(tb[TESTMODE_ATTR_POWER]);
		data.gi = nla_get_u8(tb[TESTMODE_ATTR_GI]);
		data.greenField = nla_get_u8(tb[TESTMODE_ATTR_GREEN]);
		data.preambleType = nla_get_u8(tb[TESTMODE_ATTR_PREAMBLE]);
		data.qosEnable = nla_get_u8(tb[TESTMODE_ATTR_QOS]);
		data.ackPolicy = nla_get_u8(tb[TESTMODE_ATTR_ACK]);
		data.aifsnVal = nla_get_u8(tb[TESTMODE_ATTR_AIFSN]);
		data.frequency = nla_get_u16(tb[TESTMODE_ATTR_CH]);
		data.numFrames = nla_get_u16(tb[TESTMODE_ATTR_FRAMES_NUM]);
		data.frameLen = nla_get_u16(tb[TESTMODE_ATTR_FRAMES_LEN]);
		data.destAddr = nla_get_u64(tb[TESTMODE_ATTR_ADDR_DEST]);
		data.bssid = nla_get_u64(tb[TESTMODE_ATTR_BSSID]);
	}

	status = rs_net_ctrl_rf_tx_req(c_if, &data);

	return status;
}

s32 rs_net_testmode_rf_cw(struct rs_c_if *c_if, struct nlattr **tb)
{
	s32 status = 0;
	struct rs_c_rf_cw_req data = { 0 };

	RS_TRACE(RS_FN_ENTRY_STR);

	if (!tb[TESTMODE_ATTR_START]) {
		RS_ERR("Error finding RF CW parameters\n");
		return -ENOMSG;
	}

	data.start = nla_get_u8(tb[TESTMODE_ATTR_START]);

	if (data.start == TESTMODE_VALUE_START) {
		data.txPower = nla_get_s8(tb[TESTMODE_ATTR_POWER]);
		data.frequency = nla_get_u16(tb[TESTMODE_ATTR_CH]);
	}

	status = rs_net_ctrl_rf_cw_req(c_if, &data);

	return status;
}

s32 rs_net_testmode_rf_cont(struct rs_c_if *c_if, struct nlattr **tb)
{
	s32 status = 0;
	struct rs_c_rf_cont_req data = { 0 };

	RS_TRACE(RS_FN_ENTRY_STR);

	if (!tb[TESTMODE_ATTR_START]) {
		RS_ERR("Error finding RF CONT parameters\n");
		return -ENOMSG;
	}

	data.start = nla_get_u8(tb[TESTMODE_ATTR_START]);

	if (data.start == TESTMODE_VALUE_START) {
		data.txPower = nla_get_s8(tb[TESTMODE_ATTR_POWER]);
		data.frequency = nla_get_u16(tb[TESTMODE_ATTR_CH]);
		data.txRate = nla_get_u32(tb[TESTMODE_ATTR_RATE]);
	}

	status = rs_net_ctrl_rf_cont_req(c_if, &data);

	return status;
}

s32 rs_net_testmode_rf_ch(struct rs_c_if *c_if, struct nlattr **tb)
{
	s32 status = 0;
	u16 frequency = 0;

	RS_TRACE(RS_FN_ENTRY_STR);

	if (!tb[TESTMODE_ATTR_CH]) {
		RS_ERR("Error finding RF CH parameters\n");
		return -ENOMSG;
	}

	frequency = nla_get_u16(tb[TESTMODE_ATTR_CH]);

	status = rs_net_ctrl_rf_ch_req(c_if, frequency);

	return status;
}

s32 rs_net_testmode_rf_per(struct rs_c_if *c_if, struct nlattr **tb)
{
	s32 status = 0;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct rs_c_rf_per_rsp *rf_per_rsp;
	u8 start = 0;
	struct sk_buff *skb = NULL;

	RS_TRACE(RS_FN_ENTRY_STR);

	if (!tb[TESTMODE_ATTR_START]) {
		RS_ERR("Error finding RF PER parameters\n");
		return -ENOMSG;
	}

	net_priv = rs_c_if_get_net_priv(c_if);
	rf_per_rsp = &net_priv->cmd_rsp.rf_per_rsp;

	start = nla_get_u8(tb[TESTMODE_ATTR_START]);

	status = rs_net_ctrl_rf_per_req(c_if, start, rf_per_rsp);
	if (status)
		return status;

	if (start == TESTMODE_VALUE_PER_GET) {
		skb = cfg80211_testmode_alloc_reply_skb(net_priv->wiphy, TESTMODE_SIZE);
		if (!skb) {
			RS_ERR("Error allocating memory\n");
			return -ENOMEM;
		}

		if (nla_put_u32(skb, TESTMODE_ATTR_PER_PASS, rf_per_rsp->pass))
			goto nla_put_failure;

		if (nla_put_u32(skb, TESTMODE_ATTR_PER_FCS, rf_per_rsp->fcs))
			goto nla_put_failure;

		if (nla_put_u32(skb, TESTMODE_ATTR_PER_PHY, rf_per_rsp->phy))
			goto nla_put_failure;

		if (nla_put_u32(skb, TESTMODE_ATTR_PER_OVERFLOW, rf_per_rsp->overflow))
			goto nla_put_failure;

		status = cfg80211_testmode_reply(skb);
		if (status < 0)
			RS_ERR("Error testmode per set error : %d\n", status);
	}

	return status;

nla_put_failure:
	if (skb)
		kfree_skb(skb);
	return -EMSGSIZE;
}

s32 rs_net_testmode_host_log_level(struct rs_c_if *c_if, struct nlattr **tb)
{
	s32 status = 0;
	u8 level = 0;

	if (!tb[TESTMODE_ATTR_HOST_LOG_LEVEL]) {
		RS_ERR("Error finding level attribute\n");
		return -ENOMSG;
	}

	level = nla_get_u8(tb[TESTMODE_ATTR_HOST_LOG_LEVEL]);

	RS_SET_DBG_LEVEL(level);

	return status;
}

s32 rs_net_testmode_tx_power(struct rs_c_if *c_if, struct nlattr **tb, struct rs_net_vif_priv *vif_priv)
{
	s32 status = 0;
	u32 tx_power = 0;

	if (!tb[TESTMODE_ATTR_POWER]) {
		RS_ERR("Error finding tx power attribute\n");
		return -ENOMSG;
	}

	tx_power = nla_get_u32(tb[TESTMODE_ATTR_POWER]);

	status = rs_net_ctrl_set_tx_power_req(c_if, vif_priv->vif_index, tx_power);

	return status;
}

#ifdef CONFIG_DBG_STATS

#define DBG_STATS_STARTED 1

struct rs_net_sta_priv *is_sta_valid(struct rs_net_vif_priv *vif_priv)
{
	struct rs_net_sta_priv *sta_priv = NULL;

	if (RS_NET_WDEV_IF_TYPE(vif_priv) == NL80211_IFTYPE_AP ||
	    RS_NET_WDEV_IF_TYPE(vif_priv) == NL80211_IFTYPE_P2P_GO) {
		list_for_each_entry(sta_priv, &vif_priv->ap.sta_list, list) {
			if (sta_priv->valid) {
				return sta_priv;
			}
		}
		return NULL;
	} else if (RS_NET_WDEV_IF_TYPE(vif_priv) == NL80211_IFTYPE_STATION ||
		   RS_NET_WDEV_IF_TYPE(vif_priv) == NL80211_IFTYPE_P2P_CLIENT) {
		sta_priv = vif_priv->sta.ap;
	}

	return sta_priv;
}

s32 rs_net_testmod_stats_start(struct rs_c_if *c_if, struct nlattr **tb, struct rs_net_vif_priv *vif_priv)
{
	s32 status = 0;
	struct rs_net_cfg80211_priv *net_priv = NULL;

	net_priv = rs_vif_priv_get_net_priv(vif_priv);
	status = rs_net_stats_init(net_priv);

	if (status == DBG_STATS_STARTED)
		return 0;

	rs_net_ctrl_dbg_stats_tx_req(c_if, REQ_STATS_START, NULL);

	return status;
}

s32 rs_net_testmod_stats_stop(struct rs_c_if *c_if, struct nlattr **tb, struct rs_net_vif_priv *vif_priv)
{
	s32 status = 0;
	struct rs_net_cfg80211_priv *net_priv = NULL;

	net_priv = rs_vif_priv_get_net_priv(vif_priv);
	status = rs_net_stats_deinit(net_priv);

	return status;
}

s32 rs_net_testmod_stats_tx(struct rs_c_if *c_if, struct nlattr **tb, struct rs_net_vif_priv *vif_priv)
{
	s32 status = 0;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct rs_net_sta_priv *sta_priv = NULL;
	struct rs_c_dbg_stats_tx_rsp *stats_tx_rsp;
	struct rs_net_dbg_stats *dbg_stats = NULL;
	struct wiphy *wiphy = NULL;
	struct sk_buff *skb;
	char bssid[18], mac_address[18];

	net_priv = rs_vif_priv_get_net_priv(vif_priv);
	dbg_stats = &net_priv->dbg_stats;

	net_priv = rs_vif_priv_get_net_priv(vif_priv);
	wiphy = rs_net_priv_get_wiphy(net_priv);

	if (!net_priv->dbg_stats.dbg_stats_enabled)
		return -1;

	stats_tx_rsp = &net_priv->cmd_rsp.dbg_stats_tx_rsp;
	status = rs_net_ctrl_dbg_stats_tx_req(c_if, REQ_STATS_TX, stats_tx_rsp);
	if (status)
		return status;

	if (!stats_tx_rsp->status) // not started
		return -1;

	skb = cfg80211_testmode_alloc_reply_skb(wiphy, TESTMODE_SIZE);
	if (!skb) {
		RS_ERR("Error allocating memory\n");
		return -ENOMEM;
	}

	sta_priv = is_sta_valid(vif_priv);
	if (sta_priv == NULL)
		goto not_connected;

	snprintf(bssid, sizeof(bssid), BSSID_STR, BSSID_ARRAY(sta_priv->mac_addr));
	snprintf(mac_address, sizeof(mac_address), BSSID_STR, BSSID_ARRAY(wiphy->perm_addr));

	if (nla_put_u8(skb, TESTMODE_ATTR_STATS_FLAG, stats_tx_rsp->format_mod) ||
	    nla_put(skb, TESTMODE_ATTR_STATS_TX_EPR, sizeof(stats_tx_rsp->epr), stats_tx_rsp->epr) ||
	    nla_put_string(skb, TESTMODE_ATTR_STATS_BSSID, bssid) ||
	    nla_put_string(skb, TESTMODE_ATTR_STATS_OWN_MAC, mac_address) ||
	    nla_put_u8(skb, TESTMODE_ATTR_STATS_STAIDX, sta_priv->sta_idx))
		goto nla_put_failure;

	switch (stats_tx_rsp->format_mod) {
	case FORMATMOD_NON_HT:
	case FORMATMOD_NON_HT_DUP_OFDM:
		if (nla_put(skb, TESTMODE_ATTR_STATS_TX_CCK, sizeof(stats_tx_rsp->non_ht.success),
			    stats_tx_rsp->non_ht.success) ||
		    nla_put(skb, TESTMODE_ATTR_STATS_TX_CCK_FAIL, sizeof(stats_tx_rsp->non_ht.fail),
			    stats_tx_rsp->non_ht.fail))
			goto nla_put_failure;
		break;
	case FORMATMOD_HT_MF:
	case FORMATMOD_HT_GF:
		if (nla_put(skb, TESTMODE_ATTR_STATS_TX_HT, sizeof(stats_tx_rsp->ht.success),
			    stats_tx_rsp->ht.success) ||
		    nla_put(skb, TESTMODE_ATTR_STATS_TX_HT_FAIL, sizeof(stats_tx_rsp->ht.fail),
			    stats_tx_rsp->ht.fail))
			goto nla_put_failure;
		break;
	case FORMATMOD_VHT:
		if (nla_put(skb, TESTMODE_ATTR_STATS_TX_VHT, sizeof(stats_tx_rsp->vht.success),
			    stats_tx_rsp->vht.success) ||
		    nla_put(skb, TESTMODE_ATTR_STATS_TX_VHT_FAIL, sizeof(stats_tx_rsp->vht.fail),
			    stats_tx_rsp->vht.fail))
			goto nla_put_failure;
		break;
	case FORMATMOD_HE_SU:
	case FORMATMOD_HE_ER:
	case FORMATMOD_HE_MU:
	case FORMATMOD_HE_TB:
		if (nla_put(skb, TESTMODE_ATTR_STATS_TX_HE, sizeof(stats_tx_rsp->he.success),
			    stats_tx_rsp->he.success) ||
		    nla_put(skb, TESTMODE_ATTR_STATS_TX_HE_FAIL, sizeof(stats_tx_rsp->he.fail),
			    stats_tx_rsp->he.fail))
			goto nla_put_failure;
		break;
	};

	status = cfg80211_testmode_reply(skb);
	if (status < 0)
		RS_ERR("Error testmode reply error : %d\n", status);

	return status;

not_connected:
	kfree_skb(skb);
	return -ENOTCONN;

nla_put_failure:
	kfree_skb(skb);
	return -EMSGSIZE;
}

s32 rs_net_testmod_stats_rx(struct rs_c_if *c_if, struct nlattr **tb, struct rs_net_vif_priv *vif_priv)
{
	s32 status = 0;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct rs_net_sta_priv *sta_priv = NULL;
	struct wiphy *wiphy = NULL;
	struct sk_buff *skb;
	char bssid[18], mac_address[18];
	struct rs_net_rx_stats *rx_stats = NULL;

	net_priv = rs_vif_priv_get_net_priv(vif_priv);
	wiphy = rs_net_priv_get_wiphy(net_priv);
	rx_stats = net_priv->dbg_stats.rx_stats;
	if (!rx_stats)
		return -1;

	skb = cfg80211_testmode_alloc_reply_skb(net_priv->wiphy, TESTMODE_SIZE);
	if (!skb) {
		RS_ERR("Error allocating memory\n");
		return -ENOMEM;
	}

	if (nla_put_u8(skb, TESTMODE_ATTR_STATS_READY, net_priv->dbg_stats.dbg_stats_enabled))
		goto nla_put_failure;

	sta_priv = is_sta_valid(vif_priv);
	if (sta_priv == NULL)
		goto not_connected;

	snprintf(bssid, sizeof(bssid), BSSID_STR, BSSID_ARRAY(sta_priv->mac_addr));
	snprintf(mac_address, sizeof(mac_address), BSSID_STR, BSSID_ARRAY(wiphy->perm_addr));

	if (nla_put_u8(skb, TESTMODE_ATTR_STATS_FLAG, rx_stats->flag) ||
	    nla_put_string(skb, TESTMODE_ATTR_STATS_BSSID, bssid) ||
	    nla_put_string(skb, TESTMODE_ATTR_STATS_OWN_MAC, mac_address) ||
	    nla_put_u8(skb, TESTMODE_ATTR_STATS_STAIDX, sta_priv->sta_idx))
		goto nla_put_failure;

	if (rx_stats->flag & RX_STATS_OFDM || rx_stats->flag & RX_STATS_CCK) {
		if (nla_put_u8(skb, TESTMODE_ATTR_STATS_RX_BW, rx_stats->non_ht.bw) ||
		    nla_put_u64_64bit(skb, TESTMODE_ATTR_STATS_RX_CCK, rx_stats->non_ht.cck,
				      NL80211_ATTR_PAD) ||
		    nla_put_u64_64bit(skb, TESTMODE_ATTR_STATS_RX_OFDM, rx_stats->non_ht.ofdm,
				      NL80211_ATTR_PAD))
			goto nla_put_failure;
	}

	if (rx_stats->flag & RX_STATS_HT) {
		if (nla_put_u8(skb, TESTMODE_ATTR_STATS_RX_BW, rx_stats->ht.bw) ||
		    nla_put_u8(skb, TESTMODE_ATTR_STATS_RX_NSS, rx_stats->ht.nss) ||
		    nla_put_u8(skb, TESTMODE_ATTR_STATS_RX_GI, rx_stats->ht.gi) ||
		    nla_put(skb, TESTMODE_ATTR_STATS_RX_HT, sizeof(rx_stats->ht.ht), rx_stats->ht.ht))
			goto nla_put_failure;
	}

	if (rx_stats->flag & RX_STATS_VHT) {
		if (nla_put_u8(skb, TESTMODE_ATTR_STATS_RX_BW, rx_stats->vht.bw) ||
		    nla_put_u8(skb, TESTMODE_ATTR_STATS_RX_NSS, rx_stats->vht.nss) ||
		    nla_put_u8(skb, TESTMODE_ATTR_STATS_RX_GI, rx_stats->vht.gi) ||
		    nla_put(skb, TESTMODE_ATTR_STATS_RX_VHT, sizeof(rx_stats->vht.vht), rx_stats->vht.vht))
			goto nla_put_failure;
	}

	if (rx_stats->flag & RX_STATS_HE_SU) {
		if (nla_put_u8(skb, TESTMODE_ATTR_STATS_RX_BW, rx_stats->he_su.bw) ||
		    nla_put_u8(skb, TESTMODE_ATTR_STATS_RX_NSS, rx_stats->he_su.nss) ||
		    nla_put_u8(skb, TESTMODE_ATTR_STATS_RX_GI, rx_stats->he_su.gi) ||
		    nla_put(skb, TESTMODE_ATTR_STATS_RX_HE_SU, sizeof(rx_stats->he_su.he),
			    rx_stats->he_su.he))
			goto nla_put_failure;
	}

	status = cfg80211_testmode_reply(skb);
	if (status < 0)
		RS_ERR("Error testmode reply error : %d\n", status);

	return status;

not_connected:
	kfree_skb(skb);
	return -ENOTCONN;

nla_put_failure:
	kfree_skb(skb);
	return -EMSGSIZE;
}

s32 rs_net_testmod_stats_rssi(struct rs_c_if *c_if, struct nlattr **tb, struct rs_net_vif_priv *vif_priv)
{
	s32 status = 0;
	struct rs_net_cfg80211_priv *net_priv = NULL;
	struct rs_net_sta_priv *sta_priv = NULL;
	struct rs_net_sta_stats *stats = NULL;
	struct rs_net_dbg_stats *dbg_stats = NULL;
	struct wiphy *wiphy = NULL;
	struct sk_buff *skb;
	char bssid[18], mac_address[18];

	net_priv = rs_vif_priv_get_net_priv(vif_priv);
	wiphy = rs_net_priv_get_wiphy(net_priv);
	sta_priv = vif_priv->sta.ap;
	stats = &sta_priv->stats;
	dbg_stats = &net_priv->dbg_stats;

	if (!dbg_stats->dbg_stats_enabled)
		return -1;

	skb = cfg80211_testmode_alloc_reply_skb(net_priv->wiphy, TESTMODE_SIZE);
	if (!skb) {
		RS_ERR("Error allocating memory\n");
		return -ENOMEM;
	}

	if (RS_NET_WDEV_IF_TYPE(vif_priv) == NL80211_IFTYPE_AP ||
	    RS_NET_WDEV_IF_TYPE(vif_priv) == NL80211_IFTYPE_P2P_GO)
		goto not_supported;

	if (!sta_priv) {
		goto nla_put_failure;
	}

	snprintf(bssid, sizeof(bssid), BSSID_STR, BSSID_ARRAY(sta_priv->mac_addr));
	snprintf(mac_address, sizeof(mac_address), BSSID_STR, BSSID_ARRAY(wiphy->perm_addr));

	if (nla_put_u8(skb, TESTMODE_ATTR_STATS_READY, net_priv->dbg_stats.dbg_stats_enabled) ||
	    nla_put_u32(skb, TESTMODE_ATTR_STATS_RSSI, stats->last_rx_data_ext.rssi1) ||
	    nla_put_string(skb, TESTMODE_ATTR_STATS_BSSID, bssid) ||
	    nla_put_string(skb, TESTMODE_ATTR_STATS_OWN_MAC, mac_address) ||
	    nla_put_u8(skb, TESTMODE_ATTR_STATS_STAIDX, sta_priv->sta_idx))
		goto nla_put_failure;

	status = cfg80211_testmode_reply(skb);
	if (status < 0)
		RS_ERR("Error testmode reply error : %d\n", status);

	return status;

not_supported:
	kfree_skb(skb);
	return -EPROTONOSUPPORT;

nla_put_failure:
	kfree_skb(skb);
	return -EMSGSIZE;
}
#endif

#endif // CONFIG_NL80211_TESTMODE