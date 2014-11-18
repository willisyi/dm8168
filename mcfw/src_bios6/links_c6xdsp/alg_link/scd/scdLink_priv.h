/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#ifndef _ALG_LINK_SCD_PRIV_H_
#define _ALG_LINK_SCD_PRIV_H_

#include <mcfw/src_bios6/utils/utils.h>
#include <mcfw/src_bios6/utils/utils_dma.h>
#include <mcfw/src_bios6/links_c6xdsp/system/system_priv_c6xdsp.h>
#include <mcfw/interfaces/link_api/algLink.h>
#include <time.h>
#include <mcfw/src_bios6/alg/va/DMVAL.h>
#include "scd.h"


#define ALG_LINK_SCD_OBJ_MAX                        (SYSTEM_LINK_ID_SCD_COUNT)

#define ALG_LINK_SCD_MAX_OUT_FRAMES_PER_CH          (3)

#define ALG_LINK_SCD_MAX_OUT_FRAMES                 (ALG_LINK_SCD_MAX_CH*ALG_LINK_SCD_MAX_OUT_FRAMES_PER_CH)

#define MOTION_DETECTION_SENSITIVITY(x,y)           ((x * y)/5) // 20%
#define MOTION_DETECTION_SENSITIVITY_STEP           (10)        // 10%
#define NUM_BLOCK_MOTION_DETECTION_THRESOLD(x)      (x >> 1)    // 50%

#define ALG_LINK_TAMPER_ALERT_MAX_THRESOLD                  (32)

#define ALG_LINK_TAMPER_RESET_COUNT_SCALER_CONSTANT         (20)

#define ALG_VA_BUFFER_ALIGNMENT                             (64)

typedef enum AlgLink_processTaskState {
    ALG_LINK_PROCESS_TASK_STATE_STOPPED = 0,
    /*<< Alg link state: Alg Link Process task stopped, Ready to delete. */

    ALG_LINK_PROCESS_TASK_STATE_STOPPING,
    /*<< Alg link state: Alg Link Process Task  stop command issued.  */

    ALG_LINK_PROCESS_TASK_STATE_RUNNING
    /*<< Alg link state: Alg Link Process Task created and active */

}AlgLink_processTaskState;
/**
    \brief SCD link Process Object.
*/
typedef struct AlgLink_ScdProcessObj 
{
    FVID2_Frame          processFrames[ALG_LINK_SCD_MAX_OUT_FRAMES_PER_CH];

    System_FrameInfo     processFrameInfo[ALG_LINK_SCD_MAX_OUT_FRAMES_PER_CH];
    /**< Array to keep Frame info of frames to be processed. */
    
    UInt32               processFrameSize;
    /**< Frame size to be processed */
    
    Utils_QueHandle      freeQ;
    /**< Queue to maintain list of available/free frame memory. */
    
    FVID2_Frame         *freeQMem[ALG_LINK_SCD_MAX_OUT_FRAMES_PER_CH];

} AlgLink_ScdProcessObj;

