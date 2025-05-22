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
#include "rs_c_cmd.h"
#include "rs_c_data.h"

#include "rs_net_skb.h"

////////////////////////////////////////////////////////////////////////////////
/// MACRO DEFINITION

////////////////////////////////////////////////////////////////////////////////
/// TYPE DEFINITION

////////////////////////////////////////////////////////////////////////////////
/// LOCAL VARIABLE

////////////////////////////////////////////////////////////////////////////////
/// LOCAL FUNCTION

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL FUNCTION

u8 *rs_net_skb_alloc(u32 len)
{
	struct sk_buff *temp_skb = NULL;

	temp_skb = dev_alloc_skb(len);
	if (temp_skb) {
		(void)skb_put(temp_skb, len);
		temp_skb->priority = RS_AC_BK;
	}

	return (u8 *)temp_skb;
}

void rs_net_skb_free(u8 *skb)
{
	struct sk_buff *temp_skb = (struct sk_buff *)skb;

	if (temp_skb) {
		dev_kfree_skb_any(temp_skb);
	}
}

s32 rs_net_skb_get_data(u8 *skb, u8 **out_data)
{
	s32 len = RS_FAIL;
	struct sk_buff *temp_skb = (struct sk_buff *)skb;
	u8 *temp_data = NULL;

	if (temp_skb) {
		if (out_data) {
			temp_data = temp_skb->data;
		}
		len = temp_skb->len;
	}

	if (out_data) {
		*out_data = temp_data;
	}

	return len;
}

u8 *rs_net_skb_add_tx_hdr(u8 *skb, u8 hdr_size)
{
	rs_ret ret = RS_SUCCESS;
	struct sk_buff *temp_skb = (struct sk_buff *)skb;
	u8 *temp_data = NULL;

	if (temp_skb) {
		if (skb_headroom(temp_skb) < hdr_size) {
			if (pskb_expand_head(temp_skb, hdr_size, 0, GFP_ATOMIC) != 0) {
				ret = RS_FAIL;
				RS_ERR("SKB resize failed[%d][%d]\n", skb_headroom(temp_skb), hdr_size);
			}
		}

		if (ret == RS_SUCCESS) {
			temp_data = skb_push(temp_skb, hdr_size);
		} else {
			RS_ERR("SKB resize failed!!\n");
		}
	}

	return temp_data;
}
