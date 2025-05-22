
ifeq ($(KDIR),)
#Error KDIR is not defined
endif

#######################################################################
# Tools

CAT ?= cat
CP ?= cp
FIND ?= find
MKDIR ?= mkdir
RM ?= rm
STRIP ?= strip
MAKE ?= make
MV ?= mv

#######################################################################
# Configuration

CONFIG_RSWLAN_COMBINE ?= y
ifeq ($(CONFIG_RSWLAN_COMBINE),y)
EXTRA_CFLAGS += -DCONFIG_RS_COMBINE_DRIVER
endif

CONFIG_RSWLAN_MAC_TYPE ?= fmac
ifeq ($(CONFIG_RSWLAN_MAC_TYPE),fmac)
EXTRA_CFLAGS += -DCONFIG_RS_FULLMAC
endif

CONFIG_RSWLAN_SUPPORT_5G ?= y
ifeq ($(CONFIG_RSWLAN_SUPPORT_5G),y)
EXTRA_CFLAGS += -DCONFIG_RS_SUPPORT_5G
CONFIG_RSWLAN_5G_DFS ?= y
EXTRA_CFLAGS += -DCONFIG_RS_5G_DFS
endif

# Support of P2P DebugFS for enabling/disabling NoA and OppPS
CONFIG_RSWLAN_P2P_DEBUGFS ?= n
ifeq ($(CONFIG_RSWLAN_P2P_DEBUGFS),y)
EXTRA_CFLAGS += -DCONFIG_RS_P2P_DEBUGFS
endif

# extra DEBUG config
CONFIG_RSWLAN_DEBUG_BUILD ?= n
ifeq ($(CONFIG_RSWLAN_DEBUG_BUILD),y)
EXTRA_CFLAGS += -DCONFIG_RS_DBG
DEBUG_CFLAGS := -g
endif

CONFIG_RSWLAN_CHIP_REVISION ?= AA
ifeq ($(CONFIG_RSWLAN_CHIP_REVISION),AA)
EXTRA_CFLAGS += -DDEVICE_VER_AA
endif

ifeq ($(CONFIG_RSWLAN_CHIP_REVISION),BA)
EXTRA_CFLAGS += -DDEVICE_VER_BA
endif

CONFIG_RSWLAN_DBG_STATS ?= y
ifeq ($(CONFIG_RSWLAN_DBG_STATS), y)
EXTRA_CFLAGS += -DCONFIG_DBG_STATS
endif
