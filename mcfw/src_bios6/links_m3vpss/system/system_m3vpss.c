/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include "system_priv_m3vpss.h"
#include <mcfw/src_bios6/links_common/system/system_priv_common.h>
#include <mcfw/src_bios6/utils/utils_tiler_allocator.h>
#include <mcfw/src_bios6/utils/utils_dmtimer.h>
#include <mcfw/src_bios6/utils/utils_dma.h>
#include <ti/psp/devices/vps_device.h>
#include <ti/psp/devices/tvp5158/vpsdrv_tvp5158.h>
#include <ti/psp/devices/tvp5158/vpsdrv_tvp5158.h>
#include <mcfw/interfaces/link_api/avsync_rtos.h>

System_VpssObj gSystem_objVpss;

volatile int waitFlag = 0;

void wait (void)
{
    while (waitFlag == 1) ;
}

Int32 System_init()
{
    Int32 status;

#ifdef SYSTEM_DEBUG
    const Char *versionStr;
#endif
    Vps_PlatformDeviceInitParams initPrms;
    Semaphore_Params semParams;
    Vps_PlatformInitParams platformInitPrms;

#ifdef SYSTEM_DEBUG
    Vps_printf(" %d: SYSTEM  : System VPSS Init in progress !!!\n",
               Utils_getCurTimeInMsec());
#endif

    System_dispCheckStopList();

    Utils_tilerAllocatorInit();

#ifdef SYSTEM_DEBUG_TILER_ALLOC
    Utils_tilerDebugLogEnable(TRUE);
#endif

#if defined(TI_814X_BUILD)
    /* Need to set this bit only for ti814x to support tied vencs, pin mux settings */
    /* Need to call this before accessing HD VENC registers in FVID2_init()
        Other wise it may hang in access of HDVENC registers
    */
    (* (UInt32 *)0x481C52C8) = 0x01000000;
#endif

    platformInitPrms.isPinMuxSettingReq = TRUE;
    status = Vps_platformInit(&platformInitPrms);
    UTILS_assert(status == 0);

    Utils_dmTimerInit();
    IpcOutM3Link_init();
    IpcInM3Link_init();
    IpcFramesInLink_init();
    IpcFramesOutLink_init();
#ifdef SYSTEM_DEBUG
    /*
     * Get the version string
     */
    versionStr = FVID2_getVersionString();
    Vps_printf(" %d: SYSTEM : HDVPSS Drivers Version: %s\n",
               Utils_getCurTimeInMsec(), versionStr);
#endif

#ifdef SYSTEM_DEBUG
    Vps_printf(" %d: SYSTEM  : FVID2 Init in progress !!!\n",
               Utils_getCurTimeInMsec());
#endif
    /*
     * FVID2 system init
     */
    status = FVID2_init(NULL);
    UTILS_assert(status == 0);

#ifdef SYSTEM_DEBUG
    Vps_printf(" %d: SYSTEM  : FVID2 Init in progress DONE !!!\n",
               Utils_getCurTimeInMsec());
#endif


#ifdef SYSTEM_DEBUG
    Vps_printf(" %d: SYSTEM  : Device Init in progress !!!\n",
               Utils_getCurTimeInMsec());
#endif
#if defined(TI_814X_BUILD) || defined(TI_8107_BUILD)
  #ifdef TI814X_EVM_WITHOUT_VS_CARD
    initPrms.isI2cInitReq = FALSE;
    initPrms.isI2cProbingReq = FALSE;
  #else
    initPrms.isI2cInitReq = TRUE;
    initPrms.isI2cProbingReq = TRUE;
  #endif
#else
    initPrms.isI2cInitReq = TRUE;
    initPrms.isI2cProbingReq = TRUE;
#endif

#ifndef SYSTEM_USE_VIDEO_DECODER
    initPrms.isI2cInitReq = FALSE;
    initPrms.isI2cProbingReq = FALSE;
#endif


    Vps_printf("\ninitPrms.isI2cInitReq = %d\n", initPrms.isI2cInitReq);
    Vps_printf("\ninitPrms.isI2cInitReq = %d\n", initPrms.isI2cProbingReq);
    status = Vps_platformDeviceInit(&initPrms);
    UTILS_assert(status == 0);

#ifdef SYSTEM_DEBUG
    Vps_printf(" %d: SYSTEM  : Device Init in progress DONE !!!\n",
               Utils_getCurTimeInMsec());
#endif

#ifdef TI816X_DVR
    status = Vps_tvp5158Init();
#endif

#ifdef SYSTEM_DEBUG_VIP_RES_ALLOC
   /*
     * enable logs from VIP resource allocator
     */
   Vcore_vipResDebugLogEnable(TRUE);
#endif

   Semaphore_Params_init(&semParams);
   semParams.mode = Semaphore_Mode_BINARY;

   gSystem_objVpss.vipLock[SYSTEM_VIP_0] =
        Semaphore_create(1u, &semParams, NULL);
   UTILS_assert(gSystem_objVpss.vipLock[SYSTEM_VIP_0] != NULL);

   Semaphore_Params_init(&semParams);
   semParams.mode = Semaphore_Mode_BINARY;

   gSystem_objVpss.vipLock[SYSTEM_VIP_1] =
       Semaphore_create(1u, &semParams, NULL);
   UTILS_assert(gSystem_objVpss.vipLock[SYSTEM_VIP_1] != NULL);

   System_clearVipResetFlag(SYSTEM_VIP_0);
   System_clearVipResetFlag(SYSTEM_VIP_1);

   System_allocBlankFrame();

#ifdef SYSTEM_DEBUG
    Vps_printf(" %d: SYSTEM  : System VPSS Init Done !!!\n", Utils_getCurTimeInMsec());
#endif

   Utils_dmaInit();

   System_initLinks();

#ifdef TI_8107_BUILD

    #if 0
    Vps_printf (" %d: SYSTEM  : Set Static L3 pressure for Ducati as High\n", Utils_getCurTimeInMsec());
    Utils_setL3Pressure (UTILS_INITIATOR_DUCATI, UTILS_L3_PRESSURE_HIGH);
    #endif

#endif

    #if 1
    Vps_printf (" %d: SYSTEM  : Set Static L3 pressure for HDVPSS as High\n", Utils_getCurTimeInMsec());
    Utils_setL3Pressure (UTILS_INITIATOR_HDVPSS0, UTILS_L3_PRESSURE_HIGH);
    Utils_setL3Pressure (UTILS_INITIATOR_HDVPSS1, UTILS_L3_PRESSURE_HIGH);
    #endif

   return status;
}