/**
    \brief SCD Channel parameters used to SET BEFORE CALLING PROCESS
*/
typedef struct  AlgLink_ScdchPrm
{
    U32 chId;
    /**< Unique video channel identifier, e.g. channel no. */

 	SCD_Mode mode;
    /**< Enable/Disable scd running on algo*/

    U32 width;
    /**< Set the width (in pixels) of video frame that scd will process */

    U32 height;
    /**< Set the height (in pixels) of video frame that scd will process */

    U32 stride;
    /**< Set the video frame pitch/stride of the images in the video buffer*/

    PTR curFrame;
    /** Luma pointer to current frame */

    UInt32 chBlkConfigUpdate;
    /**<Flag to check if block config update is required. */


    SCD_Sensitivity frmSensitivity;
    /**< internal threshold for scd change tolerance; globally applied to entire frame */

    Int32 thresold2WaitB4FrmAlert;
    /**< Set the thresold to wait before signaling a frame-level scene change event.
         It can take values from 0-32. */

    Int32 thresold2WaitAfterFrmAlert;
	/**< Set thresold to wait for pre-tamper conditions to return following a 
         tamper event. It can take values from 0-32.  */

    Int32 numSecs2Wait2MarkStableFrame;
	/**< Set to 1 to n for the maximum number of seconds to wait for pre-tamper
	     conditions to return following a tamper event */

    U32 frmIgnoreLightsON;
    /**< Set to 1 to ignore sudden, significantly brighter avg. scene luminance */

    U32 frmIgnoreLightsOFF;
    /**< Set to 1 to ignore sudden, significantly darker avg. scene luminance */

    U32 frmEdgeThreshold;
    /**< Set minimum number of edge pixels required to indicate non-tamper event */
    
    SCD_blkChngConfig blkConfig[ALG_LINK_SCD_MAX_BLOCKS_IN_FRAME];
    /**< Linear array of configuration of frame blocks that 
         scd will monitor for motion detection (configuration) */

    DMVALimgType imgType;
    /**< DMVL Image type. Currently only two types of images are supported 
      *    DMVAL_IMG_LUMA						= 0Only LUMA 
      *    DMVAL_IMG_YUV420_PLANARINTERLEAVED	=  YUV420, LUMA and Chroma Separate
      */
    
    
    DMVALdetectMode detectMode;
    /**< DMVL Detetcion mmode. Only Tamper detection is supported. */

    DMVALsensitivity sensitiveness;
    /**< DMVL Tamper sensitivoty */

    U32 resetCounter;

} AlgLink_ScdchPrm;

/**
   \brief SCD Link Channel Object. Keeps Run time status of respective channel *
*/

typedef struct AlgLink_ScdChObj{
    U32 chId;
    /**< Unique video channel identifier, e.g. channel no. */

    UInt32 inFrameRecvCount;
    /**< input frame recevied from previous link */

    UInt32 inFrameUserSkipCount;
    /**< input frame rejected due mismatch in FID */

    UInt32 inFrameProcessCount;
    /**< input frames actually processed */
    
    UInt32 inFrameProcessSkipCount;
    /**< input frames skipped in process task */

    UInt32 inFrameProcessTime;
    /**< Frame Process time taken by SCD algorithm */

    AlgLink_ScdOutput scdChStat;
    /**< Current Tamper status of channel */

    Utils_frameSkipContext frameSkipCtx;
    /**< Data structure for frame skip to achieve expected output frame rate */    

    UInt32  sizeA;
    /**< Number of bytes of permanent memory required by the video analytics
      *  application. 
      */

    UInt32  sizeB;
    /**< Number of bytes required by the video analytics application to generate
      * the output.
      */

    Void    *ptrA;
    /**<  Pointer to Permanent Memory block */

    Void    *ptrB;
    /**<  Pointer to Memory required for output */

    HPTimeStamp frameTS;
    /**< Frame time stamp required by alorithm. Computed at link. */

    AlgLink_ScdProcessObj  scdProcessObj;
    /**< SCD Link process object. Keeps record of Frames ready to process and 
     *   availaible empty frames in FreeQ.
     */

	DMVALout    output;
    /**< DMVL Tamper result. */

} AlgLink_ScdChObj;

/**
   \brief SCD Link Output Object. List of bit buffers, Used to store block 
          metadata, which is then passed on to the next link. 
*/
typedef struct AlgLink_ScdOutObj {
    Utils_BitBufHndl bufOutQue;
    UInt32           numAllocPools;
    Bitstream_Buf    outBufs[ALG_LINK_SCD_MAX_OUT_FRAMES];
    UInt32           outNumBufs[UTILS_BITBUF_MAX_ALLOC_POOLS];
    UInt32           buf_size[UTILS_BITBUF_MAX_ALLOC_POOLS];
    UInt32           ch2poolMap[ALG_LINK_SCD_MAX_CH];
} AlgLink_ScdOutObj;

