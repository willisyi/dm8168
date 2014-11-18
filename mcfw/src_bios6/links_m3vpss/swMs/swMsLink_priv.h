/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#ifndef _SW_MS_LINK_PRIV_H_
#define _SW_MS_LINK_PRIV_H_

#include <mcfw/src_bios6/links_m3vpss/system/system_priv_m3vpss.h>
#include <mcfw/interfaces/link_api/swMsLink.h>
#include <mcfw/src_bios6/utils/utils_dma.h>
#include <mcfw/interfaces/link_api/avsync.h>
#include <mcfw/interfaces/link_api/avsync_rtos.h>

// #define SWMS_DEBUG_FRAME_REJECT

#define SW_MS_LINK_OBJ_MAX              (4)

#define SW_MS_LINK_CMD_DO_SCALING       (0x0500)

#define SW_MS_LINK_MAX_OUT_FRAMES_DEFAULT   (SYSTEM_LINK_FRAMES_PER_CH)
#define SW_MS_LINK_MAX_OUT_FRAMES           (SW_MS_LINK_MAX_OUT_FRAMES_DEFAULT+4)


#define SW_MS_LINK_TIMER_DEFAULT_PERIOD (15)

#define SW_MS_MAX_DEI_CH                (8)

#define SW_MS_SKIP_PROCESSING           (2)

#define SW_MS_SKIP_INPUT_FRAMES_SC      (2)
#define SW_MS_SKIP_INPUT_FRAMES_DEI     (3)

#define SW_MS_MAX_WIDTH_SUPPORTED       (1920)

#define SW_MS_MAX_HEIGHT_SUPPORTED      (1200)

#define SW_MS_SCALING_INTERVAL_INVALID  (~0u)

#define SW_MS_BLANK_FRAME_PIXEL_LUMA    (0x30)
#define SW_MS_BLANK_FRAME_PIXEL_CHROMA  (0x80)

#define SW_MS_GRID_FILL_PIXEL_LUMA    (0x10)
#define SW_MS_GRID_FILL_PIXEL_CHROMA  (0x80)

#define SWMS_LINK_DMA_MAX_TRANSFERS      (1)
#define SWMS_LINK_DMA_GRID_MAX_TRANSFERS (4)

#define SWMS_LINK_MAX_DUP_PER_FRAME      (2)
#define SWMS_LINK_MAX_DUP_FRAMES         (SW_MS_LINK_MAX_OUT_FRAMES)
/** @brief Creationg of dup object failed */
#define SWMS_LINK_E_DUPOBJ_CREATE_FAILED (-8)
/** @brief Deletion of dup object failed */
#define SWMS_LINK_E_DUPOBJ_DELETE_FAILED (-9)

#define SWMS_LINK_DO_OUTBUF_BLANKING    (TRUE)

#define SWMS_LINK_MAX_FRAMES_PER_CH     (128)
#define SWMS_ENABLE_MODIFY_FRAME_POINTER_ALWAYS
#define SWMS_LINK_DISPLAY_CB_OFFSET     (8)


typedef struct {

    UInt32 framesOutCount;
    UInt32 framesUsedCount[SYSTEM_SW_MS_MAX_WIN];
    UInt32 statsStartTime;

} SwMsLink_AvgStatistics;

