/** =============================================================================
*  Texas Instruments OpenMax Multimedia Sub-system                             *
*  (c) Copyright 2009 Texas Instruments Incorporated.  All Rights Reserved.
*
*  Use of this software is controlled by the terms and conditions found
*  in the license agreement under which this software has been supplied.
 * =========================================================================== */
/**
 * ****************************************************************************
 * \file
 *    IRESMAN_APIs.c
 *
 * \brief
 *  This file contains implementation for IRESMAN APIs configure, wait and done.
 *  It also contains the ISR and SWI function implementations
 *
 * \version 1.0
 *
 *****************************************************************************
 */

/****************************************************************
*  INCLUDE FILES
****************************************************************/
#include <string.h>
#include <ti/sdo/fc/utils/dbc.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/knl/Swi.h>
#include <ti/sysbios/hal/Hwi.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/Gate.h>
#include <ti/sdo/fc/ires/hdvicp/ires_hdvicp2.h>
#include <mcfw/src_bios6/links_m3video/codec_utils/utils_encdec_prf.h>
#include <mcfw/src_bios6/links_m3video/codec_utils/hdvicp2_config.h>

/****************************************************************
* DEFINES
****************************************************************/

/** Interrupt number corresponding to IVAHD interrupt lines */
#define IVAHD_INTRPT_ID 38
#ifdef TI_816X_BUILD
#define IVAHD_INTRPT_ID1 35
#define IVAHD_INTRPT_ID2 32
#endif
/****************************************************************
* STRUCTURE DEFINITIONS
****************************************************************/

/** ========================================================================== */
/** IRESMAN_HDVICP2_SWIarg  describes the SWI Argument list.
*
* @param  cbFunctionPtr  Callback function pointer
* @param  algHandle      Algorithm Handle
* @param  cbArgs         Arguments to the callback function
*
*/
/** ========================================================================== */
typedef struct IRESMAN_HDVICP2_SWIarg {
    IRES_HDVICP2_CallbackFxn cbFunctionPtr;
    IALG_Handle algHandle;
    Void *cbArgs;
    int id;
} IRESMAN_HDVICP2_SWIarg;

typedef struct _IRESMAN__HDVICP2_context {
    /** Handle to SWI returned from Swi_create */
    Swi_Handle swiHandle;
    Swi_Struct swiStruct;
    IRESMAN_HDVICP2_SWIarg swiArg;

    /** Global variable to hold the semaphore handle */
    Semaphore_Handle semHandle;
    Semaphore_Struct semStruct;

    /** Handle to HWI returned from Hwi_create function */
    Hwi_Handle hwiHandle;
    Hwi_Struct hwiStruct;

    /** Flag to restrict HWI create to first time */
    int initialized;
	
	Bool inIVAHDprocess;
} IRESMAN__HDVICP2_context;

/****************************************************************
* FUNCTION PROTOTYPES
****************************************************************/
void IRESMAN_HDVICP2_ISR(UArg hwiArgs);
#ifdef TI_816X_BUILD
void IRESMAN_HDVICP2_ISR1(UArg hwiArgs);
void IRESMAN_HDVICP2_ISR2(UArg hwiArgs);
#endif
void IRESMAN_HDVICP2_SwiFunc(UArg swiArgs, UArg swiArgs_2);

/****************************************************************
* GLOBALS
****************************************************************/
#ifdef TI_816X_BUILD	// DBIAN
#define MAX_IVAHD 3
#endif
#if defined(TI_814X_BUILD) || defined(TI_8107_BUILD)
#define MAX_IVAHD 1
#endif
IRESMAN__HDVICP2_context _HDVICP2_context[MAX_IVAHD] = { 0 };

static int _first_time = 0;

/** ========================================================================== */
/**
* IRESMAN_HDVICP2_ISR(hwiArgs) : Call-Back function tied to IVAHD-0.
* As with Ducati a single IVA-HD resource is tied this id shall be zero.
*
* @param hwiArgs     Parameter configuration for HWI's - indicates IVAHD number
*
* @return           None
*/
/** ========================================================================== */

