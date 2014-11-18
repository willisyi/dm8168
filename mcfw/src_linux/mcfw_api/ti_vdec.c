/*==============================================================================
 * @file:       ti_vdec.c
 *
 * @brief:      Video capture mcfw function definition.
 *
 * @vers:       0.5.0.0 2011-06
 *
 *==============================================================================
 *
 * Copyright (C) 2011 Texas Instruments Incorporated - http://www.ti.com/
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <osa.h>
#include <osa_debug.h>
#include "ti_vsys_priv.h"
#include "ti_vdec_priv.h"
#include "ti_vsys.h"
#include "ti_vdec.h"

#include <mcfw/interfaces/link_api/decLink.h>
#include <mcfw/interfaces/link_api/ipcLink.h>

/* =============================================================================
 * Defines
 * =============================================================================
 */
#define VDEC_IPCBITS_DEFAULT_WIDTH                   (720)
#define VDEC_IPCBITS_DEFAULT_HEIGHT                  (576)

#define VDEC_IPCBITS_GET_BITBUF_SIZE(width,height)   ((width) * (height)/2)


/* =============================================================================
 * Globals
 * =============================================================================
 */

VDEC_MODULE_CONTEXT_S gVdecModuleContext;

/* =============================================================================
 * Externs
 * =============================================================================
 */


/* =============================================================================
 * Vdec module APIs
 * =============================================================================
 */
/**
 * \brief:
 *      Initialize parameters to be passed to init
 * \input:
 *      NA
 * \output:
 *      NA
 * \return
*       ERROR_NOERROR       --  while success
*       ERROR_CODE          --  refer for err defination
*/
Void Vdec_params_init(VDEC_PARAMS_S * pContext)
{
    UInt16 i;

    memset(pContext, 0, sizeof(VDEC_PARAMS_S));

    pContext->forceUseDecChannelParams = FALSE;

    for (i=0; i < VDEC_CHN_MAX; i++) {

        pContext->decChannelParams[i].dynamicParam.frameRate = 30;
        pContext->decChannelParams[i].dynamicParam.targetBitRate = 2 * 1000 * 1000;
        pContext->decChannelParams[i].maxVideoWidth = VDEC_IPCBITS_DEFAULT_WIDTH;
        pContext->decChannelParams[i].maxVideoHeight = VDEC_IPCBITS_DEFAULT_HEIGHT;
    }
}

/**
 * \brief:
 *      Initialize Vdec instance
 * \input:
 *      NA
 * \output:
 *      NA
 * \return
*       ERROR_NOERROR       --  while success
*       ERROR_CODE          --  refer for err defination
*/
Int32 Vdec_init(VDEC_PARAMS_S * pContext)
{
    Int i;

    gVdecModuleContext.decId = SYSTEM_LINK_ID_INVALID;
    gVdecModuleContext.ipcBitsInRTOSId = SYSTEM_LINK_ID_INVALID;
    gVdecModuleContext.ipcBitsOutHLOSId = SYSTEM_LINK_ID_INVALID;
    gVdecModuleContext.ipcM3OutId = SYSTEM_LINK_ID_INVALID;
    gVdecModuleContext.ipcM3InId = SYSTEM_LINK_ID_INVALID;
    for (i = 0; i < VDIS_DEV_MAX ; i++)
    {
        Int j;

        for (j = 0; j < VDEC_CHN_MAX; j++)
        {
            /* Map channels to invalid id by default */
            gVdecModuleContext.vdisChIdMap[i][j] = -1;
        }
    }

    if(pContext==NULL)
    {
        Vdec_params_init(&gVdecModuleContext.vdecConfig);
    }
    else
    {
        memcpy(&gVdecModuleContext.vdecConfig, pContext, sizeof(VDEC_PARAMS_S));
    }

    return 0;
}

/**
 * \brief:
 *      Finalize Vdec instance
 * \input:
 *      NA
 * \output:
 *      NA
 * \return
*       ERROR_NOERROR       --  while success
*       ERROR_CODE          --  refer for err defination
*/
Void Vdec_exit()
{
}

