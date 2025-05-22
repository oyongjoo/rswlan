// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Copyright (C) [2022-2025] Renesas Electronics Corporation and/or its
 * affiliates.
 */
#ifndef _RS_NET_TESTMODE_H_
#define _RS_NET_TESTMODE_H_

enum rs_tm_cmd
{
	TESTMODE_CMD_READ_REG = 1,
	TESTMODE_CMD_WRITE_REG,
	TESTMODE_CMD_LOGMODEFILTER_SET,
	TESTMODE_CMD_DBGLEVELFILTER_SET,
	TESTMODE_CMD_TX,
	TESTMODE_CMD_CW,
	TESTMODE_CMD_CONT,
	TESTMODE_CMD_CHANNEL,
	TESTMODE_CMD_PER,
	TESTMODE_CMD_RESET_HW,
	TESTMODE_CMD_HOST_LOG_LEVEL,
	TESTMODE_CMD_DBGOUTDIR_SET,
	TESTMODE_CMD_TX_POWER,

#ifdef CONFIG_DBG_STATS
	TESTMODE_CMD_STATS_START,
	TESTMODE_CMD_STATS_STOP,
	TESTMODE_CMD_STATS_TX,
	TESTMODE_CMD_STATS_RX,
	TESTMODE_CMD_STATS_RSSI,
#endif
	TESTMODE_CMD_MAX,
};

enum rs_tm_attr
{
	TESTMODE_ATTR_NOT_APPLICABLE = 0,
	TESTMODE_ATTR_CMD = 1,
	TESTMODE_ATTR_REG_OFFSET = 2,
	TESTMODE_ATTR_REG_VALUE32 = 3,
	TESTMODE_ATTR_REG_FILTER = 4,
	TESTMODE_ATTR_START = 5,
	TESTMODE_ATTR_CH = 6,
	TESTMODE_ATTR_FRAMES_NUM = 7,
	TESTMODE_ATTR_FRAMES_LEN = 8,
	TESTMODE_ATTR_RATE = 9,
	TESTMODE_ATTR_POWER = 10,
	TESTMODE_ATTR_ADDR_DEST = 11,
	TESTMODE_ATTR_BSSID = 12,
	TESTMODE_ATTR_GI = 13,
	TESTMODE_ATTR_GREEN = 14,
	TESTMODE_ATTR_PREAMBLE = 15,
	TESTMODE_ATTR_QOS = 16,
	TESTMODE_ATTR_ACK = 17,
	TESTMODE_ATTR_AIFSN = 18,
	TESTMODE_ATTR_PER_PASS = 19,
	TESTMODE_ATTR_PER_FCS = 20,
	TESTMODE_ATTR_PER_PHY = 21,
	TESTMODE_ATTR_PER_OVERFLOW = 22,
	TESTMODE_ATTR_HOST_LOG_LEVEL = 23,

#ifdef CONFIG_DBG_STATS
	TESTMODE_ATTR_STATS_READY,
	TESTMODE_ATTR_STATS_RSSI,
	TESTMODE_ATTR_STATS_BSSID,
	TESTMODE_ATTR_STATS_STAIDX,
	TESTMODE_ATTR_STATS_OWN_MAC,
	TESTMODE_ATTR_STATS_FLAG,

	TESTMODE_ATTR_STATS_RX_BW,
	TESTMODE_ATTR_STATS_RX_MCS,
	TESTMODE_ATTR_STATS_RX_GI,
	TESTMODE_ATTR_STATS_RX_NSS,
	TESTMODE_ATTR_STATS_RX_CCK,
	TESTMODE_ATTR_STATS_RX_OFDM,
	TESTMODE_ATTR_STATS_RX_HT,
	TESTMODE_ATTR_STATS_RX_VHT,
	TESTMODE_ATTR_STATS_RX_HE_SU,
	TESTMODE_ATTR_STATS_RX_HE_MU,
	TESTMODE_ATTR_STATS_RX_HE_EXT,
	TESTMODE_ATTR_STATS_RX_HE_TRIG,

