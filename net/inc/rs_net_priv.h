// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Copyright (C) [2022-2025] Renesas Electronics Corporation and/or its
 * affiliates.
 */

#ifndef RS_NET_PRIV_H
#define RS_NET_PRIV_H

////////////////////////////////////////////////////////////////////////////////
/// INCLUDE

#include "rs_type.h"
#include "rs_c_cmd.h"
#include "rs_c_data.h"

////////////////////////////////////////////////////////////////////////////////
/// MACRO DEFINITION

#define RS_DEFAULT_MAC_PATH	 "default_mac.ini"

#define RS_NET_KEYS_MAX		 (6)

#define RS_NET_MAX_EXT_CAPA	 (10)

#define RS_NET_INVALID_IFTYPE	 (0xFF)
#define RS_NET_WDEV_IF_TYPE(vif) (vif->wdev.iftype)

#define RS_NET_MU_GROUP_MAX	 (1)
#define RS_NET_CHANINFO_MAX	 (3)

// maximum number of TX frame per RoC
#define RS_NET_ROC_TX		 5

#define RS_NET_STA_MAX		 (4)
#define RS_NET_VIF_DEV_MAX	 (2)

// Maximum number of AP_VLAN interfaces allowed.
// At max we can have one AP_VLAN per station, but we also limit the
// maximum number of interface to 16 (to fit in avail_idx_map)
#define RS_MAX_AP_VLAN_IF \
	(((16 - RS_NET_VIF_DEV_MAX) > RS_NET_STA_MAX) ? RS_NET_STA_MAX : (16 - RS_NET_VIF_DEV_MAX))

// Maximum number of interface at driver level
#define RS_NET_VIF_MAX		  (RS_NET_VIF_DEV_MAX + RS_MAX_AP_VLAN_IF)
#define RS_NET_PRIV_STA_TABLE_MAX (RS_NET_STA_MAX + RS_NET_VIF_DEV_MAX)
#define RS_NET_PRIV_VIF_TABLE_MAX (RS_NET_VIF_MAX)
#define RS_NET_NDEV_TXQ		  (RS_NET_PRIV_STA_TABLE_MAX + 1)
#define RS_NET_NDEV_INVALID_TXQ	  (RS_NET_NDEV_TXQ - 1)

#define RS_NET_LPCA_PPM		  (20) // Low Power Clock accuracy
#define RS_NET_UAPSD_TIMEOUT	  (300) // UAPSD Timer timeout, in ms (Default: 300). If 0, UAPSD is disabled

#define RS_NET_WPI_SUBKEY_LEN	  (16)
#define RS_NET_WPI_KEY_LEN	  (32)

#define RS_NET_DFS_PULSE_MAX	  (32)
#define RS_NET_DFS_DETECTED_MAX	  (4)

#define RS_NET_CH_NOT_SET	  (0xFF)
#define RS_NET_INVALID_VIF_IDX	  (0xFF)
#define RS_NET_INVALID_STA_IDX	  (0xFF)
#define RS_NET_INVALID_TID	  (0x1F)

#define RS_MCS_MAX_HE		  (12)
#define RS_MCS_MAX_VHT		  (9)
#define RS_MCS_MAX_HT		  (8)
#define RS_GI_MAX_HE		  (3)
#define RS_GI_MAX_VHT		  (2)
#define RS_GI_MAX_HT		  (2)

////////////////////////////////////////////////////////////////////////////////
/// TYPE DEFINITION

/// Interface types
enum rs_vif_type
{
	IF_STA = 0, /// ESS STA
	IF_IBSS = 1, /// IBSS STA
	IF_AP = 2, /// AP
	IF_MESH_POINT = 3, /// Mesh Point
	IF_MONITOR = 4, /// Monitor

	VIF_MAX = 0xFF
};

enum rs_net_support_type
{
	RS_MACHW_SUPPORT_DEFAULT = 10,
	RS_MACHW_SUPPORT_HE = 20,
	RS_MACHW_SUPPORT_AP_HE = 30,
};

