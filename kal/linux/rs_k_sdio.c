// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Copyright (C) [2022-2025] Renesas Electronics Corporation and/or its
 * affiliates.
 */

////////////////////////////////////////////////////////////////////////////////
/// INCLUDE

#include <linux/module.h>
#include <linux/mmc/sdio_ids.h>
#include <linux/mmc/sdio_func.h>
#include <linux/mmc/sdio.h>
#include <linux/mmc/host.h>
#include <linux/firmware.h>
#include <linux/delay.h>

#include "rs_type.h"
#include "rs_k_mem.h"
#include "rs_k_mutex.h"
#include "rs_c_dbg.h"
#include "rs_c_cmd.h"
#include "rs_c_data.h"
#include "rs_c_if.h"

#include "rs_k_if.h"

////////////////////////////////////////////////////////////////////////////////
/// MACRO DEFINITION

#define FW_DOWNLOAD_ENABLE

#define SDIO_DRIVER_NAME	"rswlan_sdio"
#define SDIO_VENDOR_ID		(0x5245)
#define SDIO_DEVICE_ID_RRQ61000 (0x4154)
#define SDIO_DEV_IDX_RRQ61000	(0)

#define SDIO_RW_SIZE		(4)
#define SDIO_BLOCK_SIZE		(512)

#define SDIO_HOST_GP_REG	(0x24)
#define SDIO_CHIP_GP_REG	(0x28)

#define RS_SDIO_DEV_RESET_CMD	(0xdeadffff)

// SDIO fullmac firmware name
#define SDIO_FMAC_FW_NAME	RS_FMAC_FW_BASE_NAME "_sdio.bin"

#define ALIGN_4BYTE(x)		((4 - ((x) % 4)) % 4)
#define ALIGN_512BYTE(x)	((512 - ((x) % 512)) % 512)

#ifndef DEVICE_VER_AA
#define RRQ61000_BA_MULTI_BLOCK_TX
#define RRQ61000_BA_MULTI_BLOCK_RX
#define SDIO_BLOCK_SIZE_FW (64 * 1024)
#else
#define SDIO_BLOCK_SIZE_FW (512)
#endif
#define FW_DOWNLOAD_SPLIT

#define SDIO_MAX_BLOCK_CNT		  (5)

#define RS_SDIO_GET_CNT(len, blk_size)	  (((u32)(len)) / (blk_size))
#define RS_SDIO_GET_REMAIN(len, blk_size) (((u32)(len)) % (blk_size))

#define C_IF_DEV_MUTEX_INIT(c_if) \
	(void)rs_k_mutex_create(&(((struct sdio_dev_if_priv *)((c_if)->if_dev.dev_if_priv))->mutex))
#define C_IF_DEV_MUTEX_DEINIT(c_if) \
	(void)rs_k_mutex_destroy(&(((struct sdio_dev_if_priv *)((c_if)->if_dev.dev_if_priv))->mutex))
#define C_IF_DEV_MUTEX_LOCK(c_if) \
	(void)rs_k_mutex_lock(&(((struct sdio_dev_if_priv *)((c_if)->if_dev.dev_if_priv))->mutex))
#define C_IF_DEV_MUTEX_UNLOCK(c_if) \
	(void)rs_k_mutex_unlock(&(((struct sdio_dev_if_priv *)((c_if)->if_dev.dev_if_priv))->mutex))

////////////////////////////////////////////////////////////////////////////////
/// TYPE DEFINITION

struct reg_n_addr {
	u32 addr;
	u32 value;
};

struct sdio_dev_if_priv {
	struct rs_k_mutex mutex;
	bool suspend;
};

////////////////////////////////////////////////////////////////////////////////
/// LOCAL VARIABLE

