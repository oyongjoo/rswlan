// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Copyright (C) [2022-2025] Renesas Electronics Corporation and/or its
 * affiliates.
 */
#include <linux/list.h>
#include <linux/kernel.h>
#include <linux/ktime.h>
#include <net/mac80211.h>

#include "rs_net_dfs.h"
#include "rs_c_dbg.h"
#include "rs_net_cfg80211.h"
#include "rs_net_ctrl.h"
#include "rs_k_mem.h"

#define PRI_TOLERANCE 16
struct radar_types {
	enum nl80211_dfs_regions region;
	u32 num_radar_types;
	const struct rs_net_dfs_specs *spec_riu;
	const struct rs_net_dfs_specs *spec_fcu;
};

/* percentage on ppb threshold to trigger detection */
#define MIN_PPB_THRESH	50
#define PPB_THRESH(PPB) ((PPB * MIN_PPB_THRESH + 50) / 100)
#define PRF2PRI(PRF)	((1000000 + PRF / 2) / PRF)

/* width tolerance */
#define WIDTH_TOLERANCE 2
#define WIDTH_LOWER(X)	(X)
#define WIDTH_UPPER(X)	(X)

#define ETSI_PATTERN_SHORT(ID, WMIN, WMAX, PMIN, PMAX, PPB) \
	{ ID,                                               \
	  WIDTH_LOWER(WMIN),                                \
	  WIDTH_UPPER(WMAX),                                \
	  (PRF2PRI(PMAX) - PRI_TOLERANCE),                  \
	  (PRF2PRI(PMIN) + PRI_TOLERANCE),                  \
	  1,                                                \
	  PPB,                                              \
	  PPB_THRESH(PPB),                                  \
	  PRI_TOLERANCE,                                    \
	  DFS_WAVEFORM_SHORT }

#define ETSI_PATTERN_INTERLEAVED(ID, WMIN, WMAX, PMIN, PMAX, PRFMIN, PRFMAX, PPB) \
	{ ID,                                                                     \
	  WIDTH_LOWER(WMIN),                                                      \
	  WIDTH_UPPER(WMAX),                                                      \
	  (PRF2PRI(PMAX) * PRFMIN - PRI_TOLERANCE),                               \
	  (PRF2PRI(PMIN) * PRFMAX + PRI_TOLERANCE),                               \
	  PRFMAX,                                                                 \
	  PPB * PRFMAX,                                                           \
	  PPB_THRESH(PPB),                                                        \
	  PRI_TOLERANCE,                                                          \
	  DFS_WAVEFORM_INTERLEAVED }

/* radar types as defined by ETSI EN-301-893 v1.7.1 */
static const struct rs_net_dfs_specs etsi_radar_ref_types_v17_riu[] = {
	ETSI_PATTERN_SHORT(0, 0, 8, 700, 700, 18),
	ETSI_PATTERN_SHORT(1, 0, 10, 200, 1000, 10),
	ETSI_PATTERN_SHORT(2, 0, 22, 200, 1600, 15),
	ETSI_PATTERN_SHORT(3, 0, 22, 2300, 4000, 25),
	ETSI_PATTERN_SHORT(4, 20, 38, 2000, 4000, 20),
	ETSI_PATTERN_INTERLEAVED(5, 0, 8, 300, 400, 2, 3, 10),
	ETSI_PATTERN_INTERLEAVED(6, 0, 8, 400, 1200, 2, 3, 15),
};

static const struct rs_net_dfs_specs etsi_radar_ref_types_v17_fcu[] = {
	ETSI_PATTERN_SHORT(0, 0, 8, 700, 700, 18),
	ETSI_PATTERN_SHORT(1, 0, 8, 200, 1000, 10),
	ETSI_PATTERN_SHORT(2, 0, 16, 200, 1600, 15),
	ETSI_PATTERN_SHORT(3, 0, 16, 2300, 4000, 25),
	ETSI_PATTERN_SHORT(4, 20, 34, 2000, 4000, 20),
	ETSI_PATTERN_INTERLEAVED(5, 0, 8, 300, 400, 2, 3, 10),
	ETSI_PATTERN_INTERLEAVED(6, 0, 8, 400, 1200, 2, 3, 15),
};

static const struct radar_types etsi_radar_types_v17 = {
	.region = NL80211_DFS_ETSI,
	.num_radar_types = ARRAY_SIZE(etsi_radar_ref_types_v17_riu),
	.spec_riu = etsi_radar_ref_types_v17_riu,
	.spec_fcu = etsi_radar_ref_types_v17_fcu,
};

#define FCC_PATTERN(ID, WMIN, WMAX, PMIN, PMAX, PRF, PPB, TYPE)                                        \
	{ ID,  WIDTH_LOWER(WMIN), WIDTH_UPPER(WMAX), PMIN - PRI_TOLERANCE, PMAX * PRF + PRI_TOLERANCE, \
	  PRF, PPB * PRF,	  PPB_THRESH(PPB),   PRI_TOLERANCE,	   TYPE }

