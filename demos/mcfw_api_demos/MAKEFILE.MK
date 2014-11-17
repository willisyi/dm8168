
demo:
	$(MAKE) -fMAKEFILE.MK libs TARGET=depend
	$(MAKE) -fMAKEFILE.MK libs
	$(MAKE) -fMAKEFILE.MK apps TARGET=exe

mcfw_demo:
ifeq ($(DISABLE_AUDIO),no)
	$(MAKE) -fMAKEFILE.MK -C$(dvr_rdk_PATH)/demos/audio_sample MODULE=dvr_rdk_demo_mcfw_api $(TARGET)
endif
#ifeq ($(PLATFORM),ti816x-evm)
	$(MAKE) -fMAKEFILE.MK -C$(dvr_rdk_PATH)/demos/graphic MODULE=dvr_rdk_demo_mcfw_api $(TARGET)
#endif	
	$(MAKE) -fMAKEFILE.MK -C$(dvr_rdk_PATH)/demos/display_process MODULE=dvr_rdk_demo_mcfw_api $(TARGET)
	$(MAKE) -fMAKEFILE.MK -C$(dvr_rdk_PATH)/demos/mcfw_api_demos/mcfw_demo MODULE=dvr_rdk_demo_mcfw_api $(TARGET)

libs: mcfw_demo 

apps:
	$(MAKE) -fMAKEFILE.MK -C$(dvr_rdk_PATH)/demos/mcfw_api_demos/mcfw_demo MODULE=dvr_rdk_demo_mcfw_api $(TARGET)

clean:
	$(MAKE) -fMAKEFILE.MK libs TARGET=clean

all: clean demo 

.PHONY: libs apps clean demo all mcfw_demo 

