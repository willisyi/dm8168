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
#include <impeg4vdec.h>
#include <mpeg4vdec_ti.h>

#include "decLink_priv.h"
#include "decLink_err.h"
#include "decLink_mpeg4_priv.h"

#include <mcfw/src_bios6/links_m3video/codec_utils/utils_encdec.h>
#include <mcfw/src_bios6/links_m3video/codec_utils/iresman_hdvicp2_earlyacquire.h>

static IMPEG4VDEC_Handle dec_link_mpeg4_create(const IMPEG4VDEC_Fxns * fxns,
                                             const IMPEG4VDEC_Params * prms);
static Void dec_link_mpeg4_delete(IMPEG4VDEC_Handle handle);
static Int32 decLink_mpeg4_control(IMPEG4VDEC_Handle handle,
                                  IMPEG4VDEC_Cmd cmd,
                                  IMPEG4VDEC_DynamicParams * params,
                                  IMPEG4VDEC_Status * status);
static Int decLink_mpeg4_set_static_params(IMPEG4VDEC_Params * staticParams,
                                          DecLink_AlgCreateParams *
                                          algCreateParams);
static Int decLink_mpeg4_set_algObject(DecLink_MPEG4Obj * algObj,
                                      DecLink_AlgCreateParams * algCreateParams,
                                      DecLink_AlgDynamicParams *
                                      algDynamicParams);
static Int decLink_mpeg4_set_dynamic_params(IMPEG4VDEC_DynamicParams *
                                           dynamicParams,
                                           DecLink_AlgDynamicParams *
                                           algDynamicParams);
static Void decLink_mpeg4_freersrc(DecLink_MPEG4Obj * hObj, Int rsrcMask);

static Int32 DecLink_mpeg4DecoderFlush(DecLink_MPEG4Obj * hObj, Bool hardFlush);
static Int32 DecLink_mpeg4DecoderReset(DecLink_MPEG4Obj * hObj);
static Int32 DecLink_mpeg4DecoderFlushCheck(Int32);
static Int32 DecLink_mpeg4DecoderResetCheck(Int32);
static Int32 DecLink_mpeg4Decoder_checkErr(Int32, Int32);

extern IRES_Fxns MPEG4VDEC_TI_IRES;

/*
 *  ======== decLink_mpeg4_create ========
 *  Create an MPEG4VDEC instance object (using parameters specified by prms)
 */

static IMPEG4VDEC_Handle dec_link_mpeg4_create(const IMPEG4VDEC_Fxns * fxns,
                                             const IMPEG4VDEC_Params * prms)
{
    return ((IMPEG4VDEC_Handle) ALG_create((IALG_Fxns *) fxns,
                                          NULL, (IALG_Params *) prms));
}

/*
 *  ======== decLink_mpeg4_delete ========
 *  Delete the MPEG4VDEC instance object specified by handle
 */

static Void dec_link_mpeg4_delete(IMPEG4VDEC_Handle handle)
{
    ALG_delete((IALG_Handle) handle);
}


/*
 *  ======== decLink_mpeg4_control ========
 */