void IRESMAN_HDVICP2_ISR(UArg hwiArgs)
{
    /* hwiArgs indicates the IVAHD number */
    if (hwiArgs == 0)
    {
        Hwi_disableInterrupt(IVAHD_INTRPT_ID);
    }
#ifdef UTILS_ENCDEC_HDVICP_PROFILE
    {
        UInt32 curTime = Utils_encdecGetTime();

        g_HDVICP_logTbl[0].totalWait2Isr +=
            (curTime - g_HDVICP_logTbl[0].tempWaitTime);
        g_HDVICP_logTbl[0].tempPrevTime = curTime;
    }
#endif
    Swi_post(_HDVICP2_context[hwiArgs].swiHandle);
}

#ifdef TI_816X_BUILD
void IRESMAN_HDVICP2_ISR1(UArg hwiArgs)
{
    /* hwiArgs indicates the IVAHD number */
    if (hwiArgs == 1)
    {
        Hwi_disableInterrupt(IVAHD_INTRPT_ID1);
    }
#ifdef UTILS_ENCDEC_HDVICP_PROFILE
    {
        UInt32 curTime = Utils_encdecGetTime();

        g_HDVICP_logTbl[1].totalWait2Isr +=
            (curTime - g_HDVICP_logTbl[1].tempWaitTime);
        g_HDVICP_logTbl[1].tempPrevTime = curTime;
    }
#endif
    Swi_post(_HDVICP2_context[hwiArgs].swiHandle);
}

void IRESMAN_HDVICP2_ISR2(UArg hwiArgs)
{
    /* hwiArgs indicates the IVAHD number */
    if (hwiArgs == 2)
    {
        Hwi_disableInterrupt(IVAHD_INTRPT_ID2);
    }
#ifdef UTILS_ENCDEC_HDVICP_PROFILE
    {
        UInt32 curTime = Utils_encdecGetTime();

        g_HDVICP_logTbl[2].totalWait2Isr +=
            (curTime - g_HDVICP_logTbl[2].tempWaitTime);
        g_HDVICP_logTbl[2].tempPrevTime = curTime;
    }
#endif
    Swi_post(_HDVICP2_context[hwiArgs].swiHandle);
}
#endif
/** ========================================================================== */
/**
* IRESMAN_HDVICP2_SwiFunc(swiArgs, swiArgs_2) : Swi triggered by the ISR func
* Releases the semaphore that "wait" is pending on
*
* @param swiArgs     Parameter properties for SWI
* @param swiArgs_2   Parameter add to conform to XDC func. signature
*
* @return           None
*/
/** ========================================================================== */

void IRESMAN_HDVICP2_SwiFunc(UArg swiArgs, UArg swiArgs_2)
{
    IRESMAN_HDVICP2_SWIarg *CB;
    IALG_Handle algHandle;
    Void *cbArgs;
    IRES_HDVICP2_CallbackFxn cbFunctionPtr;

    DBC_require(swiArgs);

    CB = (IRESMAN_HDVICP2_SWIarg *) swiArgs;

    algHandle = CB->algHandle;
    cbArgs = CB->cbArgs;
    cbFunctionPtr = CB->cbFunctionPtr;

    cbFunctionPtr(algHandle, cbArgs);

    /** Clear the interrupt line for IVAHD-0
     * TODO: For multiple IVAHD's plugin
     * the corresponding interrupt line clear here */
    if (CB->id == 0)
    {
        Hwi_clearInterrupt(IVAHD_INTRPT_ID);
        Hwi_enableInterrupt(IVAHD_INTRPT_ID);
    }
#ifdef TI_816X_BUILD
    if (CB->id == 1)
    {
        Hwi_clearInterrupt(IVAHD_INTRPT_ID1);
        Hwi_enableInterrupt(IVAHD_INTRPT_ID1);
    }
    if (CB->id == 2)
    {
        Hwi_clearInterrupt(IVAHD_INTRPT_ID2);
        Hwi_enableInterrupt(IVAHD_INTRPT_ID2);
    }
#endif
}

/*----------------------------------------------------------------------------*/
/* Hex code to set for Stack setting, Interrupt vector setting */
/* and instruction to put ICONT in WFI mode.  */
/* This shall be placed at TCM_BASE_ADDRESS of given IVAHD, which is */
/* 0x0000 locally after reset.  */
/*----------------------------------------------------------------------------*/
#define LENGTH_BOOT_CODE  14
/**
 *  Macro defining ICONT1 DTCM offset from the base address
*/
#define ICONT1_DTCM_OFFSET 0x00000000
/**
 *  Macro defining ICONT2 DTCM offset from the base address
*/
#define ICONT2_DTCM_OFFSET 0x00010000
/**
 *  Macro defining ICONT1 ITCM offset from the base address
*/
#define ICONT1_ITCM_OFFSET 0x00008000
/**
 *  Macro defining ICONT2 ITCM offset from the base address
*/
#define ICONT2_ITCM_OFFSET 0x00018000

