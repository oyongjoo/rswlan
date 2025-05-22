/**
 * Copyright (C) [2022-2025] Renesas Electronics Corporation and/or its
 * affiliates.
 * All Rights Reserved. Confidential Information.
 * For Renesas internal use only - external distribution not permitted.
 */

#ifndef RS_C_CMD_H
#define RS_C_CMD_H

////////////////////////////////////////////////////////////////////////////////
/// INCLUDE

////////////////////////////////////////////////////////////////////////////////
/// MACRO DEFINITION

#define RS_C_PHY_CFG_BUF_SIZE (16)
#define RS_C_MAX_Q	      (4)

/// MCS bitfield maximum size (in bytes)
#define RS_C_MAX_MCS_LEN      (16) // 16 * 8 = 128

#define MAC_CAPA_HE_LEN	      (6)
#define PHY_CAPA_HE_LEN	      (11)
/// Maximum length (in bytes) of the PPE threshold data
#define HE_PPE_THRES_MAX      (25)

/// For Scanu messages
/// Maximum length of the additional ProbeReq IEs (FullMAC mode)
#define RS_C_SCAN_IE_LEN      (200)
#define RS_C_SCAN_SSID_MAX    (2)
#define RS_C_SCAN_CHANNEL_2G  (14)
#define RS_C_SCAN_CHANNEL_5G  (28)
#define RS_C_SCAN_CHANNEL_MAX (RS_C_SCAN_CHANNEL_2G + RS_C_SCAN_CHANNEL_5G)
#define RS_SCAN_PASSIVE_BIT   RS_BIT(0)

#define RS_C_TX_LIFETIME_MS   (100)

#define RS_C_BT_COEX_MAX      (3)

#define RS_C_SSID_LEN	      (32)
#define RS_C_ETH_ALEN	      (6)

/// Structure containing the parameters of the @ref MACM_BCN_CHANGE_REQ message.
#define RS_C_BCN_MAX_CSA_CPT  (2)

#define RS_C_SECURY_KEY_LEN   (32)

#define RS_C_IS_CMD(cmd)                                                                   \
	(RS_C_IS_DATA_CMD(cmd) || RS_C_IS_COMMON_CMD(cmd) || RS_C_IS_FMAC_CTRL_CMD(cmd) || \
	 RS_C_IS_FMAC_INDI_CMD(cmd) || RS_C_IS_DBG_CMD(cmd))

#define RS_C_IS_DATA_RX(cmd)	   ((cmd) == RS_CMD_DATA_RX)
#define RS_C_IS_STATUS_RX(cmd)	   ((cmd) == RS_CMD_STATUS_RX)
#define RS_C_IS_DATA_CMD(cmd)	   (((cmd) > RS_CMD_DATA_START) && ((cmd) < RS_CMD_DATA_MAX))
#define RS_C_IS_COMMON_CMD(cmd)	   (((cmd) > RS_CMD_COMMON_START) && ((cmd) < RS_CMD_COMMON_MAX))
#define RS_C_IS_FMAC_CTRL_CMD(cmd) (((cmd) > RS_CMD_FULLMAC_START) && ((cmd) < RS_CMD_FULLMAC_MAX))
#define RS_C_IS_FMAC_INDI_CMD(cmd) (((cmd) > RS_IND_FULLMAC_START) && ((cmd) < RS_IND_FULLMAC_MAX))
#define RS_C_IS_DBG_CMD(cmd)	   (((cmd) > RS_CMD_DBG_START) && ((cmd) < RS_CMD_DBG_MAX))

////////////////////////////////////////////////////////////////////////////////
/// TYPE DEFINITION

enum rs_data_cmd
{
	RS_CMD_DATA_START = 0,

	RS_CMD_DATA_RX = 1,
	RS_CMD_STATUS_RX = 2,
	RS_CMD_DATA_TX = 5,

	RS_CMD_DATA_MAX
};

enum rs_common_cmd
{
	RS_CMD_COMMON_START = 10,

	RS_DEV_RESET_CMD = 11,
	RS_DEV_GET_MAC_ADDR_CMD = 12,

	RS_CMD_COMMON_MAX
};

enum rs_ctrl_cmd
{
	RS_CMD_FULLMAC_START = 100,

