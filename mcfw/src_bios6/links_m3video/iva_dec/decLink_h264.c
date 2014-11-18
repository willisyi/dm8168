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
#include <ih264vdec.h>
#include <h264vdec_ti.h>
#include <mcfw/interfaces/common_def/ti_vsys_common_def.h>

#include "decLink_priv.h"
#include "decLink_err.h"

#include <mcfw/src_bios6/links_m3video/codec_utils/utils_encdec.h>
#include <mcfw/src_bios6/links_m3video/codec_utils/iresman_hdvicp2_earlyacquire.h>

static IH264VDEC_Handle dec_link_h264_create(const IH264VDEC_Fxns * fxns,
                                             const IH264VDEC_Params * prms);
static Void dec_link_h264_delete(IH264VDEC_Handle handle);
static Int32 decLink_h264_control(IH264VDEC_Handle handle,
                                  IH264VDEC_Cmd cmd,
                                  IH264VDEC_DynamicParams * params,
                                  IH264VDEC_Status * status);
static Int decLink_h264_set_static_params(IH264VDEC_Params * staticParams,
                                          DecLink_AlgCreateParams *
                                          algCreateParams);
static Int decLink_h264_set_algObject(DecLink_H264Obj * algObj,
                                      DecLink_AlgCreateParams * algCreateParams,
                                      DecLink_AlgDynamicParams *
                                      algDynamicParams);
static Int decLink_h264_set_dynamic_params(IH264VDEC_DynamicParams *
                                           dynamicParams,
                                           DecLink_AlgDynamicParams *
                                           algDynamicParams);
static Void decLink_h264_freersrc(DecLink_H264Obj * hObj, Int rsrcMask);

static Int32 DecLink_h264DecoderFlush(DecLink_H264Obj * hObj, Bool hardFlush);
static Int32 DecLink_h264DecoderReset(DecLink_H264Obj * hObj);
static Int32 DecLink_h264DecoderFlushCheck(Int32);
static Int32 DecLink_h264DecoderResetCheck(Int32);
static Int32 DecLink_h264Decoder_checkErr(Int32, Int32);

extern IRES_Fxns H264VDEC_TI_IRES;


/*
 *  ======== decLink_h264_create ========
 *  Create an H264VDEC instance object (using parameters specified by prms)
 */

static IH264VDEC_Handle dec_link_h264_create(const IH264VDEC_Fxns * fxns,
                                             const IH264VDEC_Params * prms)
{
    return ((IH264VDEC_Handle) ALG_create((IALG_Fxns *) fxns,
                                          NULL, (IALG_Params *) prms));
}

/*
 *  ======== decLink_h264_delete ========
 *  Delete the H264VDEC instance object specified by handle
 */

static Void dec_link_h264_delete(IH264VDEC_Handle handle)
{
    ALG_delete((IALG_Handle) handle);
}

/*
 *  ======== decLink_h264_control ========
 */

static Int32 decLink_h264_control(IH264VDEC_Handle handle,
                                  IH264VDEC_Cmd cmd,
                                  IH264VDEC_DynamicParams * params,
                                  IH264VDEC_Status * status)
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

static Void DecLink_codecFrameDebugAddInBuf(DecLink_ChObj *pChObj,FVID2_Frame *inFrame)
{
    if (pChObj->frameDebug.inBufIdx >= UTILS_ARRAYSIZE(pChObj->frameDebug.inBuf))
        pChObj->frameDebug.inBufIdx = 0;
    pChObj->frameDebug.inBuf[pChObj->frameDebug.inBufIdx] = inFrame;
    pChObj->frameDebug.inBufIdx++;
}

static Void DecLink_codecFrameDebugAddOutBuf(DecLink_ChObj *pChObj,FVID2_Frame *outFrame)
{
    if (pChObj->frameDebug.outBufIdx >= UTILS_ARRAYSIZE(pChObj->frameDebug.outBuf))
        pChObj->frameDebug.outBufIdx = 0;
    pChObj->frameDebug.outBuf[pChObj->frameDebug.outBufIdx] = outFrame;
    pChObj->frameDebug.outBufIdx++;
}