const unsigned int IVAHD_memory_wfi[LENGTH_BOOT_CODE] = {
    0xEA000006,
    0xEAFFFFFE,
    0xEAFFFFFE,
    0xEAFFFFFE,
    0xEAFFFFFE,
    0xEAFFFFFE,
    0xEAFFFFFE,
    0xEAFFFFFE,
    0xE3A00000,
    0xEE070F9A,
    0xEE070F90,
    0xE3A00000,
    0xEAFFFFFE,
    0xEAFFFFF1
};

XDAS_Int32 IVAHD_Standby_power_on_uboot_3ivahd()
{

    unsigned int length = 0;

    /*------------------------------------------------------------------------*/
    /* Assigment of pointers */
    /* A generic code shall take all address as input parameters */
    /*------------------------------------------------------------------------*/
    volatile unsigned int *prcm_ivahd_icont_rst_cntl_addr =
        (unsigned int *) 0x48180C10;
    volatile unsigned int *icont1_itcm_base_addr =
        (unsigned int *) (0x58000000 + ICONT1_ITCM_OFFSET);
    volatile unsigned int *icont2_itcm_base_addr =
        (unsigned int *) (0x58000000 + ICONT2_ITCM_OFFSET);
    /*------------------------------------------------------------------------*/
    /* Set IVAHD in reset mode to enable downloading of boot code */
    /* Please note that this state can be SKIPPED if IVAHD is alredy in reset
     */
    /* state during uboot and reset is not de-asserted */
    /* Set bit0 to 1 to put ICONT1 in reset state */
    /* Set bit1 to 1 to put ICONT2 in reset state */
    /*------------------------------------------------------------------------*/
    *prcm_ivahd_icont_rst_cntl_addr |= 0x00000003;

    /*------------------------------------------------------------------------*/
    /* Copy boot code to ICONT1 & INCOT2 memory */
    /*------------------------------------------------------------------------*/
    for (length = 0; length < LENGTH_BOOT_CODE; length++)
    {
        *icont1_itcm_base_addr++ = IVAHD_memory_wfi[length];
        *icont2_itcm_base_addr++ = IVAHD_memory_wfi[length];
    }
    /*------------------------------------------------------------------------*/
    /* Take IVAHD out of reset mode.  */
    /* Set bit0 to 0 to take ICONT1 out of reset state */
    /* Set bit1 to 0 to take ICONT1 out of reset state */
    /* This implies ICONT inside IVAHD will exectute WFI */
    /*------------------------------------------------------------------------*/
    *prcm_ivahd_icont_rst_cntl_addr &= 0xFFFFFFFC;

    /*------------------------------------------------------------------------*/
    /* As ICONT goes in WFI and there are no pending VDMA transction */
    /* entire IVAHD will be go in standby mode and PRCM will fully control */
    /* further managment of IVAHD power state */
    /*------------------------------------------------------------------------*/

    return (1);
}

/** ========================================================================== */
/**
* HDVICP_Configure(algHandle, iresHandle, cbFunctionPtr, cbArgs) :
* This API ties the ISR function to the HDVICP interrupt
* Also, creates the semaphore with 0 count and creates the Swi obj
*
* @param algHandle     Algorithm Handle
* @param iresHandle    IRES Handle
* @param cbFunctionPtr Pointer to the callback function
* @param cbArgs        Argument to be passed to the callback function
*
* @return           None
*/
/** ========================================================================== */
extern Hwi_Handle ti_sdo_fc_ires_hdvicp_HDVICP2_hwi0;
#ifdef TI_816X_BUILD
extern Hwi_Handle ti_sdo_fc_ires_hdvicp_HDVICP2_hwi1;
extern Hwi_Handle ti_sdo_fc_ires_hdvicp_HDVICP2_hwi2;
#endif

