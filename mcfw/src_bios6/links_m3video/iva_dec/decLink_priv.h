/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#ifndef _DEC_LINK_PRIV_H_
#define _DEC_LINK_PRIV_H_

#include <xdc/std.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/gates/GateMutexPri.h>
#include <mcfw/src_bios6/links_common/system/system_priv_common.h>
#include <mcfw/src_bios6/links_m3video/codec_utils/utils_encdec.h>
#include <mcfw/interfaces/link_api/decLink.h>
#include <mcfw/interfaces/link_api/system_debug.h>
#include <mcfw/src_bios6/links_m3video/codec_utils/utils_encdec_prf.h>
#include <mcfw/src_bios6/links_m3video/system/system_priv_m3video.h>
#include "decLink_err.h"
#include "decLink_algIf.h"
#include "decLink_h264_priv.h"
#include "decLink_mpeg4_priv.h"
#include "decLink_jpeg_priv.h"
#include <mcfw/src_bios6/utils/utils.h> // to use Utils_doSkipFrame().


#define DEC_LINK_OBJ_MAX                     (SYSTEM_LINK_ID_VDEC_COUNT)

#define DEC_LINK_MAX_OUT_FRAMES_PER_CH       (SYSTEM_LINK_FRAMES_PER_CH)

#define DEC_LINK_MAX_OUT_FRAMES              (DEC_LINK_MAX_CH*DEC_LINK_MAX_OUT_FRAMES_PER_CH)

#define DEC_LINK_CMD_GET_PROCESSED_DATA      (0x6000)
/** \Dec command to send late ACK */
#define DEC_LINK_CMD_LATE_ACK                (0x6001)

/** @brief Enclink internal to indicate IVA map change */
#define DEC_LINK_CMD_INTERNAL_IVAMAPCHANGE   (0)


#define DEC_LINK_MAX_REQ                     (DEC_LINK_MAX_OUT_FRAMES)

#define DEC_LINK_MAX_IVASWITCH_SERIALIZERS   (DEC_LINK_MAX_CH)

#define DEC_LINK_MAX_REQ_OBJ_DUMMY           (DEC_LINK_MAX_IVASWITCH_SERIALIZERS * 2)

#define DEC_LINK_PROCESS_TSK_SIZE            (8 * KB)

#define DEC_LINK_MAX_TASK_NAME_SIZE          (32)

#define DEC_LINK_TASK_POLL_DURATION_MS       (8)

#define DEC_LINK_PROCESS_DONE_PERIOD_MS      (4)

#define DEC_LINK_STATS_START_THRESHOLD       (5)

#define DEC_LINK_REQLIST_MAX_REQOBJS         (4)

#define DEC_LINK_MAX_DUP_PER_FRAME           (4)

#define DEC_LINK_MAX_DUP_FRAMES              (DEC_LINK_MAX_OUT_FRAMES)
#define DEC_LINK_NUM_ALGPROCESS_PER_HDVICP_ACQUIRE             (8)


#define DEC_LINK_DEFAULT_ALGPARAMS_TARGETBITRATE               (2 * 1000 * 1000)
#define DEC_LINK_DEFAULT_ALGPARAMS_TARGETFRAMERATE                          (30)
#define DEC_LINK_DEFAULT_ALGPARAMS_DECODEHEADER                  (XDM_DECODE_AU)
#define DEC_LINK_DEFAULT_ALGPARAMS_FRAMESKIPMODE                (IVIDEO_NO_SKIP)
#define DEC_LINK_DEFAULT_ALGPARAMS_NEWFRAMEFLAG                      (XDAS_TRUE)
#define DEC_LINK_DEFAULT_ALGPARAMS_DISPLAYWIDTH                              (0)
#define DEC_LINK_DEFAULT_ALGPARAMS_DISPLAYDELAY       (IVIDDEC3_DECODE_ORDER)
//#define DEC_LINK_DEFAULT_ALGPARAMS_DISPLAYDELAY    (IVIDDEC3_DISPLAY_DELAY_2)

