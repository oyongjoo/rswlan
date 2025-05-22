
# Driver Source
DRV_SDIO_SRCS := $(OS_SDIO_SRCS)
DRV_SPI_SRCS := $(OS_SPI_SRCS)
DRV_SRCS := $(OS_SRCS) \
		rs_k_dbg.c \
		rs_k_event.c \
		rs_k_mem.c \
		rs_k_mutex.c \
		rs_k_if.c \
		rs_k_spin_lock.c \
		rs_k_thread.c \
		rs_k_workqueue.c \
		rs_c_if.c \
		rs_core.c \
		rs_c_q.c \
		rs_c_ctrl.c \
		rs_c_indi.c \
		rs_c_rx.c \
		rs_c_tx.c \
		rs_c_status.c \
		rs_c_recovery.c

DRV_SDIO_SRCS := $(addprefix $(SRC_DIR)/,$(DRV_SDIO_SRCS))
DRV_SPI_SRCS := $(addprefix $(SRC_DIR)/,$(DRV_SPI_SRCS))
DRV_SRCS := $(addprefix $(SRC_DIR)/,$(DRV_SRCS))

# Objests
DRV_SDIO_OBJS := $(patsubst %.c,%.o,$(DRV_SDIO_SRCS))
DRV_SPI_OBJS := $(patsubst %.c,%.o,$(DRV_SPI_SRCS))
DRV_OBJS := $(patsubst %.c,%.o,$(DRV_SRCS))
