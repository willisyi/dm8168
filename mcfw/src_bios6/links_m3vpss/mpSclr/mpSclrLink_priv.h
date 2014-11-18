/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2012 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#ifndef _MP_SCLR_LINK_PRIV_H_
#define _MP_SCLR_LINK_PRIV_H_

#include <mcfw/src_bios6/utils/utils_dma.h>
#include <mcfw/src_bios6/links_m3vpss/system/system_priv_m3vpss.h>
#include <mcfw/interfaces/link_api/mpSclrLink.h>

#define debugMpSclrPrintFrame(x, y)
/**< Debug aid prints */
#define debugMpSclrPrintStr(x)
/**< Debug aid prints */
#define debugMpSclrPrintReqObj(x, y)
/**< Debug aid prints */

#define MP_SCLR_LINK_OBJ_MAX                (SYSTEM_LINK_ID_MP_SCLR_COUNT)
/**< Number of instances of MP SCLR */
#define MP_SCLR_LINK_MAX_FRAMES_PER_CH      (SYSTEM_LINK_FRAMES_PER_CH)  
/**< Maximum number of frames that can queued for scaling for a given channel */
#define MP_SCLR_LINK_MAX_OUTPUT_FREE_FRAMES (MP_SCLR_LINK_MAX_FRAMES_PER_CH+1)
/**< Maximum number of free frame buffers that an instance can have */
#define MP_SCLR_LINK_MP_FRAME               (0x80000000)
/**< Value used to tag used to identify Mega Pixel frame */
#define MP_SCLR_LINK_MIN_WIDTH_MP_SCALING   (1920)
/**< Minimum width required to enable MP Scaling for incomming video streams */
#define MP_SCLR_LINK_MIN_HEIGHT_MP_SCALING  (1080)
/**< Minimum Height required to enable MP Scaling for incomming video streams */
#define MP_SCLR_LINK_OUT_MAX_WIDTH          (1920)
/**< Maximum number of pixels that a scalar supports */
#define MP_SCLR_LINK_OUT_MAX_HEIGHT         (1080)
/**< Maximum number of lines that a scalar supports */
#define MP_SCLR_LINK_OUT_DATATYPE           (SYSTEM_DF_YUV422I_YUYV)
/**< The default output data format is YUV422 I, this should not be changed */
#define MP_SCLR_INVALID_U32_VALUE           (0xFFFFFFFF)
/**< Values used to mark an unsigned int 32 value as invalid */
#define MP_SCLR_MAX_REQESTS_PENDING_WITH_DVR (MP_SCLR_LINK_MAX_CH * 4)
/**< Maximum nuber of request that could pend with scalar driver for scaling */

#define MP_SCLR_LINK_CMD_SCALING_DONE       (MP_SCLR_LINK_CMD_MAX + 1)
/**< MP_SCLR Internal command, used when the scaling is completed */

#define MP_SCLR_LINK_INPUT_FRAME_MAX_WIDTH  (16)
/**< The scalar internally splits MP frame into multiple sub-windows, this macro
        control the maximum number windows on the horizontal axis, pixel count
        i.e. max width = max pixel count / 1920 */
#define MP_SCLR_LINK_INPUT_FRAME_MAX_HEIGHT (16)
/**< The scalar internally splits MP frame into multiple sub-windows, this macro
        control the maximum number windows on the vertical axis, line count
        i.e. max height = max line count / 1080 */

/* Default input formats - Should not change these values */
#define MP_SCLR_LINK_DEF_IN_WIDTH           (2048)
/**< Default input width */
#define MP_SCLR_LINK_DEF_IN_HEIGHT          (2048)
/**< Default input height */
#define MP_SCLR_LINK_DEF_IN_PITCH           (4224)
/**< Default pitch */
#define MP_SCLR_LINK_DEF_IN_DATAFORMAT      (SYSTEM_DF_YUV420SP_UV)
/**< Default data format */

typedef struct {

    FVID2_FrameList     inFrameList;
    FVID2_FrameList     outFrameList;
    FVID2_ProcessList   processList;

}MpSclrLink_ReqObj;

