/*
 * edma3_dm730_cfg.c
 *
 * EDMA3 Resource Manager Adaptation Configuration File (SoC Specific).
 *
 * Copyright (C) 2010 Texas Instruments Incorporated - http://www.ti.com/
 *
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
*/

#include <ti/sdo/edma3/drv/edma3_drv.h>


/* Determine the processor id by reading DNUM register. */
unsigned short Utils_getRegionId()
    {
    return 1;
    }


unsigned short Utils_isGblConfigRequired(unsigned int dspNum)
    {
    (void) dspNum;

    return 0;
    }

/** Interrupt no. for Transfer Completion */
#define EDMA3_CC_XFER_COMPLETION_INT                  (20u)
/** Interrupt no. for CC Error */
#define EDMA3_CC_ERROR_INT                            (21u)
/** Interrupt no. for TC 0 Error */
#define EDMA3_TC0_ERROR_INT                           (22u)
/** Interrupt no. for TC 1 Error */
#define EDMA3_TC1_ERROR_INT                           (0u)
/** Interrupt no. for TC 2 Error */
#define EDMA3_TC2_ERROR_INT                           (0u)
/** Interrupt no. for TC 3 Error */
#define EDMA3_TC3_ERROR_INT                           (0u)
/** Interrupt no. for TC 4 Error */
#define EDMA3_TC4_ERROR_INT                           (0u)
/** Interrupt no. for TC 5 Error */
#define EDMA3_TC5_ERROR_INT                           (0u)
/** Interrupt no. for TC 6 Error */
#define EDMA3_TC6_ERROR_INT                           (0u)
/** Interrupt no. for TC 7 Error */
#define EDMA3_TC7_ERROR_INT                           (0u)

#include "utils_dma_common_cfg.c"


/* Driver Instance Initialization Configuration */
EDMA3_DRV_InstanceInitConfig gUtils_dmaInstInitConfig =
        {
        /* 31     0                     63    32                    95    64                    127   96 */
        {UTILS_C6XDSP_EDMACH_ALLOC_0, UTILS_C6XDSP_EDMACH_ALLOC_1, UTILS_C6XDSP_PARAM_ALLOC_0, UTILS_C6XDSP_PARAM_ALLOC_1,
        /* 159  128                     191  160                    223  192                    255  224 */
         UTILS_C6XDSP_PARAM_ALLOC_2, UTILS_C6XDSP_PARAM_ALLOC_3, UTILS_C6XDSP_PARAM_ALLOC_4, UTILS_C6XDSP_PARAM_ALLOC_5,
        /* 287  256                     319  288                    351  320                    383  352 */
         UTILS_C6XDSP_PARAM_ALLOC_6, UTILS_C6XDSP_PARAM_ALLOC_7, UTILS_C6XDSP_PARAM_ALLOC_8, UTILS_C6XDSP_PARAM_ALLOC_9,
        /* 415  384                     447  416                    479  448                    511  480 */
         UTILS_C6XDSP_PARAM_ALLOC_10, UTILS_C6XDSP_PARAM_ALLOC_11, UTILS_C6XDSP_PARAM_ALLOC_12, UTILS_C6XDSP_PARAM_ALLOC_13},

        /* ownDmaChannels */
        /* 31     0                     63    32 */
        {UTILS_C6XDSP_EDMACH_ALLOC_0, UTILS_C6XDSP_EDMACH_ALLOC_1},

        /* ownQdmaChannels */
        /* 31     0 */
        {UTILS_C6XDSP_QDMACH_ALLOC_0},

        /* ownTccs */
        /* 31     0                     63    32 */
        {UTILS_C6XDSP_EDMACH_ALLOC_0, UTILS_C6XDSP_EDMACH_ALLOC_1},


        /* resvdPaRAMSets */
        /* 31     0     63    32     95    64     127   96 */
        {0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u,
        /* 159  128     191  160     223  192     255  224 */
         0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u,
        /* 287  256     319  288     351  320     383  352 */
         0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u,
        /* 415  384     447  416     479  448     511  480 */
         0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u,},

        /* resvdDmaChannels */
        /* 31     0    63     32 */
        {0x00000000u, 0x00000000u},

        /* resvdQdmaChannels */
        /* 31     0 */
        {0x00000000u},

        /* resvdTccs */
        /* 31     0    63     32 */
        {0x00000000u, 0x00000000u},
    };



