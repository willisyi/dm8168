/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/
#include <xdc/std.h>
#include <ti/xdais/xdas.h>
#include <ti/xdais/ialg.h>
#include <ti/sdo/fc/utils/api/alg.h>
#include <ti/sdo/fc/rman/rman.h>
#include <ti/xdais/dm/ividdec3.h>
#include <ijpegvdec.h>
#include <jpegvdec_ti.h>

#include "decLink_priv.h"
#include "decLink_err.h"
#include "decLink_jpeg_priv.h"

#include <mcfw/src_bios6/links_m3video/codec_utils/utils_encdec.h>
#include <mcfw/src_bios6/links_m3video/codec_utils/iresman_hdvicp2_earlyacquire.h>

static IJPEGVDEC_Handle dec_link_jpeg_create(const IJPEGVDEC_Fxns * fxns,
                                             const IJPEGVDEC_Params * prms);
static Void dec_link_jpeg_delete(IJPEGVDEC_Handle handle);
static Int32 decLink_jpeg_control(IJPEGVDEC_Handle handle,
                                  IJPEGVDEC_Cmd cmd,
                                  IJPEGVDEC_DynamicParams * params,
                                  IJPEGVDEC_Status * status);
static Int decLink_jpeg_set_static_params(IJPEGVDEC_Params * staticParams,
                                          DecLink_AlgCreateParams *
                                          algCreateParams);
static Int decLink_jpeg_set_algObject(DecLink_JPEGObj * algObj,
                                      DecLink_AlgCreateParams * algCreateParams,
                                      DecLink_AlgDynamicParams *
                                      algDynamicParams);
static Int decLink_jpeg_set_dynamic_params(IJPEGVDEC_DynamicParams *
                                           dynamicParams,
                                           DecLink_AlgDynamicParams *
                                           algDynamicParams);
static Void decLink_jpeg_freersrc(DecLink_JPEGObj * hObj, Int rsrcMask);

extern IRES_Fxns JPEGVDEC_TI_IRES;

/*
 *  ======== decLink_jpeg_create ========
 *  Create an JPEGVDEC instance object (using parameters specified by prms)
 */

static IJPEGVDEC_Handle dec_link_jpeg_create(const IJPEGVDEC_Fxns * fxns,
                                             const IJPEGVDEC_Params * prms)
{
    return ((IJPEGVDEC_Handle) ALG_create((IALG_Fxns *) fxns,
                                          NULL, (IALG_Params *) prms));
}

/*
 *  ======== decLink_jpeg_delete ========
 *  Delete the JPEGVDEC instance object specified by handle
 */

static Void dec_link_jpeg_delete(IJPEGVDEC_Handle handle)
{
    ALG_delete((IALG_Handle) handle);
}

/*
 *  ======== decLink_jpeg_control ========
 */

static Int32 decLink_jpeg_control(IJPEGVDEC_Handle handle,
                                  IJPEGVDEC_Cmd cmd,
                                  IJPEGVDEC_DynamicParams * params,
                                  IJPEGVDEC_Status * status)
{
    int error = 0;
    IALG_Fxns *fxns = (IALG_Fxns *) handle->fxns;

    fxns->algActivate((IALG_Handle) handle);

    error = handle->fxns->ividdec.control((IVIDDEC3_Handle) handle,
                                           (IVIDDEC3_Cmd) cmd,
                                           (IVIDDEC3_DynamicParams *) params,
                                           (IVIDDEC3_Status *) status);
    fxns->algDeactivate((IALG_Handle) handle);

    if (error != XDM_EOK)
    {
        DECLINK_INTERNAL_ERROR_LOG(error, "ALGCONTROL FAILED:CMD:%d", cmd);
    }
    return error;
}

/*
 *  ======== Declink_jpegDecodeFrameBatch ========
 */