/**
 * \brief:
 *      Start Vdec instance for capturing and proccessing
 * \input:
 *      NA
 * \output:
 *      NA
 * \return
*       ERROR_NOERROR       --  while success
*       ERROR_CODE          --  refer for err defination
*/
Int32 Vdec_start()
{
    if(gVdecModuleContext.ipcM3InId!=SYSTEM_LINK_ID_INVALID)
        System_linkStart(gVdecModuleContext.ipcM3InId);

    if(gVdecModuleContext.ipcM3OutId!=SYSTEM_LINK_ID_INVALID)
        System_linkStart(gVdecModuleContext.ipcM3OutId);

    if(gVdecModuleContext.decId!=SYSTEM_LINK_ID_INVALID)
        System_linkStart(gVdecModuleContext.decId);

    if(gVdecModuleContext.ipcBitsInRTOSId!=SYSTEM_LINK_ID_INVALID)
        System_linkStart(gVdecModuleContext.ipcBitsInRTOSId);

    if(gVdecModuleContext.ipcBitsOutHLOSId!=SYSTEM_LINK_ID_INVALID)
        System_linkStart(gVdecModuleContext.ipcBitsOutHLOSId);

    return 0;
}

/**
 * \brief:
 *      Stop Vdec instance
 * \input:
 *      NA
 * \output:
 *      NA
 * \return
*       ERROR_NOERROR       --  while success
*       ERROR_CODE          --  refer for err defination
*/
Int32 Vdec_stop()
{
    if(gVdecModuleContext.ipcBitsOutHLOSId!=SYSTEM_LINK_ID_INVALID)
        System_linkStop(gVdecModuleContext.ipcBitsOutHLOSId);

    if(gVdecModuleContext.ipcBitsInRTOSId!=SYSTEM_LINK_ID_INVALID)
        System_linkStop(gVdecModuleContext.ipcBitsInRTOSId);

    if(gVdecModuleContext.decId!=SYSTEM_LINK_ID_INVALID)
        System_linkStop(gVdecModuleContext.decId);

    if(gVdecModuleContext.ipcM3OutId!=SYSTEM_LINK_ID_INVALID)
        System_linkStop(gVdecModuleContext.ipcM3OutId);

    if(gVdecModuleContext.ipcM3InId!=SYSTEM_LINK_ID_INVALID)
        System_linkStop(gVdecModuleContext.ipcM3InId);

    return 0;
}

Int32 Vdec_delete()
{
    if(gVdecModuleContext.ipcM3InId!=SYSTEM_LINK_ID_INVALID)
        System_linkDelete(gVdecModuleContext.ipcM3InId);

    if(gVdecModuleContext.ipcM3OutId!=SYSTEM_LINK_ID_INVALID)
        System_linkDelete(gVdecModuleContext.ipcM3OutId);

    if(gVdecModuleContext.decId!=SYSTEM_LINK_ID_INVALID)
        System_linkDelete(gVdecModuleContext.decId);

    if(gVdecModuleContext.ipcBitsInRTOSId!=SYSTEM_LINK_ID_INVALID)
        System_linkDelete(gVdecModuleContext.ipcBitsInRTOSId);

    if(gVdecModuleContext.ipcBitsOutHLOSId!=SYSTEM_LINK_ID_INVALID)
        System_linkDelete(gVdecModuleContext.ipcBitsOutHLOSId);

    return 0;
}

static Void Vdec_copyBitBufInfoLink2McFw(VCODEC_BITSBUF_S *dstBuf,
                                         Bitstream_Buf    *srcBuf)
{
    dstBuf->reserved              = (UInt32)srcBuf;
    dstBuf->bufVirtAddr           = srcBuf->addr;
    dstBuf->bufSize               = srcBuf->bufSize;
    dstBuf->chnId                 = srcBuf->channelNum;
    dstBuf->codecType             = srcBuf->codingType;
    dstBuf->filledBufSize         = srcBuf->fillLength;
    dstBuf->timestamp             = srcBuf->timeStamp;
    dstBuf->upperTimeStamp        = srcBuf->upperTimeStamp;
    dstBuf->lowerTimeStamp        = srcBuf->lowerTimeStamp;
    dstBuf->bottomFieldBitBufSize = srcBuf->bottomFieldBitBufSize;
    dstBuf->inputFileChanged      = srcBuf->inputFileChanged;

    if (srcBuf->isKeyFrame)
        dstBuf->frameType      = VCODEC_FRAME_TYPE_I_FRAME;
    else
        dstBuf->frameType      = VCODEC_FRAME_TYPE_P_FRAME;
    dstBuf->bufPhysAddr    = (Void *)srcBuf->phyAddr;
    dstBuf->frameWidth     = srcBuf->frameWidth;
    dstBuf->frameHeight    = srcBuf->frameHeight;
    dstBuf->doNotDisplay   = FALSE;
    /*TODO the following members need to be added to bitstream buf */
    dstBuf->fieldId        = 0;
    dstBuf->strmId         = 0;
    dstBuf->seqId          = srcBuf->seqId;

}


