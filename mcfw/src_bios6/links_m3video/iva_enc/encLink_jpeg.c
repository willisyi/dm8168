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
#include <ti/xdais/dm/ividenc2.h>
#include <ijpegenc.h>
#include <jpegenc_ti.h>

#include "encLink_priv.h"
#include "encLink_jpeg_priv.h"

#include <mcfw/src_bios6/links_m3video/codec_utils/utils_encdec.h>


static JPEGVENC_Handle enc_link_jpeg_create(const IJPEGVENC_Fxns * fxns,
                                            const IJPEGVENC_Params * prms);
static Void enc_link_jpeg_delete(JPEGVENC_Handle handle);
static Int32 enclink_jpeg_control(JPEGVENC_Handle handle,
                                  IJPEGVENC_Cmd cmd,
                                  IJPEGVENC_DynamicParams * params,
                                  IJPEGVENC_Status * status);
static Int enclink_jpeg_set_static_params(IJPEGVENC_Params * staticParams,
                                          EncLink_AlgCreateParams *
                                          algCreateParams);
static Int enclink_jpeg_set_algObject(EncLink_JPEGObj * algObj,
                                      EncLink_AlgCreateParams * algCreateParams,
                                      EncLink_AlgDynamicParams *
                                      algDynamicParams);
static Int enclink_jpeg_set_dynamic_params(IJPEGVENC_DynamicParams *
                                           dynamicParams,
                                           EncLink_AlgDynamicParams *
                                           algDynamicParams);
static Void enclink_jpeg_freersrc(EncLink_JPEGObj * hObj, Int rsrcMask);

extern IRES_Fxns JPEGVENC_TI_IRES;

/* 
 *  ======== enc_link_jpeg_create ========
 *  Create an JPEGENC instance object (using parameters specified by prms)
 */

static JPEGVENC_Handle enc_link_jpeg_create(const IJPEGVENC_Fxns * fxns,
                                            const IJPEGVENC_Params * prms)
{
    return ((JPEGVENC_Handle) ALG_create((IALG_Fxns *) fxns,
                                         NULL, (IALG_Params *) prms));
}

/* 
 *  ======== enc_link_jpeg_delete ========
 *  Delete the JPEGENC instance object specified by handle
 */

static Void enc_link_jpeg_delete(JPEGVENC_Handle handle)
{
    ALG_delete((IALG_Handle) handle);
}

/* 
 *  ======== enc_link_jpeg_control ========
 */

static Int32 enclink_jpeg_control(JPEGVENC_Handle handle,
                                  IJPEGVENC_Cmd cmd,
                                  IJPEGVENC_DynamicParams * params,
                                  IJPEGVENC_Status * status)
{
    int error = 0;
    IALG_Fxns *fxns = (IALG_Fxns *) handle->fxns;

    fxns->algActivate((IALG_Handle) handle);

    error = handle->fxns->ividenc.control((IVIDENC2_Handle) handle, cmd,
                                          (IVIDENC2_DynamicParams *) params,
                                          (IVIDENC2_Status *) status);
    fxns->algDeactivate((IALG_Handle) handle);

    if (error != XDM_EOK)
    {
        ENCLINK_INTERNAL_ERROR_LOG(error, "ALGCONTROL FAILED:CMD:%d", cmd);
    }
    return error;
}

/* 
 *  ======== Enclink_jpegEncodeFrame ========
 */
