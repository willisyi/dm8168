/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#ifndef _SYSTEM_PRIV_M3VPSS_H_
#define _SYSTEM_PRIV_M3VPSS_H_

#include <mcfw/src_bios6/links_common/system/system_priv_common.h>

#include <mcfw/interfaces/link_api/system.h>
#include <mcfw/interfaces/link_api/captureLink.h>
#include <mcfw/interfaces/link_api/deiLink.h>
#include <mcfw/interfaces/link_api/nsfLink.h>
#include <mcfw/interfaces/link_api/displayLink.h>
#include <mcfw/interfaces/link_api/nullLink.h>
#include <mcfw/interfaces/link_api/grpxLink.h>
#include <mcfw/interfaces/link_api/dupLink.h>
#include <mcfw/interfaces/link_api/sclrLink.h>
#include <mcfw/interfaces/link_api/swMsLink.h>
#include <mcfw/interfaces/link_api/mergeLink.h>
#include <mcfw/interfaces/link_api/nullSrcLink.h>
#include <mcfw/interfaces/link_api/ipcLink.h>
#include <mcfw/interfaces/link_api/systemLink_m3vpss.h>
#include <mcfw/interfaces/link_api/systemLink_m3video.h>
#include <mcfw/interfaces/link_api/avsync.h>
#include <mcfw/interfaces/link_api/avsync_rtos.h>
#include <mcfw/interfaces/link_api/selectLink.h>
#include <mcfw/interfaces/link_api/algLink.h>
#include <mcfw/interfaces/common_def/ti_vdis_common_def.h>
#include <mcfw/interfaces/link_api/mpSclrLink.h>

#include <ti/psp/vps/vps.h>
#include <ti/psp/vps/vps_capture.h>
#include <ti/psp/vps/vps_display.h>
#include <ti/psp/vps/vps_graphics.h>
#include <ti/psp/vps/vps_displayCtrl.h>
#include <ti/psp/vps/vps_m2m.h>
#include <ti/psp/vps/vps_m2mDei.h>
#include <ti/psp/vps/vps_m2mNsf.h>
#include <ti/psp/vps/vps_m2mSc.h>
#include <ti/psp/devices/vps_videoDecoder.h>
#include <ti/psp/devices/vps_sii9022a.h>
#include <ti/psp/devices/vps_sii9134.h>
#include <ti/psp/devices/vps_thsfilters.h>
#include <ti/psp/platforms/vps_platform.h>
#include <ti/psp/devices/vps_tvp5158.h>

#define SYSTEM_VIP_0    (0)
#define SYSTEM_VIP_1    (1)
#define SYSTEM_VIP_MAX  (2)

#define SYSTEM_BLANK_FRAME_WIDTH            (1920+32)
#define SYSTEM_BLANK_FRAME_HEIGHT           (1080+24)

#define SYSTEM_BLANK_FRAME_BYTES_PER_PIXEL  (2)

#define VPDMA_LIST_ATTR         0x4810D008
#define VPDMA_LIST_STAT_SYNC    0x4810D00C
#define VPDMA_TOTAL_LIST_NO     8

#if defined (TI_814X_BUILD) || defined (TI_8107_BUILD)
#define CM_HDVPSS_HDVPSS_CLK_CTRL   0x48180820
#define VPS_MODULE_CLK              CM_HDVPSS_HDVPSS_CLK_CTRL
#endif

#ifdef TI_816X_BUILD
#define CM_ACTIVE_HDDSS_CLKCTRL     0x48180424
#define VPS_MODULE_CLK              CM_ACTIVE_HDDSS_CLKCTRL
#endif


static char *gVpss_cpuVer[SYSTEM_PLATFORM_CPU_REV_MAX] = {
    "ES1.0",
    "ES1.1",
    "ES2.0",
    "ES2.1",
    "UNKNOWN",
};

/**
 * \brief System Task Descriptor
 *
 */
typedef struct {
    FVID2_Handle fvidDisplayCtrl;
    Vps_DcConfig displayCtrlCfg;
    FVID2_Handle systemDrvHandle;
    Vps_SystemVPllClk vpllCfg[SYSTEM_VPLL_OUTPUT_MAX_VENC];

    FVID2_Handle hdmiHandle;

    UInt32 displayRes[SYSTEM_DC_MAX_VENC];
    Bool tilerEnable;

    /* locks and flags for VIP when doing reset */
    Semaphore_Handle vipLock[SYSTEM_VIP_MAX];
    Bool vipResetFlag[SYSTEM_VIP_MAX];

    UInt32 displayUnderflowCount[4];

    UInt8 *nonTiledBlankFrameAddr;
    UInt8 *tiledBlankFrameAddr;

    Bool enableConfigExtVideoEncoder;

} System_VpssObj;

extern System_VpssObj gSystem_objVpss;

Int32 System_hdmiCreate(UInt32 displayRes, System_PlatformBoardId  boardId);
Int32 System_hdmiStart(UInt32 displayRes, System_PlatformBoardId boardId);
Int32 System_hdmiStop();
Int32 System_hdmiDelete();
Int32 System_dispSetPixClk();

Int32 System_dispCheckStopList();


Int32 System_lockVip(UInt32 vipInst);
Int32 System_unlockVip(UInt32 vipInst);
Int32 System_setVipResetFlag(UInt32 vipInst);
Bool System_clearVipResetFlag(UInt32 vipInst);

Int32 System_displayUnderflowPrint(Bool runTimePrint, Bool clearAll);
Int32 System_displayUnderflowCheck(Bool clearAll);

Int32 System_displayCtrlInit(VDIS_PARAMS_S * pPrm);
Int32 System_displayCtrlDeInit(VDIS_PARAMS_S * pPrm);
Int32 System_displayCtrlSetVencOutput(VDIS_DEV_PARAM_S * pPrm);

Int32 System_videoResetVideoDevices();
UInt8 System_getVidDecI2cAddr(UInt32 vidDecId, UInt32 vipInstId);
System_PlatformBoardRev System_getBaseBoardRev();
System_PlatformBoardRev System_getDcBoardRev();
System_PlatformBoardId System_getBoardId();
Int32 System_ths7360SetSfParams(System_Ths7360SfCtrl ths7360SfCtrl);
Int32 System_ths7360SetSdParams(System_ThsFilterCtrl ths7360SdCtrl);
Int32 System_platformSelectHdCompClkSrc(System_VPllOutputClk clkSrc);
Int32 System_platformSelectHdCompSyncSrc(System_HdCompSyncSource syncSrc, UInt32 enable);
Int32 GrpxLink_start(UInt32 linkId);
Int32 GrpxLink_stop(UInt32 linkId);

Int32 System_getBlankFrame(FVID2_Frame *pFrame);
Int32 System_getOutSize(UInt32 outRes, UInt32 * width, UInt32 * height);


/* one time allocation of a blank frame for display and SW MS */
Int32 System_allocBlankFrame();

/* one time free of the balnk frame */
Int32 System_freeBlankFrame();

#endif
