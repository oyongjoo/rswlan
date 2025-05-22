// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Copyright (C) [2022-2025] Renesas Electronics Corporation and/or its
 * affiliates.
 */

#ifdef CONFIG_DEBUG_FS

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
#include "rs_c_dbg.h"

#include "rs_net_cfg80211.h"
#include "rs_net_priv.h"
#include "rs_net_dev.h"

#include "rs_net_ctrl.h"

////////////////////////////////////////////////////////////////////////////////
/// MACRO DEFINITION

#define RS_DBGFS_CR_FILE(name, parent, mode)                                                \
	do {                                                                                \
		debugfs_create_file(#name, mode, parent, net_priv, &rs_dbgfs_##name##_ops); \
	} while (0)

#define RS_DBGFS_CR_U32(name, parent, ptr, mode)              \
	do {                                                  \
		debugfs_create_u32(#name, mode, parent, ptr); \
	} while (0)

#define RS_DBGFS_RD(name)                                                                             \
	static ssize_t rs_dbgfs_##name##_read(struct file *file, char __user *user_buf, size_t count, \
					      loff_t *ppos);

#define RS_DBGFS_WR(name)                                                                                    \
	static ssize_t rs_dbgfs_##name##_write(struct file *file, const char __user *user_buf, size_t count, \
					       loff_t *ppos);

#define RS_DBGFS_OPS_RD(name)                                         \
	RS_DBGFS_RD(name);                                            \
	static const struct file_operations rs_dbgfs_##name##_ops = { \
		.read = rs_dbgfs_##name##_read,                       \
		.open = simple_open,                                  \
		.llseek = generic_file_llseek,                        \
	};

#define RS_DBGFS_OPS_WR(name)                                         \
	RS_DBGFS_WR(name);                                            \
	static const struct file_operations rs_dbgfs_##name##_ops = { \
		.write = rs_dbgfs_##name##_write,                     \
		.open = simple_open,                                  \
		.llseek = generic_file_llseek,                        \
	};

#define RS_DBGFS_OPS_RW(name)                                         \
	RS_DBGFS_RD(name);                                            \
	RS_DBGFS_WR(name);                                            \
	static const struct file_operations rs_dbgfs_##name##_ops = { \
		.write = rs_dbgfs_##name##_write,                     \
		.read = rs_dbgfs_##name##_read,                       \
		.open = simple_open,                                  \
		.llseek = generic_file_llseek,                        \
	};

////////////////////////////////////////////////////////////////////////////////
/// TYPE DEFINITION

////////////////////////////////////////////////////////////////////////////////
/// LOCAL VARIABLE

static struct dentry *root_dir;

////////////////////////////////////////////////////////////////////////////////
/// LOCAL FUNCTION

static ssize_t rs_dbgfs_stats_read(struct file *file, char __user *user_buf, size_t count, loff_t *ppos)
{
	// struct rs_net_cfg80211_priv *priv = file->private_data;
	char *buf;
	size_t len = 0, buf_len = 8192;
	ssize_t ret;
	// s32 i, total;

	buf = kzalloc(buf_len, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	len += scnprintf(buf + len, buf_len - len, "Tx: sent %u if_err %u\n", rs_c_dbg_stat.tx.nb_sent,
			 rs_c_dbg_stat.tx.nb_if_err);

	// len += scnprintf(buf + len, buf_len - len, "\n HW Queue     ");
	// for (i = 0; i < RS_TXQ_CNT; i++) {
	//	len += scnprintf(buf + len, buf_len - len, " [%1d]:%3d" , i,
	//		priv->tx.busq[i].balance);
	// }

	// len += scnprintf(buf + len, buf_len - len, "\n SKB remains  ");
	// for (i = 0; i < RS_TXQ_CNT; i++) {
	//	len += scnprintf(buf + len, buf_len - len, " [%1d]:%3d", i,
	//		skb_queue_len(&priv->tx.skb_queue[i]));
	// }

	// total = 0;
	// for (i = 0; i < RS_TXQ_CNT; i++)
	//	total += priv->tx.stat.nb_kick[i];
	// len += scnprintf(buf + len, buf_len - len, "\n Kick       %d ", total);
	// for (i = 0; i < RS_TXQ_CNT; i++) {
	//	len += scnprintf(buf + len, buf_len - len, " [%1d]:%3d", i,
	//		priv->tx.stat.nb_kick[i]);
	// }

	// len += scnprintf(buf + len, buf_len - len, "\n Drop         ");
	// for (i = 0; i < RS_TXQ_CNT; i++) {
	//	len += scnprintf(buf + len, buf_len - len, " [%1d]:%3d" ,
	//		i, priv->tx.stat.nb_drop[i]);
	// }

	// len += scnprintf(buf + len, buf_len - len, "\n Retry/Drop   ");
	// for (i = 0; i < RS_TXQ_CNT; i++) {
	//	len += scnprintf(buf + len, buf_len - len, " [%1d]:%3d/%3d",
	//		i, priv->tx.stat.nb_retry[i], priv->tx.stat.nb_retry_drop[i]);
	// }

	// len += scnprintf(buf + len, buf_len - len, "\n Stops/Wakes  ");
	// for (i = 0; i < RS_TXQ_CNT; i++) {
	//	len += scnprintf(buf + len, buf_len - len, " [%1d]:%3d/%3d",
	//		i, priv->tx.stat.stops[i], priv->tx.stat.wakes[i]);
	// }

	/************************************************************************/

	len += scnprintf(buf + len, buf_len - len, "\nRx: recv %u, err len %d\n", rs_c_dbg_stat.rx.nb_recv,
			 rs_c_dbg_stat.rx.nb_err_len);

	// len += scnprintf(buf + len, buf_len - len,
	//	" Status: forward %d other %d all %d\n",
	//	priv->stats.rx_stat_nb_forward, priv->stats.rx_stat_nb_noforward,
	//	priv->stats.rx_stat_nb_forward + priv->stats.rx_stat_nb_noforward);

	// len += scnprintf(buf + len, buf_len - len, " Error: bus %d desc %d len %d fmt %d handle %d\n",
	//	priv->rx.nb_err_bus, priv->rx.nb_err_desc, priv->rx.nb_err_len, priv->rx.nb_err_fmt,
	//	priv->rx.nb_err_handle);

	ret = simple_read_from_buffer(user_buf, count, ppos, buf, len);

	rs_k_free(buf);
	return ret;
}

RS_DBGFS_OPS_RD(stats);

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL FUNCTION

rs_ret rs_net_dbgfs_register(struct rs_net_cfg80211_priv *net_priv)
{
	rs_ret ret = RS_SUCCESS;

	root_dir = debugfs_create_dir("rrq61004", net_priv->wiphy->debugfsdir);
	if (!root_dir)
		return RS_MEMORY_FAIL;

	RS_DBGFS_CR_FILE(stats, root_dir, 0600);
	RS_DBGFS_CR_U32(log_level, root_dir, &rs_log_level, 0600);

	return ret;
}

void rs_net_dbgfs_deregister(void)
{
	if (!!root_dir)
		debugfs_remove_recursive(root_dir);
	root_dir = NULL;
}

#endif /* CONFIG_DEBUG_FS */
