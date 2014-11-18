/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#ifndef _NSF_LINK_PRIV_H_
#define _NSF_LINK_PRIV_H_

#include <mcfw/src_bios6/links_m3vpss/system/system_priv_m3vpss.h>
#include <mcfw/interfaces/link_api/nsfLink.h>

/* Maximum NSF link objects */
#define NSF_LINK_OBJ_MAX                    (4)

/* Maximum supported output queues */
#define NSF_LINK_MAX_OUTPUTS_PER_INST       (2)

/* Maximum frames allocated per channel */
#define NSF_LINK_FRAMES_PER_CH_MAX         (SYSTEM_LINK_FRAMES_PER_CH + 2)
#define NSF_LINK_FRAMES_PER_CH_DEFAULT     (NSF_LINK_FRAMES_PER_CH_MAX)

/* Maximum NF channels per output queue */
#define NSF_LINK_MAX_CH_PER_QUE             (18)

/* Define that controls whether input buffer should be dropped if
 * output buffer is unavailable at that point */
#define NSF_LINK_DROP_INPUT_ON_OUTBUF_UNAVAILABLE  (TRUE)

/*
 * Per Channel Information */
typedef struct {
    /* Channel number */
    UInt32 channelId;

    /*
     * Input/output frame queue - per channel
     * Utils_BufHndl.fullQue will be used to fill frames coming from
     * capture link.
     *
     * Utils_BufHndl.emptyQue will be used to fill the noise-filtered frames.
     * These frames will be allocated during initialization and used in the
     * FVID2_processFrames() API.
     */
    Utils_BufHndl bufInQue;

    /*
     * Maximum FVID2 Frames that may be used for noise filtering
     */
    FVID2_Frame frames[NSF_LINK_FRAMES_PER_CH_MAX];

    /*
     * Run-time parameter structure for every possible frame
     */
    System_FrameInfo frameInfo[NSF_LINK_FRAMES_PER_CH_MAX];

    /* previous output frame */
    FVID2_Frame *pPrevOutFrame;

    UInt32 nextFid;

    Bool doFrameDrop;

    /* input frame recevied from previous link */
    UInt32 inFrameRecvCount;

    /* input frame rejected due mismatch in FID */
    UInt32 inFrameRejectCount;

    /* input frames actually processed */
    UInt32 inFrameProcessCount;

    /* output frames skip based on user setting like 'skip alternate' frame */
    UInt32 outFrameUserSkipCount;

    /* output frames skip based due to output buffer not available */
    UInt32 outFrameSkipCount;

    /* number of frames actually output */
    UInt32 outFrameCount;

    /* Data structure for frame skip to achieve expected output frame rate */
    Utils_frameSkipContext frameSkipCtx;

    Vps_NsfDataFormat nsfRtParamDataFormat;
    Vps_M2mNsfRtParams nsfRtParams;

} NsfLink_ChObj;

typedef struct {
    /* NSF task structure */
    UInt32                 linkId;
    Utils_TskHndl tsk;
    char                   name[32];
    
    /* NSF Link create params - to be passed from the application */
    NsfLink_CreateParams createArgs;

    /* NSF link info */
    System_LinkInfo info;

    /* Previous link (Capture) info */
    System_LinkInfo inTskInfo;

    /* Previous link's (Capture) queue info - only 1 input queue is allowed */
    System_LinkQueInfo inQueInfo;

    /* Maximum 2 output queues */
    Utils_BufHndl bufOutQue[NSF_LINK_MAX_OUT_QUE];

    Utils_BufHndl bufEvenFieldOutQue;

    FVID2_Frame evenFieldFrames[NSF_LINK_FRAMES_PER_CH_MAX *
                                NSF_LINK_MAX_CH_PER_QUE];

    UInt32 curEvenFieldFrame;

    /* NSF channels related info */
    NsfLink_ChObj linkChInfo[NSF_LINK_MAX_CH_PER_QUE];

    /* Semaphore to signal completion of noise filtering */
    Semaphore_Handle complete;

    /* NSF FVID2 handle */
    FVID2_Handle fvidHandleNsf;

    /* Error process list - not used currently */
    FVID2_ProcessList errCbProcessList;

    /* Channel specific configuration information */
    Vps_NsfDataFormat nsfDataFormat[NSF_LINK_MAX_CH_PER_QUE];
    Vps_NsfProcessingCfg nsfProcCfg[NSF_LINK_MAX_CH_PER_QUE];

    /* Create params and returned status stored here */
    Vps_NsfCreateParams nsfCreateParams;
    Vps_NsfCreateStatus nsfCreateStatus;        /**TBD: used nowhere ??? */

    UInt32 inFrameReceiveCount;
    UInt32 inFrameGivenCount;
    UInt32 outFrameReceiveCount;
    UInt32 outFrameGivenCount;
    UInt32 processFrameReqCount;
    UInt32 getProcessFrameReqCount;
    UInt32 getFrames;

    UInt32                 statsStartTime;
    UInt32                 processFrameCount;

    UInt32 curTime;
    UInt32 totalTime;
    UInt32 memUsed[UTILS_MEM_MAXHEAPS];

} NsfLink_Obj;

Int32 NsfLink_getInfo(Utils_TskHndl * pTsk, System_LinkInfo * info);
Int32 NsfLink_getFullFrames(Utils_TskHndl * pTsk, UInt16 queId,
                            FVID2_FrameList * pFrameList);
Int32 NsfLink_putEmptyFrames(Utils_TskHndl * pTsk, UInt16 queId,
                             FVID2_FrameList * pFrameList);
Int32 NsfLink_drvCreate(NsfLink_Obj * pObj, NsfLink_CreateParams * pPrm);
Int32 NsfLink_drvProcessData(NsfLink_Obj * pObj);
Int32 NsfLink_drvDoNsfFilter(NsfLink_Obj * pObj);
Int32 NsfLink_drvPutEmptyFrames(NsfLink_Obj * pObj,
                                FVID2_FrameList * pFrameList);
Int32 NsfLink_drvDelete(NsfLink_Obj * pObj);
Int32 NsfLink_drvInitCh(NsfLink_Obj * pObj);
Int32 NsfLink_drvFreeFrames(NsfLink_Obj * pObj);
Int32 NsfLink_SetFrameRate(NsfLink_Obj * pObj, NsfLink_ChFpsParams * params);

Int32 NsfLink_drvPrintStatistics(NsfLink_Obj * pObj, Bool resetAfterPrint);
Int32 NsfLink_resetStatistics(NsfLink_Obj *pObj);
Int32 NsfLink_printBufferStatus(NsfLink_Obj * pObj);

#endif