/* *****************************************************************************
 * @def    DEC_LINK_GROUP_SUBM_MAX_SIZE
 * @brief  Defines the maximum submission batch size for the Decoder codec as                                                                
 *         dictated by the codec user guide.
 *         Currently relevant only for the H.264 decoder.Setting this value
 *         as 1 is equivalent to having simple process calls for the H.264
 *         decoder.
 *****************************************************************************
*/

#ifdef SYSTEM_DEBUG_DISABLE_PROCESS_MULTI
#define DEC_LINK_GROUP_SUBM_MAX_SIZE 1 
#else
#define DEC_LINK_GROUP_SUBM_MAX_SIZE 24
#endif



#define DEC_LINK_GROUP_CODEC_CLASS_H264  0
#define DEC_LINK_GROUP_CODEC_CLASS_JPEG  1
#define DEC_LINK_GROUP_CODEC_CLASS_MPEG4 2

typedef enum DecLink_ReqObjType_e
{
    DEC_LINK_REQ_OBJECT_TYPE_REGULAR,
    DEC_LINK_REQ_OBJECT_TYPE_DUMMY_CHDELETE,
    DEC_LINK_REQ_OBJECT_TYPE_DUMMY_IVAMAPCHANGE_PREVIVALAST,
    DEC_LINK_REQ_OBJECT_TYPE_DUMMY_IVAMAPCHANGE_NEXTIVAFIRST,
    DEC_LINK_REQ_OBJECT_TYPE_DUMMY_FLUSHFRAME
} DecLink_ReqObjType_e;

//#define SYSTEM_DEBUG_MULTI_CHANNEL_DEC
typedef struct DecLink_OutChObj {
    UInt32 outNumFrames;
    FVID2_Format outFormat;
    EncDec_ResolutionClass reslutionClass;
    FVID2_Frame outFramesPool[DEC_LINK_MAX_NUM_OUT_BUF_PER_CH*2];
    System_FrameInfo frameInfo[DEC_LINK_MAX_NUM_OUT_BUF_PER_CH*2];
    Utils_QueHandle fvidFrmQue;
    FVID2_Frame *fvidFrmQueMem[DEC_LINK_MAX_NUM_OUT_BUF_PER_CH*2];
    FVID2_Frame *outFrames[DEC_LINK_MAX_NUM_OUT_BUF_PER_CH];
    FVID2_Frame allocFrames[DEC_LINK_MAX_NUM_OUT_BUF_PER_CH];
} DecLink_OutChObj;

typedef struct DecLink_OutObj {
    UInt32 totalNumOutBufs;
    Utils_BufHndlExt bufOutQue;
    DecLink_OutChObj outChObj[DEC_LINK_MAX_CH];
} DecLink_OutObj;

typedef struct {
    Bitstream_Buf *InBuf;
    FVID2_FrameList OutFrameList;
    DecLink_ReqObjType_e type;
    ti_sysbios_knl_Semaphore_Handle     ivaSwitchSerializer;
} DecLink_ReqObj;

typedef struct DecLink_ReqList {
    UInt32 numReqObj;
    DecLink_ReqObj *reqObj[DEC_LINK_REQLIST_MAX_REQOBJS];
} DecLink_ReqList;

typedef struct DecLink_algObj {
    union {
        DecLink_H264Obj h264AlgIfObj;
        DecLink_MPEG4Obj mpeg4AlgIfObj;
        DecLink_JPEGObj jpegAlgIfObj;
    } u;
    DecLink_AlgCreateParams algCreateParams;
    DecLink_AlgDynamicParams algDynamicParams;
    FVID2_Frame *prevOutFrame;
    IRES_ResourceDescriptor resDesc[DEC_LINK_MAX_NUM_RESOURCE_DESCRIPTOR];
} DecLink_algObj;

typedef struct DecLink_periodicObj {
    Clock_Struct clkStruct;
    Clock_Handle clkHandle;
    Bool clkStarted;
} DecLink_periodicObj;

typedef struct DecLink_TrickPlayObj {
    Bool skipFrame;
    /* Data structure for frame skip to achieve expected output frame rate */
    Utils_frameSkipContext frameSkipCtx;
} DecLink_TrickPlayObj;