static const struct rs_net_dfs_specs fcc_radar_ref_types_riu[] = {
	FCC_PATTERN(0, 0, 8, 1428, 1428, 1, 18, DFS_WAVEFORM_SHORT),
	FCC_PATTERN(1, 0, 8, 518, 3066, 1, 102, DFS_WAVEFORM_WEATHER),
	FCC_PATTERN(2, 0, 8, 150, 230, 1, 23, DFS_WAVEFORM_SHORT),
	FCC_PATTERN(3, 6, 20, 200, 500, 1, 16, DFS_WAVEFORM_SHORT),
	FCC_PATTERN(4, 10, 28, 200, 500, 1, 12, DFS_WAVEFORM_SHORT),
	FCC_PATTERN(5, 50, 110, 1000, 2000, 1, 8, DFS_WAVEFORM_LONG),
	FCC_PATTERN(6, 0, 8, 333, 333, 1, 9, DFS_WAVEFORM_SHORT),
};

static const struct rs_net_dfs_specs fcc_radar_ref_types_fcu[] = {
	FCC_PATTERN(0, 0, 8, 1428, 1428, 1, 18, DFS_WAVEFORM_SHORT),
	FCC_PATTERN(1, 0, 8, 518, 3066, 1, 102, DFS_WAVEFORM_WEATHER),
	FCC_PATTERN(2, 0, 8, 150, 230, 1, 23, DFS_WAVEFORM_SHORT),
	FCC_PATTERN(3, 6, 12, 200, 500, 1, 16, DFS_WAVEFORM_SHORT),
	FCC_PATTERN(4, 10, 22, 200, 500, 1, 12, DFS_WAVEFORM_SHORT),
	FCC_PATTERN(5, 50, 104, 1000, 2000, 1, 8, DFS_WAVEFORM_LONG),
	FCC_PATTERN(6, 0, 8, 333, 333, 1, 9, DFS_WAVEFORM_SHORT),
};

static const struct radar_types fcc_radar_types = {
	.region = NL80211_DFS_FCC,
	.num_radar_types = ARRAY_SIZE(fcc_radar_ref_types_riu),
	.spec_riu = fcc_radar_ref_types_riu,
	.spec_fcu = fcc_radar_ref_types_fcu,
};

#define JP_PATTERN FCC_PATTERN
static const struct rs_net_dfs_specs jp_radar_ref_types_riu[] = {
	JP_PATTERN(0, 0, 8, 1428, 1428, 1, 18, DFS_WAVEFORM_SHORT),
	JP_PATTERN(1, 2, 8, 3846, 3846, 1, 18, DFS_WAVEFORM_SHORT),
	JP_PATTERN(2, 0, 8, 1388, 1388, 1, 18, DFS_WAVEFORM_SHORT),
	JP_PATTERN(3, 0, 8, 4000, 4000, 1, 18, DFS_WAVEFORM_SHORT),
	JP_PATTERN(4, 0, 8, 150, 230, 1, 23, DFS_WAVEFORM_SHORT),
	JP_PATTERN(5, 6, 20, 200, 500, 1, 16, DFS_WAVEFORM_SHORT),
	JP_PATTERN(6, 10, 28, 200, 500, 1, 12, DFS_WAVEFORM_SHORT),
	JP_PATTERN(7, 50, 110, 1000, 2000, 1, 8, DFS_WAVEFORM_LONG),
	JP_PATTERN(8, 0, 8, 333, 333, 1, 9, DFS_WAVEFORM_SHORT),
};

static const struct rs_net_dfs_specs jp_radar_ref_types_fcu[] = {
	JP_PATTERN(0, 0, 8, 1428, 1428, 1, 18, DFS_WAVEFORM_SHORT),
	JP_PATTERN(1, 2, 6, 3846, 3846, 1, 18, DFS_WAVEFORM_SHORT),
	JP_PATTERN(2, 0, 8, 1388, 1388, 1, 18, DFS_WAVEFORM_SHORT),
	JP_PATTERN(3, 2, 2, 4000, 4000, 1, 18, DFS_WAVEFORM_SHORT),
	JP_PATTERN(4, 0, 8, 150, 230, 1, 23, DFS_WAVEFORM_SHORT),
	JP_PATTERN(5, 6, 12, 200, 500, 1, 16, DFS_WAVEFORM_SHORT),
	JP_PATTERN(6, 10, 22, 200, 500, 1, 12, DFS_WAVEFORM_SHORT),
	JP_PATTERN(7, 50, 104, 1000, 2000, 1, 8, DFS_WAVEFORM_LONG),
	JP_PATTERN(8, 0, 8, 333, 333, 1, 9, DFS_WAVEFORM_SHORT),
};

static const struct radar_types jp_radar_types = {
	.region = NL80211_DFS_JP,
	.num_radar_types = ARRAY_SIZE(jp_radar_ref_types_riu),
	.spec_riu = jp_radar_ref_types_riu,
	.spec_fcu = jp_radar_ref_types_fcu,
};

static const struct radar_types *dfs_domains[] = {
	&etsi_radar_types_v17,
	&fcc_radar_types,
	&jp_radar_types,
};
struct pulse_elem {
	struct list_head head;
	u64 ts;
};

struct radar_pulse {
	s32 freq : 6;
	u32 fom : 4;
	u32 len : 6;
	u32 rep : 16;
};

static u32 singleton_pool_references;
static LIST_HEAD(pulse_pool);
static LIST_HEAD(pseq_pool);
static DEFINE_SPINLOCK(pool_lock);

static void net_pool_register_ref(void)
{
	spin_lock_bh(&pool_lock);
	singleton_pool_references++;
	spin_unlock_bh(&pool_lock);
}

