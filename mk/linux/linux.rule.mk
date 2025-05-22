
#######################################################################
# Rules

CONFIG_RSWLAN ?= m

ccflags-y := -I$(SRC_DIR)
ccflags-y += $(DEBUG_CFLAGS)

ifeq ($(CONFIG_RSWLAN_COMBINE),y)

ifeq ($(CONFIG_RSWLAN_SPI),)
$(MODULE_NAME)-y := $(DRV_OBJS) $(DRV_SDIO_OBJS)
else
$(MODULE_NAME)-y := $(DRV_OBJS) $(DRV_SPI_OBJS)
endif

obj-$(CONFIG_RSWLAN) += $(MODULE_NAME).o
else
CONFIG_RSWLAN_SDIO ?= m
CONFIG_RSWLAN_SPI ?= m

$(MODULE_NAME)-y := $(DRV_OBJS)
obj-$(CONFIG_RSWLAN) += $(MODULE_NAME).o
obj-$(CONFIG_RSWLAN_SDIO) += $(SDIO_MODULE_NAME).o
obj-$(CONFIG_RSWLAN_SPI) += $(SPI_MODULE_NAME).o

$(SDIO_MODULE_NAME)-objs := $(DRV_SDIO_OBJS)
$(SPI_MODULE_NAME)-objs := $(DRV_SPI_OBJS)
endif

all: version_gen.h
	$(MKDIR) -p $(BIN_DIR)

	$(MAKE) -C $(KDIR) M=$(BUILD_DIR) modules
	@$(MV) $(MODULE_NAME).ko $(BIN_DIR)/ 2>/dev/null; true
	@$(MV) $(SDIO_MODULE_NAME).ko $(BIN_DIR)/ 2>/dev/null; true
	@$(MV) $(SPI_MODULE_NAME).ko $(BIN_DIR)/ 2>/dev/null; true

modules clean:
	$(MAKE) -C $(KDIR) M=$(BUILD_DIR) clean
	@$(RM) -f src/version_gen.h

# make version_gen.h includes version info and compile time
version_gen.h:
	@echo "#define VERSION \"$(CONFIG_RSWLAN_CHIP_TYPE) version: v$(DRV_VER)\"" > $(SRC_DIR)/version_gen.h
	@echo "#define COMPILE_TIME \" - build: $(shell date '+%m-%d-%Y %H:%M:%S')\"" >> $(SRC_DIR)/version_gen.h