Int32 System_deInit()
{
   Int32 status = FVID2_SOK;
   IpcInM3Link_deInit();
   IpcOutM3Link_deInit();
   IpcFramesInLink_deInit();
   IpcFramesOutLink_deInit();

   Utils_dmaDeInit();

   Utils_dmTimerDeInit();
   System_deInitLinks();

#ifdef SYSTEM_DEBUG
   Vps_printf(" %d: SYSTEM  : System VPSS De-Init in progress !!!\n", Utils_getCurTimeInMsec());
#endif

   System_freeBlankFrame();

   Semaphore_delete(&gSystem_objVpss.vipLock[SYSTEM_VIP_0]);
   Semaphore_delete(&gSystem_objVpss.vipLock[SYSTEM_VIP_1]);


#ifdef SYSTEM_DEBUG
   Vps_printf(" %d: SYSTEM  : Platform Device De-init in progress !!!\n", Utils_getCurTimeInMsec());
#endif

#ifdef TI816X_DVR
   status = Vps_tvp5158DeInit();
   UTILS_assert(status == FVID2_SOK);
#endif

   Vps_platformDeviceDeInit();

#ifdef SYSTEM_DEBUG
   Vps_printf(" %d: SYSTEM  : FVID2 De-init in progress !!!\n", Utils_getCurTimeInMsec());
#endif

   /*
     * FVID2 system de-init
     */
   FVID2_deInit(NULL);

#ifdef SYSTEM_DEBUG
   Vps_printf(" %d: SYSTEM  : Platform De-init in progress !!!\n", Utils_getCurTimeInMsec());
#endif

   Vps_platformDeInit();

#ifdef SYSTEM_USE_TILER
   Utils_tilerAllocatorDeInit();
#endif

#ifdef SYSTEM_DEBUG
   Vps_printf(" %d: SYSTEM  : System VPSS De-Init Done !!!\n",

              Utils_getCurTimeInMsec());
#endif

   return status;
}


Void System_initLinks()
{

   Vps_printf(" %d: SYSTEM  : Initializing Links !!! \r\n", Utils_getCurTimeInMsec());
   System_memPrintHeapStatus();

   CaptureLink_init();
   NsfLink_init();
   DeiLink_init();
   DisplayLink_init();
   NullLink_init();
   NullSrcLink_init();
   DupLink_init();
   SclrLink_init();
   SwMsLink_init();
   MergeLink_init();
   SelectLink_init();
   GrpxLink_init();
   AvsyncLink_init();

#ifdef TI_816X_BUILD
   MpSclrLink_init();
#endif /* TI_816X_BUILD */

   Vps_printf(" %d: SYSTEM  : Initializing Links ... DONE !!! \r\n", Utils_getCurTimeInMsec());
}