	RS_MM_RESET_CMD = 101, // MAC Manager
	RS_MM_START_CMD = 102,
	RS_MM_GET_VER_CMD = 103,
	RS_MM_ADD_IF_CMD = 104,
	RS_MM_RM_IF_CMD = 105,
	RS_MM_ADD_KEY_CMD = 106,
	RS_MM_DEL_KEY_CMD = 107,
	RS_MM_SET_POWER_CMD = 108,
	RS_MM_GET_RSSI_CMD = 109,
	RS_MM_REMAIN_ON_CHANNEL_CMD = 110,
	RS_MM_SET_EDCA_CMD = 111,
	RS_MM_TIME_SYNC_CMD = 112,

	RS_ME_CONFIG_CMD = 113, // MAC Entity
	RS_ME_CHAN_CONFIG_CMD = 114,
	RS_ME_SET_CONTROL_PORT_CMD = 115,
	RS_ME_ADD_STA_CMD = 116,
	RS_ME_DEL_STA_CMD = 117,
	RS_ME_TRAFFIC_IND_CMD = 118,
	RS_ME_GET_RATE_STATS_CMD = 119,
	RS_ME_SET_RATE_CFG_CMD = 120,
	RS_ME_SET_MON_CFG_CMD = 121,
	RS_ME_SET_PS_MODE_CMD = 122,

	RS_SC_START_CMD = 123, // SCan
	RS_SC_JOIN_CMD = 124,
	RS_SC_GET_RESULT_CMD = 125,
	RS_SC_CANCEL_CMD = 126,

	RS_SM_CONNECT_CMD = 127, // Station Manager
	RS_SM_DISCONNECT_CMD = 128,
	RS_SM_EXTERNAL_AUTH_REQUIRED_RSP_CMD = 129,
	RS_SM_FT_AUTH_RSP_CMD = 130,

	RS_TWT_SETUP_CMD = 131, // TWT
	RS_TWT_TEARDOWN_CMD = 132,

	RS_AM_START_CMD = 133, // Ap Manager
	RS_AM_STOP_CMD = 134,
	RS_AM_START_CHAN_AVAIL_CMD = 135,
	RS_AM_STOP_CHAN_AVAIL_CMD = 136,
	RS_AM_PROBE_CLIENT_CMD = 137,
	RS_AM_BCN_CHANGE_CMD = 138,
	RS_AM_ISOLATE_CMD = 139,

	RS_TD_CHAN_SWITCH_CMD = 140, // TDLS
	RS_TD_CHAN_SWITCH_CANCEL_CMD = 141,
	RS_TD_PEER_TRAFFIC_IND_CMD = 142,

	RS_CMD_FULLMAC_MAX
};

enum rs_ctrl_ind
{
	RS_IND_FULLMAC_START = 180,

	RS_ME_TKIP_MIC_FAILURE_IND = 181, // MAC Entity
	RS_ME_TX_CREDITS_UPDATE_IND = 182,

	RS_SC_RESULT_IND = 183, // SCan
	RS_SC_COMPLETE_IND = 184,
	RS_SC_CHANNEL_SURVEY_IND = 185,

	RS_SM_CONNECT_IND = 186, // Station Manager
	RS_SM_DISCONNECT_IND = 187,
	RS_SM_FT_AUTH_IND = 188,
	RS_SM_EXTERNAL_AUTH_REQUIRED_IND = 189,

	RS_TWT_SETUP_IND = 190, // TWT

	RS_AM_PROBE_CLIENT_IND = 191, // Ap Manager

	RS_MM_CHANNEL_SWITCH_IND = 192, // MAC Manager
	RS_MM_CHANNEL_PRE_SWITCH_IND = 193,
	RS_MM_REMAIN_ON_CHANNEL_EXP_IND = 194,
	RS_MM_PS_CHANGE_IND = 195,
	RS_MM_TRAFFIC_REQ_IND = 196,
	RS_MM_P2P_VIF_PS_CHANGE_IND = 197,
	RS_MM_CSA_COUNTER_IND = 198,
	RS_MM_P2P_NOA_UPDATE_IND = 199,
	RS_MM_RSSI_STATUS_IND = 200,
	RS_MM_CSA_FINISH_IND = 201,
	RS_MM_CSA_TRAFFIC_IND = 202,
	RS_MM_PACKET_LOSS_IND = 203,

	RS_TD_CHAN_SWITCH_IND = 204, // TDLS
	RS_TD_CHAN_SWITCH_BASE_CHAN_IND = 205,
	RS_TD_PEER_PS_IND = 206,

