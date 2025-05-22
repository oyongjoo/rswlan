#include <net/cfg80211.h>

#include "rs_net_priv.h"
#include "rs_k_mem.h"
#include "rs_c_dbg.h"
#include "rs_net_stats.h"
#include "rs_net_ctrl.h"

#ifdef CONFIG_DBG_STATS

void rs_net_rx_status_update(struct rs_net_cfg80211_priv *net_priv, struct rs_net_sta_stats *stats)
{
	struct rs_net_rx_stats *rx_stats = net_priv->dbg_stats.rx_stats;
	struct rs_c_rx_ext_hdr *last_rx = &stats->last_rx_data_ext;

	if (!(net_priv->dbg_stats.dbg_stats_enabled && rx_stats))
		return;

	rx_stats->format_mode = last_rx->format_mod;

	switch (last_rx->format_mod) {
	case FORMATMOD_NON_HT:
		rx_stats->non_ht.ofdm++;
		rx_stats->flag |= RX_STATS_OFDM;
		fallthrough;
	case FORMATMOD_NON_HT_DUP_OFDM:
		rx_stats->non_ht.cck++;
		rx_stats->flag |= RX_STATS_CCK;
		rx_stats->non_ht.bw = last_rx->ch_bw;
		break;
	case FORMATMOD_HT_MF:
	case FORMATMOD_HT_GF:
		rx_stats->flag |= RX_STATS_HT;
		rx_stats->ht.bw = last_rx->ch_bw & 0x1;
		rx_stats->ht.nss = (last_rx->ht.num_extn_ss) & 0x3;
		rx_stats->ht.mcs = last_rx->ht.mcs & 0x7;
		rx_stats->ht.gi = last_rx->ht.short_gi & 0x1;
		rx_stats->ht.ht[rx_stats->ht.gi][rx_stats->ht.mcs]++;
		break;
	case FORMATMOD_VHT:
		rx_stats->vht.bw = last_rx->ch_bw;
		rx_stats->vht.nss = last_rx->vht.nss;
		rx_stats->vht.mcs = last_rx->vht.mcs;
		rx_stats->vht.gi = last_rx->vht.short_gi & 0x1;
		rx_stats->vht.vht[rx_stats->vht.gi][rx_stats->vht.mcs]++;
		rx_stats->flag |= RX_STATS_VHT;
		break;
	case FORMATMOD_HE_MU:
	case FORMATMOD_HE_SU:
	case FORMATMOD_HE_ER:
	case FORMATMOD_HE_TB:
		rx_stats->flag |= RX_STATS_HE_SU;
		rx_stats->he_su.mcs = last_rx->he.mcs;
		rx_stats->he_su.nss = last_rx->he.nss;
		rx_stats->he_su.gi = last_rx->he.gi_type;
		rx_stats->he_su.he[rx_stats->he_su.gi][rx_stats->he_su.mcs]++;
		break;
	default:
		return;
	}

	return;
}

int rs_net_stats_init(struct rs_net_cfg80211_priv *net_priv)
{
	struct rs_net_rx_stats *rx_stats = NULL;

	if (net_priv->dbg_stats.dbg_stats_enabled)
		return 1;

	rx_stats = (struct rs_net_rx_stats *)rs_k_calloc(sizeof(struct rs_net_rx_stats));
	if (!rx_stats) {
		RS_ERR("[+ %s/%d] alloc failed!!\n", __func__, __LINE__);
		return -ENOMEM;
	}

	net_priv->dbg_stats.rx_stats = rx_stats;
	net_priv->dbg_stats.dbg_stats_enabled = true;

	return 0;
}

int rs_net_stats_deinit(struct rs_net_cfg80211_priv *net_priv)
{
	struct rs_net_rx_stats *rx_stats = net_priv->dbg_stats.rx_stats;
	struct rs_c_if *c_if = NULL;

	if (rx_stats) {
		rs_k_free(rx_stats);
		rx_stats = NULL;
	}

	net_priv->dbg_stats.rx_stats = NULL;
	net_priv->dbg_stats.dbg_stats_enabled = false;

	c_if = rs_net_priv_get_c_if(net_priv);
	rs_net_ctrl_dbg_stats_tx_req(c_if, REQ_STATS_STOP, NULL);

	return 0;
}

#endif