/* =============================================================================
 * * Texas Instruments OpenMax Multimedia Sub-system * * (c) Copyright 2009 Texas 
 * Instruments Incorporated.  All Rights Reserved. * * Use of this software is
 * controlled by the terms and conditions found * in the license agreement under 
 * which this software has been supplied. *
 * =========================================================================== */
/**
 *****************************************************************************
 * \file
 *    hdvicp2_config.c                                                         
 *
 * \brief  
 *  This file contains device level configuration information for IVA HD
 *    
 * \version 1.0
 *
 *****************************************************************************
 */

/****************************************************************
*  INCLUDE FILES
****************************************************************/
#include <xdc/std.h>
#include <ti/sysbios/hal/Hwi.h>
#include <ti/psp/vps/vps.h>
#include <mcfw/src_bios6/utils/utils_trace.h>
#include <mcfw/src_bios6/links_m3video/codec_utils/hdvicp2_config.h>
#include "hdvicp2_soc_config.h"

#define ICONT_PER_IVA                        (2)
#define ICONT_SELF_BRANCH_INSTURCTION_OPCODE (0xEAFFFFFE)
#define HDVICP2_PRCM_MAX_RETRY_COUNT         (100)

typedef struct HDVICP2_PRCM_RegInfo {
    UInt32 CM_IVAHD_CLKSTCTRL;
    UInt32 CM_IVAHD_IVAHD_CLKCTRL;
    UInt32 CM_IVAHD_SL2_CLKCTRL;
    UInt32 PM_IVAHD_PWRSTST;
    UInt32 RM_IVAHD_RSTCTRL;
    UInt32 RM_IVAHD_RSTST;
} HDVICP2_PRCM_RegInfo;

typedef struct HDVICP2_BWREG_RegInfo {
    UInt32 IVA_L3_BWREG_BW_REGADDR;
    UInt32 IVA_L3_BWREG_BYTECNT_REGADDR;
    UInt32 IVA_L3_BWREG_PRI_REGADDR;
    UInt32 IVA_L3_BWREG_APPLY_REGADDR;
} HDVICP2_BWREG_RegInfo;

typedef struct HDVICP2_SocConfigInfo_s {
    UInt32 numHDVICP2;
    HDVICP2_PRCM_RegInfo prcmRegInfo[HDVICP_NUM_RESOURCES];
    UInt32 itcmAddr[HDVICP_NUM_RESOURCES][ICONT_PER_IVA];
    HDVICP2_BWREG_RegInfo bwRegulatorRegInfo[HDVICP_NUM_RESOURCES];
} HDVICP2_SocConfigInfo_s;