	RS_DBG_ERROR_IND = 207, // DBG
#ifdef CONFIG_RS_5G_DFS
	RS_RADAR_DETECT_IND = 208, // RADAR DETECT
#endif 
	RS_IND_FULLMAC_MAX
};

enum rs_dbg_cmd
{
	RS_CMD_DBG_START = 220,

	RS_DBG_MEM_READ_CMD = 221,
	RS_DBG_MEM_WRITE_CMD = 222,
	RS_DBG_SET_MOD_FILTER_CMD = 223,
	RS_DBG_SET_SEV_FILTER_CMD = 224,
	RS_DBG_SET_OUT_DIR_CMD = 225,
	RS_DBG_GET_SYS_STAT_CMD = 226,
	RS_DBG_RF_TX_CMD = 227,
	RS_DBG_RF_CW_CMD = 228,
	RS_DBG_RF_CONT_CMD = 229,
	RS_DBG_RF_CH_CMD = 230,
	RS_DBG_RF_PER_CMD = 231,
	RS_DBG_STATS_TX_CMD = 232,

	RS_CMD_DBG_MAX
};

// Access Categories
enum rs_c_ac
{
	RS_AC_BK = 0, // Background
	RS_AC_BE, // Best-effort
	RS_AC_VI, // Video
	RS_AC_VO, // Voice

	RS_AC_MAX,
};

// Traffic Priority Identifier
enum rs_c_tid
{
	RS_TID_0, // AC_BE
	RS_TID_1, // AC_BK
	RS_TID_2, // AC_BK
	RS_TID_3, // AC_BE
	RS_TID_4, // AC_VI
	RS_TID_5, // AC_VI
	RS_TID_6, // AC_VI
	RS_TID_7, // AC_VO

	RS_TID_MGT, // Non standard - for management frame

	RS_TID_MAX,
};

/// Operating Channel Bandwidth
enum mac_chan_bandwidth // TODO : change
{
	/// 20MHz BW
	PHY_CHNL_BW_20,
	/// 40MHz BW
	PHY_CHNL_BW_40,
	/// 80MHz BW
	PHY_CHNL_BW_80,
	/// 160MHz BW
	PHY_CHNL_BW_160,
	/// 80+80MHz BW
	PHY_CHNL_BW_80P80,
	/// Reserved BW
	PHY_CHNL_BW_OTHER,
};

/// Channel Flag
enum mac_chan_flags // TODO : change
{
	/// Cannot initiate radiation on this channel
	CHAN_NO_IR = RS_BIT(0),
	/// Channel is not allowed
	CHAN_DISABLED = RS_BIT(1),
	/// Radar detection required on this channel
	CHAN_RADAR = RS_BIT(2),
	/// Can be primary channel of HT40- operating channel
	CHAN_HT40M = RS_BIT(3),
	/// Can be primary channel of HT40+ operating channel
	CHAN_HT40P = RS_BIT(4),
	/// Can be primary channel of VHT80 (1st position 10/70) operating channel
	CHAN_VHT80_10_70 = RS_BIT(5),
	/// Can be primary channel of VHT80 (2nd position 30/50) operating channel
	CHAN_VHT80_30_50 = RS_BIT(6),
	/// Can be primary channel of VHT80 (3rd position 50/30) operating channel
	CHAN_VHT80_50_30 = RS_BIT(7),
	/// Can be primary channel of VHT80 (4th position 70/10) operating channel
	CHAN_VHT80_70_10 = RS_BIT(8)
};

enum machw_version // TODO : change
{
	RS_MACHW_DEFAULT = 10,
	RS_MACHW_HE = 20,
	RS_MACHW_HE_AP = 30,
};

/// Connection flags
enum rs_c_connection_flags
{
	/// Flag indicating whether the control port is controlled by host or not
	CONTROL_PORT_HOST = RS_BIT(0),
	/// Flag indicating whether the control port frame shall be sent unencrypted
	CONTROL_PORT_NO_ENC = RS_BIT(1),
	/// Flag indicating whether HT and VHT shall be disabled or not
	DISABLE_HT = RS_BIT(2),
	/// Flag indicating whether a pairwise key has to be used
	USE_PAIRWISE_KEY = RS_BIT(3),
	/// Flag indicating whether MFP is in use
	MFP_IN_USE = RS_BIT(4),
	/// Flag indicating whether Reassociation should be used instead of Association
	REASSOCIATION = RS_BIT(5),
	/// Flag indicating Connection request if part of FT over DS
	FT_OVER_DS = RS_BIT(6),
	/// Flag indicating whether encryption is used or not on the connection
	USE_PRIVACY = RS_BIT(7),
	/// Flag indicating whether usage of SPP A-MSDUs is required
	REQUIRE_SPP_AMSDU = RS_BIT(8),
};

