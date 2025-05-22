// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Copyright (C) [2022-2025] Renesas Electronics Corporation and/or its
 * affiliates.
 */

////////////////////////////////////////////////////////////////////////////////
/// INCLUDE

#include <linux/version.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/spi/spi.h>
#include <linux/firmware.h>
#include <linux/delay.h>
#include <linux/of_gpio.h>
#include <linux/interrupt.h>

#include "rs_type.h"
#include "rs_k_mem.h"
#include "rs_k_mutex.h"
#include "rs_c_dbg.h"
#include "rs_c_cmd.h"
#include "rs_c_data.h"
#include "rs_c_if.h"
#include "rs_k_spi.h"
#include "rs_k_if.h"
#include "rs_core.h"

////////////////////////////////////////////////////////////////////////////////
/// MACRO DEFINITION

#define FW_DOWNLOAD_ENABLE

// SPI_NAME_SIZE
#define SPI_DRIVER_NAME		  "rswlan_spi"
#define SPI_VENDOR_ID		  (0x5245)
#define SPI_DEVICE_ID_RRQ61000	  (0x4154)
#define SPI_DEVICE_COMPATIBLE	  "renesas"
#define SPI_DEVICE_NAME_RRQ61000  "rrq61000"
#define SPI_DEV_IDX_RRQ61000	  (0)

// SPI fullmac firmware name
#define SPI_FMAC_FW_NAME	  RS_FMAC_FW_BASE_NAME "_spi.bin"

#define RRQ61000_FW_PROTOCOL_SIZE 12 // preamble[2], length[4], image_crc[4], spi_mode[1] , crc[1] = 12 byte
#define RRQ61000_FW_RECEIVE_SIZE  4

#define C_IF_DEV_MUTEX_INIT(c_if) \
	(void)rs_k_mutex_create(&(((struct spi_dev_if_priv *)((c_if)->if_dev.dev_if_priv))->mutex))
#define C_IF_DEV_MUTEX_DEINIT(c_if) \
	(void)rs_k_mutex_destroy(&(((struct spi_dev_if_priv *)((c_if)->if_dev.dev_if_priv))->mutex))
#define C_IF_DEV_MUTEX_LOCK(c_if) \
	(void)rs_k_mutex_lock(&(((struct spi_dev_if_priv *)((c_if)->if_dev.dev_if_priv))->mutex))
#define C_IF_DEV_MUTEX_UNLOCK(c_if) \
	(void)rs_k_mutex_unlock(&(((struct spi_dev_if_priv *)((c_if)->if_dev.dev_if_priv))->mutex))

////////////////////////////////////////////////////////////////////////////////
/// TYPE DEFINITION

#define SPI_HEADER_SIZE 8
#define USE_GPIO_RESET	1
#define USE_GPIO_STATE	1

struct spi_dev_if_priv {
	struct rs_k_mutex mutex;

	u32 buff_len;
	u8 *rx_buff;
	u8 *tx_buff;

	s32 gpio_reset;
	s32 gpio_irq;
	s32 gpio_irq_nb;
#ifdef 	USE_GPIO_STATE
	s32 gpio_irq_state;
	s32 gpio_irq_state_nb;
#endif	
};

struct k_spi_header {
	u32 cmd : 16;
	u32 len : 16;
};

////////////////////////////////////////////////////////////////////////////////
/// LOCAL VARIABLE
#if USE_GPIO_STATE
static unsigned int	state_change_flag = 1;
#endif
// static unsigned long prev_xfer_time; // = jiffies;

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

static void k_spi_recv_handler(struct spi_device *spi_dev)
{
	struct rs_c_if *c_if = spi_get_drvdata(spi_dev);

	if (c_if && c_if->if_cb && c_if->if_cb->recv_cb) {
		(void)c_if->if_cb->recv_cb(c_if);
	}
}

static rs_ret k_spi_enable_int(struct rs_c_if *c_if)
{
	rs_ret ret = RS_SUCCESS;

	return ret;
}

static rs_ret k_spi_disable_int(struct spi_device *spi_dev)
{
	s32 ret = RS_SUCCESS;

	return ret;
}

irqreturn_t rs_irq_handler(s32 irq, void *dev_id)
{
	struct rs_c_if *c_if = (struct rs_c_if *)dev_id;
	struct spi_device *spi_dev = rs_c_if_get_dev(c_if);
	RS_DBG(RS_FN_ENTRY_STR_ ":c_if[0x%p]\n", __func__, c_if);

	k_spi_recv_handler(spi_dev);

	return IRQ_HANDLED;
}