void HDVICP_Configure(IALG_Handle algHandle,
                      struct IRES_HDVICP2_Obj *iresHandle,
                      IRES_HDVICP2_CallbackFxn cbFunctionPtr, Void * cbArgs)
{
    Hwi_Params hwiParams;
    Swi_Params swiParams;
    Semaphore_Params semParams;
    Error_Block eb;

    Error_init(&eb);

    DBC_require(iresHandle);

    if (_first_time == 0)
    {
        // IVAHD_Standby_power_on_uboot_3ivahd();

        memset(_HDVICP2_context, 0x00, sizeof(_HDVICP2_context));
        _first_time = 1;
    }

    /*-----------------------------------------------------------------------*/
    /* Initialize the Handle to the MB processing function that is */
    /* required to be called by the ISR.  */
    /*-----------------------------------------------------------------------*/
    _HDVICP2_context[iresHandle->id].swiArg.algHandle = algHandle;

    /*-----------------------------------------------------------------------*/
    /* Initialize the global function ptr to the MB processing function */
    /* which will be called from the ISR.  */
    /*-----------------------------------------------------------------------*/
    _HDVICP2_context[iresHandle->id].swiArg.cbFunctionPtr = cbFunctionPtr;

    /*-----------------------------------------------------------------------*/
    /* Initialize the Argument to the MB processing function that is */
    /* required to be called by the ISR.  */
    /*-----------------------------------------------------------------------*/
    _HDVICP2_context[iresHandle->id].swiArg.cbArgs = cbArgs;
    _HDVICP2_context[iresHandle->id].swiArg.id = iresHandle->id;

    if (!_HDVICP2_context[iresHandle->id].initialized)
    {
        _HDVICP2_context[iresHandle->id].initialized = 1;

        /* create a Swi to be called to release the semaphore in "done" */
        Swi_Params_init(&swiParams);
        swiParams.arg0 = (UArg) (&_HDVICP2_context[iresHandle->id].swiArg);
        Swi_construct(&_HDVICP2_context[iresHandle->id].swiStruct,
                      IRESMAN_HDVICP2_SwiFunc, &swiParams, &eb);
        _HDVICP2_context[iresHandle->id].swiHandle =
            Swi_handle(&_HDVICP2_context[iresHandle->id].swiStruct);

        /* configure interrupt table with the ISR function */
        Hwi_Params_init(&hwiParams);
        hwiParams.arg = iresHandle->id;

        _HDVICP2_context[iresHandle->id].hwiHandle =
            Hwi_handle(&_HDVICP2_context[iresHandle->id].hwiStruct);

        /*--------------------------------------------------------------------*/
        /* Create the semaphore for indicating IVAHD-codec start/end.  */
        /* Create the semaphore with count = 0.  */
        /*--------------------------------------------------------------------*/

        Semaphore_Params_init(&semParams);
        Semaphore_construct(&_HDVICP2_context[iresHandle->id].semStruct, 0,
                            &semParams);
        _HDVICP2_context[iresHandle->id].semHandle =
            Semaphore_handle(&_HDVICP2_context[iresHandle->id].semStruct);

    }

    return;
}

/** ========================================================================== */
/**
* HDVICP_Wait(handle, iresHandle, yieldCtxt) :
* Pends for the semaphore that is posted by "done"
*
* @param handle        Algorithm Handle
* @param iresHandle    IRES Handle
* @param yieldCtxt     <TBD>
*
* @return           None
*/
/** ========================================================================== */
XDAS_UInt32 HDVICP_Wait(IALG_Handle handle, IRES_HDVICP2_Handle iresHandle,
                        IRES_YieldContext * yieldCtxt)
{

    DBC_require(iresHandle);
#ifdef UTILS_ENCDEC_HDVICP_PROFILE
    {
        UInt32 curTime = Utils_encdecGetTime();

        g_HDVICP_logTbl[iresHandle->id].totalAcquire2wait +=
            (curTime - g_HDVICP_logTbl[iresHandle->id].tempAcquireTime);
        g_HDVICP_logTbl[iresHandle->id].tempWaitTime = curTime;
    }
#endif
    /* pend for the semaphore ie. block on the semaphore */
    _HDVICP2_context[iresHandle->id].inIVAHDprocess = TRUE;	
    Semaphore_pend(_HDVICP2_context[iresHandle->id].semHandle,
                   BIOS_WAIT_FOREVER);
	
	_HDVICP2_context[iresHandle->id].inIVAHDprocess = FALSE;
    DBC_ensure(success);

    return (XDAS_TRUE);
}

