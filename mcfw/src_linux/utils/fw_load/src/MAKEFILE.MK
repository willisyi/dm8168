
include $(dvr_rdk_PATH)/makerules/common_header_a8.mk

LIBS=$(LIB_DIR)/fw_load.a $(SYSLINK_LIBS)

INCLUDE+= $(SYSLINK_INC)

include $(dvr_rdk_PATH)/makerules/common_footer_a8.mk


