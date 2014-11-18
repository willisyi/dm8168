/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include "system_priv_common.h"
#include <mcfw/src_bios6/utils/utils_common.h>
#include <ti/sysbios/knl/Task.h>

#include <mcfw/src_bios6/alg/simcop/inc/cpisCore.h>
#include <mcfw/src_bios6/alg/simcop/inc/iss_init.h>

#include <ti/sysbios/knl/Semaphore.h>

typedef struct
{
    CPIS_Init cpisInitPrm;
    Semaphore_Handle simcopLock;

} System_SimcopObj;

System_SimcopObj gSystem_SimcopObj;

void System_simcopCacheWbInv(void *addr, Uint32 size, Bool wait)
{
    Cache_wbInv(addr, size, TRUE, NULL);
}

void System_lockSimcop(void *arg)
{
   	Semaphore_pend(gSystem_SimcopObj.simcopLock,BIOS_WAIT_FOREVER);
}


void System_unlockSimcop(void *arg)
{
   	Semaphore_post(gSystem_SimcopObj.simcopLock);	
}


Int32 System_initSimcop()
{
    Int32 status;
    Semaphore_Params simcopLockParams;

#ifdef SYSTEM_DEBUG
    Vps_printf(" %d: SYSTEM  : SIMCOP Init in progress !!!\n",
               Utils_getCurTimeInMsec());
#endif

    memset(&gSystem_SimcopObj.cpisInitPrm, 0, sizeof(gSystem_SimcopObj.cpisInitPrm));

   Semaphore_Params_init(&simcopLockParams);
   simcopLockParams.mode = ti_sysbios_knl_Semaphore_Mode_BINARY;

   gSystem_SimcopObj.simcopLock =
            Semaphore_create(1,&simcopLockParams, NULL);
   UTILS_assert(gSystem_SimcopObj.simcopLock != NULL);


    gSystem_SimcopObj.cpisInitPrm.cacheWbInv          = System_simcopCacheWbInv;
    gSystem_SimcopObj.cpisInitPrm.staticDmaAlloc      = 1;
    gSystem_SimcopObj.cpisInitPrm.maxNumDma           = 1;
    gSystem_SimcopObj.cpisInitPrm.maxNumProcFunc      = 1;
    gSystem_SimcopObj.cpisInitPrm.lock                = System_lockSimcop;
    gSystem_SimcopObj.cpisInitPrm.unlock              = System_unlockSimcop;
    gSystem_SimcopObj.cpisInitPrm.initFC              = CPIS_INIT_FC_ALL;
    gSystem_SimcopObj.cpisInitPrm.engineName          = "alg_server";
    gSystem_SimcopObj.cpisInitPrm.codecEngineHandle   = NULL;

    gSystem_SimcopObj.cpisInitPrm.memSize = CPIS_getMemSize(gSystem_SimcopObj.cpisInitPrm.maxNumProcFunc);

#ifdef SYSTEM_DEBUG
    Vps_printf(" %d: SYSTEM  : SIMCOP needs %d B of memory !!!\n",
               Utils_getCurTimeInMsec(),
               gSystem_SimcopObj.cpisInitPrm.memSize
                );
#endif

    gSystem_SimcopObj.cpisInitPrm.mem = (void *)malloc(gSystem_SimcopObj.cpisInitPrm.memSize);
    if(gSystem_SimcopObj.cpisInitPrm.mem == NULL)
       return FVID2_EFAIL;

    /* Initialize CPIS */
    status = CPIS_init(&gSystem_SimcopObj.cpisInitPrm);

    UTILS_assert(status==FVID2_SOK);

    {
        extern Task_Handle VICP_IP_RUN_hTsk;

        UTILS_assert(VICP_IP_RUN_hTsk!=NULL);

        status = Utils_prfLoadRegister( VICP_IP_RUN_hTsk, "SIMCOP");
        UTILS_assert(status==FVID2_SOK);
    }

#ifdef SYSTEM_DEBUG
    Vps_printf(" %d: SYSTEM  : SIMCOP Init in progress DONE !!!\n",
               Utils_getCurTimeInMsec());
#endif

    return status;
}

Int32 System_deInitSimcop()
{
#ifdef SYSTEM_DEBUG
    Vps_printf(" %d: SYSTEM  : SIMCOP De-Init in progress !!!\n",
               Utils_getCurTimeInMsec());
#endif

    /* CPIS De Init */
    CPIS_deInit();

    if(gSystem_SimcopObj.cpisInitPrm.mem!=NULL)
        free(gSystem_SimcopObj.cpisInitPrm.mem);

    Semaphore_delete(&gSystem_SimcopObj.simcopLock);

#ifdef SYSTEM_DEBUG
    Vps_printf(" %d: SYSTEM  : SIMCOP De-Init in progress DONE !!!\n",
               Utils_getCurTimeInMsec());
#endif

    return FVID2_SOK;
}

Int32 System_initIss()
{
   Int32 status = FVID2_SOK;

#ifdef SYSTEM_DEBUG
    Vps_printf(" %d: SYSTEM  : ISS Init in progress !!!\n",
               Utils_getCurTimeInMsec());
#endif

#ifdef SYSTEM_DEBUG
    Vps_printf(" %d: SYSTEM  : ISS Power-ON in progress !!!\n",
               Utils_getCurTimeInMsec());
#endif

    /* Power ON Iss */
    *(volatile UInt32*)0x48180D00 = 0x2; /* PM_ISP_PWRSTCTRL     */
    *(volatile UInt32*)0x48180D10 = 0x3; /* RM_ISP_RSTCTRL       */
    *(volatile UInt32*)0x48180700 = 0x2; /* CM_ISP_CLKSTCTRL     */
    *(volatile UInt32*)0x48180720 = 0x2; /* CM_ISP_ISP_CLKCTRL   */
    *(volatile UInt32*)0x48180724 = 0x2; /* CM_ISP_FDIF_CLKCTRL  */
    Task_sleep(10);

#ifdef SYSTEM_DEBUG
    Vps_printf(" %d: SYSTEM  : ISS Power-ON in progress DONE !!!\n",
               Utils_getCurTimeInMsec());
#endif

    status = Iss_init(NULL);
    UTILS_assert(status == 0);

#ifdef SYSTEM_DEBUG
    Vps_printf(" %d: SYSTEM  : ISS Init in progress DONE !!!\n",
               Utils_getCurTimeInMsec());
#endif

    System_initSimcop();

   return status;
}


Int32 System_deInitIss()
{
   Int32 status = FVID2_SOK;

    System_deInitSimcop();

#ifdef SYSTEM_DEBUG
    Vps_printf(" %d: SYSTEM  : ISS De-Init in progress !!!\n",
               Utils_getCurTimeInMsec());
#endif



    status = Iss_deInit(NULL);
    UTILS_assert(status == 0);

#ifdef SYSTEM_DEBUG
    Vps_printf(" %d: SYSTEM  : ISS De-Init in progress DONE !!!\n",
               Utils_getCurTimeInMsec());
#endif


   return status;
}


