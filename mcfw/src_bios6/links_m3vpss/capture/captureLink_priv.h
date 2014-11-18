 /*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/*******************************************************************************
    Channel Numbers in capture link
    ===============================

    Capture driver needs that 'channelNum' be unique across all capture
    driver instances.

    Capture link requires that 'channelNum' of frames put in the output
    queue start from 0 and go upto ((number of channels in que) - 1).

    This is done so that the subsequent link need not have to do any channel
    number remapping and complexity of channel number remapping is retricted
    within the capture link.

    Due to this it could happen that 'channelNum' of frame returned by
    capture driver could be, say, 0xNN, and 'channelNum' of the same frame
    when put in output queue needs to be, say, 0xMM.

    This 'channelNum' remapping between capture driver and capture link output
    queue is done as below.

    For a given capture driver channelNum,
    - first 4bits (CAPTURE_LINK_MAX_CH_PER_OUT_QUE) is CH ID of channel in QUE
      and
    - rest of the bits are output queue number 0..CAPTURE_LINK_MAX_OUT_QUE-1

    When a frame is put in the output queue only the CH ID of channel in QUE is
    retained in the FVID2_Frame

    When a frame is returned back to the capture driver this CH ID is combined
    with QUE ID to form 'channelNum' for capture driver.

    Examples,

    For 4CH per port input and single output queue channelNum will be as follows

    Capture driver              Output      Output Que
    channelNum                  QUE ID      channelNum
    0           CH0 VIP0 PortA  0           0
    1           CH1 VIP0 PortA  0           1
    2           CH2 VIP0 PortA  0           2
    3           CH3 VIP0 PortA  0           3
    4           CH0 VIP0 PortB  0           4
    5           CH1 VIP0 PortB  0           5
    6           CH2 VIP0 PortB  0           6
    7           CH3 VIP0 PortB  0           7
    8           CH0 VIP1 PortA  0           8
    9           CH1 VIP1 PortA  0           9
    10          CH2 VIP1 PortA  0           10
    11          CH3 VIP1 PortA  0           11
    12          CH0 VIP1 PortB  0           12
    13          CH1 VIP1 PortB  0           13
    14          CH2 VIP1 PortB  0           14
    15          CH3 VIP1 PortB  0           15

    For 1CH input and 2 outputs per port and 4 output queue channelNum will be as follows

    Capture driver                          Output      Output Que
    channelNum                              QUE ID      channelNum
    0           CH0 VIP0 PortA - output 0     0           0
    16          CH0 VIP0 PortA - output 1     1           0
    32          CH0 VIP1 PortA - output 0     2           0
    64          CH0 VIP1 PortA - output 1     3           0

    For 1CH input and 2 outputs per port and 1 output queue channelNum will be as follows

    Capture driver                          Output      Output Que
    channelNum                              QUE ID      channelNum
    0           CH0 VIP0 PortA - output 0     0           0
    1           CH0 VIP0 PortA - output 1     0           1
    2           CH0 VIP1 PortA - output 0     0           2
    3           CH0 VIP1 PortA - output 1     0           3

    For 1CH input and 2 outputs per port and 2 output queue channelNum will be as follows

    Capture driver                          Output      Output Que
    channelNum                              QUE ID      channelNum
    0           CH0 VIP0 PortA - output 0     0           0
    1           CH0 VIP0 PortA - output 1     0           1
    16          CH0 VIP1 PortA - output 0     1           0
    17          CH0 VIP1 PortA - output 1     1           1

********************************************************************************/
#ifndef _CAPTURE_LINK_PRIV_H_
#define _CAPTURE_LINK_PRIV_H_

#include <mcfw/src_bios6/links_m3vpss/system/system_priv_m3vpss.h>
#include <mcfw/interfaces/link_api/captureLink.h>

#include <mcfw/interfaces/common_def/ti_vsys_common_def.h>
#include <mcfw/interfaces/common_def/ti_vcap_common_def.h>
#include <mcfw/interfaces/link_api/avsync.h>
#include <mcfw/src_bios6/utils/utils_dma.h>

/* special captureChannelNum for extra channel buffers */
#define CAPTURE_LINK_EXTRA_FRAME_CH_ID      (0x12345678)

