/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#ifndef _ENC_LINK_PRIV_H_
#define _ENC_LINK_PRIV_H_

#include <xdc/std.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/gates/GateMutexPri.h>
#include <mcfw/src_bios6/links_m3vpss/system/system_priv_m3vpss.h>
#include <mcfw/interfaces/link_api/encLink.h>
#include <mcfw/interfaces/common_def/ti_venc_common_def.h>
#include <mcfw/src_bios6/links_m3video/system/system_priv_m3video.h>
#include <mcfw/src_bios6/links_m3video/codec_utils/utils_encdec.h>
#include <mcfw/interfaces/link_api/system_debug.h>
#include <mcfw/src_bios6/links_m3video/codec_utils/utils_encdec_prf.h>
#include "encLink_algIf.h"
#include "encLink_h264_priv.h"
#include "encLink_jpeg_priv.h" //Added MJPEG Encoder Support

/* =============================================================================
 * All success and failure codes for the module
 * ============================================================================= */

/** @brief Operation successful. */
#define ENC_LINK_S_SUCCESS                   (0)

/** @brief General Failure */
#define ENC_LINK_E_FAIL                      (-1)

/** @brief Argument passed to function is invalid. */
#define ENC_LINK_E_INVALIDARG                (-2)

/** @brief Encoder algorithm create failed */
#define ENC_LINK_E_ALGCREATEFAILED           (-3)

/** @brief RMAN assign resource failed */
#define ENC_LINK_E_RMANRSRCASSIGNFAILED      (-4)

/** @brief XDM_SETPARAM failed */
#define ENC_LINK_E_ALGSETPARAMSFAILED        (-5)

/** @brief Unknown codec type failed */
#define ENC_LINK_E_UNSUPPORTEDCODEC          (-6)

/** @brief Creation of task failed */

#define ENC_LINK_E_TSKCREATEFAILED           (-7)

/** @brief XDM_GETBUFINFO failed */
#define ENC_LINK_E_ALGGETBUFINFOFAILED       (-8)

/** @brief Enclink internal to indicate IVA map change */
#define ENC_LINK_CMD_INTERNAL_IVAMAPCHANGE   (0)


#define ENC_LINK_OBJ_MAX                     (SYSTEM_LINK_ID_VENC_COUNT)

#define ENC_LINK_MAX_OUT_FRAMES_PER_CH       (SYSTEM_LINK_FRAMES_PER_CH + 2)

#define ENC_LINK_MAX_OUT_FRAMES              (ENC_LINK_MAX_CH*ENC_LINK_MAX_OUT_FRAMES_PER_CH)

#define ENC_LINK_CMD_GET_PROCESSED_DATA      (0x6000)

#define ENC_LINK_MAX_REQ                     (ENC_LINK_MAX_OUT_FRAMES)

#define ENC_LINK_MAX_IVASWITCH_SERIALIZERS   (ENC_LINK_MAX_CH)

#define ENC_LINK_MAX_REQ_OBJ_DUMMY           (ENC_LINK_MAX_IVASWITCH_SERIALIZERS * 2)

#define ENC_LINK_PROCESS_TSK_SIZE            (8 * KB)

#define ENC_LINK_MAX_TASK_NAME_SIZE          (32)

#define ENC_LINK_TASK_POLL_DURATION_MS       (8)

#define ENC_LINK_PROCESS_DONE_PERIOD_MS      (4)
#define ENC_LINK_STATS_START_THRESHOLD       (5)
#define ENC_LINK_REQLIST_MAX_REQOBJS         (4)
#define ENC_LINK_NUM_ALGPROCESS_PER_HDVICP_ACQUIRE                       (8)
#define ENC_LINK_DEFAULT_ALGPARAMS_STARTX                                (0)
#define ENC_LINK_DEFAULT_ALGPARAMS_STARTY                                (0)
#define ENC_LINK_DEFAULT_ALGPARAMS_MVACCURACY  \
                                             (IVIDENC2_MOTIONVECTOR_QUARTERPEL)