/**
 * enum rs_sta_auth_status - STATION flags
 *
 * @RS_STA_AUTH_EXT: External authentication is in progress
 */
enum rs_c_sm_auth_status
{
	RS_STA_AUTH_EXT = RS_BIT(0),
	RS_STA_AUTH_FT_OVER_DS = RS_BIT(1),
	RS_STA_AUTH_FT_OVER_AIR = RS_BIT(2),
};

enum rs_c_sta_cap_flags
{
	STA_CAP_QOS = RS_BIT(0),
	STA_CAP_HT = RS_BIT(1),
	STA_CAP_VHT = RS_BIT(2),
	STA_CAP_MFP = RS_BIT(3),
	STA_OP_NOT_IE = RS_BIT(4),
	STA_CAP_HE = RS_BIT(5),
	STA_CAP_SHORT_PREAMBLE = RS_BIT(6),
};

enum rs_tdls_state
{
	RS_TDLS_STATE_LINK_IDLE,
	RS_TDLS_STATE_TX_REQ,
	RS_TDLS_STATE_TX_RSP,
	RS_TDLS_STATE_LINK_ACTIVE,
	RS_TDLS_STATE_MAX
};

enum rs_c_roc_op
{
	RS_ROC_OP_START,
	RS_ROC_OP_STOP
};

struct rs_c_reset_req {
	u64 time;
	u32 bt_coex;
};

struct rs_c_time_sync_req {
	u64 time;
};

struct rs_c_phy_cfg {
	u8 boot_mode;
	u8 band; 
	u8 country_code[4];
};

struct rs_c_dev_start_req {
	struct rs_c_phy_cfg phy_cfg;
	u32 uapsd_timeout;
	u16 lp_clk_accuracy;
	u16 tx_timeout[RS_C_MAX_Q];
	u16 rx_hostbuf_size;
};

struct rs_c_fw_ver_rsp {
	u32 fw_version;

	u32 machw_features;
	u32 machw_version;
	u32 phy_feature;
	u32 phy_version;
	u32 mac_feat;
	u16 sta_max_count;
	u8 vif_max_count;
};

struct rs_c_mac_addr {
	u16 addr[ETH_ADDR_LEN / 2];
};

struct rs_c_add_if_req {
	u8 iftype;
	struct rs_c_mac_addr addr;
	bool p2p;
	bool uf;
};

struct rs_c_add_if_rsp {
	u8 status;
	u8 vif_index;
};

struct rs_c_remove_if_req {
	u8 vif_index;
};

struct rs_c_oper_ch_info {
	u8 ch_band;
	u8 ch_bw;
	u16 freq_prim20;
	u16 freq_cen1;
	u16 freq_cen2;
	u16 ch_flags;
	s8 tx_max_pwr;
};

/// MAC HT capability information element
struct htcapability {
	u16 ht_capa_info;
	u8 a_mpdu_param;
	u8 mcs_rate[RS_C_MAX_MCS_LEN];
	u16 ht_extended_capa;
	u32 tx_beamforming_capa;
	u8 asel_capa;
};

/// MAC VHT capability information element
struct vhtcapability {
	u32 vht_capa_info;
	u16 rx_mcs_map;
	u16 rx_highest;
	u16 tx_mcs_map;
	u16 tx_highest;
};

struct he_mcs_nss_supp {
	u16 rx_mcs_80;
	u16 tx_mcs_80;
	u16 rx_mcs_160;
	u16 tx_mcs_160;
	u16 rx_mcs_80p80;
	u16 tx_mcs_80p80;
};

struct hecapability {
	u8 mac_cap_info[MAC_CAPA_HE_LEN];
	u8 phy_cap_info[PHY_CAPA_HE_LEN];
	struct he_mcs_nss_supp mcs_supp;
	u8 ppe_thres[HE_PPE_THRES_MAX];
};

struct rs_c_me_config_req { // TODO : change
	struct htcapability ht_cap;
	struct vhtcapability vht_cap;
	struct hecapability he_cap;
	u16 tx_lft;
	u8 phy_bw_max;
	bool ht_supp;
	bool vht_supp;
	bool he_supp;
	bool he_ul_on;
	bool ps_on;
	bool ant_div_on;
	bool dpsm;
	s32 amsdu_tx;
};

