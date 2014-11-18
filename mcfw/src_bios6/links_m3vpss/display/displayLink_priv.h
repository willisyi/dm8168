/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#ifndef _DISPLAY_LINK_PRIV_H_
#define _DISPLAY_LINK_PRIV_H_

#include <mcfw/src_bios6/links_m3vpss/system/system_priv_m3vpss.h>
#include <mcfw/interfaces/link_api/displayLink.h>

#include <mcfw/src_bios6/utils/utils_dma.h>
#include <mcfw/interfaces/link_api/avsync_rtos.h>

#define DISPLAY_LINK_BLANK_FRAME_CHANNEL_NUM     (0xFFFF)

#define DISPLAY_LINK_BLANK_FRAME_INIT_QUE        (2)

#define DISPLAY_LINK_OBJ_MAX                     (3)

#define DISPLAY_LINK_CMD_DO_DEQUE                (0x0500)

#define DISPLAY_LINK_DONE_PERIOD_MS              (16u)

/*
 * The maximum number of frame info is equal to max display queue length.
 * This is set to max number of frames queued X 2 since we need to
 * queue frames as two separate fields
 */
#define DISPLAY_LINK_MAX_FRAMEINFO               (SYSTEM_LINK_MAX_FRAMES_PER_CH)

#define DISPLAY_LINK_INVALID_INQUEID             (~(0u))

typedef struct DisplayLink_FrameInfo
{
    System_FrameInfo *origFrameInfo;
    Bool              isFieldPair;
    FVID2_Frame       frame;
    FVID2_Frame       *origFrame[2];
    UInt32            activeQueueId;
} DisplayLink_FrameInfo;

typedef  struct DisplayLink_drvRtParams
{
    Vps_DispRtParams dispRtPrms;
    Vps_FrameParams  inFrmPrms;
} DisplayLink_drvRtParams;

typedef struct DisplayLink_periodicObj {
    Clock_Struct clkStruct;
    Clock_Handle clkHandle;
    Bool clkStarted;
} DisplayLink_periodicObj;

#define DISPLAY_LINK_DMA_MAX_TRANSFERS           (10)

typedef struct {
    UInt32 tskId;

    Utils_TskHndl tsk;

    DisplayLink_CreateParams createArgs;

    System_LinkInfo inTskInfo[DISPLAY_LINK_MAX_NUM_INPUT_QUEUES];

    System_LinkQueInfo inQueInfo;

    Semaphore_Handle lock;
    Semaphore_Handle complete;

    UInt32 curDisplayChannelNum;
    UInt32 queueCount;

    FVID2_Handle displayHndl;

    Vps_DispCreateParams displayCreateArgs;
    Vps_DispCreateStatus displayCreateStatus;
    FVID2_Format displayFormat;
    UInt32 displayInstId;

    FVID2_Frame blankFrame;

    UInt32 inFrameGetCount;
    UInt32 inFramePutCount;

    UInt32 dequeCount;
    UInt32 cbCount;

    UInt32 totalTime;

    UInt32 startTime;
    UInt32 prevTime;
    UInt32 minCbTime;
    UInt32 maxCbTime;
    UInt32 lastCbTime;

    UInt32 maxLatency;
    UInt32 minLatency;
    UInt32 drvTopFieldLatency;
    UInt32 numTopFields;
    struct DisplayLink_FrameInfoObj {
        DisplayLink_FrameInfo infoMem[DISPLAY_LINK_MAX_FRAMEINFO];
        Utils_QueHandle       infoQ;
        DisplayLink_FrameInfo *infoQMem[DISPLAY_LINK_MAX_FRAMEINFO];
    } frameInfo;
    UInt32         maxQueueCount;
    UInt32         numBufsInDriver;
    Bool           fieldInput[DISPLAY_LINK_MAX_NUM_INPUT_QUEUES];
    UInt32         curActiveQueue;
    UInt32         nextFid;
    FVID2_Frame    *topField;
    UInt32         numDisplayBufPlanes;
    struct DisplayLink_rtParamsObj
    {
        Utils_QueHandle freeQ;
        DisplayLink_drvRtParams paramsMem[SYSTEM_LINK_MAX_FRAMES_PER_CH];
        DisplayLink_drvRtParams *freeQMem[SYSTEM_LINK_MAX_FRAMES_PER_CH];
    } rtParams;

    Bool enableFieldSeparatedInputMode;

    DisplayLink_periodicObj prd;
    Bool isDisplayRunning;

} DisplayLink_Obj;


Int32 DisplayLink_drvCreate(DisplayLink_Obj * pObj,
                            DisplayLink_CreateParams * pPrm);
Int32 DisplayLink_drvStart(DisplayLink_Obj * pObj);
Int32 DisplayLink_drvProcessData(DisplayLink_Obj * pObj);
Int32 DisplayLink_drvStop(DisplayLink_Obj * pObj);
Int32 DisplayLink_drvDelete(DisplayLink_Obj * pObj);
Int32 DisplayLink_drvLock(DisplayLink_Obj * pObj);
Int32 DisplayLink_drvUnlock(DisplayLink_Obj * pObj);
Int32 DisplayLink_drvSwitchCh(DisplayLink_Obj * pObj, DisplayLink_SwitchChannelParams *prm);
Int32 DisplayLink_drvPrintStatistics(DisplayLink_Obj * pObj);
Int32 DisplayLink_drvSwitchActiveQueue(DisplayLink_Obj * pObj,
                                       DisplayLink_SwitchActiveQueueParams *prm);
Int32 DisplayLink_drvSetFmt(DisplayLink_Obj * pObj, FVID2_Format *pFormat);

Int32 DisplayLink_drvSwitchInputMode(DisplayLink_Obj * pObj, DisplayLink_SwitchInputMode *pPrm);

Int32 DisplayLink_printBufferStatus(DisplayLink_Obj * pObj);

#endif