typedef struct {

    UInt32          inChanNum;
    /**< This channel instance associated with input channel number */

    Vps_ScConfig    scCfg;
    /**< Per Channel Scalar configuration */
    
    Utils_BufHndl   inQue;
    /**< Queue to hold MP scaling requests */
    FVID2_Format    inFmt;
    /**< Current expected input stream */
    FVID2_Format    outFmt;
    /**< Current output format for a given channel, also SC output format */

#ifndef NO_MP_SCALING
    Vps_SubFrameParams subWinParams;
    /**< Configurations to split MP frame into sub-windows */
    Vps_SubFrameHorzSubFrmInfo splitWinParms;
    /**< For the current inFmt & outFmt, this will hold the sub window details,
            for this channel. */
    FVID2_SubFrameInfo ipSelSubWin;
    /**< Used to select the sub-window, that would be scaled */
#endif /* NO_MP_SCALING */
    
    UInt32          isFrameBufAlloced;
    /**< Flag to indicate if output buffers allocated for this instace of MP 
       Scalar */
    FVID2_Frame     chFrames[MP_SCLR_LINK_MAX_OUTPUT_FREE_FRAMES];
    /* Pool of frames - for a channel */
    System_FrameInfo chFrameInfo[MP_SCLR_LINK_MAX_OUTPUT_FREE_FRAMES];
    /* System Frame info - would primarily be interstead in isMpFrame */
    FVID2_Format    chFrameFormat[MP_SCLR_LINK_MAX_OUTPUT_FREE_FRAMES];
    /* Frames Format, when output resolution requires to be changed */
    FVID2_Frame     intrFrame;
    /* Intermediate frame used to hold the horizontal slice */

    /* Stats */
    UInt32 inFrameProcessCount;
    /**< Input frames actually processed */
    UInt32 outFrameSkipCount;
    /**< Frames dropped due to output buffer not being available */
    UInt32 outFrameCount;
    /**< Number of frames actually output */
    UInt32 inReqsNotProcessed;
    /**< Input MP frames not processed due to lack of request objects, 
            o/p frames, etc... */

    UInt32 statsStartTime;
    /**< Stats log start time, i.e. when channel is created */
    UInt32 minLatencyDrv;
    /**< Minimum time take by the driver to scale a an MP Frame */
    UInt32 maxLatencyDrv;
    /**< Maximum time take by the driver to scale a an MP Frame */

    UInt32 minLatency;
    /**< Minimum time to process a MP frame, latency in msecs */
    UInt32 maxLatency;
    /**< Maximum time to process a MP frame, latency in msecs */

}MpSclrLink_ChObj;


typedef struct {

    UInt32          linkId;
    Utils_TskHndl   mainTskHndl;
    char            mainTskName[32];

    Utils_TskHndl   mpScTskHndl;
    char            mpScTskName[32];

    MpSclrLink_CreateParams createArgs;
    System_LinkInfo     inTskInfo;
    System_LinkQueInfo  inQueInfo;
    System_LinkInfo     info;

    /* Links input output frame Q specifics */
    Utils_BufHndl       linkBufQ; 
    /**< Output FULL frames and input EMPTY frames. At init allocated
         frames are put into empty Q*/

    MpSclrLink_ChObj    chObj[MP_SCLR_LINK_MAX_CH];
    Int32               ipChaNumToChanMap[MP_SCLR_LINK_MAX_CH];
    /* Mapping between input video stream channel number to MP Scalar channel */

    Utils_QueHandle     reqQ;
    /**< Q to hold list of process objects */
    MpSclrLink_ReqObj   *memReqQ[MP_SCLR_MAX_REQESTS_PENDING_WITH_DVR];
    /**< Memory required for Q implementation */
    MpSclrLink_ReqObj   reqObjs[MP_SCLR_MAX_REQESTS_PENDING_WITH_DVR];
    /**< Pool of process list, inlist and outlist */
    Utils_QueHandle     compReqQ;
    /**< Q to hold Completed process objects */
    MpSclrLink_ReqObj   *memCompReqQ[MP_SCLR_MAX_REQESTS_PENDING_WITH_DVR];
    /**< Memory required for Q implementation */
    
    Semaphore_Handle    scalingDone;
    /* Semaphore to signal completion of Scaling */
    FVID2_Handle        fvidHandle;
    /* Error list, required by driver in case of errors */
    FVID2_ProcessList   errProcessList;

    /* Primarily used to give back frames of previous link */
    FVID2_FrameList         freeFrameList;

    Vps_M2mScChParams       drvChArgs[MP_SCLR_LINK_MAX_CH];
    Vps_M2mScCreateParams   scCreateParams;
    Vps_M2mScCreateStatus   scCreateStatus;
    
    Utils_DmaChObj          dmaCh;

    /* Maximum width supported by the scalar */
    UInt32 maxWidthSupported;
    
    /* Tracks the number of requests pending */
    UInt32 reqQueCount;
    
    UInt32 inFrameFwdCount;
    /**< Input frames which is not MP frame, passed on to next link */
    UInt32 inFrameRecvCount;
    /**< Input frames recevied count, from previous link */
    UInt32 inFrameNotProcessed;
    /**< Input MP frames not processed due to lack of channels */
    UInt32 inReqsNotProcReqObj;
    /**< Input MP frames not processed due to lack of request objects */

} MpSclrLink_Obj;

/* Functions implement in mpSclrLink_drv */
Int32 
    MpSclrLink_drvCreate(MpSclrLink_Obj *pObj, MpSclrLink_CreateParams *pPrms);
Int32 MpSclrLink_drvDelete(MpSclrLink_Obj *pObj);

Int32 MpSclrLink_drvMpProcessAllCh(MpSclrLink_Obj *pObj);
Int32 MpSclrLink_drvProcessMpFrames(MpSclrLink_Obj *pObj);


Int32 MpSclrLink_drvGetChOutputRes(MpSclrLink_Obj *pObj, 
                                    MpSclrLink_chDynamicSetOutRes *params);
Int32 MpSclrLink_drvSetChOutputRes(MpSclrLink_Obj *pObj, 
                                    MpSclrLink_chDynamicSetOutRes *params);
Int32 MpSclrLink_drvPrintStatistics(MpSclrLink_Obj *pObj, UInt32 flag);
Int32 MpSclrLink_drvMpPostProcessedFrames(MpSclrLink_Obj *pObj);

#endif /* _MP_SCLR_LINK_PRIV_H_ */