#define DECLINK_FRAME_DEBUG_MAX_COUNT     (32)

typedef struct DecLink_codecFrameDebugObj {
    UInt32 inBufIdx;
    UInt32 outBufIdx;
    FVID2_Frame *inBuf[DECLINK_FRAME_DEBUG_MAX_COUNT];
    FVID2_Frame *outBuf[DECLINK_FRAME_DEBUG_MAX_COUNT];
} DecLink_codecFrameDebugObj;

typedef struct DecLink_ChObj {
    Utils_QueHandle inQue;
    DecLink_algObj algObj;
    Bool IFrameOnlyDecode;
    Bool disableChn;
    Bool skipFrame;
    UInt32 allocPoolID;
    UInt32 prevFrmRecvTime;
    UInt32 totalProcessTime;
    UInt32 totalFrameIntervalTime;
    UInt32 totalInFrameCnt;
    UInt32 inBufSkipCount;
    UInt32 inBufQueCount;
    UInt32 processReqestCount;
    UInt32 getProcessedBufCount;
    UInt32 numReqObjPerProcess;
    UInt32 numBufsInCodec;
    UInt32 algCreateStatusLocal;
    Bitstream_Buf *inBitBufMem[DEC_LINK_MAX_REQ];
    Bitstream_Buf dummyBitBuf;
    DecLink_TrickPlayObj trickPlayObj;
    Bool isFirstIDRFrameFound;

    UInt32 inFrameRecvCount;
    UInt32 outFrameCount;
    UInt32 inFrameUserSkipCount;
    DecLink_codecFrameDebugObj frameDebug;
    VDEC_CH_ERROR_MSG decErrorMsg;
    ti_sysbios_knl_Semaphore_Struct codecProcessMutexMem;
    ti_sysbios_knl_Semaphore_Handle codecProcessMutex;
} DecLink_ChObj;

typedef struct DecLink_DupObj {
    Utils_QueHandle dupQue;
    FVID2_Frame *dupQueMem[DEC_LINK_MAX_DUP_FRAMES];
    FVID2_Frame dupFrameMem[DEC_LINK_MAX_DUP_FRAMES];
    System_FrameInfo frameInfo[DEC_LINK_MAX_DUP_FRAMES];
} DecLink_DupObj;

typedef struct DecLink_ReqBatch {
    UInt32 numReqObjsInBatch;
    DecLink_ReqObj *pReqObj[DEC_LINK_GROUP_SUBM_MAX_SIZE];
    UInt64 channelSubmittedFlag;
    UInt32 codecSubmittedFlag;
} DecLink_ReqBatch;

typedef struct DecLink_ReqBatchStatistics {
    UInt32 numBatchesSubmitted;
    UInt32 currentBatchSize;
    UInt32 aggregateBatchSize;
    UInt32 averageBatchSize;
    UInt32 maxAchievedBatchSize;
} DecLink_ReqBatchStatistics;


typedef struct DecLink_DebugBatchPrepStats {
    UInt32 numBatchCreated;
    UInt32 numReasonSizeExceeded;
    UInt32 numReasonReqObjQueEmpty;
    UInt32 numReasonResoultionClass;
    UInt32 numReasonContentType;
    UInt32 numReasonChannelRepeat;
    UInt32 numReasonCodecSwitch;
} DecLink_DebugBatchPrepStats;