struct prim_ch_def {
	u16 ch_freq;
	u8 ch_band;
	s8 tx_max_pwr;
	u16 ch_flags;
};

struct rs_c_mac_ssid {
	u8 ssid_len;
	u8 ssid[RS_C_SSID_LEN];
};

struct rs_c_me_chan_config_req {
	struct prim_ch_def chan2G4[RS_C_SCAN_CHANNEL_2G];
	struct prim_ch_def chan5G[RS_C_SCAN_CHANNEL_5G];
	u8 chan2G4_cnt;
	u8 chan5G_cnt;
};

struct rs_c_sc_start_req {
	struct prim_ch_def chan[RS_C_SCAN_CHANNEL_MAX];
	struct rs_c_mac_ssid ssid[RS_C_SCAN_SSID_MAX];
	u8 bssid[RS_C_ETH_ALEN];
	u32 ie_addr;
	u16 ie_len;
	u8 vif_idx;
	u8 n_channels;
	u8 n_ssids;
	bool no_cck;
	u32 duration;
	u8 ie[];
};

struct rs_c_sc_cancel_req {
	u8 vif_idx;
};

struct rs_c_sm_connect_req {
	struct rs_c_mac_ssid ssid;
	struct rs_c_mac_addr bssid;
	struct prim_ch_def chan;
	u32 flags;
	u16 ctrl_port_ethertype;
	u16 listen_interval;
	bool dont_wait_bcmc;
	u8 auth_type;
	u8 uapsd_queues;
	u8 vif_idx;
	u16 ie_len;
	u32 ie_buf[];
};

struct rs_c_sm_connect_rsp {
	u8 status;
};

struct rs_c_change_bcn_req {
	u8 bcn_ptr[512];
	u16 bcn_len;
	u16 tim_oft;
	u8 tim_len;
	u8 vif_id;
	u8 csa_oft[RS_C_BCN_MAX_CSA_CPT];
};

#define SUPP_RATESET_SIZE 12

struct supp_rateset {
	u8 length;
	u8 array[SUPP_RATESET_SIZE];
};

struct rs_c_sta_add_req {
	struct rs_c_mac_addr mac_addr;
	struct supp_rateset rate_set;
	struct htcapability ht_cap;
	struct vhtcapability vht_cap;
	struct hecapability he_cap;
	u32 flags;
	u16 aid;
	u8 uapsd_queues;
	u8 max_sp;
	u8 opmode_notif;
	u8 vif_idx;
	bool tdls_sta;
	bool tdls_sta_initiator;
	bool tdls_chsw_allowed;
};

struct rs_c_sta_del_req {
	u8 sta_idx;
	bool is_tdls_sta;
};

struct rs_c_key_add_rsp {
	u8 status;
	u8 hw_key_index;
};

struct rs_c_sta_add_rsp {
	u8 sta_idx;
	u8 status;
	u8 pm_state;
};

struct rs_c_ap_start_rsp {
	u8 status;
	u8 vif_idx;
	u8 ch_idx;
	u8 bcmc_idx;
};

struct rs_c_cas_start_rsp {
	u8 status;
	u8 ch_idx;
};

struct rs_c_mon_mode_rsp {
	u8 chan_index;
	struct rs_c_oper_ch_info chan;
};

struct rs_c_probe_client_rsp {
	u8 status;
	u32 probe_id;
};

struct rs_c_mem_read_rsp {
	u32 memaddr;
	u32 memdata;
};

struct rs_c_rf_per_rsp {
	u32 pass;
	u32 fcs;
	u32 phy;
	u32 overflow;
};

#ifdef CONFIG_DBG_STATS
struct rs_c_dbg_stats_tx_rsp {
	u8 status;
	u64 t_success_cnt[4]; // cck, ht, vht, he
	u64 t_fail_cnt[4]; // cck, ht, vht, he
	u32 epr[2];
	u8 format_mod;
	union {
		struct {
			u64 success[15];
			u64 fail[15];
		} non_ht;
		struct {
			u64 success[2][8]; // gi(2) * mcs(0~7)
			u64 fail[2][8];
		} ht;
		struct {
			u64 success[2][10]; // gi(2) * mcs(0~9)
			u64 fail[2][10];
		} vht;
		struct {
			u64 success[3][10]; // gi(3) * mcs(0~9)
			u64 fail[3][10];
		} he;
	};
};
#endif