/** ========================================================================== */
/**
* HDVICP_Done(handle, iresHandle) :
* This function will be called by the interrupt handler
* function when it detects an end-of-frame processing
*
* @param handle        Algorithm Handle
* @param iresHandle    IRES Handle
*
* @return           None
*/
/** ========================================================================== */

void HDVICP_Done(IALG_Handle handle, IRES_HDVICP2_Handle iresHandle)
{
    DBC_require(iresHandle);
#ifdef UTILS_ENCDEC_HDVICP_PROFILE
    {
        UInt32 curTime = Utils_encdecGetTime();

        g_HDVICP_logTbl[iresHandle->id].totalIsr2Done +=
            (curTime - g_HDVICP_logTbl[iresHandle->id].tempPrevTime);
        g_HDVICP_logTbl[iresHandle->id].totalWait2Done +=
            (curTime - g_HDVICP_logTbl[iresHandle->id].tempWaitTime);
        g_HDVICP_logTbl[iresHandle->id].tempPrevTime = curTime;
    }
#endif
    Semaphore_post(_HDVICP2_context[iresHandle->id].semHandle);
}

XDAS_Int32 IVAHD_Standby_power_on_uboot(IRES_HDVICP2_Handle iresHandle)
{

    unsigned int length = 0;

    /*------------------------------------------------------------------------*/
    /* Assigment of pointers */
    /* A generic code shall take all address as input parameters */
    /*------------------------------------------------------------------------*/
    volatile unsigned int *prcm_ivahd_icont_rst_cntl_addr =
        (unsigned int *) iresHandle->resetControlAddress;
    volatile unsigned int *icont1_itcm_base_addr =
        (unsigned int *) ((unsigned long) (iresHandle->registerBaseAddress) +
                          ICONT1_ITCM_OFFSET);
    volatile unsigned int *icont2_itcm_base_addr =
        (unsigned int *) ((unsigned long) (iresHandle->registerBaseAddress) +
                          ICONT2_ITCM_OFFSET);
    /**-----------------------------------------------------------------------*
     * Set IVAHD in reset mode to enable downloading of boot code             *
     * Please note that this state can be SKIPPED if IVAHD is alredy in reset *
     * state during uboot and reset is not de-asserted                        *
     * Set bit0 to 1 to put ICONT1 in reset state                             *
     * Set bit1 to 1 to put ICONT2 in reset state                             *
     *------------------------------------------------------------------------*/
    *prcm_ivahd_icont_rst_cntl_addr |= 0x00000003;

    /*------------------------------------------------------------------------*/
    /* Copy boot code to ICONT1 & INCOT2 memory */
    /*------------------------------------------------------------------------*/
    for (length = 0; length < LENGTH_BOOT_CODE; length++)
    {
        *icont1_itcm_base_addr++ = IVAHD_memory_wfi[length];
        *icont2_itcm_base_addr++ = IVAHD_memory_wfi[length];
    }
    /*------------------------------------------------------------------------*/
    /* Take IVAHD out of reset mode.  */
    /* Set bit0 to 0 to take ICONT1 out of reset state */
    /* Set bit1 to 0 to take ICONT1 out of reset state */
    /* This implies ICONT inside IVAHD will exectute WFI */
    /*------------------------------------------------------------------------*/
    *prcm_ivahd_icont_rst_cntl_addr &= 0xFFFFFFFC;

    /*------------------------------------------------------------------------*/
    /* As ICONT goes in WFI and there are no pending VDMA transction */
    /* entire IVAHD will be go in standby mode and PRCM will fully control */
    /* further managment of IVAHD power state */
    /*------------------------------------------------------------------------*/

    return (1);
}

#if 0
static Void delayLoop(Int delay_cnt)
{
    volatile int i;

    for (i = 0; i < delay_cnt; i++)
    {
        asm(" NOP ");
    }
}

Int g_ResetOnce = TRUE;

#define IVAHD_CALC_PID_ADDR_OFFSET (0x008D8000)
#define IVAHD_CALC_PID_VALUE       (0x4C020300)
#define IVAHD_SYSCTRL_ADDR_OFFSET  (0x0005A400)
#define IVAHD_SYSCTRL_STDBYST_ADDR_OFFSET  (IVAHD_SYSCTRL_ADDR_OFFSET + 0x58)