static void net_pool_deregister_ref(void)
{
	spin_lock_bh(&pool_lock);
	singleton_pool_references--;
	if (singleton_pool_references == 0) {
		/* free singleton pools with no references left */
		struct rs_net_pri_sequence *ps, *ps0;
		struct pulse_elem *p, *p0;

		list_for_each_entry_safe(p, p0, &pulse_pool, head) {
			list_del(&p->head);
			rs_k_free(p);
		}
		list_for_each_entry_safe(ps, ps0, &pseq_pool, head) {
			list_del(&ps->head);
			rs_k_free(ps);
		}
	}
	spin_unlock_bh(&pool_lock);
}

static void net_pool_put_pulse_elem(struct pulse_elem *pe)
{
	spin_lock_bh(&pool_lock);
	list_add(&pe->head, &pulse_pool);
	spin_unlock_bh(&pool_lock);
}

static void net_pool_put_pseq_elem(struct rs_net_pri_sequence *pse)
{
	spin_lock_bh(&pool_lock);
	list_add(&pse->head, &pseq_pool);
	spin_unlock_bh(&pool_lock);
}

static struct rs_net_pri_sequence *net_pool_get_pseq_elem(void)
{
	struct rs_net_pri_sequence *pse = NULL;
	spin_lock_bh(&pool_lock);
	if (!list_empty(&pseq_pool)) {
		pse = list_first_entry(&pseq_pool, struct rs_net_pri_sequence, head);
		list_del(&pse->head);
	}
	spin_unlock_bh(&pool_lock);

	if (pse == NULL) {
		pse = rs_k_malloc(sizeof(*pse));
	}

	return pse;
}

static struct pulse_elem *net_pool_get_pulse_elem(void)
{
	struct pulse_elem *pe = NULL;
	spin_lock_bh(&pool_lock);
	if (!list_empty(&pulse_pool)) {
		pe = list_first_entry(&pulse_pool, struct pulse_elem, head);
		list_del(&pe->head);
	}
	spin_unlock_bh(&pool_lock);
	return pe;
}

static struct pulse_elem *net_pulse_queue_get_tail(struct rs_net_pri_detector *pde)
{
	struct list_head *l = &pde->pulses;
	if (list_empty(l))
		return NULL;
	return list_entry(l->prev, struct pulse_elem, head);
}

static bool net_pulse_queue_dequeue(struct rs_net_pri_detector *pde)
{
	struct pulse_elem *p = net_pulse_queue_get_tail(pde);
	if (p != NULL) {
		list_del_init(&p->head);
		pde->count--;
		/* give it back to pool */
		net_pool_put_pulse_elem(p);
	}
	return (pde->count > 0);
}

static void net_pulse_queue_check_window(struct rs_net_pri_detector *pde)
{
	u64 min_valid_ts;
	struct pulse_elem *p;

	/* there is no delta time with less than 2 pulses */
	if (pde->count < 2)
		return;

	if (pde->last_ts <= pde->window_size)
		return;

	min_valid_ts = pde->last_ts - pde->window_size;
	while ((p = net_pulse_queue_get_tail(pde)) != NULL) {
		if (p->ts >= min_valid_ts)
			return;
		net_pulse_queue_dequeue(pde);
	}
}

static bool net_pulse_queue_enqueue(struct rs_net_pri_detector *pde, u64 ts)
{
	struct pulse_elem *p = net_pool_get_pulse_elem();
	if (p == NULL) {
		p = rs_k_malloc(sizeof(*p));
		if (p == NULL) {
			return false;
		}
	}
	INIT_LIST_HEAD(&p->head);
	p->ts = ts;
	list_add(&p->head, &pde->pulses);
	pde->count++;
	pde->last_ts = ts;
	net_pulse_queue_check_window(pde);
	if (pde->count >= pde->max_count)
		net_pulse_queue_dequeue(pde);

	return true;
}

static u32 net_pde_get_multiple(u32 val, u32 fraction, u32 tolerance)
{
	u32 remainder;
	u32 factor;
	u32 delta;

	if (fraction == 0)
		return 0;

	delta = (val < fraction) ? (fraction - val) : (val - fraction);

	if (delta <= tolerance)
		/* val and fraction are within tolerance */
		return 1;

	factor = val / fraction;
	remainder = val % fraction;
	if (remainder > tolerance) {
		/* no exact match */
		if ((fraction - remainder) <= tolerance)
			/* remainder is within tolerance */
			factor++;
		else
			factor = 0;
	}
	return factor;
}