#define ENC_LINK_DEFAULT_ALGPARAMS_TARGETFRAMERATEX1000                  (30000)
#define ENC_LINK_DEFAULT_ALGPARAMS_REFFRAMERATEX1000                     (30000)
/*Added MJPEG Encoder Support*/
#define ENC_LINK_MJPEG_DEFAULT_ALGPARAMS_TARGETFRAMERATEX1000            (1000)
#define ENC_LINK_MJPEG_DEFAULT_ALGPARAMS_REFFRAMERATEX1000               (1000)

/**
 *****************************************************************************
 * @def    ENC_LINK_DEFAULT_ALGPARAMS_MAXINTERFRAMEINTERVAL
 * @brief  Default interframe interval
 *
 * I to P frame distance. e.g. = 1 if no B frames, 2 to insert one B frame.
 * @remarks   This is used for setting the maximum number of B frames
 *            between two refererence frames.
 *****************************************************************************
*/

#define ENC_LINK_DEFAULT_ALGPARAMS_MAXINTERFRAMEINTERVAL         (1)
#define ENC_LINK_DEFAULT_ALGPARAMS_INTRAFRAMEINTERVAL            (30)
#define ENC_LINK_DEFAULT_ALGPARAMS_ENCODINGPRESET                (XDM_DEFAULT)
#define ENC_LINK_DEFAULT_ALGPARAMS_ANALYTICINFO                  (0)
#define ENC_LINK_DEFAULT_ALGPARAMS_ENABLEWATERMARKING            (0)
#define ENC_LINK_DEFAULT_ALGPARAMS_INPUTFRAMERATE                (30)
#define ENC_LINK_DEFAULT_ALGPARAMS_RATECONTROLPRESET             (IVIDEO_STORAGE)
#define ENC_LINK_DEFAULT_ALGPARAMS_TARGETBITRATE                 (2 * 1000 * 1000)
#define ENC_LINK_DEFAULT_ALGPARAMS_MAXBITRATE                    (-1)
#define ENC_LINK_DEFAULT_ALGPARAMS_VBRDURATION                   (8)
#define ENC_LINK_DEFAULT_ALGPARAMS_VBRSENSITIVITY                (0)
#define ENC_LINK_DEFAULT_ALGPARAMS_QPMIN                         (0)
#define ENC_LINK_DEFAULT_ALGPARAMS_QPMAX                         (40)
#define ENC_LINK_DEFAULT_ALGPARAMS_QPINIT                        (-1)



/**
 *****************************************************************************
 * @def    ENC_LINK_SETCONFIG_BITMASK
 * @brief  Bit mask values for each dynamic encoder configuration paramters
 *****************************************************************************
*/
#define ENC_LINK_SETCONFIG_BITMASK_BITRARATE                (0)
#define ENC_LINK_SETCONFIG_BITMASK_FPS                      (1)
#define ENC_LINK_SETCONFIG_BITMASK_INTRAI                   (2)
#define ENC_LINK_SETCONFIG_BITMASK_FORCEI                   (3)
#define ENC_LINK_SETCONFIG_BITMASK_QPI                      (4)
#define ENC_LINK_SETCONFIG_BITMASK_QPP                      (5)
#define ENC_LINK_SETCONFIG_BITMASK_RCALGO                   (6)
#define ENC_LINK_SETCONFIG_BITMASK_VBRD                     (7)
#define ENC_LINK_SETCONFIG_BITMASK_VBRS                     (8)
#define ENC_LINK_SETCONFIG_BITMASK_ROI                      (9)
#define ENC_LINK_SETCONFIG_BITMASK_RESET_VALUE              (0xFFFFFFFF)

