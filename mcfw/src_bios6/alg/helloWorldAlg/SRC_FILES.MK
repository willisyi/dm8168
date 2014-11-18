
SRCDIR += alg/helloWorldAlg/src

SRCS_HELLOWORLD_ALG = helloWorldAlg_TI_ialg.c

#Other algorithm source files
#SRCS_HELLOWORLD += helloWorldAlg_1.c
#SRCS_HELLOWORLD += helloWorldAlg_2.c


SRCS_c6xdsp +=  $(SRCS_HELLOWORLD_ALG)
INCDIR += $(SRCDIR) $(SRCDIR)/../

