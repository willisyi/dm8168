/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#ifndef _HDVICP2_SOC_CONFIG_H_
#define _HDVICP2_SOC_CONFIG_H_

#include <xdc/std.h>

#ifdef TI_816X_BUILD
#    include <ti/psp/cslr/soc_TI816x.h>
#    define  PRCM_BASE_ADDR                                CSL_TI816x_PRCM_BASE
#    define  HDVICP_NUM_RESOURCES                          (3)
#else
#    if defined(TI_814X_BUILD) || defined(TI_8107_BUILD)
#        include <ti/psp/cslr/soc_TI814x.h>
#        define  PRCM_BASE_ADDR                            CSL_TI814x_PRCM_BASE
#        define  HDVICP_NUM_RESOURCES                      (1)
#    else
#        error "Unknow Device. Configure appropriate PRCM ADDR"
#    endif
#endif

/* IVAHD0 PRCM memory declarations */
#define CM_IVAHD0_CLKSTCTRL         (PRCM_BASE_ADDR + 0x0600)
#define CM_IVAHD0_IVAHD_CLKCTRL     (PRCM_BASE_ADDR + 0x0620)
#define CM_IVAHD0_SL2_CLKCTRL       (PRCM_BASE_ADDR + 0x0624)
#define PM_IVAHD0_PWRSTST           (PRCM_BASE_ADDR + 0x0C04)
#define RM_IVAHD0_RSTCTRL           (PRCM_BASE_ADDR + 0x0C10)
#define RM_IVAHD0_RSTST             (PRCM_BASE_ADDR + 0x0C14)
/* END OF IVAHD0 PRCM Definition */

/* IVAHD1 PRCM Definition */
#define CM_IVAHD1_CLKSTCTRL         (PRCM_BASE_ADDR + 0x0700)
#define CM_IVAHD1_IVAHD_CLKCTRL     (PRCM_BASE_ADDR + 0x0720)
#define CM_IVAHD1_SL2_CLKCTRL       (PRCM_BASE_ADDR + 0x0724)
#define PM_IVAHD1_PWRSTST           (PRCM_BASE_ADDR + 0x0D04)
#define RM_IVAHD1_RSTCTRL           (PRCM_BASE_ADDR + 0x0D10)
#define RM_IVAHD1_RSTST             (PRCM_BASE_ADDR + 0x0D14)
/* END OF IVAHD1 PRCM Definition */

/* IVAHD2 PRCM Definition */
#define CM_IVAHD2_CLKSTCTRL         (PRCM_BASE_ADDR + 0x0800)
#define CM_IVAHD2_IVAHD_CLKCTRL     (PRCM_BASE_ADDR + 0x0820)
#define CM_IVAHD2_SL2_CLKCTRL       (PRCM_BASE_ADDR + 0x0824)
#define PM_IVAHD2_PWRSTST           (PRCM_BASE_ADDR + 0x0E04)
#define RM_IVAHD2_RSTCTRL           (PRCM_BASE_ADDR + 0x0E10)
#define RM_IVAHD2_RSTST             (PRCM_BASE_ADDR + 0x0E14)
/* END OF IVAHD2 PRCM Definition */

#define IVA0_ICONT1_ITCM0_ADDR      (0x58088000)
#define IVA0_ICONT2_ITCM0_ADDR      (0x58098000)

#define IVA1_ICONT1_ITCM0_ADDR      (0x5A088000)
#define IVA1_ICONT2_ITCM0_ADDR      (0x5A098000)

#define IVA2_ICONT1_ITCM0_ADDR      (0x53088000)
#define IVA2_ICONT2_ITCM0_ADDR      (0x53098000)

/* IVAHD0 BW regulator Definition */
#define IVA0_L3_BWREG_BW_REGADDR      0x44001c08
#define IVA0_L3_BWREG_BYTECNT_REGADDR 0x44001c0c
#define IVA0_L3_BWREG_PRI_REGADDR     0x44001c10
#define IVA0_L3_BWREG_APPLY_REGADDR   0x44001c14
/* END OF IVAHD0 BW regulator Definition */

/* IVAHD1 BW regulator Definition */
#define IVA1_L3_BWREG_BW_REGADDR      0x44001d08
#define IVA1_L3_BWREG_BYTECNT_REGADDR 0x44001d0c
#define IVA1_L3_BWREG_PRI_REGADDR     0x44001d10
#define IVA1_L3_BWREG_APPLY_REGADDR   0x44001d14
/* END OF IVAHD1 BW regulator Definition */

/* IVAHD2 BW regulator Definition */
#define IVA2_L3_BWREG_BW_REGADDR      0x44001e08
#define IVA2_L3_BWREG_BYTECNT_REGADDR 0x44001e0c
#define IVA2_L3_BWREG_PRI_REGADDR     0x44001e10
#define IVA2_L3_BWREG_APPLY_REGADDR   0x44001e14
/* END OF IVAHD2 BW regulator Definition */
#define DMM_BASE_ADDR                  0x4E000000
#define DMM_PRIORITY_BASE_ADDR 		   (DMM_BASE_ADDR + 0x624)

#endif                                                     /* _HDVICP2_SOC_CONFIG_H_ 
                                                            */