Int32 Enclink_jpegEncodeFrame(EncLink_ChObj * pChObj, EncLink_ReqObj * pReqObj)
{
    int error;
    Int32 i;
    IJPEGVENC_InArgs *inArgs;
    IJPEGVENC_OutArgs *outArgs;
    IVIDEO2_BufDesc *inputBufDesc;
    XDM2_BufDesc *outputBufDesc;
    JPEGVENC_Handle handle;
    IALG_Fxns *fxns = NULL;
    IVIDEO_ContentType contentType;

    inArgs = &pChObj->algObj.u.jpegAlgIfObj.inArgs;
    outArgs = &pChObj->algObj.u.jpegAlgIfObj.outArgs;
    inputBufDesc = &pChObj->algObj.u.jpegAlgIfObj.inBufs;
    outputBufDesc = &pChObj->algObj.u.jpegAlgIfObj.outBufs;
    handle = pChObj->algObj.u.jpegAlgIfObj.algHandle;



    UTILS_assert(handle != NULL);

    fxns = (IALG_Fxns *) handle->fxns;

    inArgs->videnc2InArgs.inputID =
        (UInt32) pReqObj->InFrameList.frames[0]->addr[0][0];
    for (i = 0; i < inputBufDesc->numPlanes; i++)
    {
        /* Set proper buffer addresses for Frame data */
        /*---------------------------------------------------------------*/
        if (pChObj->algObj.algCreateParams.tilerEnable)
        {
            inputBufDesc->planeDesc[i].buf =
                (Ptr)
                Utils_tilerAddr2CpuAddr((UInt32)
                                        (pReqObj->InFrameList.frames[0]->
                                         addr[0][i]));
        }
        else
        {
            inputBufDesc->planeDesc[i].buf =
                pReqObj->InFrameList.frames[0]->addr[0][i];
        }
    }

    pReqObj->OutBuf->mvDataFilledSize = 0;
    pReqObj->OutBuf->temporalId = 0;
    pReqObj->OutBuf->numTemporalLayerSetInCodec = 0;
    for (i = 0; i < outputBufDesc->numBufs; i++)
    {
        /* Set proper buffer addresses for bitstream data */
      /*---------------------------------------------------------------*/
        outputBufDesc->descs[i].buf = pReqObj->OutBuf->addr;
        outputBufDesc->descs[i].bufSize.bytes = pReqObj->OutBuf->bufSize;
    }

    fxns->algActivate((IALG_Handle) handle);
    error =
        handle->fxns->ividenc.process((IVIDENC2_Handle) handle,
                                      inputBufDesc, outputBufDesc,
                                      (IVIDENC2_InArgs *) inArgs,
                                      (IVIDENC2_OutArgs *) outArgs);
    fxns->algDeactivate((IALG_Handle) handle);

    pReqObj->OutBuf->fillLength = outArgs->videnc2OutArgs.bytesGenerated;

    if(Utils_encdecIsJPEG(pChObj->algObj.u.jpegAlgIfObj.format) == TRUE)
            pReqObj->OutBuf->codingType = VCODEC_TYPE_MJPEG;

    pReqObj->OutBuf->startOffset = 0;

    if (pChObj->algObj.u.jpegAlgIfObj.staticParams.videnc2Params.
        inputContentType == IVIDEO_PROGRESSIVE)
    {
        contentType = IVIDEO_PROGRESSIVE;
    }
    else
    {
        contentType =
            Utils_encdecMapFVID2FID2XDMContentType((FVID2_Fid) pReqObj->
                                                   InFrameList.frames[0]->
                                                   fid);
    }
    pReqObj->OutBuf->isKeyFrame =
        Utils_encdecIsGopStart(outArgs->videnc2OutArgs.encodedFrameType,
                               contentType);
    pReqObj->OutBuf->frameWidth = inputBufDesc->imageRegion.bottomRight.x -
                                  inputBufDesc->imageRegion.topLeft.x;
    pReqObj->OutBuf->frameHeight = inputBufDesc->imageRegion.bottomRight.y -
                                  inputBufDesc->imageRegion.topLeft.y;

    if (error != XDM_EOK)
    {
        ENCLINK_INTERNAL_ERROR_LOG(error, "ALGPROCESS FAILED");
    }

    return (error);
}

static Int enclink_jpeg_set_static_params(IJPEGVENC_Params * staticParams,
                                          EncLink_AlgCreateParams *
                                          algCreateParams)
{
    /* Initialize default values for static params */
    *staticParams = JPEGVENC_TI_PARAMS;

    staticParams->videnc2Params.maxHeight = algCreateParams->maxHeight;

    staticParams->videnc2Params.maxWidth = algCreateParams->maxWidth;

    staticParams->videnc2Params.maxInterFrameInterval = NULL;
    staticParams->videnc2Params.inputContentType = IVIDEO_PROGRESSIVE;

    staticParams->videnc2Params.inputChromaFormat =
        algCreateParams->inputChromaFormat;

    staticParams->videnc2Params.profile = algCreateParams->profile;

    staticParams->videnc2Params.level = algCreateParams->level;

    staticParams->videnc2Params.numInputDataUnits = 1;
    staticParams->videnc2Params.numOutputDataUnits = 1;
    return 0;
}

