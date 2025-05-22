// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Copyright (C) [2022-2025] Renesas Electronics Corporation and/or its
 * affiliates.
 */

#ifndef RS_K_FILE_H
#define RS_K_FILE_H

////////////////////////////////////////////////////////////////////////////////
/// INCLUDE

#include "rs_type.h"

////////////////////////////////////////////////////////////////////////////////
/// MACRO DEFINITION

////////////////////////////////////////////////////////////////////////////////
/// TYPE DEFINITION

struct rs_k_file {
	void *file;

	s32 pos;
	s32 flags;
	s32 mode;
};

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL VARIABLE

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL FUNCTION

rs_ret rs_k_fopen(struct rs_k_file *k_file, const u8 *file_path, s32 flags, s32 mode);

rs_ret rs_k_fclose(struct rs_k_file *k_file);

s32 rs_k_fread(struct rs_k_file *k_file, u8 *buf, s32 buf_len);

s32 rs_k_fwrite(struct rs_k_file *k_file, const u8 *buf, s32 buf_len);

#endif /* RS_K_FILE_H */