#if USE_GPIO_STATE
irqreturn_t rs_irq_handler_state(s32 irq, void *dev_id)
{
	state_change_flag = 1;

	return IRQ_HANDLED;
}

static void k_spi_reset_state_change(void)
{
	state_change_flag = 0;
}

static rs_ret k_spi_wait_state_change(u32 timeout_us)
{
	rs_ret ret = RS_FAIL;

	for(int i = 0; i < timeout_us / 10; i++) {
		if(state_change_flag) {
			ret = RS_SUCCESS;
			break;
		}
		udelay(10);
	}

	return ret;
}
#endif // #if USE_GPIO_STATE

#define GPIO22 593
#define GPIO23 594
#define GPIO24 595
static rs_ret k_spi_init(struct rs_c_if *c_if)
{
	rs_ret ret = RS_SUCCESS;
	struct spi_dev_if_priv *dev_if_priv = NULL;
	struct spi_device *spi_dev = rs_c_if_get_dev(c_if);
	struct device_node *np = NULL;
	s32 temp_gpio_reset = -1;
	s32 temp_gpio_irq = -1;
	s32 temp_gpio_irq_nb = -1;
	s32 temp_gpio_irq_state = -1;
	s32 temp_gpio_irq_state_nb = -1;

	if ((c_if != NULL) && (spi_dev != NULL)) {
		np = spi_dev->dev.of_node;
		dev_if_priv = c_if->if_dev.dev_if_priv;

		if ((np == NULL) || (dev_if_priv == NULL)) {
			ret = RS_FAIL;
		}
	}

	if (ret == RS_SUCCESS) {
		// for RRQ61000 booting
		spi_dev->max_speed_hz = 1000000; /* The firmware download 4MHz did not work on the RPI-5 board */
		spi_dev->bits_per_word = 8;

		ret = spi_setup(spi_dev);
		if (!ret) {
			dev_info(&spi_dev->dev, "CS %d Mode %d %dMhz, %dbit \n", spi_dev->chip_select,
				 spi_dev->mode, spi_dev->max_speed_hz / 1000000, spi_dev->bits_per_word);
		} else {
			dev_info(&spi_dev->dev, "Failed spi_setup, ret 0x%x \n", ret);
			ret = RS_FAIL;
		}
	}
#if 0 // RPI5 not yet support named gpio 
	if (ret == RS_SUCCESS) {
		temp_gpio_reset = of_get_named_gpio(np, "irq-gpios", 0);
		if (gpio_is_valid(temp_gpio_reset)) {
			temp_gpio_irq = temp_gpio_reset;
			dev_info(&spi_dev->dev, "GPIO%d(irq-gpios): Valid check success\n", temp_gpio_reset);
		} else {
			dev_err(&spi_dev->dev, "GPIO%d(irq-gpios): Valid check failure\n", temp_gpio_reset);

			ret = RS_FAIL;
		}
	}

	if (ret == RS_SUCCESS) {
#if USE_GPIO_RESET
		temp_gpio_reset = of_get_named_gpio(np, "reset-gpios", 0);
		if (gpio_is_valid(temp_gpio_reset)) {
			dev_info(&spi_dev->dev, "GPIO%d(reset-gpios): Valid check success\n",
				 temp_gpio_reset);
		} else {
			dev_err(&spi_dev->dev, "GPIO%d(reset-gpios): Valid check failure\n", temp_gpio_reset);

			ret = RS_FAIL;
		}
#endif //  USE_GPIO_RESET
	}
#endif // #if 0

	if (ret == RS_SUCCESS) {
		if(temp_gpio_irq < 0) {
			temp_gpio_irq = GPIO24;	
		}
		ret = gpio_request(temp_gpio_irq, "irq-gpios");
		if (ret != 0) {
			dev_err(&spi_dev->dev, "gpio_request irq-gpios err %d\n", ret);
			ret = RS_FAIL;
		}
		ret = gpio_direction_input(temp_gpio_irq);
		if (ret != 0) {
			dev_err(&spi_dev->dev, "gpio_direction_input irq-gpios err %d\n", ret);
			ret = RS_FAIL;
		}
	}

	if (ret == RS_SUCCESS) {
#if USE_GPIO_RESET
		if(temp_gpio_reset < 0) {
			temp_gpio_reset = GPIO23;
		}
		ret = gpio_request(temp_gpio_reset, "reset-gpios");
		if (ret != 0) {
			dev_err(&spi_dev->dev, "gpio_request reset-gpios err %d\n", ret);
			ret = RS_FAIL;
		}
		ret = gpio_direction_output(temp_gpio_reset, 1);
		if (ret != 0) {
			dev_err(&spi_dev->dev, "gpio_direction_output reset-gpios err %d\n", ret);
			ret = RS_FAIL;
		}
#endif
	}

	if (ret == RS_SUCCESS) {
		if (gpio_get_value(temp_gpio_irq) == 1)
			pr_info("gpio 1\n");
		else if (gpio_get_value(temp_gpio_irq) == 0)
			pr_info("gpio 0\n");
		else
			pr_info("somthing wrong\n");

		// active gpio interrupt after FW downloading
		temp_gpio_irq_nb = gpio_to_irq(temp_gpio_irq);
		pr_info("GPIO IRQ number = %d\n", temp_gpio_irq_nb);
		ret = request_irq(temp_gpio_irq_nb, rs_irq_handler, IRQF_TRIGGER_RISING, "rswlan_irq", c_if);
		if (ret != 0) {
			dev_err(&spi_dev->dev, "request_irq err %d\n", ret);
			ret = RS_FAIL;
		}
	}

#if USE_GPIO_STATE
		if (ret == RS_SUCCESS) {
			if(temp_gpio_irq_state < 0) {
				temp_gpio_irq_state = GPIO22;	
			}
			ret = gpio_request(temp_gpio_irq_state, "irq-gpios-state");
			if (ret != 0) {
				dev_err(&spi_dev->dev, "gpio_request irq-gpios-state err %d\n", ret);
				ret = RS_FAIL;
			}
			ret = gpio_direction_input(temp_gpio_irq_state);
			if (ret != 0) {
				dev_err(&spi_dev->dev, "gpio_direction_input irq-gpios err %d\n", ret);
				ret = RS_FAIL;
			}
		}

		// active gpio interrupt after FW downloading
		temp_gpio_irq_state_nb = gpio_to_irq(temp_gpio_irq_state);
		pr_info("GPIO IRQ STATE number = %d\n", temp_gpio_irq_state_nb);
		ret = request_irq(temp_gpio_irq_state_nb, rs_irq_handler_state, IRQF_TRIGGER_RISING, "rswlan_irq_state", c_if);
		if (ret != 0) {
			dev_err(&spi_dev->dev, "request_irq_state err %d\n", ret);
			ret = RS_FAIL;
		}
#endif // #if USE_GPIO_STATE

	if (ret == RS_SUCCESS) {
		/* Reset Slave board */
		gpio_set_value(temp_gpio_reset, 0);
		mdelay(1);
		gpio_set_value(temp_gpio_reset, 1);
		mdelay(10);

		dev_if_priv->gpio_reset = temp_gpio_reset;
		dev_if_priv->gpio_irq = temp_gpio_irq;
		dev_if_priv->gpio_irq_nb = temp_gpio_irq_nb;
#if USE_GPIO_STATE
		dev_if_priv->gpio_irq_state = temp_gpio_irq_state;
		dev_if_priv->gpio_irq_state_nb = temp_gpio_irq_state_nb;
#endif
	} else {
		dev_if_priv->gpio_reset = -1;
		dev_if_priv->gpio_irq = -1;
		dev_if_priv->gpio_irq_nb = -1;
#if USE_GPIO_STATE
		dev_if_priv->gpio_irq_state = -1;
		dev_if_priv->gpio_irq_state_nb = -1;
#endif
	}

	return ret;
}