Int32 DecLinkH264_codecFlush(DecLink_ChObj *pChObj,
                             IH264VDEC_InArgs *inArgs,
                             IH264VDEC_OutArgs *outArgs,
                             XDM2_BufDesc *inputBufDesc,
                             XDM2_BufDesc *outputBufDesc,
                             IH264VDEC_Handle handle,
                             FVID2_FrameList *freeFrameList,
                             Bool hardFlush)
{
    Int32 freeBufIdx;
    DecLink_H264Obj *hObj;
    IALG_Fxns *fxns;
    Int32 retValue=XDM_EOK;
    Int32 status = XDM_EFAIL;
    Int32 needReset = FALSE;

    hObj = &(pChObj->algObj.u.h264AlgIfObj);
    fxns = (IALG_Fxns *) handle->fxns;

    needReset = 
      DecLink_h264DecoderResetCheck(outArgs->viddec3OutArgs.
                                    extendedError);
    status = DecLink_h264DecoderFlush(hObj, hardFlush);

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

             retValue = handle->fxns->ividdec3.process((IVIDDEC3_Handle) 
                                                       handle,
                                                       inputBufDesc,
                                                       outputBufDesc,
                                                       (IVIDDEC3_InArgs *) 
                                                       inArgs,
                                                       (IVIDDEC3_OutArgs *) 
                                                       outArgs);

              fxns->algDeactivate((IALG_Handle) handle);

              freeBufIdx = 0;
              while (outArgs->viddec3OutArgs.freeBufID[freeBufIdx] != 0)
              {
                  freeFrameList->frames[freeFrameList->numFrames] =
                      (FVID2_Frame *) outArgs->viddec3OutArgs.
                                      freeBufID[freeBufIdx];
                  DecLink_codecFrameDebugAddOutBuf(pChObj,
                          freeFrameList->frames[freeFrameList->numFrames]);
                  freeFrameList->numFrames++;
                  pChObj->numBufsInCodec--;
                  freeBufIdx++;
              }

        } while(retValue == XDM_EOK);
    }       
    if(needReset == TRUE)
    {
        DecLink_h264DecoderReset(hObj);
    }
    
    return (needReset);
}

/*
 *  ======== Declink_h264DecodeFrame ========
 */
