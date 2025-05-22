# SPDX-License-Identifier: GPL-2.0-or-later
#
# Copyright (C) [2022-2025] Renesas Electronics Corporation and/or its
# affiliates.
#

#######################################################################
# Makefile.$(OS) : Makefile.linux
# 1. license.hdr
# 2. dir.mk
# 3. linux.cfg.mk
# 4. linux.src.mk
# 5. src.mk
# 6. linux.rules.mk

#######################################################################
# OS definition

OS ?= linux

OS_MAKEFILE_CFG = $(OS).cfg.mk
OS_MAKEFILE_SRC = $(OS).src.mk
OS_MAKEFILE_RULE = $(OS).rule.mk
OS_BUILD_SH = *.sh

# Tools
CAT ?= cat
CD ?= cd
CHMOD ?= chmod
CP ?= cp
MKDIR ?= mkdir
RM ?= rm
STRIP ?= strip

#######################################################################
# definition

# suffix
SUFFIX_C := .c
SUFFIX_H := .h

# Path
ROOT_DIR ?= $(PWD)

OS_DIR ?= $(OS)

BUILD_DIR := $(ROOT_DIR)/build
BUILD_OS_DIR := $(BUILD_DIR)/$(OS_DIR)
BUILD_BIN_DIR := $(BUILD_OS_DIR)/bin
BUILD_OBJ_DIR := $(BUILD_OS_DIR)/obj
BUILD_SRC_DIR := $(BUILD_OS_DIR)/src

KAL_DIR := $(ROOT_DIR)/kal
KAL_INC_DIR := $(KAL_DIR)/inc
KAL_SRC_DIR := $(KAL_DIR)/$(OS_DIR)

CORE_DIR := $(ROOT_DIR)/core
CORE_INC_DIR := $(CORE_DIR)/inc
CORE_SRC_DIR := $(CORE_DIR)

NET_DIR := $(ROOT_DIR)/net
NET_INC_DIR := $(NET_DIR)/inc
NET_SRC_DIR := $(NET_DIR)/$(OS_DIR)

MAKEFILE_DIR := $(ROOT_DIR)/mk
MAKEFILE_OS_DIR := $(MAKEFILE_DIR)/$(OS_DIR)

include $(MAKEFILE_OS_DIR)/$(OS_MAKEFILE_SRC)

# Makefile list
MAKEFILE_LIST := $(MAKEFILE_DIR)/license.hdr \
		$(MAKEFILE_DIR)/dir.mk \
		$(MAKEFILE_OS_DIR)/$(OS_MAKEFILE_CFG) \
		$(MAKEFILE_OS_DIR)/$(OS_MAKEFILE_SRC) \
		$(MAKEFILE_DIR)/src.mk \
		$(MAKEFILE_OS_DIR)/$(OS_MAKEFILE_RULE)

#######################################################################
# Source files

############
# KAL Source

KAL_SRCS := $(KAL_OS_SRCS) \
		rs_k_dbg.c \
		rs_k_event.c \
		rs_k_mem.c \
		rs_k_mutex.c \
		rs_k_if.c \
		rs_k_thread.c \
		rs_k_workqueue.c

KAL_SRCS := $(addprefix $(KAL_SRC_DIR)/,$(KAL_SRCS))

#############
# Core Source

CORE_SRCS := rs_c_if.c \
		rs_core.c \
		rs_c_q.c \
		rs_c_ctrl.c \
		rs_c_indi.c \
		rs_c_rx.c \
		rs_c_tx.c \
		rs_c_status.c \
		rs_c_recovery.c

CORE_SRCS := $(addprefix $(CORE_SRC_DIR)/,$(CORE_SRCS))

###################
# Net & Main Source

NET_SRCS := $(NET_OS_SRCS)

NET_SRCS := $(addprefix $(NET_SRC_DIR)/,$(NET_SRCS))

#######################################################################
# Rules definition

# build
all : build_init gen_driver

.PHONY: build_init
build_init:
	@echo
	@echo "=make build path===="
	$(MKDIR) -p $(BUILD_DIR)
	$(MKDIR) -p $(BUILD_OS_DIR)
	$(MKDIR) -p $(BUILD_SRC_DIR)

.PHONY: gen_driver
gen_driver:
	@echo
	@echo "=generate driver===="
	@echo " -> update Kernel Adaptation Layer"
	@$(CP) -ruv $(KAL_SRC_DIR)/. $(BUILD_SRC_DIR)
	@$(CP) -ruv $(KAL_INC_DIR)/. $(BUILD_SRC_DIR)

	@echo
	@echo " -> update CORE Layer"
	@$(CP) -uv $(CORE_SRC_DIR)/*.c $(BUILD_SRC_DIR)
	@$(CP) -ruv $(CORE_INC_DIR)/. $(BUILD_SRC_DIR)

	@echo
	@echo " -> update NET & MAIN Layer"
	@$(CP) -ruv $(NET_SRC_DIR)/. $(BUILD_SRC_DIR)
	@$(CP) -ruv $(NET_INC_DIR)/. $(BUILD_SRC_DIR)

	@echo
	@echo " -> upate build script"
	@$(CP) -uv $(MAKEFILE_OS_DIR)/$(OS_BUILD_SH) $(BUILD_OS_DIR)
	$(CHMOD) 755 $(BUILD_OS_DIR)/$(OS_BUILD_SH)

	@echo
	@echo "=generate makefile===="
	@$(CAT) $(MAKEFILE_LIST) > $(BUILD_OS_DIR)/Makefile
	@echo "--------------------"
	#$(CAT) $(BUILD_OS_DIR)/Makefile
	@echo "--------------------"

# clean
.PHONY: clean
clean:
	$(RM) -r $(BUILD_DIR)*

# install