static Void Vdec_copyBitBufInfoMcFw2Link(Bitstream_Buf    *dstBuf,
                                         VCODEC_BITSBUF_S *srcBuf)
{

    dstBuf->channelNum = srcBuf->chnId;
    dstBuf->fillLength = srcBuf->filledBufSize;
    dstBuf->bottomFieldBitBufSize = srcBuf->bottomFieldBitBufSize;
    dstBuf->inputFileChanged = srcBuf->inputFileChanged;
    dstBuf->codingType = srcBuf->codecType;
    dstBuf->timeStamp  = srcBuf->timestamp;
    dstBuf->upperTimeStamp = srcBuf->upperTimeStamp;
    dstBuf->lowerTimeStamp = srcBuf->lowerTimeStamp;
    if (srcBuf->frameType == VCODEC_FRAME_TYPE_I_FRAME)
    {
        dstBuf->isKeyFrame = TRUE;
    }
    else
    {
        dstBuf->isKeyFrame = FALSE;
    }
    dstBuf->doNotDisplay   = srcBuf->doNotDisplay;
    dstBuf->seqId          = srcBuf->seqId;
}

/**
    \brief Give buffers for decode to McFW

    Buffers that need to be decode are given to the McFW.

    \param pBitsBufList [IN]   List of Bistream Buffers

    \return SUCCESS or FAIL
*/
Int32 Vdec_requestBitstreamBuffer(VDEC_BUF_REQUEST_S * bufReq, VCODEC_BITSBUF_LIST_S *pBitsBufList, UInt32 timeout)
{

    IpcBitsOutLinkHLOS_BitstreamBufReqInfo ipcReqInfo;
    Bitstream_BufList ipcBufList;
    Bitstream_Buf *pInBuf;
    VCODEC_BITSBUF_S *pOutBuf;
    UInt32 i;

    if (bufReq->numBufs > VIDBITSTREAM_MAX_BITSTREAM_BUFS)
    {
        OSA_printf("MCFW_VDEC: Maximum number of requested bufs greater "
                   "than max not supported. Requested Num:%d,Max Supported:%d",
                    bufReq->numBufs, VIDBITSTREAM_MAX_BITSTREAM_BUFS);
        bufReq->numBufs = VIDBITSTREAM_MAX_BITSTREAM_BUFS;
    }
    ipcBufList.numBufs = 0;
    ipcReqInfo.numBufs = bufReq->numBufs;
    OSA_COMPILETIME_ASSERT(IPC_BITSOUTHLOS_BITBUFREQTYPE_CHID == VDEC_BUFREQTYPE_CHID);
    OSA_COMPILETIME_ASSERT(IPC_BITSOUTHLOS_BITBUFREQTYPE_BUFSIZE == VDEC_BUFREQTYPE_BUFSIZE);
    ipcReqInfo.reqType = (IpcBitsOutLinkHLOS_BitstreamBufReqType)bufReq->reqType;
    for (i = 0; i < VIDBITSTREAM_MAX_BITSTREAM_BUFS; i++)
    {
        if (IPC_BITSOUTHLOS_BITBUFREQTYPE_CHID == ipcReqInfo.reqType)
            ipcReqInfo.u[i].chNum      = bufReq->u[i].chNum;
        else
            ipcReqInfo.u[i].minBufSize = bufReq->u[i].minBufSize;
    }

    IpcBitsOutLink_getEmptyVideoBitStreamBufs(gVdecModuleContext.ipcBitsOutHLOSId,
                                              &ipcBufList,
                                              &ipcReqInfo);

    pBitsBufList->numBufs = ipcBufList.numBufs;
    for (i = 0; i < ipcBufList.numBufs; i++)
    {
        pOutBuf = &pBitsBufList->bitsBuf[i];
        pInBuf = ipcBufList.bufs[i];
        Vdec_copyBitBufInfoLink2McFw(pOutBuf, pInBuf);
    }
    return 0;
}