enum rs_net_chipher_type
{
	RS_CIPHER_WEP40 = 0,
	RS_CIPHER_TKIP = 1,
	RS_CIPHER_CCMP = 2,
	RS_CIPHER_WEP104 = 3,
	RS_CIPHER_SMS4 = 4,
	RS_CIPHER_AES_CMAC_128 = 5,
	RS_CIPHER_GCMP_128 = 6,
	RS_CIPHER_GCMP_256 = 7,
	RS_CIPHER_CCMP_256 = 8,
	RS_CIPHER_BIP_GMAC_128 = 9,
	RS_CIPHER_BIP_GMAC_256 = 10,
	RS_CIPHER_BIP_CMAC_256 = 11,
	RS_CIPHER_INVALID = 0xFF,
};

enum rs_net_dfs_type
{
	RS_DFS_RIU = 0,
	RS_DFS_FCU,
	RS_DFS_CHAIN_LAST
};

enum rs_net_dfs_waveform_type
{
	DFS_WAVEFORM_SHORT,
	DFS_WAVEFORM_WEATHER,
	DFS_WAVEFORM_INTERLEAVED,
	DFS_WAVEFORM_LONG
};

enum rs_net_ps_mode
{
	RS_NET_PS_MODE_OFF = 0,
	RS_NET_PS_MODE_ON = 1,
	RS_NET_PS_MODE_ON_DYN = 2
};

enum rs_net_ap_flags
{
	RS_NET_AP_ISOLATE = RS_BIT(0),
	RS_NET_AP_USER_MESH_PM = RS_BIT(1),
	RS_NET_AP_CREATE_MESH_PATH = RS_BIT(2),
};

enum rs_net_format_mode
{
	FORMATMOD_NON_HT = 0,
	FORMATMOD_NON_HT_DUP_OFDM,
	FORMATMOD_HT_MF,
	FORMATMOD_HT_GF,
	FORMATMOD_VHT,
	FORMATMOD_HE_SU,
	FORMATMOD_HE_MU,
	FORMATMOD_HE_ER,
	FORMATMOD_HE_TB,
};

//////
// TWT
/// TWT Flow configuration
struct twt_conf_tag { // TODO : Change
	/// Flow Type (0: Announced, 1: Unannounced)
	u8 flow_type;
	/// Wake interval Exponent
	u8 wake_int_exp;
	/// Unit of measurement of TWT Minimum Wake Duration (0:256us, 1:tu)
	bool wake_dur_unit;
	/// Nominal Minimum TWT Wake Duration
	u8 min_twt_wake_dur;
	/// TWT Wake Interval Mantissa
	u16 wake_int_mantissa;
};

/// TWT Setup indication message structure
struct twt_setup_ind { // TODO : Change
	/// Response type
	u8 resp_type;
	/// STA Index
	u8 sta_idx;
	/// TWT Setup configuration
	struct twt_conf_tag conf;
};

//////////
// AP mode
struct rs_net_beacon_info { // TODO : Change
	u8 *head;
	u8 *tail;
	u8 *ies;
	size_t head_len;
	size_t tail_len;
	size_t ies_len;
	size_t tim_len;
	size_t len;
	u8 dtim_period;
};

struct rs_net_csa_info {
	struct rs_net_vif_priv *vif;
	struct rs_net_beacon_info bcn;
	u8 buf[512];
	struct cfg80211_chan_def chandef;
	int count;
	int status;
	int ch_idx;
	struct work_struct work;
};

////////////////////
// Remain On Channel
struct rs_net_remain_on_channel { // TODO : Change
	struct rs_net_vif_priv *vif;
	struct ieee80211_channel *chan;
	unsigned int duration;
	bool internal;
	bool on_chan;
	s32 tx_cnt;
	u64 tx_cookie[RS_NET_ROC_TX];
};

//////////
// Station
struct rs_net_key_info { // TODO : Change
	u8 key_index;
};

struct rs_net_tdls { // TODO : Change
	bool active;
	bool initiator;
	bool chsw_en;
	u8 last_tid;
	u16 last_sn;
	bool ps_on;
	bool chsw_allowed;
};

struct rs_net_sta_stats {
	u32 last_acttive_time;
	struct rs_c_rx_ext_hdr last_rx_data_ext;
	struct rs_c_rx_ext_hdr last_stats;
};

struct rs_net_chan_info {
	struct cfg80211_chan_def chan_def;
	u8 count;
};

