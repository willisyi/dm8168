 /*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/
#define  ti_sysbios_family_arm_m3_Hwi__nolocalnames
#define  ti_sysbios_hal_Hwi__nolocalnames
#include <elf_linkage.h>
#include <xdc/std.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/Assert.h>
#include <xdc/runtime/Text.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/ipc/MultiProc.h>
#include <mcfw/interfaces/common_def/ti_vsys_common_def.h>
#include <ti/sysbios/family/arm/m3/Hwi.h>
#include <ti/psp/vps/vps.h>
#include <mcfw/src_bios6/utils/utils_trace.h>
#include <mcfw/src_bios6/links_common/system/system_priv_ipc.h>

#ifdef MIN
#undef MIN
#endif

#define MIN(a,b) ((a) < (b) ? (a) : (b))


#pragma DATA_SECTION(gExceptionInfo,".bss:exceptionContextNonCached");
TI_EXPORT VSYS_SLAVE_CORE_EXCEPTION_INFO_S VSYS_EXCEPTION_CONTEXT_SYMBOL_NAME =
{.exceptionActive = FALSE};


extern
Void utils_exception_asm_copy_regs(VSYS_SLAVE_CORE_EXCEPTION_CONTEXT_M3_S *excCtx);

static
Void utils_exception_copy_regs(ti_sysbios_family_arm_m3_Hwi_ExcContext *excCtxSrc,
                               VSYS_SLAVE_CORE_EXCEPTION_CONTEXT_M3_S *excCtxDst)
{
    excCtxDst->threadType      = excCtxSrc->threadType;
    excCtxDst->threadHandle    = excCtxSrc->threadHandle;
    excCtxDst->threadStack     = excCtxSrc->threadStack;
    excCtxDst->threadStackSize = excCtxSrc->threadStackSize;
    excCtxDst->r0              = excCtxSrc->r0;
    excCtxDst->r1              = excCtxSrc->r1;
    excCtxDst->r2              = excCtxSrc->r2;
    excCtxDst->r3              = excCtxSrc->r3;
    excCtxDst->r4              = excCtxSrc->r4;
    excCtxDst->r5              = excCtxSrc->r5;
    excCtxDst->r6              = excCtxSrc->r6;
    excCtxDst->r7              = excCtxSrc->r7;
    excCtxDst->r8              = excCtxSrc->r8;
    excCtxDst->r9              = excCtxSrc->r9;
    excCtxDst->r10             = excCtxSrc->r10;
    excCtxDst->r11             = excCtxSrc->r11;
    excCtxDst->r12             = excCtxSrc->r12;
    excCtxDst->sp              = excCtxSrc->sp;
    excCtxDst->lr              = excCtxSrc->lr;
    excCtxDst->pc              = excCtxSrc->pc;
    excCtxDst->psr             = excCtxSrc->psr;
    excCtxDst->ICSR            = excCtxSrc->ICSR;
    excCtxDst->MMFSR           = excCtxSrc->MMFSR;
    excCtxDst->BFSR            = excCtxSrc->BFSR;
    excCtxDst->UFSR            = excCtxSrc->UFSR;
    excCtxDst->HFSR            = excCtxSrc->HFSR;
    excCtxDst->DFSR            = excCtxSrc->DFSR;
    excCtxDst->MMAR            = excCtxSrc->MMAR;
    excCtxDst->BFAR            = excCtxSrc->BFAR;
    excCtxDst->AFSR            = excCtxSrc->AFSR;
}

static
Void utils_hw_exception_copy_info(ti_sysbios_family_arm_m3_Hwi_ExcContext *excCtx)
{
    char *tskName;
    char *coreName;

    gExceptionInfo.exceptionActive = TRUE;
    UTILS_COMPILETIME_ASSERT(sizeof(gExceptionInfo.excContextM3) == sizeof(*excCtx));
    utils_exception_copy_regs(excCtx,&gExceptionInfo.excContextM3);
    tskName = Utils_prfLoadGetTaskName(excCtx->threadHandle);
    if (tskName)
    {
        strncpy(gExceptionInfo.excTaskName,tskName,sizeof(gExceptionInfo.excTaskName));
        UTILS_COMPILETIME_ASSERT(sizeof(gExceptionInfo.excTaskName) > 0);
        gExceptionInfo.excTaskName[sizeof(gExceptionInfo.excTaskName) - 1] = 0;
    }
    else
    {
        gExceptionInfo.excTaskName[0] = 0;
    }
    coreName = MultiProc_getName(MultiProc_self());
    UTILS_assert(coreName != NULL);
    strncpy(gExceptionInfo.excCoreName,coreName,sizeof(gExceptionInfo.excCoreName));
    UTILS_COMPILETIME_ASSERT(sizeof(gExceptionInfo.excCoreName) > 0);
    gExceptionInfo.excCoreName[sizeof(gExceptionInfo.excCoreName) - 1] = 0;
    memcpy(gExceptionInfo.excStack,
           excCtx->threadStack,
           MIN(UTILS_ARRAYSIZE(gExceptionInfo.excStack),excCtx->threadStackSize));
    strncpy(gExceptionInfo.excInfo,"H/W EXCEPTION", sizeof(gExceptionInfo.excInfo));
    gExceptionInfo.excSiteInfo[0] = 0;
}

static
Void utils_exception_send_notify_to_host()
{
    UInt32 curProcId = System_getSelfProcId();

    System_ipcSendNotify(SYSTEM_HOST_LINK_ID_EXCEPTION_NOTIFY_BASE + curProcId);
}

 /*
 * By default, when an exception occurs, an ExcContext structure is allocated on the ISR stack
 * and filled in within the exception handler. If excContextBuffer is initialized by the user,
 * the ExcContext structure will be placed at that address instead. The buffer must be large
 * enough to contain an ExcContext structure.
 *
 * By default, when an exception occurs, a pointer to the base address of the stack being used
 * by the thread causing the exception is used. If excStackBuffer is initialized by the user,
 * the stack contents of the thread causing the exception will be copied to that address instead.
 * The buffer must be large enough to contain the largest task stack or ISR stack defined in the application.
 *
 */
 
