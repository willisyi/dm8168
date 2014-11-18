/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#ifndef _DEI_LINK_PRIV_H_
#define _DEI_LINK_PRIV_H_

#include <mcfw/src_bios6/links_m3vpss/system/system_priv_m3vpss.h>
#include <mcfw/interfaces/link_api/deiLink.h>

#define DEI_LINK_OBJ_MAX                     (4)

#define DEI_LINK_MAX_OUT_FRAMES_PER_CH       (SYSTEM_LINK_FRAMES_PER_CH + 2)

#define DEI_LINK_MAX_REQ                     (4)

#define FPS_WRAPAROUND_COUNT (30)

#define FRAME_RATE_30_MASK	0x3FFFFFFF
#define FRAME_RATE_26_MASK	0x1F7F7F7F
#define FRAME_RATE_15_MASK	0x2AAAAAAA
#define FRAME_RATE_08_MASK	0x28888888
#define FRAME_RATE_01_MASK	0x00000001
#define FRAME_RATE_05_MASK	0x20820820
#define FRAME_RATE_00_MASK	0x00000000

typedef struct {
    Utils_BufHndl bufOutQue;
    FVID2_Frame outFrames[DEI_LINK_MAX_CH][DEI_LINK_MAX_OUT_FRAMES_PER_CH];
    System_FrameInfo frameInfo[DEI_LINK_MAX_CH][DEI_LINK_MAX_OUT_FRAMES_PER_CH];
    UInt32 outNumFrames[DEI_LINK_MAX_CH];
    Utils_QueHandle emptyBufQue[DEI_LINK_MAX_CH];
    FVID2_Frame *outFramesMem [DEI_LINK_MAX_CH][DEI_LINK_MAX_OUT_FRAMES_PER_CH];
   /** Holds individual channel empty buffers */
} DeiLink_OutObj;

typedef struct {
	Bool enableOut[DEI_LINK_MAX_OUT_QUE];
    Utils_BufHndl inQue;
    UInt32 nextFid;

    FVID2_Format outFormat[DEI_LINK_MAX_OUT_QUE];
    Vps_DeiHqConfig deiHqCfg;
    Vps_DeiConfig deiCfg;
    Vps_ScConfig scCfg[DEI_LINK_MAX_OUT_QUE];
    Vps_CropConfig scCropConfig[DEI_LINK_MAX_OUT_QUE];
    /** For dynamic change of parameters to generate 2 different outputs from VIP-SC */
    Vps_FrameParams deiRtOutFrmPrm[DEI_LINK_MAX_OUT_QUE];
    Vps_FrameParams vipRtOutFrmPrm[DEI_LINK_MAX_OUT_QUE];
    Vps_M2mDeiRtParams deiRtPrm;
    Vps_DeiRtConfig deiRtCfg;

    Bool chRtOutInfoUpdateForced[DEI_LINK_MAX_OUT_QUE];
    Bool chRtOutInfoUpdate[DEI_LINK_MAX_OUT_QUE];
    UInt32 chRtOutInfoUpdateWhileDrop[DEI_LINK_MAX_OUT_QUE];
    Bool chRtOutDeiTertiaryOutQFlag[DEI_LINK_MAX_OUT_QUE];

    Uint32 curFrameNum;
    UInt32 frameSkipCount[DEI_LINK_MAX_OUT_QUE];

    /* input frame recevied from previous link */
    UInt32 inFrameRecvCount;

    /* input frame rejected due mismatch in FID */
    UInt32 inFrameRejectCount;

    /* input frames actually processed */
    UInt32 inFrameProcessCount;

    /* output frames skip based on user setting like 'skip alternate' frame */
    UInt32 outFrameUserSkipCount[DEI_LINK_MAX_OUT_QUE];

    /* output frames dropped due to buffer not being available */
    UInt32 outFrameSkipCount[DEI_LINK_MAX_OUT_QUE];

    /* number of frames actually output */
    UInt32 outFrameCount[DEI_LINK_MAX_OUT_QUE];

    UInt32 minLatency;
    /* minimum capture/source to SW Mosaic output latency in msecs */

    UInt32 maxLatency;
    /* maximum capture/source to SW Mosaic output latency in msecs */

    /* Data structure for frame skip to achieve expected output frame rate */
    Utils_frameSkipContext frameSkipCtx [DEI_LINK_MAX_OUT_QUE];

    /* Data structure for frame skip to achieve expected de-interlace module output frame rate */
    Utils_frameSkipContext frameSkipCtxDei;

    /* previous input frame, used for DEI in DM814x, DM810x */
    FVID2_Frame *pInFrameN_1;
    FVID2_Frame *pInFrameN_2;

    Bool setEvenOddEvenPatternDeiFlag;
} DeiLink_ChObj;