static Int enclink_jpeg_set_algObject(EncLink_JPEGObj * algObj,
                                      EncLink_AlgCreateParams * algCreateParams,
                                      EncLink_AlgDynamicParams *
                                      algDynamicParams)
{
    IJPEGVENC_InArgs *inArgs;
    IJPEGVENC_OutArgs *outArgs;
    IVIDEO2_BufDesc *inputBufDesc;
    XDM2_BufDesc *outputBufDesc;
    IJPEGVENC_Status *status;
    Int i;

    inArgs = &algObj->inArgs;
    outArgs = &algObj->outArgs;
    inputBufDesc = &algObj->inBufs;
    outputBufDesc = &algObj->outBufs;
    status = &algObj->status;

     /*-----------------------------------------------------------------------*/
    /* Initialize the input ID in input arguments to the bufferid of the */
    /* buffer element returned from getfreebuffer() function.  */
     /*-----------------------------------------------------------------------*/
    /* inputID need to update before every encode process call */
    inArgs->videnc2InArgs.inputID = 0;
    inArgs->videnc2InArgs.control = IVIDENC2_CTRL_DEFAULT;

    outArgs->videnc2OutArgs.extendedError = 0;
    outArgs->videnc2OutArgs.bytesGenerated = 0;
    outArgs->videnc2OutArgs.encodedFrameType = IVIDEO_I_FRAME;
    outArgs->videnc2OutArgs.inputFrameSkip = 0;
    memset(&outArgs->videnc2OutArgs.freeBufID, 0,
           sizeof(outArgs->videnc2OutArgs.freeBufID));
    outArgs->videnc2OutArgs.reconBufs.planeDesc[0].buf = NULL;
    outArgs->videnc2OutArgs.reconBufs.planeDesc[1].buf = NULL;
    outArgs->videnc2OutArgs.reconBufs.imagePitch[0] = 0;

    /*------------------------------------------------------------------------*/
    /* Initialise output discriptors */
    /*------------------------------------------------------------------------*/
    outputBufDesc->numBufs = 0;
    for (i = 0; i < algObj->status.videnc2Status.bufInfo.minNumOutBufs; i++)
    {

        outputBufDesc->numBufs++;
        outputBufDesc->descs[i].memType = XDM_MEMTYPE_RAW;
        outputBufDesc->descs[i].bufSize.bytes =
            algObj->status.videnc2Status.bufInfo.minOutBufSize[i].bytes;

        if (i == 0)
        {
        /*-------------------------------------------------------------------*/
            /* Set proper buffer addresses for bitstream data */
        /*-------------------------------------------------------------------*/
            outputBufDesc->descs[0].buf = NULL;
        }
        else
        {
            if (status->videnc2Status.bufInfo.minOutBufSize[i].bytes
                > ANALYTICINFO_OUTPUT_BUFF_SIZE)
            {
                Vps_printf
                    ("\nMemory could not get allocated for Analytic info buffer\n");
            }
        /*-------------------------------------------------------------------*/
            /* Set proper buffer addresses for MV & SAD data */
        /*-------------------------------------------------------------------*/
            outputBufDesc->descs[i].buf = NULL;
        }
    }

    /*------------------------------------------------------------------------*/
    /* Video buffer layout, field interleaved or field separated */
    /* Only IVIDEO_FIELD_INTERLEAVED and IVIDEO_FIELD_SEPARATED are supported 
     */
    /*------------------------------------------------------------------------*/
    inputBufDesc->dataLayout = algCreateParams->dataLayout;

    /*------------------------------------------------------------------------*/
    /* Flag to indicate field order in interlaced content */
    /* Supported values are */
    /* 0 - Bottom field first */
    /* 1 - Top filed first */
    /* TODO : need to find defalut parameter */
    /*------------------------------------------------------------------------*/
    inputBufDesc->topFieldFirstFlag = 1;

    /*------------------------------------------------------------------------*/
    /* Initialize the input buffer properties as required by algorithm */
    /* based on info received by preceding GETBUFINFO call.  */
    /*------------------------------------------------------------------------*/
    inputBufDesc->numPlanes = 2;                           /* status.videnc2Status.bufInfo.minNumInBufs; 
                                                            */
    inputBufDesc->numMetaPlanes = 0;
    /*------------------------------------------------------------------------*/
    /* Set entire Image region in the buffer by using config parameters */
    /*------------------------------------------------------------------------*/
    inputBufDesc->imageRegion.topLeft.x = algDynamicParams->startX;
    inputBufDesc->imageRegion.topLeft.y = algDynamicParams->startY;
    inputBufDesc->imageRegion.bottomRight.x = algDynamicParams->startX +
        algObj->dynamicParams.videnc2DynamicParams.inputWidth;
    inputBufDesc->imageRegion.bottomRight.y = algDynamicParams->startY +
        algObj->dynamicParams.videnc2DynamicParams.inputHeight;
    /*------------------------------------------------------------------------*/
    /* Set proper Image region in the buffer by using config parameters */
    /*------------------------------------------------------------------------*/
    inputBufDesc->activeFrameRegion.topLeft.x = algDynamicParams->startX;
    inputBufDesc->activeFrameRegion.topLeft.y = algDynamicParams->startY;
    inputBufDesc->activeFrameRegion.bottomRight.x = algDynamicParams->startX +
        algObj->dynamicParams.videnc2DynamicParams.inputWidth;
    inputBufDesc->activeFrameRegion.bottomRight.y = algDynamicParams->startY +
        algObj->dynamicParams.videnc2DynamicParams.inputHeight;
    /*------------------------------------------------------------------------*/
    /* Image pitch is capture width */
    /*------------------------------------------------------------------------*/
    if (algCreateParams->tilerEnable)
    {
        inputBufDesc->imagePitch[0] = VPSUTILS_TILER_CNT_8BIT_PITCH;
    }
    else
    {
        inputBufDesc->imagePitch[0] =
            algObj->dynamicParams.videnc2DynamicParams.captureWidth;
    }
    if (algCreateParams->tilerEnable)
    {
        inputBufDesc->imagePitch[1] = VPSUTILS_TILER_CNT_16BIT_PITCH;
    }
    else
    {
        inputBufDesc->imagePitch[1] =
            algObj->dynamicParams.videnc2DynamicParams.captureWidth;
    }
    /*------------------------------------------------------------------------*/
    /* Set Content type and chroma format from encoder parameters */
    /*------------------------------------------------------------------------*/
    inputBufDesc->contentType =
        algObj->staticParams.videnc2Params.inputContentType;
    inputBufDesc->chromaFormat =
        algObj->staticParams.videnc2Params.inputChromaFormat;

    /*------------------------------------------------------------------------*/
    /* Assign memory pointers adn sizes for the all the input buffers */
    /*------------------------------------------------------------------------*/
    for (i = 0; i < algObj->status.videnc2Status.bufInfo.minNumInBufs; i++)
    {
        inputBufDesc->planeDesc[i].buf = NULL;
        if (algCreateParams->tilerEnable)
        {
            if (i & 0x1)
            {
                inputBufDesc->planeDesc[i].memType = XDM_MEMTYPE_TILED16;
            }
            else
            {
                inputBufDesc->planeDesc[i].memType = XDM_MEMTYPE_TILED8;
            }
            inputBufDesc->planeDesc[i].bufSize.tileMem.width =
                algObj->status.videnc2Status.bufInfo.minInBufSize[i].tileMem.
                width;
            inputBufDesc->planeDesc[i].bufSize.tileMem.height =
                algObj->status.videnc2Status.bufInfo.minInBufSize[i].tileMem.
                height;
        }
        else
        {
            inputBufDesc->planeDesc[i].memType = XDM_MEMTYPE_RAW;
            inputBufDesc->planeDesc[i].bufSize.bytes =
                algObj->status.videnc2Status.bufInfo.minInBufSize[i].tileMem.
                width *
                algObj->status.videnc2Status.bufInfo.minInBufSize[i].tileMem.
                height;

        }
    }
    /*------------------------------------------------------------------------*/
    /* Set second field offset width and height according to input data */
    /* When second field of the input data starts at 0 it represents 2 fields 
     */
    /* are seperated and provided at 2 diff process calls (60 process call) */
    /*------------------------------------------------------------------------*/
    if ((inputBufDesc->dataLayout == IVIDEO_FIELD_SEPARATED) &&
        (algCreateParams->singleBuf == FALSE) &&
        (algObj->staticParams.videnc2Params.inputContentType ==
         IVIDEO_INTERLACED))
    {
        inputBufDesc->secondFieldOffsetHeight[0] = 0;
        inputBufDesc->secondFieldOffsetHeight[1] = 0;
        inputBufDesc->secondFieldOffsetHeight[2] = 0;
    }
    else
    {
        inputBufDesc->secondFieldOffsetHeight[0] =
            algObj->dynamicParams.videnc2DynamicParams.inputHeight;
        inputBufDesc->secondFieldOffsetHeight[1] =
            (algObj->dynamicParams.videnc2DynamicParams.inputHeight >> 1);
        inputBufDesc->secondFieldOffsetHeight[2] =
            (algObj->dynamicParams.videnc2DynamicParams.inputHeight >> 1);
    }
    inputBufDesc->secondFieldOffsetWidth[0] = 0;
    inputBufDesc->secondFieldOffsetWidth[1] = 0;
    inputBufDesc->secondFieldOffsetWidth[2] = 0;

    /*------------------------------------------------------------------------*/
    /* Set The address of unregistered user data in meta data plane desc */
    /*------------------------------------------------------------------------*/
    inputBufDesc->numMetaPlanes = 0;

    return 0;
}