/**
   \brief SCD Link Object. Keeps Run time status of whole link, Alg handles, 
          Channel Object etc. *
*/
typedef struct AlgLink_ScdObj {

    Utils_TskHndl      * pTsk;

    Utils_TskHndl        processTsk;

    AlgLink_processTaskState processTskState;

    char                 processTskName[32];

    System_LinkOutQueParams outQueParams[ALG_LINK_MAX_OUT_QUE];
    /**< Output Queue params, Copy of ALG link in SCD object. */

    Utils_QueHandle      processQ;

    FVID2_Frame        * processQMem[ALG_LINK_SCD_MAX_OUT_FRAMES_PER_CH*ALG_LINK_SCD_MAX_CH];

    Utils_DmaChObj       dmaCh;
    /**< DMA channel object */

    System_LinkQueInfo * inQueInfo;

    AlgLink_ScdchPrm     chParams[ALG_LINK_SCD_MAX_CH];
    /**< Array of Channe params of SCD algorithm */

    AlgLink_ScdCreateParams createArgs;
    /**<  SCD algorithm create time params */

    AlgLink_ScdChObj   chObj[ALG_LINK_SCD_MAX_CH];
    /**< Array of Channe Ojects of SCD algorithm */

    AlgLink_ScdOutObj  outObj[ALG_LINK_MAX_OUT_QUE];

    void *algHndl;
    /**< handle to SCD algorithm */
    
    DMVALhandle dmvalHndl[ALG_LINK_SCD_MAX_CH];
    /**< Array of handles of the DMVL algorithm */

    UInt32 statsStartTime;

    UInt32 totalFrameCount;

} AlgLink_ScdObj;


Int32 AlgLink_ScdalgCreate(AlgLink_ScdObj * pObj);
Int32 AlgLink_ScdalgProcessData(AlgLink_ScdObj * pObj);
Int32 AlgLink_ScdalgDelete(AlgLink_ScdObj * pObj);

Int32 AlgLink_ScdAlgCopyFrame(AlgLink_ScdObj * pObj, FVID2_Frame *pFrame);

Int32 AlgLink_ScdAlgSubmitFrames(AlgLink_ScdObj * pObj,
                                FVID2_FrameList *pFrameList
                                );

Int32 AlgLink_ScdalgSetChblkUpdate(AlgLink_ScdObj * pObj,
                            AlgLink_ScdChblkUpdate * params);
                                                          
Int32 AlgLink_ScdalgSetChScdSensitivity(AlgLink_ScdObj * pObj,
                            AlgLink_ScdChParams * params);
Int32 AlgLink_ScdalgSetChScdMode(AlgLink_ScdObj * pObj,
                            AlgLink_ScdChParams * params);
Int32 AlgLink_ScdalgSetChScdIgnoreLightsOn(AlgLink_ScdObj * pObj,
                            AlgLink_ScdChParams * params);
Int32 AlgLink_ScdalgSetChScdIgnoreLightsOff(AlgLink_ScdObj * pObj,
                            AlgLink_ScdChParams * params);

Int32 AlgLink_ScdalgGetChResetChannel(AlgLink_ScdObj * pObj,
                            AlgLink_ScdChCtrl * params);

Int32 AlgLink_ScdresetStatistics(AlgLink_ScdObj *pObj);
Int32 AlgLink_ScdprintStatistics (AlgLink_ScdObj *pObj, Bool resetAfterPrint);
Int32 AlgLink_scdAlgGetAllChFrameStatus(AlgLink_ScdObj * pObj, AlgLink_ScdAllChFrameStatus *pPrm);

Int32 AlgLink_scdAlgProcessTskInit(AlgLink_ScdObj *pObj, UInt32 objId);
Int32 AlgLink_scdAlgProcessTskDeInit(AlgLink_ScdObj *pObj);
Int32 AlgLink_scdAlgProcessTskSendCmd(AlgLink_ScdObj *pObj, UInt32 cmd);

#endif