typedef struct {
    Bool applyRtPrm;
    UInt32 expectedFid;
    FVID2_ScanFormat scanFormat;
    FVID2_Frame blankFrame;
    FVID2_Frame curOutFrame;
    Vps_FrameParams scRtInFrmPrm;
    Vps_FrameParams scRtOutFrmPrm;
    Vps_CropConfig scRtCropCfg;
    Vps_ScRtConfig scRtCfg;
    Vps_DeiRtConfig deiRtCfg;
    Vps_M2mScRtParams scRtPrm;
    Vps_M2mDeiRtParams deiRtPrm;

    UInt32 framesRecvCount;
    /* number of frames received for this window */

    UInt32 framesInvalidChCount;
    /* frames received for a window, but channel ID associated with that window
        does not match the incoming channel ID

        Normally this condition should not happen
     */

    UInt32 framesFidInvalidCount;
    /* number of frames that were rejected for for this window.
        due to FID mismatch,
        - this happens when using DEI SW MS and same FID is received consecutively
        OR
        - this happens when using SC SW MS and ODD field FID is received
    */

    UInt32 framesRejectCount;
    /* number of frames that were rejected for for this window.
        Possible reasons for rejection include
        - During mosaic switch older queued frames get rejected
     */

    UInt32 framesQueRejectCount;
    /* number of frames reject for queing due to window que being full
        normally this condition should not occur
    */

    UInt32 framesQueuedCount;
    /* number of frames queued for display
    */

    UInt32 framesDroppedCount;
    /* number of frames that were dropped AND not displayed.
        Conditions that can cause this
        - due to accumalation of frames in the que
    */

    UInt32 framesRepeatCount;
    /* number of frames that were repeated since new input was not avialable */

    UInt32 framesAccEventCount;
    /* number of times the event of frames being accumalated happened */

    UInt32 framesAccMax;
    /* maximum number of frames that were accumalted when this event occured */

    UInt32 framesAccMin;
    /* minimum number of frames that were accumalated when this event occured */

    UInt32 framesUsedCount;
    /* number of frames that were actually used for SW MS
    */

    UInt32 minLatency;
    /* minimum capture/source to SW Mosaic output latency in msecs */

    UInt32 maxLatency;
    /* maximum capture/source to SW Mosaic output latency in msecs */

    Bool isBlankFrameDisplay;

} SwMsLink_OutWinObj;

typedef struct SwMsLink_chObj
{
    FVID2_Frame *pCurInFrame;
    Bool         isPlaybackChannel;
    AvsyncLink_VidQueObj inQue;
    FVID2_Frame *inQueMem[SWMS_LINK_MAX_FRAMES_PER_CH];
    /**< Memory for in que data - video frames */
    Bool         isInputReceived;
} SwMsLink_chObj;


typedef struct
{
    Vps_M2mDeiCreateParams deiCreateParams;
    Vps_M2mDeiCreateStatus deiCreateStatus;
    Vps_M2mDeiChParams deiChParams[SYSTEM_SW_MS_MAX_WIN];
    Vps_DeiHqConfig deiHqCfg;
    Vps_DeiConfig deiCfg;
}deiCfg;

typedef struct
{
    Vps_M2mScCreateParams scCreateParams;
    Vps_M2mScCreateStatus scCreateStatus;
    Vps_M2mScChParams scChParams[SYSTEM_SW_MS_MAX_WIN];
}scCfg;

typedef union
{
    deiCfg dei;
    scCfg sc;
}SwMsLink_DrvCfg;

typedef struct {
    Bool isDeiDrv;
    Bool bypassDei;
    Bool forceBypassDei; /* use DEI in bypass mode only */
    UInt32 vipInstId; /** used for VIP Lock */
    UInt32 startWin;
    UInt32 endWin;
    UInt32 drvInstId;
    SwMsLink_DrvCfg cfg;
    FVID2_Handle fvidHandle;
    FVID2_ProcessList errCbProcessList;
    FVID2_FrameList inFrameList;
    FVID2_FrameList outFrameList;
    FVID2_ProcessList processList;
    Semaphore_Handle complete;
    FVID2_Format drvOutFormat[SYSTEM_SW_MS_MAX_WIN];
    FVID2_Format drvVipOutFormat[SYSTEM_SW_MS_MAX_WIN];
    Vps_ScConfig scCfg;
    Vps_CropConfig scCropCfg[SYSTEM_SW_MS_MAX_WIN];
    Vps_ScConfig vipScCfg;
    Vps_CropConfig vipScCropCfg[SYSTEM_SW_MS_MAX_WIN];
} SwMsLink_DrvObj;

typedef struct SwMsLink_DupObj {
    Utils_QueHandle dupQue;
    FVID2_Frame *dupQueMem[SWMS_LINK_MAX_DUP_FRAMES];
    FVID2_Frame dupFrameMem[SWMS_LINK_MAX_DUP_FRAMES];
    System_FrameInfo frameInfo[SWMS_LINK_MAX_DUP_FRAMES];
} SwMsLink_DupObj;