Int32 Declink_jpegDecodeFrameBatch(DecLink_Obj * pObj,
                                   DecLink_ReqBatch * pReqObjBatch,
                                   FVID2_FrameList * freeFrameList)
{
    int error = XDM_EFAIL, reqObjIdx, chId;
    Int32 i, freeBufIdx, prosIdx;
    IJPEGVDEC_InArgs *inArgs;
    IJPEGVDEC_OutArgs *outArgs;
    XDM2_BufDesc *inputBufDesc;
    XDM2_BufDesc *outputBufDesc;
    IJPEGVDEC_Handle handle;
    IALG_Fxns *fxns = NULL;
    FVID2_Frame *outFrame = NULL;
    IVIDEO2_BufDesc *displayBufs = NULL;
    UInt32 bytesConsumed;
    DecLink_ReqObj *pReqObj;
    DecLink_ChObj *pChObj;
    System_FrameInfo *pFrameInfo;

    /*Make sure that the Req Object is not empty*/
    UTILS_assert (pReqObjBatch->numReqObjsInBatch > 0);
    
    for (reqObjIdx = 0; reqObjIdx < pReqObjBatch->numReqObjsInBatch; reqObjIdx++)
    {
        pReqObj = pReqObjBatch->pReqObj[reqObjIdx];
        chId = pReqObj->InBuf->channelNum;
        pChObj = &pObj->chObj[chId];
        
        inArgs = &pChObj->algObj.u.jpegAlgIfObj.inArgs;
        outArgs = &pChObj->algObj.u.jpegAlgIfObj.outArgs;
        inputBufDesc = &pChObj->algObj.u.jpegAlgIfObj.inBufs;
        outputBufDesc = &pChObj->algObj.u.jpegAlgIfObj.outBufs;
        handle = pChObj->algObj.u.jpegAlgIfObj.algHandle;
        
        UTILS_assert(handle != NULL);
        
        fxns = (IALG_Fxns *) handle->fxns;



        bytesConsumed = 0;

        for (prosIdx=0; prosIdx< pReqObj->OutFrameList.numFrames; prosIdx++)
        {
            /*----------------------------------------------------------------*/
            /* Initialize the input ID in input arguments to the bufferid of  */
            /* buffer element returned from getfreebuffer() function.         */
            /*----------------------------------------------------------------*/
            /* inputID & numBytes need to update before every decode call */

            if (FALSE == outArgs->viddecOutArgs.outBufsInUseFlag)
            {
                outFrame = pReqObj->OutFrameList.frames[prosIdx];
            }
            else
            {
                UTILS_assert(NULL != pChObj->algObj.prevOutFrame);
                /* Previous buffer was in use. Free the current outBuf */
                outFrame = pChObj->algObj.prevOutFrame;
                freeFrameList->frames[freeFrameList->numFrames] = 
                                pReqObj->OutFrameList.frames[prosIdx];
                pChObj->numBufsInCodec--;
                freeFrameList->numFrames++;
            }

            inArgs->viddecInArgs.inputID = (UInt32) outFrame;
            inArgs->viddecInArgs.numBytes = pReqObj->InBuf->fillLength - 
                                                            bytesConsumed;

            for (i = 0; i < inputBufDesc->numBufs; i++)
            {
                /* Set proper buffer addresses for bitstreamn data */
                /*---------------------------------------------------------------*/
                inputBufDesc->descs[i].buf = (XDAS_Int8 *) pReqObj->InBuf->addr 
                                                           +  bytesConsumed;
                inputBufDesc->descs[i].bufSize.bytes = pReqObj->InBuf->bufSize;
            }

            for (i = 0; i < outputBufDesc->numBufs; i++)
            {
                /* Set proper buffer addresses for Frame data */
                /*------------------------------------------------------------*/
                if (pChObj->algObj.algCreateParams.tilerEnable)
                {
                    outputBufDesc->descs[i].buf =
                        (Ptr)
                        Utils_tilerAddr2CpuAddr((UInt32) (outFrame->addr[0][i]));
                }
                else
                {
                    outputBufDesc->descs[i].buf = outFrame->addr[0][i];
                }
            }
          
            fxns->algActivate((IALG_Handle) handle);
            error = handle->fxns->ividdec.process((IVIDDEC3_Handle) handle,
                                                  inputBufDesc,
                                                  outputBufDesc,
                                                  (IVIDDEC3_InArgs *) inArgs,
                                                  (IVIDDEC3_OutArgs *) outArgs);
            fxns->algDeactivate((IALG_Handle) handle);
            bytesConsumed = outArgs->viddecOutArgs.bytesConsumed;
            if (error != XDM_EOK)
            {
                DECLINK_INTERNAL_ERROR_LOG(error, "ALGPROCESS FAILED:STATUS");
            }
            pChObj->algObj.prevOutFrame = outFrame;
            pReqObj->OutFrameList.frames[prosIdx] = NULL;
            UTILS_assert(outArgs->viddecOutArgs.displayBufsMode ==
                         IVIDDEC3_DISPLAYBUFS_EMBEDDED);
            displayBufs = &(outArgs->viddecOutArgs.displayBufs.bufDesc[0]);
            if ((outArgs->viddecOutArgs.outputID[0] != 0)
                && (displayBufs->numPlanes))
            {
                XDAS_Int8 *pExpectedBuf;

                pReqObj->OutFrameList.frames[prosIdx] =
                  (FVID2_Frame *) outArgs->viddecOutArgs.outputID[0];
                if (pChObj->algObj.algCreateParams.tilerEnable)
                {
                    pExpectedBuf = (Ptr) Utils_tilerAddr2CpuAddr(
                        (UInt32) pReqObj->OutFrameList.frames[prosIdx]->addr[0][0]);
                }
                else
                {
                    pExpectedBuf = pReqObj->OutFrameList.frames[prosIdx]->addr[0][0];
                }
                UTILS_assert(displayBufs->planeDesc[0].buf == pExpectedBuf);
                /* Enable this code once SysTemFrameInfo is updated with support
                 * for storing frame resolution info */
                pFrameInfo = (System_FrameInfo *) 
                             pReqObj->OutFrameList.frames[prosIdx]->appData;
                {
                    UTILS_assert(pFrameInfo != NULL);
                    pFrameInfo->rtChInfo.width =
                        displayBufs->activeFrameRegion.bottomRight.x -
                        displayBufs->activeFrameRegion.topLeft.x;
                    pFrameInfo->rtChInfo.height =
                        displayBufs->activeFrameRegion.bottomRight.y -
                        displayBufs->activeFrameRegion.topLeft.y;
                    pFrameInfo->rtChInfo.pitch[0] = displayBufs->imagePitch[0];
                    pFrameInfo->rtChInfo.pitch[1] = displayBufs->imagePitch[1];
                    pFrameInfo->rtChInfo.startX = 
                        displayBufs->activeFrameRegion.topLeft.x;
                    pFrameInfo->rtChInfo.startY = 
                        displayBufs->activeFrameRegion.topLeft.y;
                    pFrameInfo->rtChInfoUpdate = TRUE;
                }
                pReqObj->OutFrameList.frames[prosIdx]->fid =
                    Utils_encdecMapXDMContentType2FVID2FID(displayBufs->
                                                           contentType);
            }
            freeBufIdx = 0;
            while (outArgs->viddecOutArgs.freeBufID[freeBufIdx] != 0)
            {
                freeFrameList->frames[freeFrameList->numFrames] =
                  (FVID2_Frame *) outArgs->viddecOutArgs.freeBufID[freeBufIdx];
                freeFrameList->numFrames++;
                pChObj->numBufsInCodec--;
                freeBufIdx++;
            }
        }
    
	}

    return (error);
}