Int32 Declink_h264DecodeFrameBatch(DecLink_Obj * pObj,
                                   DecLink_ReqBatch * pReqObjBatch,
                                   FVID2_FrameList * freeFrameList,
								                   Int32 tskId)
{
    Int32 multiError = XDM_EFAIL, error, extendedError;
    Bool doDisplay = TRUE;
    Int32 i, reqObjIdx, freeBufIdx, prosIdx, chId;
    IH264VDEC_InArgs *inArgs;
    IH264VDEC_OutArgs *outArgs;
    XDM2_BufDesc *inputBufDesc;
    XDM2_BufDesc *outputBufDesc;
    IH264VDEC_Handle handle = NULL;
    IALG_Fxns *fxns = NULL;
    DecLink_ReqObj *pReqObj = NULL;
    FVID2_Frame *outFrame = NULL;
    IVIDEO2_BufDesc *displayBufs = NULL;
    UInt32 bytesConsumed;
    System_FrameInfo *pFrameInfo;
    IH264VDEC_ProcessParamsList processList;
    DecLink_ChObj *pChObj;
    Int32 needReset = FALSE;

    processList.numEntries = 0;
    
    /*Make sure that the Req Object is not empty*/
    UTILS_assert (pReqObjBatch->numReqObjsInBatch > 0);

    for (reqObjIdx = 0; reqObjIdx < pReqObjBatch->numReqObjsInBatch; reqObjIdx++)
    {
        pReqObj = pReqObjBatch->pReqObj[reqObjIdx];
        chId = pReqObj->InBuf->channelNum;
        
        pChObj = &pObj->chObj[chId];
        
        inArgs = &pChObj->algObj.u.h264AlgIfObj.inArgs;
        outArgs = &pChObj->algObj.u.h264AlgIfObj.outArgs;
        inputBufDesc = &pChObj->algObj.u.h264AlgIfObj.inBufs;
        outputBufDesc = &pChObj->algObj.u.h264AlgIfObj.outBufs;
        handle = pChObj->algObj.u.h264AlgIfObj.algHandle;

         bytesConsumed = 0;
        
        /*In the N-channel decoder scheme, field merged mode is not supported yet*/
         /*Support to be added!!*/
        UTILS_assert (pReqObj->OutFrameList.numFrames < 2);
        pChObj->algObj.u.h264AlgIfObj.numProcessCalls++;
        for (prosIdx = 0; prosIdx < pReqObj->OutFrameList.numFrames; prosIdx++)
        {
            /*-------------------------------------------------------------------*/
            /* Initialize the input ID in input arguments to the bufferid of */
            /* buffer element returned from getfreebuffer() function.  */
            /*-------------------------------------------------------------------*/
            /* inputID & numBytes need to update before every decode call */
            

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
                DecLink_codecFrameDebugAddOutBuf(pChObj,freeFrameList->frames[freeFrameList->numFrames]);
                freeFrameList->numFrames++;
                pChObj->numBufsInCodec--;
            }

            inArgs->viddec3InArgs.inputID = (UInt32) outFrame;
            DecLink_codecFrameDebugAddInBuf(pChObj,outFrame);
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
                        (Ptr) Utils_tilerAddr2CpuAddr((UInt32) (outFrame->addr[0][i]));
                }
                else
                {
                    outputBufDesc->descs[i].buf = outFrame->addr[0][i];
                }
            }
      
      
        }
        
        processList.processParams[processList.numEntries].handle = handle;
        processList.processParams[processList.numEntries].inBufs = inputBufDesc;
        processList.processParams[processList.numEntries].outBufs = outputBufDesc;
        processList.processParams[processList.numEntries].inArgs = 
                                                      (IVIDDEC3_InArgs *)inArgs;
        processList.processParams[processList.numEntries].outArgs = 
                                                      (IVIDDEC3_OutArgs *)outArgs;
        processList.numEntries++;       

      
    }
    
    if (handle != NULL)
    {
        multiError = handle->fxns->processMulti(&processList);
    }
    else
    {
        UTILS_assert (FALSE);
    }

    UTILS_assert (pReqObjBatch->numReqObjsInBatch == processList.numEntries);

    for (reqObjIdx = 0; reqObjIdx < pReqObjBatch->numReqObjsInBatch; reqObjIdx++)
    {
        pReqObj = pReqObjBatch->pReqObj[reqObjIdx];
        chId = pReqObj->InBuf->channelNum;
        pChObj = &pObj->chObj[chId];
        /*ProcessList is created in the same order as ReqObjBatch */
        inArgs = (IH264VDEC_InArgs *) processList.processParams[reqObjIdx].inArgs;
        outArgs = (IH264VDEC_OutArgs *)processList.processParams[reqObjIdx].outArgs;
        inputBufDesc = processList.processParams[reqObjIdx].inBufs;
        outputBufDesc = processList.processParams[reqObjIdx].outBufs;
        handle = processList.processParams[reqObjIdx].handle;
        
        UTILS_assert(handle != NULL);
        
        fxns = (IALG_Fxns *) handle->fxns;
       
        outFrame = (FVID2_Frame *) inArgs->viddec3InArgs.inputID;
        
        if (pReqObj->InBuf->inputFileChanged == TRUE)
        {
            pChObj->isFirstIDRFrameFound = FALSE;
            pReqObj->InBuf->inputFileChanged = FALSE;
        }
        UTILS_assert(outArgs->viddec3OutArgs.displayBufsMode ==
                              IVIDDEC3_DISPLAYBUFS_EMBEDDED);
        displayBufs = &(outArgs->viddec3OutArgs.displayBufs.bufDesc[0]);
        if ((pChObj->isFirstIDRFrameFound == FALSE) &&
            (!outArgs->viddec3OutArgs.extendedError) &&
            ((displayBufs->frameType == IVIDEO_IDR_FRAME) ||
             (displayBufs->frameType == IVIDEO_I_FRAME)))
        {
            pChObj->isFirstIDRFrameFound = TRUE;
        }
        
        for (prosIdx = 0; prosIdx < pReqObj->OutFrameList.numFrames; prosIdx++)
        {

            doDisplay = TRUE;
            bytesConsumed = outArgs->viddec3OutArgs.bytesConsumed;
            extendedError = outArgs->viddec3OutArgs.extendedError;

            if(((outArgs->viddec3OutArgs.extendedError)&&
                (!DecLink_h264Decoder_checkErr(extendedError, IH264VDEC_ERR_GAPSINFRAMENUM))&&
                (!DecLink_h264Decoder_checkErr(extendedError, IH264VDEC_ERR_INVALIDPARAM_IGNORE)))
               ||
               (pChObj->isFirstIDRFrameFound == FALSE))
            {   
                
                #ifdef SYSTEM_DEBUG_DEC
                if (DecLink_h264Decoder_checkErr(outArgs->viddec3OutArgs.
                                                 extendedError, 
                                                 IH264VDEC_ERR_PICSIZECHANGE))
                {
                Vps_printf ("DEC_LINK: CH%d decoder changing resolution \n", chId);
                } 
                else
                {
                DECLINK_INTERNAL_ERROR_LOG(multiError, "ALGPROCESS FAILED:STATUS");
                Vps_printf ("outArgs->viddec3OutArgs.extendedError for channel %d Error: 0x%x\n",
                             chId, outArgs->viddec3OutArgs.extendedError);
                Vps_printf ("Sequence called number %d\n", pChObj->algObj.u.h264AlgIfObj.numProcessCalls);
                }
                #endif
                
                pChObj->decErrorMsg.errorMsg = extendedError;
                pChObj->decErrorMsg.chId = chId;
                if(pChObj->decErrorMsg.reportA8 == TRUE)
                {
                      System_linkControl(
                                         SYSTEM_LINK_ID_HOST, 
                                         VSYS_EVENT_DECODER_ERROR, 
                                         &(pChObj->decErrorMsg),
                                         sizeof(DecLink_ChErrorMsg), 
                                         FALSE);
                      pChObj->decErrorMsg.reportA8 = FALSE;
                }
                /* Workaround to fix a decode bug - WA start
                   Some error cases the decoder is putting the same free
                   buffer info in multiple freeBufIdx.  This bug will make 
                   Dec link crash. This WA to avoid such condition.
                   Remove this WA once the decoder fix this issue.
                */
                for (i = 0; i < 10; i++)
                {
                    freeBufIdx = i + 1;
                    while (outArgs->viddec3OutArgs.freeBufID[freeBufIdx] != 0)
                    {
                        if (outArgs->viddec3OutArgs.freeBufID[freeBufIdx] == 
                            outArgs->viddec3OutArgs.freeBufID[i])
                        {
                           outArgs->viddec3OutArgs.freeBufID[freeBufIdx] = 0;
                        }
                        freeBufIdx++;
                    }
                }
                /* Workaround to fix a decode bug - WA End */

                doDisplay = FALSE;
                freeBufIdx = 0;
                
                while (outArgs->viddec3OutArgs.freeBufID[freeBufIdx] != 0)
                {
                    if ((DecLink_h264Decoder_checkErr(outArgs->viddec3OutArgs.
                                                      extendedError, 
                                                      IH264VDEC_ERR_PICSIZECHANGE)) 
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
                        DecLink_codecFrameDebugAddOutBuf(pChObj,freeFrameList->frames[freeFrameList->numFrames]);
                        freeFrameList->numFrames++;
                        pChObj->numBufsInCodec--;
                        freeBufIdx++;
                    }
                }
#ifndef DEC_LINK_SUPRESS_ERROR_AND_RESET

                needReset = DecLinkH264_codecFlush(pChObj, inArgs, 
                                                   outArgs, inputBufDesc,
                                                   outputBufDesc, handle, 
                                                   freeFrameList, FALSE);
                pReqObj->OutFrameList.frames[prosIdx] = NULL;

                if (DecLink_h264Decoder_checkErr(extendedError,
                                                 IH264VDEC_ERR_PICSIZECHANGE))
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
                    displayBufs = &(outArgs->viddec3OutArgs.displayBufs.bufDesc[0]);
                    if ((pChObj->isFirstIDRFrameFound == FALSE) &&
                        (!outArgs->viddec3OutArgs.extendedError) &&
                        ((displayBufs->frameType == IVIDEO_IDR_FRAME) ||
                         (displayBufs->frameType == IVIDEO_I_FRAME)))
                    {
                        pChObj->isFirstIDRFrameFound = TRUE;
                    }
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
                    DecLink_codecFrameDebugAddOutBuf(pChObj,freeFrameList->frames[freeFrameList->numFrames]);
                    freeFrameList->numFrames++;
                    pChObj->numBufsInCodec--;
                    freeBufIdx++;                   
                }
            }          
        }  
    }
    return (multiError);
}