Void System_deInitLinks()
{


   Vps_printf(" %d: SYSTEM  : De-Initializing Links !!! \r\n", Utils_getCurTimeInMsec());

   GrpxLink_deInit();
   SelectLink_deInit();
   MergeLink_deInit();
   NullLink_deInit();
   DisplayLink_deInit();
   NullSrcLink_deInit();
   DeiLink_deInit();
   NsfLink_deInit();
   CaptureLink_deInit();
   DupLink_deInit();
   SclrLink_deInit();
   SwMsLink_deInit();
   AvsyncLink_deInit();

#ifdef TI_816X_BUILD
   MpSclrLink_deInit();
#endif /* TI_816X_BUILD */

   System_memPrintHeapStatus();

   Vps_printf(" %d: SYSTEM  : De-Initializing Links ... DONE !!! \r\n", Utils_getCurTimeInMsec());
}


Int32 System_lockVip(UInt32 vipInst)
{
   if (vipInst < SYSTEM_VIP_MAX)
   {
       Semaphore_pend(gSystem_objVpss.vipLock[vipInst], BIOS_WAIT_FOREVER);
   }

   return FVID2_SOK;
}


Int32 System_unlockVip(UInt32 vipInst)
{

   if (vipInst < SYSTEM_VIP_MAX)
   {
       Semaphore_post(gSystem_objVpss.vipLock[vipInst]);
   }

   return FVID2_SOK;
}


Int32 System_setVipResetFlag(UInt32 vipInst)
{

   if (vipInst < SYSTEM_VIP_MAX)
   {
       gSystem_objVpss.vipResetFlag[vipInst] = TRUE;
   }

   return FVID2_SOK;
}


Bool System_clearVipResetFlag(UInt32 vipInst)
{

   Bool isReset = FALSE;

   if (vipInst < SYSTEM_VIP_MAX)
   {
       isReset = gSystem_objVpss.vipResetFlag[vipInst];
       gSystem_objVpss.vipResetFlag[vipInst] = FALSE;
   }

   return isReset;

}


Int32 System_videoResetVideoDevices()
{

   Int32 status = FVID2_SOK;


   return status;
}


UInt8 System_getVidDecI2cAddr(UInt32 vidDecId, UInt32 vipInstId)
{

   UInt8 devAddr = NULL;

#ifdef TI814X_DVR
   UInt8 devAddrTvp5158[VPS_CAPT_INST_MAX] = { 0x5b, 0x5a, 0x59, 0x58 };

   switch (vidDecId)
   {
       case FVID2_VPS_VID_DEC_TVP5158_DRV:
           devAddr = devAddrTvp5158[vipInstId];
           break;

       default:
           break;
   }
#endif

#ifdef TI8107_DVR
   UInt8 devAddrTvp5158[VPS_CAPT_INST_MAX] = { 0x58, 0x59, 0x5a, 0x5b };

   switch (vidDecId)
   {
       case FVID2_VPS_VID_DEC_TVP5158_DRV:
           devAddr = devAddrTvp5158[vipInstId];
           break;

       default:
           break;
   }
#endif


#ifdef TI816X_DVR
   UInt8 devAddrTvp5158[VPS_CAPT_INST_MAX] = { 0x5c, 0x5d, 0x5e, 0x5f };
   UInt8 devAddrSii9135[VPS_CAPT_INST_MAX] = { 0x31, 0x00, 0x30, 0x00 };
   UInt8 devAddrTvp7002[VPS_CAPT_INST_MAX] = { 0x5d, 0x00, 0x5c, 0x00 };

   // GT_assert( GT_DEFAULT_MASK, vipInstId<VPS_CAPT_INST_MAX);

   switch (vidDecId)
   {
       case FVID2_VPS_VID_DEC_TVP5158_DRV:
           devAddr = devAddrTvp5158[vipInstId];
           break;

       case FVID2_VPS_VID_DEC_SII9135_DRV:
           devAddr = devAddrSii9135[vipInstId];
           break;

       case FVID2_VPS_VID_DEC_TVP7002_DRV:
           devAddr = devAddrTvp7002[vipInstId];
           break;

       default:
           break;
           // GT_assert( GT_DEFAULT_MASK, 0);
   }
#endif

#if defined (TI816X_EVM) || defined (TI816X_CZ) || defined (TI816X_ETV) || defined (TI814X_EVM) || defined (TI810X_EVM)

   devAddr = Vps_platformGetVidDecI2cAddr(vidDecId, vipInstId);
#endif

   return (devAddr);

}


System_PlatformBoardRev System_getBaseBoardRev()
{
#if defined (TI816X_DVR)
   return SYSTEM_PLATFORM_BOARD_DVR_REV_NONE;
#else
   return (System_PlatformBoardRev)(Vps_platformGetBaseBoardRev());
#endif

}