struct rs_net_sta_priv { // TODO : Change
	struct list_head list;
	bool valid;
	u8 mac_addr[ETH_ADDR_LEN];
	u16 aid;
	u8 sta_idx;
	u8 vif_idx;
	u8 vlan_idx;
	enum nl80211_band band;
	enum nl80211_chan_width width;
	u16 center_freq;
	u32 center_freq1;
	u32 center_freq2;
	u8 ch_idx;
	bool qos;
	u8 acm;
	u16 uapsd_tids;
	struct rs_net_key_info key;
	bool ht;
	bool vht;
	bool he;
	u32 ac_param[RS_AC_MAX];
	struct rs_net_tdls tdls;
	struct rs_net_sta_stats stats;
	enum nl80211_mesh_power_mode mesh_pm;
	int listen_interval;
	struct twt_setup_ind twt_ind;
	struct list_head he_mu;
};

//////////////
// VIF private
struct rs_net_vif_priv {
	struct list_head list;
	struct ieee80211_vif *vif;
	u8 vif_index;

	struct wireless_dev wdev;
	struct net_device *ndev;

	void *net_priv;

	// tdls
	bool roc_tdls;
	u8 tdls_status;
	bool tdls_chsw_prohibited;

	///////////
	// Fullmac

	u8 net_tx_stopped;

	struct rs_net_key_info key[RS_NET_KEYS_MAX];
	u8 drv_vif_index;
	u8 ch_index;
	bool up;
	bool use_4addr;
	bool is_resending; // multicast frame is resending
	s32 generation; // station generated by this vif
	union {
		struct {
			u32 flags;
			struct rs_net_sta_priv *ap;
			struct rs_net_sta_priv *tdls_sta;
			u8 *ft_assoc_ies;
			s32 ft_assoc_ies_len;
			u8 ft_target_ap[ETH_ADDR_LEN];
		} sta;
		struct {
			u32 flags; // TODO
			u8 ap_isolate;
			struct list_head sta_list;
			struct rs_net_beacon_info bcn;
			s32 bcn_interval;
			u8 bcmc_index;
			struct rs_net_csa_info *csa;

			struct list_head mpath_list;
			struct list_head proxy_list;
			enum nl80211_mesh_power_mode mesh_pm;
			enum nl80211_mesh_power_mode next_mesh_pm;
		} ap;
		struct {
			struct rs_net_vif_priv *master;
			struct rs_net_sta_priv *sta_4a;
		} ap_vlan;
	};

	///////////
};

/**
 * struct rs_net_dfs_specs - detector specs for a radar pattern type
 * @type_id: pattern type, as defined by regulatory
 * @width_min: minimum radar pulse width in [us]
 * @width_max: maximum radar pulse width in [us]
 * @pri_min: minimum pulse repetition interval in [us] (including tolerance)
 * @pri_max: minimum pri in [us] (including tolerance)
 * @num_pri: maximum number of different pri for this type
 * @ppb: pulses per bursts for this type
 * @ppb_thresh: number of pulses required to trigger detection
 * @max_pri_tolerance: pulse time stamp tolerance on both sides [us]
//  * @type: Type of radar waveform
 * @chirp: chirp required for the radar pattern
 */
struct rs_net_dfs_specs {
	u8 type_id;
	u8 width_min;
	u8 width_max;
	u16 pri_min;
	u16 pri_max;
	u8 num_pri;
	u8 ppb;
	u8 ppb_thresh;
	u8 max_pri_tolerance;
	enum rs_net_dfs_waveform_type waveform;
};

struct rs_net_dfs_pulse {
	/* Last radar pulses received */
	int index;
	int count;
	u32 buffer[RS_NET_DFS_PULSE_MAX];
};

struct rs_net_dfs_detected {
	u16 index;
	u16 count;
	s64 time[RS_NET_DFS_DETECTED_MAX];
	s16 freq[RS_NET_DFS_DETECTED_MAX];
};

/**
 * struct rs_net_dfs_pool_stats - DFS Statistics for global pools
 */
struct rs_net_dfs_pool_stats {
	u32 pool_reference;
	u32 pulse_allocated;
	u32 pulse_alloc_error;
	u32 pulse_used;
	u32 pseq_allocated;
	u32 pseq_alloc_error;
	u32 pseq_used;
};

/**
 * struct pulse_event - describing pulses reported by PHY
 * @ts: pulse time stamp in us
 * @freq: channel frequency in MHz
 * @width: pulse duration in us
 * @rssi: rssi of radar event
 * @chirp: chirp detected in pulse
 */