static bool net_pde_short_create_sequences(struct rs_net_pri_detector *pde, u64 ts, u32 min_count)
{
	struct pulse_elem *p;
	u16 pulse_idx = 0;

	list_for_each_entry(p, &pde->pulses, head) {
		struct rs_net_pri_sequence ps, *new_ps;
		struct pulse_elem *p2;
		u32 tmp_false_count;
		u64 min_valid_ts;
		u32 delta_ts = ts - p->ts;
		pulse_idx++;

		if (delta_ts < pde->rs->pri_min)
			continue;

		if (delta_ts > pde->rs->pri_max)
			break;

		ps.count = 2;
		ps.count_falses = pulse_idx - 1;
		ps.first_ts = p->ts;
		ps.last_ts = ts;
		ps.pri = ts - p->ts;
		ps.dur = ps.pri * (pde->rs->ppb - 1) + 2 * pde->rs->max_pri_tolerance;

		p2 = p;
		tmp_false_count = 0;
		if (ps.dur > ts)
			min_valid_ts = 0;
		else
			min_valid_ts = ts - ps.dur;

		list_for_each_entry_continue(p2, &pde->pulses, head) {
			u32 factor;
			if (p2->ts < min_valid_ts)
				break;

			factor =
				net_pde_get_multiple(ps.last_ts - p2->ts, ps.pri, pde->rs->max_pri_tolerance);
			if (factor > 0) {
				ps.count++;
				ps.first_ts = p2->ts;
				/*
                 * on match, add the intermediate falses
                 * and reset counter
                 */
				ps.count_falses += tmp_false_count;
				tmp_false_count = 0;
			} else {
				/* this is a potential false one */
				tmp_false_count++;
			}
		}
		if (ps.count <= min_count) {
			continue;
		}
		/* this is a valid one, add it */
		ps.deadline_ts = ps.first_ts + ps.dur;
		if (pde->rs->waveform == DFS_WAVEFORM_WEATHER) {
			ps.ppb_thresh = 19000000 / (360 * ps.pri);
			ps.ppb_thresh = PPB_THRESH(ps.ppb_thresh);
		} else {
			ps.ppb_thresh = pde->rs->ppb_thresh;
		}

		new_ps = net_pool_get_pseq_elem();
		if (new_ps == NULL) {
			return false;
		}
		rs_k_memcpy(new_ps, &ps, sizeof(ps));
		INIT_LIST_HEAD(&new_ps->head);
		list_add(&new_ps->head, &pde->sequences);
	}
	return true;
}

static u32 net_pde_short_add_to_existing_seqs(struct rs_net_pri_detector *pde, u64 ts)
{
	u32 max_count = 0;
	struct rs_net_pri_sequence *ps, *ps2;
	list_for_each_entry_safe(ps, ps2, &pde->sequences, head) {
		u32 delta_ts;
		u32 factor;

		if (ts > ps->deadline_ts) {
			list_del_init(&ps->head);
			net_pool_put_pseq_elem(ps);
			continue;
		}

		delta_ts = ts - ps->last_ts;
		factor = net_pde_get_multiple(delta_ts, ps->pri, pde->rs->max_pri_tolerance);

		if (factor > 0) {
			ps->last_ts = ts;
			ps->count++;

			if (max_count < ps->count)
				max_count = ps->count;
		} else {
			ps->count_falses++;
		}
	}
	return max_count;
}

static struct rs_net_pri_sequence *net_pde_short_check_detection(struct rs_net_pri_detector *pde)
{
	struct rs_net_pri_sequence *ps;

	if (list_empty(&pde->sequences))
		return NULL;

	list_for_each_entry(ps, &pde->sequences, head) {
		if ((ps->count >= ps->ppb_thresh) && (ps->count * pde->rs->num_pri > ps->count_falses)) {
			return ps;
		}
	}
	return NULL;
}

static void net_pde_short_init(struct rs_net_pri_detector *pde)
{
	pde->window_size = pde->rs->pri_max * pde->rs->ppb * pde->rs->num_pri;
	pde->max_count = pde->rs->ppb * 2;
}

static void net_pri_detector_reset(struct rs_net_pri_detector *pde, u64 ts);

static struct rs_net_pri_sequence *net_pde_short_add_pulse(struct rs_net_pri_detector *pde, u16 len, u64 ts,
							   u16 pri)
{
	u32 max_updated_seq;
	struct rs_net_pri_sequence *ps;
	const struct rs_net_dfs_specs *rs = pde->rs;

	if (pde->count == 0) {
		net_pulse_queue_enqueue(pde, ts);
		return NULL;
	}

	if ((ts - pde->last_ts) < rs->max_pri_tolerance) {
		return NULL;
	}

	max_updated_seq = net_pde_short_add_to_existing_seqs(pde, ts);

	if (!net_pde_short_create_sequences(pde, ts, max_updated_seq)) {
		net_pri_detector_reset(pde, ts);
		return NULL;
	}

	ps = net_pde_short_check_detection(pde);

	if (ps == NULL)
		net_pulse_queue_enqueue(pde, ts);

	return ps;
}

static struct rs_net_pri_detector_ops net_pri_detector_short = {
	.init = net_pde_short_init,
	.add_pulse = net_pde_short_add_pulse,
	.reset_on_pri_overflow = 1,
};

/***************************************************************************
 * Long waveform
 **************************************************************************/
#define LONG_RADAR_DURATION	      12000000
#define LONG_RADAR_BURST_MIN_DURATION (12000000 / 20)
#define LONG_RADAR_MAX_BURST	      20

static void net_pde_long_init(struct rs_net_pri_detector *pde)
{
	pde->window_size = LONG_RADAR_DURATION;
	pde->max_count = LONG_RADAR_MAX_BURST;
}