System_PlatformBoardRev System_getDcBoardRev()
{
#if defined (TI816X_DVR)
   return SYSTEM_PLATFORM_BOARD_DVR_REV_NONE;
#else
   return (System_PlatformBoardRev)(Vps_platformGetDcBoardRev());
#endif
}


System_PlatformBoardId System_getBoardId()
{
#if defined (TI816X_DVR)
   return SYSTEM_PLATFORM_BOARD_DVR;
#else
   return (System_PlatformBoardId)(Vps_platformGetBoardId());
#endif
}


Int32 System_ths7360SetSfParams(System_Ths7360SfCtrl ths7360SfCtrl)
{
   Int32 status = FVID2_SOK;

#if defined(TI816X_EVM) || defined(TI8107_EVM) || defined(TI8107_DVR)
   status = Vps_ths7360SetSfParams((Vps_Ths7360SfCtrl)ths7360SfCtrl);
#endif

   return status;
}


Int32 System_ths7360SetSdParams(System_ThsFilterCtrl ths7360SdCtrl)
{

   Int32 status = FVID2_SOK;

#if defined(TI816X_EVM) || defined(TI8107_EVM) || defined(TI8107_DVR)
   status = Vps_ths7360SetSdParams((Vps_ThsFilterCtrl)ths7360SdCtrl);
#endif

   return status;
}

Int32 System_platformSelectHdCompClkSrc(System_VPllOutputClk clkSrc)
{
    Int32 status = FVID2_SOK;

#if defined(TI_8107_BUILD)
	status = Vps_platformSelectHdCompClkSrc((Vps_VPllOutputClk)clkSrc);
#endif

    return (status);
}

Int32 System_platformSelectHdCompSyncSrc(System_HdCompSyncSource syncSrc,
					UInt32 enable)
{
    Int32 status = FVID2_SOK;

#ifdef TI816X_DVR
    if (Vps_platformGetCpuRev() >= SYSTEM_PLATFORM_CPU_REV_2_0) {
        /*configure the pins for the
        HDCOMP Discrete sync output for PG2.0 only
        this should not be done here since pinmux should be configured from A8
        but in the current SW arch, M3 still is used to configure the pin mux
        so we have to do here instead from A8 board level.
        will move if M3 is not used to do the pinmux*/
        if (syncSrc == SYSTEM_HDCOMP_SYNC_SRC_DVO1) {
           (* (UInt32 *)0x4814086c) = 0x02;
           (* (UInt32 *)0x48140870) = 0x02;
        } else {
           (* (UInt32 *)0x48140894) = 0x00000001;
           (* (UInt32 *)0x48140898) = 0x00000001;
        }
        status = Vps_platformSelectHdCompSyncSource((Vps_PlatformHdCompSyncSource)syncSrc,
                                                    enable);
    }
#endif

    return status;
}
Int32 System_allocBlankFrame()
{
    UInt32 memSize;
    Int32 i;

    memSize = SYSTEM_BLANK_FRAME_WIDTH*SYSTEM_BLANK_FRAME_HEIGHT*SYSTEM_BLANK_FRAME_BYTES_PER_PIXEL;

    gSystem_objVpss.nonTiledBlankFrameAddr = Utils_memAlloc(memSize, VPS_BUFFER_ALIGNMENT*2);

    UTILS_assert(gSystem_objVpss.nonTiledBlankFrameAddr!=NULL);

    /* initialize the blank frame to be black color (0x00800080: black in YUYV) */
    memset(gSystem_objVpss.nonTiledBlankFrameAddr, 0x80, memSize);
    for (i = 0; i < memSize/SYSTEM_BLANK_FRAME_BYTES_PER_PIXEL; i++)
    {
        *(gSystem_objVpss.nonTiledBlankFrameAddr + i*2) = 0x00;
    }

    return 0;
}

Int32 System_getBlankFrame(FVID2_Frame *pFrame)
{
    memset(pFrame, 0, sizeof(*pFrame));

    pFrame->addr[0][0] = gSystem_objVpss.nonTiledBlankFrameAddr;
    pFrame->addr[0][1] = gSystem_objVpss.nonTiledBlankFrameAddr;

    return 0;
}

Int32 System_freeBlankFrame()
{
    UInt32 memSize;

    memSize = SYSTEM_BLANK_FRAME_WIDTH*SYSTEM_BLANK_FRAME_HEIGHT*SYSTEM_BLANK_FRAME_BYTES_PER_PIXEL;

    Utils_memFree(gSystem_objVpss.nonTiledBlankFrameAddr, memSize);

    return 0;
}

Int32 System_getOutSize(UInt32 outRes, UInt32 * width, UInt32 * height)
{
    Vsys_getResSize(outRes, width, height);
    return 0;
}