static XDAS_Int32 decLink_h264_map_dpbsize2codecparam(Int32 dpbSizeInFrames)
{
    XDAS_Int32 h264VdecDPBSizeInFrames;

    if(DEC_LINK_DPB_SIZE_IN_FRAMES_DEFAULT == dpbSizeInFrames)
    {
        h264VdecDPBSizeInFrames = IH264VDEC_DPB_NUMFRAMES_DEFAULT;
    }
    else
    {
        if ((dpbSizeInFrames >= IH264VDEC_DPB_NUMFRAMES_0)
            &&
            (dpbSizeInFrames <= IH264VDEC_DPB_NUMFRAMES_16))
        {
            h264VdecDPBSizeInFrames =  dpbSizeInFrames;
        }
        else
        {
           Vps_printf("%d:DECLINK: Invalid param passed for DPBBufSize param [%d] "
                      "Forcing to default value [%d]",
                      dpbSizeInFrames,
                      IH264VDEC_DPB_NUMFRAMES_DEFAULT);
           h264VdecDPBSizeInFrames =  IH264VDEC_DPB_NUMFRAMES_DEFAULT;
        }
    }
    return h264VdecDPBSizeInFrames;
}

static Int decLink_h264_set_static_params(IH264VDEC_Params * staticParams,
                                          DecLink_AlgCreateParams *
                                          algCreateParams)
{
    /* Initialize default values for static params */
    *staticParams = IH264VDEC_PARAMS;

    staticParams->viddec3Params.size = sizeof(IH264VDEC_Params);
    /* Both width & height needs to be align with 16 bytes */
    staticParams->viddec3Params.maxHeight =
                  VpsUtils_align(algCreateParams->maxHeight, 16);

    staticParams->viddec3Params.maxWidth =
                  VpsUtils_align(algCreateParams->maxWidth, 16);

    staticParams->presetProfileIdc = algCreateParams->presetProfile;

    staticParams->presetLevelIdc = algCreateParams->presetLevel;
    
    if (TRUE == algCreateParams->fieldMergeDecodeEnable)
    {
        staticParams->processCallLevel = IH264VDEC_FRAMELEVELPROCESSCALL;
    }
    else
    {
        staticParams->processCallLevel = IH264VDEC_FIELDLEVELPROCESSCALL;
    }

    staticParams->viddec3Params.displayDelay = algCreateParams->displayDelay;
    staticParams->dpbSizeInFrames =
        decLink_h264_map_dpbsize2codecparam(algCreateParams->dpbBufSizeInFrames);

    /* Enabling debug logging inside the codec. Details in appendix H in H.264 
     * decoder user guide.
     */
    staticParams->debugTraceLevel = 0;
    staticParams->lastNFramesToLog = 31;
    
    return 0;
}