static struct rs_net_pri_sequence *net_pde_long_add_pulse(struct rs_net_pri_detector *pde, u16 len, u64 ts,
							  u16 pri)
{
	struct rs_net_pri_sequence *ps;
	const struct rs_net_dfs_specs *rs = pde->rs;

	if (list_empty(&pde->sequences)) {
		ps = net_pool_get_pseq_elem();
		if (ps == NULL) {
			return NULL;
		}

		ps->count = 1;
		ps->count_falses = 1;
		ps->first_ts = ts;
		ps->last_ts = ts;
		ps->deadline_ts = ts + pde->window_size;
		ps->pri = 0;
		INIT_LIST_HEAD(&ps->head);
		list_add(&ps->head, &pde->sequences);
		net_pulse_queue_enqueue(pde, ts);
	} else {
		u32 delta_ts;

		ps = (struct rs_net_pri_sequence *)pde->sequences.next;

		delta_ts = ts - ps->last_ts;
		ps->last_ts = ts;

		if (delta_ts < rs->pri_max) {
			/* ignore pulse too close from previous one */
		} else if ((delta_ts >= rs->pri_min) && (delta_ts <= rs->pri_max)) {
			ps->count_falses++;
		} else if ((ps->count > 2) && (ps->dur + delta_ts) < LONG_RADAR_BURST_MIN_DURATION) {
			/* not enough time between burst, ignore pulse */
		} else {
			ps->count++;
			ps->count_falses = 1;

			if (ts > ps->deadline_ts) {
				struct pulse_elem *p;
				u64 min_valid_ts;

				min_valid_ts = ts - pde->window_size;
				while ((p = net_pulse_queue_get_tail(pde)) != NULL) {
					if (p->ts >= min_valid_ts) {
						ps->first_ts = p->ts;
						ps->deadline_ts = p->ts + pde->window_size;
						break;
					}
					net_pulse_queue_dequeue(pde);
					ps->count--;
				}
			}

			if (ps->count > pde->rs->ppb_thresh && (ts - ps->first_ts) > (pde->window_size / 2)) {
				return ps;
			} else {
				net_pulse_queue_enqueue(pde, ts);
				ps->dur = delta_ts;
			}
		}
	}

	return NULL;
}

static struct rs_net_pri_detector_ops net_pri_detector_long = {
	.init = net_pde_long_init,
	.add_pulse = net_pde_long_add_pulse,
	.reset_on_pri_overflow = 0,
};

struct rs_net_pri_detector *net_pri_detector_init(struct rs_net_dpd *dpd, u16 dfs_type, u16 freq)
{
	struct rs_net_pri_detector *pde;

	pde = kzalloc(sizeof(*pde), GFP_ATOMIC);
	if (pde == NULL)
		return NULL;

	INIT_LIST_HEAD(&pde->sequences);
	INIT_LIST_HEAD(&pde->pulses);
	INIT_LIST_HEAD(&pde->head);
	list_add(&pde->head, &dpd->ch_detect[dfs_type]);

	pde->rs = &dpd->dfs_spec[dfs_type];
	pde->freq = freq;

	if (pde->rs->waveform == DFS_WAVEFORM_LONG) {
		pde->ops = &net_pri_detector_long;
	} else {
		pde->ops = &net_pri_detector_short;
	}

	/* Init dependent of specs */
	pde->ops->init(pde);

	net_pool_register_ref();
	return pde;
}

static void net_pri_detector_reset(struct rs_net_pri_detector *pde, u64 ts)
{
	struct rs_net_pri_sequence *ps, *ps0;
	struct pulse_elem *p, *p0;
	list_for_each_entry_safe(ps, ps0, &pde->sequences, head) {
		list_del_init(&ps->head);
		net_pool_put_pseq_elem(ps);
	}
	list_for_each_entry_safe(p, p0, &pde->pulses, head) {
		list_del_init(&p->head);
		net_pool_put_pulse_elem(p);
	}
	pde->count = 0;
	pde->last_ts = ts;
}

static void pri_detector_exit(struct rs_net_pri_detector *pde)
{
	net_pri_detector_reset(pde, 0);
	net_pool_deregister_ref();
	list_del(&pde->head);
	rs_k_free(pde);
}

static struct rs_net_pri_detector *pri_detector_get(struct rs_net_dpd *dpd, u16 freq, u16 dfs_type)
{
	struct rs_net_pri_detector *pde, *cur = NULL;
	list_for_each_entry(pde, &dpd->ch_detect[dfs_type], head) {
		if (pde->freq == freq) {
			if (pde->count)
				return pde;
			else
				cur = pde;
		} else if (pde->freq - 2 == freq && pde->count) {
			return pde;
		} else if (pde->freq + 2 == freq && pde->count) {
			return pde;
		}
	}

	if (cur)
		return cur;
	else
		return net_pri_detector_init(dpd, dfs_type, freq);
}

static void dpd_reset(struct rs_net_dpd *dpd)
{
	struct rs_net_pri_detector *pde;
	int i;

	for (i = 0; i < dpd->num_dfs_types; i++) {
		if (!list_empty(&dpd->ch_detect[i]))
			list_for_each_entry(pde, &dpd->ch_detect[i], head)
				net_pri_detector_reset(pde, dpd->last_pulse_ts);
	}

	dpd->last_pulse_ts = 0;
	dpd->prev_jiffies = jiffies;
}

static void dpd_exit(struct rs_net_dpd *dpd)
{
	struct rs_net_pri_detector *pde, *pde0;
	int i;

	for (i = 0; i < dpd->num_dfs_types; i++) {
		if (!list_empty(&dpd->ch_detect[i]))
			list_for_each_entry_safe(pde, pde0, &dpd->ch_detect[i], head)
				pri_detector_exit(pde);
	}
	rs_k_free(dpd);
}

static void dfs_pattern_detector_pri_overflow(struct rs_net_dpd *dpd)
{
	struct rs_net_pri_detector *pde;
	int i;

	for (i = 0; i < dpd->num_dfs_types; i++) {
		list_for_each_entry(pde, &dpd->ch_detect[i], head)
			if (!list_empty(&dpd->ch_detect[i]))
				if (pde->ops->reset_on_pri_overflow)
					net_pri_detector_reset(pde, dpd->last_pulse_ts);
	}
}