typedef struct SwMsLink_singleWinCopyObj {
    Ptr pSrcWindow;
    Ptr pDstWindow;
    UInt32 winWidth;   
    UInt32 winHeight;  
} SwMsLink_singleWinCopyObj;

typedef struct SwMsLink_WinCopyObj {
    UInt32 numWindows;
    SwMsLink_singleWinCopyObj cpObj[SYSTEM_SW_MS_MAX_WIN];
} SwMsLink_WinCopyObj;

typedef struct {
    UInt32 linkId;

    Utils_TskHndl tsk;

    char name[32];

    SwMsLink_CreateParams createArgs;
    System_LinkInfo info;
    System_LinkInfo inTskInfo;
    System_LinkQueInfo inQueInfo;

    SwMsLink_LayoutPrm  layoutParams;

    Bool switchLayout;

    UInt32 maxWinId;
    SwMsLink_OutWinObj winObj[SYSTEM_SW_MS_MAX_WIN];
    System_LinkChInfo rtChannelInfo[SYSTEM_SW_MS_MAX_CH_ID];
    Bool rtParamUpdate;

    Semaphore_Handle lock;

    UInt32 inFrameGetCount;
    UInt32 inFramePutCount;

    Clock_Handle timer;
    UInt32 timerPeriod;
    /**< Rate at which output frames should be generated,
         should typically be equal to the display rate

         Example, for 60fps, timerPeriod should be 1000/60 = 16
    */

    Utils_BufHndl bufOutQue;
    FVID2_Frame outFrames[SW_MS_LINK_MAX_OUT_FRAMES];
    System_FrameInfo outFrameInfo[SW_MS_LINK_MAX_OUT_FRAMES];
    FVID2_Format outFrameFormat;
    FVID2_Format bufferFrameFormat;
    FVID2_Format blankBufferFormat;
    Ptr          lastOutBufPtr;
    SwMsLink_WinCopyObj winCopyObj;

    Vps_FrameParams blankFrameScRtInFrmPrm;
    Vps_CropConfig  blankFrameScRtCropCfg;

    FVID2_FrameList freeFrameList;

    FVID2_Frame outFrameDrop;
    FVID2_FrameList outFrameDropList;

    UInt32 vipInstId; /** used for VIP Lock - Assuming that 1 instance cannot operate with different VIPs */
    Bool vipLockRequired;

    SwMsLink_DrvObj DrvObj[2 * SYSTEM_SW_MS_MAX_INST];
    /* 4 instance and 4 more for bypass dei drv use, for example
       if DrvObj[3] is dei drv, so DrvObj[3 + SYSTEM_SW_MS_MAX_INST]
       is it's the bypass mode for this dei sc.
     */

    UInt32 frameCount;
    UInt32 totalTime;

    UInt32 skipProcessing;

    UInt32 framesOutReqCount;
    /* number of times SW MS was triggered for outputing frames */

    UInt32 framesOutDropCount;
    /* number of frames that could not be output
    due to output buffer unavailability */

    UInt32 framesOutRejectCount;
    /* number of frames that were generated but could not be sent to the next link
        since next link output queue was full
    */

    UInt32 framesOutCount;
    /* number of frames that were output */

    UInt32 statsStartTime;
    /* time at which stats were reset - used to calculate FPS relative to this start time */

    UInt32 prevDoScalingTime;
    /* time at which the last SW_MS_LINK_CMD_DO_SCALING command was processed */

    UInt32 scalingInterval;
    /* time interval in ms between successive scaling operations */

    UInt32 scalingIntervalMin;
    /* min time interval in ms between successive scaling operations */
    UInt32 scalingIntervalMax;
    /* max time interval in ms between successive scaling operations */
    SwMsLink_chObj chObj[SYSTEM_SW_MS_MAX_CH_ID];
    /* SwMs channel specific object */
    Utils_DmaChObj dmaObj;
    /* DMA object for blanking output buffer */

    Utils_DmaChObj gridDmaObj;
    /* DMA object for Drawing grids on Layout frames */

    UInt32 memUsed[UTILS_MEM_MAXHEAPS];
    /* Log of memory used from different system heaps */
    SwMsLink_DupObj dupObj;
    Bool SwmsProcessTieWithDisplayLocalFlag;
    Bool enableOuputDupLocalFlag;
    UInt32 lastDisplayCBTS;

    SwMsLink_AvgStatistics avgStats;

} SwMsLink_Obj;