static Int decLink_h264_set_algObject(DecLink_H264Obj * algObj,
                                      DecLink_AlgCreateParams * algCreateParams,
                                      DecLink_AlgDynamicParams *
                                      algDynamicParams)
{
    UInt32 bufCnt;
    IH264VDEC_InArgs *inArgs;
    IH264VDEC_OutArgs *outArgs;
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
    
    algObj->numProcessCalls = 0;
    
    return 0;
}

static Int decLink_h264_set_dynamic_params(IH264VDEC_DynamicParams *
                                           dynamicParams,
                                           DecLink_AlgDynamicParams *
                                           algDynamicParams)
{
    *dynamicParams = IH264VDEC_TI_DYNAMICPARAMS;

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

#define DECLINKH264_ALGREATE_RSRC_NONE                                       (0)
#define DECLINKH264_ALGREATE_RSRC_ALGCREATED                           (1 <<  0)
#define DECLINKH264_ALGREATE_RSRC_IRES_ASSIGNED                        (1 <<  1)
#define DECLINKH264_ALGREATE_RSRC_ALL (                                        \
                                       DECLINKH264_ALGREATE_RSRC_ALGCREATED |  \
                                       DECLINKH264_ALGREATE_RSRC_IRES_ASSIGNED \
                                      )

static Void decLink_h264_freersrc(DecLink_H264Obj * hObj, Int rsrcMask)
{
    if (rsrcMask & DECLINKH264_ALGREATE_RSRC_IRES_ASSIGNED)
    {
        IRES_Status iresStatus;

        iresStatus =
            RMAN_freeResources((IALG_Handle) hObj->algHandle,
                               &H264VDEC_TI_IRES, hObj->scratchID);
        if (iresStatus != IRES_OK)
        {
            DECLINK_INTERNAL_ERROR_LOG(iresStatus, "RMAN_freeResources FAILED");
        }
    }
    if (rsrcMask & DECLINKH264_ALGREATE_RSRC_ALGCREATED)
    {
        dec_link_h264_delete(hObj->algHandle);
        hObj->algHandle = NULL;
    }
}