struct rs_net_dfs_pulse_event {
	u64 ts;
	u16 freq;
	u8 width;
	u8 rssi;
	bool chirp;
};

/**
 * struct rs_net_dpd - DFS pattern detector
 * @exit(): destructor
 * @set_dfs_domain(): set DFS domain, resets detector lines upon domain changes
 * @add_pulse(): add radar pulse to detector, returns true on detection
 * @region: active DFS region, NL80211_DFS_UNSET until set
 * @num_radar_types: number of different radar types
 * @last_pulse_ts: time stamp of last valid pulse in usecs
 * @radar_detector_specs: array of radar detection specs
 * @ch_detect: list connecting channel_detector elements
 */
struct rs_net_dpd {
	u8 enabled;
	enum nl80211_dfs_regions region;
	u8 num_dfs_types;
	u64 last_pulse_ts;
	u32 prev_jiffies;
	const struct rs_net_dfs_specs *dfs_spec;
	struct list_head ch_detect[];
};

struct rs_net_dfs_priv {
	struct rs_net_dfs_pulse pulses[RS_DFS_CHAIN_LAST];
	struct rs_net_dpd *dpd[RS_DFS_CHAIN_LAST];
	struct rs_net_dfs_detected detected[RS_DFS_CHAIN_LAST];
	struct work_struct dfs_work; /* Work used to process radar pulses */
	spinlock_t lock; /* lock for pulses processing */

	/* In softmac cac is handled by mac80211 */
#ifdef CONFIG_RS_FULLMAC
	struct delayed_work cac_work; /* Work used to handle CAC */
	struct rs_net_vif_priv *cac_vif; /* vif on which we started CAC */
#endif
};

/**
 * struct rs_net_pri_sequence - sequence of pulses matching one PRI
 * @head: list_head
 * @pri: pulse repetition interval (PRI) in usecs
 * @dur: duration of sequence in usecs
 * @count: number of pulses in this sequence
 * @count_falses: number of not matching pulses in this sequence
 * @first_ts: time stamp of first pulse in usecs
 * @last_ts: time stamp of last pulse in usecs
 * @deadline_ts: deadline when this sequence becomes invalid (first_ts + dur)
 * @ppb_thresh: Number of pulses to validate detection
 *              (need for weather radar whose value depends of pri)
 */
struct rs_net_pri_sequence {
	struct list_head head;
	u32 pri;
	u32 dur;
	u32 count;
	u32 count_falses;
	u64 first_ts;
	u64 last_ts;
	u64 deadline_ts;
	u8 ppb_thresh;
};

/**
 * struct rs_net_pri_detector - PRI detector element for a dedicated radar type
 * @exit(): destructor
 * @add_pulse(): add pulse event, returns pri_sequence if pattern was detected
 * @reset(): clear states and reset to given time stamp
 * @rs: detector specs for this detector element
 * @last_ts: last pulse time stamp considered for this element in usecs
 * @sequences: list_head holding potential pulse sequences
 * @pulses: list connecting pulse_elem objects
 * @count: number of pulses in queue
 * @max_count: maximum number of pulses to be queued
 * @window_size: window size back from newest pulse time stamp in usecs
 */
struct rs_net_pri_detector {
	struct list_head head;
	const struct rs_net_dfs_specs *rs;

	/* private: internal use only */
	u64 last_ts;
	struct list_head sequences;
	struct list_head pulses;
	u32 count;
	u32 max_count;
	u32 window_size;
	struct rs_net_pri_detector_ops *ops;
	u16 freq;
};

struct rs_net_pri_detector_ops {
	void (*init)(struct rs_net_pri_detector *pde);
	struct rs_net_pri_sequence *(*add_pulse)(struct rs_net_pri_detector *pde, u16 len, u64 ts, u16 pri);
	int reset_on_pri_overflow;
};
#ifdef CONFIG_DBG_STATS
struct stats_non_ht {
	u8 bw;
	u64 ofdm;
	u64 cck;
};

