/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#ifndef _SW_MS_LINK_PRIV_H_
#define _SW_MS_LINK_PRIV_H_

#include <mcfw/src_bios6/links_m3vpss/system/system_priv_m3vpss.h>
#include <mcfw/interfaces/link_api/sclrLink.h>

#define SCLR_LINK_OBJ_MAX                     (SYSTEM_LINK_ID_SCLR_COUNT)

#define SCLR_LINK_MAX_CH                      (16)

#define SCLR_LINK_MAX_OUT_FRAMES_PER_CH       (SYSTEM_LINK_FRAMES_PER_CH)

#define SCLR_LINK_MAX_OUT_FRAMES              (SCLR_LINK_MAX_CH*SCLR_LINK_MAX_OUT_FRAMES_PER_CH)

#define SCLR_LINK_MAX_REQ                     (4)

#define FPS_WRAPAROUND_COUNT                  (30)

#define SCLR_LINK_MAX_WIDTH_SUPPORTED         (1920)

#define SCLR_LINK_MAX_HEIGHT_SUPPORTED        (1080)

#define SCLR_LINK_ENABLE_MODIFY_FRAME_POINTER_ALWAYS


typedef struct {
    Utils_BufHndl bufOutQue;
    FVID2_Frame outFrames     [SCLR_LINK_MAX_OUT_FRAMES];
    System_FrameInfo frameInfo[SCLR_LINK_MAX_OUT_FRAMES];
    FVID2_Format outFormat;
    UInt32 outNumFrames;
} SclrLink_OutObj;

typedef struct {
    Bool          enableOut;
    Utils_BufHndl inQue;
    UInt32        nextFid;
    Bool          doFrameDrop;

    FVID2_Format    outFormat;
    Vps_ScConfig    scCfg;
    Vps_CropConfig  scCropConfig;
    Vps_FrameParams sclrRtInFrmPrm;
    Vps_FrameParams sclrRtOutFrmPrm;
    Vps_M2mScRtParams sclrRtPrm;
    Bool chRtOutInfoUpdate;
    System_LinkChInfo rtChannelInfo;

    /** SCLR link to skip one specific FID type */
    Bool isSkipOneFidType;
    UInt32 skipFidType;
    
    Uint32 curFrameNum;
    UInt32 frameSkipCount;

    /* input frame recevied from previous link */
    UInt32 inFrameRecvCount;

    /* input frame rejected due mismatch in FID */
    UInt32 inFrameRejectCount;

    /* input frames actually processed */
    UInt32 inFrameProcessCount;

    /* output frames skip based on user setting like 'skip alternate' frame */
    UInt32 outFrameUserSkipCount;

    /* output frames dropped due to buffer not being available */
    UInt32 outFrameSkipCount;

    /* number of frames actually output */
    UInt32 outFrameCount;

    UInt32 minLatency;
    /* minimum capture/source to SW Mosaic output latency in msecs */

    UInt32 maxLatency;
    /* maximum capture/source to SW Mosaic output latency in msecs */

    /* Data structure for frame skip to achieve expected output frame rate */
    Utils_frameSkipContext frameSkipCtx;

}SclrLink_ChObj;


typedef struct {
    FVID2_ProcessList processList;
    FVID2_FrameList inFrameList;
    FVID2_FrameList outFrameList;
} SclrLink_ReqObj;

typedef struct {
    UInt32                 linkId;
    Utils_TskHndl          tsk;
    char                   name[32];

    SclrLink_CreateParams  createArgs;
    System_LinkInfo        inTskInfo;
    System_LinkQueInfo     inQueInfo;

    SclrLink_ReqObj        reqObj[SCLR_LINK_MAX_REQ];
    Utils_QueHandle        reqQue;
    SclrLink_ReqObj        *reqQueMem[SCLR_LINK_MAX_REQ];
    UInt32                 reqQueCount;
    Bool                   isReqPend;
    UInt32                 reqNumOutLists;

    SclrLink_OutObj        outObj;
    System_LinkInfo        info;

    Vps_M2mScChParams      drvChArgs[SCLR_LINK_MAX_CH];
    SclrLink_ChObj         chObj[SCLR_LINK_MAX_CH];

    FVID2_Handle           fvidHandle;
    FVID2_ProcessList      errProcessList;
    FVID2_Frame            outFrameDrop;

    Vps_M2mScCreateParams  scCreateParams;
    Vps_M2mScCreateStatus  scCreateStatus;

    /* Semaphore to signal completion of Scalar */
    Semaphore_Handle       complete;

    UInt32                 inFrameGetCount;
    UInt32                 inFramePutCount;
    UInt32                 outFrameGetCount;
    UInt32                 outFramePutCount;
    UInt32                 processFrameReqPendCount;
    UInt32                 processFrameReqPendSubmitCount;
    UInt32                 processFrameCount;
    UInt32                 getProcessFrameCount;
    UInt32                 processFrameReqCount;
    UInt32                 getProcessFrameReqCount;

    UInt32                 totalTime;
    UInt32                 curTime;
    UInt32                 givenInFrames;
    UInt32                 returnedInFrames;
    UInt32                 frameCount;

    Bool                   loadUpsampleCoeffs;
    UInt32                 statsStartTime;
    /* time at which stats were reset - used to calculate FPS relative to this start time */
    UInt32                 memUsed[UTILS_MEM_MAXHEAPS];

} SclrLink_Obj;


Int32 SclrLink_getInfo(Utils_TskHndl * pTsk, System_LinkInfo * info);
Int32 SclrLink_getFullFrames(Utils_TskHndl * pTsk, UInt16 queId,
                             FVID2_FrameList * pFrameList);
Int32 SclrLink_putEmptyFrames(Utils_TskHndl * pTsk, UInt16 queId,
                              FVID2_FrameList * pFrameList);


Int32 SclrLink_drvCreate(SclrLink_Obj * pObj, SclrLink_CreateParams * pPrm);
Int32 SclrLink_drvStart(SclrLink_Obj * pObj);
Int32 SclrLink_drvProcessData(SclrLink_Obj * pObj);
Int32 SclrLink_drvDoScaling(SclrLink_Obj * pObj);
Int32 SclrLink_drvStop(SclrLink_Obj * pObj);
Int32 SclrLink_drvDelete(SclrLink_Obj * pObj);
Int32 SclrLink_drvGetProcessedData(SclrLink_Obj * pObj);

Int32 SclrLink_drvSetChannelInfo(SclrLink_Obj * pObj, SclrLink_ChannelInfo *channelInfo);
Int32 SclrLink_SetFrameRate(SclrLink_Obj * pObj, SclrLink_ChFpsParams * params);

Int32 SclrLink_drvPrintStatistics(SclrLink_Obj * pObj, Bool resetAfterPrint);
Int32 SclrLink_resetStatistics(SclrLink_Obj *pObj);

Int32 SclrLink_drvSetChDynamicOutputRes(SclrLink_Obj * pObj, 
                                       SclrLink_chDynamicSetOutRes * params);
Int32 SclrLink_drvGetChDynamicOutputRes(SclrLink_Obj * pObj, 
                                       SclrLink_chDynamicSetOutRes * params);
Int32 SclrLink_drvDynamicSkipFidType(SclrLink_Obj * pObj, 
                                     SclrLink_chDynamicSkipFidType * params);

#endif