#define ENC_LINK_MV_DATA_SIZE           (8 * 1024)
/*
 *****************************************************************************
 * @def    ENC_LINK_GROUP_SUBM_MAX_SIZE
 * @brief  Defines the maximum submission batch size for the Encoder codec as
 *         dictated by the codec user guide
 *         Currently relevant only for the H.264 encoder.Setting this value
 *         as 1 is equivalent to having simple process calls for the H.264
 *         encoder.
 *****************************************************************************
*/
#ifdef SYSTEM_DEBUG_DISABLE_PROCESS_MULTI
#define ENC_LINK_GROUP_SUBM_MAX_SIZE   1
#else
#define ENC_LINK_GROUP_SUBM_MAX_SIZE   24
#endif

#define ENC_LINK_GROUP_CODEC_CLASS_H264 0
#define ENC_LINK_GROUP_CODEC_CLASS_JPEG 1


//#define SYSTEM_DEBUG_MULTI_CHANNEL_ENC
//#define SYSTEM_DEBUG_MULTI_CHANNEL_ENC_OVERHEAD_LOG

#define ENC_LINK_SIZEOF_ANALYTICHEADERINFO    \
       (sizeof(EncLink_h264_AnalyticHeaderInfo))

/* Adding another 100 bytes just as a safety measure to the calculated MV buffer size*/
#define ENC_LINK_MVSIZE_SAFETY_BYTES  (100)


/** @enum ENC_LINK_GET_MVBUF_SIZE
 *  @brief Macro that returns max size of mv buffer for a frame
 *  MVData size = 
     (76 + ( (MBs_In_Picture + 63) & 0xFFFFFFC0) * 2 + (MBs_In_Picture + 512) * 8 ) bytes
 *   
 */
#define ENC_LINK_GET_MVBUF_SIZE(width,height)          \
        (ENC_LINK_SIZEOF_ANALYTICHEADERINFO + \
        (((((width) * (height)/ (16 * 16)) + 63) & 0xffffffc0)*2) + \
        ((((width) * (height)/ (16 * 16)) + 512) *8) + ENC_LINK_MVSIZE_SAFETY_BYTES)


typedef enum EncLink_ReqObjType_e
{
    ENC_LINK_REQ_OBJECT_TYPE_REGULAR,
    ENC_LINK_REQ_OBJECT_TYPE_DUMMY_IVAMAPCHANGE_PREVIVALAST,
    ENC_LINK_REQ_OBJECT_TYPE_DUMMY_IVAMAPCHANGE_NEXTIVAFIRST
} EncLink_ReqObjType_e;

typedef struct EncLink_processFrameRate {
   Int32 firstTime;
   Int32 inCnt;
   Int32 outCnt;
   Int32 multipleCnt;
} EncLink_processFrameRate;

typedef struct EncLink_OutObj {
    Utils_BitBufHndl bufOutQue;
    UInt32 numAllocPools;
    Bitstream_Buf outBufs[ENC_LINK_MAX_OUT_FRAMES];
    System_FrameInfo frameInfo[ENC_LINK_MAX_OUT_FRAMES];
    UInt32 outNumBufs[UTILS_BITBUF_MAX_ALLOC_POOLS];
    UInt32 buf_size[UTILS_BITBUF_MAX_ALLOC_POOLS];
    UInt32 ch2poolMap[ENC_LINK_MAX_CH];
} EncLink_OutObj;

typedef struct EncLink_ReqObj {
    FVID2_FrameList InFrameList;
    Bitstream_Buf *OutBuf;
    EncLink_ReqObjType_e type;
    ti_sysbios_knl_Semaphore_Handle     ivaSwitchSerializer;
} EncLink_ReqObj;

typedef struct EncLink_ReqList {
    UInt32 numReqObj;
    EncLink_ReqObj *reqObj[ENC_LINK_REQLIST_MAX_REQOBJS];
} EncLink_ReqList;

typedef struct EncLink_ReqBatch {
    UInt32 numReqObjsInBatch;
    EncLink_ReqObj *pReqObj[ENC_LINK_GROUP_SUBM_MAX_SIZE];
    UInt64 channelSubmittedFlag;
    UInt32 codecSubmittedFlag;
} EncLink_ReqBatch;