Void Utils_exceptionHookFxn(ti_sysbios_family_arm_m3_Hwi_ExcContext *excCtx)
{

    Vps_rprintf ("Unhandled Exception:");
    if (excCtx->threadType == BIOS_ThreadType_Hwi){
            Vps_rprintf ("Exception occurred in ThreadType_HWI");
    } else if (excCtx->threadType == BIOS_ThreadType_Swi){
            Vps_rprintf ("Exception occurred in ThreadType_SWI");
    } else if (excCtx->threadType == BIOS_ThreadType_Task){
            Vps_rprintf ("Exception occurred in ThreadType_Task");
    } else if (excCtx->threadType == BIOS_ThreadType_Main){
            Vps_rprintf ("Exception occurred in ThreadType_Main");
    }

    Vps_rprintf ("handle: 0x%x.\n", excCtx->threadHandle);
    Vps_rprintf ("stack base: 0x%x.\n", excCtx->threadStack);
    Vps_rprintf ("stack size: 0x%x.\n", excCtx->threadStackSize);

    Vps_rprintf ("R0 = 0x%08x  R8  = 0x%08x\n", excCtx->r0, excCtx->r8);
    Vps_rprintf ("R1 = 0x%08x  R9  = 0x%08x\n", excCtx->r1, excCtx->r9);
    Vps_rprintf ("R2 = 0x%08x  R10 = 0x%08x\n", excCtx->r2, excCtx->r10);
    Vps_rprintf ("R3 = 0x%08x  R11 = 0x%08x\n", excCtx->r3, excCtx->r11);
    Vps_rprintf ("R4 = 0x%08x  R12 = 0x%08x\n", excCtx->r4, excCtx->r12);
    Vps_rprintf ("R5 = 0x%08x  SP(R13) = 0x%08x\n", excCtx->r5, excCtx->sp);
    Vps_rprintf ("R6 = 0x%08x  LR(R14) = 0x%08x\n", excCtx->r6, excCtx->lr);
    Vps_rprintf ("R7 = 0x%08x  PC(R15) = 0x%08x\n", excCtx->r7, excCtx->pc);
    Vps_rprintf ("PSR = 0x%08x\n", excCtx->psr);
    Vps_rprintf ("ICSR = 0x%08x\n", ti_sysbios_family_arm_m3_Hwi_nvic.ICSR);
    Vps_rprintf ("MMFSR = 0x%02x\n", ti_sysbios_family_arm_m3_Hwi_nvic.MMFSR);
    Vps_rprintf ("BFSR = 0x%02x\n", ti_sysbios_family_arm_m3_Hwi_nvic.BFSR);
    Vps_rprintf ("UFSR = 0x%04x\n", ti_sysbios_family_arm_m3_Hwi_nvic.UFSR);
    Vps_rprintf ("HFSR = 0x%08x\n", ti_sysbios_family_arm_m3_Hwi_nvic.HFSR);
    Vps_rprintf ("DFSR = 0x%08x\n", ti_sysbios_family_arm_m3_Hwi_nvic.DFSR);
    Vps_rprintf ("MMAR = 0x%08x\n", ti_sysbios_family_arm_m3_Hwi_nvic.MMAR);
    Vps_rprintf ("BFAR = 0x%08x\n", ti_sysbios_family_arm_m3_Hwi_nvic.BFAR);
    Vps_rprintf ("AFSR = 0x%08x\n", ti_sysbios_family_arm_m3_Hwi_nvic.AFSR);
    Vps_rprintf ("Terminating Execution...");
    utils_hw_exception_copy_info(excCtx);
    utils_exception_send_notify_to_host();
}


