
include $(dvr_rdk_PATH)/makerules/common_header_a8.mk

INCLUDE+= $(OSA_INC)

LIBS=$(LIB_DIR)/i2c_rdwr.a $(LIB_DIR)/dvr_rdk_osa.a

include $(dvr_rdk_PATH)/makerules/common_footer_a8.mk


