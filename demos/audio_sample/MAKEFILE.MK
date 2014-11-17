
#INCLUDE = -I /opt/DVRRDK_03.00.00.00/dvr_rdk/demos/link_api_demos/common

include $(dvr_rdk_PATH)/makerules/common_header_a8.mk

INCLUDE+= $(LINUXDEVKIT_INC) $(OSA_INC) $(SYSLINK_INC) $(DEVICES_INC) $(MCFW_INC) $(CODEC_INC)

include $(dvr_rdk_PATH)/makerules/common_footer_a8.mk