static inline s32 firmware_write(struct rs_c_if *c_if, const u8 *buf, s32 len)
{
	s32 err = 0;
	struct spi_message m;
	struct spi_transfer d;
	u8 *tx_buf = rs_k_calloc(len);

	struct spi_device *spi;

	spi_message_init(&m);
	rs_k_memset(&d, 0, sizeof(d));

	spi = rs_c_if_get_dev(c_if);

	rs_k_memcpy(tx_buf, buf, len);

	d.tx_buf = tx_buf;
	d.rx_buf = tx_buf;
	d.len = len;
	spi_message_add_tail(&d, &m);

	err = spi_sync(spi, &m);

	rs_k_free(tx_buf);

	return err;
}

static inline s32 firmware_read(struct rs_c_if *c_if, u8 *buf, s32 len)
{
	s32 err = 0;
	struct spi_message m;
	struct spi_transfer d;
	struct spi_device *spi;

	u8 *rx_buf = rs_k_calloc(len);

	spi_message_init(&m);
	rs_k_memset(&d, 0, sizeof(d));
	rs_k_memset(rx_buf, 0x0, len);

	spi = rs_c_if_get_dev(c_if);

	d.tx_buf = rx_buf;
	d.rx_buf = rx_buf;
	d.len = len;

	spi_message_add_tail(&d, &m);

	err = spi_sync(spi, &m);

	if (!err && buf) {
		rs_k_memcpy(buf, d.rx_buf, len);
	}

	rs_k_free(rx_buf);

	return err;
}