static Int enclink_jpeg_set_dynamic_params(IJPEGVENC_DynamicParams *
                                           dynamicParams,
                                           EncLink_AlgDynamicParams *
                                           algDynamicParams)
{
    *dynamicParams = JPEGVENC_TI_DYNAMICPARAMS;
    dynamicParams->videnc2DynamicParams.inputWidth =
        algDynamicParams->inputWidth;
    dynamicParams->videnc2DynamicParams.inputHeight =
        algDynamicParams->inputHeight;
    dynamicParams->videnc2DynamicParams.captureWidth =
        algDynamicParams->inputPitch;
    dynamicParams->videnc2DynamicParams.targetBitRate =
        algDynamicParams->targetBitRate;
    dynamicParams->videnc2DynamicParams.targetFrameRate =
        algDynamicParams->targetFrameRate;
    dynamicParams->videnc2DynamicParams.interFrameInterval =
        algDynamicParams->interFrameInterval;
    dynamicParams->videnc2DynamicParams.intraFrameInterval =
        algDynamicParams->intraFrameInterval;
    dynamicParams->videnc2DynamicParams.mvAccuracy =
        algDynamicParams->mvAccuracy;
    dynamicParams->videnc2DynamicParams.refFrameRate =
        algDynamicParams->refFrameRate;
    dynamicParams->videnc2DynamicParams.ignoreOutbufSizeFlag = XDAS_FALSE;
    dynamicParams->qualityFactor =
        algDynamicParams->qualityFactor;

    return 0;
}