#define CAPTURE_LINK_TSK_TRIGGER_COUNT   (2)

/*Temp Buffer allocaton required only for 816X PG1.1*/
#ifdef TI_8107_BUILD
#define CAPTURE_LINK_TMP_BUF_SIZE        (0)
#else
#define CAPTURE_LINK_TMP_BUF_SIZE        (1920*1200*2)
#endif

#define CAPTURE_LINK_HEIGHT_MIN_LINES    (288)

#define CAPTURE_LINK_RAW_VBI_LINES       (40)

#define CAPTURE_VIDEO_DETECT_CHECK_INTERVAL     (200) // in msec

/* frame allocated per channel - new capture approach needs more frames */
#define CAPTURE_LINK_FRAMES_PER_CH     (SYSTEM_LINK_FRAMES_PER_CH+6)

#define CAPTURE_LINK_MAX_FRAMES_PER_CH  (SYSTEM_LINK_MAX_FRAMES_PER_CH)

#define CAPTURE_LINK_MIN_FRAMES_PER_CH  (6)

#define CAPTURE_LINK_SKIP_ODD_FIELDS_THRESHOLD  (4)

/* SD crop length */
#define CAPTURE_LINK_SD_CROP_LEN_PIXELS_LEFTMARGIN       (8)
#define CAPTURE_LINK_SD_CROP_LEN_PIXELS_RIGHTMARGIN      (8)

#define CAPTURE_DRV_MAX_WIDTH_LIMIT_TI816X_PG_1_1 (960)

/* Worst case frames per handle */
#define CAPTURE_LINK_MAX_FRAMES_PER_HANDLE \
    (VPS_CAPT_STREAM_ID_MAX* \
     VPS_CAPT_CH_PER_PORT_MAX* \
      CAPTURE_LINK_MAX_FRAMES_PER_CH \
    )

/* make capture driver channelNum */
static inline UInt32 CaptureLink_makeChannelNum(UInt32 queId, UInt32 queChId)
{
    return CAPTURE_LINK_MAX_CH_PER_OUT_QUE * queId + queChId;
}

/* extract que ID from capture driver channelNum */
static inline UInt32 CaptureLink_getQueId(UInt32 channelNum)
{
    return channelNum / CAPTURE_LINK_MAX_CH_PER_OUT_QUE;
}

/* extract que CH ID from capture driver channelNum */
static inline UInt32 CaptureLink_getQueChId(UInt32 channelNum)
{
    return channelNum % CAPTURE_LINK_MAX_CH_PER_OUT_QUE;
}

/*
    This object stores information of extra frames that used for
    capture for a given channel when frames from the normal
    buffer pool are exhausted.

    This information is used only when
    CaptureLink_CreateParams.numExtraBufs > 0
*/
typedef struct {

    /*
     * FVID2 Frames that will be used for capture
     */
    FVID2_Frame frames[CAPTURE_LINK_MAX_FRAMES_PER_CH];

    /*
     * Original FVID2_frame buffer addresses. This info is needed
     * at time of free to restore original buffer pointer which might
     * have got modified when doing sdCrop in capture link
     */
    Ptr        origAddr[CAPTURE_LINK_MAX_FRAMES_PER_CH][FVID2_MAX_FIELDS][FVID2_MAX_PLANES];

    /*
     * Run-time parameter structure for every frame
     * One run-time parameter is associated to one FVID2_Frame during link create
     */
    System_FrameInfo frameInfo[CAPTURE_LINK_MAX_FRAMES_PER_CH];

    FVID2_Format format;

    Bool tilerUsed;

    UInt32 captureChannelNum;

} CaptureLink_ExtraFrameObj;

/*
 * Driver instance information */
