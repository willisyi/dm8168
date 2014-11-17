
demo:
	$(MAKE) -fMAKEFILE.MK libs TARGET=depend
	$(MAKE) -fMAKEFILE.MK libs
	$(MAKE) -fMAKEFILE.MK apps TARGET=exe

libs:
	$(MAKE) -fMAKEFILE.MK -C$(dvr_rdk_PATH)/demos/link_api_demos/vpss_chains MODULE=dvr_rdk_demo_link_api $(TARGET)
	$(MAKE) -fMAKEFILE.MK -C$(dvr_rdk_PATH)/demos/link_api_demos/vpss_video_chains MODULE=dvr_rdk_demo_link_api $(TARGET)
	$(MAKE) -fMAKEFILE.MK -C$(dvr_rdk_PATH)/demos/link_api_demos/video_chains MODULE=dvr_rdk_demo_link_api $(TARGET)
	$(MAKE) -fMAKEFILE.MK -C$(dvr_rdk_PATH)/demos/link_api_demos/common MODULE=dvr_rdk_demo_link_api $(TARGET)
	$(MAKE) -fMAKEFILE.MK -C$(dvr_rdk_PATH)/demos/audio_sample MODULE=dvr_rdk_demo_link_api $(TARGET)
	$(MAKE) -fMAKEFILE.MK -C$(dvr_rdk_PATH)/demos/graphic MODULE=dvr_rdk_demo_link_api $(TARGET)

apps:
	$(MAKE) -fMAKEFILE.MK -C$(dvr_rdk_PATH)/demos/link_api_demos/common MODULE=dvr_rdk_demo_link_api $(TARGET)

clean:
	$(MAKE) -fMAKEFILE.MK libs TARGET=clean

all: clean demo