typedef struct EncLink_algObj {
    union {
        EncLink_H264Obj h264AlgIfObj;
        EncLink_JPEGObj jpegAlgIfObj;//Added MJPEG Encoder Support
    } u;
    EncLink_AlgCreateParams algCreateParams;
    EncLink_AlgDynamicParams algDynamicParams;
    UInt32 setConfigBitMask;
    UInt32 getConfigFlag;
} EncLink_algObj;

typedef struct EncLink_periodicObj {
    ti_sysbios_knl_Clock_Struct clkStruct;
    ti_sysbios_knl_Clock_Handle clkHandle;
    Bool clkStarted;
} EncLink_periodicObj;

typedef struct EncLink_ChObj {
    Utils_QueHandle inQue;
    EncLink_algObj algObj;
    EncLink_processFrameRate frameStatus;
    Bool disableChn;
    Int32  inputFrameRate;
    Uint32 curFrameNum;
    UInt32 nextFid;
    UInt32 allocPoolID;
    UInt32 prevFrmRecvTime;
    UInt32 totalProcessTime;
    UInt32 totalFrameIntervalTime;
    UInt32 totalInFrameCnt;
    UInt32 inFrameSkipCount;
    UInt32 inFrameQueCount;
    UInt32 processReqestCount;
    UInt32 getProcessedFrameCount;
    UInt32 numReqObjPerProcess;
    FVID2_Frame *inFrameMem[ENC_LINK_MAX_REQ];

    UInt32 inFrameRecvCount;
    UInt32 inFrameRejectCount;
    UInt32 inFrameUserSkipCount;
    UInt32 outFrameCount;
    UInt32 minLatency;
    UInt32 maxLatency;
    Bool   forceAvoidSkipFrame;
    Bool   forceDumpFrame;
    Bitstream_Buf dummyBitBuf;
    ti_sysbios_knl_Semaphore_Struct codecProcessMutexMem;
    ti_sysbios_knl_Semaphore_Handle codecProcessMutex;
} EncLink_ChObj;
typedef struct EncLink_ReqBatchStatistics {
    UInt32 numBatchesSubmitted;
    UInt32 currentBatchSize;
    UInt32 aggregateBatchSize;
    UInt32 averageBatchSize;
    UInt32 maxAchievedBatchSize;
} EncLink_ReqBatchStatistics;

typedef struct EncLink_DebugBatchPrepStats {
    UInt32 numBatchCreated;
    UInt32 numReasonSizeExceeded;
    UInt32 numReasonReqObjQueEmpty;
    UInt32 numReasonResoultionClass;
    UInt32 numReasonContentType;
    UInt32 numReasonChannelRepeat;
    UInt32 numReasonCodecSwitch;
} EncLink_DebugBatchPrepStats;

