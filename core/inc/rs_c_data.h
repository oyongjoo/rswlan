// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Copyright (C) [2022-2025] Renesas Electronics Corporation and/or its
 * affiliates.
 */

#ifndef RS_C_DATA_H
#define RS_C_DATA_H

////////////////////////////////////////////////////////////////////////////////
/// INCLUDE

#include "rs_type.h"

////////////////////////////////////////////////////////////////////////////////
/// MACRO DEFINITION

#define RS_C_CTRL_DATA_LEN		      (980)
#define RS_C_INDI_DATA_LEN		      (980)

// Max 1700 bytes
#define RS_C_DATA_SIZE			      (1700)

#define RS_C_BASE_HDR_SIZE		      (sizeof(u8) + sizeof(u8) + sizeof(u16))
#define RS_C_GET_DATA_SIZE(ext_len, data_len) (RS_C_BASE_HDR_SIZE + (ext_len) + (data_len))

#define RS_C_CTRL_REQ_EXT_LEN		      (0)
#define RS_C_CTRL_RSP_EXT_LEN		      (sizeof(struct rs_c_ctrl_rsp_ext_hdr))
#define RS_C_INDI_EXT_LEN		      (sizeof(struct rs_c_indi_ext_hdr))

#define RS_C_RX_EXT_LEN			      (sizeof(struct rs_c_rx_ext_hdr))
#define RS_C_RX_STATUS_EXT_LEN		      (sizeof(struct rs_c_rx_status_ext_hdr))
#define RS_C_TX_EXT_LEN			      (sizeof(struct rs_c_tx_ext_hdr))

////////////////////////////////////////////////////////////////////////////////
/// TYPE DEFINITION

// common if data header
struct rs_c_data {
	u8 cmd;
	u8 ext_len; // length of extra information (bytes, max 255 bytes)
	u16 data_len; // length of data (bytes)

	// data
	u8 data[RS_C_DATA_SIZE];
};

struct rs_c_ctrl_req {
	u8 cmd;
	u8 reserved;
	u16 data_len; // length of data (bytes)

	u8 data[RS_C_CTRL_DATA_LEN];
};

// Command response buffer
struct rs_c_ctrl_rsp_ext_hdr {
	u32 status : 32;
};

struct rs_c_ctrl_rsp {
	u8 cmd;
	u8 ext_len;
	u16 data_len; // length of data (bytes)

	// extra information
	struct rs_c_ctrl_rsp_ext_hdr ext_hdr;

	u8 data[RS_C_CTRL_DATA_LEN];
};

struct rs_c_indi_ext_hdr {
	u32 status : 32;
};

struct rs_c_indi {
	u8 cmd;
	u8 ext_len;
	u16 data_len; // length of data (bytes)

	// extra information
	struct rs_c_indi_ext_hdr ext_hdr;

	u8 data[RS_C_INDI_DATA_LEN];
};

enum rs_c_rx_ext_hdr_flags
{
	RS_C_EXT_FLAGS_MGMT = RS_BIT(1),
	RS_C_EXT_FLAGS_MGMT_NO_CCK = RS_BIT(2),
	RS_C_EXT_FLAGS_AMSDU = RS_BIT(3),
	RS_C_EXT_FLAGS_MGMT_ROBUST = RS_BIT(4),
	RS_C_EXT_FLAGS_4ADDR = RS_BIT(5),
	RS_C_EXT_FLAGS_EOSP = RS_BIT(6),
	RS_C_EXT_FLAGS_MESH_FWD = RS_BIT(7),
	RS_C_EXT_FLAGS_TDLS = RS_BIT(8),
	RS_C_EXT_FLAGS_SN = RS_BIT(9)
};

struct rs_c_rx_ext_ht {
	u16 short_gi : 1;
	u16 mcs : 7;
	u16 num_extn_ss : 2;
	u16 reserced : 6;
} __packed;

struct rs_c_rx_ext_vht {
	u16 short_gi : 1;
	u16 mcs : 7;
	u16 nss : 3;
	u16 reserved : 5;
} __packed;

struct rs_c_rx_ext_he {
	u16 ru_size : 3;
	u16 mcs : 7;
	u16 nss : 3;
	u16 gi_type : 2;
	u16 dcm : 1;
	u16 reserved : 3;
} __packed;

struct rs_c_rx_ext_hdr {
	u32 status : 32;
	u32 amsdu : 1;
	u32 mpdu : 1;
	u32 addr_4 : 1;
	u32 mesh_peer_new : 1;
	u32 priority : 3;
	u32 vif_idx : 8;
	u32 sta_idx : 8;
	u32 dest_sta_idx : 8;
	u32 ch_bw : 2;
	u32 rssi1 : 8;
	u32 band : 4;
	u32 channel_type : 8;
	u32 prim20_freq : 16;
	u32 center1_freq : 16;
	u32 center2_freq : 16;
	u32 format_mod : 4;
	u32 pre_type : 1;
	u32 leg_rate : 4;

	union {
		struct rs_c_rx_ext_ht ht;
		struct rs_c_rx_ext_vht vht;
		struct rs_c_rx_ext_he he;
	};
};

// RX(FW -> HOST) Header
struct rs_c_rx_data {
	u8 cmd;
	u8 ext_len; // length of extra information (bytes, max 255 bytes)
	u16 data_len; // length of data (bytes)

	// extra information
	struct rs_c_rx_ext_hdr ext_hdr;

	// data
	u8 data[RS_C_DATA_SIZE];
};

struct rs_c_tx_ext_hdr {
	u32 buf_idx : 3;
	u32 tid : 5;
	u32 vif_idx : 8;
	u32 sta_idx : 8;
	u32 flags : 10;
	u32 sn : 24;
};

struct rs_c_rx_status_ext_hdr {
	u32 status : 32;
};

// RX Status (FW -> HOST) Header
struct rs_c_rx_status {
	// common if header
	u8 cmd;
	u8 ext_len; // extra information length (bytes, max 255 bytes)
	u16 data_len;

	// extra information
	struct rs_c_rx_status_ext_hdr ext_hdr;
};

// TX(FW <- HOST) Header
struct rs_c_tx_data {
	// common if header
	u8 cmd;
	u8 ext_len; // extra information length (bytes, max 255 bytes)
	u16 data_len;

	// extra information
	struct rs_c_tx_ext_hdr ext_hdr;

	// 802.11 MAC Frame or MSDU frame : IEEE 802.3 with SNAP Header
	u8 data[RS_C_DATA_SIZE];
};

/* conversion table for legacy rate */
struct rs_c_legrate {
	s16 idx;
	u16 rate; // in 100Kbps
};

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL VARIABLE

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL FUNCTION

#endif /* RS_C_DATA_H */