static Int decLink_jpeg_set_static_params(IJPEGVDEC_Params * staticParams,
                                          DecLink_AlgCreateParams *
                                          algCreateParams)
{
    /* Initialize default values for static params */
    *staticParams = JPEGVDEC_TI_Static_Params;

    staticParams->viddecParams.maxHeight = algCreateParams->maxHeight;

    staticParams->viddecParams.maxWidth = algCreateParams->maxWidth;

    staticParams->viddecParams.displayDelay = algCreateParams->displayDelay;
    return 0;
}

static Int decLink_jpeg_set_algObject(DecLink_JPEGObj * algObj,
                                      DecLink_AlgCreateParams * algCreateParams,
                                      DecLink_AlgDynamicParams *
                                      algDynamicParams)
{
    UInt32 bufCnt;
    IJPEGVDEC_InArgs *inArgs;
    IJPEGVDEC_OutArgs *outArgs;
    XDM2_BufDesc *inputBufDesc;
    XDM2_BufDesc *outputBufDesc;

    inArgs = &algObj->inArgs;
    outArgs = &algObj->outArgs;
    inputBufDesc = &algObj->inBufs;
    outputBufDesc = &algObj->outBufs;

     /*-----------------------------------------------------------------------*/
    /* Initialize the input ID in input arguments to the bufferid of */
    /* buffer element returned from getfreebuffer() function.  */
     /*-----------------------------------------------------------------------*/
    /* inputID & numBytes need to update before every decode process call */
    inArgs->viddecInArgs.inputID = 0;
    inArgs->viddecInArgs.numBytes = 0;

    /*------------------------------------------------------------------------*/
    /* The outBufsInUseFlag tells us whether the previous input buffer given */
    /* by the application to the algorithm is still in use or not. Since */
    /* this is before the first decode call, assign this flag to 0. The */
    /* algorithm will take care to initialize this flag appropriately from */
    /* hereon for the current configuration.  */
    /*------------------------------------------------------------------------*/
    outArgs->viddecOutArgs.outBufsInUseFlag = 0;
    outArgs->viddecOutArgs.bytesConsumed = 0;
    outArgs->viddecOutArgs.freeBufID[0] = 0;
    outArgs->viddecOutArgs.outputID[0] = 0;
    outArgs->viddecOutArgs.extendedError = 0;
    outArgs->viddecOutArgs.displayBufsMode = IVIDDEC3_DISPLAYBUFS_EMBEDDED;
    memset(&outArgs->viddecOutArgs.displayBufs.bufDesc, 0,
           sizeof(outArgs->viddecOutArgs.displayBufs.bufDesc));
    outArgs->viddecOutArgs.displayBufs.pBufDesc[0] = NULL;
    outArgs->viddecOutArgs.decodedBufs.contentType = IVIDEO_PROGRESSIVE_FRAME;
    outArgs->viddecOutArgs.decodedBufs.frameType = IVIDEO_I_FRAME;
    outArgs->viddecOutArgs.decodedBufs.extendedError = 0;

    /*------------------------------------------------------------------------*/
    /* Initialize the input buffer properties as required by algorithm */
    /* based on info received by preceding GETBUFINFO call. First init the */
    /* number of input bufs.  */
    /*------------------------------------------------------------------------*/
    inputBufDesc->numBufs = algObj->status.viddecStatus.bufInfo.minNumInBufs;
    /*------------------------------------------------------------------------*/
    /* For the num of input bufs, initialize the buffer pointer addresses */
    /* and buffer sizes.  */
    /*------------------------------------------------------------------------*/
    inputBufDesc->descs[0].buf = NULL;
    inputBufDesc->descs[0].bufSize.bytes = 0;
    inputBufDesc->descs[0].memType =
        algObj->status.viddecStatus.bufInfo.inBufMemoryType[0];
    inputBufDesc->descs[0].memType = XDM_MEMTYPE_RAW;

    outputBufDesc->numBufs = algObj->status.viddecStatus.bufInfo.minNumOutBufs;
    for (bufCnt = 0; bufCnt < outputBufDesc->numBufs; bufCnt++)
    {
        outputBufDesc->descs[bufCnt].buf = NULL;
        outputBufDesc->descs[bufCnt].memType =
            algObj->status.viddecStatus.bufInfo.outBufMemoryType[bufCnt];
        if (algCreateParams->tilerEnable)
        {
            if (bufCnt & 0x1)
            {
                outputBufDesc->descs[bufCnt].memType = XDM_MEMTYPE_TILED16;
            }
            else
            {
                outputBufDesc->descs[bufCnt].memType = XDM_MEMTYPE_TILED8;
            }
        }
        else
        {
            outputBufDesc->descs[bufCnt].memType = XDM_MEMTYPE_RAW;
        }

        if (outputBufDesc->descs[bufCnt].memType != XDM_MEMTYPE_RAW)
        {
            outputBufDesc->descs[bufCnt].bufSize.tileMem.width =
                algObj->status.viddecStatus.bufInfo.minOutBufSize[bufCnt].
                tileMem.width;
            outputBufDesc->descs[bufCnt].bufSize.tileMem.height =
                algObj->status.viddecStatus.bufInfo.minOutBufSize[bufCnt].
                tileMem.height;
        }
        else
        {
            outputBufDesc->descs[bufCnt].bufSize.bytes =
                (algObj->status.viddecStatus.bufInfo.minOutBufSize[bufCnt].
                tileMem.width *
                algObj->status.viddecStatus.bufInfo.minOutBufSize[bufCnt].
                tileMem.height);
        }
    }

    return 0;
}