typedef struct EncLink_Obj {
    UInt32 linkId;
    Utils_TskHndl tsk;
    System_LinkInfo inTskInfo;
    System_LinkQueInfo inQueInfo;
    EncLink_OutObj outObj;
    EncLink_ReqObj reqObj[ENC_LINK_MAX_REQ];
    Utils_QueHandle reqQue;
    EncLink_ReqObj *reqQueMem[ENC_LINK_MAX_REQ];
    struct encDummyReqObj_s {
        EncLink_ReqObj reqObjDummy[ENC_LINK_MAX_REQ_OBJ_DUMMY];
        Utils_QueHandle reqQueDummy;
        EncLink_ReqObj *reqQueMemDummy[ENC_LINK_MAX_REQ_OBJ_DUMMY];
    } encDummyReqObj;
    volatile System_VideoLinkState state;
    Bool isReqPend;
    UInt32 inFrameGetCount;
    UInt32 inFramePutCount;
    System_LinkInfo info;
    UInt32 ch2ProcessTskId[ENC_LINK_MAX_CH];
    UInt32 ch2ProcessTskIdNext[ENC_LINK_MAX_CH];
    EncLink_ChObj chObj[ENC_LINK_MAX_CH];
    EncLink_CreateParams createArgs;
    EncLink_periodicObj periodicObj;
    Utils_QueHandle processDoneQue;
    EncLink_ReqObj *processDoneQueMem[ENC_LINK_MAX_OUT_FRAMES];
    struct encProcessTsk_s {
        ti_sysbios_knl_Task_Struct tskStruct;
        ti_sysbios_knl_Task_Handle tsk;
        char name[ENC_LINK_MAX_TASK_NAME_SIZE];
        Utils_QueHandle processQue;
        EncLink_ReqObj *processQueMem[ENC_LINK_MAX_OUT_FRAMES];
        HDVICP_tskEnv tskEnv;
    } encProcessTsk[NUM_HDVICP_RESOURCES];
    ti_sysbios_gates_GateMutexPri_Struct  ivaChMapMutexObj;
    ti_sysbios_gates_GateMutexPri_Handle  ivaChMapMutex;
    struct encIVASwitchSerializeObj_s {
        Utils_QueHandle freeSerializerQue;
        ti_sysbios_knl_Semaphore_Struct *freeSerializerQueMem[ENC_LINK_MAX_IVASWITCH_SERIALIZERS];
        ti_sysbios_knl_Semaphore_Struct freeSerializerSemMem[ENC_LINK_MAX_IVASWITCH_SERIALIZERS];
    } encIVASwitchSerializeObj;

    UInt32 statsStartTime;
    UInt32 memUsed[UTILS_MEM_MAXHEAPS];
    EncLink_ReqBatch reqObjBatch[NUM_HDVICP_RESOURCES];
    EncLink_ReqBatchStatistics batchStatistics[NUM_HDVICP_RESOURCES];
    EncLink_DebugBatchPrepStats debugBatchPrepStats[NUM_HDVICP_RESOURCES];

} EncLink_Obj;

Int32 EncLink_getInfo(Utils_TskHndl * pTsk, System_LinkInfo * info);
Int32 EncLink_getFullBufs(Utils_TskHndl * pTsk, UInt16 queId,
                          Bitstream_BufList * pBufList);
Int32 EncLink_putEmptyBufs(Utils_TskHndl * pTsk, UInt16 queId,
                           Bitstream_BufList * pBufList);

Int32 EncLink_codecCreate(EncLink_Obj * pObj, EncLink_CreateParams * pPrm);
Int32 EncLink_codecProcessData(EncLink_Obj * pObj);
Int32 EncLink_codecGetProcessedDataMsgHandler(EncLink_Obj * pObj);
Int32 EncLink_codecStop(EncLink_Obj * pObj);
Int32 EncLink_codecDelete(EncLink_Obj * pObj);
Int32 EncLink_getCurLinkID(Void * key);

Int32 EncLink_codecGetProcessedDataMsgHandler(EncLink_Obj * pObj);

Int32 Enclink_h264EncodeFrameBatch(EncLink_Obj * pObj, 
                                   EncLink_ReqBatch * reqObjBatch,
                                   Int32 tskId);

Int32 Enclink_jpegEncodeFrame(EncLink_ChObj * pChObj,
                              EncLink_ReqObj * pReqObj);

Int32 EncLink_codecGetDynParams(EncLink_Obj * pObj,
                              EncLink_GetDynParams * params);
Int32 EncLink_codecSetBitrate(EncLink_Obj * pObj,
                              EncLink_ChBitRateParams * parms);
Int32 EncLink_codecSetFps(EncLink_Obj * pObj, EncLink_ChFpsParams * params);
Int32 EncLink_codecInputSetFps(EncLink_Obj * pObj, EncLink_ChInputFpsParam * params);
Int32 EncLink_codecSetIntraIRate(EncLink_Obj * pObj, EncLink_ChIntraFrIntParams * params);
Int32 EncLink_codecSetForceIDR(EncLink_Obj * pObj, EncLink_ChForceIFrParams * params);
Int32 EncLink_codecSetrcAlg(EncLink_Obj * pObj, EncLink_ChRcAlgParams* params);
Int32 EncLink_codecSetqpParamI(EncLink_Obj * pObj, EncLink_ChQPParams * params);
Int32 EncLink_codecSetqpParamP(EncLink_Obj * pObj, EncLink_ChQPParams * params);
Int32 EncLink_codecForceDumpFrame(EncLink_Obj * pObj, EncLink_ChannelInfo * params);
Int32 EncLink_codecSetVBRDuration(EncLink_Obj * pObj, EncLink_ChCVBRDurationParams *params);
Int32 EncLink_codecSetVBRSensitivity(EncLink_Obj * pObj, EncLink_ChCVBRSensitivityParams *params);
Int32 EncLink_codecSetROIPrms(EncLink_Obj * pObj, EncLink_ChROIParams * params);


