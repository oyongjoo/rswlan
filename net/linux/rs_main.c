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
#include "rs_c_if.h"
#include "rs_core.h"
#include "rs_c_status.h"

////////////////////////////////////////////////////////////////////////////////
/// MACRO DEFINITION

#define MODULE_VER_NUM	    VERSION

#define RS_WLAN_DESCRIPTION "Renesas wireless driver"
#define RS_WLAN_COPYRIGHT   "Copyright (C) 2021-2025 Renesas Electronics Corporation"
#define RS_WLAN_AUTHOR	    "Renesas Electronics"
#define RS_WLAN_VERSION	    MODULE_VER_NUM

////////////////////////////////////////////////////////////////////////////////
/// TYPE DEFINITION

////////////////////////////////////////////////////////////////////////////////
/// LOCAL VARIABLE

////////////////////////////////////////////////////////////////////////////////
/// LOCAL FUNCTION

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL FUNCTION

#ifndef CONFIG_RS_COMBINE_DRIVER
EXPORT_SYMBOL(rs_core_init);
EXPORT_SYMBOL(rs_core_deinit);
EXPORT_SYMBOL(rs_c_if_deinit);
EXPORT_SYMBOL(rs_c_if_get_dev);
EXPORT_SYMBOL(rs_c_if_set_dev);
EXPORT_SYMBOL(rs_c_if_get_dev_if);
EXPORT_SYMBOL(rs_c_if_set_dev_if);
EXPORT_SYMBOL(rs_c_status);

MODULE_DESCRIPTION(RS_WLAN_DESCRIPTION);
MODULE_VERSION(RS_WLAN_VERSION);
MODULE_AUTHOR(RS_WLAN_AUTHOR);
MODULE_LICENSE("GPL");
#endif