Int32 SwMsLink_getInfo(Utils_TskHndl * pTsk, System_LinkInfo * info);
Int32 SwMsLink_getFullFrames(Utils_TskHndl * pTsk, UInt16 queId,
                             FVID2_FrameList * pFrameList);
Int32 SwMsLink_putEmptyFrames(Utils_TskHndl * pTsk, UInt16 queId,
                              FVID2_FrameList * pFrameList);

Int32 SwMsLink_drvGetLayoutParams(SwMsLink_Obj * pObj, SwMsLink_LayoutPrm * layoutParams);

Int32 SwMsLink_drvCreate(SwMsLink_Obj * pObj, SwMsLink_CreateParams * pPrm);
Int32 SwMsLink_drvStart(SwMsLink_Obj * pObj);
Int32 SwMsLink_drvProcessData(SwMsLink_Obj * pObj);
Int32 SwMsLink_drvDoScaling(SwMsLink_Obj * pObj);
Int32 SwMsLink_drvMakeFrameLists(SwMsLink_Obj * pObj, FVID2_Frame * pOutFrame);
Int32 SwMsLink_drvStop(SwMsLink_Obj * pObj);
Int32 SwMsLink_drvDelete(SwMsLink_Obj * pObj);
Int32 SwMsLink_drvLock(SwMsLink_Obj * pObj);
Int32 SwMsLink_drvUnlock(SwMsLink_Obj * pObj);

Int32 SwMsLink_drvSwitchLayout(SwMsLink_Obj * pObj,
                               SwMsLink_LayoutPrm * layoutParams,
                               Bool isLockAlredayTaken);

Int32 SwMsLink_drvCreateWinObj(SwMsLink_Obj * pObj, UInt32 winId);
Int32 SwMsLink_drvCreateDeiDrv(SwMsLink_Obj * pObj, SwMsLink_DrvObj *pDrvObj);
Int32 SwMsLink_drvCreateScDrv(SwMsLink_Obj * pObj, SwMsLink_DrvObj *pDrvObj);
Int32 SwMsLink_drvDeleteDrv(SwMsLink_Obj * pObj, SwMsLink_DrvObj *pDrvObj);
Int32 SwMsLink_drvClockPeriodReconfigure(SwMsLink_Obj * pObj);
Int32 SwMsLink_drvGetTimerPeriod(SwMsLink_Obj * pObj,
                                 SwMsLink_LayoutPrm * layoutParams);

Int32 SwMsLink_drvDoDma(SwMsLink_Obj * pObj, FVID2_Frame *pFrame);
Int32 SwMsLink_drvDmaCreate(SwMsLink_Obj * pObj);
Int32 SwMsLink_drvDmaDelete(SwMsLink_Obj * pObj);


Void SwMsLink_tskFlushRepeatCommands(SwMsLink_Obj * pObj, Utils_TskHndl * pTsk, UInt32 cmd);

Int32 SwMsLink_updateLayoutParams(SwMsLink_LayoutPrm * layoutParams,  UInt32 outPitch);

Int32 SwMsLink_drvModifyFramePointer(SwMsLink_Obj * pObj, SwMsLink_DrvObj *pDrvObj, Bool addOffset);
Int32 SwMsLink_drvPrintStatistics(SwMsLink_Obj * pObj, Bool resetAfterPrint);
Int32 SwMsLink_drvPrintLayoutParams(SwMsLink_Obj * pObj);

Int32 SwMsLink_drvGetInputChInfoFromWinId(SwMsLink_Obj * pObj,SwMsLink_WinInfo * pCropInfo);
Int32 SwMsLink_drvSetCropParam(SwMsLink_Obj * pObj,SwMsLink_WinInfo * pCropInfo);
Int32 SwMsLink_drvFreeProcessedFrames(SwMsLink_Obj * pObj,
                                      FVID2_FrameList * freeFrameList);

Int32 SwMsLink_printBufferStatus (SwMsLink_Obj * pObj);
Int32 SwMsLink_flushBuffers(SwMsLink_Obj * pObj,SwMsLink_FlushParams *prm);

#endif