static Int decLink_jpeg_set_dynamic_params(IJPEGVDEC_DynamicParams *
                                           dynamicParams,
                                           DecLink_AlgDynamicParams *
                                           algDynamicParams)
{
    *dynamicParams = JPEGVDEC_TI_DynamicParams;

    dynamicParams->viddecDynamicParams.decodeHeader =
        algDynamicParams->decodeHeader;
    dynamicParams->viddecDynamicParams.displayWidth =
        algDynamicParams->displayWidth;
    dynamicParams->viddecDynamicParams.frameSkipMode =
        algDynamicParams->frameSkipMode;
    dynamicParams->viddecDynamicParams.newFrameFlag =
        algDynamicParams->newFrameFlag;

    return DEC_LINK_S_SUCCESS;
}

#define DECLINKJPEG_ALGREATE_RSRC_NONE                                       (0)
#define DECLINKJPEG_ALGREATE_RSRC_ALGCREATED                           (1 <<  0)
#define DECLINKJPEG_ALGREATE_RSRC_IRES_ASSIGNED                        (1 <<  1)
#define DECLINKJPEG_ALGREATE_RSRC_ALL (                                        \
                                       DECLINKJPEG_ALGREATE_RSRC_ALGCREATED |  \
                                       DECLINKJPEG_ALGREATE_RSRC_IRES_ASSIGNED \
                                      )