static inline s32 bus_write(struct rs_c_if *c_if, u8 *buf, s32 len)
{
	s32 err = -1;
	struct spi_device *spi = NULL;
	struct spi_message msg = { 0 };
	struct spi_transfer data_tr = { 0 };
	struct spi_dev_if_priv *dev_if_priv = NULL;
	char write_cmd[4] = {RS_CMD_DATA_TX, 0, 0, 0};

	*(unsigned short *)(&write_cmd[2]) = (unsigned short)len;

	spi = rs_c_if_get_dev(c_if);

	if (spi != NULL) {
		dev_if_priv = c_if->if_dev.dev_if_priv;

		if ((dev_if_priv != NULL) && (dev_if_priv->tx_buff != NULL) &&
		    (len <= dev_if_priv->buff_len)) {
				
			(void)rs_k_memcpy(dev_if_priv->tx_buff, write_cmd, 4);
			spi_message_init(&msg);

			data_tr.tx_buf = dev_if_priv->tx_buff;
			data_tr.rx_buf = NULL;
			data_tr.len = 4;

			spi_message_add_tail(&data_tr, &msg);

#if USE_GPIO_STATE
			k_spi_reset_state_change();
			err = spi_sync(spi, &msg);
			k_spi_wait_state_change(2000);
#else
			err = spi_sync(spi, &msg);
			udelay(200);
#endif
			(void)rs_k_memcpy(dev_if_priv->tx_buff, buf, len);
			len = (((len - 1) / 4) + 1) * 4;

			spi_message_init(&msg);

			data_tr.tx_buf = dev_if_priv->tx_buff;
			data_tr.rx_buf = NULL;
			data_tr.len = len;

			spi_message_add_tail(&data_tr, &msg);
#if USE_GPIO_STATE
			k_spi_reset_state_change();
			err = spi_sync(spi, &msg);
			k_spi_wait_state_change(2000);
#else
			err = spi_sync(spi, &msg);
#endif
		}
	}

	return err;
}

static inline s32 bus_read(struct rs_c_if *c_if, u8 *buf, s32 len)
{
	s32 err = -1;
	struct spi_device *spi = NULL;
	struct spi_message msg = { 0 };
	struct spi_transfer data_tr = { 0 };
	struct spi_dev_if_priv *dev_if_priv = NULL;
	char read_cmd[4] = {RS_CMD_DATA_RX, 0, 0, 0};

	spi = rs_c_if_get_dev(c_if);

	if (spi != NULL) {
		dev_if_priv = c_if->if_dev.dev_if_priv;

		if ((dev_if_priv != NULL) && (dev_if_priv->rx_buff != NULL) &&
		    (len <= dev_if_priv->buff_len)) {
								
			(void)rs_k_memcpy(dev_if_priv->tx_buff, read_cmd, 4);
			spi_message_init(&msg);

			data_tr.tx_buf = dev_if_priv->tx_buff;
			data_tr.rx_buf = NULL;
			data_tr.len = 4;

			spi_message_add_tail(&data_tr, &msg);

#if USE_GPIO_STATE
			k_spi_reset_state_change();
			err = spi_sync(spi, &msg);
			k_spi_wait_state_change(2000);
#else
			err = spi_sync(spi, &msg);
			udelay(200);
#endif
			spi_message_init(&msg);

			data_tr.tx_buf = NULL;
			data_tr.rx_buf = dev_if_priv->rx_buff;
			data_tr.len = dev_if_priv->buff_len;

			spi_message_add_tail(&data_tr, &msg);

#if USE_GPIO_STATE
			k_spi_reset_state_change();
			err = spi_sync(spi, &msg);
			k_spi_wait_state_change(2000);
#else
			err = spi_sync(spi, &msg);
#endif
			if (err == 0) {
				(void)rs_k_memcpy(buf, dev_if_priv->rx_buff, len);
			}
		}
	}

	return err;
}