typedef struct {

    /*
     * VIP capture driver Instance ID
     */
    UInt32 instId;

    /*
     * Capture driver create time parameters
     */
    Vps_CaptCreateParams createArgs;

    /*
     * Create status returned by capture driver during FVID2_create()
     */
    Vps_CaptCreateStatus createStatus;

    /*
     * Capture driver Callback params
     */
    FVID2_CbParams cbPrm;

    /*
     * Capture driver FVID2 handle
     */
    FVID2_Handle captureVipHandle;

    /*
     * FVID2 Frames that will be used for capture
     */
    FVID2_Frame frames[CAPTURE_LINK_MAX_FRAMES_PER_HANDLE];

    /*
     * Original FVID2_frame buffer addresses. This info is needed
     * at time of free to restore original buffer pointer which might
     * have got modified when doing sdCrop in capture link
     */
    Ptr        origAddr[CAPTURE_LINK_MAX_FRAMES_PER_HANDLE][FVID2_MAX_FIELDS][FVID2_MAX_PLANES];

    /*
     * Run-time parameter structure for every frame
     * One run-time parameter is associated to one FVID2_Frame during link create
     */
    System_FrameInfo frameInfo[CAPTURE_LINK_MAX_FRAMES_PER_HANDLE];

    /*
     * Video decoder device FVID2 handle
     */
    FVID2_Handle videoDecoderHandle;

    /* Video decoder device create args */
    Vps_VideoDecoderCreateParams vidDecCreateArgs;

    /* Video decoder device create status */
    Vps_VideoDecoderCreateStatus vidDecCreateStatus;

    /* Video decoder device video mode setup args */
    Vps_VideoDecoderVideoModeParams vidDecVideoModeArgs;

    VCAP_VIDEO_SOURCE_CH_STATUS_S vidDecCurStatus[VPS_CAPT_CH_PER_PORT_MAX];

    /** Max possible width x height across all channels
     *  Capture buffers are allocated for this width x height */
    UInt32 maxWidth;
    UInt32 maxHeight;

    /**< Number of individual channels per outputs */
    UInt32 numChPerOutput;

    UInt64 captureStartTime;
} CaptureLink_InstObj;

typedef struct {

    UInt32 instId;
    UInt32 oddFieldSkipRatio;
    UInt32 frameSkipMask;
    Bool   skipOddFields;
    UInt32 skipOddFieldsThreshold;
    UInt32 prevFid;

    UInt32      numBlindArea;
    /**< number of valid blind area for each channel */

    Bool chBlindAreaConfigUpdate;
    /**<flag to indicate if blind area config is update or not */
    CaptureLink_BlindInfo blindAreaConfig;
    /**<blind area config for the channel */

    Utils_DmaFill2D blindArea[CAPTURE_LINK_MAX_BLIND_AREA_PER_CHNL];
    /**<blind erea info for the channel */

} CaptureLink_ChObj;

/* Capture link information */
typedef struct {
    /* Capture link task */
    Utils_TskHndl tsk;

    /* Capture link create arguments */
    CaptureLink_CreateParams createArgs;

    /* Capture link output queues */
    Utils_BufHndl bufQue[CAPTURE_LINK_MAX_OUT_QUE];

    /* Global capture driver handle */
    FVID2_Handle fvidHandleVipAll;

    /* Capture driver instance specific information */
    CaptureLink_InstObj instObj[VPS_CAPT_INST_MAX];

    /* Capture link info that is returned when queried by next link */
    System_LinkInfo info;
  
    /* capture link run-time statistics for debug */
    UInt32 captureDequeuedFrameCount;
    UInt32 captureQueuedFrameCount;
    UInt32 cbCount;
    UInt32 cbCountServicedCount;
    UInt32 captureFrameCount[CAPTURE_LINK_MAX_OUT_QUE]
                            [CAPTURE_LINK_MAX_CH_PER_OUT_QUE];

    UInt32 startTime;
    UInt32 prevTime;
    UInt32 prevFrameCount;
    UInt32 exeTime;

    UInt32 totalCpuLoad;
    Uint32 cpuLoadCount;

    UInt32 prevVideoDetectCheckTime;

    UInt8 *tmpBufAddr;

    Bool enableCheckOverflowDetect;

    UInt32 resetCount;
    UInt32 resetTotalTime;
    UInt32 prevResetTime;

    Bool isPalMode;

    Int32 brightness;
    Int32 contrast;
    Int32 saturation;
    Int32 hue;
    UInt32 memUsed[UTILS_MEM_MAXHEAPS];

    CaptureLink_ExtraFrameObj extraFrameObj;

    Utils_DmaChObj dmaObj;
    /**<blind erea info for the channel */
    UInt32 chToVipInstMap[CAPTURE_LINK_MAX_OUT_QUE * CAPTURE_LINK_MAX_CH_PER_OUT_QUE];
    CaptureLink_ChObj chObj[CAPTURE_LINK_MAX_OUT_QUE][CAPTURE_LINK_MAX_CH_PER_OUT_QUE];

} CaptureLink_Obj;

