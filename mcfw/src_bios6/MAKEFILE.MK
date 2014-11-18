#Disable XDC package build by default
ENABLE_XDC_PKG_BUILD ?= false
XDC_BUILD_PKG_LIST_COMMON += $(fc_PATH)/packages/ti.sdo.fc.dskt2
XDC_BUILD_PKG_LIST_M3      = $(fc_PATH)/packages/ti.sdo.fc.ires.hdvicp
XDC_BUILD_PKG_LIST_M3      += $(XDC_BUILD_PKG_LIST_COMMON)
XDC_BUILD_PKG_LIST_C6XDSP  = 
XDC_BUILD_PKG_LIST_C6XDSP += $(XDC_BUILD_PKG_LIST_COMMON)

FIRMWARE_OUT_PATH=$(TARGET_FS_DIR)/firmware/
XEM3_IN_PATH=$(DEST_ROOT)/dvr_rdk/bin/$(PLATFORM)/

DO_FXN_PROFILE  ?= false
DO_DSP_FXN_PROFILE ?= false

apps:
ifneq ($(PLATFORM),ti810x-evm)
	$(MAKE) -C $(dvr_rdk_PATH)/mcfw/src_bios6 CORE=c6xdsp ENABLE_FXN_PROFILE=$(DO_DSP_FXN_PROFILE)
endif
	$(MAKE) -C $(dvr_rdk_PATH)/mcfw/src_bios6 CORE=m3vpss ENABLE_FXN_PROFILE=$(DO_FXN_PROFILE)
	$(MAKE) -C $(dvr_rdk_PATH)/mcfw/src_bios6 CORE=m3video
ifeq ($(ENABLE_XDC_PKG_BUILD),true)
ifneq ($(PLATFORM),ti810x-evm)
	$(xdc_PATH)/xdc XDCBUILDCFG=$(dvr_rdk_PATH)/mcfw/src_bios6/cfg/ti816x/config_1G_128MLinux.bld XDCARGS="profile=$(PROFILE_c6xdsp)" -PD $(subst .,/,$(XDC_BUILD_PKG_LIST_C6XDSP))
endif
endif
ifneq ($(PLATFORM),ti810x-evm)
	$(MAKE) -C $(dvr_rdk_PATH)/mcfw/src_bios6/main_app xdc_configuro CORE=c6xdsp ENABLE_FXN_PROFILE=$(DO_DSP_FXN_PROFILE)
	$(MAKE) -C $(dvr_rdk_PATH)/mcfw/src_bios6/main_app all CORE=c6xdsp
endif
ifeq ($(ENABLE_XDC_PKG_BUILD),true)
	$(xdc_PATH)/xdc XDCBUILDCFG=$(dvr_rdk_PATH)/mcfw/src_bios6/cfg/ti816x/config_1G_128MLinux.bld XDCARGS="profile=$(PROFILE_m3vpss)" -PD $(subst .,/,$(XDC_BUILD_PKG_LIST_M3))
endif
	$(MAKE) -C $(dvr_rdk_PATH)/mcfw/src_bios6/main_app xdc_configuro CORE=m3vpss ENABLE_FXN_PROFILE=$(DO_FXN_PROFILE)
	$(MAKE) -C $(dvr_rdk_PATH)/mcfw/src_bios6/main_app CORE=m3vpss
	$(MAKE) -C $(dvr_rdk_PATH)/mcfw/src_bios6/main_app xdc_configuro CORE=m3video
	$(MAKE) -C $(dvr_rdk_PATH)/mcfw/src_bios6/main_app all CORE=m3video
	-mkdir -p $(FIRMWARE_OUT_PATH)
	cp $(XEM3_IN_PATH)/dvr_rdk_m3video_$(PROFILE_m3video).xem3 $(FIRMWARE_OUT_PATH)/dvr_rdk_fw_m3video.xem3
	cp $(XEM3_IN_PATH)/dvr_rdk_m3vpss_$(PROFILE_m3vpss).xem3 $(FIRMWARE_OUT_PATH)/dvr_rdk_fw_m3vpss.xem3
ifneq ($(PLATFORM),ti810x-evm)
	cp $(XEM3_IN_PATH)/dvr_rdk_c6xdsp_$(PROFILE_c6xdsp).xe674 $(FIRMWARE_OUT_PATH)/dvr_rdk_fw_c6xdsp.xe674
endif

clean:
ifneq ($(PLATFORM),ti810x-evm)
	$(MAKE) -C $(dvr_rdk_PATH)/mcfw/src_bios6 clean CORE=c6xdsp
endif
	$(MAKE) -C $(dvr_rdk_PATH)/mcfw/src_bios6 clean CORE=m3vpss
	$(MAKE) -C $(dvr_rdk_PATH)/mcfw/src_bios6 clean CORE=m3video
ifneq ($(PLATFORM),ti810x-evm)
	$(MAKE) -C $(dvr_rdk_PATH)/mcfw/src_bios6/main_app clean CORE=c6xdsp	
endif
	$(MAKE) -C $(dvr_rdk_PATH)/mcfw/src_bios6/main_app clean CORE=m3vpss
	$(MAKE) -C $(dvr_rdk_PATH)/mcfw/src_bios6/main_app clean CORE=m3video
ifeq ($(ENABLE_XDC_PKG_BUILD),true)
ifneq ($(PLATFORM),ti810x-evm)
	$(xdc_PATH)/xdc clean XDCBUILDCFG=$(dvr_rdk_PATH)/mcfw/src_bios6/cfg/ti816x/config_1G_128MLinux.bld -PD $(subst .,/,$(XDC_BUILD_PKG_LIST_C6XDSP))
endif
endif
ifeq ($(ENABLE_XDC_PKG_BUILD),true)
	$(xdc_PATH)/xdc clean XDCBUILDCFG=$(dvr_rdk_PATH)/mcfw/src_bios6/cfg/ti816x/config_1G_128MLinux.bld -PD $(subst .,/,$(XDC_BUILD_PKG_LIST_M3))
endif

all: clean apps