static bool dpd_add_pulse(struct rs_net_dpd *dpd, enum rs_net_dfs_type chain, u16 freq, u16 pri, u16 len,
			  u32 now)
{
	u32 i;

	if (dpd->region == NL80211_DFS_UNSET)
		return true;

	if (pri == 0) {
		u32 delta_jiffie;
		if (unlikely(now < dpd->prev_jiffies)) {
			delta_jiffie = 0xffffffff - dpd->prev_jiffies + now;
		} else {
			delta_jiffie = now - dpd->prev_jiffies;
		}
		dpd->last_pulse_ts += jiffies_to_usecs(delta_jiffie);
		dpd->prev_jiffies = now;
		dfs_pattern_detector_pri_overflow(dpd);
	} else {
		dpd->last_pulse_ts += pri;
	}

	for (i = 0; i < dpd->num_dfs_types; i++) {
		struct rs_net_pri_sequence *ps;
		struct rs_net_pri_detector *pde;
		const struct rs_net_dfs_specs *rs = &dpd->dfs_spec[i];

		if ((rs->width_min > len) || (rs->width_max < len)) {
			continue;
		}

		pde = pri_detector_get(dpd, freq, i);
		ps = pde->ops->add_pulse(pde, len, dpd->last_pulse_ts, pri);

		if (ps != NULL) {
			dpd_reset(dpd);
			return true;
		}
	}

	return false;
}

static const struct radar_types *net_get_dfs_domain_types(enum nl80211_dfs_regions region)
{
	u32 i;
	for (i = 0; i < ARRAY_SIZE(dfs_domains); i++) {
		if (dfs_domains[i]->region == region)
			return dfs_domains[i];
	}
	return NULL;
}

static u16 net_get_dfs_max_types(void)
{
	u32 i;
	u16 max = 0;
	for (i = 0; i < ARRAY_SIZE(dfs_domains); i++) {
		if (dfs_domains[i]->num_radar_types > max)
			max = dfs_domains[i]->num_radar_types;
	}
	return max;
}

static bool net_dpd_set_domain(struct rs_net_dpd *dpd, enum nl80211_dfs_regions region, u8 chain)
{
	const struct radar_types *rt;
	struct rs_net_pri_detector *pde, *pde0;
	int i;

	if (dpd->region == region)
		return true;

	dpd->region = NL80211_DFS_UNSET;

	rt = net_get_dfs_domain_types(region);
	if (rt == NULL)
		return false;

	/* delete all pri detectors for previous DFS domain */
	for (i = 0; i < dpd->num_dfs_types; i++) {
		if (!list_empty(&dpd->ch_detect[i]))
			list_for_each_entry_safe(pde, pde0, &dpd->ch_detect[i], head)
				pri_detector_exit(pde);
	}

	if (chain == RS_DFS_RIU)
		dpd->dfs_spec = rt->spec_riu;
	else
		dpd->dfs_spec = rt->spec_fcu;
	dpd->num_dfs_types = rt->num_radar_types;

	dpd->region = region;
	return true;
}

static struct rs_net_dpd *net_dpd_init(enum nl80211_dfs_regions region, u8 chain)
{
	struct rs_net_dpd *dpd;
	u16 i, max_radar_type = net_get_dfs_max_types();

	dpd = rs_k_malloc(sizeof(*dpd) + max_radar_type * sizeof(dpd->ch_detect[0]));
	if (dpd == NULL)
		return NULL;

	dpd->region = NL80211_DFS_UNSET;
	dpd->enabled = RS_DFS_DETECT_DISABLE;
	dpd->last_pulse_ts = 0;
	dpd->prev_jiffies = jiffies;
	dpd->num_dfs_types = 0;
	for (i = 0; i < max_radar_type; i++)
		INIT_LIST_HEAD(&dpd->ch_detect[i]);

	if (net_dpd_set_domain(dpd, region, chain))
		return dpd;

	RS_DBG(RS_FN_ENTRY_STR_ " : DFG Couldn't set DFS domain to %d\n", __func__, region);
	rs_k_free(dpd);
	return NULL;
}

/******************************************************************************
 * driver interface
 *****************************************************************************/
static u16 net_get_center_freq(struct rs_net_cfg80211_priv *net_priv, u8 chain)
{
	struct rs_net_chan_info *chan_info = NULL;

	chan_info = rs_net_cfg80211_get_chaninfo(net_priv->dfs.cac_vif);

	if (chan_info == NULL) {
		WARN(1, "Radar pulse without channel information");
	} else
		return chan_info->chan_def.center_freq1;

	return 0;
}

static void net_dfs_detected(struct rs_net_cfg80211_priv *net_priv)
{
	struct cfg80211_chan_def chan_def;
	struct rs_net_chan_info *chan_info = NULL;

	chan_info = rs_net_cfg80211_get_chaninfo(net_priv->dfs.cac_vif);

	if (chan_info == NULL) {
		WARN(1, "Radar detected without channel information");
		return;
	}

	chan_def = chan_info->chan_def;

	rs_net_cac_stop(&net_priv->dfs);
	cfg80211_radar_event(net_priv->wiphy, &chan_def, GFP_KERNEL);
}