static Void decLink_jpeg_freersrc(DecLink_JPEGObj * hObj, Int rsrcMask)
{
    if (rsrcMask & DECLINKJPEG_ALGREATE_RSRC_IRES_ASSIGNED)
    {
        IRES_Status iresStatus;

        iresStatus =
            RMAN_freeResources((IALG_Handle) hObj->algHandle,
                               &JPEGVDEC_TI_IRES, hObj->scratchID);
        if (iresStatus != IRES_OK)
        {
            DECLINK_INTERNAL_ERROR_LOG(iresStatus, "RMAN_freeResources FAILED");
        }
    }
    if (rsrcMask & DECLINKJPEG_ALGREATE_RSRC_ALGCREATED)
    {
        dec_link_jpeg_delete(hObj->algHandle);
        hObj->algHandle = NULL;
    }
}

Int DecLinkJPEG_algCreate(DecLink_JPEGObj * hObj,
                          DecLink_AlgCreateParams * algCreateParams,
                          DecLink_AlgDynamicParams * algDynamicParams,
                          Int linkID, Int channelID, Int scratchGroupID,
                          FVID2_Format *pFormat, UInt32 numFrames,
                          IRES_ResourceDescriptor resDesc[])
{
    Int retVal = DEC_LINK_S_SUCCESS;
    Int rsrcMask = DECLINKJPEG_ALGREATE_RSRC_NONE;

    UTILS_assert(Utils_encdecIsJPEG(algCreateParams->format) == TRUE);
    hObj->linkID = linkID;
    hObj->channelID = channelID;
    hObj->scratchID = scratchGroupID;

    memset(&hObj->inArgs, 0, sizeof(hObj->inArgs));
    memset(&hObj->outArgs, 0, sizeof(hObj->outArgs));
    memset(&hObj->inBufs, 0, sizeof(hObj->inBufs));
    memset(&hObj->outBufs, 0, sizeof(hObj->outBufs));
    memset(&hObj->status, 0, sizeof(hObj->status));
    memset(&hObj->memUsed, 0, sizeof(hObj->memUsed));

    hObj->staticParams.viddecParams.size = sizeof(IJPEGVDEC_Params);
    hObj->status.viddecStatus.size = sizeof(IJPEGVDEC_Status);
    hObj->dynamicParams.viddecDynamicParams.size =
        sizeof(IJPEGVDEC_DynamicParams);
    hObj->inArgs.viddecInArgs.size = sizeof(IJPEGVDEC_InArgs);
    hObj->outArgs.viddecOutArgs.size = sizeof(IJPEGVDEC_OutArgs);

    decLink_jpeg_set_static_params(&hObj->staticParams, algCreateParams);
    decLink_jpeg_set_dynamic_params(&hObj->dynamicParams, algDynamicParams);

    UTILS_MEMLOG_USED_START();
    hObj->algHandle =
        dec_link_jpeg_create((IJPEGVDEC_Fxns *) & JPEGVDEC_TI_IJPEGVDEC,
                             &hObj->staticParams);
    UTILS_assertError((NULL != hObj->algHandle),
                      retVal, DEC_LINK_E_ALGCREATEFAILED, linkID, channelID);

    if (!UTILS_ISERROR(retVal))
    {
        Int32 status = UTILS_ENCDEC_S_SUCCESS;
        if (algCreateParams->tilerEnable == FALSE)
        {
            status = Utils_encdec_checkResourceAvail((IALG_Handle) hObj->algHandle,
                           &JPEGVDEC_TI_IRES, pFormat, numFrames, resDesc);
        }
        UTILS_assertError((status == UTILS_ENCDEC_S_SUCCESS), retVal,
                          DEC_LINK_E_RMANRSRCASSIGNFAILED, linkID, channelID);
    }
    if (!UTILS_ISERROR(retVal))
    {
        IRES_Status iresStatus;

        rsrcMask |= DECLINKJPEG_ALGREATE_RSRC_ALGCREATED;
        iresStatus = RMAN_assignResources((IALG_Handle) hObj->algHandle,
                                          &JPEGVDEC_TI_IRES, scratchGroupID);
        UTILS_assertError((iresStatus == IRES_OK), retVal,
                          DEC_LINK_E_RMANRSRCASSIGNFAILED, linkID, channelID);
    }
    if (!UTILS_ISERROR(retVal))
    {
        Int algStatus;

        rsrcMask |= DECLINKJPEG_ALGREATE_RSRC_IRES_ASSIGNED;

        hObj->status.viddecStatus.data.buf = &(hObj->versionInfo[0]);
        hObj->status.viddecStatus.data.bufSize = sizeof(hObj->versionInfo);
        algStatus = decLink_jpeg_control(hObj->algHandle, XDM_GETVERSION,
                                         &(hObj->dynamicParams),
                                         &(hObj->status));
        if (algStatus == XDM_EOK)
        {
            DECLINK_INFO_LOG_VERBOSE(hObj->linkID, hObj->channelID,
                                     "JPEGDecCreated:%s", hObj->versionInfo);
        }

        algStatus = decLink_jpeg_control(hObj->algHandle, XDM_GETBUFINFO,
                                         &(hObj->dynamicParams),
                                         &(hObj->status));
        if (algStatus == XDM_EOK)
        {
            DECLINK_INFO_LOG_VERBOSE(hObj->linkID, hObj->channelID,
                                     "XDM_GETBUFINFO done");
        }

        algStatus = decLink_jpeg_control(hObj->algHandle,
                                         XDM_SETPARAMS,
                                         &hObj->dynamicParams, &hObj->status);
        UTILS_assertError((algStatus == XDM_EOK), retVal,
                          DEC_LINK_E_ALGSETPARAMSFAILED, linkID, channelID);
    }
    if (!UTILS_ISERROR(retVal))
    {
        decLink_jpeg_control(hObj->algHandle,
                             XDM_GETSTATUS,
                             &hObj->dynamicParams, &hObj->status);
    }
    if (UTILS_ISERROR(retVal))
    {
        decLink_jpeg_freersrc(hObj, rsrcMask);
    }
    else
    {
        /* Initialize the Inarg, OutArg, InBuf & OutBuf objects */
        decLink_jpeg_set_algObject(hObj, algCreateParams, algDynamicParams);
    }
    
    UTILS_MEMLOG_USED_END(hObj->memUsed);
    UTILS_MEMLOG_PRINT("DECLINK_JPEG",
                       hObj->memUsed,
                       (sizeof(hObj->memUsed) / sizeof(hObj->memUsed[0])));
    return retVal;
}

Void DecLinkJPEG_algDelete(DecLink_JPEGObj * hObj)
{
    UTILS_MEMLOG_FREE_START();
    if (hObj->algHandle)
    {
        decLink_jpeg_freersrc(hObj, DECLINKJPEG_ALGREATE_RSRC_ALL);
    }

    if (hObj->algHandle)
    {
        dec_link_jpeg_delete(hObj->algHandle);
    }
    UTILS_MEMLOG_FREE_END(hObj->memUsed, 0 /* dont care */ );
}

