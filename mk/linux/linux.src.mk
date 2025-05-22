
#######################################################################
# Linux Source files

MODULE_NAME := rswlan
SDIO_MODULE_NAME := $(MODULE_NAME)_sdio
SPI_MODULE_NAME := $(MODULE_NAME)_spi

KAL_OS_SDIO_SRCS := rs_k_sdio.c
KAL_OS_SPI_SRCS := rs_k_spi.c
KAL_OS_SRCS :=

NET_OS_SRCS := rs_net.c \
		rs_net_cfg80211.c \
		rs_net_dbgfs.c \
		rs_net_dev.c \
		rs_net_skb.c \
		rs_net_priv.c \
		rs_net_params.c \
		rs_net_ctrl.c \
		rs_net_rx_indi.c \
		rs_net_rx_data.c \
		rs_net_tx_data.c	\
		rs_net_testmode.c	\
		rs_net_stats.c

ifeq ($(CONFIG_RSWLAN_5G_DFS),y)
NET_OS_SRCS += rs_net_dfs.c
endif

MAIN_OS_SDIO_SRCS := rs_main_sdio.c
MAIN_OS_SPI_SRCS := rs_main_spi.c
MAIN_OS_SRCS := rs_main.c

OS_SDIO_SRCS := $(KAL_OS_SDIO_SRCS) \
		$(MAIN_OS_SDIO_SRCS)

OS_SPI_SRCS := $(KAL_OS_SPI_SRCS) \
		$(MAIN_OS_SPI_SRCS)

OS_SRCS := $(KAL_OS_SRCS) \
		$(NET_OS_SRCS) \
		$(MAIN_OS_SRCS)