/**
    \brief Give buffers for decode to McFW

    Buffers that need to be decode are given to the McFW.

    \param pBitsBufList [IN]   List of Bistream Buffers

    \return SUCCESS or FAIL
*/
Int32 Vdec_putBitstreamBuffer(VCODEC_BITSBUF_LIST_S *pBitsBufList)
{
    Bitstream_BufList ipcBufList;
    Bitstream_Buf *pInBuf;
    VCODEC_BITSBUF_S *pOutBuf;
    UInt32 i;

    ipcBufList.numBufs = pBitsBufList->numBufs;
    for (i = 0; i < ipcBufList.numBufs; i++)
    {
        ipcBufList.bufs[i] =
            (Bitstream_Buf*)pBitsBufList->bitsBuf[i].reserved;
        pOutBuf = &pBitsBufList->bitsBuf[i];
        pInBuf = ipcBufList.bufs[i];
        Vdec_copyBitBufInfoMcFw2Link(pInBuf,pOutBuf);
    }

    IpcBitsOutLink_putFullVideoBitStreamBufs(gVdecModuleContext.ipcBitsOutHLOSId,
                                             &ipcBufList);
    return 0;
}


/* =============================================================================
 * Vdec device related APIs
 * =============================================================================
 */

/* =============================================================================
 * Vdec channel related APIs
 * =============================================================================
 */

/**
 * \brief:
 *      Enable the specific decoder channel
 * \input:
 *      vdecChnId       -- decoder channel id
 * \output:
 *      NA
 * \return
*       TI_MEDIA_SUCCESS    --  while success
*       ERROR_CODE          --  refer for err defination
*/
Int32 Vdec_enableChn(VDEC_CHN vdecChnId)
{
    DecLink_ChannelInfo params = {0};

    printf("\r\nEnable Channel: %d", vdecChnId);

    params.chId = vdecChnId;

    System_linkControl(gVdecModuleContext.decId, DEC_LINK_CMD_ENABLE_CHANNEL,
                       &params, sizeof(params), TRUE);

    return 0;

}

/**
 * \brief:
 *      Set playback control configuration to trickplay logicl
 * \input:
 *      vdecChnId       -- decoder channel id
 * \output:
 *      NA
 * \return
*       TI_MEDIA_SUCCESS    --  while success
*       ERROR_CODE          --  refer for err defination
*/

void Vdec_setTplayConfig(VDEC_CHN vdecChnId, VDEC_TPLAY speed)
{
    DecLink_TPlayConfig params = {0};
    UInt32 tPlaySpeed = VDEC_TPLAY_1X;

    printf("\r\nEnable Channel: %d, speed = %d", vdecChnId, speed);

    params.chId = vdecChnId;
    params.targetFps = 30;
    tPlaySpeed = speed;

    if(tPlaySpeed == VDEC_TPLAY_IFRAMEONLY)
        params.inputFps = params.targetFps;
    else if(tPlaySpeed == VDEC_TPLAY_HALFX || tPlaySpeed == VDEC_TPLAY_QUARTERX)
        params.inputFps = params.targetFps;
    else
        params.inputFps = params.targetFps*tPlaySpeed;

    System_linkControl(gVdecModuleContext.decId, DEC_LINK_CMD_SET_TRICKPLAYCONFIG,
                       &params, sizeof(params), TRUE);

}

/**
 * \brief:
 *      Dynamically create a channel on the excisting decode link
 * \input:
 *      vdecChnId       -- decoder channel id
 * \output:
 *      NA
 * \return
*       TI_MEDIA_SUCCESS    --  while success
*       ERROR_CODE          --  refer for err defination
*/