static rs_ret k_spi_read_status(struct rs_c_if *c_if, u8 *data, u32 len)
{
	return RS_SUCCESS;
}

static rs_ret k_spi_reload(struct rs_c_if *c_if)
{
	rs_ret ret = RS_FAIL;

	return ret;
}

static rs_ret k_spi_read(struct rs_c_if *c_if, u32 addr, u8 *data, u32 len)
{
	rs_ret ret = RS_FAIL;	
#if USE_GPIO_STATE
	C_IF_DEV_MUTEX_LOCK(c_if);
	ret = bus_read(c_if, data, len);
	C_IF_DEV_MUTEX_UNLOCK(c_if);
#else
	udelay(500);
	C_IF_DEV_MUTEX_LOCK(c_if);
	ret = bus_read(c_if, data, len);
	C_IF_DEV_MUTEX_UNLOCK(c_if);
#endif

	RS_DBG("P:%s[%d]:r[%d]:addr 0x%x, data [%d][%02X %02X %02X %02X %02X %02X %02X %02X]\n",
	       __func__, __LINE__, ret, addr, len, data[0], data[1], data[2], data[3], data[4], data[5],
	       data[6], data[7]);

	return ret;
}

static rs_ret k_spi_write(struct rs_c_if *c_if, u32 addr, u8 *data, u32 len)
{
	rs_ret ret = RS_FAIL;
#if USE_GPIO_STATE
	C_IF_DEV_MUTEX_LOCK(c_if);
	ret = bus_write(c_if, data, len);
	C_IF_DEV_MUTEX_UNLOCK(c_if);
#else
	udelay(500);
	C_IF_DEV_MUTEX_LOCK(c_if);
	ret = bus_write(c_if, data, len);
	C_IF_DEV_MUTEX_UNLOCK(c_if);
#endif

	RS_DBG("P:%s[%d]:r[%d]:addr 0x%x, data [%d][%02X %02X %02X %02X %02X %02X %02X %02X]\n",
	       __func__, __LINE__, ret, addr, len, data[0], data[1], data[2], data[3], data[4], data[5],
	       data[6], data[7]);

	return ret;
}

////////////////////////////////////////////////////////////////////////////////

static u32 local_crc32(const void *buf, size_t size)
{
	const uint8_t *p = buf;
	u32 crc;

	crc = ~0U;
	while (size--)
		crc = crc32_tab[(crc ^ *p++) & 0xFF] ^ (crc >> 8);
	return crc ^ ~0U;
}