static const u32 crc32_tab[] = {
	0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
	0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
	0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
	0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
	0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
	0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
	0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
	0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
	0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
	0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
	0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
	0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
	0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
	0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
	0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
	0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
	0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
	0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
	0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
	0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
	0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
	0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
	0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236, 0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
	0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
	0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
	0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
	0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
	0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
	0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
	0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
	0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
	0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

////////////////////////////////////////////////////////////////////////////////
/// LOCAL FUNCTION

static void k_sdio_recv_handler(struct sdio_func *func)
{
	u8 reg = 0;
	s32 err = -1;

	struct rs_c_if *c_if = sdio_get_drvdata(func);

	sdio_claim_host(func);

	func->num = 1;
	// clear sdio interrupt status
	reg = sdio_readb(func, 0x8, &err);
	if (err != 0) {
		RS_ERR("sdio_readsb err (%d) !!!\n", err);
	}

	// reg = 0x05;
	(void)sdio_writeb(func, reg, 0x8, &err);

	if (err != 0) {
		RS_ERR("sdio_readsb err <%d> !!!\n", err);
	}

	sdio_release_host(func);

	if (c_if && c_if->if_cb && c_if->if_cb->recv_cb) {
		(void)c_if->if_cb->recv_cb(c_if);
	}
}

static rs_ret k_sdio_enable_int(struct rs_c_if *c_if)
{
	rs_ret ret = RS_FAIL;
	s32 err = 0;
	u8 reg = 0;
	struct sdio_func *func = NULL;

	func = rs_c_if_get_dev(c_if);

	if (c_if->if_cb && c_if->if_cb->recv_cb && func) {
		sdio_claim_host(func);

		// enable interrupt
		reg = sdio_f0_readb(func, SDIO_CCCR_IENx, &err);
		if (err == 0) {
			func->num = 0;
			reg |= RS_BIT(0);
			reg |= RS_BIT(func->num);
			sdio_writeb(func, reg, SDIO_CCCR_IENx, &err);

			if (err == 0) {
				reg = RS_BIT(5); //0x20 SDIO_CCCR_CAP_E4MI
				sdio_writeb(func, reg, SDIO_CCCR_CAPS, &err);
			}

			if (err == 0) {
				func->num = 1;
				reg = RS_BIT(2); //0x4;
				sdio_writeb(func, reg, SDIO_CCCR_CIS, &err);
			}

			if (err == 0) {
				// Claim and activate the IRQ for the given SDIO function. The provided
				// handler will be called when that IRQ is asserted.  The host is always
				// claimed already when the handler is called so the handler must not
				// call sdio_claim_host() nor sdio_release_host().
				err = sdio_claim_irq(func, k_sdio_recv_handler);
				if (err != 0) {
					RS_ERR("failed set sdio_claim_irq: ret=%d\n", ret);
				}
			}
		} else {
			RS_ERR("failed to read SDIO_CCCR_IENx: reg=0x%x, err=%x\n", reg, err);
		}

		sdio_release_host(func);
	}

	if (err == 0) {
		ret = RS_SUCCESS;
	}

	return ret;
}

static rs_ret k_sdio_disable_int(struct sdio_func *func)
{
	s32 ret = RS_SUCCESS;

	(void)sdio_claim_host(func);

	ret = sdio_release_irq(func);
	if (ret != RS_SUCCESS) {
		RS_ERR("Failed to release sdio irq: %d\n", ret);
	}

	(void)sdio_release_host(func);

	return ret;
}

static rs_ret k_sdio_init(struct rs_c_if *c_if)
{
	rs_ret ret = RS_FAIL;
	s32 err = 0;
	u8 reg = 0;
	struct sdio_func *func = NULL;

	func = rs_c_if_get_dev(c_if);
	if (c_if && func) {
		sdio_claim_host(func);

		sdio_enable_func(func);
		sdio_set_block_size(func, SDIO_BLOCK_SIZE);

		// check sdio config
		// cccr_card_cap
		reg = sdio_f0_readb(func, SDIO_CCCR_CAPS, &err);
		if ((reg == 0xFF) && (err != 0)) {
			RS_ERR("failed to read SDIO_CCCR_CAPS: reg=0x%x, err=%x\n", reg, err);
		} else {
			// Check SDIO_CCCR_IF
			reg = sdio_f0_readb(func, SDIO_CCCR_IF, &err);
			if ((reg == 0xFF) && (err != 0)) {
				RS_ERR("failed to read SDIO_CCCR_IF: reg=0x%x, err=%x\n", reg, err);
			} else {
				// func 0 : cccr_bus_if_control 4bit mode only // PTEST
				reg = SDIO_BUS_WIDTH_4BIT | 0x80;
				func->num = 0;
				sdio_writeb(func, reg, SDIO_CCCR_IF, &err);
			}
		}

		sdio_release_host(func);
	}

	if (err == 0) {
		ret = RS_SUCCESS;
	}

	return ret;
}

static rs_ret k_sdio_read(struct rs_c_if *c_if, u32 addr, u8 *data, u32 len)
{
	rs_ret ret = RS_FAIL;
	s32 err = 0;
	struct sdio_func *func = NULL;
	s32 temp_len = len;
#ifndef RRQ61000_BA_MULTI_BLOCK_RX
	s32 i = 0;
	u32 cnt = 0;
	u32 remain = 0;
	s32 rx_buf_pos = 0;
#endif

	func = rs_c_if_get_dev(c_if);
	if (c_if && func) {
		func->num = 1;

		sdio_claim_host(func);

#ifndef RRQ61000_BA_MULTI_BLOCK_RX
		temp_len += ALIGN_4BYTE(len);
		cnt = RS_SDIO_GET_CNT(temp_len, SDIO_BLOCK_SIZE);
		remain = RS_SDIO_GET_REMAIN(temp_len, SDIO_BLOCK_SIZE);

		if (cnt > SDIO_MAX_BLOCK_CNT) {
			RS_ERR("sdio read cnt[%d]:len[%d][%d]:rem[%d] !!!\n", cnt, len, temp_len, remain);
			err = 0;
			cnt = 0;
			remain = 0;
		}

		for (i = 0; i < cnt; i++) {
			err = sdio_readsb(func, (data + rx_buf_pos), i, SDIO_BLOCK_SIZE);
			if (err != 0) {
				RS_ERR("sdio_readsb err %d, %d, %d, %d %d %d !!!\n", err, i, rx_buf_pos, cnt,
				       temp_len, remain);
				break;
			}
			if (i == 0) {
				if (RS_C_IS_CMD(((struct rs_c_data *)data)->cmd)) {
					temp_len = RS_C_GET_DATA_SIZE(((struct rs_c_data *)data)->ext_len,
								      ((struct rs_c_data *)data)->data_len);
					temp_len += ALIGN_4BYTE(temp_len);

					cnt = RS_SDIO_GET_CNT(temp_len, SDIO_BLOCK_SIZE);
					if (cnt == 0) {
						remain = 0;
						// RS_DBG("[%d]:sdio read i[%d]:cnt[%d]:len[%d]:rem[%d] !!!\n",
						//        __LINE__, i, cnt, temp_len, remain);

					} else if (cnt > SDIO_MAX_BLOCK_CNT) {
						RS_ERR("sdio read i[%d]:cnt[%d]:len[%d]:rem[%d] !!!\n", i,
						       cnt, temp_len, remain);
						err = -1;
						cnt = 0;
						remain = 0;
						break;
					} else {
						remain = RS_SDIO_GET_REMAIN(temp_len, SDIO_BLOCK_SIZE);
						// RS_DBG("[%d]:cmd[%d]:sdio read i[%d]:cnt[%d]:len[%d][%d], rem[%d]\n",
						//        __LINE__, (((struct rs_c_data *)data)->cmd), i, cnt,
						//        RS_C_GET_DATA_SIZE(
						//	       ((struct rs_c_data *)data)->ext_len,
						//	       ((struct rs_c_data *)data)->data_len),
						//        temp_len, remain);
					}
				} else {
					// RS_DBG("skip invalid data !!![%d][%d]\n", cnt,
					//        (((struct rs_c_data *)data)->cmd));
					err = -1;
					cnt = 0;
					remain = 0;
					break; // skip invalid data
				}
			}
			rx_buf_pos += SDIO_BLOCK_SIZE;
		}
		if ((err == 0) && (remain > 0)) {
			// RS_DBG("[%d]:sdio read i[%d]:cnt[%d], rem[%d] !!!\n", __LINE__, i, cnt, remain);

			err = sdio_readsb(func, (data + rx_buf_pos), i, remain);
			rx_buf_pos += remain;
		}

		// RS_DBG("P:%s:err[%d]:c_if[%p]:dev[%p]:cnt[%d]:remain[%d]:i[%d]:rx_buf_pos[%d]\n", __func__,
		//        err, c_if, c_if->if_dev.dev, cnt, remain, i, rx_buf_pos);
#else
		temp_len += ALIGN_512BYTE(len); // 4 byte align

		err = sdio_readsb(func, data, 0, temp_len); // auto incre
		if (err != 0) {
			RS_ERR("sdio_readsb err %d !!!\n", err);
		}
		// RS_DBG("P:%s[%d]: 0x%x, 0x%x, 0x%x\n", __func__, __LINE__, addr, *(uint32_t *)data, temp_len);
#endif

		sdio_release_host(func);

		if (err == 0) {
			ret = RS_SUCCESS;
		}
	}

	return ret;
}

static rs_ret k_sdio_write(struct rs_c_if *c_if, u32 addr, u8 *data, u32 len)
{
	rs_ret ret = RS_FAIL;
	s32 err = 0;
	struct sdio_func *func = NULL;
	s32 temp_len = len;
#ifndef RRQ61000_BA_MULTI_BLOCK_TX
	s32 i = 0;
	u32 cnt = 0;
	u32 remain = 0;
	s32 tx_buf_pos = 0;
#endif

	func = rs_c_if_get_dev(c_if);
	if (c_if && func) {
		func->num = 1;

		sdio_claim_host(func);

#ifndef RRQ61000_BA_MULTI_BLOCK_TX
		temp_len += ALIGN_4BYTE(len); // 4 byte align
		cnt = RS_SDIO_GET_CNT(temp_len, SDIO_BLOCK_SIZE);
		remain = RS_SDIO_GET_REMAIN(temp_len, SDIO_BLOCK_SIZE);

		if (cnt > SDIO_MAX_BLOCK_CNT) {
			RS_ERR("sdio read cnt[%d]:len[%d][%d]:rem[%d] !!!\n", cnt, len, temp_len, remain);
			err = -1;
			cnt = 0;
			remain = 0;
		}

		for (i = 0; i < cnt; i++) {
			err = sdio_writesb(func, i, (data + tx_buf_pos), SDIO_BLOCK_SIZE);
			if (err != 0) {
				RS_ERR("sdio_writesb err %d, %d, %d, %d %d %d !!!\n", err, i, tx_buf_pos, cnt,
				       temp_len, remain);
				break;
			}

			tx_buf_pos += SDIO_BLOCK_SIZE;
		}
		if (err == 0 && remain > 0) {
			err = sdio_writesb(func, i, (data + tx_buf_pos), remain);
			tx_buf_pos += remain;
		}

		// RS_DBG("P:%s:err[%d]:c_if[%p]:dev[%p]:cnt[%d]:remain[%d]:i[%d]:tx_buf_pos[%d]\n", __func__,
		//        err, c_if, c_if->if_dev.dev, cnt, remain, i, tx_buf_pos);

#else
		temp_len += ALIGN_512BYTE(len); // 4 byte align

		if ((err = sdio_writesb(func, 0, data, temp_len)) == 0) { // auto incre
			ret = RS_SUCCESS;
		} else {
			RS_ERR("sdio_writesb err %d !!!\n", err);
		}
#endif

		sdio_release_host(func);

		if (err == 0) {
			ret = RS_SUCCESS;
		}
	}

	return ret;
}

static rs_ret k_sdio_read_status(struct rs_c_if *c_if, u8 *data, u32 len)
{
	rs_ret ret = RS_FAIL;
	s32 err = -1;
	struct sdio_func *func = NULL;

	func = rs_c_if_get_dev(c_if);
	if (c_if && func) {
		func->num = 1;

		sdio_claim_host(func);

		if (len > 0) {
			err = sdio_readsb(func, data, RS_C_IF_STATUS_CMD, len);
		}

		if (err != 0) {
			RS_ERR("sdio_readsb err [%d] !!!\n", err);
		} else {
			ret = RS_SUCCESS;
		}

		sdio_release_host(func);
	}

	return ret;
}

static rs_ret k_sdio_reload(struct rs_c_if *c_if)
{
	rs_ret ret = RS_FAIL;
	struct sdio_func *func = NULL;

	func = rs_c_if_get_dev(c_if);
	if (c_if && func) {
		func->card->host->rescan_disable = 0;
		mmc_detect_change(func->card->host, 100);

		ret = RS_SUCCESS;
	}

	return ret;
}

////////////////////////////////////////////////////////////////////////////////

static u32 local_crc32(const void *buf, u32 size)
{
	const u8 *p = buf;
	u32 crc;

	crc = ~0U;
	while (size--)
		crc = crc32_tab[(crc ^ *p++) & 0xFF] ^ (crc >> 8);
	return crc ^ ~0U;
}

static rs_ret k_sdio_fw_download(struct rs_c_if *c_if)
{
	rs_ret ret = RS_FAIL;
	s32 err = 0;

	struct sdio_func *func = NULL;
	const u8 *filename = SDIO_FMAC_FW_NAME;
	const struct firmware *lmac_fw_buff = NULL;
	u8 *fw_buff = NULL;
	u32 *wtemp = NULL;
	u32 img_crc = 0;
	u32 msg_data = 0;
	s32 i = 0;
#ifdef FW_DOWNLOAD_SPLIT
	s32 iteration = 0, remain_len = 0;
#endif

#ifdef FW_DOWNLOAD_ENABLE
	u16 blk_size;
	u8 reg;
#endif

	RS_INFO("F/W downloading...\n");

	func = rs_c_if_get_dev(c_if);

	if (c_if && func) {
		err = request_firmware(&lmac_fw_buff, filename, &func->dev);
		if (err == 0) {
			wtemp = rs_k_calloc(SDIO_RW_SIZE);
			ret = RS_SUCCESS;
		} else {
			RS_ERR("%s: Failed to get %s (%d)\n", __func__, filename, err);
		}
	}

	if (ret == RS_SUCCESS && lmac_fw_buff && wtemp) {
#ifdef FW_DOWNLOAD_SPLIT
		fw_buff = devm_kmalloc(&func->dev, SDIO_BLOCK_SIZE_FW, GFP_KERNEL);
#else
		fw_buff = devm_kmalloc(&func->dev, lmac_fw_buff->size + ALIGN_512BYTE(lmac_fw_buff->size),
				       GFP_KERNEL);
#endif

		msg_data = lmac_fw_buff->size;

		RS_DBG("fw size %d h:%08x\n", msg_data, msg_data);
		func->num = 1;
		sdio_claim_host(func);
		/* first send fw size */
		//void sdio_writel(struct sdio_func *func, u32 b, unsigned int addr, int *err_ret)
		sdio_writel(func, msg_data, SDIO_HOST_GP_REG, &err);
		if (err != 0) {
			RS_ERR("Size write NG\n");
			ret = RS_FAIL;
		}

		if (ret == RS_SUCCESS) {
			mdelay(100);

			/* Read back Ack */
			msg_data = 0;
			func->num = 1;
			/* u8 sdio_readb(struct sdio_func *func, unsigned int addr, int *err_ret) */
			/* u32 sdio_readl(struct sdio_func *func, unsigned int addr, int *err_ret) */

			RS_DBG("check ack\n");
			for (i = 0; i < 10; i++) {
				msg_data = sdio_readl(func, SDIO_CHIP_GP_REG, &err);
				if (err != 0) {
					ret = RS_FAIL;
					break;
				}
				// RS_DBG("ack from TIN %08x\r\n", msg_data);
				if (msg_data == 0x00022000) {
					RS_DBG("TIN Ack\r\n");
					break;
				}
				msleep(10);
			}

			if (i >= 10) {
				ret = RS_FAIL;
			}
		}

		if (ret == RS_SUCCESS) {
#ifdef FW_DOWNLOAD_SPLIT
			iteration = lmac_fw_buff->size / SDIO_BLOCK_SIZE_FW;
			remain_len = lmac_fw_buff->size % SDIO_BLOCK_SIZE_FW;

			RS_DBG("iteration %d, remain_len %d\n", iteration, remain_len);
			for (i = 0; i < iteration; i++) {
				memcpy(fw_buff, &lmac_fw_buff->data[i * SDIO_BLOCK_SIZE_FW],
				       SDIO_BLOCK_SIZE_FW);
				err = sdio_writesb(func, 0, fw_buff, SDIO_BLOCK_SIZE_FW);
				if (err != 0) {
					ret = RS_FAIL;
					break;
				}
				//				msleep(2);
			}

			if (ret == RS_SUCCESS) {
				memcpy(fw_buff, &lmac_fw_buff->data[i * SDIO_BLOCK_SIZE_FW], remain_len);
				sdio_writesb(func, 0, fw_buff, remain_len);
			}

#else
			memcpy(fw_buff, lmac_fw_buff->data, lmac_fw_buff->size);
			err = sdio_writesb(func, 0, fw_buff,
					   lmac_fw_buff->size + ALIGN_512BYTE(lmac_fw_buff->size));
			if (err != 0) {
				ret = RS_FAIL;
			}
#endif
		}

		if (ret == RS_SUCCESS) {
			msleep(100);

			// send CRC
			img_crc = local_crc32(lmac_fw_buff->data, lmac_fw_buff->size);
			RS_DBG("wifi FW CRC is %08x\n", img_crc);

			sdio_writel(func, img_crc, SDIO_HOST_GP_REG, &err);
			if (err != 0) {
				ret = RS_FAIL;
				RS_ERR("CRC32 write NG\n");
			}
		}

		if (ret == RS_SUCCESS) {
			msleep(10);
			// Check CRC
			for (i = 0; i < 20; i++) {
				msg_data = sdio_readl(func, SDIO_CHIP_GP_REG, &err);
				// RS_INFO("ack from TIN %08x\r\n", msg_data);
				if (msg_data == 0x00022000) {
					// RS_INFO("TIN boot success\r\n");
					break;
				} else if (msg_data == 0x20000002) {
					RS_INFO("TIN boot NG\r\n");
					ret = RS_FAIL;
					break;
				} else if (msg_data == 0xf0f0f0f0) {
					// RS_INFO("CRC checking... need to check again.\r\n");
					msleep(50);
					//					ret = RS_FAIL;
				}
			}
		}
	} else {
		ret = RS_MEMORY_FAIL;
	}

#ifdef FW_DOWNLOAD_ENABLE
	msleep(100);
	func->num = 0;
	//sdio_set_block_size(rs_core->bus.sdio, SDIO_BLOCK_SIZE);
	//wait sync
	for (i = 0; i < 20; i++) {
		reg = sdio_f0_readb(func, 0x110, &err);
		blk_size = reg;
		// RS_INFO("reg %02x\r\n", reg);
		reg = sdio_f0_readb(func, 0x111, &err);
		blk_size |= reg << 8;
		// RS_INFO("reg %02x\r\n", reg);
		// RS_INFO("blk size %04x\r\n", blk_size);
		msleep(100);
		if (blk_size == SDIO_BLOCK_SIZE) {
			// RS_INFO("sync Done\n");
			break;
		}
	}
#endif

	if (ret == RS_SUCCESS) {
		RS_INFO("F/W downloading Done.\n");
	} else {
		RS_INFO("F/W downloading Failed!!\n");
	}

	if (wtemp) {
		rs_k_free(wtemp);
	}
	if (lmac_fw_buff) {
		release_firmware(lmac_fw_buff);
	}
	sdio_release_host(func);
	if (fw_buff) {
		devm_kfree(&func->dev, fw_buff);
	}

	return ret;
}

static void k_sdio_host_reset(struct sdio_func *func)
{
	s32 err = 0;
	u32 commit = RS_SDIO_DEV_RESET_CMD;

	sdio_claim_host(func);
	sdio_writel(func, commit, SDIO_HOST_GP_REG, &err);
	sdio_release_host(func);
}

////////////////////////////////////////////////////////////////////////////////

static void k_sdio_rrq61000_remove(struct sdio_func *func)
{
	struct rs_c_if *c_if = sdio_get_drvdata(func);

	RS_DBG(RS_FN_ENTRY_STR_ ":c_if[0x%p]\n", __func__, c_if);

	if (c_if) {
		if (c_if->if_cb && c_if->if_cb->core_deinit_cb) {
			(void)(c_if->if_cb->core_deinit_cb)(c_if);
		}

		(void)sdio_claim_host(func);
		(void)sdio_set_drvdata(func, NULL);
		(void)k_sdio_disable_int(func);

		c_if->if_dev.vendor = 0;
		c_if->if_dev.device = 0;
		(void)rs_c_if_set_dev(c_if, NULL);
		(void)rs_c_if_set_dev_if(c_if, NULL);
		c_if->if_ops.read = NULL;
		c_if->if_ops.write = NULL;
		c_if->if_ops.read_status = NULL;
		c_if->if_ops.reload = NULL;

		if (c_if->if_dev.dev_if_priv != NULL) {
			C_IF_DEV_MUTEX_DEINIT(c_if);

			rs_k_free(c_if->if_dev.dev_if_priv);
			c_if->if_dev.dev_if_priv = NULL;
		}

		k_sdio_host_reset(func);

#ifdef FW_DOWNLOAD_ENABLE
		func->card->host->rescan_disable = 0;
#endif
		(void)sdio_disable_func(func);
		(void)sdio_release_host(func);

#ifdef FW_DOWNLOAD_ENABLE
		mmc_detect_change(func->card->host, 100);
#endif

		rs_k_free(c_if);
	}
}

static rs_ret k_sdio_rrq61000_probe(struct sdio_func *func, const struct sdio_device_id *id)
{
	rs_ret ret = RS_FAIL;
	struct rs_c_if *c_if = NULL;
	struct sdio_dev_if_priv *dev_if_priv = NULL;

	if ((func != NULL) && (id != NULL)) {
		c_if = rs_k_calloc(sizeof(struct rs_c_if));

		if (c_if) {
			dev_if_priv = rs_k_calloc(sizeof(struct sdio_dev_if_priv));
			if (dev_if_priv != NULL) {
				c_if->if_dev.dev_if_priv = dev_if_priv;
				C_IF_DEV_MUTEX_INIT(c_if);

				ret = RS_SUCCESS;
			} else {
				ret = RS_MEMORY_FAIL;
			}

			if (ret == RS_SUCCESS) {
				c_if->if_dev.vendor = id->vendor;
				c_if->if_dev.device = id->device;
				(void)rs_c_if_set_dev(c_if, (void *)func);
				(void)rs_c_if_set_dev_if(c_if, (void *)&func->dev);
				c_if->if_ops.read = k_sdio_read;
				c_if->if_ops.write = k_sdio_write;
				c_if->if_ops.read_status = k_sdio_read_status;
				c_if->if_ops.reload = k_sdio_reload;

				if ((id) && (id->driver_data != 0)) {
					c_if->if_cb = (struct rs_c_if_cb *)(id->driver_data);
				}
				RS_DBG("P:%s:v[0x%X]:d[0x%X]:dev[0x%p]:if_cb[0x%p]:id[0x%p]:drv_data[0x%X]\n",
				       __func__, c_if->if_dev.vendor, c_if->if_dev.device, c_if->if_dev.dev,
				       c_if->if_cb, id, id->driver_data);

				(void)sdio_set_drvdata(func, c_if);

				ret = k_sdio_init(c_if);
			}

#ifdef FW_DOWNLOAD_ENABLE
			if (ret == RS_SUCCESS) {
				sdio_claim_host(func);
				// rescan disable
				func->card->host->rescan_disable = 1;
				sdio_release_host(func);

				pr_info("sdio fw_download\n");
				ret = k_sdio_fw_download(c_if);
				mdelay(500);
			}
#endif

			if (ret == RS_SUCCESS) {
				ret = k_sdio_enable_int(c_if);
			}

			if (ret == RS_SUCCESS) {
				if ((c_if->if_cb) && (c_if->if_cb->core_init_cb)) {
					ret = c_if->if_cb->core_init_cb(c_if);
				} else {
					ret = RS_FAIL;
				}
			}
		}

		if ((c_if != NULL) && (ret != RS_SUCCESS)) {
			k_sdio_rrq61000_remove(func);
		}
	}

	return ret;
}

static void k_sdio_remove(struct sdio_func *func)
{
	struct rs_c_if *c_if = sdio_get_drvdata(func);

	RS_TRACE(RS_FN_ENTRY_STR_ ":c_if[0x%p]\n", __func__, c_if);

	if (c_if) {
		RS_DBG(RS_FN_ENTRY_STR_ ":v[0x%X]:d[0x%X]\n", __func__, c_if->if_dev.vendor,
		       c_if->if_dev.device);
		if ((c_if->if_dev.vendor == SDIO_VENDOR_ID) &&
		    (c_if->if_dev.device == SDIO_DEVICE_ID_RRQ61000)) {
			k_sdio_rrq61000_remove(func);
		}
	}
}

static s32 k_sdio_probe(struct sdio_func *func, const struct sdio_device_id *id)
{
	s32 ret = 0;

	RS_TRACE(RS_FN_ENTRY_STR_ ":v[0x%X]:d[0x%X]\n", __func__, id->vendor, id->device);

	if ((id->vendor == SDIO_VENDOR_ID) && (id->device == SDIO_DEVICE_ID_RRQ61000)) {
		ret = k_sdio_rrq61000_probe(func, id);
	}

	if (ret != RS_SUCCESS) {
		ret = -ENOMEM;
	}

	return ret;
}

#ifdef CONFIG_PM_SLEEP
static s32 k_sdio_rrq61000_suspend(struct device *device)
{
	struct sdio_func *func = dev_to_sdio_func(device);
	struct rs_c_if *c_if = sdio_get_drvdata(func);
	struct sdio_dev_if_priv *dev_if_priv = NULL;
	mmc_pm_flag_t flags;
	s32 ret = 0;

	RS_DBG(RS_FN_ENTRY_STR_ ":c_if[0x%p]\n", __func__, c_if);

	// net_priv = (struct rs_net_cfg80211_priv *)rs_c_if_get_net_priv(c_if);

	if (!c_if || !(c_if->if_dev.dev_if_priv)) {
		return -ENOSYS;
	}

	dev_if_priv = c_if->if_dev.dev_if_priv;
	dev_if_priv->suspend = true;

	flags = sdio_get_host_pm_caps(func);

	if (!(flags & MMC_PM_KEEP_POWER)) {
		dev_err(device, "%s: cannot remain alive while host is suspended\n", sdio_func_id(func));
		return -ENOSYS;
	}

	ret = sdio_set_host_pm_flags(func, MMC_PM_KEEP_POWER);
	if (ret)
		return ret;

	// TODO sdio_suspend

	if (ret)
		return ret;

	return sdio_set_host_pm_flags(func, MMC_PM_WAKE_SDIO_IRQ);
}

static s32 k_sdio_rrq61000_resume(struct device *device)
{
	struct sdio_func *func = dev_to_sdio_func(device);
	struct rs_c_if *c_if = sdio_get_drvdata(func);
	struct sdio_dev_if_priv *dev_if_priv = NULL;
	s32 ret = 0;

	if (!c_if || !(c_if->if_dev.dev_if_priv)) {
		return -ENOSYS;
	}

	RS_DBG(RS_FN_ENTRY_STR_ ":c_if[0x%p]\n", __func__, c_if);

	dev_if_priv = c_if->if_dev.dev_if_priv;
	dev_if_priv->suspend = false;

	// TODO : resume

	return ret;
}

static const struct dev_pm_ops sdio_rrq61000_pm_ops = {
	.suspend = k_sdio_rrq61000_suspend,
	.resume = k_sdio_rrq61000_resume,
};

#define K_SDIO_RRQ61000_PM_OPS (&sdio_rrq61000_pm_ops)

#else
#define K_SDIO_RRQ61000_PM_OPS NULL
#endif

////////////////////////////////////////////////////////////////////////////////
/// SDIO Module definition

static struct sdio_device_id sdio_dev_table[] = {
	{ SDIO_DEVICE(SDIO_VENDOR_ID, SDIO_DEVICE_ID_RRQ61000) },
	{},
};

MODULE_DEVICE_TABLE(sdio, sdio_dev_table);

static struct sdio_driver sdio_drv_table = { /* RRQ61004 driver */
					     .name = SDIO_DRIVER_NAME,
					     .id_table = &sdio_dev_table[SDIO_DEV_IDX_RRQ61000],
					     .probe = k_sdio_probe,
					     .remove = k_sdio_remove,
#ifdef CONFIG_PM_SLEEP
					     .drv.pm = K_SDIO_RRQ61000_PM_OPS
#endif

};

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL FUNCTION

rs_ret rs_k_sdio_init(rs_c_callback_t core_init_cb, rs_c_callback_t core_deinit_cb, rs_c_callback_t recv_cb)
{
	rs_ret ret = RS_FAIL;
	s32 err = 0;
	struct rs_c_if_cb *if_cb = NULL;

	RS_TRACE(RS_FN_ENTRY_STR);

	// RRQ61000
	if_cb = rs_k_calloc(sizeof(struct rs_c_if_cb));
	if (if_cb) {
		if_cb->core_init_cb = core_init_cb;
		if_cb->core_deinit_cb = core_deinit_cb;
		if_cb->recv_cb = recv_cb;

		sdio_dev_table[SDIO_DEV_IDX_RRQ61000].driver_data = (kernel_ulong_t)(if_cb);

		RS_DBG("P:%s:driver_data[0x%X]:if_cb[0x%p]\n", __func__,
		       sdio_dev_table[SDIO_DEV_IDX_RRQ61000].driver_data, if_cb);
	}

	err = sdio_register_driver(&sdio_drv_table);

	if (err == 0) {
		ret = RS_SUCCESS;
	}

	return ret;
}

rs_ret rs_k_sdio_deinit(struct rs_c_if *c_if)
{
	rs_ret ret = RS_SUCCESS;
	struct rs_c_if_cb *if_cb = NULL;

	RS_TRACE(RS_FN_ENTRY_STR);

	sdio_unregister_driver(&sdio_drv_table);

	// RRQ61000
	if_cb = (struct rs_c_if_cb *)(sdio_dev_table[SDIO_DEV_IDX_RRQ61000].driver_data);
	if (if_cb != 0) {
		RS_DBG("P:%s:driver_data[0x%X]:if_cb[0x%p]\n", __func__,
		       sdio_dev_table[SDIO_DEV_IDX_RRQ61000].driver_data, if_cb);

		sdio_dev_table[SDIO_DEV_IDX_RRQ61000].driver_data = 0;
		rs_k_free(if_cb);
	}

	return ret;
}