#define ENCLINKJPEG_ALGREATE_RSRC_NONE                                       (0)
#define ENCLINKJPEG_ALGREATE_RSRC_ALGCREATED                           (1 <<  0)
#define ENCLINKJPEG_ALGREATE_RSRC_IRES_ASSIGNED                        (1 <<  1)
#define ENCLINKJPEG_ALGREATE_RSRC_ALL (                                        \
                                       ENCLINKJPEG_ALGREATE_RSRC_ALGCREATED |  \
                                       ENCLINKJPEG_ALGREATE_RSRC_IRES_ASSIGNED \
                                      )

static Void enclink_jpeg_freersrc(EncLink_JPEGObj * hObj, Int rsrcMask)
{
    if (rsrcMask & ENCLINKJPEG_ALGREATE_RSRC_IRES_ASSIGNED)
    {
        IRES_Status iresStatus;

        iresStatus =
            RMAN_freeResources((IALG_Handle) hObj->algHandle,
                               &JPEGVENC_TI_IRES, hObj->scratchID);
        if (iresStatus != IRES_OK)
        {
            ENCLINK_INTERNAL_ERROR_LOG(iresStatus, "RMAN_freeResources");
        }
    }
    if (rsrcMask & ENCLINKJPEG_ALGREATE_RSRC_ALGCREATED)
    {
        enc_link_jpeg_delete(hObj->algHandle);
        hObj->algHandle = NULL;
    }
}