struct rs_c_cmd_rsp {
	struct rs_c_fw_ver_rsp fw_ver;
	struct rs_c_add_if_rsp add_if;
	struct rs_c_sm_connect_rsp connect_rsp;
	struct rs_c_key_add_rsp key_add_rsp;
	struct rs_c_sta_add_rsp sta_add_rsp;
	struct rs_c_ap_start_rsp ap_start_rsp;
	struct rs_c_cas_start_rsp cac_start_rsp;
	struct rs_c_mon_mode_rsp mon_mode_rsp;
	struct rs_c_probe_client_rsp probe_client_rsp;
	struct rs_c_mem_read_rsp mem_read_rsp;
	struct rs_c_rf_per_rsp rf_per_rsp;
#ifdef CONFIG_DBG_STATS
	struct rs_c_dbg_stats_tx_rsp dbg_stats_tx_rsp;
#endif
};

struct rs_c_sc_survey_info {
	u16 freq;
	s8 chan_noise_dbm;
	u32 chan_dwell_ms;
	u32 chan_busy_ms;
};

struct rs_c_survey_info {
	u32 filled;
	u32 chan_dwell_ms;
	u32 chan_busy_ms;
	s8 chan_noise_dbm;
};

struct rs_c_legacy_info {
	u32 format_mod : 4;
	u32 ch_bw : 3;
	u32 pre_type : 1;
	u32 leg_length : 12;
	u32 leg_rate : 4;
} __packed;

struct rs_c_sc_result_ind {
	u16 length;
	u16 framectrl;
	u16 center_freq;
	u8 band;
	u8 sta_idx;
	u8 inst_nbr;
	s8 rssi;
	struct rs_c_legacy_info legacy_info;
	u32 payload[];
};

struct rs_c_sm_connect_ind {
	u16 conn_status;
	struct rs_c_mac_addr bssid;
	bool is_roam;
	u8 vif_idx;
	u8 ap_idx;
	u8 ch_idx;
	bool flag_qos;
	u8 acm_bits;
	u16 assoc_req_ie_len;
	u16 assoc_rsp_ie_len;
	u16 assoc_id;
	struct rs_c_oper_ch_info oper_chan;
	u32 edca_param[RS_AC_MAX];
	u8 assoc_ie_buf[256];
};

struct rs_c_disconnect_req {
	u16 deauth_reason;
	u8 vif_idx;
};

struct rs_c_sm_disconnect_ind {
	u16 reason_code;
	u8 vif_idx;
	bool reassoc;
};

struct rs_c_secury_key_info {
	u8 length;
	u32 array[RS_C_SECURY_KEY_LEN / 4];
};

struct rs_c_key_add_req {
	u8 key_idx;
	u8 sta_id;
	struct rs_c_secury_key_info key;
	u8 cipher_suite;
	u8 vif_id;
	u8 spp;
	bool pairwise;
};

struct rs_c_key_del_req {
	u8 key_index;
};

struct rs_c_port_control_req {
	u8 sta_idx;
	bool port_control_state;
};

struct rs_c_roc_req {
	u8 op_code;
	u8 vif_index;
	struct rs_c_oper_ch_info chan;
	u32 duration_ms;
};

struct rs_c_beacon__info {
	u8 *head;
	u8 *tail;
	u8 *ies;
	size_t head_len;
	size_t tail_len;
	size_t ies_len;
	size_t tim_len;
	size_t len;
	u8 dtim;
};

struct rs_c_ap_start_req {
	struct supp_rateset basic_rates;
	struct rs_c_oper_ch_info chan;
	u16 tim_oft;
	u16 bcn_int;
	u32 flags;
	u16 ctrl_port_ethertype;
	u8 tim_len;
	u8 vif_idx;
	u8 beacon[512];
	u16 bcn_len;
};

struct rs_c_ap_stop_req {
	u8 vif_idx;
};

struct rs_c_cac_start_req {
	struct rs_c_oper_ch_info chan;
	u8 vif_idx;
};

struct rs_c_cac_stop_req {
	u8 vif_idx;
};

struct rs_c_mon_mode_req {
	struct rs_c_oper_ch_info chan;
	bool ch_valid;
	bool uf_enable;
};

