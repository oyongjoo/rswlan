// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Copyright (C) [2022-2025] Renesas Electronics Corporation and/or its
 * affiliates.
 */

////////////////////////////////////////////////////////////////////////////////
/// INCLUDE

#include <linux/version.h>
#include <linux/module.h>

#include "version_gen.h"

#include "rs_type.h"
#include "rs_c_dbg.h"
#include "rs_core.h"
#include "rs_c_status.h"

#include "rs_k_sdio.h"

////////////////////////////////////////////////////////////////////////////////
/// MACRO DEFINITION

#define MODULE_VER_NUM	    VERSION

#define RS_WLAN_DESCRIPTION "Renesas wireless SDIO driver"
#define RS_WLAN_COPYRIGHT   "Copyright (C) 2021-2025 Renesas Electronics Corporation"
#define RS_WLAN_AUTHOR	    "Renesas Electronics"
#define RS_WLAN_VERSION	    MODULE_VER_NUM

////////////////////////////////////////////////////////////////////////////////
/// TYPE DEFINITION

////////////////////////////////////////////////////////////////////////////////
/// LOCAL VARIABLE

////////////////////////////////////////////////////////////////////////////////
/// LOCAL FUNCTION

static void rs_sdio_version(void)
{
	RS_INFO(VERSION COMPILE_TIME "\n");
}

static s32 __init rs_sdio_mod_init(void)
{
	s32 ret = 0;

	rs_sdio_version();

	ret = rs_k_sdio_init(rs_core_init, rs_core_deinit, rs_c_status);

	return ret;
}

static void __exit rs_sdio_mod_exit(void)
{
	(void)rs_k_sdio_deinit(NULL);
}

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL FUNCTION

module_init(rs_sdio_mod_init);
module_exit(rs_sdio_mod_exit);

MODULE_DESCRIPTION(RS_WLAN_DESCRIPTION);
MODULE_VERSION(RS_WLAN_VERSION);
MODULE_AUTHOR(RS_WLAN_AUTHOR);
MODULE_LICENSE("GPL");