Int EncLinkJPEG_algCreate(EncLink_JPEGObj * hObj,
                          EncLink_AlgCreateParams * algCreateParams,
                          EncLink_AlgDynamicParams * algDynamicParams,
                          Int linkID, Int channelID, Int scratchGroupID)
{
    Int retVal = ENC_LINK_S_SUCCESS;
    Int rsrcMask = ENCLINKJPEG_ALGREATE_RSRC_NONE;
    Int algStatus;

    UTILS_assert(Utils_encdecIsJPEG(algCreateParams->format) == TRUE);
    hObj->format = algCreateParams->format;
    hObj->linkID = linkID;
    hObj->channelID = channelID;
    hObj->scratchID = scratchGroupID;

    memset(&hObj->inArgs, 0, sizeof(hObj->inArgs));
    memset(&hObj->outArgs, 0, sizeof(hObj->outArgs));
    memset(&hObj->inBufs, 0, sizeof(hObj->inBufs));
    memset(&hObj->outBufs, 0, sizeof(hObj->outBufs));
    memset(&hObj->status, 0, sizeof(hObj->status));
    memset(&hObj->memUsed, 0, sizeof(hObj->memUsed));

    hObj->status.videnc2Status.size = sizeof(IJPEGVENC_Status);
    hObj->inArgs.videnc2InArgs.size = sizeof(IJPEGVENC_InArgs);
    hObj->outArgs.videnc2OutArgs.size = sizeof(IJPEGVENC_OutArgs);
    hObj->staticParams.videnc2Params.size = sizeof(IVIDENC2_Params);
    hObj->dynamicParams.videnc2DynamicParams.size =
        sizeof(IVIDENC2_DynamicParams);

    enclink_jpeg_set_static_params(&hObj->staticParams, algCreateParams);
    enclink_jpeg_set_dynamic_params(&hObj->dynamicParams, algDynamicParams);

    UTILS_MEMLOG_USED_START();
    hObj->algHandle =
        enc_link_jpeg_create((IJPEGVENC_Fxns *) & JPEGVENC_TI_IJPEGVENC,
                             &hObj->staticParams);
    UTILS_assertError((NULL != hObj->algHandle),
                      retVal, ENC_LINK_E_ALGCREATEFAILED, linkID, channelID);
    if (!UTILS_ISERROR(retVal))
    {
        IRES_Status iresStatus;

        rsrcMask |= ENCLINKJPEG_ALGREATE_RSRC_ALGCREATED;
        iresStatus = RMAN_assignResources((IALG_Handle) hObj->algHandle,
                                          &JPEGVENC_TI_IRES, scratchGroupID);
        UTILS_assertError((iresStatus == IRES_OK), retVal,
                          ENC_LINK_E_RMANRSRCASSIGNFAILED, linkID, channelID);
    }
    if (!UTILS_ISERROR(retVal))
    {

        rsrcMask |= ENCLINKJPEG_ALGREATE_RSRC_IRES_ASSIGNED;

        hObj->status.videnc2Status.data.buf = &(hObj->versionInfo[0]);
        hObj->status.videnc2Status.data.bufSize = sizeof(hObj->versionInfo);
        algStatus = enclink_jpeg_control(hObj->algHandle, XDM_GETVERSION,
                                         &(hObj->dynamicParams),
                                         &(hObj->status));
        if (algStatus == XDM_EOK)
        {
            ENCLINK_VERBOSE_INFO_LOG(hObj->linkID, hObj->channelID,
                                     "JPEGEncCreated:%s", hObj->versionInfo);

        }
        algStatus = enclink_jpeg_control(hObj->algHandle,
                                         XDM_SETDEFAULT,
                                         &hObj->dynamicParams, &hObj->status);
        UTILS_assertError((algStatus == XDM_EOK), retVal,
                          ENC_LINK_E_ALGSETPARAMSFAILED, linkID, channelID);
    }
    if (!UTILS_ISERROR(retVal))
    {
        algStatus = enclink_jpeg_control(hObj->algHandle,
                                         XDM_SETPARAMS,
                                         &hObj->dynamicParams, &hObj->status);
        UTILS_assertError((algStatus == XDM_EOK), retVal,
                          ENC_LINK_E_ALGSETPARAMSFAILED, linkID, channelID);
    }

    if (!UTILS_ISERROR(retVal))
    {
        enclink_jpeg_control(hObj->algHandle,
                             XDM_GETSTATUS,
                             &hObj->dynamicParams, &hObj->status);
    }
    if (!UTILS_ISERROR(retVal))
    {
        algStatus =
            enclink_jpeg_control(hObj->algHandle,
                                 XDM_GETBUFINFO,
                                 &hObj->dynamicParams, &hObj->status);
        UTILS_assertError((algStatus == XDM_EOK), retVal,
                          ENC_LINK_E_ALGGETBUFINFOFAILED, linkID, channelID);
    }
    if (UTILS_ISERROR(retVal))
    {
        enclink_jpeg_freersrc(hObj, rsrcMask);
    }
    /* Initialize the Inarg, OutArg, InBuf & OutBuf objects */
    enclink_jpeg_set_algObject(hObj, algCreateParams, algDynamicParams);

    UTILS_MEMLOG_USED_END(hObj->memUsed);
    UTILS_MEMLOG_PRINT("ENCLINK_JPEG",
                       hObj->memUsed,
                       (sizeof(hObj->memUsed) / sizeof(hObj->memUsed[0])));

    return retVal;
}

