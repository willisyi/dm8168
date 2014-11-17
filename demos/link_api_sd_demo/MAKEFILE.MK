
demo:
	$(MAKE) -fMAKEFILE.MK libs TARGET=depend
	$(MAKE) -fMAKEFILE.MK libs
	$(MAKE) -fMAKEFILE.MK apps TARGET=exe

libs:
	$(MAKE) -fMAKEFILE.MK -C$(dvr_rdk_PATH)/demos/link_api_sd_demo/sd_demo MODULE=dvr_rdk_sd_demo_api $(TARGET)

apps:
	$(MAKE) -fMAKEFILE.MK -C$(dvr_rdk_PATH)/demos/link_api_sd_demo/sd_demo MODULE=dvr_rdk_sd_demo_api $(TARGET)

clean:
	$(MAKE) -fMAKEFILE.MK libs TARGET=clean

all: clean demo 