Int32 Vdec_createChn(VDEC_CHN_CREATE_PARAMS_S *vdecCrePrm)
{
     DecLink_addChannelInfo params = {0};
     IpcBitsOutHLOSLink_createChBufParams ipcBufCreateParams;
     Int32 status = ERROR_NONE;

     printf("\r\nOpen Channel: %d \n", vdecCrePrm->chNum);
     params.chId = vdecCrePrm->chNum;
     params.chInfo.width = vdecCrePrm->targetMaxWidth;
     params.chInfo.height = vdecCrePrm->targetMaxHeight;
     params.chInfo.scanFormat = vdecCrePrm->scanFormat;
     params.createPrm.targetMaxWidth = vdecCrePrm->targetMaxWidth;
     params.createPrm.targetMaxHeight = vdecCrePrm->targetMaxHeight;
     params.createPrm.defaultDynamicParams.targetFrameRate = 
                                           vdecCrePrm->targetFrameRate;
     params.createPrm.defaultDynamicParams.targetBitRate = 
                                           vdecCrePrm->targetBitRate;
     params.createPrm.displayDelay = vdecCrePrm->displayDelay;
     params.createPrm.numBufPerCh = vdecCrePrm->numBufPerCh;
     params.createPrm.fieldMergeDecodeEnable = FALSE;
     params.createPrm.processCallLevel = FALSE;
     params.createPrm.dpbBufSizeInFrames = IH264VDEC_DPB_NUMFRAMES_AUTO;
     params.createPrm.algCreateStatus = DEC_LINK_ALG_CREATE_STATUS_CREATE;

    switch (vdecCrePrm->decFormat)
    {
        case VDEC_CHN_H264: /* H264 */
            params.createPrm.format = IVIDEO_H264HP;
            params.createPrm.profile = IH264VDEC_PROFILE_ANY;
            break;
        case VDEC_CHN_MPEG4: /* MPEG4 */
            params.createPrm.format = IVIDEO_MPEG4ASP;
            params.createPrm.profile = 0;
            /* Display delay need to set anything otherthan decode order 
             * otherwise XDM flush doesn't work for MPEG4 decoder */
            params.createPrm.displayDelay = -1;
            break;
        case VDEC_CHN_MJPEG: /* MJPEG */
            params.createPrm.format = IVIDEO_MJPEG;
            params.createPrm.profile = 0;
            params.createPrm.displayDelay = 0;
            break;
        default: /* D1 */
            printf("\r\nCodec Type: %d, returning \n", vdecCrePrm->decFormat);
            status = ERROR_FAIL;
            break;
    }

    if (ERROR_NONE == status)
    {
        status = System_linkControl(gVdecModuleContext.decId, DEC_LINK_CMD_CREATE_CHANNEL,
                           &params, sizeof(params), TRUE);

        if (0 == status)
        {
            memset (&ipcBufCreateParams,0,sizeof(ipcBufCreateParams));
            ipcBufCreateParams.chId = vdecCrePrm->chNum;
            ipcBufCreateParams.maxWidth  = vdecCrePrm->targetMaxWidth;
            ipcBufCreateParams.maxHeight = vdecCrePrm->targetMaxHeight;
            if (vdecCrePrm->numBitBufPerCh == VDEC_NUM_BUF_PER_CH_DEFAULT)
                ipcBufCreateParams.numBufs = 0;
            else
                ipcBufCreateParams.numBufs = vdecCrePrm->numBitBufPerCh;
            status = System_linkControl(gVdecModuleContext.ipcBitsOutHLOSId,
                                        IPCBITSOUT_LINK_CMD_CREATE_CH_BUFFER,
                                        &ipcBufCreateParams,
                                        sizeof(ipcBufCreateParams), TRUE);
        }
    }
    return status;
}

/**
 * \brief:
 *      Dynamically delete a channel from the excisting decode link
 * \input:
 *      vdecChnId       -- decoder channel id
 * \output:
 *      NA
 * \return
*       TI_MEDIA_SUCCESS    --  while success
*       ERROR_CODE          --  refer for err defination
*/

Int32 Vdec_deleteChn(VDEC_CHN vdecChnId)
{
    Int32 status = ERROR_NONE;
    DecLink_ChannelInfo params = {0};
    IpcBitsOutHLOSLink_deleteChBufParams ipcBufDelPrms;

    printf("\r\nClose Channel: %d \n", vdecChnId);

    params.chId = vdecChnId;

    status = System_linkControl(gVdecModuleContext.decId, DEC_LINK_CMD_DELETE_CHANNEL,
                                &params, sizeof(params), TRUE);

    if (0 == status)
    {
        printf("\r\nIPCBITS Close Channel: %d \n", vdecChnId);
        memset(&ipcBufDelPrms,0,sizeof(ipcBufDelPrms));
        ipcBufDelPrms.chId = vdecChnId;
        status =
        System_linkControl(gVdecModuleContext.ipcBitsOutHLOSId,
                           IPCBITSOUT_LINK_CMD_DELETE_CH_BUFFER,
                           &ipcBufDelPrms,
                           sizeof(ipcBufDelPrms), TRUE);
    }
    if (status != 0)
    {
        printf("WARNING:Vdec_deleteChn failed!!!.Error Code [%d]",status);
    }
    return status;
}