static void rs_net_dfs_process_pulse(struct work_struct *ws)
{
	struct rs_net_dfs_priv *dfs = container_of(ws, struct rs_net_dfs_priv, dfs_work);
	struct rs_net_cfg80211_priv *net_priv = container_of(dfs, struct rs_net_cfg80211_priv, dfs);
	int chain;
	u32 pulses[RS_DFS_CHAIN_LAST][RS_NET_DFS_PULSE_MAX];
	u16 pulses_count[RS_DFS_CHAIN_LAST];
	u32 now = jiffies;

	spin_lock_bh(&dfs->lock);
	for (chain = RS_DFS_RIU; chain < RS_DFS_CHAIN_LAST; chain++) {
		int start, count;

		count = dfs->pulses[chain].count;
		start = dfs->pulses[chain].index - count;
		if (start < 0)
			start += RS_NET_DFS_PULSE_MAX;

		pulses_count[chain] = count;
		if (count == 0)
			continue;

		if ((start + count) > RS_NET_DFS_PULSE_MAX) {
			u16 count1 = (RS_NET_DFS_PULSE_MAX - start);
			rs_k_memcpy(&(pulses[chain][0]), &(dfs->pulses[chain].buffer[start]),
				    count1 * sizeof(struct radar_pulse));
			rs_k_memcpy(&(pulses[chain][count1]), &(dfs->pulses[chain].buffer[0]),
				    (count - count1) * sizeof(struct radar_pulse));
		} else {
			rs_k_memcpy(&(pulses[chain][0]), &(dfs->pulses[chain].buffer[start]),
				    count * sizeof(struct radar_pulse));
		}
		dfs->pulses[chain].count = 0;
	}
	spin_unlock_bh(&dfs->lock);

	for (chain = RS_DFS_RIU; chain < RS_DFS_CHAIN_LAST; chain++) {
		int i;
		u16 freq;

		if (pulses_count[chain] == 0)
			continue;

		freq = net_get_center_freq(net_priv, chain);

		for (i = 0; i < pulses_count[chain]; i++) {
			struct radar_pulse *p = (struct radar_pulse *)&pulses[chain][i];
			if (dpd_add_pulse(dfs->dpd[chain], chain, freq + (2 * p->freq), p->rep, (p->len * 2),
					  now)) {
				u16 idx = dfs->detected[chain].index;

				if (chain == RS_DFS_RIU) {
					if (dfs->dpd[chain]->enabled == RS_DFS_DETECT_REPORT) {
						net_dfs_detected(net_priv);
						rs_net_dfs_detection_enable(dfs, RS_DFS_DETECT_DISABLE,
									    chain);
						dfs->pulses[chain].count = 0;
					}
				}

				dfs->detected[chain].freq[idx] = (s16)freq + (2 * p->freq);
				dfs->detected[chain].time[idx] = ktime_get_real_seconds();
				dfs->detected[chain].index = ((idx + 1) % RS_NET_DFS_DETECTED_MAX);
				dfs->detected[chain].count++;
				break;
			}
		}
	}
}

#ifdef CONFIG_RS_FULLMAC
static void net_dfs_cac_work(struct work_struct *ws)
{
	struct delayed_work *dw = container_of(ws, struct delayed_work, work);
	struct rs_net_dfs_priv *dfs = container_of(dw, struct rs_net_dfs_priv, cac_work);
	struct rs_net_cfg80211_priv *net_priv = container_of(dfs, struct rs_net_cfg80211_priv, dfs);
	struct rs_net_chan_info *chan_info = NULL;
	struct rs_c_if *c_if = NULL;

	if (dfs->cac_vif == NULL) {
		WARN(1, "CAC finished but no vif set");
		return;
	}

	chan_info = rs_net_cfg80211_get_chaninfo(net_priv->dfs.cac_vif);
	cfg80211_cac_event(dfs->cac_vif->ndev, &chan_info->chan_def, NL80211_RADAR_CAC_FINISHED, GFP_KERNEL);
	c_if = rs_net_priv_get_c_if(net_priv);
	rs_net_ctrl_cac_stop_req(c_if, dfs->cac_vif);
	rs_net_cfg80211_unset_chaninfo(dfs->cac_vif);

	dfs->cac_vif = NULL;
}
#endif /* CONFIG_RS_FULLMAC */

bool rs_net_dfs_detection_init(struct rs_net_dfs_priv *dfs)
{
	spin_lock_init(&dfs->lock);

	dfs->dpd[RS_DFS_RIU] = net_dpd_init(NL80211_DFS_UNSET, RS_DFS_RIU);
	if (dfs->dpd[RS_DFS_RIU] == NULL)
		return false;

	dfs->dpd[RS_DFS_FCU] = net_dpd_init(NL80211_DFS_UNSET, RS_DFS_FCU);

	if (dfs->dpd[RS_DFS_FCU] == NULL) {
		rs_net_dfs_detection_deinit(dfs);
		return false;
	}

	INIT_WORK(&dfs->dfs_work, rs_net_dfs_process_pulse);
#ifdef CONFIG_RS_FULLMAC
	INIT_DELAYED_WORK(&dfs->cac_work, net_dfs_cac_work);
	dfs->cac_vif = NULL;
#endif /* CONFIG_RS_FULLMAC */
	return true;
}

void rs_net_dfs_detection_deinit(struct rs_net_dfs_priv *dfs)
{
	if (dfs->dpd[RS_DFS_RIU]) {
		dpd_exit(dfs->dpd[RS_DFS_RIU]);
		dfs->dpd[RS_DFS_RIU] = NULL;
	}
	if (dfs->dpd[RS_DFS_FCU]) {
		dpd_exit(dfs->dpd[RS_DFS_FCU]);
		dfs->dpd[RS_DFS_FCU] = NULL;
	}
}

