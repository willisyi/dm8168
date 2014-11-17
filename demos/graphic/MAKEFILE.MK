
include $(dvr_rdk_PATH)/makerules/common_header_a8.mk

INCLUDE+= -I$(KERNELDIR)/usr/include -I$(KERNELDIR)/arch/arm/include/ $(MCFW_INC) $(OSA_INC) $(DEVICES_INC)

include $(dvr_rdk_PATH)/makerules/common_footer_a8.mk