static Int32 decLink_mpeg4_control(IMPEG4VDEC_Handle handle,
                                  IMPEG4VDEC_Cmd cmd,
                                  IMPEG4VDEC_DynamicParams * params,
                                  IMPEG4VDEC_Status * status)
{
    int error = 0;
    IALG_Fxns *fxns = (IALG_Fxns *) handle->fxns;

    fxns->algActivate((IALG_Handle) handle);

    error = handle->fxns->ividdec3.control((IVIDDEC3_Handle) handle,
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

Int32 DecLinkMPEG4_codecFlush(DecLink_ChObj *pChObj,
                              IMPEG4VDEC_InArgs *inArgs,
                              IMPEG4VDEC_OutArgs *outArgs,
                              XDM2_BufDesc *inputBufDesc,
                              XDM2_BufDesc *outputBufDesc,
                              IMPEG4VDEC_Handle handle,
                              FVID2_FrameList *freeFrameList,
                              Bool hardFlush)
{
    Int32 freeBufIdx;
    DecLink_MPEG4Obj *hObj;
    IALG_Fxns *fxns;
    Int32 retValue=XDM_EOK;
    Int32 status = XDM_EFAIL;
    Int32 needReset = FALSE;

    hObj = &(pChObj->algObj.u.mpeg4AlgIfObj);
    fxns = (IALG_Fxns *) handle->fxns;

    needReset = 
      DecLink_mpeg4DecoderResetCheck(outArgs->viddec3OutArgs.
                                     extendedError);
    status = DecLink_mpeg4DecoderFlush(hObj, hardFlush);

    if (status == XDM_EOK)
    {
        do
        {

            #ifdef SYSTEM_DEBUG_DEC
            Vps_printf(" %d: DECODE: CH%d: Decoder Flushing !!!\n",
                Utils_getCurTimeInMsec(),
                hObj->channelID
            );
            #endif

            fxns->algActivate((IALG_Handle) handle);

            retValue = handle->fxns->ividdec3.process((IVIDDEC3_Handle) handle,
                                       inputBufDesc,
                                       outputBufDesc,
                                       (IVIDDEC3_InArgs *) inArgs,
                                       (IVIDDEC3_OutArgs *) outArgs);

            fxns->algDeactivate((IALG_Handle) handle);

            freeBufIdx = 0;
            while (outArgs->viddec3OutArgs.freeBufID[freeBufIdx] != 0)
            {
                freeFrameList->frames[freeFrameList->numFrames] =
                    (FVID2_Frame *) outArgs->viddec3OutArgs.freeBufID[freeBufIdx];
                freeFrameList->numFrames++;
                pChObj->numBufsInCodec--;
                freeBufIdx++;
            }
        /* No need to call the .process call in a loop for MPEG4, But even if 
         * you call in a loop should have break-out. We want both h264 & MPEG4
         * behave similar way, so still keep the do while loop. Remove the 
         * "&& 0" hack onec codec corrcet the behaviour 
         */
        }while((retValue == XDM_EOK) && 0 );
    }       
    if(needReset == TRUE)
    {
        DecLink_mpeg4DecoderReset(hObj);
    }

    return (needReset);
}
/*
 *  ======== Declink_mpeg4DecodeFrameBatch ========
 */
Int32 Declink_mpeg4DecodeFrameBatch(DecLink_Obj * pObj,
                                    DecLink_ReqBatch * pReqObjBatch,
                                    FVID2_FrameList * freeFrameList)
{
    int error = XDM_EFAIL, extendedError, chId;
    Bool doDisplay = TRUE;
    Int32 i, reqObjIdx, freeBufIdx, prosIdx;
    IMPEG4VDEC_InArgs *inArgs;
    IMPEG4VDEC_OutArgs *outArgs;
    XDM2_BufDesc *inputBufDesc;
    XDM2_BufDesc *outputBufDesc;
    IMPEG4VDEC_Handle handle;
    IALG_Fxns *fxns = NULL;
    DecLink_ReqObj *pReqObj = NULL;
    FVID2_Frame *outFrame = NULL;
    IVIDEO2_BufDesc *displayBufs = NULL;
    UInt32 bytesConsumed;
    DecLink_ChObj *pChObj;
    System_FrameInfo *pFrameInfo;
    Int32 needReset = FALSE;

    /*Make sure that the Req Object is not empty*/
    UTILS_assert (pReqObjBatch->numReqObjsInBatch > 0);
    
    for (reqObjIdx = 0; reqObjIdx < pReqObjBatch->numReqObjsInBatch; reqObjIdx++)
    {
        pReqObj = pReqObjBatch->pReqObj[reqObjIdx];
        chId = pReqObj->InBuf->channelNum;
        pChObj = &pObj->chObj[chId];
      
        inArgs = &pChObj->algObj.u.mpeg4AlgIfObj.inArgs;
        outArgs = &pChObj->algObj.u.mpeg4AlgIfObj.outArgs;
        inputBufDesc = &pChObj->algObj.u.mpeg4AlgIfObj.inBufs;
        outputBufDesc = &pChObj->algObj.u.mpeg4AlgIfObj.outBufs;
        handle = pChObj->algObj.u.mpeg4AlgIfObj.algHandle;
      
        UTILS_assert(handle != NULL);
      
        fxns = (IALG_Fxns *) handle->fxns;
      
        bytesConsumed = 0;

        for (prosIdx=0; prosIdx<pReqObj->OutFrameList.numFrames; prosIdx++)
        {
            /*-------------------------------------------------------------------*/
            /* Initialize the input ID in input arguments to the bufferid of     */
            /* buffer element returned from getfreebuffer() function.            */
            /*-------------------------------------------------------------------*/
            /* inputID & numBytes need to update before every decode call        */


            if (FALSE == outArgs->viddec3OutArgs.outBufsInUseFlag)
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

            inArgs->viddec3InArgs.inputID = (UInt32) outFrame;
            inArgs->viddec3InArgs.numBytes = pReqObj->InBuf->fillLength -
                                                             bytesConsumed;

            for (i = 0; i < inputBufDesc->numBufs; i++)
            {
                /* Set proper buffer addresses for bitstreamn data */
                /*---------------------------------------------------------------*/
                inputBufDesc->descs[i].buf = (XDAS_Int8 *) pReqObj->InBuf->addr +
                                                           bytesConsumed;
                inputBufDesc->descs[i].bufSize.bytes = pReqObj->InBuf->bufSize;
            }

            for (i = 0; i < outputBufDesc->numBufs; i++)
            {
                /* Set proper buffer addresses for Frame data */
                /*---------------------------------------------------------------*/
                if (pChObj->algObj.algCreateParams.tilerEnable)
                {
                    outputBufDesc->descs[i].buf =
                        (Ptr)Utils_tilerAddr2CpuAddr((UInt32) (outFrame->addr[0][i]));
                }
                else
                {
                    outputBufDesc->descs[i].buf = outFrame->addr[0][i];
                }
            }
            fxns->algActivate((IALG_Handle) handle);
            error = handle->fxns->ividdec3.process((IVIDDEC3_Handle) handle,
                                                   inputBufDesc,
                                                   outputBufDesc,
                                                   (IVIDDEC3_InArgs *) inArgs,
                                                   (IVIDDEC3_OutArgs *) outArgs);
            fxns->algDeactivate((IALG_Handle) handle);
            doDisplay = TRUE;
            bytesConsumed = outArgs->viddec3OutArgs.bytesConsumed;
            extendedError = outArgs->viddec3OutArgs.extendedError;

            if(outArgs->viddec3OutArgs.extendedError)
            {

                #ifdef SYSTEM_DEBUG_DEC
                if (DecLink_mpeg4Decoder_checkErr(outArgs->viddec3OutArgs.
                                                  extendedError, 
                                                  IMPEG4D_ERR_PICSIZECHANGE))
                {
                Vps_printf ("DEC_LINK: CH%d decoder changing resolution \n", chId);
                } 
                else
                {
                DECLINK_INTERNAL_ERROR_LOG(error, "ALGPROCESS FAILED:STATUS");
                Vps_printf ("outArgs->viddec3OutArgs.extendedError for channel %d Error: 0x%x\n",
                             chId, outArgs->viddec3OutArgs.extendedError);
                }
                #endif

                doDisplay = FALSE;
                freeBufIdx = 0;

                while (outArgs->viddec3OutArgs.freeBufID[freeBufIdx] != 0)
                {
                    if ((DecLink_mpeg4Decoder_checkErr(outArgs->viddec3OutArgs.
                                                       extendedError, 
                                                       IMPEG4D_ERR_PICSIZECHANGE)) 
                                                       && (outArgs->viddec3OutArgs.
                                                          freeBufID[freeBufIdx] 
                                                          == (UInt32) outFrame))
                    {
                        freeBufIdx++;
                    } 
                    else
                    {
                        freeFrameList->frames[freeFrameList->numFrames] =
                            (FVID2_Frame *) outArgs->viddec3OutArgs.freeBufID[freeBufIdx];
                        freeFrameList->numFrames++;
                        pChObj->numBufsInCodec--;
                        freeBufIdx++;
                    }
                }
#ifndef DEC_LINK_SUPRESS_ERROR_AND_RESET

                needReset = DecLinkMPEG4_codecFlush(pChObj, inArgs, 
                                                    outArgs, inputBufDesc,
                                                    outputBufDesc, handle, 
                                                    freeFrameList, FALSE);
                pReqObj->OutFrameList.frames[prosIdx] = NULL;

                if (DecLink_mpeg4Decoder_checkErr(extendedError,
                                                 IMPEG4D_ERR_PICSIZECHANGE))
                {
                    if(needReset != TRUE)
                    {
                        inArgs->viddec3InArgs.numBytes = 
                                pReqObj->InBuf->fillLength - bytesConsumed;
                        for (i = 0; i < inputBufDesc->numBufs; i++)
                        {
                            /* Set proper buffer addresses for bitstreamn */
                            /* data                                       */
                            /*--------------------------------------------*/
                            inputBufDesc->descs[i].buf = 
                                          (XDAS_Int8 *) pReqObj->InBuf->addr 
                                           + bytesConsumed;
                            inputBufDesc->descs[i].bufSize.bytes = pReqObj->InBuf->bufSize;
                        }
                    }
                    
                    fxns->algActivate((IALG_Handle) handle);
                    error = handle->fxns->ividdec3.process((IVIDDEC3_Handle) 
                                                           handle,
                                                           inputBufDesc,
                                                           outputBufDesc,
                                                           (IVIDDEC3_InArgs *) 
                                                           inArgs,
                                                           (IVIDDEC3_OutArgs *) 
                                                           outArgs);
                    fxns->algDeactivate((IALG_Handle) handle);
                    doDisplay = TRUE;
                    if (error != XDM_EOK)
                    {
                        DECLINK_INTERNAL_ERROR_LOG(error, "ALGPROCESS FAILED:STATUS");
                        Vps_printf ("outArgs->viddec3OutArgs.extendedError "
                                    "for channel %d with Resolution chnage Error: 0x%x\n",
                                     chId, outArgs->viddec3OutArgs.extendedError);
                    }
                }
#endif
            }

            if(doDisplay == TRUE)
            {
                pChObj->algObj.prevOutFrame = outFrame;
                pReqObj->OutFrameList.frames[prosIdx] = NULL;
                UTILS_assert(outArgs->viddec3OutArgs.displayBufsMode ==
                             IVIDDEC3_DISPLAYBUFS_EMBEDDED);
                displayBufs = &(outArgs->viddec3OutArgs.displayBufs.bufDesc[0]);
                if ((outArgs->viddec3OutArgs.outputID[0] != 0)
                    && (displayBufs->numPlanes))
                {
                    XDAS_Int8 *pExpectedBuf;

                    pReqObj->OutFrameList.frames[prosIdx] =
                        (FVID2_Frame *) outArgs->viddec3OutArgs.outputID[0];
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
                while (outArgs->viddec3OutArgs.freeBufID[freeBufIdx] != 0)
                {
                    freeFrameList->frames[freeFrameList->numFrames] =
                        (FVID2_Frame *) outArgs->viddec3OutArgs.freeBufID[freeBufIdx];
                    freeFrameList->numFrames++;
                    pChObj->numBufsInCodec--;
                    freeBufIdx++;
                }
            }
        }
    }
    return (error);
}

static Int decLink_mpeg4_set_static_params(IMPEG4VDEC_Params * staticParams,
                                          DecLink_AlgCreateParams *
                                          algCreateParams)
{
    /* Initialize default values for static params */
    *staticParams = IMPEG4VDEC_PARAMS;

    /* Both width & height needs to be align with 16 bytes */
    staticParams->viddec3Params.maxHeight =
                  VpsUtils_align(algCreateParams->maxHeight, 16);

    staticParams->viddec3Params.maxWidth =
                  VpsUtils_align(algCreateParams->maxWidth, 16);

    staticParams->viddec3Params.displayDelay = algCreateParams->displayDelay;
    return 0;
}

static Int decLink_mpeg4_set_algObject(DecLink_MPEG4Obj * algObj,
                                      DecLink_AlgCreateParams * algCreateParams,
                                      DecLink_AlgDynamicParams *
                                      algDynamicParams)
{
    UInt32 bufCnt;
    IMPEG4VDEC_InArgs *inArgs;
    IMPEG4VDEC_OutArgs *outArgs;
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
    inArgs->viddec3InArgs.inputID = 0;
    inArgs->viddec3InArgs.numBytes = 0;

    /*------------------------------------------------------------------------*/
    /* The outBufsInUseFlag tells us whether the previous input buffer given */
    /* by the application to the algorithm is still in use or not. Since */
    /* this is before the first decode call, assign this flag to 0. The */
    /* algorithm will take care to initialize this flag appropriately from */
    /* hereon for the current configuration.  */
    /*------------------------------------------------------------------------*/
    outArgs->viddec3OutArgs.outBufsInUseFlag = 0;
    outArgs->viddec3OutArgs.bytesConsumed = 0;
    outArgs->viddec3OutArgs.freeBufID[0] = 0;
    outArgs->viddec3OutArgs.outputID[0] = 0;
    outArgs->viddec3OutArgs.extendedError = 0;
    outArgs->viddec3OutArgs.displayBufsMode = IVIDDEC3_DISPLAYBUFS_EMBEDDED;
    memset(&outArgs->viddec3OutArgs.displayBufs.bufDesc, 0,
           sizeof(outArgs->viddec3OutArgs.displayBufs.bufDesc));
    outArgs->viddec3OutArgs.displayBufs.pBufDesc[0] = NULL;
    outArgs->viddec3OutArgs.decodedBufs.contentType = IVIDEO_PROGRESSIVE_FRAME;
    outArgs->viddec3OutArgs.decodedBufs.frameType = IVIDEO_I_FRAME;
    outArgs->viddec3OutArgs.decodedBufs.extendedError = 0;

    /*------------------------------------------------------------------------*/
    /* Initialize the input buffer properties as required by algorithm */
    /* based on info received by preceding GETBUFINFO call. First init the */
    /* number of input bufs.  */
    /*------------------------------------------------------------------------*/
    inputBufDesc->numBufs = algObj->status.viddec3Status.bufInfo.minNumInBufs;
    /*------------------------------------------------------------------------*/
    /* For the num of input bufs, initialize the buffer pointer addresses */
    /* and buffer sizes.  */
    /*------------------------------------------------------------------------*/
    inputBufDesc->descs[0].buf = NULL;
    inputBufDesc->descs[0].bufSize.bytes = 0;
    inputBufDesc->descs[0].memType =
        algObj->status.viddec3Status.bufInfo.inBufMemoryType[0];
    inputBufDesc->descs[0].memType = XDM_MEMTYPE_RAW;

    outputBufDesc->numBufs = algObj->status.viddec3Status.bufInfo.minNumOutBufs;
    for (bufCnt = 0; bufCnt < outputBufDesc->numBufs; bufCnt++)
    {
        outputBufDesc->descs[bufCnt].buf = NULL;
        outputBufDesc->descs[bufCnt].memType =
            algObj->status.viddec3Status.bufInfo.outBufMemoryType[bufCnt];
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
                algObj->status.viddec3Status.bufInfo.minOutBufSize[bufCnt].
                tileMem.width;
            outputBufDesc->descs[bufCnt].bufSize.tileMem.height =
                algObj->status.viddec3Status.bufInfo.minOutBufSize[bufCnt].
                tileMem.height;
        }
        else
        {
            outputBufDesc->descs[bufCnt].bufSize.bytes =
                (algObj->status.viddec3Status.bufInfo.minOutBufSize[bufCnt].
                tileMem.width *
                algObj->status.viddec3Status.bufInfo.minOutBufSize[bufCnt].
                tileMem.height);
        }
    }

    return 0;
}

static Int decLink_mpeg4_set_dynamic_params(IMPEG4VDEC_DynamicParams *
                                           dynamicParams,
                                           DecLink_AlgDynamicParams *
                                           algDynamicParams)
{
    *dynamicParams = IMPEG4VDEC_TI_DYNAMICPARAMS;

    dynamicParams->viddec3DynamicParams.decodeHeader =
        algDynamicParams->decodeHeader;
    dynamicParams->viddec3DynamicParams.displayWidth =
        algDynamicParams->displayWidth;
    dynamicParams->viddec3DynamicParams.frameSkipMode =
        algDynamicParams->frameSkipMode;
    dynamicParams->viddec3DynamicParams.newFrameFlag =
        algDynamicParams->newFrameFlag;

    return DEC_LINK_S_SUCCESS;
}

#define DECLINKMPEG4_ALGREATE_RSRC_NONE                                       (0)
#define DECLINKMPEG4_ALGREATE_RSRC_ALGCREATED                           (1 <<  0)
#define DECLINKMPEG4_ALGREATE_RSRC_IRES_ASSIGNED                        (1 <<  1)
#define DECLINKMPEG4_ALGREATE_RSRC_ALL (                                        \
                                       DECLINKMPEG4_ALGREATE_RSRC_ALGCREATED |  \
                                       DECLINKMPEG4_ALGREATE_RSRC_IRES_ASSIGNED \
                                       )

static Void decLink_mpeg4_freersrc(DecLink_MPEG4Obj * hObj, Int rsrcMask)
{
    if (rsrcMask & DECLINKMPEG4_ALGREATE_RSRC_IRES_ASSIGNED)
    {
        IRES_Status iresStatus;

        iresStatus =
            RMAN_freeResources((IALG_Handle) hObj->algHandle,
                               &MPEG4VDEC_TI_IRES, hObj->scratchID);
        if (iresStatus != IRES_OK)
        {
            DECLINK_INTERNAL_ERROR_LOG(iresStatus, "RMAN_freeResources FAILED");
        }
    }
    if (rsrcMask & DECLINKMPEG4_ALGREATE_RSRC_ALGCREATED)
    {
        dec_link_mpeg4_delete(hObj->algHandle);
        hObj->algHandle = NULL;
    }
}

Int DecLinkMPEG4_algCreate(DecLink_MPEG4Obj * hObj,
                           DecLink_AlgCreateParams * algCreateParams,
                           DecLink_AlgDynamicParams * algDynamicParams,
                           Int linkID, Int channelID, Int scratchGroupID,
                           FVID2_Format *pFormat, UInt32 numFrames,
                           IRES_ResourceDescriptor resDesc[])
{
    Int retVal = DEC_LINK_S_SUCCESS;
    Int rsrcMask = DECLINKMPEG4_ALGREATE_RSRC_NONE;

    UTILS_assert(Utils_encdecIsMPEG4(algCreateParams->format) == TRUE);
    hObj->linkID = linkID;
    hObj->channelID = channelID;
    hObj->scratchID = scratchGroupID;

    memset(&hObj->inArgs, 0, sizeof(hObj->inArgs));
    memset(&hObj->outArgs, 0, sizeof(hObj->outArgs));
    memset(&hObj->inBufs, 0, sizeof(hObj->inBufs));
    memset(&hObj->outBufs, 0, sizeof(hObj->outBufs));
    memset(&hObj->status, 0, sizeof(hObj->status));
    memset(&hObj->memUsed, 0, sizeof(hObj->memUsed));

    hObj->staticParams.viddec3Params.size = sizeof(IMPEG4VDEC_Params);
    hObj->status.viddec3Status.size = sizeof(IMPEG4VDEC_Status);
    hObj->dynamicParams.viddec3DynamicParams.size =
        sizeof(IMPEG4VDEC_DynamicParams);
    hObj->inArgs.viddec3InArgs.size = sizeof(IMPEG4VDEC_InArgs);
    hObj->outArgs.viddec3OutArgs.size = sizeof(IMPEG4VDEC_OutArgs);

    decLink_mpeg4_set_static_params(&hObj->staticParams, algCreateParams);
    decLink_mpeg4_set_dynamic_params(&hObj->dynamicParams, algDynamicParams);

    UTILS_MEMLOG_USED_START();
    hObj->algHandle =
        dec_link_mpeg4_create((IMPEG4VDEC_Fxns *) & MPEG4VDEC_TI_IMPEG4VDEC,
                             &hObj->staticParams);
    UTILS_assertError((NULL != hObj->algHandle),
                      retVal, DEC_LINK_E_ALGCREATEFAILED, linkID, channelID);

    if (!UTILS_ISERROR(retVal))
    {
        Int32 status = UTILS_ENCDEC_S_SUCCESS;
        if (algCreateParams->tilerEnable == FALSE)
        {
            status = Utils_encdec_checkResourceAvail((IALG_Handle) hObj->algHandle,
                           &MPEG4VDEC_TI_IRES, pFormat, numFrames, resDesc);
        }
        UTILS_assertError((status == UTILS_ENCDEC_S_SUCCESS), retVal,
                          DEC_LINK_E_RMANRSRCASSIGNFAILED, linkID, channelID);
    }
    if (!UTILS_ISERROR(retVal))
    {
        IRES_Status iresStatus;

        rsrcMask |= DECLINKMPEG4_ALGREATE_RSRC_ALGCREATED;
        iresStatus = RMAN_assignResources((IALG_Handle) hObj->algHandle,
                                          &MPEG4VDEC_TI_IRES, scratchGroupID);
        UTILS_assertError((iresStatus == IRES_OK), retVal,
                          DEC_LINK_E_RMANRSRCASSIGNFAILED, linkID, channelID);
    }
    if (!UTILS_ISERROR(retVal))
    {
        Int algStatus;

        rsrcMask |= DECLINKMPEG4_ALGREATE_RSRC_IRES_ASSIGNED;

        hObj->status.viddec3Status.data.buf = &(hObj->versionInfo[0]);
        hObj->status.viddec3Status.data.bufSize = sizeof(hObj->versionInfo);
        algStatus = decLink_mpeg4_control(hObj->algHandle, XDM_GETVERSION,
                                         &(hObj->dynamicParams),
                                         &(hObj->status));
        if (algStatus == XDM_EOK)
        {
            DECLINK_INFO_LOG_VERBOSE(hObj->linkID, hObj->channelID,
                                     "MPEG4DecCreated:%s", hObj->versionInfo);
        }

        algStatus = decLink_mpeg4_control(hObj->algHandle, XDM_GETBUFINFO,
                                         &(hObj->dynamicParams),
                                         &(hObj->status));
        if (algStatus == XDM_EOK)
        {
            DECLINK_INFO_LOG_VERBOSE(hObj->linkID, hObj->channelID,
                                     "XDM_GETBUFINFO done");
        }

        algStatus = decLink_mpeg4_control(hObj->algHandle,
                                         XDM_SETPARAMS,
                                         &hObj->dynamicParams, &hObj->status);
        UTILS_assertError((algStatus == XDM_EOK), retVal,
                          DEC_LINK_E_ALGSETPARAMSFAILED, linkID, channelID);
    }
    if (!UTILS_ISERROR(retVal))
    {
        decLink_mpeg4_control(hObj->algHandle,
                             XDM_GETSTATUS,
                             &hObj->dynamicParams, &hObj->status);
    }
    if (UTILS_ISERROR(retVal))
    {
        decLink_mpeg4_freersrc(hObj, rsrcMask);
    }
    else
    {
        /* Initialize the Inarg, OutArg, InBuf & OutBuf objects */
        decLink_mpeg4_set_algObject(hObj, algCreateParams, algDynamicParams);
    }

    UTILS_MEMLOG_USED_END(hObj->memUsed);
    UTILS_MEMLOG_PRINT("DECLINK_MPEG4",
						hObj->memUsed, 
					    (sizeof(hObj->memUsed) / sizeof(hObj->memUsed[0])));
    return retVal;
}

Void DecLinkMPEG4_algDelete(DecLink_MPEG4Obj * hObj)
{
    UTILS_MEMLOG_FREE_START();
    if (hObj->algHandle)
    {
        decLink_mpeg4_freersrc(hObj, DECLINKMPEG4_ALGREATE_RSRC_ALL);
    }

    if (hObj->algHandle)
    {
        dec_link_mpeg4_delete(hObj->algHandle);
    }
    UTILS_MEMLOG_FREE_END(hObj->memUsed, 0 /* dont care */ );

}

static Int32 DecLink_mpeg4DecoderFlushCheck(Int32 errorCode)
{

  /*----------------------------------------------------------------------*/
  /* Under certain error conditions, the application need to stop decoding*/
  /* the current stream and do an XDM_FLUSH which enables the codec to    */
  /* flush (display and free up) the frames locked by it. The following   */
  /* error conditions fall in this category.                              */
  /*----------------------------------------------------------------------*/
   if((DecLink_mpeg4Decoder_checkErr(errorCode, IMPEG4D_ERR_STREAM_END)) ||
     (DecLink_mpeg4Decoder_checkErr(errorCode, IMPEG4D_ERR_PICSIZECHANGE)) ||
     (DecLink_mpeg4Decoder_checkErr(errorCode, IMPEG4D_ERR_UNSUPPRESOLUTION)) ||
     (DecLink_mpeg4Decoder_checkErr(errorCode, IMPEG4D_ERR_VOP_TIME_INCREMENT_RES_ZERO))
	)
   {
     return TRUE;
   }
   else
   {
     return FALSE;
   }
}

static Int32 DecLink_mpeg4Decoder_checkErr(Int32 errMsg, Int32 errVal)
{
  if(errMsg & (1 << errVal))
  {
    return TRUE;
  }
  else
  {
    return FALSE;
  }
}

static Int32 DecLink_mpeg4DecoderFlush(DecLink_MPEG4Obj * hObj, Bool hardFlush)
{
    int error = XDM_EFAIL;

    IMPEG4VDEC_Handle handle;
    IMPEG4VDEC_OutArgs *outArgs;
    IALG_Fxns *fxns = NULL;
    Int32 doFlush;

    outArgs = &hObj->outArgs;
    handle = hObj->algHandle;
    fxns = (IALG_Fxns *) handle->fxns;

    /* Check if XDM_FLUSH is required or not. */
    doFlush = DecLink_mpeg4DecoderFlushCheck(outArgs->viddec3OutArgs.extendedError);

    if(doFlush || hardFlush)
    {
        #ifdef SYSTEM_DEBUG_DEC
        Vps_printf(" %d: DECODE: CH%d: Decoder flush needed (%d)!!!\n",
            Utils_getCurTimeInMsec(),
            hObj->channelID,
            outArgs->viddec3OutArgs.extendedError
        );
        #endif

       fxns->algActivate((IALG_Handle) handle);

       error = decLink_mpeg4_control(handle,
                                   XDM_FLUSH,
                                   &(hObj->dynamicParams),
                                   &(hObj->status));

       fxns->algDeactivate((IALG_Handle) handle);

    }

    return (error);
}

static Int32 DecLink_mpeg4DecoderResetCheck(Int32 errorCode)
{
   Int32 reset;
   if(DecLink_mpeg4Decoder_checkErr(errorCode, IMPEG4D_ERR_STREAM_END))
     reset = TRUE;
   else if(DecLink_mpeg4Decoder_checkErr(errorCode, IMPEG4D_ERR_UNSUPPRESOLUTION))
     reset = TRUE;
   else if(DecLink_mpeg4Decoder_checkErr(errorCode, IMPEG4D_ERR_PICSIZECHANGE))
     reset = TRUE;
   else
     reset = FALSE;

   return reset;
}

static Int32 DecLink_mpeg4DecoderReset(DecLink_MPEG4Obj * hObj)
{
    int error;

    IMPEG4VDEC_Handle handle;
    IALG_Fxns *fxns = NULL;

    handle = hObj->algHandle;
    fxns = (IALG_Fxns *) handle->fxns;

    fxns->algActivate((IALG_Handle) handle);
    error = decLink_mpeg4_control(handle,
                               XDM_RESET,
                               &(hObj->dynamicParams),
                               &(hObj->status));

    fxns->algDeactivate((IALG_Handle) handle);

    return (error);
}