const HDVICP2_SocConfigInfo_s HDVICP2_SocConfigInfo =
    {.numHDVICP2 = HDVICP_NUM_RESOURCES,
    .prcmRegInfo = {
#if (HDVICP_NUM_RESOURCES >= 1)
                    {
                     .CM_IVAHD_CLKSTCTRL = CM_IVAHD0_CLKSTCTRL,
                     .CM_IVAHD_IVAHD_CLKCTRL = CM_IVAHD0_IVAHD_CLKCTRL,
                     .CM_IVAHD_SL2_CLKCTRL = CM_IVAHD0_SL2_CLKCTRL,
                     .PM_IVAHD_PWRSTST = PM_IVAHD0_PWRSTST,
                     .RM_IVAHD_RSTCTRL = RM_IVAHD0_RSTCTRL,
                     .RM_IVAHD_RSTST = RM_IVAHD0_RSTST}
#endif
#if (HDVICP_NUM_RESOURCES >= 2)
                    ,
                    {
                     .CM_IVAHD_CLKSTCTRL = CM_IVAHD1_CLKSTCTRL,
                     .CM_IVAHD_IVAHD_CLKCTRL = CM_IVAHD1_IVAHD_CLKCTRL,
                     .CM_IVAHD_SL2_CLKCTRL = CM_IVAHD1_SL2_CLKCTRL,
                     .PM_IVAHD_PWRSTST = PM_IVAHD1_PWRSTST,
                     .RM_IVAHD_RSTCTRL = RM_IVAHD1_RSTCTRL,
                     .RM_IVAHD_RSTST = RM_IVAHD1_RSTST}
#endif
#if (HDVICP_NUM_RESOURCES >= 3)
                    ,
                    {
                     .CM_IVAHD_CLKSTCTRL = CM_IVAHD2_CLKSTCTRL,
                     .CM_IVAHD_IVAHD_CLKCTRL = CM_IVAHD2_IVAHD_CLKCTRL,
                     .CM_IVAHD_SL2_CLKCTRL = CM_IVAHD2_SL2_CLKCTRL,
                     .PM_IVAHD_PWRSTST = PM_IVAHD2_PWRSTST,
                     .RM_IVAHD_RSTCTRL = RM_IVAHD2_RSTCTRL,
                     .RM_IVAHD_RSTST = RM_IVAHD2_RSTST}
#endif
#if (HDVICP_NUM_RESOURCES >= 4)
#error "Unintialized PRCM entry . Do initialization"
#endif
                    },
    .itcmAddr = {
#if (HDVICP_NUM_RESOURCES >= 1)
                 {
                  IVA0_ICONT1_ITCM0_ADDR,
                  IVA0_ICONT2_ITCM0_ADDR}
#endif
#if (HDVICP_NUM_RESOURCES >= 2)
                 ,
                 {
                  IVA1_ICONT1_ITCM0_ADDR,
                  IVA1_ICONT2_ITCM0_ADDR}
#endif
#if (HDVICP_NUM_RESOURCES >= 3)
                 ,
                 {
                  IVA2_ICONT1_ITCM0_ADDR,
                  IVA2_ICONT2_ITCM0_ADDR}
#endif
#if (HDVICP_NUM_RESOURCES >= 4)
#error "Unintialized ICONT ITCM entry . Do initialization"
#endif
                 },
    .bwRegulatorRegInfo = {
#if (HDVICP_NUM_RESOURCES >= 1)
                           {
                            .IVA_L3_BWREG_BW_REGADDR = IVA0_L3_BWREG_BW_REGADDR,
                            .IVA_L3_BWREG_BYTECNT_REGADDR =
                            IVA0_L3_BWREG_BYTECNT_REGADDR,
                            .IVA_L3_BWREG_PRI_REGADDR =
                            IVA0_L3_BWREG_PRI_REGADDR,
                            .IVA_L3_BWREG_APPLY_REGADDR =
                            IVA0_L3_BWREG_APPLY_REGADDR,
                            }
#endif
#if (HDVICP_NUM_RESOURCES >= 2)
                           ,
                           {
                            .IVA_L3_BWREG_BW_REGADDR = IVA1_L3_BWREG_BW_REGADDR,
                            .IVA_L3_BWREG_BYTECNT_REGADDR =
                            IVA1_L3_BWREG_BYTECNT_REGADDR,
                            .IVA_L3_BWREG_PRI_REGADDR =
                            IVA1_L3_BWREG_PRI_REGADDR,
                            .IVA_L3_BWREG_APPLY_REGADDR =
                            IVA1_L3_BWREG_APPLY_REGADDR,
                            }
#endif
#if (HDVICP_NUM_RESOURCES >= 3)
                           ,
                           {
                            .IVA_L3_BWREG_BW_REGADDR = IVA2_L3_BWREG_BW_REGADDR,
                            .IVA_L3_BWREG_BYTECNT_REGADDR =
                            IVA2_L3_BWREG_BYTECNT_REGADDR,
                            .IVA_L3_BWREG_PRI_REGADDR =
                            IVA2_L3_BWREG_PRI_REGADDR,
                            .IVA_L3_BWREG_APPLY_REGADDR =
                            IVA2_L3_BWREG_APPLY_REGADDR,
                            }
#endif
#if (HDVICP_NUM_RESOURCES >= 4)
#error "Unintialized Bandwidth regulator entry . Do initialization"
#endif
                           }
};

#ifdef WR_MEM_32
#undef WR_MEM_32
#endif
#define WR_MEM_32(addr, data)                                                  \
                           *(volatile unsigned int*)(addr) =(unsigned int)(data)

#ifdef RD_MEM_32
#undef RD_MEM_32
#endif

#define RD_MEM_32(addr)                          *(volatile unsigned int*)(addr)

static Void delayLoop(Int delay_cnt)
{
    volatile int i;

    for (i = 0; i < delay_cnt; i++)
    {
        asm(" NOP ");
    }
}