XDAS_UInt32 HDVICP_Reset(IALG_Handle handle, IRES_HDVICP2_Handle iresHandle)
{
    static Bool reset_done[3] = { FALSE, FALSE, FALSE };
    IArg key;
    volatile UInt32 *sysctrl_stdbyst_regs_addr =
        ((UInt32 *) ((UInt32) iresHandle->registerBaseAddress +
                     IVAHD_SYSCTRL_STDBYST_ADDR_OFFSET));

    key = Gate_enterSystem();

    if (TRUE == reset_done[iresHandle->id])
    {
        while ((*sysctrl_stdbyst_regs_addr & 0x7) != 7) ;
    }

    if ((FALSE == reset_done[iresHandle->id]) || (FALSE == g_ResetOnce))
    {

        Hwi_setFunc(ti_sdo_fc_ires_hdvicp_HDVICP2_hwi0, IRESMAN_HDVICP2_ISR, 0);
        Hwi_setFunc(ti_sdo_fc_ires_hdvicp_HDVICP2_hwi1, IRESMAN_HDVICP2_ISR1,
                    1);
        Hwi_setFunc(ti_sdo_fc_ires_hdvicp_HDVICP2_hwi2, IRESMAN_HDVICP2_ISR2,
                    2);

#ifndef PLATFORM_SIM
        {
            volatile unsigned int *ivahd_reset_addr;
            volatile unsigned int *ivahd_reset_status_addr;
            volatile unsigned int ivahd_calc_pid;

            ivahd_reset_addr = (unsigned int *) iresHandle->resetControlAddress;

            ivahd_reset_status_addr = (unsigned int *) ivahd_reset_addr + 1;

            *(ivahd_reset_status_addr - (0x610 / 4)) &= 0xFFFFFEFF;
            delayLoop(10);

            *ivahd_reset_addr |= 0x00000007;
            delayLoop(10);

            while (!(*ivahd_reset_status_addr & 0x00000007)) ;

            // *ivahd_reset_status_addr |= 0x00000007;
            delayLoop(10);

            *ivahd_reset_addr &= 0xFFFFFFFB;
            delayLoop(10);

            *(ivahd_reset_status_addr - (0x610 / 4)) |= 0x00000100;
            delayLoop(100);

            IVAHD_Standby_power_on_uboot(iresHandle);
            delayLoop(100);
            do
            {
                ivahd_calc_pid =
                    *(UInt32 *) (((UInt32) iresHandle->registerBaseAddress +
                                  IVAHD_CALC_PID_ADDR_OFFSET));

            } while (ivahd_calc_pid != IVAHD_CALC_PID_VALUE);
            reset_done[iresHandle->id] = TRUE;
            while ((*sysctrl_stdbyst_regs_addr & 0x7) != 7) ;
        }
#endif
    }
    Gate_leaveSystem(key);
    return (XDAS_TRUE);
}

#else

#define PRCM_BASE_ADDR          (0x48180000)
#define PRM_IVAHD0              (PRCM_BASE_ADDR + 0xC00)
#define PRM_IVAHD1              (PRCM_BASE_ADDR + 0xD00)
#define PRM_IVAHD2              (PRCM_BASE_ADDR + 0xE00)

#define WR_MEM_32(addr, data) *(volatile unsigned int*)(addr) =(unsigned int)(data)
#define RD_MEM_32(addr) *(volatile unsigned int*)(addr)

Int g_TurnOnPowerDomainForIVA_done = FALSE;

void TurnOnPowerDomainForIVA(void)
{
  /* PM_IVAHDx_PWRSTCTRL.POWERSTATE = 0x3; */
  //WR_MEM_32(PRM_IVAHD0 , 3);
  //WR_MEM_32(PRM_IVAHD1 , 3);
  //WR_MEM_32(PRM_IVAHD2 , 3);

  Hwi_setFunc(ti_sdo_fc_ires_hdvicp_HDVICP2_hwi0, IRESMAN_HDVICP2_ISR, 0);
#ifdef TI_816X_BUILD
  Hwi_setFunc(ti_sdo_fc_ires_hdvicp_HDVICP2_hwi1, IRESMAN_HDVICP2_ISR1, 1);
  Hwi_setFunc(ti_sdo_fc_ires_hdvicp_HDVICP2_hwi2, IRESMAN_HDVICP2_ISR2, 2);
#endif  
}


UInt RESET_COUNT[3];