struct rs_c_probe_client_req {
	u8 vif_idx;
	u8 sta_idx;
};

struct rs_net_ap_isolate_req {
	u8 ap_isolate;
};

struct rs_c_set_tx_power_req {
	u8 vif_idx;
	s8 tx_power;
};

struct rs_c_edca_req {
	u32 edca_param;
	bool uapsd_enabled;
	u8 ac;
	u8 vif_idx;
};

struct rs_c_set_power_mgmt_req {
	u8 ps_mode;
};

struct rs_c_cqm_rssi_config_req {
	u8 vif_idx;
	s8 rssi_thold;
	u8 rssi_hyst;
};

struct rs_c_me_mic_failure_ind {
	struct rs_c_mac_addr mac_addr;
	u64 tsc;
	bool key_type; // group (true) or unicast (false) ?
	u8 keyid;
	u8 vif_idx;
};

struct rs_c_rssi_status_ind {
	u8 vif_index;
	bool rssi_status;
	s8 rssi;
};

struct rs_c_dbg_mem_read_req {
	u32 mem_addr;
};

struct rs_c_dbg_mem_write_req {
	u32 mem_addr;
	u32 mem_value;
};

struct rs_c_dbg_mode_filter_req {
	u32 mode_filter;
};

struct rs_c_dbg_level_filter_req {
	u32 level_filter;
};

struct rs_c_rf_tx_req {
	u64 bssid;
	u64 destAddr;
	u16 frequency;
	u16 numFrames;
	u16 frameLen;
	u8 start;
	u32 txRate;
	u32 txPower;
	u8 gi;
	u8 greenField;
	u8 preambleType;
	u8 qosEnable;
	u8 ackPolicy;
	u8 aifsnVal;
};

struct rs_c_rf_cw_req {
	u8 start;
	u32 txPower;
	u16 frequency;
};

struct rs_c_rf_cont_req {
	u16 frequency;
	u8 start;
	u32 txRate;
	u32 txPower;
};

struct rs_c_rf_ch_req {
	u16 frequency;
};

struct rs_c_rf_per_req {
	u8 start;
};

struct rs_c_sm_ext_auth_req_ind {
	u8 vif_idx;
	struct rs_c_mac_ssid ssid;
	struct rs_c_mac_addr bssid;
	u32 key_mgmt_suite;
};

struct rs_c_sm_ext_auth_req_rsp {
	u8 vif_idx;
	u16 status;
};

struct rs_c_channel_switch_ind {
	u8 chan_index;
	bool roc_req;
	u8 vif_index;
	bool roc_tdls;
	u32 duration_us;
	u16 freq;
};

struct rs_c_channel_pre_switch_ind {
	u8 chan_index;
};

struct rs_c_sm_ft_auth_ind {
	u8 vif_idx;
	u16 ft_ie_len;
	u32 ft_ie_buf[0];
};

struct rs_c_ap_probe_client_ind {
	u8 vif_idx;
	u8 sta_idx;
	bool client_present;
	u32 probe_id;
};

struct rs_c_ps_change_ind {
	u8 sta_idx;
	u8 ps_state;
};

struct rs_c_traffic_req_ind {
	u8 sta_idx;
	u8 pkt_cnt;
	bool uapsd;
};

struct rs_c_pktloss_ind {
	u8 vif_index;
	struct rs_c_mac_addr mac_addr;
	u32 num_packets;
};

struct rs_c_csa_counter_ind {
	u8 vif_index;
	u8 csa_count;
};

struct rs_c_csa_finish_ind {
	u8 vif_index;
	u8 status;
	u8 chan_idx;
};

struct rs_c_csa_traffic_ind {
	u8 vif_index;
	bool enable;
};

struct rs_c_p2p_ps_cahnge_ind {
	u8 vif_index;
	u8 ps_on;
};

struct rs_c_p2p_noa_update_ind {
	u8 vif_index;
	u8 noa_ins_idx;
	u8 noa_type;
	u8 count;
	u32 duration_us;
	u32 interval_us;
	u32 start_time;
};

#ifdef CONFIG_DBG_STATS
struct rs_c_dbg_stats_tx_req {
	u8 req_type;
};
#endif

#ifdef CONFIG_RS_5G_DFS
struct rs_c_dfs_array_ind {
	u32 pulse[4];
	u32 idx;
	u32 cnt;
};
#endif

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL VARIABLE

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL FUNCTION

#endif /* RS_C_CMD_H */
