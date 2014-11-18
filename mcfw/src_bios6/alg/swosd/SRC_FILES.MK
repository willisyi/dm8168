
SRCDIR += alg/swosd/src

SRCS_SWOSD = sw_osd_ti_alg_if.c
SRCS_SWOSD += sw_osd_ti_alg.c
SRCS_SWOSD += sw_osd_ti_dma.c
SRCS_SWOSD += sw_osd_ti_dmaOpt.c
SRCS_SWOSD += sw_osd_ti_ialg.c
SRCS_SWOSD += sw_osd_ti_ires.c


SRCS_c6xdsp +=  $(SRCS_SWOSD)
INCDIR += $(SRCDIR)