Int DecLinkH264_algCreate(DecLink_H264Obj * hObj,
                          DecLink_AlgCreateParams * algCreateParams,
                          DecLink_AlgDynamicParams * algDynamicParams,
                          Int linkID, Int channelID, Int scratchGroupID,
                          FVID2_Format *pFormat, UInt32 numFrames,
                          IRES_ResourceDescriptor resDesc[])
{
    Int retVal = DEC_LINK_S_SUCCESS;
    Int rsrcMask = DECLINKH264_ALGREATE_RSRC_NONE;

    UTILS_assert(Utils_encdecIsH264(algCreateParams->format) == TRUE);
    hObj->linkID = linkID;
    hObj->channelID = channelID;
    hObj->scratchID = scratchGroupID;

    memset(&hObj->inArgs, 0, sizeof(hObj->inArgs));
    memset(&hObj->outArgs, 0, sizeof(hObj->outArgs));
    memset(&hObj->inBufs, 0, sizeof(hObj->inBufs));
    memset(&hObj->outBufs, 0, sizeof(hObj->outBufs));
    memset(&hObj->status, 0, sizeof(hObj->status));
    memset(&hObj->memUsed, 0, sizeof(hObj->memUsed));

    hObj->staticParams.viddec3Params.size = sizeof(IH264VDEC_Params);
    hObj->status.viddec3Status.size = sizeof(IH264VDEC_Status);
    hObj->dynamicParams.viddec3DynamicParams.size =
        sizeof(IH264VDEC_DynamicParams);
    hObj->inArgs.viddec3InArgs.size = sizeof(IH264VDEC_InArgs);
    hObj->outArgs.viddec3OutArgs.size = sizeof(IH264VDEC_OutArgs);

    decLink_h264_set_static_params(&hObj->staticParams, algCreateParams);
    decLink_h264_set_dynamic_params(&hObj->dynamicParams, algDynamicParams);

    UTILS_MEMLOG_USED_START();
    hObj->algHandle =
        dec_link_h264_create((IH264VDEC_Fxns *) & H264VDEC_TI_IH264VDEC_MULTI,
                             &hObj->staticParams);
    UTILS_assertError((NULL != hObj->algHandle),
                      retVal, DEC_LINK_E_ALGCREATEFAILED, linkID, channelID);

    if (!UTILS_ISERROR(retVal))
    {
        Int32 status = UTILS_ENCDEC_S_SUCCESS;
        if (algCreateParams->tilerEnable == FALSE)
        {
            status = Utils_encdec_checkResourceAvail((IALG_Handle) hObj->algHandle,
                           &H264VDEC_TI_IRES, pFormat, numFrames, resDesc);
        }
        UTILS_assertError((status == UTILS_ENCDEC_S_SUCCESS), retVal,
                          DEC_LINK_E_RMANRSRCASSIGNFAILED, linkID, channelID);
    }
    if (!UTILS_ISERROR(retVal))
    {
        IRES_Status iresStatus;

        rsrcMask |= DECLINKH264_ALGREATE_RSRC_ALGCREATED;
        iresStatus = RMAN_assignResources((IALG_Handle) hObj->algHandle,
                                          &H264VDEC_TI_IRES, scratchGroupID);
        UTILS_assertError((iresStatus == IRES_OK), retVal,
                          DEC_LINK_E_RMANRSRCASSIGNFAILED, linkID, channelID);
    }
    if (!UTILS_ISERROR(retVal))
    {
        Int algStatus;

        rsrcMask |= DECLINKH264_ALGREATE_RSRC_IRES_ASSIGNED;

        hObj->status.viddec3Status.data.buf = &(hObj->versionInfo[0]);
        hObj->status.viddec3Status.data.bufSize = sizeof(hObj->versionInfo);
        algStatus = decLink_h264_control(hObj->algHandle, XDM_GETVERSION,
                                         &(hObj->dynamicParams),
                                         &(hObj->status));
        if (algStatus == XDM_EOK)
        {
            DECLINK_INFO_LOG_VERBOSE(hObj->linkID, hObj->channelID,
                                     "H264DecCreated:%s", hObj->versionInfo);
        }

        algStatus = decLink_h264_control(hObj->algHandle, XDM_GETBUFINFO,
                                         &(hObj->dynamicParams),
                                         &(hObj->status));
        if (algStatus == XDM_EOK)
        {
            DECLINK_INFO_LOG_VERBOSE(hObj->linkID, hObj->channelID,
                                     "XDM_GETBUFINFO done");
        }

        algStatus = decLink_h264_control(hObj->algHandle,
                                         XDM_SETPARAMS,
                                         &hObj->dynamicParams, &hObj->status);
        UTILS_assertError((algStatus == XDM_EOK), retVal,
                          DEC_LINK_E_ALGSETPARAMSFAILED, linkID, channelID);
    }
    if (!UTILS_ISERROR(retVal))
    {
        decLink_h264_control(hObj->algHandle,
                             XDM_GETSTATUS,
                             &hObj->dynamicParams, &hObj->status);
    }
    if (UTILS_ISERROR(retVal))
    {
        decLink_h264_freersrc(hObj, rsrcMask);
    } 
    else
    {
        /* Initialize the Inarg, OutArg, InBuf & OutBuf objects */
        decLink_h264_set_algObject(hObj, algCreateParams, algDynamicParams);
    }

    UTILS_MEMLOG_USED_END(hObj->memUsed);
    UTILS_MEMLOG_PRINT("DECLINK_H264",
                       hObj->memUsed,
                       (sizeof(hObj->memUsed) / sizeof(hObj->memUsed[0])));
    return retVal;
}