XDAS_UInt32 HDVICP_Reset(IALG_Handle handle, IRES_HDVICP2_Handle iresHandle)
{
    IArg key;
    static Bool reset_done[3] = {FALSE,FALSE,FALSE};
    volatile unsigned int temp;
    unsigned int RM_IVAHD0_RSTCTRL =
       (unsigned int)iresHandle->resetControlAddress; //(PRM_IVAHD0 + 0x10)
    unsigned int RM_IVAHD0_RSTST =
       (RM_IVAHD0_RSTCTRL + 4); //(PRM_IVAHD0 + 0x14)
    unsigned int CM_IVAHD0_IVAHD_CLKCTRL =
       ((RM_IVAHD0_RSTCTRL - 0x610 ) + 0x20); //(PRCM_BASE_ADDR + 0x0620)
    unsigned int CM_IVAHD0_SL2_CLKCTRL =
       (CM_IVAHD0_IVAHD_CLKCTRL + 4); //(PRCM_BASE_ADDR + 0x0624)

    key= Gate_enterSystem ();
    RESET_COUNT[iresHandle->id]++;

    if (FALSE == reset_done[iresHandle->id]) {
        IVAHD_Standby_power_on_uboot(iresHandle);
        reset_done[iresHandle->id] = TRUE;
    }

    if (FALSE == g_TurnOnPowerDomainForIVA_done){
        TurnOnPowerDomainForIVA();
        g_TurnOnPowerDomainForIVA_done = TRUE;
    }

    /* CM_IVAHD0_IVAHD_CLKCTRL. MODULEMODE = 0x0;
    while(CM_IVAHD0_IVAHD_CLKCTRL.IDLEST != 0x3); */
    WR_MEM_32(CM_IVAHD0_IVAHD_CLKCTRL , 0);
    do{
        temp = RD_MEM_32(CM_IVAHD0_IVAHD_CLKCTRL);
        temp = ((temp >> 16) & 3);
    }while(temp != 3);
    /* CM_IVAHD0_SL2_CLKCTRL. MODULEMODE = 0x0;
       while(CM_IVAHD0_SL2_CLKCTRL.IDLEST != 0x3); */
    WR_MEM_32(CM_IVAHD0_SL2_CLKCTRL , 0);
    do{
        temp = RD_MEM_32(CM_IVAHD0_SL2_CLKCTRL);
        temp = ((temp >> 16) & 3);
    }while(temp != 3);
    /* RM_IVAHD0_RSTST     = IVA_RST3 | IVA_RST2 | IVA_RST1; : 0x7 :
       RM_IVAHD0_RSTCTRL = IVA_RST3 | IVA_RST2 | IVA_RST1; : 0x7 */
    WR_MEM_32(RM_IVAHD0_RSTST , 7);
    WR_MEM_32(RM_IVAHD0_RSTCTRL , 7);
    /* CM_IVAHD0_SL2_CLKCTRL. MODULEMODE = 0x2;
       CM_IVAHD0_IVAHD_CLKCTRL. MODULEMODE = 0x2; */
    WR_MEM_32(CM_IVAHD0_SL2_CLKCTRL , 2);
    WR_MEM_32(CM_IVAHD0_IVAHD_CLKCTRL , 2);
    /* RM_IVAHD0_RSTCTRL = !IVA_RST3 | IVA_RST2 | IVA_RST1; : 0x3 */
    WR_MEM_32(RM_IVAHD0_RSTCTRL , 3);

    /* Wait for !IDLE
    while(CM_IVAHD0_IVAHD_CLKCTRL.IDLEST != 0x0);
    while(CM_IVAHD0_SL2_CLKCTRL.IDLEST != 0x0); */
    do{
        temp = RD_MEM_32(CM_IVAHD0_IVAHD_CLKCTRL);
        temp = ((temp >> 16) & 3);
    }while(temp != 0);
    do{
        temp = RD_MEM_32(CM_IVAHD0_SL2_CLKCTRL);
        temp = ((temp >> 16) & 3);
    } while(temp != 0);

    /* while(RM_IVAHD0_RSTST.IVA_RST3 != 0x1); */
    do {
        temp = RD_MEM_32(RM_IVAHD0_RSTST);
        temp = ((temp >> 2) & 1);
    } while(temp != 1);

    Gate_leaveSystem(key);
    return (XDAS_TRUE);
}

#endif