Int32 EncLink_codecDisableChannel(EncLink_Obj * pObj,
                              EncLink_ChannelInfo* params);
Int32 EncLink_codecEnableChannel(EncLink_Obj * pObj,
                              EncLink_ChannelInfo* params);
Bool  EncLink_doSkipFrame(EncLink_ChObj *pChObj, Int32 chId);
Int32 EncLinkH264_algSetConfig(EncLink_algObj * algObj);

Int32 EncLinkH264_algGetConfig(EncLink_algObj * algObj);
Int32 EncLinkJPEG_algSetConfig(EncLink_algObj * algObj);
Int32 EncLinkJPEG_algGetConfig(EncLink_algObj * algObj);
Int EncLinkH264_algDynamicParamUpdate(EncLink_H264Obj * hObj,
                               EncLink_AlgCreateParams * algCreateParams,
                               EncLink_AlgDynamicParams * algDynamicParams);
Int EncLinkJPEG_algDynamicParamUpdate(EncLink_JPEGObj * hObj,
                               EncLink_AlgCreateParams * algCreateParams,
                               EncLink_AlgDynamicParams * algDynamicParams);

Int32 EncLink_resetStatistics(EncLink_Obj * pObj);
Int32 EncLink_printStatistics (EncLink_Obj * pObj, Bool resetAfterPrint);
Int32 EncLink_printBufferStatus (EncLink_Obj * pObj);
Int32 EncLink_codecIVAMapChangeHandler(EncLink_Obj * pObj);

#ifdef SYSTEM_DEBUG_ENC
#define ENCLINK_INFO_LOG(linkID,chID,...)       do {                           \
                                                     Vps_printf(               \
                                                      "\n %d: ENCODE: CH%d: ", \
                                                      Utils_getCurTimeInMsec(),chID); \
                                                      Vps_printf(__VA_ARGS__);\
                                                } while(0)
#else
#define ENCLINK_INFO_LOG(...)
#endif

#ifdef SYSTEM_DEBUG_ENC_VERBOSE
#define ENCLINK_VERBOSE_INFO_LOG(linkID,chID,...)  ENCLINK_INFO_LOG(linkID, \
                                                                    chID,   \
                                                                    __VA_ARGS__);
#else
#define ENCLINK_VERBOSE_INFO_LOG(...)
#endif

#ifdef SYSTEM_DEBUG_ENC_RT
#define ENCLINK_INFO_LOG_RT(info)                    
#else
#define ENCLINK_INFO_LOG_RT(info)
#endif

#define ENCLINK_VARGS_NUMARGS(...)  (sizeof((int[]){(int)__VA_ARGS__})/sizeof(int))

#define ENCLINK_INTERNAL_ERROR_LOG(errCode,...)                                \
      do {                                                                     \
        Vps_printf("\n%d:!ERROR!:ENCLINK::%s:[%d]::INTERNAL ERROR:%d",         \
                   Utils_getCurTimeInMsec(),                                   \
                   __FILE__,                                                   \
                   __LINE__,                                                   \
                   errCode);                                                   \
        if(ENCLINK_VARGS_NUMARGS(__VA_ARGS__)) {                               \
            Vps_printf(__VA_ARGS__);                                           \
        }                                                                      \
      } while (0)

#endif                                                     /* _ENC_LINK_PRIV_H_
                                                            */