typedef struct DecLink_Obj {
    UInt32 linkId;
    Utils_TskHndl tsk;
    System_LinkInfo inTskInfo;
    System_LinkQueInfo inQueInfo;
    DecLink_OutObj outObj;
    struct decDummyReqObj_s {
        DecLink_ReqObj reqObjDummy[DEC_LINK_MAX_REQ_OBJ_DUMMY];
        Utils_QueHandle reqQueDummy;
        DecLink_ReqObj *reqQueMemDummy[DEC_LINK_MAX_REQ_OBJ_DUMMY];
    } decDummyReqObj;
    GateMutexPri_Struct  ivaChMapMutexObj;
    GateMutexPri_Handle  ivaChMapMutex;
    UInt32 ch2ProcessTskId[DEC_LINK_MAX_CH];
    UInt32 ch2ProcessTskIdNext[DEC_LINK_MAX_CH];
    DecLink_ReqObj reqObj[DEC_LINK_MAX_REQ];
    Utils_QueHandle reqQue;
    DecLink_ReqObj *reqQueMem[DEC_LINK_MAX_REQ];
    volatile System_VideoLinkState state;
    Bool isReqPend;
    UInt32 inBufPutCount;
    UInt32 inBufGetCount;
    System_LinkInfo info;
    DecLink_ChObj chObj[DEC_LINK_MAX_CH];
    DecLink_CreateParams createArgs;
    DecLink_periodicObj periodicObj;
    DecLink_DupObj dupObj;
    Utils_QueHandle processDoneQue;
    DecLink_ReqObj *processDoneQueMem[DEC_LINK_MAX_OUT_FRAMES];
    struct decProcessTsk_s {
        Task_Struct tskStruct;
        Task_Handle tsk;
        char name[DEC_LINK_MAX_TASK_NAME_SIZE];
        Utils_QueHandle processQue;
        DecLink_ReqObj *processQueMem[DEC_LINK_MAX_OUT_FRAMES];
        HDVICP_tskEnv tskEnv;
    } decProcessTsk[NUM_HDVICP_RESOURCES];
    struct decIVASwitchSerializeObj_s {
        Utils_QueHandle freeSerializerQue;
        ti_sysbios_knl_Semaphore_Struct *freeSerializerQueMem[DEC_LINK_MAX_IVASWITCH_SERIALIZERS];
        ti_sysbios_knl_Semaphore_Struct freeSerializerSemMem[DEC_LINK_MAX_IVASWITCH_SERIALIZERS];
    } decIVASwitchSerializeObj;
    Bool newDataProcessOnFrameFree;

    UInt32 statsStartTime;
    UInt32 memUsed[UTILS_MEM_MAXHEAPS];
    DecLink_ReqBatch reqObjBatch[NUM_HDVICP_RESOURCES];
    DecLink_ReqBatchStatistics batchStatistics[NUM_HDVICP_RESOURCES];
    DecLink_DebugBatchPrepStats debugBatchPrepStats[NUM_HDVICP_RESOURCES];
    Utils_MsgHndl *pMsgTmp;
    Int32 lateAckStatus;
} DecLink_Obj;

Int32 DecLink_getInfo(Utils_TskHndl * pTsk, System_LinkInfo * info);
Int32 DecLink_getFullFrames(Utils_TskHndl * pTsk, UInt16 queId,
                            FVID2_FrameList * pFrameList);
Int32 DecLink_putEmptyFrames(Utils_TskHndl * pTsk, UInt16 queId,
                             FVID2_FrameList * pFrameList);

Int32 DecLink_codecCreate(DecLink_Obj * pObj, DecLink_CreateParams * pPrm);
Int32 DecLink_codecProcessData(DecLink_Obj * pObj);
Int32 DecLink_codecGetProcessedDataMsgHandler(DecLink_Obj * pObj);
Int32 DecLink_codecStop(DecLink_Obj * pObj);
Int32 DecLink_codecDelete(DecLink_Obj * pObj);
Int32 DecLink_codecDisableChannel(DecLink_Obj * pObj,
                              DecLink_ChannelInfo* params);
Int32 DecLink_codecEnableChannel(DecLink_Obj * pObj,
                              DecLink_ChannelInfo* params);
Int32 DecLink_setTPlayConfig(DecLink_Obj * pObj,
                              DecLink_TPlayConfig* params);
Int32 DecLink_getCurLinkID(Void * key);
Int32 DecLink_codecGetProcessedDataMsgHandler(DecLink_Obj * pObj);
Int32 DecLink_codecFreeProcessedFrames(DecLink_Obj * pObj,
                                       FVID2_FrameList * freeFrameList);


Int32 Declink_h264DecodeFrameBatch(DecLink_Obj * pObj,
                                   DecLink_ReqBatch * pReqObjBatch,
                                   FVID2_FrameList * freeFrameList, Int32 tskId);