static
Void utils_sw_exception_copy_info()
{
    char *tskName;
    char *coreName;
    ti_sysbios_knl_Task_Handle tskHndl = ti_sysbios_knl_Task_self();
    ti_sysbios_knl_Task_Stat tskStat;


    if (tskHndl != NULL)
    {
        ti_sysbios_knl_Task_stat(tskHndl,&tskStat);
        gExceptionInfo.excContextM3.threadHandle = tskHndl;
        gExceptionInfo.excContextM3.threadStack  = tskStat.stack;
        gExceptionInfo.excContextM3.threadStackSize = tskStat.stackSize;
    }
    else
    {
        gExceptionInfo.excContextM3.threadHandle = NULL;
        gExceptionInfo.excContextM3.threadStack  = NULL;
        gExceptionInfo.excContextM3.threadStackSize = 0;
    }
    gExceptionInfo.excContextM3.threadType      = BIOS_getThreadType();
    gExceptionInfo.exceptionActive = FALSE;
    utils_exception_asm_copy_regs(&gExceptionInfo.excContextM3);
    tskName = Utils_prfLoadGetTaskName(tskHndl);
    if (tskName)
    {
        strncpy(gExceptionInfo.excTaskName,tskName,sizeof(gExceptionInfo.excTaskName));
        UTILS_COMPILETIME_ASSERT(sizeof(gExceptionInfo.excTaskName) > 0);
        gExceptionInfo.excTaskName[sizeof(gExceptionInfo.excTaskName) - 1] = 0;
    }
    else
    {
        gExceptionInfo.excTaskName[0] = 0;
    }
    coreName = MultiProc_getName(MultiProc_self());
    UTILS_assert(coreName != NULL);
    strncpy(gExceptionInfo.excCoreName,coreName,sizeof(gExceptionInfo.excCoreName));
    UTILS_COMPILETIME_ASSERT(sizeof(gExceptionInfo.excCoreName) > 0);
    gExceptionInfo.excCoreName[sizeof(gExceptionInfo.excCoreName) - 1] = 0;
    if (tskHndl != NULL)
    {
        memcpy(gExceptionInfo.excStack,
               tskStat.stack,
               MIN(UTILS_ARRAYSIZE(gExceptionInfo.excStack),tskStat.stackSize));
    }
}


Void Utils_errorRaiseHook(Error_Block *eb)
{
    if (eb)
    {
        String errMsg;
        Error_Data *data;
        Char *errSite = &gExceptionInfo.excSiteInfo[0];
        Char **errSitePtr = &errSite;
        static Bool exceptionServiced = FALSE;

        if (FALSE == exceptionServiced)
        {
            errMsg = Error_getMsg(eb);
            data   = Error_getData(eb);
            Text_putSite(Error_getSite(eb), errSitePtr,sizeof(gExceptionInfo.excSiteInfo));
            UTILS_COMPILETIME_ASSERT(sizeof(gExceptionInfo.excSiteInfo) > 0);
            gExceptionInfo.excSiteInfo[sizeof(gExceptionInfo.excSiteInfo) - 1] = 0;
            /*
             * If this is an Assert thread, defang Gate threadtype check
             */
            if (Error_getId(eb) == xdc_runtime_Assert_E_assertFailed)
            {

                /* lock task scheduler */
                Task_disable();
                exceptionServiced = TRUE;
                Vps_rprintf("!!!XDC RUNTIME ASSERT FAILED");
                UTILS_COMPILETIME_ASSERT(sizeof(gExceptionInfo.excInfo) > 0);
                gExceptionInfo.excInfo[0] = 0;
                utils_sw_exception_copy_info();
                if (errMsg)
                {
                    snprintf(gExceptionInfo.excInfo,
                             sizeof(gExceptionInfo.excInfo),
                             errMsg,
                             data->arg[0],
                             data->arg[1]);
                }
                Vps_rprintf("xdc.runtime.Error @ %s",gExceptionInfo.excSiteInfo);
                if (errMsg)
                {
                    Vps_rprintf(errMsg,data->arg[0], data->arg[1]);
                }
                utils_exception_send_notify_to_host();
            }
        }
    }
    Error_print(eb);
}

Void Utils_assertHook(Char *file,Int lineNum,Char *assertInfo)
{

    snprintf(gExceptionInfo.excSiteInfo,
             sizeof(gExceptionInfo.excSiteInfo),
             "%s:%d",file,lineNum);
    UTILS_COMPILETIME_ASSERT(sizeof(gExceptionInfo.excSiteInfo) > 0);
    gExceptionInfo.excSiteInfo[sizeof(gExceptionInfo.excSiteInfo) - 1] = 0;

    snprintf(gExceptionInfo.excInfo,
             sizeof(gExceptionInfo.excInfo),
             "%s",assertInfo);
    UTILS_COMPILETIME_ASSERT(sizeof(gExceptionInfo.excInfo) > 0);
    gExceptionInfo.excSiteInfo[sizeof(gExceptionInfo.excInfo) - 1] = 0;
    utils_sw_exception_copy_info();
    utils_exception_send_notify_to_host();
}
