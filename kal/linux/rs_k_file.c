// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Copyright (C) [2022-2025] Renesas Electronics Corporation and/or its
 * affiliates.
 */

////////////////////////////////////////////////////////////////////////////////
/// INCLUDE

#include <linux/version.h>
#include <linux/uaccess.h>
#include <linux/fs.h>

#include "rs_type.h"

#include "rs_k_file.h"

////////////////////////////////////////////////////////////////////////////////
/// MACRO DEFINITION

#ifndef CONFIG_SET_FS

#define F_FS_INIT()
#define F_FS_SET()
#define F_FS_RST()

#else

#define F_FS_INIT() static mm_segment_t fs;
#define F_FS_SET()                 \
	{                          \
		fs = get_fs();     \
		set_fs(KERNEL_DS); \
	}
#define F_FS_RST() set_fs(fs)

#endif

////////////////////////////////////////////////////////////////////////////////
/// TYPE DEFINITION

////////////////////////////////////////////////////////////////////////////////
/// LOCAL VARIABLE

////////////////////////////////////////////////////////////////////////////////
/// LOCAL FUNCTION

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL FUNCTION

rs_ret rs_k_fopen(struct rs_k_file *k_file, const u8 *file_path, s32 flags, s32 mode)
{
	rs_ret ret = RS_FAIL;

	F_FS_INIT();

	if (k_file) {
		F_FS_SET();
		k_file->file = filp_open((const char *)file_path, (int)flags, mode);
		F_FS_RST();

		if (k_file->file) {
			ret = RS_SUCCESS;

			k_file->flags = flags;
			k_file->mode = mode;
		}
	}

	return ret;
}

rs_ret rs_k_fclose(struct rs_k_file *k_file)
{
	rs_ret ret = RS_FAIL;

	F_FS_INIT();

	if (k_file && k_file->file) {
		F_FS_SET();
		ret = filp_close(k_file->file, NULL);
		F_FS_RST();

		if (ret == 0) {
			k_file->file = NULL;
			k_file->flags = -1;
			k_file->mode = -1;
		}
	}

	return ret;
}

s32 rs_k_fread(struct rs_k_file *k_file, u8 *buf, s32 buf_len)
{
	s32 ret = RS_FAIL;
	loff_t pos = 0;

	F_FS_INIT();

	if (k_file && k_file->file && buf) {
		pos = k_file->pos;
		F_FS_SET();
		ret = kernel_read(k_file->file, (char __user *)buf, buf_len, &pos);
		F_FS_RST();

		if (ret >= 0) {
			k_file->pos = (s32)pos;
		}
	}

	return ret;
}

s32 rs_k_fwrite(struct rs_k_file *k_file, const u8 *buf, s32 buf_len)
{
	s32 ret = RS_FAIL;
	loff_t pos = 0;

	F_FS_INIT();

	if (k_file && k_file->file && buf) {
		pos = k_file->pos;
		F_FS_SET();
		ret = kernel_write(k_file->file, (const char __user *)buf, buf_len, &pos);
		F_FS_RST();

		if (ret >= 0) {
			k_file->pos = (s32)pos;
		}
	}

	return ret;
}