Int32 Declink_jpegDecodeFrameBatch(DecLink_Obj * pObj,
                                   DecLink_ReqBatch * pReqObjBatch,
                                   FVID2_FrameList * freeFrameList);
Int32 Declink_mpeg4DecodeFrameBatch(DecLink_Obj * pObj,
                                    DecLink_ReqBatch * pReqObjBatch,
                                    FVID2_FrameList * freeFrameList);

Int32 DecLink_printStatistics (DecLink_Obj * pObj, Bool resetAfterPrint);
Int32 DecLink_resetStatistics(DecLink_Obj * pObj);
Int32 DecLink_printBufferStatus (DecLink_Obj * pObj);
Int32 DecLink_codecIVAMapChangeHandler(DecLink_Obj * pObj);
Int32 DecLink_codecCreateChannelHandler(DecLink_Obj * pObj,
                                        DecLink_addChannelInfo* params);
Int32 DecLink_codecDeleteChannelHandler(DecLink_Obj * pObj,
                                        DecLink_ChannelInfo* params);
Int32 DecLink_getBufferStatus (DecLink_Obj * pObj,DecLink_BufferStats *bufStats);

Int32 DecLink_resetDecErrorReporting(DecLink_Obj * pObj, 
                                     DecLink_ChErrorReport * decErrReportCtrl);

Int32 DecLinkH264_codecFlush(DecLink_ChObj *pChObj,
                             IH264VDEC_InArgs *inArgs,
                             IH264VDEC_OutArgs *outArgs,
                             XDM2_BufDesc *inputBufDesc,
                             XDM2_BufDesc *outputBufDesc,
                             IH264VDEC_Handle handle,
                             FVID2_FrameList *freeFrameList,
                             Bool hardFlush);

Int32 DecLinkMPEG4_codecFlush(DecLink_ChObj *pChObj,
                              IMPEG4VDEC_InArgs *inArgs,
                              IMPEG4VDEC_OutArgs *outArgs,
                              XDM2_BufDesc *inputBufDesc,
                              XDM2_BufDesc *outputBufDesc,
                              IMPEG4VDEC_Handle handle,
                              FVID2_FrameList *freeFrameList,
                              Bool hardFlush);

#ifdef SYSTEM_DEBUG_DEC
#define DECLINK_INFO_LOG(linkID,chID,...)       do {                           \
                                                     Vps_printf(               \
                                                      "\n %d: DECODE: CH%d: ", \
                                                      Utils_getCurTimeInMsec(), chID); \
                                                      Vps_printf(__VA_ARGS__);\
                                                } while(0)
#else
#define DECLINK_INFO_LOG(linkID,chID,...)
#endif

#ifdef SYSTEM_DEBUG_DEC_VERBOSE
#define DECLINK_INFO_LOG_VERBOSE(linkID,chID,...)      DECLINK_INFO_LOG(linkID,\
                                                                        chID,  \
                                                                    __VA_ARGS__);
#else
#define DECLINK_INFO_LOG_VERBOSE(linkID,chID,...)
#endif

#ifdef SYSTEM_DEBUG_DEC_RT
#define DECLINK_INFO_LOG_RT(...)
#else
#define DECLINK_INFO_LOG_RT(...)
#endif

#define DECLINK_VARGS_NUMARGS(...)  (sizeof((int[]){(int)__VA_ARGS__})/sizeof(int))

#define DECLINK_INTERNAL_ERROR_LOG(errCode,...)                                \
      do {                                                                     \
        Vps_printf("\n%d:DECLINK::%s:[%d]::INTERNAL ERROR:%d",                 \
                   Utils_getCurTimeInMsec(),                                           \
                   __FILE__,                                                   \
                   __LINE__,                                                   \
                   errCode);                                                   \
        if(DECLINK_VARGS_NUMARGS(__VA_ARGS__)) {                               \
            Vps_printf(__VA_ARGS__);                                           \
        }                                                                      \
      } while (0)

#endif                                                     /* _DEC_LINK_PRIV_H_
                                                            */
