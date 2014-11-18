

#ifndef _MEM_STATS_H_
#define _MEM_STATS_H_

#include <osa.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#define MAX_SAMPLE_TIME_IN_MSECS       (5000)

#define PERF_CNT_PHYS_BASE_ADDR        (0x4c000000)
#define PERF_CNT_MAP_SIZE              (0x100)
#define PERF_CNT_1_OFFSET              (0x80)
#define PERF_CNT_2_OFFSET              (0x84)
#define PERF_CNT_CFG_OFFSET            (0x88)
#define PERF_CNT_SEL_OFFSET            (0x8c)



#define MASTERID_HDVPSS_0              (0x24)
#define MASTERID_HDVPSS_1              (0x25)

#define MASTERID_IVA_0                 (0x28)
#define MASTERID_IVA_1                 (0x2C)

#define MASTERID_IVA_2                 (0x2D)
#define MASTERID_ALL                   (0xFF)

#define MASTERID_EMAC_0                (0x30)
#define MASTERID_EMAC_1                (0x31)

#define MASTERID_USB_DMA               (0x34)
#define MASTERID_USB_QMGR              (0x35)

#define MASTERID_SATA                  (0x39)
#define MASTERID_PCIE                  (0x3A)

#define MASTERID_A8                    (0x00)
#define MASTERID_M3                    (0x0E)

#define MASTERID_SYSTEM_MMU            (0x0A)

#define MASTERID_DSP_MDMA              (0x08)
#define MASTERID_DSP_CFG               (0x09)

#define MASTERID_EDMA_0_RD             (0x18)
#define MASTERID_EDMA_0_WR             (0x1C)

#define MASTERID_EDMA_1_RD             (0x19)
#define MASTERID_EDMA_1_WR             (0x1D)

#define MASTERID_EDMA_2_RD             (0x1A)
#define MASTERID_EDMA_2_WR             (0x1E)

#define MASTERID_EDMA_3_RD             (0x1B)
#define MASTERID_EDMA_3_WR             (0x1F)



#define MASTER_NAME_ALL                   "ALL      "
#define MASTER_NAME_IVA_0                 "IVA_0    "

#define MASTER_NAME_IVA_1                 "IVA_1    "
#define MASTER_NAME_IVA_2                 "IVA_2    "

#define MASTER_NAME_HDVPSS_0              "HDVPSS_0 "
#define MASTER_NAME_HDVPSS_1              "HDVPSS_1 "

#define MASTER_NAME_A8                    "A8       "
#define MASTER_NAME_M3                    "M3       "

#define MASTER_NAME_EDMA_0_RD             "EDMA_0_RD"
#define MASTER_NAME_EDMA_0_WR             "EDMA_0_WR"

#define MASTER_NAME_EDMA_1_RD             "EDMA_1_RD"
#define MASTER_NAME_EDMA_1_WR             "EDMA_1_WR"

#define MASTER_NAME_EDMA_2_RD             "EDMA_2_RD"
#define MASTER_NAME_EDMA_2_WR             "EDMA_2_WR"

#define MASTER_NAME_EDMA_3_RD             "EDMA_3_RD"
#define MASTER_NAME_EDMA_3_WR             "EDMA_3_WR"

#define MASTER_NAME_EMAC_0                "EMAC_0   "
#define MASTER_NAME_EMAC_1                "EMAC_1   "

#define MASTER_NAME_USB_DMA               "USB_DMA  "
#define MASTER_NAME_USB_QMGR              "USB_QMGR "

#define MASTER_NAME_SATA                  "SATA     "
#define MASTER_NAME_PCIE                  "PCIE     "

#define MASTER_NAME_SYSTEM_MMU            "SYS_MMU  "

#define MASTER_NAME_DSP_MDMA              "DSP_MDMA "
#define MASTER_NAME_DSP_CFG               "DSP_CFG  "


typedef struct
{
  unsigned int mmapMemAddr;
  unsigned int mmapMemSize;

  int    memDevFd;

  volatile unsigned int *pMemVirtAddr;
  volatile unsigned int *PERF_CNT_1;
  volatile unsigned int *PERF_CNT_2;
  volatile unsigned int *PERF_CNT_CFG;
  volatile unsigned int *PERF_CNT_SEL;

} MEM_STATS_Ctrl;


#endif