extern CaptureLink_Obj gCaptureLink_obj;

Int32 CaptureLink_getInfo(Utils_TskHndl * pTsk, System_LinkInfo * info);
Int32 CaptureLink_getFullFrames(Utils_TskHndl * pTsk, UInt16 queId,
                                FVID2_FrameList * pFrameList);
Int32 CaptureLink_putEmptyFrames(Utils_TskHndl * pTsk, UInt16 queId,
                                 FVID2_FrameList * pFrameList);

Int32 CaptureLink_drvCreate(CaptureLink_Obj * pObj,
                            CaptureLink_CreateParams * pPrm);
Int32 CaptureLink_drvConfigureVideoDecoder(CaptureLink_Obj * pObj, UInt32 timeout);
Int32 CaptureLink_drvStart(CaptureLink_Obj * pObj);
Int32 CaptureLink_drvProcessData(CaptureLink_Obj * pObj);
Int32 CaptureLink_drvStop(CaptureLink_Obj * pObj);
Int32 CaptureLink_drvDelete(CaptureLink_Obj * pObj);
Int32 CaptureLink_drvAllocAndQueueFrames(CaptureLink_Obj * pObj,
                                         UInt32 instId);
Int32 CaptureLink_drvFreeFrames(CaptureLink_Obj * pObj,
                                UInt32 instId);

UInt32 CaptureLink_drvIsDataFormatTiled(Vps_CaptCreateParams * createArgs,
                                        UInt16 streamId);
Int32 CaptureLink_drvInitCreateArgs(Vps_CaptCreateParams * createArgs);

Int32 CaptureLink_drvPutEmptyFrames(CaptureLink_Obj * pObj,
                                    FVID2_FrameList * pFrameList);
Int32 CaptureLink_getCpuLoad();

Int32 CaptureLink_drvPrintStatus(CaptureLink_Obj * pObj);
Int32 CaptureLink_drvOverflowDetectAndReset(CaptureLink_Obj * pObj,
                                            Bool doForceReset);

Int32 CaptureLink_drvDetectVideoStandard(CaptureLink_Obj * pObj, Int32 instId);

Int32 CaptureLink_drvSetColor(CaptureLink_Obj * pObj, Int32 contrast,
                              Int32 brightness, Int32 saturation, Int32 hue, Int32 ChId);
Void CaptureLink_drvSetFrameCropBufPtr(CaptureLink_Obj * pObj,
                                       FVID2_FrameList * pFrameList);
Void CaptureLink_drvReSetFrameCropBufPtr(CaptureLink_Obj * pObj,
                                       FVID2_FrameList * pFrameList);
Int32 CaptureLink_drvDetectVideo(CaptureLink_Obj * pObj, Bool notifyHostOnChange, Bool printInfo);

Int32 CaptureLink_drvGetVideoStatus(CaptureLink_Obj * pObj, VCAP_VIDEO_SOURCE_STATUS_S *pStatus);

Int32 CaptureLink_drvSetAudioParams(CaptureLink_Obj * pObj, Capture_AudioModeParams * ptr);

Int32 CaptureLink_printBufferStatus(CaptureLink_Obj * pObj);

Int32 CaptureLink_drvAllocAndQueueExtraFrames(CaptureLink_Obj * pObj, UInt32 instId, UInt32 streamId, UInt32 chId);
Int32 CaptureLink_drvFreeExtraFrames(CaptureLink_Obj * pObj);

Int32 CaptureLink_drvSetExtraFramesChId(CaptureLink_Obj * pObj, CaptureLink_ExtraFramesChId *pPrm);

Int32 CapterLink_drvBlindAreaConfigure(CaptureLink_Obj * pObj,
                                          CaptureLink_BlindInfo *blindAreaInfo);

Int32 CaptureLink_drvSkipOddFields(
            CaptureLink_Obj * pObj,
            CaptureLink_SkipOddFields *pPrm
            );

#endif