/**
 * \brief:
 *      Disable the specific decoder channel
 * \input:
 *      vdecChnId       -- decoder channel id
 * \output:
 *      NA
 * \return
*       TI_MEDIA_SUCCESS    --  while success
*       ERROR_CODE          --  refer for err defination
*/
Int32 Vdec_disableChn(VDEC_CHN vdecChnId)
{
    DecLink_ChannelInfo params = {0};

    printf("\r\nDisable Channel: %d", vdecChnId);

    params.chId = vdecChnId;

    System_linkControl(gVdecModuleContext.decId, DEC_LINK_CMD_DISABLE_CHANNEL,
                       &params, sizeof(params), TRUE);

    return 0;
}

/**
 * \brief:
 *      Enable/Disable the specific decoder channel error reporting
 * \input:
 *      vdecChnId        -- decoder channel id
 *      enableErrReport -- (1 - enable)/(0 - disable) decoder Error Reporting
 * \output:
 *      NA
 * \return
*       TI_MEDIA_SUCCESS    --  while success
*       ERROR_CODE          --  refer for err defination
*/
Int32 Vdec_decErrReport(VDEC_CHN vdecChnId, Bool enableErrReport)
{

    DecLink_ChErrorReport params = {0};

    params.chId             = vdecChnId;
    params.enableErrReport = enableErrReport;

    System_linkControl(gVdecModuleContext.decId, DEC_LINK_CMD_RESET_DEC_ERR_REPORTING,
                       &params, sizeof(params), TRUE);

    return 0;
}