Void DecLinkH264_algDelete(DecLink_H264Obj * hObj)
{
    UTILS_MEMLOG_FREE_START();
    if (hObj->algHandle)
    {
        decLink_h264_freersrc(hObj, DECLINKH264_ALGREATE_RSRC_ALL);
    }

    if (hObj->algHandle)
    {
        dec_link_h264_delete(hObj->algHandle);
    }
    UTILS_MEMLOG_FREE_END(hObj->memUsed, 0 /* dont care */ );

}

static Int32 DecLink_h264DecoderFlushCheck(Int32 errorCode)
{

  /*----------------------------------------------------------------------*/
  /* Under certain error conditions, the application need to stop decoding*/
  /* the current stream and do an XDM_FLUSH which enables the codec to    */
  /* flush (display and free up) the frames locked by it. The following   */
  /* error conditions fall in this category.                              */
  /*----------------------------------------------------------------------*/
   if((DecLink_h264Decoder_checkErr(errorCode, IH264VDEC_ERR_STREAM_END)) ||
     (DecLink_h264Decoder_checkErr(errorCode, IH264VDEC_ERR_PICSIZECHANGE)) ||
     (DecLink_h264Decoder_checkErr(errorCode, IH264VDEC_ERR_UNSUPPRESOLUTION)) ||
     (DecLink_h264Decoder_checkErr(errorCode, IH264VDEC_ERR_NUMREF_FRAMES)) ||
     (DecLink_h264Decoder_checkErr(errorCode, IH264VDEC_ERR_DATA_SYNC)) ||
     (DecLink_h264Decoder_checkErr(errorCode, IH264VDEC_ERR_DISPLAYWIDTH)))
   {
     return TRUE;
   }
   else
   {
     return FALSE;
   }
}

