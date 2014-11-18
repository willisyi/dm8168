
SRCDIR += links_m3video/system \
		links_m3video/iva_enc links_m3video/iva_dec links_m3video/codec_utils/src

SRCS_CODECUTILS = alg_control.c alg_create.c alg_malloc.c iresman_hdvicp2_fwif.c \
                  utils_encdec.c iresman_tiledmemory.c hdvicp2_config.c

SRCS_ENCLINK    = encLink_common.c encLink_h264.c encLink_tsk.c encLink_jpeg.c

SRCS_DECLINK    = decLink_common.c decLink_h264.c decLink_mpeg4.c decLink_jpeg.c decLink_tsk.c 

SRCS_SYSTEMVID  = system_m3video.c systemLink_tsk_m3video.c 

SRCS_m3video +=  $(SRCS_SYSTEMVID) $(SRCS_CODECUTILS) $(SRCS_ENCLINK) \
                 $(SRCS_DECLINK)

SRCS_m3video += utils_tiler.c