	//STATS TX

	TESTMODE_ATTR_STATS_TX_CCK,
	TESTMODE_ATTR_STATS_TX_CCK_FAIL,
	TESTMODE_ATTR_STATS_TX_OFDM,
	TESTMODE_ATTR_STATS_TX_OFDM_FAIL,
	TESTMODE_ATTR_STATS_TX_HT,
	TESTMODE_ATTR_STATS_TX_HT_FAIL,
	TESTMODE_ATTR_STATS_TX_VHT,
	TESTMODE_ATTR_STATS_TX_VHT_FAIL,
	TESTMODE_ATTR_STATS_TX_HE,
	TESTMODE_ATTR_STATS_TX_HE_FAIL,
	TESTMODE_ATTR_STATS_TX_EPR,
#endif

	TESTMODE_ATTR_MAX,
};

enum
{
	TESTMODE_VALUE_STOP = 0,
	TESTMODE_VALUE_START,
};

enum
{
	TESTMODE_VALUE_PER_STOP = 0,
	TESTMODE_VALUE_PER_START,
	TESTMODE_VALUE_PER_GET,
	TESTMODE_VALUE_PER_RESET,
};

enum rate_value // R_[80211b/g/n] + <Number> Mbps
{
	RATE_B1 = 0,
	RATE_B2,
	RATE_B5_5,
	RATE_B11,
	RATE_G6,
	RATE_G9,
	RATE_G12,
	RATE_G18,
	RATE_G24,
	RATE_G36,
	RATE_G48,
	RATE_G54,
	RATE_N6_5,
	RATE_N13,
	RATE_N19_5,
	RATE_N26,
	RATE_N39,
	RATE_N52,
	RATE_N58_5,
	RATE_N65,
};

enum
{
	TESTMODE_VALUE_SHORT = 0,
	TESTMODE_VALUE_LONG,
};

enum
{
	TESTMODE_VALUE_OFF = 0,
	TESTMODE_VALUE_ON,
};

enum
{
	TESTMODE_VALUE_NO = 0,
	TESTMODE_VALUE_NORM,
	TESTMODE_VALUE_BA,
	TESTMODE_VALUE_CBA,
};

s32 rs_net_testmode_reg(struct rs_c_if *c_if, struct nlattr **tb);
s32 rs_net_testmode_dbg_filter(struct rs_c_if *c_if, struct nlattr **tb);
s32 rs_net_testmode_rf_tx(struct rs_c_if *c_if, struct nlattr **tb);
s32 rs_net_testmode_rf_cw(struct rs_c_if *c_if, struct nlattr **tb);
s32 rs_net_testmode_rf_cont(struct rs_c_if *c_if, struct nlattr **tb);
s32 rs_net_testmode_rf_ch(struct rs_c_if *c_if, struct nlattr **tb);
s32 rs_net_testmode_rf_per(struct rs_c_if *c_if, struct nlattr **tb);
s32 rs_net_testmode_host_log_level(struct rs_c_if *c_if, struct nlattr **tb);
s32 rs_net_testmode_tx_power(struct rs_c_if *c_if, struct nlattr **tb, struct rs_net_vif_priv *vif_priv);
#ifdef CONFIG_DBG_STATS
s32 rs_net_testmod_stats_start(struct rs_c_if *c_if, struct nlattr **tb, struct rs_net_vif_priv *vif_priv);
s32 rs_net_testmod_stats_stop(struct rs_c_if *c_if, struct nlattr **tb, struct rs_net_vif_priv *vif_priv);
s32 rs_net_testmod_stats_tx(struct rs_c_if *c_if, struct nlattr **tb, struct rs_net_vif_priv *vif_priv);
s32 rs_net_testmod_stats_rx(struct rs_c_if *c_if, struct nlattr **tb, struct rs_net_vif_priv *vif_priv);
s32 rs_net_testmod_stats_rssi(struct rs_c_if *c_if, struct nlattr **tb, struct rs_net_vif_priv *vif_priv);
#endif
#endif // _RS_NET_TESTMODE_H_