Int32 Vdec_create(System_LinkInQueParams *vdecOutQue, UInt32 vdecNextLinkId, Bool tilerEnable, UInt32 numFramesPerCh)
{
    IpcBitsOutLinkHLOS_CreateParams   ipcBitsOutHostPrm;
    IpcBitsInLinkRTOS_CreateParams    ipcBitsInVideoPrm;
    DecLink_CreateParams              decPrm;
    IpcLink_CreateParams              ipcOutVideoPrm;
    IpcLink_CreateParams              ipcInVpssPrm;

    Int32 i;

    MULTICH_INIT_STRUCT(IpcLink_CreateParams,ipcInVpssPrm);
    MULTICH_INIT_STRUCT(IpcLink_CreateParams,ipcOutVideoPrm);
    MULTICH_INIT_STRUCT(IpcBitsOutLinkHLOS_CreateParams,ipcBitsOutHostPrm);
    MULTICH_INIT_STRUCT(IpcBitsInLinkRTOS_CreateParams,ipcBitsInVideoPrm);
    MULTICH_INIT_STRUCT(DecLink_CreateParams, decPrm);

    gVdecModuleContext.ipcBitsOutHLOSId = SYSTEM_HOST_LINK_ID_IPC_BITS_OUT_0;
    gVdecModuleContext.ipcBitsInRTOSId  = SYSTEM_VIDEO_LINK_ID_IPC_BITS_IN_0;
    gVdecModuleContext.decId            = SYSTEM_LINK_ID_VDEC_0;
    gVdecModuleContext.ipcM3OutId       = SYSTEM_VIDEO_LINK_ID_IPC_OUT_M3_0;
    gVdecModuleContext.ipcM3InId        = SYSTEM_VPSS_LINK_ID_IPC_IN_M3_0;

    ipcBitsOutHostPrm.baseCreateParams.outQueParams[0].nextLink = gVdecModuleContext.ipcBitsInRTOSId;
    ipcBitsOutHostPrm.baseCreateParams.notifyNextLink       = TRUE;
    ipcBitsOutHostPrm.baseCreateParams.notifyPrevLink       = FALSE;
    ipcBitsOutHostPrm.baseCreateParams.noNotifyMode         = FALSE;
    ipcBitsOutHostPrm.baseCreateParams.numOutQue            = 1;
    ipcBitsOutHostPrm.inQueInfo.numCh                       = gVdecModuleContext.vdecConfig.numChn;
    ipcBitsOutHostPrm.bufPoolPerCh                          = TRUE;

    for (i=0; i<ipcBitsOutHostPrm.inQueInfo.numCh; i++)
    {
        ipcBitsOutHostPrm.inQueInfo.chInfo[i].width =
            gVdecModuleContext.vdecConfig.decChannelParams[i].maxVideoWidth;

        ipcBitsOutHostPrm.inQueInfo.chInfo[i].height =
            gVdecModuleContext.vdecConfig.decChannelParams[i].maxVideoHeight;

        ipcBitsOutHostPrm.inQueInfo.chInfo[i].scanFormat =
            SYSTEM_SF_PROGRESSIVE;

        ipcBitsOutHostPrm.inQueInfo.chInfo[i].bufType        = 0; // NOT USED
        ipcBitsOutHostPrm.inQueInfo.chInfo[i].codingformat   = 0; // NOT USED
        ipcBitsOutHostPrm.inQueInfo.chInfo[i].dataFormat     = 0; // NOT USED
        ipcBitsOutHostPrm.inQueInfo.chInfo[i].memType        = 0; // NOT USED
        ipcBitsOutHostPrm.inQueInfo.chInfo[i].startX         = 0; // NOT USED
        ipcBitsOutHostPrm.inQueInfo.chInfo[i].startY         = 0; // NOT USED
        ipcBitsOutHostPrm.inQueInfo.chInfo[i].pitch[0]       = 0; // NOT USED
        ipcBitsOutHostPrm.inQueInfo.chInfo[i].pitch[1]       = 0; // NOT USED
        ipcBitsOutHostPrm.inQueInfo.chInfo[i].pitch[2]       = 0; // NOT USED
    }

    ipcBitsInVideoPrm.baseCreateParams.inQueParams.prevLinkId    = gVdecModuleContext.ipcBitsOutHLOSId;
    ipcBitsInVideoPrm.baseCreateParams.inQueParams.prevLinkQueId = 0;
    ipcBitsInVideoPrm.baseCreateParams.outQueParams[0].nextLink  = gVdecModuleContext.decId;
    ipcBitsInVideoPrm.baseCreateParams.noNotifyMode              = FALSE;
    ipcBitsInVideoPrm.baseCreateParams.notifyNextLink            = TRUE;
    ipcBitsInVideoPrm.baseCreateParams.notifyPrevLink            = FALSE;
    ipcBitsInVideoPrm.baseCreateParams.numOutQue                 = 1;

    for (i=0; i<ipcBitsOutHostPrm.inQueInfo.numCh; i++)
    {
        decPrm.chCreateParams[i].format                 = IVIDEO_H264HP;
        decPrm.chCreateParams[i].profile                = IH264VDEC_PROFILE_ANY;
        decPrm.chCreateParams[i].fieldMergeDecodeEnable = FALSE;

        decPrm.chCreateParams[i].targetMaxWidth  =
            ipcBitsOutHostPrm.inQueInfo.chInfo[i].width;

        decPrm.chCreateParams[i].targetMaxHeight =
            ipcBitsOutHostPrm.inQueInfo.chInfo[i].height;

        decPrm.chCreateParams[i].defaultDynamicParams.targetFrameRate =
            gVdecModuleContext.vdecConfig.decChannelParams[i].dynamicParam.frameRate;

        decPrm.chCreateParams[i].defaultDynamicParams.targetBitRate =
            gVdecModuleContext.vdecConfig.decChannelParams[i].dynamicParam.targetBitRate;
        decPrm.chCreateParams[i].numBufPerCh = numFramesPerCh;
    }

    decPrm.inQueParams.prevLinkId       = gVdecModuleContext.ipcBitsInRTOSId;
    decPrm.inQueParams.prevLinkQueId    = 0;
    decPrm.outQueParams.nextLink        = gVdecModuleContext.ipcM3OutId;
    decPrm.tilerEnable                  = tilerEnable;

    ipcOutVideoPrm.inQueParams.prevLinkId    = gVdecModuleContext.decId;
    ipcOutVideoPrm.inQueParams.prevLinkQueId = 0;
    ipcOutVideoPrm.outQueParams[0].nextLink  = gVdecModuleContext.ipcM3InId;
    ipcOutVideoPrm.notifyNextLink            = TRUE;
    ipcOutVideoPrm.notifyPrevLink            = TRUE;
    ipcOutVideoPrm.numOutQue                 = 1;

    ipcInVpssPrm.inQueParams.prevLinkId    = gVdecModuleContext.ipcM3OutId;
    ipcInVpssPrm.inQueParams.prevLinkQueId = 0;
    ipcInVpssPrm.outQueParams[0].nextLink  = vdecNextLinkId;
    ipcInVpssPrm.notifyNextLink            = TRUE;
    ipcInVpssPrm.notifyPrevLink            = TRUE;
    ipcInVpssPrm.numOutQue                 = 1;

    vdecOutQue->prevLinkId = gVdecModuleContext.ipcM3InId;
    vdecOutQue->prevLinkQueId = 0;

    System_linkCreate(gVdecModuleContext.ipcBitsOutHLOSId, &ipcBitsOutHostPrm, sizeof(ipcBitsOutHostPrm));
    System_linkCreate(gVdecModuleContext.ipcBitsInRTOSId , &ipcBitsInVideoPrm, sizeof(ipcBitsInVideoPrm));
    System_linkCreate(gVdecModuleContext.decId           , &decPrm           , sizeof(decPrm)           );
    System_linkCreate(gVdecModuleContext.ipcM3OutId      , &ipcOutVideoPrm   , sizeof(ipcOutVideoPrm)   );
    System_linkCreate(gVdecModuleContext.ipcM3InId       , &ipcInVpssPrm     , sizeof(ipcInVpssPrm)     );

    return 0;
}