struct rs_net_rx_stats {
	u8 format_mode;
	u8 flag;
	struct stats_non_ht non_ht;
	struct {
		u8 bw;
		u8 mcs;
		u8 gi;
		u8 nss;
		u64 ht[RS_GI_MAX_HT][RS_MCS_MAX_HT];
	} ht;
	struct {
		u8 bw;
		u8 mcs;
		u8 gi;
		u8 nss;
		u64 vht[RS_GI_MAX_VHT][RS_MCS_MAX_VHT];
	} vht;
	struct {
		u8 bw;
		u8 mcs;
		u8 gi;
		u8 nss;
		u64 he[RS_GI_MAX_HE][RS_MCS_MAX_HE];
	} he_su;
};

struct rs_net_dbg_stats {
	struct rs_net_rx_stats *rx_stats;
	bool dbg_stats_enabled;
};
#endif

// rswlan private data in wihpy
struct rs_net_cfg80211_priv {
	struct device *dev_if;
	struct rs_core *core;

	// Global wifi config
	struct wiphy *wiphy;

	u8 ext_capa[RS_NET_MAX_EXT_CAPA];
	unsigned long flags;
	struct ieee80211_vif *_vifs[4];
	struct list_head vifs;
	struct list_head chan_ctxts;
	enum nl80211_band cur_band;
	u16 cur_freq;
	struct rs_c_phy_cfg phy_config;

	s32 machw_version;

	struct rs_net_sta_priv sta_table[RS_NET_PRIV_STA_TABLE_MAX];

	// Virtual I/F
	struct rs_net_vif_priv *vif_table[RS_NET_PRIV_VIF_TABLE_MAX];
	u16 avail_idx_map;
	s32 vif_started;
	s8 mon_vif_index;

	// Channels
	struct rs_net_chan_info chaninfo_table[RS_NET_CHANINFO_MAX];
	u8 chaninfo_index;
	struct rs_net_remain_on_channel *roc;
	struct cfg80211_scan_request *scan_request;
	struct rs_net_dfs_priv dfs;

	struct rs_c_survey_info survey_table[RS_C_SCAN_CHANNEL_MAX];

	struct rs_c_cmd_rsp cmd_rsp;

	struct delayed_work wq_ts;
#ifdef CONFIG_DBG_STATS
	struct rs_net_dbg_stats dbg_stats;
#endif
};

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL VARIABLE

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL FUNCTION

// Get c_if from net_priv
struct rs_c_if *rs_net_priv_get_c_if(struct rs_net_cfg80211_priv *net_priv);

// Set wiphy to net_priv
rs_ret rs_net_priv_set_wiphy(struct rs_net_cfg80211_priv *net_priv, struct wiphy *wiphy);

// Get wiphy from net_priv
struct wiphy *rs_net_priv_get_wiphy(struct rs_net_cfg80211_priv *net_priv);

// Get net_device from vif_priv
struct net_device *rs_vif_priv_get_ndev(struct rs_net_vif_priv *vif_priv);

// Set net_device to vif_priv
rs_ret rs_vif_priv_set_ndev(struct rs_net_vif_priv *vif_priv, struct net_device *ndev);

// Get wdev from vif_priv
struct wireless_dev *rs_vif_priv_get_wdev(struct rs_net_vif_priv *vif_priv);

// Get sta_info from net_priv
struct rs_net_sta_priv *rs_net_priv_get_sta_info(struct rs_net_cfg80211_priv *net_priv, s16 sta_index);

// Get sta_info from mac_address
struct rs_net_sta_priv *rs_net_priv_get_sta_info_from_mac(struct rs_net_cfg80211_priv *net_priv,
							  const u8 *mac_addr);
// Set vif_priv to net_priv
rs_ret rs_net_priv_set_vif_priv(struct rs_net_cfg80211_priv *net_priv, s16 vif_index,
				struct rs_net_vif_priv *vif_priv);

// Get vif_priv from net_priv
struct rs_net_vif_priv *rs_net_priv_get_vif_priv(struct rs_net_cfg80211_priv *net_priv, s16 vif_index);

// Set net_priv to vif_priv
rs_ret rs_vif_priv_set_net_priv(struct rs_net_vif_priv *vif_priv, struct rs_net_cfg80211_priv *net_priv);

// Get net_priv from vif_priv
struct rs_net_cfg80211_priv *rs_vif_priv_get_net_priv(struct rs_net_vif_priv *vif_priv);

// Get H/W version
s32 rs_net_get_hw_ver(u32 ver);

#endif /* RS_NET_PRIV_H */