bool rs_net_dfs_set_domain(struct rs_net_dfs_priv *dfs, enum nl80211_dfs_regions region, u8 chain)
{
	if (dfs->dpd[0] == NULL)
		return false;

	return (net_dpd_set_domain(dfs->dpd[RS_DFS_RIU], region, RS_DFS_RIU) &&
		net_dpd_set_domain(dfs->dpd[RS_DFS_FCU], region, RS_DFS_FCU));
}

void rs_net_dfs_detection_enable(struct rs_net_dfs_priv *dfs, u8 enable, u8 chain)
{
	if (chain < RS_DFS_CHAIN_LAST) {
		spin_lock_bh(&dfs->lock);
		dfs->dpd[chain]->enabled = enable;
		spin_unlock_bh(&dfs->lock);
	}
}

bool rs_net_dfs_detection_enbaled(struct rs_net_dfs_priv *dfs, u8 chain)
{
	return dfs->dpd[chain]->enabled != RS_DFS_DETECT_DISABLE;
}

#ifdef CONFIG_RS_FULLMAC
void rs_net_dfs_cac_start(struct rs_net_dfs_priv *dfs, u32 cac_time_ms, struct rs_net_vif_priv *vif)
{
	WARN(dfs->cac_vif != NULL, "CAC already in progress");
	dfs->cac_vif = vif;
	schedule_delayed_work(&dfs->cac_work, msecs_to_jiffies(cac_time_ms));
}

void rs_net_dfs_detection_enabled_on_cur_channel(struct rs_net_cfg80211_priv *net_priv)
{
	struct rs_net_chan_info *chan_info = NULL;

	chan_info = rs_net_cfg80211_get_chaninfo(net_priv->dfs.cac_vif);
	if (chan_info == NULL)
		return;

	if (chan_info->chan_def.chan->flags & IEEE80211_CHAN_RADAR) {
		rs_net_dfs_detection_enable(&net_priv->dfs, RS_DFS_DETECT_REPORT, RS_DFS_RIU);
	} else {
		rs_net_dfs_detection_enable(&net_priv->dfs, RS_DFS_DETECT_DISABLE, RS_DFS_RIU);
	}
}
#endif /* CONFIG_RS_FULLMAC */

static int net_dfs_dump_pri_detector(char *buf, size_t len, struct rs_net_pri_detector *pde)
{
	char freq_info[] = "Freq = %3.dMhz\n";
	char seq_info[] = " pri    | count | false \n";
	struct rs_net_pri_sequence *seq;
	int res, write = 0;

	if (list_empty(&pde->sequences)) {
		return 0;
	}

	if (buf == NULL) {
		int nb_seq = 1;
		list_for_each_entry(seq, &pde->sequences, head) {
			nb_seq++;
		}

		return (sizeof(freq_info) + nb_seq * sizeof(seq_info));
	}

	res = scnprintf(buf, len, freq_info, pde->freq);
	write += res;
	len -= res;

	res = scnprintf(&buf[write], len, "%s", seq_info);
	write += res;
	len -= res;

	list_for_each_entry(seq, &pde->sequences, head) {
		res = scnprintf(&buf[write], len, " %6.d |   %2.d  |    %.2d \n", seq->pri, seq->count,
				seq->count_falses);
		write += res;
		len -= res;
	}

	return write;
}

int rs_net_dfs_dump_pattern_detector(char *buf, size_t len, struct rs_net_dfs_priv *dfs, u8 chain)
{
	struct rs_net_dpd *dpd = dfs->dpd[chain];
	char info[] = "Type = %3.d\n";
	struct rs_net_pri_detector *pde;
	int i, res, write = 0;

	/* if buf is NULL return size needed for dump */
	if (buf == NULL) {
		int size_needed = 0;

		for (i = 0; i < dpd->num_dfs_types; i++) {
			list_for_each_entry(pde, &dpd->ch_detect[i], head) {
				size_needed += net_dfs_dump_pri_detector(NULL, 0, pde);
			}
			size_needed += sizeof(info);

			return size_needed;
		}
	}

	for (i = 0; i < dpd->num_dfs_types; i++) {
		res = scnprintf(&buf[write], len, info, i);

		write += res;
		len -= res;
		list_for_each_entry(pde, &dpd->ch_detect[i], head) {
			res = net_dfs_dump_pri_detector(&buf[write], len, pde);
			write += res;
			len -= res;
		}
	}

	return write;
}

int rs_net_dfs_dump_detected(char *buf, size_t len, struct rs_net_dfs_priv *dfs, u8 chain)
{
	struct rs_net_dfs_detected *detect = &(dfs->detected[chain]);
	char info[] = "2001/02/02 - 02:20 5126MHz\n";
	int idx, i, res, write = 0;
	int count = detect->count;

	if (count > RS_NET_DFS_DETECTED_MAX)
		count = RS_NET_DFS_DETECTED_MAX;

	if (buf == NULL) {
		return (count * sizeof(info)) + 1;
	}

	idx = (detect->index - detect->count) % RS_NET_DFS_DETECTED_MAX;

	for (i = 0; i < count; i++) {
		struct tm tm;
		time64_to_tm(detect->time[idx], 0, &tm);

		res = scnprintf(&buf[write], len, "%.4d/%.2d/%.2d - %.2d:%.2d %4.4dMHz\n",
				(int)tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min,
				detect->freq[idx]);
		write += res;
		len -= res;

		idx = (idx + 1) % RS_NET_DFS_DETECTED_MAX;
	}

	return write;
}