static rs_ret k_spi_fw_download(struct rs_c_if *c_if)
{
	rs_ret ret = RS_FAIL;
	s32 err = 0;
	int i;

	struct spi_device *spi_dev = NULL;
	const u8 *filename = SPI_FMAC_FW_NAME;
	const struct firmware *lmac_fw_buff = NULL;
	char preamble[RRQ61000_FW_PROTOCOL_SIZE] = { 0x70, 0x50, 0x00, 0x00, 0x00, 0x00,
						     0x00, 0x00, 0x00, 0x00, 0x08, 0x00 };
	u8 crc = 0xff;
	u32 image_crc32, fw_length, ack;

	RS_INFO("F/W downloading...\n");

	spi_dev = rs_c_if_get_dev(c_if);

	if (c_if && spi_dev) {
		err = request_firmware(&lmac_fw_buff, filename, &spi_dev->dev);
		if (err == 0) {
			RS_VERB("FW name %s, size %ld, First 4 bytes : %02x, %02x, %02x, %02x\n", filename,
				lmac_fw_buff->size, lmac_fw_buff->data[0], lmac_fw_buff->data[1],
				lmac_fw_buff->data[2], lmac_fw_buff->data[3]);
			ret = RS_SUCCESS;
		} else {
			RS_ERR("%s: Failed to get %s (%d)\n", __func__, filename, err);
			return ret;
		}
	}

	image_crc32 = local_crc32(lmac_fw_buff->data, lmac_fw_buff->size);
	rs_k_memcpy(&preamble[6], &image_crc32, sizeof(unsigned int));
	fw_length = lmac_fw_buff->size;
	rs_k_memcpy(&preamble[2], &fw_length, sizeof(unsigned int));

	for (i = 0; i < 11; i++) {
		crc ^= preamble[i];
	}
	preamble[11] = crc;

	/* send preamble */
	firmware_write(c_if, preamble, RRQ61000_FW_PROTOCOL_SIZE);
	udelay(100);

	/* get ack */
	firmware_read(c_if, (u8 *)&ack, RRQ61000_FW_RECEIVE_SIZE);
	RS_INFO("ack 0x%08x\n", ack);
	if (ack != 0x00022000)
		goto RRQ61000_FW_DOWNLOAD_NG;
	
	mdelay(4); // if rpi5 need delay 4ms

	/* send payload */
	firmware_write(c_if, (u8 *)lmac_fw_buff->data, lmac_fw_buff->size);
	udelay(500);

	/* get ack */
	for (i = 0; i < 10; i++) {
		firmware_read(c_if, (u8 *)&ack, RRQ61000_FW_RECEIVE_SIZE);
		RS_INFO("ack 0x%08x\n", ack);
		if (ack == 0xf0f0f0f0) {
			mdelay(2); /* RPI5 need to delay */
			continue;
		} else if (ack == 0x00022000) {
			break;
		} else {
			// NG
			goto RRQ61000_FW_DOWNLOAD_NG;
		}
	}

	RS_INFO("F/W downloading Done.\n");

	if (lmac_fw_buff) {
		release_firmware(lmac_fw_buff);
	}

	return ret;

RRQ61000_FW_DOWNLOAD_NG:
	ret = RS_FAIL;

	pr_info("FW downloading NG!.\n");
	release_firmware(lmac_fw_buff);
	return ret;
}

static void k_spi_host_reset(struct spi_device *spi_dev)
{
}

////////////////////////////////////////////////////////////////////////////////

static int k_spi_rrq61000_remove(struct spi_device *spi_dev)
{
	struct rs_c_if *c_if = spi_get_drvdata(spi_dev);
	struct spi_dev_if_priv *dev_if_priv = NULL;

	RS_TRACE(RS_FN_ENTRY_STR);

	if (c_if) {
		if (c_if->if_cb && c_if->if_cb->core_deinit_cb) {
			(void)(c_if->if_cb->core_deinit_cb)(c_if);
		}

		(void)k_spi_disable_int(spi_dev);

		k_spi_host_reset(spi_dev);

		(void)spi_set_drvdata(spi_dev, NULL);

		c_if->if_dev.vendor = 0;
		c_if->if_dev.device = 0;
		(void)rs_c_if_set_dev(c_if, NULL);
		(void)rs_c_if_set_dev_if(c_if, NULL);
		c_if->if_ops.read = NULL;
		c_if->if_ops.write = NULL;
		c_if->if_ops.read_status = NULL;
		c_if->if_ops.reload = NULL;

		if (c_if->if_dev.dev_if_priv != NULL) {
			dev_if_priv = c_if->if_dev.dev_if_priv;

			dev_if_priv->buff_len = 0;

			if (dev_if_priv->rx_buff) {
				rs_k_free(dev_if_priv->rx_buff);
				dev_if_priv->rx_buff = NULL;
			}

			if (dev_if_priv->tx_buff) {
				rs_k_free(dev_if_priv->tx_buff);
				dev_if_priv->tx_buff = NULL;
			}

			if (dev_if_priv->gpio_irq_nb >= 0) {
				free_irq(dev_if_priv->gpio_irq_nb, c_if);
			}

			if (dev_if_priv->gpio_irq >= 0) {
				/* release gpio */
				gpio_free(dev_if_priv->gpio_irq);
			}
#if USE_GPIO_STATE
			if (dev_if_priv->gpio_irq_state_nb >= 0) {
				free_irq(dev_if_priv->gpio_irq_state_nb, c_if);
			}
			if (dev_if_priv->gpio_irq_state >= 0) {
				/* Release gpio */
				gpio_free(dev_if_priv->gpio_irq_state);
			}
#endif
			if (dev_if_priv->gpio_reset >= 0) {
				gpio_free(dev_if_priv->gpio_reset);
			}

			C_IF_DEV_MUTEX_DEINIT(c_if);

			rs_k_free(dev_if_priv);
			c_if->if_dev.dev_if_priv = NULL;
		}

		rs_k_free(c_if);
	}

	return 0;
}