static
Void HDVICP2_PRCM(UInt resId)
{
    volatile HDVICP2_PRCM_RegInfo *prcmReg;
    UInt retryCount = 0;

    Vps_printf(" %d: HDVICP: Doing PRCM for IVAHD[%d] ... \n",
        Utils_getCurTimeInMsec(), resId);
    UTILS_assert(resId < HDVICP_NUM_RESOURCES);
    prcmReg =
        (volatile HDVICP2_PRCM_RegInfo *) &HDVICP2_SocConfigInfo.
        prcmRegInfo[resId];

    WR_MEM_32(prcmReg->CM_IVAHD_CLKSTCTRL, 2);             /* Enable Power
                                                            * Domain
                                                            * Transition */
    while (RD_MEM_32(prcmReg->PM_IVAHD_PWRSTST) != 0x37) ; /* Check Power is
                                                            * ON */
    WR_MEM_32(prcmReg->CM_IVAHD_IVAHD_CLKCTRL, 2);         /* Enable IVHD0
                                                            * Clocks */
    WR_MEM_32(prcmReg->CM_IVAHD_SL2_CLKCTRL, 2);           /* Enable IVHD0
                                                            * SL2 Clocks */

    while (((RD_MEM_32(prcmReg->CM_IVAHD_CLKSTCTRL) & 0x100)) != 0x100) ;   /* IVAHD0_GCLK 
                                                                             * is 
                                                                             * Active */

    WR_MEM_32(prcmReg->RM_IVAHD_RSTCTRL, 3);               /* Enable IVHD0
                                                            * logic & SL2 */

    while (RD_MEM_32(prcmReg->RM_IVAHD_RSTST) != 4)
    {
        delayLoop(1000);
        retryCount++;
        if (retryCount >= HDVICP2_PRCM_MAX_RETRY_COUNT)
        {
            Vps_printf(" %d: HDVICP: WARNING: RM_IVAHD_RSTST Timed OUT [0x%x] \n",
                       Utils_getCurTimeInMsec(), RD_MEM_32(prcmReg->RM_IVAHD_RSTST));
            break;
        }
    }

    /* Write Self Branch Instruction in ICONT1 ITCM 0 Location */
    WR_MEM_32(HDVICP2_SocConfigInfo.itcmAddr[resId][0],
              ICONT_SELF_BRANCH_INSTURCTION_OPCODE);

    /* Write Self Branch Instruction in ICONT2 ITCM 0 Location */
    WR_MEM_32(HDVICP2_SocConfigInfo.itcmAddr[resId][1],
              ICONT_SELF_BRANCH_INSTURCTION_OPCODE);

    WR_MEM_32(prcmReg->RM_IVAHD_RSTCTRL, 0);               /* Bring ICONT1 &
                                                            * ICONT2 out of
                                                            * Reset */

    while (RD_MEM_32(prcmReg->RM_IVAHD_RSTST) != 7) ;      /* ICONT1 & ICONT2 
                                                            * are out of
                                                            * Reset */

    Vps_printf(" %d: HDVICP: PRCM for IVAHD[%d] ... DONE.\n",
        Utils_getCurTimeInMsec(), resId);
}

extern UInt ti_sdo_fc_ires_hdvicp_HDVICP2_interrupts[];

Void HDVICP2_PRCM_All()
{
    Int i;

    for (i = 0; i < HDVICP2_SocConfigInfo.numHDVICP2; i++)
    {
        HDVICP2_PRCM(i);
    }

}

Void HDVICP2_ClearIVAInterrupts()
{
    Int i;

    for (i = 0; i < HDVICP2_SocConfigInfo.numHDVICP2; i++)
    {
        Hwi_clearInterrupt(ti_sdo_fc_ires_hdvicp_HDVICP2_interrupts[i]);
    }
}

static
Void HDVICP2_configBandwidthRegulator(UInt resId)
{
    volatile HDVICP2_BWREG_RegInfo *bwReg;

    Vps_printf("\nEntered BW REG Config for IVHD[%d] \n", resId);
    UTILS_assert(resId < HDVICP_NUM_RESOURCES);
    bwReg =
        (volatile HDVICP2_BWREG_RegInfo *) &HDVICP2_SocConfigInfo.
        bwRegulatorRegInfo[resId];

    /* TODO. THIS IS CURRENTLY HARDCODED. THIS SHOULD CALCULATED FROM
     * EXPECTED IVA BANDWIDTH USAGE */
    WR_MEM_32(bwReg->IVA_L3_BWREG_BW_REGADDR, 0x80);
    WR_MEM_32(bwReg->IVA_L3_BWREG_BYTECNT_REGADDR, 0xFA0);
    WR_MEM_32(bwReg->IVA_L3_BWREG_PRI_REGADDR, 0x1);
    WR_MEM_32(bwReg->IVA_L3_BWREG_APPLY_REGADDR, 0x1);

    Vps_printf("\n Leaving BW REG Config for IVHD[%d] \n", resId);
}

Void HDVICP2_ConfigBWREG_All()
{
    Int i;

    for (i = 0; i < HDVICP2_SocConfigInfo.numHDVICP2; i++)
    {
        HDVICP2_configBandwidthRegulator(i);
    }

}

Void HDVICP2_setIVADMMPriorityHigh()
{
#ifdef TI_816X_BUILD                                       /* TODO for
                                                            * Centaurus */
    /* IVA-HDs DMM priority set to high */
    WR_MEM_32((DMM_PRIORITY_BASE_ADDR + 0x10), 0x00990009);
#else
//#error "HDVICP2_setIVADMMPriorityHigh TODO for this device"
#endif
}

Void HDVICP2_setDucatiDMMPriorityHigh()
{
#ifdef TI_816X_BUILD                                       /* TODO for
                                                            * Centaurus */
    /* Ducati DMM priority set to high */
    WR_MEM_32((DMM_PRIORITY_BASE_ADDR), 0x08000000);
#else
//#error "HDVICP2_setDucatiDMMPriorityHigh TODO for this device"
#endif
}

Void HDVICP2_Init()
{
    HDVICP2_PRCM_All();
    #if 0
    HDVICP2_setDucatiDMMPriorityHigh();
    HDVICP2_setIVADMMPriorityHigh();
    #endif
    // HDVICP2_ConfigBWREG_All();
}

UInt32 HDVICP2_GetNumberOfIVAs()
{
    return (HDVICP2_SocConfigInfo.numHDVICP2);
}