static Int32 DecLink_h264Decoder_checkErr(Int32 errMsg, Int32 errVal)
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

static Int32 DecLink_h264DecoderFlush(DecLink_H264Obj * hObj, Bool hardFlush)
{
    int error = XDM_EFAIL;

    IH264VDEC_Handle handle;
    IH264VDEC_OutArgs *outArgs;
    IALG_Fxns *fxns = NULL;
    Int32 doFlush;

    outArgs = &hObj->outArgs;
    handle = hObj->algHandle;
    fxns = (IALG_Fxns *) handle->fxns;

    /* Check if XDM_FLUSH is required or not. */
    doFlush = DecLink_h264DecoderFlushCheck(outArgs->viddec3OutArgs.extendedError);

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

       error = decLink_h264_control(handle,
                                   XDM_FLUSH,
                                   &(hObj->dynamicParams),
                                   &(hObj->status));

       fxns->algDeactivate((IALG_Handle) handle);

    }

    return (error);
}

static Int32 DecLink_h264DecoderResetCheck(Int32 errorCode)
{
   Int32 reset;
   if(DecLink_h264Decoder_checkErr(errorCode, IH264VDEC_ERR_DATA_SYNC))
     reset = TRUE;
   else if(DecLink_h264Decoder_checkErr(errorCode, IH264VDEC_ERR_NUMREF_FRAMES))
     reset = TRUE;
   else if(DecLink_h264Decoder_checkErr(errorCode, IH264VDEC_ERR_UNSUPPRESOLUTION))
     reset = TRUE;
   else
     reset = FALSE;

   return reset;
}

static Int32 DecLink_h264DecoderReset(DecLink_H264Obj * hObj)
{
    int error;

    IH264VDEC_Handle handle;
    IALG_Fxns *fxns = NULL;

    handle = hObj->algHandle;
    fxns = (IALG_Fxns *) handle->fxns;

    fxns->algActivate((IALG_Handle) handle);
    error = decLink_h264_control(handle,
                               XDM_RESET,
                               &(hObj->dynamicParams),
                               &(hObj->status));

    fxns->algDeactivate((IALG_Handle) handle);

    return (error);
}