static rs_ret k_spi_rrq61000_probe(struct spi_device *spi_dev)
{
	rs_ret ret = RS_FAIL;
	struct rs_c_if *c_if = NULL;
	struct spi_dev_if_priv *dev_if_priv = NULL;
	const struct spi_device_id *id = NULL;

	RS_TRACE(RS_FN_ENTRY_STR);

	c_if = rs_k_calloc(sizeof(struct rs_c_if));
	id = spi_get_device_id(spi_dev);

	if ((c_if != NULL) && (id != NULL)) {
		dev_if_priv = rs_k_calloc(sizeof(struct spi_dev_if_priv));
		if (dev_if_priv != NULL) {
			dev_if_priv->buff_len = sizeof(struct rs_c_rx_data);
			dev_if_priv->rx_buff = rs_k_calloc(dev_if_priv->buff_len);
			dev_if_priv->tx_buff = rs_k_calloc(dev_if_priv->buff_len);

			if ((dev_if_priv->rx_buff != NULL) && (dev_if_priv->tx_buff != NULL)) {
				c_if->if_dev.dev_if_priv = dev_if_priv;
				C_IF_DEV_MUTEX_INIT(c_if);

				ret = RS_SUCCESS;
			} else {
				ret = RS_MEMORY_FAIL;
			}

		} else {
			ret = RS_MEMORY_FAIL;
		}

		if (ret == RS_SUCCESS) {
			c_if->if_dev.vendor = SPI_VENDOR_ID;
			c_if->if_dev.device = SPI_DEVICE_ID_RRQ61000;
			(void)rs_c_if_set_dev(c_if, (void *)spi_dev);
			(void)rs_c_if_set_dev_if(c_if, (void *)&spi_dev->dev);
			c_if->if_ops.read = k_spi_read;
			c_if->if_ops.write = k_spi_write;
			c_if->if_ops.read_status = k_spi_read_status;
			c_if->if_ops.reload = k_spi_reload;

			if (id->driver_data != 0) {
				c_if->if_cb = (struct rs_c_if_cb *)(id->driver_data);
			}
			RS_INFO("P:%s:v[0x%X]:d[0x%X]:dev[0x%p]:if_cb[0x%p]:id[0x%p]:drv_data[0x%X]\n",
				__func__, c_if->if_dev.vendor, c_if->if_dev.device, c_if->if_dev.dev,
				c_if->if_cb, id, id->driver_data);

			(void)spi_set_drvdata(spi_dev, c_if);

			ret = k_spi_init(c_if);
		}

#ifdef FW_DOWNLOAD_ENABLE
		pr_info("spi fw_download\n");
		ret = k_spi_fw_download(c_if);
#else
		pr_info("spi fw_download skip.\n");
#endif

		if (ret == RS_SUCCESS) {
			/* change SPI setting to 32bit mode */
			spi_dev->bits_per_word = 32;
			spi_dev->max_speed_hz = 25000000; /* RPI-5 support 25MHz SPI clock for interface with host. */
			mdelay(100);
			spi_finalize_current_transfer(spi_dev->controller);
			ret = spi_setup(spi_dev);
			if (!ret) {
				dev_info(&spi_dev->dev,
					 "New setting for SPI device to CS %d Mode %d %dMhz, %dbit \n",
					 spi_dev->chip_select, spi_dev->mode, spi_dev->max_speed_hz / 1000000,
					 spi_dev->bits_per_word);
				mdelay(100);
			}

			if (ret == RS_SUCCESS) {
				ret = k_spi_enable_int(c_if);
			}

			if (ret == RS_SUCCESS) {
				if ((c_if->if_cb) && (c_if->if_cb->core_init_cb)) {
					if (c_if->if_cb->core_init_cb(c_if) != RS_SUCCESS) {
						ret = RS_FAIL;
					}
				}
			}
		}
	}

	if ((c_if != NULL) && (ret != RS_SUCCESS)) {
		(void)k_spi_rrq61000_remove(spi_dev);
	}

	return ret;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 18, 0)