typedef struct {
    FVID2_ProcessList processList;

    FVID2_FrameList inFrameList;
    FVID2_FrameList outFrameList[DEI_LINK_MAX_DRIVER_OUT_QUE];
    UInt32 outList0QueIdMap[DEI_LINK_MAX_CH];
    UInt32 outList1QueIdMap[DEI_LINK_MAX_CH];

    /* only used in DM814x, DM810x */
    Vps_M2mDeiOverridePrevFldBuf    prevFldBuf;

    FVID2_FrameList inFrameListN;
    FVID2_FrameList inFrameListN_1;
    FVID2_FrameList inFrameListN_2;

} DeiLink_ReqObj;

typedef struct {
    UInt32 linkId;

    char name[32];

    Utils_TskHndl tsk;

    DeiLink_CreateParams createArgs;

    System_LinkInfo inTskInfo;
    System_LinkQueInfo inQueInfo;

    DeiLink_ReqObj reqObj[DEI_LINK_MAX_REQ];
    Utils_QueHandle reqQue;
    DeiLink_ReqObj *reqQueMem[DEI_LINK_MAX_REQ];
    UInt32 reqQueCount;
    Bool isReqPend;
    UInt32 reqNumOutLists;

    DeiLink_OutObj outObj[DEI_LINK_MAX_OUT_QUE];
    System_LinkInfo info;

    Vps_M2mDeiChParams drvChArgs[DEI_LINK_MAX_CH];
    DeiLink_ChObj chObj[DEI_LINK_MAX_CH];

    Vps_M2mDeiCreateParams drvCreateArgs;
    Vps_M2mDeiCreateStatus drvCreateStatus;
    FVID2_Handle fvidHandle;
    FVID2_ProcessList errProcessList;
    UInt32 drvInstId;
    UInt32 vipInstId; /** used when reloading SC co-effs after reset */

    FVID2_Frame outFrameDrop;

    /* Semaphore to signal completion of DEI */
    Semaphore_Handle complete;

    UInt32 inFrameGetCount;
    UInt32 inFrameSkipCount;
    UInt32 inFramePutCount;
    UInt32 outFrameGetCount[DEI_LINK_MAX_OUT_QUE];
    UInt32 outFramePutCount[DEI_LINK_MAX_OUT_QUE];
    UInt32 processFrameReqPendCount;
    UInt32 processFrameReqPendSubmitCount;
    UInt32 processFrameCount;
    UInt32 getProcessFrameCount;
    UInt32 processFrameReqCount;
    UInt32 getProcessFrameReqCount;

    UInt32 totalTime;
    UInt32 curTime;

    UInt32 givenInFrames;
    UInt32 returnedInFrames;

    Bool loadUpsampleCoeffs;

    UInt32 statsStartTime;
    UInt32 memUsed[UTILS_MEM_MAXHEAPS];

    UInt32 useOverridePrevFldBuf;
} DeiLink_Obj;


Int32 DeiLink_getInfo(Utils_TskHndl * pTsk, System_LinkInfo * info);
Int32 DeiLink_getFullFrames(Utils_TskHndl * pTsk, UInt16 queId,
                            FVID2_FrameList * pFrameList);
Int32 DeiLink_putEmptyFrames(Utils_TskHndl * pTsk, UInt16 queId,
                             FVID2_FrameList * pFrameList);

Int32 DeiLink_drvCreate(DeiLink_Obj * pObj, DeiLink_CreateParams * pPrm);
Int32 DeiLink_drvProcessData(DeiLink_Obj * pObj);
Int32 DeiLink_drvGetProcessedData(DeiLink_Obj * pObj);
Int32 DeiLink_drvStop(DeiLink_Obj * pObj);
Int32 DeiLink_drvDelete(DeiLink_Obj * pObj);
Int32 DeiLink_drvSetChannelInfo(DeiLink_Obj * pObj, DeiLink_ChannelInfo *channelInfo);
Int32 DeiLink_SetFrameRate(DeiLink_Obj * pObj, DeiLink_ChFpsParams * params);
Int32 DeiLink_printStatistics (DeiLink_Obj *pObj, Bool resetAfterPrint);
Int32 DeiLink_printBufferStatus(DeiLink_Obj * pObj);
Int32 DeiLink_resetStatistics(DeiLink_Obj *pObj);
Int32 DeiLink_drvSetChDynamicOutputRes(DeiLink_Obj * pObj,
                                       DeiLink_chDynamicSetOutRes * params);
Int32 DeiLink_drvGetChDynamicOutputRes(DeiLink_Obj * pObj,
                                       DeiLink_chDynamicSetOutRes * params);

Int32 DeiLink_drvFlushChannel(DeiLink_Obj * pObj, UInt32 chId);
Int32 DeiLink_drvSetEvenOddEvenDei(DeiLink_Obj * pObj, DeiLink_ChSetEvenOddEvenPatternDeiParams * params);

#endif