Void EncLinkJPEG_algDelete(EncLink_JPEGObj * hObj)
{
    UTILS_MEMLOG_FREE_START();
    if (hObj->algHandle)
    {
        enclink_jpeg_freersrc(hObj, ENCLINKJPEG_ALGREATE_RSRC_ALL);
    }

    if (hObj->algHandle)
    {
        enc_link_jpeg_delete(hObj->algHandle);
    }
    UTILS_MEMLOG_FREE_END(hObj->memUsed, 0 /* dont care */ );
}

Int32 EncLinkJPEG_algSetConfig(EncLink_algObj * algObj)
{
    Int32 status = ENC_LINK_S_SUCCESS;
    UInt32 bitMask;
    Bool setConfigFlag = FALSE;
    UInt key;

    key = Hwi_disable();
    bitMask = algObj->setConfigBitMask;

    /* Set the modified encoder bitRate value */
    if ((bitMask >> ENC_LINK_SETCONFIG_BITMASK_BITRARATE) & 0x1)
    {

        algObj->u.jpegAlgIfObj.dynamicParams.videnc2DynamicParams.
            targetBitRate = algObj->algDynamicParams.targetBitRate;
        #ifdef SYSTEM_VERBOSE_PRINTS
        Vps_printf("\n ENCLINK: new targetbitrate to set:%d \n",
                   algObj->algDynamicParams.targetBitRate);
        #endif
        algObj->setConfigBitMask &= (ENC_LINK_SETCONFIG_BITMASK_RESET_VALUE ^
                                     (1 <<
                                      ENC_LINK_SETCONFIG_BITMASK_BITRARATE));
        setConfigFlag = TRUE;
    }

    /* Set the modified encoder Fps value */
    if ((bitMask >> ENC_LINK_SETCONFIG_BITMASK_FPS) & 0x1)
    {
        algObj->u.jpegAlgIfObj.dynamicParams.videnc2DynamicParams.
            targetFrameRate = algObj->algDynamicParams.targetFrameRate;
        algObj->u.jpegAlgIfObj.dynamicParams.videnc2DynamicParams.
            targetBitRate = algObj->algDynamicParams.targetBitRate;
        #ifdef SYSTEM_VERBOSE_PRINTS
        Vps_printf("\n ENCLINK: new targetbitrate to set:%d \n",
                   algObj->algDynamicParams.targetBitRate);
        Vps_printf("\n ENCLINK: new targetframerate to set:%d \n",
                   algObj->algDynamicParams.targetFrameRate);
        #endif
        algObj->setConfigBitMask &= (ENC_LINK_SETCONFIG_BITMASK_RESET_VALUE ^
                                     (1 << ENC_LINK_SETCONFIG_BITMASK_FPS));
        setConfigFlag = TRUE;
    }

    /* Set the modified encoder Intra Frame Interval(GOP) value */
    if ((bitMask >> ENC_LINK_SETCONFIG_BITMASK_INTRAI) & 0x1)
    {
        algObj->u.jpegAlgIfObj.dynamicParams.videnc2DynamicParams.
            intraFrameInterval = algObj->algDynamicParams.intraFrameInterval;
        #ifdef SYSTEM_VERBOSE_PRINTS
        Vps_printf("\n ENCLINK: new intraFrameInterval to set:%d \n",
                   algObj->algDynamicParams.intraFrameInterval);
        #endif
        algObj->setConfigBitMask &= (ENC_LINK_SETCONFIG_BITMASK_RESET_VALUE ^
                                     (1 << ENC_LINK_SETCONFIG_BITMASK_INTRAI));
        setConfigFlag = TRUE;
    }

    /* toggle Force IDR */
    if ((bitMask >> ENC_LINK_SETCONFIG_BITMASK_FORCEI) & 0x1)
    {

        algObj->algDynamicParams.forceFrame = TRUE;
        algObj->algDynamicParams.forceFrameStatus = FALSE;

        algObj->setConfigBitMask &= (ENC_LINK_SETCONFIG_BITMASK_RESET_VALUE ^
                                     (1 << ENC_LINK_SETCONFIG_BITMASK_FORCEI));
        setConfigFlag = TRUE;
    }

    /* Set the modified Qualityfactor value for a jpeg Frame */
    if ((bitMask >>  ENC_LINK_SETCONFIG_BITMASK_QPI) & 0x1)
    {
        algObj->u.jpegAlgIfObj.dynamicParams.qualityFactor = algObj->algDynamicParams.qpInitI;
        #ifdef SYSTEM_VERBOSE_PRINTS
        Vps_printf("\n ENCLINK: new qualityFactor Param to set:%d\n",
                algObj->algDynamicParams.qpInitI);
        #endif
        algObj->setConfigBitMask &= (ENC_LINK_SETCONFIG_BITMASK_RESET_VALUE ^
                                    (1 << ENC_LINK_SETCONFIG_BITMASK_QPI));
        setConfigFlag = TRUE;
    }

    Hwi_restore(key);

    if (setConfigFlag)
    {
        status = enclink_jpeg_control(algObj->u.jpegAlgIfObj.algHandle,
                                      XDM_SETPARAMS,
                                      &algObj->u.jpegAlgIfObj.dynamicParams,
                                      &algObj->u.jpegAlgIfObj.status);
        if (UTILS_ISERROR(status))
        {
            UTILS_warn("\n ENCLINK: ERROR in Run time parameters changes, 
			Extended Error code:%d \n", algObj->u.jpegAlgIfObj.status.videnc2Status.extendedError);
        }
        else
        {
            #ifdef SYSTEM_VERBOSE_PRINTS
            Vps_printf("\n ENCLINK: Run time parameters changed %d\n",
                       algObj->u.jpegAlgIfObj.status.videnc2Status.
                       extendedError);
            #endif
        }
    }

    return (status);
}

Int32 EncLinkJPEG_algGetConfig(EncLink_algObj * algObj)
{
    Int retVal = ENC_LINK_S_SUCCESS;
    IJPEGVENC_DynamicParams dynamicParams;
    IJPEGVENC_Status status;

    if(algObj->getConfigFlag == TRUE)
    {

        status.videnc2Status.size = sizeof(IJPEGVENC_Status);
        dynamicParams.videnc2DynamicParams.size = sizeof(IJPEGVENC_DynamicParams);

        retVal = enclink_jpeg_control(algObj->u.jpegAlgIfObj.algHandle,
                                      XDM_GETSTATUS, &dynamicParams, &status);
        if (UTILS_ISERROR(retVal))
        {
            UTILS_warn("\n ENCLINK: ERROR in Run time parameters changes, 
                  Extended Error code:%d \n", status.videnc2Status.extendedError);
        }

        algObj->getConfigFlag = FALSE;

        algObj->algDynamicParams.inputWidth =
            status.videnc2Status.encDynamicParams.inputWidth;
        algObj->algDynamicParams.inputHeight =
            status.videnc2Status.encDynamicParams.inputHeight;
        algObj->algDynamicParams.targetBitRate =
            status.videnc2Status.encDynamicParams.targetBitRate;
        algObj->algDynamicParams.targetFrameRate =
            status.videnc2Status.encDynamicParams.targetFrameRate;
        algObj->algDynamicParams.intraFrameInterval =
            status.videnc2Status.encDynamicParams.intraFrameInterval;
        algObj->algDynamicParams.forceFrame =
            status.videnc2Status.encDynamicParams.forceFrame;
        algObj->algDynamicParams.refFrameRate =
            status.videnc2Status.encDynamicParams.refFrameRate;
    }
    return (retVal);
}

Int EncLinkJPEG_algDynamicParamUpdate(EncLink_JPEGObj * hObj,
                               EncLink_AlgCreateParams * algCreateParams,
                               EncLink_AlgDynamicParams * algDynamicParams)
{
    Int retVal = ENC_LINK_S_SUCCESS;

    enclink_jpeg_set_dynamic_params(&hObj->dynamicParams, algDynamicParams);
    enclink_jpeg_set_algObject(hObj, algCreateParams, algDynamicParams);

    return (retVal);
}