static void k_spi_remove(struct spi_device *spi_dev)
#else
static int k_spi_remove(struct spi_device *spi_dev)
#endif
{
	struct rs_c_if *c_if = NULL;
	const struct spi_device_id *id = NULL;

	RS_TRACE(RS_FN_ENTRY_STR);

	if (spi_dev != NULL) {
		RS_INFO(RS_FN_ENTRY_STR_ ":id[%p]\n", __func__, id);

		c_if = spi_get_drvdata(spi_dev);
		id = spi_get_device_id(spi_dev);
	}

	if ((c_if != NULL) && (id != NULL)) {
		RS_INFO(RS_FN_ENTRY_STR_ ":dev[%s]\n", __func__, id->name);

		if (strcmp(id->name, SPI_DEVICE_NAME_RRQ61000) == 0) {
			(void)k_spi_rrq61000_remove(spi_dev);
		} else {
			RS_INFO("remove failed : unknown spi dev[%s]\n", id->name);
		}
	} else {
		RS_INFO(RS_FN_ENTRY_STR_ ":id[%p]\n", __func__, id);
	}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 18, 0)
	return 0;
#endif
}

static s32 k_spi_probe(struct spi_device *spi_dev)
{
	s32 ret = RS_FAIL;
	const struct spi_device_id *id = NULL;

	RS_TRACE(RS_FN_ENTRY_STR);

	id = spi_get_device_id(spi_dev);

	if (id != NULL) {
		RS_INFO(RS_FN_ENTRY_STR_ ":dev[%s]\n", __func__, id->name);

		if (strcmp(id->name, SPI_DEVICE_NAME_RRQ61000) == 0) {
			ret = k_spi_rrq61000_probe(spi_dev);
		} else {
			RS_INFO("probe failed : unknown spi dev[%s]\n", id->name);
		}
	} else {
		RS_INFO(RS_FN_ENTRY_STR_ ":id[%p]\n", __func__, id);
	}

	RS_INFO("probe ret[%d]\n", ret);
	if (ret != RS_SUCCESS) {
		ret = -ENODEV;
	}

	return ret;
}

////////////////////////////////////////////////////////////////////////////////
/// SPI Module definition

static struct spi_device_id spi_dev_table[] = { { SPI_DEVICE_NAME_RRQ61000, 0 }, {} };

MODULE_DEVICE_TABLE(spi, spi_dev_table);

static const struct of_device_id of_spi_match[] = { {
							    .compatible = SPI_DEVICE_COMPATIBLE
							    "," SPI_DEVICE_NAME_RRQ61000,
						    },
						    {} };

MODULE_DEVICE_TABLE(of, of_spi_match);

static struct spi_driver spi_drv_table = { .id_table = spi_dev_table,
					   .probe = k_spi_probe,
					   .remove = k_spi_remove,
					   // .shutdown =
					   .driver = {
						   .name = SPI_DRIVER_NAME,
						   .of_match_table = of_match_ptr(of_spi_match),
						   // .pm =
					   } };

////////////////////////////////////////////////////////////////////////////////
/// GLOBAL FUNCTION

rs_ret rs_k_spi_init(rs_c_callback_t core_init_cb, rs_c_callback_t core_deinit_cb, rs_c_callback_t recv_cb)
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

		spi_dev_table[SPI_DEV_IDX_RRQ61000].driver_data = (kernel_ulong_t)(if_cb);

		RS_INFO("P:%s:driver_data[0x%X]:if_cb[0x%p]\n", __func__,
			spi_dev_table[SPI_DEV_IDX_RRQ61000].driver_data, if_cb);

		err = spi_register_driver(&spi_drv_table);
	}

	if (err == 0) {
		ret = RS_SUCCESS;
	}

	return ret;
}

rs_ret rs_k_spi_deinit(struct rs_c_if *c_if)
{
	rs_ret ret = RS_SUCCESS;
	struct rs_c_if_cb *if_cb = NULL;

	RS_TRACE(RS_FN_ENTRY_STR);

	// RRQ61000
	if_cb = (struct rs_c_if_cb *)(spi_dev_table[SPI_DEV_IDX_RRQ61000].driver_data);
	RS_INFO("P:%s[%d]:if_cb[%p]\n", __func__, __LINE__, if_cb);

	if (if_cb != 0) {
		spi_unregister_driver(&spi_drv_table);
		spi_dev_table[SPI_DEV_IDX_RRQ61000].driver_data = 0;
		rs_k_free(if_cb);
	}

	return ret;
}