static
Void  Vdec_copyChannelInfo(VIDEO_CHANNEL_LIST_INFO_S *dst,
                           System_LinkQueInfo        *src)
{
    Int i;

    OSA_COMPILETIME_ASSERT(OSA_ARRAYSIZE(src->chInfo)  ==
                           OSA_ARRAYSIZE(dst->chInfo));
    OSA_assert(src->numCh <= OSA_ARRAYSIZE(src->chInfo));
    dst->numCh = src->numCh;
    for (i = 0; i < src->numCh; i++)
    {
        dst->chInfo[i].width  = src->chInfo[i].width;
        dst->chInfo[i].height = src->chInfo[i].height;
    }
}

Int32 Vdec_getChannelInfo(VIDEO_CHANNEL_LIST_INFO_S *channelListInfo)
{
    System_LinkQueInfo inQueInfo;
    Int32 status;

    status =
    IpcBitsOutLink_getInQueInfo(gVdecModuleContext.ipcBitsOutHLOSId,
                                &inQueInfo);
    if (status == ERROR_NONE)
    {
        Vdec_copyChannelInfo(channelListInfo,&inQueInfo);
    }
    else
    {
        channelListInfo->numCh = 0;
    }
    return status;
}

Int32 Vdec_getBufferStatistics(VDEC_BUFFER_STATS_S *bufStats)
{
    memset(&bufStats->stats,0,sizeof(bufStats->stats));

    System_linkControl(gVdecModuleContext.decId, DEC_LINK_CMD_GET_BUFFER_STATISTICS,
                       bufStats, sizeof(*bufStats), TRUE);

    return 0;
}



Int32 Vdec_mapDec2DisplayChId(UInt32 displayID,VDEC_CHN vdecChnId , UInt32 *vdisChnId)
{
    Int32 status = ERROR_NONE;

    if ((vdecChnId < VDEC_CHN_MAX)
        &&
        (displayID < VDIS_DEV_MAX))
    {
        *vdisChnId = gVdecModuleContext.vdisChIdMap[displayID][vdecChnId];
        //printf("VDEC[%d]:Vdec_mapDec2DisplayChId [%d->%d]\n",displayID, vdecChnId,*vdisChnId);
    }
    else
    {
        status = ERROR_FAIL;
    }
    return status;
}

Int32 Vdec_setDecCh2DisplayChMap(UInt32 displayID, UInt32 numCh, VDEC_DISPLAY_CHN_MAP_S chMap[])
{
    Int32 status = ERROR_NONE;
    Int i;
    VDEC_CHN vdecChnId;

    if ((displayID < VDIS_DEV_MAX))
    {
        for (i = 0; i < numCh; i++)
        {
            vdecChnId = chMap[i].decChnId;
            if (vdecChnId < VDEC_CHN_MAX)
            {
                gVdecModuleContext.vdisChIdMap[displayID][vdecChnId] =
                    chMap[i].displayChnId;
            }
            else
            {
                status = ERROR_FAIL;
            }
        }
    }
    else
    {
        status = ERROR_FAIL;
    }
    return status;
}
