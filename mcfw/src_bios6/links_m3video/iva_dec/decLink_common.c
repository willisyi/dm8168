/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include <stdlib.h>
#include <xdc/std.h>
#include <xdc/runtime/Error.h>
#include <ti/sysbios/hal/Hwi.h>
#include <ti/sysbios/knl/Task.h>
#include <mcfw/src_bios6/utils/utils.h>
#include <mcfw/src_bios6/utils/utils_mem.h>
#include <mcfw/src_bios6/links_m3video/codec_utils/utils_encdec.h>
#include <mcfw/interfaces/link_api/system_tiler.h>
#include "decLink_priv.h"
#include <mcfw/src_bios6/links_m3video/codec_utils/hdvicp2_config.h>

#define DEC_LINK_HDVICP2_EARLY_ACQUIRE_ENABLE

static Int32 DecLink_codecCreateReqObj(DecLink_Obj * pObj);
static Int32 DecLink_codecCreateOutObjCommon(DecLink_Obj * pObj);
static Int32 DecLink_codecMapCh2ResolutionPool(DecLink_Obj * pObj, UInt32 chId);
static Int32 DecLink_codecMapCh2ProcessTskId(DecLink_Obj * pObj, UInt32 chId);
static Int32 DecLink_codecCreateOutChObj(DecLink_Obj * pObj, UInt32 chId);
static Int32 DecLink_codecCreateChObj(DecLink_Obj * pObj, UInt32 chId);
static Int32 DecLink_codecCreateDecObj(DecLink_Obj * pObj, UInt32 chId);
static Int32 DecLink_codecCreateChannel(DecLink_Obj * pObj, UInt32 chId);
static Int32 DecLink_codecDeleteChannel(DecLink_Obj * pObj, UInt32 chId);
static Void  DecLink_codecProcessTskFxn(UArg arg1, UArg arg2);
static Int32 DecLink_codecCreateProcessTsk(DecLink_Obj * pObj, UInt32 tskId);
static Int32 DecLink_codecDeleteProcessTsk(DecLink_Obj * pObj, UInt32 tskId);
static Int32 DecLink_codecQueueBufsToChQue(DecLink_Obj * pObj);
static Int32 DecLink_codecSubmitData(DecLink_Obj * pObj);
static Int32 DecLink_codecGetProcessedData(DecLink_Obj * pObj);
static Int32 decLink_map_displayDelay2CodecParam(Int32 displayDelay);
static Int32 DecLink_codecFlushNDeleteChannel(DecLink_Obj * pObj, UInt32 chId);
static Int32 DecLink_dupFrame(DecLink_Obj * pObj, FVID2_Frame * pOrgFrame,
                              FVID2_Frame ** ppDupFrame);
static Int32 DecLink_PrepareBatch (DecLink_Obj *pObj, UInt32 tskId, 
                                   DecLink_ReqObj *pReqObj, 
                                   DecLink_ReqBatch *pReqObjBatch);

static Int32 DecLink_codecCreateIvaMapMutex(DecLink_Obj * pObj);
static Int32 DecLink_codecDeleteIvaMapMutex(DecLink_Obj * pObj);
static Int32 DecLink_codecCreateIvaSwitchSerializerObj(DecLink_Obj * pObj);
static Int32 DecLink_codecDeleteIvaSwitchSerializerObj(DecLink_Obj * pObj);
static Void DecLink_codecRegisterIVAMapCb(DecLink_Obj * pObj);
static Void DecLink_codecUnregisterIVAMapCb(DecLink_Obj * pObj);
static Int32 DecLink_codecCreateReqObjDummy(DecLink_Obj * pObj);
static Int32 DecLink_codecDeleteReqObjDummy(DecLink_Obj * pObj);

DecLink_ChCreateParams DECLINK_DEFAULTCHCREATEPARAMS_H264 = {
    .format = IVIDEO_H264HP,
    .profile = IH264VDEC_PROFILE_ANY,
    .fieldMergeDecodeEnable = FALSE,
    .targetMaxWidth = UTILS_ENCDEC_RESOLUTION_CLASS_D1_WIDTH,
    .targetMaxHeight = UTILS_ENCDEC_RESOLUTION_CLASS_D1_HEIGHT,
    .displayDelay = DEC_LINK_DEFAULT_ALGPARAMS_DISPLAYDELAY,
    .processCallLevel = IH264VDEC_FIELDLEVELPROCESSCALL,
    .numBufPerCh = DEC_LINK_NUM_BUFFERS_PER_CHANNEL,   
    .dpbBufSizeInFrames = DEC_LINK_DPB_SIZE_IN_FRAMES_DEFAULT,
    .defaultDynamicParams = {
                             .targetFrameRate =
                             DEC_LINK_DEFAULT_ALGPARAMS_TARGETFRAMERATE,
                             .targetBitRate =
                             DEC_LINK_DEFAULT_ALGPARAMS_TARGETBITRATE,
                             }
};

DecLink_ChCreateParams DECLINK_DEFAULTCHCREATEPARAMS_MPEG4 = {
    .format = IVIDEO_MPEG4ASP,
    .profile = NULL,
    .fieldMergeDecodeEnable = TRUE,
    .targetMaxWidth = UTILS_ENCDEC_RESOLUTION_CLASS_D1_WIDTH,
    .targetMaxHeight = UTILS_ENCDEC_RESOLUTION_CLASS_D1_HEIGHT,
    .displayDelay = DEC_LINK_DEFAULT_ALGPARAMS_DISPLAYDELAY,
    .numBufPerCh = DEC_LINK_NUM_BUFFERS_PER_CHANNEL,
    .defaultDynamicParams = {
                             .targetFrameRate = NULL,
                             .targetBitRate = NULL,
                             }
};

DecLink_ChCreateParams DECLINK_DEFAULTCHCREATEPARAMS_JPEG = {
    .format = IVIDEO_MJPEG,
    .profile = NULL,
    .fieldMergeDecodeEnable = TRUE,
    .targetMaxWidth = UTILS_ENCDEC_RESOLUTION_CLASS_D1_WIDTH,
    .targetMaxHeight = UTILS_ENCDEC_RESOLUTION_CLASS_D1_HEIGHT,
    .displayDelay = DEC_LINK_DEFAULT_ALGPARAMS_DISPLAYDELAY,
    .numBufPerCh = DEC_LINK_NUM_BUFFERS_PER_CHANNEL,    
    .dpbBufSizeInFrames = DEC_LINK_DPB_SIZE_IN_FRAMES_DEFAULT,
    .defaultDynamicParams = {
                             .targetFrameRate = NULL,
                             .targetBitRate = NULL,
                             }
};


static Int32 DecLink_codecCreateReqObj(DecLink_Obj * pObj)
{
    Int32 status;
    UInt32 reqId;

    memset(pObj->reqObj, 0, sizeof(pObj->reqObj));

    status = Utils_queCreate(&pObj->reqQue,
                             (DEC_LINK_MAX_REQ),
                             pObj->reqQueMem, UTILS_QUE_FLAG_BLOCK_QUE_GET);
    UTILS_assert(status == FVID2_SOK);

    pObj->isReqPend = FALSE;

    for (reqId = 0; reqId < DEC_LINK_MAX_REQ; reqId++)
    {
        status =
            Utils_quePut(&pObj->reqQue, &pObj->reqObj[reqId], BIOS_NO_WAIT);
        UTILS_assert(status == FVID2_SOK);
    }

    return FVID2_SOK;
}

static Int32 DecLink_codecCreateDupObj(DecLink_Obj * pObj)
{
    Int32 status;
    Int i;

    memset(&pObj->dupObj, 0, sizeof(pObj->dupObj));
    status = Utils_queCreate(&pObj->dupObj.dupQue,
                             UTILS_ARRAYSIZE(pObj->dupObj.dupQueMem),
                             pObj->dupObj.dupQueMem,
                             UTILS_QUE_FLAG_BLOCK_QUE_GET);
    UTILS_assertError(!UTILS_ISERROR(status),
                      status,
                      DEC_LINK_E_DUPOBJ_CREATE_FAILED, pObj->linkId, -1);
    if (!UTILS_ISERROR(status))
    {
        for (i = 0; i < DEC_LINK_MAX_DUP_FRAMES; i++)
        {
            pObj->dupObj.frameInfo[i].pVdecOrgFrame = NULL;
            pObj->dupObj.frameInfo[i].vdecRefCount = 0;
            pObj->dupObj.dupFrameMem[i].appData = &(pObj->dupObj.frameInfo[i]);
            status = Utils_quePut(&pObj->dupObj.dupQue,
                                  &pObj->dupObj.dupFrameMem[i], BIOS_NO_WAIT);
            UTILS_assert(!UTILS_ISERROR(status));
        }
    }
    return status;
}

static Int32 DecLink_codecDeleteDupObj(DecLink_Obj * pObj)
{
    Int32 status;

    UTILS_assertError((Utils_queIsFull(&pObj->dupObj.dupQue) == TRUE),
                      status,
                      DEC_LINK_E_DUPOBJ_DELETE_FAILED, pObj->linkId, -1);
    status = Utils_queDelete(&pObj->dupObj.dupQue);
    UTILS_assertError(!UTILS_ISERROR(status),
                      status,
                      DEC_LINK_E_DUPOBJ_DELETE_FAILED, pObj->linkId, -1);
    return status;
}

static Void DecLink_codecPrdCalloutFcn(UArg arg)
{
    DecLink_Obj *pObj = (DecLink_Obj *) arg;
    Int32 status;

    status = System_sendLinkCmd(pObj->linkId, DEC_LINK_CMD_GET_PROCESSED_DATA);


    if (UTILS_ISERROR(status))
    {
        #ifdef SYSTEM_DEBUG_CMD_ERROR
        UTILS_warn("DECLINK:[%s:%d]:"
                   "System_sendLinkCmd DEC_LINK_CMD_GET_PROCESSED_DATA failed"
                   "errCode = %d", __FILE__, __LINE__, status);
        #endif
    }

}

static Int32 DecLink_codecCreatePrdObj(DecLink_Obj * pObj)
{
    Clock_Params clockParams;

    Clock_Params_init(&clockParams);
    clockParams.arg = (UArg) pObj;
    UTILS_assert(pObj->periodicObj.clkHandle == NULL);

    Clock_construct(&(pObj->periodicObj.clkStruct), DecLink_codecPrdCalloutFcn,
                    1, &clockParams);
    pObj->periodicObj.clkHandle = Clock_handle(&pObj->periodicObj.clkStruct);
    pObj->periodicObj.clkStarted = FALSE;

    return DEC_LINK_S_SUCCESS;

}

static Int32 DecLink_codecDeletePrdObj(DecLink_Obj * pObj)
{
    /* Stop the clock */
    Clock_stop(pObj->periodicObj.clkHandle);
    Clock_destruct(&(pObj->periodicObj.clkStruct));
    pObj->periodicObj.clkHandle = NULL;
    pObj->periodicObj.clkStarted = FALSE;

    return DEC_LINK_S_SUCCESS;
}

static Int32 DecLink_codecStartPrdObj(DecLink_Obj * pObj, UInt period)
{

    UTILS_assert(pObj->periodicObj.clkHandle != NULL);

    if (FALSE == pObj->periodicObj.clkStarted)
    {
        Clock_setPeriod(pObj->periodicObj.clkHandle, period);
        Clock_setTimeout(pObj->periodicObj.clkHandle, period);
        Clock_start(pObj->periodicObj.clkHandle);
        pObj->periodicObj.clkStarted = TRUE;
    }

    return DEC_LINK_S_SUCCESS;
}

static Int32 DecLink_codecStopPrdObj(DecLink_Obj * pObj)
{

    UTILS_assert(pObj->periodicObj.clkHandle != NULL);

    if (TRUE == pObj->periodicObj.clkStarted)
    {
        Clock_stop(pObj->periodicObj.clkHandle);
        pObj->periodicObj.clkStarted = FALSE;
    }
    return DEC_LINK_S_SUCCESS;
}

static Int32 DecLink_codecCreateOutObjCommon(DecLink_Obj * pObj)
{
    Int32 status;
    Int32 outId, chId, frameId;
    DecLink_OutObj *pOutObj;
    System_FrameInfo *pFrameInfo;

    pObj->outObj.totalNumOutBufs = 0;
    pObj->info.numQue = DEC_LINK_MAX_OUT_QUE;

    for (outId = 0u; outId < DEC_LINK_MAX_OUT_QUE; outId++)
    {
        pObj->info.queInfo[outId].numCh = pObj->inQueInfo.numCh;
    }

    status = Utils_queCreate(&pObj->processDoneQue,
                             DEC_LINK_MAX_OUT_FRAMES,
                             pObj->processDoneQueMem,
                             (UTILS_QUE_FLAG_BLOCK_QUE_GET |
                              UTILS_QUE_FLAG_BLOCK_QUE_PUT));
    UTILS_assert(status == FVID2_SOK);

    pOutObj = &pObj->outObj;
    status = Utils_bufCreateExt(&pOutObj->bufOutQue, TRUE, FALSE,
                                pObj->inQueInfo.numCh);
    UTILS_assert(status == FVID2_SOK);

    for (chId = 0; chId < pObj->inQueInfo.numCh; chId++)
    {
        status = Utils_queCreate(&pOutObj->outChObj[chId].fvidFrmQue,
                                 DEC_LINK_MAX_NUM_OUT_BUF_PER_CH*2,
                                 pOutObj->outChObj[chId].fvidFrmQueMem,
                                 UTILS_QUE_FLAG_NO_BLOCK_QUE);
        UTILS_assert(status == FVID2_SOK);

        for (frameId = 0; frameId < DEC_LINK_MAX_NUM_OUT_BUF_PER_CH*2; frameId++)
        {

            pFrameInfo = (System_FrameInfo *)
                          &pOutObj->outChObj[chId].frameInfo[frameId];
            UTILS_assert(pFrameInfo != NULL);
            pFrameInfo->allocPoolID = chId;
            pFrameInfo->invalidFrame = FALSE;
            pFrameInfo->rtChInfoUpdate = FALSE;
            pOutObj->outChObj[chId].outFramesPool[frameId].appData = pFrameInfo;
            status = Utils_quePut(&pOutObj->outChObj[chId].fvidFrmQue, 
                &pOutObj->outChObj[chId].outFramesPool[frameId], BIOS_NO_WAIT);
            UTILS_assert(status == FVID2_SOK);
        }
    }

    return (status);
}

static Int32 DecLink_codecPopulateOutFrmFormat(DecLink_Obj *pObj, UInt32 chId)
{
    DecLink_OutObj *pOutObj;
    Int32 status = DEC_LINK_S_SUCCESS;
    FVID2_Format *pFormat;

    pOutObj = &pObj->outObj;
    pOutObj->outChObj[chId].outNumFrames = 
                            pObj->createArgs.chCreateParams[chId].numBufPerCh;
    UTILS_assert(pOutObj->outChObj[chId].outNumFrames <= DEC_LINK_MAX_OUT_FRAMES);
    UTILS_assert(pOutObj->outChObj[chId].outNumFrames <= DEC_LINK_MAX_NUM_OUT_BUF_PER_CH);

    pFormat = &pOutObj->outChObj[chId].outFormat;

    pFormat->channelNum = 0;
    pFormat->dataFormat = FVID2_DF_YUV420SP_UV;
    pFormat->fieldMerged[0] = FALSE;
    pFormat->fieldMerged[1] = FALSE;
    pFormat->fieldMerged[2] = FALSE;

    switch (pOutObj->outChObj[chId].reslutionClass)
    {
        /* Modify this with formula to calculate the single buffer
         * size, if possible */
        case UTILS_ENCDEC_RESOLUTION_CLASS_CIF:       // CIF
            pFormat->width =
                UTILS_ENCDEC_GET_PADDED_WIDTH
                (UTILS_ENCDEC_RESOLUTION_CLASS_CIF_WIDTH);
            pFormat->height =
                UTILS_ENCDEC_GET_PADDED_HEIGHT
                (UTILS_ENCDEC_RESOLUTION_CLASS_CIF_HEIGHT);
            pFormat->pitch[0] =
                VpsUtils_align(pFormat->width, VPS_BUFFER_ALIGNMENT);
            break;
        case UTILS_ENCDEC_RESOLUTION_CLASS_D1:        // D1
            pFormat->width =
                UTILS_ENCDEC_GET_PADDED_WIDTH
                (UTILS_ENCDEC_RESOLUTION_CLASS_D1_WIDTH);
            pFormat->height =
                UTILS_ENCDEC_GET_PADDED_HEIGHT
                (UTILS_ENCDEC_RESOLUTION_CLASS_D1_HEIGHT);
            pFormat->pitch[0] =
                VpsUtils_align(pFormat->width, VPS_BUFFER_ALIGNMENT);
            break;
        case UTILS_ENCDEC_RESOLUTION_CLASS_720P:      // 720p
            pFormat->width =
                UTILS_ENCDEC_GET_PADDED_WIDTH
                (UTILS_ENCDEC_RESOLUTION_CLASS_720P_WIDTH);
            pFormat->height =
                UTILS_ENCDEC_GET_PADDED_HEIGHT
                (UTILS_ENCDEC_RESOLUTION_CLASS_720P_HEIGHT);
            pFormat->pitch[0] =
                VpsUtils_align(pFormat->width, VPS_BUFFER_ALIGNMENT);
            break;
        case UTILS_ENCDEC_RESOLUTION_CLASS_1080P:     // 1080p
            pFormat->width =
                UTILS_ENCDEC_GET_PADDED_WIDTH
                (UTILS_ENCDEC_RESOLUTION_CLASS_1080P_WIDTH);
            pFormat->height =
                UTILS_ENCDEC_GET_PADDED_HEIGHT
                (UTILS_ENCDEC_RESOLUTION_CLASS_1080P_HEIGHT);
            pFormat->pitch[0] =
                VpsUtils_align(pFormat->width, VPS_BUFFER_ALIGNMENT);
            break;
        case UTILS_ENCDEC_RESOLUTION_CLASS_WUXGA:     // WUXGA
            pFormat->width =
                UTILS_ENCDEC_GET_PADDED_WIDTH
                (UTILS_ENCDEC_RESOLUTION_CLASS_WUXGA_WIDTH);
            pFormat->height =
                UTILS_ENCDEC_GET_PADDED_HEIGHT
                (UTILS_ENCDEC_RESOLUTION_CLASS_WUXGA_HEIGHT);
            pFormat->pitch[0] =
                VpsUtils_align(pFormat->width, VPS_BUFFER_ALIGNMENT);
            break;
        case UTILS_ENCDEC_RESOLUTION_CLASS_16MP:     // 16MP
            pFormat->width =
                UTILS_ENCDEC_GET_PADDED_WIDTH
                (UTILS_ENCDEC_RESOLUTION_CLASS_16MP_WIDTH);
            pFormat->height =
                UTILS_ENCDEC_GET_PADDED_HEIGHT
                (UTILS_ENCDEC_RESOLUTION_CLASS_16MP_HEIGHT);
            pFormat->pitch[0] =
                VpsUtils_align(pFormat->width, VPS_BUFFER_ALIGNMENT);
            break;
        default:
            UTILS_warn("DECLINK: Unknown reslutionClass");
            UTILS_assert(1);
            break;
    }

    pFormat->pitch[1] = pFormat->pitch[0];
    if (pObj->createArgs.tilerEnable &&
        (pFormat->dataFormat == FVID2_DF_YUV420SP_UV))
    {
        pFormat->pitch[0] = VPSUTILS_TILER_CNT_8BIT_PITCH;
        pFormat->pitch[1] = VPSUTILS_TILER_CNT_16BIT_PITCH;
    }
    pFormat->pitch[2] = 0;

    pFormat->scanFormat = FVID2_SF_PROGRESSIVE;
    pFormat->bpp = FVID2_BPP_BITS16;
    pFormat->reserved = NULL;

    return (status);
}

static Int32 DecLink_codecCreateOutChObj(DecLink_Obj * pObj, UInt32 chId)
{
    DecLink_OutObj *pOutObj;
    Int32 status = DEC_LINK_S_SUCCESS;
    Int32 queStatus = DEC_LINK_S_SUCCESS;
    UInt32 frameId, outId;
    FVID2_Format *pFormat;
    FVID2_Frame  *fvidFrm;
    System_LinkChInfo *pOutChInfo;
    System_FrameInfo *pFrameInfo[DEC_LINK_MAX_NUM_OUT_BUF_PER_CH];

    pOutObj = &pObj->outObj;
    pFormat = &pOutObj->outChObj[chId].outFormat;

    DecLink_codecPopulateOutFrmFormat(pObj, chId);

    if (pObj->createArgs.chCreateParams[chId].algCreateStatus == 
        DEC_LINK_ALG_CREATE_STATUS_CREATE)
    {
        pObj->outObj.totalNumOutBufs += pOutObj->outChObj[chId].outNumFrames;
        UTILS_assert(pObj->outObj.totalNumOutBufs <= DEC_LINK_MAX_OUT_FRAMES);

        for (frameId = 0; frameId < pOutObj->outChObj[chId].outNumFrames; frameId++)
        {
            status = Utils_queGet(&pOutObj->outChObj[chId].fvidFrmQue,
                                  (Ptr *)&fvidFrm,1,BIOS_NO_WAIT);
            UTILS_assert(status == FVID2_SOK);
            pOutObj->outChObj[chId].outFrames[frameId] = fvidFrm;
            pFrameInfo[frameId] = pOutObj->outChObj[chId].outFrames[frameId]->appData;
        }

        if (pObj->createArgs.tilerEnable)
        {
            status = Utils_tilerFrameAlloc(&pOutObj->outChObj[chId].outFormat,
                                           pOutObj->outChObj[chId].outFramesPool,
                                           pOutObj->outChObj[chId].outNumFrames);
            UTILS_assert(status == FVID2_SOK);
        }
        else
        {
            for (frameId = 0; frameId < pOutObj->outChObj[chId].outNumFrames; frameId++)
            {
                status = Utils_memFrameAlloc(&pOutObj->outChObj[chId].outFormat,
                               pOutObj->outChObj[chId].outFrames[frameId],
                               1);
                //UTILS_assert(status == FVID2_SOK);
                if (status != FVID2_SOK)
                {
                    break;
                }
                /* Take a copy of the original frame. Will be used at free time */
                pOutObj->outChObj[chId].allocFrames[frameId] = 
                                *pOutObj->outChObj[chId].outFrames[frameId];
            }
            
            if (status != FVID2_SOK)
            {
                pObj->outObj.totalNumOutBufs -= 
                     (pOutObj->outChObj[chId].outNumFrames - frameId);
                pOutObj->outChObj[chId].outNumFrames = frameId;
                Vps_printf("DECLINK: ERROR!!! During Channel Open, "
                    "Only %d output buffers are getting allocated due to "
                    "insufficient memory, might affect CH%d performance \n", 
                    frameId, chId);
            }
        }

        for (frameId = 0; frameId < pOutObj->outChObj[chId].outNumFrames; frameId++)
        {
            pOutObj->outChObj[chId].outFrames[frameId]->appData = pFrameInfo[frameId];
            queStatus = Utils_bufPutEmptyFrameExt(&pOutObj->bufOutQue,
                              pOutObj->outChObj[chId].outFrames[frameId]);
            UTILS_assert(queStatus == FVID2_SOK);
        }

        pObj->chObj[chId].algCreateStatusLocal = 
                         DEC_LINK_ALG_CREATE_STATUS_CREATE_DONE;
        pObj->createArgs.chCreateParams[chId].algCreateStatus = 
                         DEC_LINK_ALG_CREATE_STATUS_CREATE_DONE;
    }

    for (outId = 0u; outId < DEC_LINK_MAX_OUT_QUE; outId++)
    {
        pFormat = &pObj->outObj.outChObj[chId].outFormat;
        pOutChInfo = &pObj->info.queInfo[outId].chInfo[chId];

        pOutChInfo->bufType = SYSTEM_BUF_TYPE_VIDFRAME;
        pOutChInfo->codingformat =
            pObj->createArgs.chCreateParams[chId].format;
        pOutChInfo->memType = VPS_VPDMA_MT_NONTILEDMEM;
        if (pObj->createArgs.tilerEnable)
        {
            pOutChInfo->memType = VPS_VPDMA_MT_TILEDMEM;
        }
        pOutChInfo->dataFormat = pFormat->dataFormat;
        pOutChInfo->scanFormat = pObj->inQueInfo.chInfo[chId].scanFormat;
        if (pObj->createArgs.chCreateParams[chId].format == IVIDEO_MJPEG)
        {
            pOutChInfo->startX = 0;
            pOutChInfo->startY = 0;
        }
        else
        {
            pOutChInfo->startX = 32;
            pOutChInfo->startY = 24;
        }
        pOutChInfo->width = pObj->inQueInfo.chInfo[chId].width;
        pOutChInfo->height = pObj->inQueInfo.chInfo[chId].height;
        pOutChInfo->pitch[0] = pFormat->pitch[0];
        pOutChInfo->pitch[1] = pFormat->pitch[1];
    }

    return (status);
}

static Void DecLink_codecInitFrameDebugObj(DecLink_ChObj *pChObj)
{
    pChObj->frameDebug.inBufIdx = 0;
    pChObj->frameDebug.outBufIdx = 0;
}


static Int32 DecLink_codecCreateChObj(DecLink_Obj * pObj, UInt32 chId)
{
    DecLink_ChObj *pChObj;
    Int32 status;
    ti_sysbios_knl_Semaphore_Params semPrms;

    pChObj = &pObj->chObj[chId];

    status = Utils_queCreate(&pChObj->inQue, DEC_LINK_MAX_REQ,
                             pChObj->inBitBufMem,
                             (UTILS_QUE_FLAG_BLOCK_QUE_GET |
                              UTILS_QUE_FLAG_BLOCK_QUE_PUT));
    UTILS_assert(status == FVID2_SOK);

    pChObj->allocPoolID = chId;

    pChObj->decErrorMsg.chId = chId;
    pChObj->decErrorMsg.reportA8 = TRUE;
    pChObj->decErrorMsg.errorMsg = 0;

    pChObj->totalInFrameCnt = 0;
    pChObj->algCreateStatusLocal = DEC_LINK_ALG_CREATE_STATUS_DONOT_CREATE;
    DecLink_codecInitFrameDebugObj(pChObj);

    ti_sysbios_knl_Semaphore_Params_init(&semPrms);
    semPrms.mode = ti_sysbios_knl_Semaphore_Mode_BINARY;
    ti_sysbios_knl_Semaphore_construct(&pChObj->codecProcessMutexMem,
                                       1,
                                       &semPrms);
    UTILS_assert(pChObj->codecProcessMutex == NULL);
    pChObj->codecProcessMutex =
    ti_sysbios_knl_Semaphore_handle(&pChObj->codecProcessMutexMem);

    return FVID2_SOK;
}

static Int32 declink_codec_set_ch_alg_create_params(DecLink_Obj * pObj,
                                                    UInt32 chId)
{
    DecLink_ChObj *pChObj;

    pChObj = &pObj->chObj[chId];
    pChObj->algObj.algCreateParams.format =
        (IVIDEO_Format) pObj->createArgs.chCreateParams[chId].format;
    pChObj->algObj.algCreateParams.presetProfile =
        pObj->createArgs.chCreateParams[chId].profile;
    pChObj->algObj.algCreateParams.fieldMergeDecodeEnable =
        pObj->createArgs.chCreateParams[chId].fieldMergeDecodeEnable;
    pChObj->algObj.algCreateParams.maxWidth =
        pObj->createArgs.chCreateParams[chId].targetMaxWidth;
    pChObj->algObj.algCreateParams.maxHeight =
        pObj->createArgs.chCreateParams[chId].targetMaxHeight;
    pChObj->algObj.algCreateParams.maxFrameRate =
        pObj->createArgs.chCreateParams[chId].defaultDynamicParams.
        targetFrameRate;
    pChObj->algObj.algCreateParams.maxBitRate =
        pObj->createArgs.chCreateParams[chId].defaultDynamicParams.
        targetBitRate;
    pChObj->algObj.algCreateParams.tilerEnable = pObj->createArgs.tilerEnable;

    pChObj->algObj.algCreateParams.displayDelay =
        decLink_map_displayDelay2CodecParam(pObj->createArgs.chCreateParams[chId].displayDelay);

    Utils_encdecGetCodecLevel(pChObj->algObj.algCreateParams.format,
                              pChObj->algObj.algCreateParams.maxWidth,
                              pChObj->algObj.algCreateParams.maxHeight,
                              pObj->createArgs.chCreateParams[chId].
                              defaultDynamicParams.targetFrameRate,
                              pObj->createArgs.chCreateParams[chId].
                              defaultDynamicParams.targetBitRate,
                              &(pChObj->algObj.algCreateParams.presetLevel),
                              FALSE);
    pChObj->algObj.algCreateParams.dpbBufSizeInFrames =
                      pObj->createArgs.chCreateParams[chId].dpbBufSizeInFrames;

    return DEC_LINK_S_SUCCESS;
}

static Int32 declink_codec_set_ch_alg_default_dynamic_params(DecLink_Obj * pObj,
                                                             UInt32 chId)
{
    DecLink_ChObj *pChObj;

    pChObj = &pObj->chObj[chId];
    pChObj->algObj.algDynamicParams.decodeHeader =
        DEC_LINK_DEFAULT_ALGPARAMS_DECODEHEADER;
#if 0
    pChObj->algObj.algDynamicParams.displayWidth =
        DEC_LINK_DEFAULT_ALGPARAMS_DISPLAYWIDTH;
#endif
    pChObj->algObj.algDynamicParams.displayWidth =
        pObj->outObj.outChObj[chId].outFormat.pitch[0];
    pChObj->algObj.algDynamicParams.frameSkipMode =
        DEC_LINK_DEFAULT_ALGPARAMS_FRAMESKIPMODE;
    pChObj->algObj.algDynamicParams.newFrameFlag =
        DEC_LINK_DEFAULT_ALGPARAMS_NEWFRAMEFLAG;

    return DEC_LINK_S_SUCCESS;
}

static Int32 DecLink_codecCreateDecObj(DecLink_Obj * pObj, UInt32 chId)
{
    Int retVal;
    DecLink_ChObj *pChObj;
    Int scratchGroupID;
    UInt32 contentType;

    pChObj = &pObj->chObj[chId];
    scratchGroupID = -1;

    DecLink_codecPopulateOutFrmFormat(pObj, chId);
    declink_codec_set_ch_alg_create_params(pObj, chId);
    declink_codec_set_ch_alg_default_dynamic_params(pObj, chId);

    contentType =
        Utils_encdecMapFVID2XDMContentType(pObj->inQueInfo.chInfo[chId].
                                           scanFormat);

    if (contentType == IVIDEO_INTERLACED)
    {
        pChObj->numReqObjPerProcess = 2;
    }
    else
    {
        pChObj->numReqObjPerProcess = 1;
    }
    pChObj->algObj.prevOutFrame = NULL;
    switch (pChObj->algObj.algCreateParams.format)
    {
        case IVIDEO_H264BP:
        case IVIDEO_H264MP:
        case IVIDEO_H264HP:
            retVal = DecLinkH264_algCreate(&pChObj->algObj.u.h264AlgIfObj,
                                           &pChObj->algObj.algCreateParams,
                                           &pChObj->algObj.algDynamicParams,
                                           pObj->linkId, chId, scratchGroupID,
                                           &pObj->outObj.outChObj[chId].outFormat,
                                           pObj->outObj.outChObj[chId].outNumFrames,
                                           &pChObj->algObj.resDesc[0]);
            break;

        case IVIDEO_MPEG4SP:
        case IVIDEO_MPEG4ASP:
            retVal = DecLinkMPEG4_algCreate(&pChObj->algObj.u.mpeg4AlgIfObj,
                                            &pChObj->algObj.algCreateParams,
                                            &pChObj->algObj.algDynamicParams,
                                            pObj->linkId, chId, scratchGroupID,
                                            &pObj->outObj.outChObj[chId].outFormat,
                                            pObj->outObj.outChObj[chId].outNumFrames,
                                            &pChObj->algObj.resDesc[0]);
            break;

        case IVIDEO_MJPEG:
            retVal = DecLinkJPEG_algCreate(&pChObj->algObj.u.jpegAlgIfObj,
                                           &pChObj->algObj.algCreateParams,
                                           &pChObj->algObj.algDynamicParams,
                                           pObj->linkId, chId, scratchGroupID,
                                           &pObj->outObj.outChObj[chId].outFormat,
                                           pObj->outObj.outChObj[chId].outNumFrames,
                                           &pChObj->algObj.resDesc[0]);
            break;

        default:
            retVal = DEC_LINK_E_UNSUPPORTEDCODEC;
            UTILS_assert(retVal == DEC_LINK_S_SUCCESS);
            break;
    }

    return retVal;
}

static Int32 DecLink_codecCreateChannel(DecLink_Obj * pObj, UInt32 chId)
{
    DecLink_ChObj *pChObj;
    Int32 status = DEC_LINK_S_SUCCESS;

    UTILS_assert(chId <= pObj->inQueInfo.numCh);
    pChObj = &pObj->chObj[chId];
    pObj->outObj.outChObj[chId].reslutionClass =
        (EncDec_ResolutionClass) (UTILS_ENCDEC_RESOLUTION_CLASS_LAST + 1);
    pObj->outObj.outChObj[chId].outNumFrames = 0;
    pChObj->isFirstIDRFrameFound = FALSE;

    status = DecLink_codecMapCh2ResolutionPool(pObj, chId);
    UTILS_assert(status == DEC_LINK_S_SUCCESS);
    /* Set numBufPerCh to the default value if not set properly */
    if(pObj->createArgs.chCreateParams[chId].numBufPerCh <= 0)
    {
        pObj->createArgs.chCreateParams[chId].numBufPerCh = 
                         DEC_LINK_MAX_OUT_FRAMES_PER_CH;
    }
    
    if (DEC_LINK_MAX_NUM_OUT_BUF_PER_CH < 
        pObj->createArgs.chCreateParams[chId].numBufPerCh)
    {
       Vps_printf("\n DECLINK: WARNING: User is asking for %d buffers per CH. But max allowed is %d. \n"
          " Over riding user requested with max allowed \n\n",
            pObj->createArgs.chCreateParams[chId].numBufPerCh, 
            DEC_LINK_MAX_NUM_OUT_BUF_PER_CH);
        pObj->createArgs.chCreateParams[chId].numBufPerCh = 
              DEC_LINK_MAX_NUM_OUT_BUF_PER_CH;
    }

    DecLink_codecMapCh2ProcessTskId(pObj, chId);

    if (pObj->createArgs.chCreateParams[chId].algCreateStatus == 
        DEC_LINK_ALG_CREATE_STATUS_CREATE)
    {
        status = DecLink_codecCreateDecObj(pObj, chId);
        if (status != DEC_LINK_S_SUCCESS)
        {   
            pObj->createArgs.chCreateParams[chId].algCreateStatus = 
                             DEC_LINK_ALG_CREATE_STATUS_DELETE;
        }
    }
    else
    {
        #ifdef SYSTEM_DEBUG_DEC
        Vps_printf("%d: DECODE: CodecInst and OutFrm bufs are NOT created for CH%d\n",
            Utils_getCurTimeInMsec(),
            chId            );
        #endif
    }

    if (status == DEC_LINK_S_SUCCESS)
    {
        status = DecLink_codecCreateOutChObj(pObj, chId);
        //UTILS_assert(status == DEC_LINK_S_SUCCESS);

        #ifdef SYSTEM_DEBUG_DEC
        Vps_printf(" %d: DECODE: Creating CH%d of %d x %d [%s] [%s],"
                   "target bitrate = %d Kbps ... \n",
            Utils_getCurTimeInMsec(),
            chId,
            pObj->inQueInfo.chInfo[chId].width,
            pObj->inQueInfo.chInfo[chId].height,
            gSystem_nameScanFormat[pObj->inQueInfo.chInfo[chId].scanFormat],
            gSystem_nameMemoryType[pObj->createArgs.tilerEnable],
            pObj->createArgs.chCreateParams[chId].defaultDynamicParams.
                                                  targetBitRate/1000
            );
        #endif
    }
    pChObj->IFrameOnlyDecode = FALSE;
    pChObj->inBufQueCount = 0;
    pChObj->processReqestCount = 0;
    pChObj->getProcessedBufCount = 0;
    pChObj->skipFrame = FALSE;
    pChObj->disableChn = FALSE;

    return status;
}

static Int32 declink_codec_init_outframe(DecLink_Obj * pObj,
                                         UInt32 chId, FVID2_Frame * pFrame)
{
    System_FrameInfo *pFrameInfo;

    pFrameInfo = (System_FrameInfo *) pFrame->appData;
    UTILS_assert((pFrameInfo != NULL)
                 &&
                 UTILS_ARRAYISVALIDENTRY(pFrameInfo,
                                         pObj->outObj.outChObj[chId].frameInfo));
    pFrameInfo->vdecRefCount = 1;
    pFrameInfo->pVdecOrgFrame = NULL;

    /* By Default, the output data type is 420 SP, update the rtparam,
        other link will use this when rtChInfoUpdate is set */
    pFrameInfo->rtChInfo.dataFormat = SYSTEM_DF_YUV420SP_UV;

    return DEC_LINK_S_SUCCESS;
}

static Int32 DecLink_codecGetDisplayFrame(DecLink_Obj * pObj,
                                          FVID2_Frame * outFrame,
                                          FVID2_FrameList * freeFrameList,
                                          FVID2_Frame ** displayFrame)
{
    Int i, j, status = DEC_LINK_S_SUCCESS;
    Bool doDup = TRUE;
    UInt32 chId;

    if (outFrame != NULL)
    {
        *displayFrame = NULL;
        chId = outFrame->channelNum;
        UTILS_assert(UTILS_ARRAYISVALIDENTRY(outFrame,
                                             pObj->outObj.outChObj[chId].outFramesPool));

        for (i = 0; i < freeFrameList->numFrames; i++)
        {
            if (freeFrameList->frames[i] == outFrame)
            {
                /* This frame is going to be used as display frame. Remove
                 * it from the freeFrameList */
                for (j = (i + 1); j < freeFrameList->numFrames; j++)
                {
                    freeFrameList->frames[j - 1] = freeFrameList->frames[j];
                }
                freeFrameList->numFrames -= 1;
                doDup = FALSE;
                break;
            }
        }
    }
    else
    {
        doDup = FALSE;
    }
    if (FALSE == doDup)
    {
        *displayFrame = outFrame;
    }
    else
    {
        status = DecLink_dupFrame(pObj, outFrame, displayFrame);
    }
    return status;
}

Int32 DecLink_codecFreeProcessedFrames(DecLink_Obj * pObj,
                                       FVID2_FrameList * freeFrameList)
{
    Int i, status = DEC_LINK_S_SUCCESS;
    FVID2_Frame *freeFrame;
    FVID2_Frame *origFrame;
    System_FrameInfo *freeFrameInfo;
    UInt cookie;
    Bool bufFreeDone = FALSE;

    cookie = Hwi_disable();

    for (i = 0; i < freeFrameList->numFrames; i++)
    {
        freeFrame = freeFrameList->frames[i];
        UTILS_assert(freeFrame != NULL);
        freeFrameInfo = freeFrame->appData;
        UTILS_assert(freeFrameInfo != NULL);
        if (freeFrameInfo->pVdecOrgFrame)
        {
            UTILS_assert(UTILS_ARRAYISVALIDENTRY(freeFrame,
                                                 pObj->dupObj.dupFrameMem));
            origFrame = freeFrameInfo->pVdecOrgFrame;
            status = Utils_quePut(&pObj->dupObj.dupQue,
                                  freeFrame, BIOS_NO_WAIT);
            UTILS_assert(!UTILS_ISERROR(status));
            freeFrame = origFrame;
            freeFrameInfo = origFrame->appData;
        }
        UTILS_assert((freeFrameInfo->pVdecOrgFrame == NULL)
                     && (freeFrameInfo->vdecRefCount > 0));
        freeFrameInfo->vdecRefCount--;
        if (freeFrameInfo->vdecRefCount == 0)
        {
            if (freeFrameInfo->invalidFrame == TRUE)
            {
                freeFrameInfo->invalidFrame = FALSE;
                status = Utils_quePut(
                    &pObj->outObj.outChObj[freeFrameInfo->allocPoolID].fvidFrmQue, 
                    freeFrame, BIOS_NO_WAIT);
                UTILS_assert(status == FVID2_SOK);
            }
            else
            {
                status = Utils_bufPutEmptyFrameExt(&pObj->outObj.bufOutQue, 
                                                    freeFrame);
                UTILS_assert(!UTILS_ISERROR(status));
                bufFreeDone = TRUE;
            }
        }
    }

    Hwi_restore(cookie);
    if ((TRUE == pObj->newDataProcessOnFrameFree)
      &&
      (bufFreeDone))
    {
        status = System_sendLinkCmd(pObj->linkId,
                                    SYSTEM_CMD_NEW_DATA);

        if (UTILS_ISERROR(status))
        {
            UTILS_warn("DECLINK:[%s:%d]:"
                       "System_sendLinkCmd SYSTEM_CMD_NEW_DATA failed"
                       "errCode = %d", __FILE__, __LINE__, status);
        }
        else
        {
          pObj->newDataProcessOnFrameFree = FALSE;
        }
    }
    return status;
}

static Int32 DecLink_codecHandleDummyReqObj(DecLink_Obj  *pObj,
                                            DecLink_ReqObj *pReqObj,
                                            UInt32 ivaId)
{
    Int32 status = DEC_LINK_S_SUCCESS;

    switch (pReqObj->type)
    {
        case DEC_LINK_REQ_OBJECT_TYPE_DUMMY_CHDELETE:
        {
            Vps_printf("DEC : Delete CH%d Got the Dummy Object queued !!!\n",
                        pReqObj->InBuf->channelNum);
            DecLink_codecFlushNDeleteChannel(pObj, pReqObj->InBuf->channelNum);
            UTILS_assert(pReqObj->ivaSwitchSerializer == NULL);
            break;
        }
        case DEC_LINK_REQ_OBJECT_TYPE_DUMMY_IVAMAPCHANGE_PREVIVALAST:
        {
            UTILS_assert(pReqObj->InBuf != NULL);
            UTILS_assert((pReqObj->ivaSwitchSerializer != NULL)
                         &&
                         (UTILS_ARRAYISVALIDENTRY(ti_sysbios_knl_Semaphore_struct(pReqObj->ivaSwitchSerializer),
                          pObj->decIVASwitchSerializeObj.freeSerializerSemMem)));
            DECLINK_INFO_LOG(pObj->linkId,pReqObj->InBuf->channelNum,
                             "Iva Map Change Serialization: Last Frame of Prev IVA [%d] received",ivaId);
            ti_sysbios_knl_Semaphore_post(pReqObj->ivaSwitchSerializer);
            break;
        }

        case DEC_LINK_REQ_OBJECT_TYPE_DUMMY_IVAMAPCHANGE_NEXTIVAFIRST:
        {
            Bool semStatus;

            UTILS_assert(pReqObj->InBuf != NULL);
            UTILS_assert((pReqObj->ivaSwitchSerializer != NULL)
                         &&
                         (UTILS_ARRAYISVALIDENTRY(ti_sysbios_knl_Semaphore_struct(pReqObj->ivaSwitchSerializer),
                          pObj->decIVASwitchSerializeObj.freeSerializerSemMem)));
            DECLINK_INFO_LOG(pObj->linkId,pReqObj->InBuf->channelNum,
                             "Iva Map Change Serialization: First Frame of Next IVA [%d] received",ivaId);
            semStatus = ti_sysbios_knl_Semaphore_pend(pReqObj->ivaSwitchSerializer,ti_sysbios_BIOS_WAIT_FOREVER);
            UTILS_assert(semStatus == TRUE);
            DECLINK_INFO_LOG(pObj->linkId,pReqObj->InBuf->channelNum,
                             "Iva Map Change Serialization: First Frame of Next IVA [%d] received. Serialization complete!!",ivaId);
            status = Utils_quePut(&pObj->decIVASwitchSerializeObj.freeSerializerQue,
                                  Semaphore_struct(pReqObj->ivaSwitchSerializer),
                                  ti_sysbios_BIOS_NO_WAIT);
            UTILS_assert(status == 0);
            break;
        }
        case DEC_LINK_REQ_OBJECT_TYPE_DUMMY_FLUSHFRAME:
        {
            UTILS_assert(pReqObj->InBuf != NULL);
            UTILS_assert(pReqObj->InBuf->flushFrame == TRUE);
            UTILS_assert(pReqObj->ivaSwitchSerializer == NULL);
            DECLINK_INFO_LOG(pObj->linkId,pReqObj->InBuf->channelNum,"Flush Frame Received in ProcessQue");
            break;
        }
        default:
            /* Unsupported reqObjType.*/
            UTILS_assert(0);
            break;
    }

    if (DEC_LINK_REQ_OBJECT_TYPE_DUMMY_FLUSHFRAME != pReqObj->type)
    {
        pReqObj->type = DEC_LINK_REQ_OBJECT_TYPE_REGULAR;
        pReqObj->ivaSwitchSerializer = NULL;
        UTILS_assert(UTILS_ARRAYISVALIDENTRY(pReqObj,pObj->decDummyReqObj.reqObjDummy));
        status = Utils_quePut(&pObj->decDummyReqObj.reqQueDummy, pReqObj, ti_sysbios_BIOS_NO_WAIT);
        UTILS_assert(status == FVID2_SOK);
    }
    else
    {
        Int i;

        pReqObj->type = DEC_LINK_REQ_OBJECT_TYPE_REGULAR;
        DecLink_codecFreeProcessedFrames(pObj,&pReqObj->OutFrameList);
        for (i = 0; i < pReqObj->OutFrameList.numFrames; i++)
        {
            pReqObj->OutFrameList.frames[i] = NULL;
        }
        pReqObj->OutFrameList.numFrames = 0;
        status = Utils_quePut(&pObj->processDoneQue, pReqObj,
                              BIOS_NO_WAIT);
        UTILS_assert(status == FVID2_SOK);
    }
    return status;
}

static Void DecLink_codecProcessAcquireProcessMutex(DecLink_Obj *pObj,
                                                    DecLink_ReqBatch *reqBatch)
{

    Int32 chId, i;
    DecLink_ChObj *pChObj;
    DecLink_ReqObj *pReqObj;

    for (i = 0; i < reqBatch->numReqObjsInBatch; i++)
    {
        pReqObj = reqBatch->pReqObj[i];
        chId = pReqObj->InBuf->channelNum;
        UTILS_assert(chId < UTILS_ARRAYSIZE(pObj->chObj));
        pChObj = &pObj->chObj[chId];
        ti_sysbios_knl_Semaphore_pend(pChObj->codecProcessMutex,ti_sysbios_BIOS_WAIT_FOREVER);
    }
}


static Void DecLink_codecProcessReleaseProcessMutex(DecLink_Obj *pObj,
                                                    DecLink_ReqBatch *reqBatch)
{

    Int32 chId, i;
    DecLink_ChObj *pChObj;
    DecLink_ReqObj *pReqObj;

    for (i = 0; i < reqBatch->numReqObjsInBatch; i++)
    {
        pReqObj = reqBatch->pReqObj[i];
        chId = pReqObj->InBuf->channelNum;
        UTILS_assert(chId < UTILS_ARRAYSIZE(pObj->chObj));
        pChObj = &pObj->chObj[chId];
        ti_sysbios_knl_Semaphore_post(pChObj->codecProcessMutex);
    }
}

static Void DecLink_codecProcessTskFxn(UArg arg1, UArg arg2)
{
    Int32 status, chId, i, j;
    DecLink_Obj *pObj;
    DecLink_ChObj *pChObj;
    DecLink_ReqObj *pReqObj;
    FVID2_FrameList freeFrameList;
    UInt32 tskId;
    HDVICP_tskEnv  *tskEnv;
    
    pObj = (DecLink_Obj *) arg1;
    tskId = (UInt32) arg2;

    while (pObj->state != SYSTEM_LINK_STATE_STOP)
    {
        pObj->reqObjBatch[tskId].numReqObjsInBatch = 0;
        status = DEC_LINK_S_SUCCESS;

        pReqObj = NULL;

        status = Utils_queGet(&pObj->decProcessTsk[tskId].processQue,
                              (Ptr *) & pReqObj, 1, BIOS_WAIT_FOREVER);

        if (!UTILS_ISERROR(status))
        {
            if (pReqObj->type != DEC_LINK_REQ_OBJECT_TYPE_REGULAR)
            {
                DecLink_codecHandleDummyReqObj(pObj, pReqObj,tskId);
                continue;
            }
            UTILS_assert(pReqObj->type == DEC_LINK_REQ_OBJECT_TYPE_REGULAR);
            status = DecLink_PrepareBatch (pObj, tskId, pReqObj, 
                                  &pObj->reqObjBatch[tskId]);

            if (UTILS_ISERROR(status))
            {
                UTILS_warn("DEC : IVAHDID : %d ENCLINK:ERROR in "
                           "DecLink_SubmitBatch.Status[%d]", tskId, status);
            }
            else
            {
                /*Log Batch size statistics*/
                pObj->batchStatistics[tskId].numBatchesSubmitted++;

                pObj->batchStatistics[tskId].currentBatchSize = pObj->
                  reqObjBatch[tskId].numReqObjsInBatch;

                if (pObj->batchStatistics[tskId].maxAchievedBatchSize <
                    pObj->batchStatistics[tskId].currentBatchSize)
                {
                  pObj->batchStatistics[tskId].maxAchievedBatchSize =
                    pObj->batchStatistics[tskId].currentBatchSize;
                }

                pObj->batchStatistics[tskId].aggregateBatchSize =
                  pObj->batchStatistics[tskId].aggregateBatchSize +
                  pObj->batchStatistics[tskId].currentBatchSize;

                pObj->batchStatistics[tskId].averageBatchSize =
                  pObj->batchStatistics[tskId].aggregateBatchSize /
                  pObj->batchStatistics[tskId].numBatchesSubmitted;
            }
        }
        freeFrameList.numFrames = 0;
        if (pObj->reqObjBatch[tskId].numReqObjsInBatch)
        {
            DecLink_codecProcessAcquireProcessMutex(pObj,
                                                    &pObj->reqObjBatch[tskId]);
            /*Its made sure that for every batch created all ReqObj have the same
            codec. And every Request Batch has atleast one ReqObj */
            chId = pObj->reqObjBatch[tskId].pReqObj[0]->InBuf->channelNum;
            pChObj = &pObj->chObj[chId];

            /*For every batch submitted, send the batch size to FC so that total
            number of channels processed can be logged.*/            
            tskEnv = Task_getEnv(Task_self());
            UTILS_assert(tskEnv!= NULL);
            tskEnv->batchSize = pObj->reqObjBatch[tskId].numReqObjsInBatch;            
            
            switch (pChObj->algObj.algCreateParams.format)
            {
                case IVIDEO_H264BP:
                case IVIDEO_H264MP:
                case IVIDEO_H264HP:
                    status = 
                        Declink_h264DecodeFrameBatch(pObj, 
                                                     &pObj->reqObjBatch[tskId],
                                                     &freeFrameList, tskId);
                    if (UTILS_ISERROR(status))
                    {
                         #ifndef DEC_LINK_SUPRESS_ERROR_AND_RESET
                         /*
                         UTILS_warn("DECLINK:ERROR in "
                              "Declink_h264DecodeFrameBatch.Status[%d]", status); 
                         */
                         #endif   
                    }
               break;
         
               case IVIDEO_MPEG4SP:
               case IVIDEO_MPEG4ASP:
                status = Declink_mpeg4DecodeFrameBatch(pObj, 
                                                            &pObj->reqObjBatch[tskId],
                                                            &freeFrameList);
                if (UTILS_ISERROR(status))
                {
                  #ifndef DEC_LINK_SUPRESS_ERROR_AND_RESET
                   UTILS_warn("DECLINK:ERROR in "
                      "Declink_mpeg4DecodeFrameBatch.Status[%d]", status);
                  #endif

                }
               break;

                case IVIDEO_MJPEG:

                   status = 
                      Declink_jpegDecodeFrameBatch(pObj, 
                                                   &pObj->reqObjBatch[tskId],
                                                   &freeFrameList);
                   if (UTILS_ISERROR(status))
                   {
                       UTILS_warn("DECLINK:ERROR in "
                             "Declink_jpegDecodeFrameBatch.Status[%d]", status);
                   }
                   
                 break;

                default:
                    UTILS_assert(FALSE);
                    break;
            }
            DecLink_codecProcessReleaseProcessMutex(pObj,&pObj->reqObjBatch[tskId]);
        }
        for (i = 0; i < pObj->reqObjBatch[tskId].numReqObjsInBatch; i++)
        {
            pReqObj = pObj->reqObjBatch[tskId].pReqObj[i];

            for (j = 0; j < pReqObj->OutFrameList.numFrames; j++)
            {
                FVID2_Frame *displayFrame;

                DecLink_codecGetDisplayFrame(pObj,
                                         pReqObj->OutFrameList.frames[j],
                                         &freeFrameList, &displayFrame);
                pReqObj->OutFrameList.frames[j] = displayFrame;

            }
            status = Utils_quePut(&pObj->processDoneQue, pReqObj,
                                  BIOS_NO_WAIT);
            UTILS_assert(status == FVID2_SOK);
        }
        DecLink_codecFreeProcessedFrames(pObj, &freeFrameList);
    }

    return;
}

#pragma DATA_ALIGN(gDecProcessTskStack, 32)
#pragma DATA_SECTION(gDecProcessTskStack, ".bss:taskStackSection:dec_process")
UInt8 gDecProcessTskStack[NUM_HDVICP_RESOURCES][DEC_LINK_PROCESS_TSK_SIZE];

static Int32 DecLink_codecCreateProcessTsk(DecLink_Obj * pObj, UInt32 tskId)
{
    Int32 status = DEC_LINK_S_SUCCESS;
    Task_Params tskParams;
    Error_Block ebObj;
    Error_Block *eb = &ebObj;

    Error_init(eb);

    Task_Params_init(&tskParams);

    snprintf(pObj->decProcessTsk[tskId].name,
             (sizeof(pObj->decProcessTsk[tskId].name) - 1),
             "DEC_PROCESS_TSK_%d ", tskId);
    pObj->decProcessTsk[tskId].
          name[(sizeof(pObj->decProcessTsk[tskId].name) - 1)] = 0;
    tskParams.priority = IVASVR_TSK_PRI+1;
    tskParams.stack = &gDecProcessTskStack[tskId][0];
    tskParams.stackSize = DEC_LINK_PROCESS_TSK_SIZE;
    tskParams.arg0 = (UArg) pObj;
    tskParams.arg1 = (UArg) tskId;
    tskParams.instance->name = pObj->decProcessTsk[tskId].name;

    Task_construct(&pObj->decProcessTsk[tskId].tskStruct,
                   DecLink_codecProcessTskFxn, &tskParams, eb);
    UTILS_assertError((Error_check(eb) == FALSE), status,
                      DEC_LINK_E_TSKCREATEFAILED, pObj->linkId, tskId);

    if (DEC_LINK_S_SUCCESS == status)
    {
        pObj->decProcessTsk[tskId].tsk =
              Task_handle(&pObj->decProcessTsk[tskId].tskStruct);
        pObj->decProcessTsk[tskId].tskEnv.size = sizeof(pObj->decProcessTsk[tskId].tskEnv);
        pObj->decProcessTsk[tskId].tskEnv.ivaID = tskId;
        Task_setEnv(pObj->decProcessTsk[tskId].tsk,
                &pObj->decProcessTsk[tskId].tskEnv);
        Utils_prfLoadRegister(pObj->decProcessTsk[tskId].tsk,
                              pObj->decProcessTsk[tskId].name);
    }
    return status;
}

static Int32 DecLink_codecDeleteProcessTsk(DecLink_Obj * pObj, UInt32 tskId)
{
    Int32 status = DEC_LINK_S_SUCCESS;

    Utils_queUnBlock(&pObj->decProcessTsk[tskId].processQue);
    Utils_queUnBlock(&pObj->processDoneQue);
    while (Task_getMode(pObj->decProcessTsk[tskId].tsk) != Task_Mode_TERMINATED)
    {
        Task_sleep(DEC_LINK_TASK_POLL_DURATION_MS);
    }
    Utils_prfLoadUnRegister(pObj->decProcessTsk[tskId].tsk);

    Task_destruct(&pObj->decProcessTsk[tskId].tskStruct);
    pObj->decProcessTsk[tskId].tsk = NULL;
    return status;
}

static Int32 DecLink_codecMapCh2ProcessTskId(DecLink_Obj * pObj, UInt32 chId)
{
    Int32 status = DEC_LINK_S_SUCCESS;

    pObj->ch2ProcessTskId[chId] = Utils_encdecGetDecoderIVAID (chId);

    return status;
}

static Int32 DecLink_codecMapCh2ResolutionPool(DecLink_Obj * pObj, UInt32 chId)
{
    Int32 status = DEC_LINK_E_FAIL;
    DecLink_OutObj *pOutObj;

    pOutObj = &pObj->outObj;

    if ((pObj->createArgs.chCreateParams[chId].targetMaxWidth <=
         UTILS_ENCDEC_RESOLUTION_CLASS_CIF_WIDTH) &&
        (pObj->createArgs.chCreateParams[chId].targetMaxHeight <=
         UTILS_ENCDEC_RESOLUTION_CLASS_CIF_HEIGHT))
    {
        pOutObj->outChObj[chId].reslutionClass = UTILS_ENCDEC_RESOLUTION_CLASS_CIF;
        status = DEC_LINK_S_SUCCESS;
    }
    if ((status != DEC_LINK_S_SUCCESS) &&
        (pObj->createArgs.chCreateParams[chId].targetMaxWidth <=
         UTILS_ENCDEC_RESOLUTION_CLASS_D1_WIDTH) &&
        (pObj->createArgs.chCreateParams[chId].targetMaxHeight <=
         UTILS_ENCDEC_RESOLUTION_CLASS_D1_HEIGHT))
    {
        pOutObj->outChObj[chId].reslutionClass = UTILS_ENCDEC_RESOLUTION_CLASS_D1;
        status = DEC_LINK_S_SUCCESS;
    }
    if ((status != DEC_LINK_S_SUCCESS) &&
        (pObj->createArgs.chCreateParams[chId].targetMaxWidth <=
         UTILS_ENCDEC_RESOLUTION_CLASS_720P_WIDTH) &&
        (pObj->createArgs.chCreateParams[chId].targetMaxHeight <=
         UTILS_ENCDEC_RESOLUTION_CLASS_720P_HEIGHT))
    {
        pOutObj->outChObj[chId].reslutionClass = UTILS_ENCDEC_RESOLUTION_CLASS_720P;
        status = DEC_LINK_S_SUCCESS;
    }
    if ((status != DEC_LINK_S_SUCCESS) &&
        (pObj->createArgs.chCreateParams[chId].targetMaxWidth <=
         UTILS_ENCDEC_RESOLUTION_CLASS_1080P_WIDTH) &&
        (pObj->createArgs.chCreateParams[chId].targetMaxHeight <=
         UTILS_ENCDEC_RESOLUTION_CLASS_1080P_HEIGHT))
    {
        pOutObj->outChObj[chId].reslutionClass = UTILS_ENCDEC_RESOLUTION_CLASS_1080P;
        status = DEC_LINK_S_SUCCESS;
    }
    if ((status != DEC_LINK_S_SUCCESS) &&
        (pObj->createArgs.chCreateParams[chId].targetMaxWidth <=
         UTILS_ENCDEC_RESOLUTION_CLASS_WUXGA_WIDTH) &&
        (pObj->createArgs.chCreateParams[chId].targetMaxHeight <=
         UTILS_ENCDEC_RESOLUTION_CLASS_WUXGA_HEIGHT))
    {
        pOutObj->outChObj[chId].reslutionClass = UTILS_ENCDEC_RESOLUTION_CLASS_WUXGA;
        status = DEC_LINK_S_SUCCESS;
    }
    if ((status != DEC_LINK_S_SUCCESS) &&
        (pObj->createArgs.chCreateParams[chId].targetMaxWidth <=
         UTILS_ENCDEC_RESOLUTION_CLASS_16MP_WIDTH) &&
        (pObj->createArgs.chCreateParams[chId].targetMaxHeight <=
         UTILS_ENCDEC_RESOLUTION_CLASS_16MP_HEIGHT))
    {
        pOutObj->outChObj[chId].reslutionClass = UTILS_ENCDEC_RESOLUTION_CLASS_16MP;
        status = DEC_LINK_S_SUCCESS;
    }
    UTILS_assert(status == DEC_LINK_S_SUCCESS);

    return (status);
}

static void DecLink_codecCreateInitStats(DecLink_Obj * pObj)
{
    Int32 chId;
    DecLink_ChObj *pChObj;

    pObj->inBufGetCount = 0;
    pObj->inBufPutCount = 0;

    for (chId = 0; chId < pObj->inQueInfo.numCh; chId++)
    {
        pChObj = &pObj->chObj[chId];

        pChObj->prevFrmRecvTime = 0;
        pChObj->totalProcessTime = 0;
        pChObj->totalFrameIntervalTime = 0;
        pChObj->totalInFrameCnt = 0;
        pChObj->inBufQueCount = 0;
        pChObj->processReqestCount = 0;
        pChObj->getProcessedBufCount = 0;
        pChObj->numBufsInCodec       = 0;

        pChObj->disableChn = FALSE;
        pChObj->skipFrame = FALSE;
    }

    return;
}

static void DecLink_initTPlayConfig(DecLink_Obj * pObj)
{
    Int32 chId;
    DecLink_ChObj *pChObj;

    for (chId = 0; chId < pObj->inQueInfo.numCh; chId++)
    {
        pChObj = &pObj->chObj[chId];

        pChObj->trickPlayObj.skipFrame = FALSE;
        pChObj->trickPlayObj.frameSkipCtx.outputFrameRate = 30;
        pChObj->trickPlayObj.frameSkipCtx.inputFrameRate  = 30;
        pChObj->trickPlayObj.frameSkipCtx.inCnt = 0;
        pChObj->trickPlayObj.frameSkipCtx.outCnt = 0;
        pChObj->trickPlayObj.frameSkipCtx.multipleCnt = 0;
        pChObj->trickPlayObj.frameSkipCtx.firstTime = TRUE;

    }

    return;
}

Int32 DecLink_codecCreate(DecLink_Obj * pObj, DecLink_CreateParams * pPrm)
{
    Int32 status;
    Int32 chId, tskId;

    #ifdef SYSTEM_DEBUG_DEC
    Vps_printf(" %d: DECODE: Create in progress ... !!!\n",
            Utils_getCurTimeInMsec()
        );
    #endif

    #ifdef SYSTEM_DEBUG_MEMALLOC
    Vps_printf ("Before DEC Create:\n");
    System_memPrintHeapStatus();
    #endif

    UTILS_MEMLOG_USED_START();
    memcpy(&pObj->createArgs, pPrm, sizeof(*pPrm));

    status = System_linkGetInfo(pPrm->inQueParams.prevLinkId, &pObj->inTskInfo);
    UTILS_assert(status == FVID2_SOK);
    UTILS_assert(pPrm->inQueParams.prevLinkQueId < pObj->inTskInfo.numQue);

    memcpy(&pObj->inQueInfo,
           &pObj->inTskInfo.queInfo[pPrm->inQueParams.prevLinkQueId],
           sizeof(pObj->inQueInfo));
    UTILS_assert(pObj->inQueInfo.numCh <= DEC_LINK_MAX_CH);

    #ifndef SYSTEM_USE_TILER
    if (pObj->createArgs.tilerEnable)
    {
        Vps_printf("DECLINK:!!WARNING.FORCIBLY DISABLING TILER since"
                   "tiler is disabled at build time");
        pObj->createArgs.tilerEnable = FALSE;
    }
    #endif

#if DECLINK_UPDATE_CREATEPARAMS_LOCALLY
    /* ENABLE the define DECLINK_UPDATE_CREATEPARAMS_LOCALLY if App is not
     * configuring the create time paramters */
    for (chId = 0; chId < pObj->inQueInfo.numCh; chId++)
    {
        if(pObj->createArgs.chCreateParams[chId].format ==  IVIDEO_H264HP)
            pObj->createArgs.chCreateParams[chId] = DECLINK_DEFAULTCHCREATEPARAMS_H264;
        else if(pObj->createArgs.chCreateParams[chId].format ==  IVIDEO_MPEG4ASP)
            pObj->createArgs.chCreateParams[chId] = DECLINK_DEFAULTCHCREATEPARAMS_MPEG4;
        else if(pObj->createArgs.chCreateParams[chId].format ==  IVIDEO_MJPEG)
            pObj->createArgs.chCreateParams[chId] = DECLINK_DEFAULTCHCREATEPARAMS_JPEG;
    }
#endif

    DecLink_initTPlayConfig(pObj);
    DecLink_codecCreateInitStats(pObj);
    DecLink_resetStatistics(pObj);

    DecLink_codecCreateOutObjCommon(pObj);
    for (chId = 0; chId < pObj->inQueInfo.numCh; chId++)
    {
        DecLink_codecCreateChObj(pObj, chId);
        status = DecLink_codecCreateChannel(pObj, chId);
        UTILS_assert(status == DEC_LINK_S_SUCCESS);
    }
    DecLink_codecCreateReqObj(pObj);
    pObj->state = SYSTEM_LINK_STATE_START;

    DecLink_codecCreateReqObjDummy(pObj);

    #ifdef SYSTEM_DEBUG_DEC
    Vps_printf(" %d: DECODE: All CH Create ... DONE !!!\n",
            Utils_getCurTimeInMsec()
        );
    #endif

    DecLink_codecRegisterIVAMapCb(pObj);
    for (tskId = 0; tskId < NUM_HDVICP_RESOURCES; tskId++)
    {
        status = Utils_queCreate(&pObj->decProcessTsk[tskId].processQue,
                                 DEC_LINK_MAX_OUT_FRAMES,
                                 pObj->decProcessTsk[tskId].processQueMem,
                                 (UTILS_QUE_FLAG_BLOCK_QUE_GET |
                                  UTILS_QUE_FLAG_BLOCK_QUE_PUT));
        UTILS_assert(status == FVID2_SOK);
        DecLink_codecCreateProcessTsk(pObj, tskId);
    }

    DecLink_codecCreateDupObj(pObj);
    DecLink_codecCreatePrdObj(pObj);
    DecLink_codecStartPrdObj(pObj, DEC_LINK_PROCESS_DONE_PERIOD_MS);

    UTILS_MEMLOG_USED_END(pObj->memUsed);
    UTILS_MEMLOG_PRINT("DECLINK",
                       pObj->memUsed,
                       UTILS_ARRAYSIZE(pObj->memUsed));
    #ifdef SYSTEM_DEBUG_DEC
    Vps_printf(" %d: DECODE: Create ... DONE !!!\n",
            Utils_getCurTimeInMsec()
        );
    #endif

    #ifdef SYSTEM_DEBUG_MEMALLOC
    Vps_printf ("After DEC Create:\n");
    System_memPrintHeapStatus();
    #endif
    return FVID2_SOK;
}

static Int32 DecLink_codecQueueBufsToChQue(DecLink_Obj * pObj)
{
    UInt32 bufId, freeBufNum;
    Bitstream_Buf *pBuf;
    System_LinkInQueParams *pInQueParams;
    Bitstream_BufList bufList;
    DecLink_ChObj *pChObj;
    Int32 status;
    UInt32 curTime;

    pInQueParams = &pObj->createArgs.inQueParams;

    System_getLinksFullBufs(pInQueParams->prevLinkId,
                            pInQueParams->prevLinkQueId, &bufList);

    if (bufList.numBufs)
    {
        pObj->inBufGetCount += bufList.numBufs;

        freeBufNum = 0;
        curTime = Utils_getCurTimeInMsec();

        for (bufId = 0; bufId < bufList.numBufs; bufId++)
        {
            pBuf = bufList.bufs[bufId];

            pChObj = &pObj->chObj[pBuf->channelNum];

            pChObj->inFrameRecvCount++;

            // pBuf->fid = pChObj->nextFid;
            if(pChObj->disableChn && pChObj->skipFrame == FALSE)
            {
                pChObj->skipFrame = TRUE;
            }
            else if((pChObj->disableChn == FALSE) && pChObj->skipFrame)
            {
                if(pBuf->isKeyFrame == TRUE)
                {
                    pChObj->skipFrame = FALSE;
                }
            }

            if (((   ((pChObj->IFrameOnlyDecode) && (!pBuf->isKeyFrame))
                || pChObj->skipFrame
                || (pChObj->algCreateStatusLocal != DEC_LINK_ALG_CREATE_STATUS_CREATE_DONE)))
               &&
               (!pBuf->flushFrame))
            {
                pChObj->inBufSkipCount++;

                pChObj->inFrameUserSkipCount++;

                bufList.bufs[freeBufNum] = pBuf;
                freeBufNum++;
            }
            else
            {
                if ((pChObj->algCreateStatusLocal != DEC_LINK_ALG_CREATE_STATUS_CREATE_DONE)
                    &&
                    (TRUE == pBuf->flushFrame))
                {
                    DecLink_ReqObj *pReqObj = NULL;
                    UInt32 tskId;

                    UTILS_assert(Utils_queIsEmpty(&pChObj->inQue) == TRUE);
                    status =
                        Utils_queGet(&pObj->reqQue, (Ptr *) & pReqObj, 1,
                                     ti_sysbios_BIOS_NO_WAIT);
                    UTILS_assert(status == 0);
                    pReqObj->type = DEC_LINK_REQ_OBJECT_TYPE_DUMMY_FLUSHFRAME;
                    pReqObj->ivaSwitchSerializer = NULL;
                    pReqObj->InBuf = pBuf;
                    pReqObj->OutFrameList.numFrames = 0;
                    pReqObj->OutFrameList.frames[0] = NULL;

                    tskId = pObj->ch2ProcessTskId[pBuf->channelNum];

                    DECLINK_INFO_LOG(pObj->linkId,pBuf->channelNum,"Queing flush Frame to processQ");
                    status =
                    Utils_quePut(&pObj->decProcessTsk[tskId].processQue,
                                 pReqObj, ti_sysbios_BIOS_NO_WAIT);
                    UTILS_assert(status == FVID2_SOK);
                }
                else
                {
                    pChObj->totalInFrameCnt++;
                    if (pChObj->totalInFrameCnt > DEC_LINK_STATS_START_THRESHOLD)
                    {
                        pChObj->totalFrameIntervalTime +=
                            (curTime - pChObj->prevFrmRecvTime);
                    }
                    else
                    {
                        pChObj->totalFrameIntervalTime = 0;
                        pChObj->totalProcessTime = 0;

                        DecLink_resetStatistics(pObj);
                    }
                    pChObj->prevFrmRecvTime = curTime;

                    status = Utils_quePut(&pChObj->inQue, pBuf, BIOS_NO_WAIT);
                    UTILS_assert(status == FVID2_SOK);

                    pChObj->inBufQueCount++;
                }
            }
        }

        if (freeBufNum)
        {
            bufList.numBufs = freeBufNum;
            System_putLinksEmptyBufs(pInQueParams->prevLinkId,
                                     pInQueParams->prevLinkQueId, &bufList);
            pObj->inBufPutCount += freeBufNum;
        }
    }

    return FVID2_SOK;
}

static Int32 DecLink_codecSubmitData(DecLink_Obj * pObj)
{
    DecLink_ReqObj *pReqObj;
    DecLink_ChObj *pChObj;
    UInt32 chCount,chIdIndex, numProcessCh;
    Bitstream_Buf *pInBuf;
    FVID2_Frame *pOutFrame;
    Int32 status = FVID2_EFAIL, numReqObjPerProcess;
    UInt32 tskId, i;
    static UInt32 startChID = 0;

    System_FrameInfo *pOutFrameInfo;
    UInt32 curTime = Utils_getCurTimeInMsec();

    numProcessCh = 0;
    chIdIndex    = startChID;
    for (chCount = 0; chCount < pObj->inQueInfo.numCh; chCount++,chIdIndex++)
    {
      numReqObjPerProcess = 0;
      if (chIdIndex >= pObj->inQueInfo.numCh)
          chIdIndex = 0;
      pChObj = &pObj->chObj[chIdIndex];
      if (Utils_queIsEmpty(&pObj->outObj.bufOutQue.
                           emptyQue[pChObj->allocPoolID]))
      {
          pObj->newDataProcessOnFrameFree = TRUE;
      }

      while(numReqObjPerProcess < pChObj->numReqObjPerProcess) {
        numReqObjPerProcess++;
        status =
            Utils_queGet(&pObj->reqQue, (Ptr *) & pReqObj, 1,
                         BIOS_NO_WAIT);

        if (UTILS_ISERROR(status)) {
            break;
        }
        pReqObj->type = DEC_LINK_REQ_OBJECT_TYPE_REGULAR;
        pReqObj->ivaSwitchSerializer = NULL;

        tskId = pObj->ch2ProcessTskId[chIdIndex];
        
        if (pChObj->algObj.algCreateParams.fieldMergeDecodeEnable)
        {
           /* pReqObj->OutFrameList.numFrames should be set to 2 once         */
           /*  codec has support to consume 2 output pointers rather than     */
           /*  just one pointer with 2 contigous fields in field merged       */
           /*  interlaced decode use case.                                    */
            pReqObj->OutFrameList.numFrames = 1;
        }
        else
        {
            pReqObj->OutFrameList.numFrames = 1;
        }
        if ((status == FVID2_SOK) &&
            (Utils_queGetQueuedCount(&pChObj->inQue)) &&
            (Utils_queGetQueuedCount(&pObj->outObj.bufOutQue.emptyQue[pChObj->
                   allocPoolID]) >= pReqObj->OutFrameList.numFrames) &&
            !(Utils_queIsFull(&pObj->decProcessTsk[tskId].processQue)))
        {
            for (i=0; i<pReqObj->OutFrameList.numFrames; i++)
            {
                pOutFrame = NULL;
                status =
                    Utils_bufGetEmptyFrameExt(&pObj->outObj.bufOutQue,
                                              &pOutFrame,
                                              chIdIndex,
                                              BIOS_NO_WAIT);
                if (pOutFrame)
                {
                    declink_codec_init_outframe(pObj, chIdIndex, pOutFrame);
                    pReqObj->OutFrameList.frames[i] = pOutFrame;
                }
                else
                {
                    break;
                }
            }
            if ((status == FVID2_SOK) && (pOutFrame))
            {
                Utils_queGet(&pChObj->inQue, (Ptr *) & pInBuf, 1, BIOS_NO_WAIT);
                UTILS_assert(status == FVID2_SOK);
                if (pInBuf->flushFrame)
                {
                    pReqObj->type = DEC_LINK_REQ_OBJECT_TYPE_DUMMY_FLUSHFRAME;
                    DECLINK_INFO_LOG(pObj->linkId,pInBuf->channelNum,"Queing flush Frame to processQ");
                }
                pReqObj->InBuf = pInBuf;
                pChObj->inBufQueCount--;

                for (i=0; i<pReqObj->OutFrameList.numFrames; i++)
                {
                    pReqObj->OutFrameList.frames[i]->channelNum =
                                                     pInBuf->channelNum;
                    //pInBuf->timeStamp  = curTime;
                    pReqObj->OutFrameList.frames[i]->timeStamp=
                               pInBuf->timeStamp;


                    pOutFrameInfo = (System_FrameInfo *) pReqObj->OutFrameList.frames[i]->appData;
                    pOutFrameInfo->ts64  = (UInt32)pInBuf->upperTimeStamp;
                    pOutFrameInfo->ts64 <<= 32;
                    pOutFrameInfo->ts64  = pOutFrameInfo->ts64 | ((UInt32)pInBuf->lowerTimeStamp);
                    pOutFrameInfo->seqId = pInBuf->seqId;
                    pOutFrameInfo->isPlayBackChannel = TRUE;
                }
                numProcessCh++;
                status =
                    Utils_quePut(&pObj->decProcessTsk[tskId].processQue,
                                 pReqObj, BIOS_NO_WAIT);
                UTILS_assert(status == FVID2_SOK);
                pChObj->processReqestCount++;
            }
            else
            {
                status = Utils_quePut(&pObj->reqQue, pReqObj, BIOS_NO_WAIT);
                startChID = chIdIndex;
                UTILS_assert(status == FVID2_SOK);
                status = FVID2_EFAIL;
                continue;
            }
        }
        else
        {
            status = Utils_quePut(&pObj->reqQue, pReqObj, BIOS_NO_WAIT);
            UTILS_assert(status == FVID2_SOK);
            startChID = chIdIndex;
            status = FVID2_EFAIL;
            if (Utils_queIsEmpty(&pObj->outObj.bufOutQue.
                                 emptyQue[pChObj->allocPoolID]))
            {
                pObj->newDataProcessOnFrameFree = TRUE;
            }
        }
      }
    }

    return status;
}

Int32 DecLink_codecProcessData(DecLink_Obj * pObj)
{
    Int32 status;

    pObj->newDataProcessOnFrameFree = FALSE;
    DecLink_codecQueueBufsToChQue(pObj);

    do
    {
        status = DecLink_codecSubmitData(pObj);
    } while (status == FVID2_SOK);

    return FVID2_SOK;
}

static Int32 DecLink_codecGetProcessedData(DecLink_Obj * pObj)
{
    Bitstream_BufList inBufList;
    FVID2_FrameList outFrameList;
    FVID2_FrameList outFrameSkipList;
    UInt32 chId, sendCmd;
    System_LinkInQueParams *pInQueParams;
    DecLink_ChObj *pChObj;
    DecLink_ReqObj *pReqObj;
    Int32 status, j;
    UInt32 curTime;

    sendCmd = FALSE;
    inBufList.numBufs = 0;
    inBufList.appData = NULL;
    outFrameList.numFrames = 0;
    outFrameSkipList.numFrames = 0;
    curTime = Utils_getCurTimeInMsec();

    while(!Utils_queIsEmpty(&pObj->processDoneQue)
          &&
          (inBufList.numBufs < (VIDBITSTREAM_MAX_BITSTREAM_BUFS - 1))
          &&
          (outFrameList.numFrames < (FVID2_MAX_FVID_FRAME_PTR - 1)))
    {
        status = Utils_queGet(&pObj->processDoneQue, (Ptr *) & pReqObj, 1,
                              BIOS_NO_WAIT);
        if (status != FVID2_SOK)
        {
            break;
        }

        UTILS_assert(pReqObj->InBuf != NULL);
        chId = pReqObj->InBuf->channelNum;
        pChObj = &pObj->chObj[chId];

        //if (pChObj->totalInFrameCnt > DEC_LINK_STATS_START_THRESHOLD)
        {
            if (curTime > pReqObj->InBuf->timeStamp)
            {
                pChObj->totalProcessTime +=
                     (curTime - pReqObj->InBuf->timeStamp);
            }
        }

        pChObj->getProcessedBufCount++;

        pChObj->outFrameCount++;

        if (pReqObj->InBuf->flushFrame)
        {
            DECLINK_INFO_LOG(pObj->linkId,pReqObj->InBuf->channelNum,"Freeing flush Frame");
        }
        inBufList.bufs[inBufList.numBufs] = pReqObj->InBuf;
        inBufList.numBufs++;

        for (j = 0; j < pReqObj->OutFrameList.numFrames; j++)
        {
            if (pReqObj->OutFrameList.frames[j])
            {
                UTILS_assert(pReqObj->InBuf->channelNum ==
                             pReqObj->OutFrameList.frames[j]->channelNum);
                UTILS_assert(pChObj->allocPoolID < UTILS_BUF_MAX_ALLOC_POOLS);


                pChObj->trickPlayObj.skipFrame = Utils_doSkipFrame(&(pChObj->trickPlayObj.frameSkipCtx));

                if (pChObj->trickPlayObj.skipFrame == TRUE)
                {
                    /* Skip the output frame */
                    outFrameSkipList.frames[outFrameSkipList.numFrames] =
                                        pReqObj->OutFrameList.frames[j];
                    outFrameSkipList.numFrames++;
                }
                else
                {
                    outFrameList.frames[outFrameList.numFrames] =
                                        pReqObj->OutFrameList.frames[j];
                    outFrameList.numFrames++;
                }
            }
        }
        status = Utils_quePut(&pObj->reqQue, pReqObj, BIOS_NO_WAIT);
        UTILS_assert(status == FVID2_SOK);
    }

    if (outFrameList.numFrames)
    {
        status = Utils_bufPutFullExt(&pObj->outObj.bufOutQue,
                                     &outFrameList);
        UTILS_assert(status == FVID2_SOK);
        sendCmd = TRUE;
    }

    if (outFrameSkipList.numFrames)
    {
        status = DecLink_codecFreeProcessedFrames(pObj, &outFrameSkipList);
        UTILS_assert(status == DEC_LINK_S_SUCCESS);
    }

    if (inBufList.numBufs)
    {
        /* Free input frames */
        pInQueParams = &pObj->createArgs.inQueParams;
        System_putLinksEmptyBufs(pInQueParams->prevLinkId,
                                 pInQueParams->prevLinkQueId, &inBufList);
        pObj->inBufPutCount += inBufList.numBufs;
    }

    /* Send-out the output bitbuffer */
    if (sendCmd == TRUE)
    {
        System_sendLinkCmd(pObj->createArgs.outQueParams.nextLink,
                           SYSTEM_CMD_NEW_DATA);
    }

    return FVID2_SOK;
}

Int32 DecLink_codecGetProcessedDataMsgHandler(DecLink_Obj * pObj)
{
    Int32 status;

    status = DecLink_codecGetProcessedData(pObj);
    UTILS_assert(status == FVID2_SOK);

    return DEC_LINK_S_SUCCESS;

}

static
Int32 DecLink_codecFreeInQueuedBufs(DecLink_Obj * pObj)
{
    System_LinkInQueParams *pInQueParams;
    Bitstream_BufList bufList;

    pInQueParams = &pObj->createArgs.inQueParams;
    System_getLinksFullBufs(pInQueParams->prevLinkId,
                            pInQueParams->prevLinkQueId, &bufList);
    if (bufList.numBufs)
    {
        pObj->inBufGetCount += bufList.numBufs;
        /* Free input frames */
        System_putLinksEmptyBufs(pInQueParams->prevLinkId,
                                 pInQueParams->prevLinkQueId, &bufList);
        pObj->inBufPutCount += bufList.numBufs;
    }
    return DEC_LINK_S_SUCCESS;
}

Int32 DecLink_codecStop(DecLink_Obj * pObj)
{
    Int32 rtnValue = FVID2_SOK;
    UInt32 tskId;

       #ifdef SYSTEM_DEBUG_DEC
       Vps_printf(" %d: DECODE: Stop in progress !!!\n",
            Utils_getCurTimeInMsec()
        );
       #endif

    for (tskId = 0; tskId < NUM_HDVICP_RESOURCES; tskId++)
    {
        Utils_queUnBlock(&pObj->decProcessTsk[tskId].processQue);
    }
    while (!Utils_queIsFull(&pObj->reqQue))
    {
        Utils_tskWaitCmd(&pObj->tsk, NULL, DEC_LINK_CMD_GET_PROCESSED_DATA);
        DecLink_codecGetProcessedDataMsgHandler(pObj);
    }

    DecLink_codecFreeInQueuedBufs(pObj);

       #ifdef SYSTEM_DEBUG_DEC
       Vps_printf(" %d: DECODE: Stop Done !!!\n",
            Utils_getCurTimeInMsec()
        );
       #endif

    return (rtnValue);
}

static Int32 DecLink_codecDeleteChObj(DecLink_Obj * pObj, UInt32 chId)
{
    DecLink_ChObj *pChObj;
    Int32 status;

    pChObj = &pObj->chObj[chId];

    status = Utils_queDelete(&pChObj->inQue);
    UTILS_assert(status == FVID2_SOK);

    ti_sysbios_knl_Semaphore_destruct(&pChObj->codecProcessMutexMem);
    UTILS_assert(pChObj->codecProcessMutex != NULL);
    pChObj->codecProcessMutex = NULL;

    return FVID2_SOK;
}

static Int32 DecLink_codecDeleteChannel(DecLink_Obj * pObj, UInt32 chId)
{
    Int retVal = DEC_LINK_S_SUCCESS;
    DecLink_ChObj *pChObj;
    DecLink_OutObj *pOutObj;
    UInt32 frameId;
    Int32 status;

    UTILS_assert(chId <= pObj->inQueInfo.numCh);
    if (pObj->createArgs.chCreateParams[chId].algCreateStatus == 
                         DEC_LINK_ALG_CREATE_STATUS_CREATE_DONE)
    {
        pChObj = &pObj->chObj[chId];
        pOutObj = &pObj->outObj;

        switch (pChObj->algObj.algCreateParams.format)
        {
            case IVIDEO_H264BP:
            case IVIDEO_H264MP:
            case IVIDEO_H264HP:
                DecLinkH264_algDelete(&pChObj->algObj.u.h264AlgIfObj);
                break;

            case IVIDEO_MPEG4SP:
            case IVIDEO_MPEG4ASP:
                DecLinkMPEG4_algDelete(&pChObj->algObj.u.mpeg4AlgIfObj);
                break;

            case IVIDEO_MJPEG:
                DecLinkJPEG_algDelete(&pChObj->algObj.u.jpegAlgIfObj);
                break;

            default:
                retVal = DEC_LINK_E_UNSUPPORTEDCODEC;
                break;
        }
        UTILS_assert(retVal == DEC_LINK_S_SUCCESS);

        if (!pObj->createArgs.tilerEnable)
        {
            for (frameId = 0; frameId < pOutObj->outChObj[chId].outNumFrames; frameId++)
            {
                status = Utils_memFrameFree(&pOutObj->outChObj[chId].outFormat,
                               &pOutObj->outChObj[chId].allocFrames[frameId],
                               1);
                UTILS_assert(status == FVID2_SOK);
            }
        }
        pObj->createArgs.chCreateParams[chId].algCreateStatus = 
                         DEC_LINK_ALG_CREATE_STATUS_DELETE;
        pChObj->algCreateStatusLocal = DEC_LINK_ALG_CREATE_STATUS_DELETE;
        
        pObj->outObj.totalNumOutBufs -= pOutObj->outChObj[chId].outNumFrames;

        if (pChObj->totalInFrameCnt > DEC_LINK_STATS_START_THRESHOLD)
        {
            pChObj->totalInFrameCnt -= DEC_LINK_STATS_START_THRESHOLD;
        }
        #ifdef SYSTEM_VERBOSE_PRINTS
        Vps_printf(" %d: DECODE: CH%d: "
                    "Processed Frames : %8d, "
                    "Total Process Time : %8d, "
                    "Total Frame Interval: %8d, "
                    "Dropped Frames: %8d, "
                    "FPS: %8d \n",
                    Utils_getCurTimeInMsec(),
                    chId,
                    pChObj->totalInFrameCnt,
                    pChObj->totalProcessTime,
                    pChObj->totalFrameIntervalTime,
                    pChObj->inBufSkipCount,
                    (pChObj->totalInFrameCnt) /
                    (pChObj->totalFrameIntervalTime/1000)
                 );
        #endif
    }
    else
    {
        #ifdef SYSTEM_DEBUG_DEC
        Vps_printf("%d: DECODE: CodecInst and OutFrm bufs were NOT created for CH%d\n",
            Utils_getCurTimeInMsec(),
            chId            );
        #endif
    }

    return DEC_LINK_S_SUCCESS;
}

Int32 DecLink_codecDelete(DecLink_Obj * pObj)
{
    UInt32 outId, chId, tskId;
    DecLink_OutObj *pOutObj;

    #ifdef SYSTEM_DEBUG_DEC
    Vps_printf(" %d: DECODE: Delete in progress !!!\n",
            Utils_getCurTimeInMsec()
        );
    #endif

    pObj->state = SYSTEM_LINK_STATE_STOP;
    DecLink_codecStopPrdObj(pObj);
    for (tskId = 0; tskId < NUM_HDVICP_RESOURCES; tskId++)
    {
        DecLink_codecDeleteProcessTsk(pObj, tskId);
        Utils_queDelete(&pObj->decProcessTsk[tskId].processQue);
    }

    DecLink_codecUnregisterIVAMapCb(pObj);
    for (chId = 0; chId < pObj->inQueInfo.numCh; chId++)
    {
        DecLink_codecDeleteChObj(pObj, chId);
        DecLink_codecDeleteChannel(pObj, chId);
    }

    Utils_queDelete(&pObj->processDoneQue);
    DecLink_codecDeleteDupObj(pObj);
    DecLink_codecDeletePrdObj(pObj);

    for (outId = 0; outId < DEC_LINK_MAX_OUT_QUE; outId++)
    {
        pOutObj = &pObj->outObj;

        Utils_bufDeleteExt(&pOutObj->bufOutQue);
    }

    Utils_queDelete(&pObj->reqQue);

    DecLink_codecDeleteReqObjDummy(pObj);

    if (pObj->createArgs.tilerEnable)
    {
        SystemTiler_freeAll();
    }

    #ifdef SYSTEM_DEBUG_DEC
    Vps_printf(" %d: DECODE: Delete Done !!!\n",
            Utils_getCurTimeInMsec()
        );
    #endif

    return FVID2_SOK;
}

Int32 DecLink_resetStatistics(DecLink_Obj * pObj)
{
    UInt32 chId, tskId;
    DecLink_ChObj *pChObj;

    for (chId = 0; chId < pObj->inQueInfo.numCh; chId++)
    {
        pChObj = &pObj->chObj[chId];

        pChObj->inFrameRecvCount = 0;
        pChObj->inFrameUserSkipCount = 0;
        pChObj->outFrameCount = 0;
    }
    for ( tskId = 0; tskId < NUM_HDVICP_RESOURCES; tskId++)
    {
        pObj->batchStatistics[tskId].numBatchesSubmitted = 0;
        pObj->batchStatistics[tskId].aggregateBatchSize = 0;
        pObj->batchStatistics[tskId].averageBatchSize = 0;
        pObj->batchStatistics[tskId].maxAchievedBatchSize = 0;     
    }
    for ( tskId = 0; tskId < NUM_HDVICP_RESOURCES; tskId++)
    {
        pObj->debugBatchPrepStats[tskId].numBatchCreated = 0;
        pObj->debugBatchPrepStats[tskId].numReasonSizeExceeded = 0;
        pObj->debugBatchPrepStats[tskId].numReasonReqObjQueEmpty = 0;
        pObj->debugBatchPrepStats[tskId].numReasonResoultionClass = 0;
        pObj->debugBatchPrepStats[tskId].numReasonContentType = 0;
        pObj->debugBatchPrepStats[tskId].numReasonChannelRepeat = 0;
        pObj->debugBatchPrepStats[tskId].numReasonCodecSwitch = 0;
    }    
    pObj->statsStartTime = Utils_getCurTimeInMsec();

    return 0;
}

Int32 DecLink_printStatistics (DecLink_Obj * pObj, Bool resetAfterPrint)
{
    UInt32 chId, ivaId;
    DecLink_ChObj *pChObj;
    UInt32 elaspedTime;

    elaspedTime = Utils_getCurTimeInMsec() - pObj->statsStartTime; // in msecs
    elaspedTime /= 1000; // convert to secs

    Vps_printf( " \n"
            " *** DECODE Statistics *** \n"
            " \n"
            " Elasped Time           : %d secs\n"
            " \n"
            " \n"
            " CH  | In Recv In User  Out \n"
            " Num | FPS     Skip FPS FPS \n"
            " -----------------------------------\n",
            elaspedTime
            );

    for (chId = 0; chId < pObj->inQueInfo.numCh; chId++)
    {
        pChObj = &pObj->chObj[chId];

        Vps_printf( " %3d | %7d %8d %3d\n",
                chId,
            pChObj->inFrameRecvCount/elaspedTime,
            pChObj->inFrameUserSkipCount/elaspedTime,
            pChObj->outFrameCount/elaspedTime
             );
    }
    
    Vps_printf( " \n");
    
    Vps_printf("Multi Channel Decode Average Submit Batch Size \n");
    Vps_printf("Max Submit Batch Size : %d\n", DEC_LINK_GROUP_SUBM_MAX_SIZE);
    
    for (ivaId = 0; ivaId < NUM_HDVICP_RESOURCES; ivaId++)
    {
      Vps_printf ("IVAHD_%d Average Batch Size : %d\n", 
                  ivaId, pObj->batchStatistics[ivaId].averageBatchSize);
      Vps_printf ("IVAHD_%d Max achieved Batch Size : %d\n", 
                  ivaId, pObj->batchStatistics[ivaId].maxAchievedBatchSize);                  
    }
    
    Vps_printf( " \n");
    
    Vps_printf("Multi Channel Decode Batch break Stats \n");
    
    for (ivaId = 0; ivaId < NUM_HDVICP_RESOURCES; ivaId++)
    {
        Vps_printf ("Total Number of Batches created: %d \n", 
                    pObj->debugBatchPrepStats[ivaId].numBatchCreated);
        Vps_printf ("All numbers are based off total number of Batches created\n"); 
                  
        Vps_printf ("\t Batch breaks due to batch size"
                    "exceeding limit: %d %%\n", 
                    (pObj->debugBatchPrepStats[ivaId].numReasonSizeExceeded 
                    * 100)/pObj->debugBatchPrepStats[ivaId].numBatchCreated);
        Vps_printf ("\t Batch breaks due to ReqObj Que being empty"
                    ": %d %%\n", 
                    (pObj->debugBatchPrepStats[ivaId].numReasonReqObjQueEmpty 
                    * 100)/pObj->debugBatchPrepStats[ivaId].numBatchCreated);
        Vps_printf ("\t Batch breaks due to changed resolution class" 
                    ": %d %%\n", 
                    (pObj->debugBatchPrepStats[ivaId].numReasonResoultionClass 
                    * 100)/pObj->debugBatchPrepStats[ivaId].numBatchCreated);                    
        Vps_printf ("\t Batch breaks due to interlace and progressive"
                    "content mix: %d %%\n", 
                    (pObj->debugBatchPrepStats[ivaId].numReasonContentType 
                    * 100)/pObj->debugBatchPrepStats[ivaId].numBatchCreated);
        Vps_printf ("\t Batch breaks due to channel repeat"
                    ": %d %%\n", 
                    (pObj->debugBatchPrepStats[ivaId].numReasonChannelRepeat 
                    * 100)/pObj->debugBatchPrepStats[ivaId].numBatchCreated);
        Vps_printf ("\t Batch breaks due to different codec"
                    ": %d %%\n", 
                    (pObj->debugBatchPrepStats[ivaId].numReasonCodecSwitch 
                    * 100)/pObj->debugBatchPrepStats[ivaId].numBatchCreated);                    
    }    

    Vps_printf( " \n");

    if(resetAfterPrint)
    {
        DecLink_resetStatistics(pObj);
    }

   return FVID2_SOK;
}

static
Int32 DecLink_dupFrame(DecLink_Obj * pObj, FVID2_Frame * pOrgFrame,
                       FVID2_Frame ** ppDupFrame)
{
    Int status = DEC_LINK_S_SUCCESS;
    FVID2_Frame *pFrame;
    System_FrameInfo *pFrameInfo, *pOrgFrameInfo;

    status =
        Utils_queGet(&pObj->dupObj.dupQue, (Ptr *) & pFrame, 1, BIOS_NO_WAIT);
    UTILS_assert(status == FVID2_SOK);
    UTILS_assert(pFrame != NULL);
    pFrameInfo = (System_FrameInfo *) pFrame->appData;
    UTILS_assert(pFrameInfo != NULL);
    while (((System_FrameInfo *) pOrgFrame->appData)->pVdecOrgFrame != NULL)
    {
        pOrgFrame = ((System_FrameInfo *) pOrgFrame->appData)->pVdecOrgFrame;
    }
    pOrgFrameInfo = pOrgFrame->appData;
    memcpy(pFrame, pOrgFrame, sizeof(*pOrgFrame));
    pOrgFrameInfo = pOrgFrame->appData;
    memcpy(pFrameInfo, pOrgFrameInfo, sizeof(*pOrgFrameInfo));

    pFrame->appData = pFrameInfo;
    pFrameInfo->pVdecOrgFrame = pOrgFrame;
    UTILS_assert(pOrgFrameInfo->vdecRefCount <= DEC_LINK_MAX_DUP_PER_FRAME);
    pOrgFrameInfo->vdecRefCount++;
    *ppDupFrame = pFrame;

    return status;
}

Int32 DecLink_codecDisableChannel(DecLink_Obj * pObj,
                              DecLink_ChannelInfo* params)
{
    Int32 status = DEC_LINK_S_SUCCESS;
    DecLink_ChObj *pChObj;
    UInt key;

    key = Hwi_disable();
    pChObj = &pObj->chObj[params->chId];
    pChObj->disableChn = TRUE;
    Hwi_restore(key);

    return (status);
}

Int32 DecLink_codecEnableChannel(DecLink_Obj * pObj,
                              DecLink_ChannelInfo* params)
{
    Int32 status = DEC_LINK_S_SUCCESS;
    DecLink_ChObj *pChObj;
    UInt key;

    key = Hwi_disable();
    pChObj = &pObj->chObj[params->chId];
    pChObj->disableChn = FALSE;
    Hwi_restore(key);

    return (status);
}


Int32 DecLink_setTPlayConfig(DecLink_Obj * pObj,
                              DecLink_TPlayConfig* params)
{
    Int32 status = DEC_LINK_S_SUCCESS;
    DecLink_ChObj *pChObj;
    UInt key;

    key = Hwi_disable();
    pChObj = &pObj->chObj[params->chId];
    pChObj->trickPlayObj.frameSkipCtx.inputFrameRate = params->inputFps;
    pChObj->trickPlayObj.frameSkipCtx.outputFrameRate = params->targetFps;
    pChObj->trickPlayObj.frameSkipCtx.firstTime = TRUE;

    Vps_printf("\r\n DecLink_setTPlayConfig : Ch :%d InputputFrameRate :%d, trickPlay outputFrameRate: %d ",
         params->chId,
         pChObj->trickPlayObj.frameSkipCtx.inputFrameRate,
         pChObj->trickPlayObj.frameSkipCtx.outputFrameRate);

    Hwi_restore(key);

    return (status);
}

static Int32 DecLink_PrepareBatch  (DecLink_Obj *pObj, UInt32 tskId, 
                                   DecLink_ReqObj *pReqObj, 
                                   DecLink_ReqBatch *pReqObjBatch)
{
  Int32 channelId, newObjChannelId, codecClassSwitch = 0;
  Int32 status = FVID2_SOK;
  UInt32 contentType;
#ifdef SYSTEM_DEBUG_MULTI_CHANNEL_DEC
  Int32 i;
#endif

  Bool batchPreperationDone = FALSE;
  DecLink_ReqObj *pNewReqObj;
  DecLink_ChObj *pChObj;
  Int32 maxBatchSize;

  /*Reset the submitted flag at the start of batch preperation*/
  pReqObjBatch->channelSubmittedFlag = 0x0;
  pReqObjBatch->codecSubmittedFlag = 0x0;
  pReqObjBatch->numReqObjsInBatch = 0;

  pReqObjBatch->pReqObj[pReqObjBatch->numReqObjsInBatch++] = pReqObj;

  channelId = pReqObj->InBuf->channelNum;
  
  contentType =
           Utils_encdecMapFVID2XDMContentType(pObj->inQueInfo.chInfo[channelId].
                                              scanFormat);  
  pChObj = &pObj->chObj[channelId];
  pChObj->numBufsInCodec += pReqObj->OutFrameList.numFrames;
  
  /*Since this is the first ReqList in the Batch, the channel submit and codec 
     submit bits wont have been set a-priori.*/
  pReqObjBatch->channelSubmittedFlag = pReqObjBatch->channelSubmittedFlag | 
                                        (0x1 << channelId);
  
  maxBatchSize = DEC_LINK_GROUP_SUBM_MAX_SIZE;
  
  switch (pObj->outObj.outChObj[channelId].reslutionClass)
  {
      case UTILS_ENCDEC_RESOLUTION_CLASS_16MP:
      case UTILS_ENCDEC_RESOLUTION_CLASS_WUXGA:
      case UTILS_ENCDEC_RESOLUTION_CLASS_1080P:
          /*If the element has resolution between 1080p and 720p, 
            only 1 channels possible per Batch*/
          maxBatchSize = 1;
          break;
      
      case UTILS_ENCDEC_RESOLUTION_CLASS_720P:
      case UTILS_ENCDEC_RESOLUTION_CLASS_D1:
      case UTILS_ENCDEC_RESOLUTION_CLASS_CIF:
          /*If the element has resolution anything <= 720, Batch can have 
            a max of 24 channels*/
          maxBatchSize = MIN (24,DEC_LINK_GROUP_SUBM_MAX_SIZE);
          break;

      default:
          UTILS_assert(FALSE);
          break;
  }

  /*Set the flag for which codec class this REqObject belongs to.*/
  switch (pChObj->algObj.algCreateParams.format)
  {
      case IVIDEO_H264BP:
      case IVIDEO_H264MP:
      case IVIDEO_H264HP:
          pReqObjBatch->codecSubmittedFlag = pReqObjBatch->codecSubmittedFlag |
                                              (0x1 << 
                                               DEC_LINK_GROUP_CODEC_CLASS_H264);
          break;
      case IVIDEO_MJPEG:
          pReqObjBatch->codecSubmittedFlag = pReqObjBatch->codecSubmittedFlag |
                                              (0x1 << 
                                               DEC_LINK_GROUP_CODEC_CLASS_JPEG);
          break;
      case IVIDEO_MPEG4ASP:
      case IVIDEO_MPEG4SP:
          pReqObjBatch->codecSubmittedFlag = pReqObjBatch->codecSubmittedFlag |
                                              (0x1 << 
                                               DEC_LINK_GROUP_CODEC_CLASS_MPEG4);
          break;
      default:
          UTILS_assert(FALSE);
  }
                                        
  while (FALSE == batchPreperationDone)  
  {
    if (pReqObjBatch->numReqObjsInBatch >= 
        MIN (DEC_LINK_GROUP_SUBM_MAX_SIZE, maxBatchSize))
    {
      /*The number of Request Lists has exceeded the maximum batch size 
        supported be the codec.*/
      #ifdef SYSTEM_DEBUG_MULTI_CHANNEL_DEC    
      Vps_printf ("DEC : IVAHDID : %d Number of Req Objs exceeded limit: %d\n", tskId,
                  maxBatchSize);
      #endif   
      pObj->debugBatchPrepStats[tskId].numReasonSizeExceeded++;
      batchPreperationDone = TRUE;
      continue;
    }
    
    if (Utils_queIsEmpty (&pObj->decProcessTsk[tskId].processQue))
    {
      /*There are no more request Objects to be dequeued. Batch generation done*/
      #ifdef SYSTEM_DEBUG_MULTI_CHANNEL_DEC      
      Vps_printf ("DEC : IVAHDID : %d Incoming Queue is empty. Batch generation complete!!",
                  tskId);
      #endif
      pObj->debugBatchPrepStats[tskId].numReasonReqObjQueEmpty++;
      batchPreperationDone = TRUE;
      continue;

    }
    
    /*Peek at the next Request Obj in the Process Queue*/    
    status = Utils_quePeek(&pObj->decProcessTsk[tskId].processQue,
                           (Ptr *) &pNewReqObj);
    
    if (status != FVID2_SOK)
      return status;                           
    
    if (pNewReqObj->type != DEC_LINK_REQ_OBJECT_TYPE_REGULAR)
    {
        batchPreperationDone = TRUE;
        continue;
    }
    
    newObjChannelId = pNewReqObj->InBuf->channelNum;

    if ((UTILS_ENCDEC_RESOLUTION_CLASS_16MP ==
         pObj->outObj.outChObj[newObjChannelId].reslutionClass) ||
        (UTILS_ENCDEC_RESOLUTION_CLASS_WUXGA ==
         pObj->outObj.outChObj[newObjChannelId].reslutionClass) ||
        (UTILS_ENCDEC_RESOLUTION_CLASS_1080P ==
         pObj->outObj.outChObj[newObjChannelId].reslutionClass))
    {
        /*If the element has resolution between 1080p and 720p, 
          only 1 channels possible per Batch. As we already have an element in, 
          break out*/
        batchPreperationDone = TRUE;
        pObj->debugBatchPrepStats[tskId].numReasonResoultionClass++;
        continue;
    }
    else if ((UTILS_ENCDEC_RESOLUTION_CLASS_720P ==
              pObj->outObj.outChObj[newObjChannelId].reslutionClass) ||
             (UTILS_ENCDEC_RESOLUTION_CLASS_D1 == 
              pObj->outObj.outChObj[newObjChannelId].reslutionClass) ||
             (UTILS_ENCDEC_RESOLUTION_CLASS_CIF ==  
              pObj->outObj.outChObj[newObjChannelId].reslutionClass))
    {
        /*If the element has resolution <=720P, Batch can have 
          max 24 channels*/
        if (pReqObjBatch->numReqObjsInBatch >= 
            MIN (24,DEC_LINK_GROUP_SUBM_MAX_SIZE))
        {
            batchPreperationDone = TRUE;
            pObj->debugBatchPrepStats[tskId].numReasonResoultionClass++;
            continue;      
        }
        else
        {
            maxBatchSize = MIN (maxBatchSize, 24);  
        }
    }
    else
    {
      UTILS_assert(FALSE);
    }

    /*If the new element's Content type doesnt match that of the first ReqObj,
      stop the Batch preperation. Each batch can have either all progressive or
      all interlaced channels*/ 
    if (contentType != Utils_encdecMapFVID2XDMContentType(pObj->inQueInfo.chInfo
                                                          [newObjChannelId].
                                                          scanFormat))
    {
      #ifdef SYSTEM_DEBUG_MULTI_CHANNEL_DEC      
      Vps_printf ("DEC : IVAHDID : %d Interlaced or Progressive switch happened!!\n", 
                  tskId);
      #endif
      batchPreperationDone = TRUE;
      pObj->debugBatchPrepStats[tskId].numReasonContentType++;
      continue;     
    }
    
    /*Check if the channel has already been inserted in the batch*/      
    if (pReqObjBatch->channelSubmittedFlag & (0x1 << newObjChannelId))
    {
      /*Codec doesnt support multiple entries of the same channel in the same
        multi process call. So the batch generation ends here.*/
      #ifdef SYSTEM_DEBUG_MULTI_CHANNEL_DEC      
      Vps_printf ("DEC : IVAHDID : %d Channel repeated within Batch!!\n", tskId);
      #endif           
      batchPreperationDone = TRUE;
      pObj->debugBatchPrepStats[tskId].numReasonChannelRepeat++;
      continue;    
    }
    else
    {
      /*This is a new channel so set the bit for this channel*/
      pReqObjBatch->channelSubmittedFlag = pReqObjBatch->channelSubmittedFlag |
                                           (0x1 << newObjChannelId);
    }

    /*Check if there is a codec switch. If yes, batch generation is completed.*/
    pChObj = &pObj->chObj[newObjChannelId];
    
    /*Check the flag for which codec class this Request Object belongs to.*/
    switch (pChObj->algObj.algCreateParams.format)
    {
        case IVIDEO_H264BP:
        case IVIDEO_H264MP:
        case IVIDEO_H264HP:
            codecClassSwitch = pReqObjBatch->codecSubmittedFlag &
                                                (0x1 << 
                                                 DEC_LINK_GROUP_CODEC_CLASS_H264);
            break;
        case IVIDEO_MJPEG:
            codecClassSwitch = pReqObjBatch->codecSubmittedFlag &
                                                (0x1 << 
                                                 DEC_LINK_GROUP_CODEC_CLASS_JPEG);
            break;
        case IVIDEO_MPEG4ASP:
        case IVIDEO_MPEG4SP:
            codecClassSwitch = pReqObjBatch->codecSubmittedFlag &
                                                (0x1 << 
                                                 DEC_LINK_GROUP_CODEC_CLASS_MPEG4);        
             break;
        default:
            UTILS_assert(FALSE);
    }    
    
    if (! codecClassSwitch)
    {
      /*A codec switch from JPEG to H264 or vice-versa has happened and this 
        Request object cannot be part of the batch to be submitted. 
        Batch generation done.*/
      #ifdef SYSTEM_DEBUG_MULTI_CHANNEL_DEC      
      Vps_printf ("DEC : IVAHDID : %d Codec Switch occured!!. Batch generation complete!!",
                  tskId);
      #endif         
      batchPreperationDone = TRUE;
      pObj->debugBatchPrepStats[tskId].numReasonCodecSwitch++;
      continue;      
    }
    
    /*Now that the Request Obj is eligible to be part of the Batch, include it.
     */
    status = Utils_queGet(&pObj->decProcessTsk[tskId].processQue,
                          (Ptr *) &pNewReqObj, 1, BIOS_WAIT_FOREVER);     

    UTILS_assert(pNewReqObj->type == DEC_LINK_REQ_OBJECT_TYPE_REGULAR);
    pReqObjBatch->pReqObj[pReqObjBatch->numReqObjsInBatch++] = pNewReqObj;
    pChObj->numBufsInCodec += pNewReqObj->OutFrameList.numFrames;
      
  }
  
  if (FVID2_SOK == status)
  {
    /*Print Debug details of the Batch created*/
    #ifdef SYSTEM_DEBUG_MULTI_CHANNEL_DEC
    Vps_printf("DEC : IVAHDID : %d Batch creation ... DONE !!!\n", tskId);
    Vps_printf("DEC : IVAHDID : %d Number of Req Objs in Batch : %d\n", 
               tskId, pReqObjBatch->numReqObjsInBatch);
    Vps_printf ("DEC : IVAHDID : %d Channels included in Batch:\n", tskId);
    for ( i = 0; i < pReqObjBatch->numReqObjsInBatch; i++)
    {
      Vps_printf ("DEC : IVAHDID : %d %d\n", tskId, 
                  pReqObjBatch->pReqObj[i]->InBuf->channelNum);
    }

    #endif    
  }
  
  pObj->debugBatchPrepStats[tskId].numBatchCreated++;
  return status;
}

Int32 DecLink_printBufferStatus (DecLink_Obj * pObj)
{
    Uint8 str[256];
    
    Vps_rprintf(
        " \n"
        " *** Decode Statistics *** \n"
        "\n %d: DEC: Rcvd from prev = %d, Returned to prev = %d\r\n",
         Utils_getCurTimeInMsec(), pObj->inBufGetCount, pObj->inBufPutCount);

    sprintf ((char *)str, "DEC Out ");
    Utils_bufExtPrintStatus(str, &pObj->outObj.bufOutQue);
    return 0;
}

Int32 DecLink_getBufferStatus (DecLink_Obj * pObj,DecLink_BufferStats *bufStats)
{
    Int i;
    UInt32 chId;

    /* First queue all input frames to channel specific queu */
    DecLink_codecQueueBufsToChQue(pObj);
    for (i = 0; i < bufStats->numCh; i++)
    {
        chId = bufStats->chId[i];
        if (chId < UTILS_ARRAYSIZE(pObj->chObj))
        {
            bufStats->stats[chId].numInBufQueCount = Utils_queGetQueuedCount(&pObj->chObj[chId].inQue);
            bufStats->stats[chId].numOutBufQueCount =
             pObj->outObj.outChObj[chId].outNumFrames -
             Utils_queGetQueuedCount(&pObj->outObj.bufOutQue.emptyQue[chId]);
        }
    }
    return 0;
}

static Int32 decLink_map_displayDelay2CodecParam(Int32 displayDelay)
{
    Int32 VdecMaxDisplayDelay;

    if ((displayDelay >= IVIDDEC3_DISPLAY_DELAY_AUTO)
        &&
        (displayDelay <= IVIDDEC3_DISPLAY_DELAY_16))
    {
        VdecMaxDisplayDelay =  displayDelay;
    }
    else
    {
       Vps_printf("DECLINK: Invalid param passed for MaxDisplayDelay param [%d] "
                  "Forcing to default value [%d]\n",
                  displayDelay,
                  IVIDDEC3_DECODE_ORDER);
       VdecMaxDisplayDelay =  IVIDDEC3_DECODE_ORDER;
    }

    return VdecMaxDisplayDelay;
}

Int32 DecLink_codecCreateChannelHandler(DecLink_Obj * pObj,
                                        DecLink_addChannelInfo* params)
{
    Int32 status = DEC_LINK_S_SUCCESS;
    DecLink_ChObj *pChObj;
    UInt32 chId;
    UInt key;

    chId = params->chId;
    UTILS_assert(chId <= pObj->inQueInfo.numCh);
    pChObj = &pObj->chObj[chId];
    UTILS_assert(params->createPrm.algCreateStatus == 
                         DEC_LINK_ALG_CREATE_STATUS_CREATE);

    if (pObj->createArgs.chCreateParams[chId].algCreateStatus == 
                         DEC_LINK_ALG_CREATE_STATUS_CREATE_DONE)
    {
        Vps_printf("DECLINK: ERROR!!! During Channel Open, "
                   "CH%d is alreday in open state (opened earlier): "
                   "close first and try open the channel again \n ", chId);
        return (status);
    }

    key = Hwi_disable();
    pChObj->algCreateStatusLocal = DEC_LINK_ALG_CREATE_STATUS_CREATE;
    memcpy (&pObj->inQueInfo.chInfo[chId], &params->chInfo, 
                                           sizeof (System_LinkChInfo));
    memcpy(&pObj->createArgs.chCreateParams[chId], &params->createPrm, 
                                           sizeof(DecLink_ChCreateParams));

    UTILS_assert(Utils_queGetQueuedCount(
                 &pObj->outObj.bufOutQue.emptyQue[chId]) == 0);
    status = Utils_queReset(&pObj->outObj.bufOutQue.emptyQue[chId]);
    UTILS_assert(status == FVID2_SOK);
    Hwi_restore(key);

    status = DecLink_codecCreateChannel(pObj, chId);
    if (status == DEC_LINK_S_SUCCESS)
    {
        key = Hwi_disable();
        pChObj->algCreateStatusLocal = DEC_LINK_ALG_CREATE_STATUS_CREATE_DONE;
        Hwi_restore(key);
        #ifdef SYSTEM_DEBUG_DEC
        Vps_printf(" %d: DECODE: CH%d: Decoder Create CH done!!!\n",
            Utils_getCurTimeInMsec(), chId );
        #endif
    }
    else
    {
        DecLink_ChannelInfo params;
        params.chId = chId;
        DecLink_codecDeleteChannelHandler(pObj, &params);
        Vps_printf("DECLINK: ERROR!!! During Channel Open, "
                   "Need more memory to open new CH%d; "
                   "Delete a few other channels and try again \n ", chId);
    }

    return (status);
}

Int32 DecLink_codecDeleteChannelHandler(DecLink_Obj * pObj,
                                        DecLink_ChannelInfo* params)
{
    Int32 status = DEC_LINK_S_SUCCESS;
    UInt32 chId, tskId, frameId;
    DecLink_ChObj *pChObj;
    System_FrameInfo *pFrameInfo;
    DecLink_ReqObj *pReqObj;
    Bitstream_Buf *pInBuf;
    Bitstream_BufList inBufList;
    System_LinkInQueParams *pInQueParams;
    UInt key;

    chId = params->chId;
    UTILS_assert(chId <= pObj->inQueInfo.numCh);
    pChObj = &pObj->chObj[chId];
    tskId = pObj->ch2ProcessTskId[chId];

    key = Hwi_disable();
    pChObj->algCreateStatusLocal = DEC_LINK_ALG_CREATE_STATUS_DELETE;
    for (frameId = 0; frameId < pObj->outObj.outChObj[chId].outNumFrames; frameId++)
    {
        pFrameInfo = (System_FrameInfo *)
                      pObj->outObj.outChObj[chId].outFrames[frameId]->appData;
        pFrameInfo->invalidFrame = TRUE;
    }
    Hwi_restore(key);

    inBufList.numBufs = 0;
    while (Utils_queGetQueuedCount(&pChObj->inQue))
    {
        Utils_queGet(&pChObj->inQue, (Ptr *) & pInBuf, 1, BIOS_NO_WAIT);
        UTILS_assert(status == FVID2_SOK);
        inBufList.bufs[inBufList.numBufs] = pInBuf;
        inBufList.numBufs++;
        if (inBufList.numBufs > VIDBITSTREAM_MAX_BITSTREAM_BUFS/2)
        {
            /* Free input frames */
            pInQueParams = &pObj->createArgs.inQueParams;
            System_putLinksEmptyBufs(pInQueParams->prevLinkId,
                                     pInQueParams->prevLinkQueId, &inBufList);
            pObj->inBufPutCount += inBufList.numBufs;
            inBufList.numBufs = 0;
        }
    }

    if (inBufList.numBufs)
    {
        /* Free input frames */
        pInQueParams = &pObj->createArgs.inQueParams;
        System_putLinksEmptyBufs(pInQueParams->prevLinkId,
                                 pInQueParams->prevLinkQueId, &inBufList);
        pObj->inBufPutCount += inBufList.numBufs;
        inBufList.numBufs = 0;
    }

    status = Utils_queGet(&pObj->decDummyReqObj.reqQueDummy, (Ptr *) & pReqObj, 1,
                          BIOS_NO_WAIT);
    UTILS_assert(status == FVID2_SOK);

    UTILS_assert(UTILS_ARRAYISVALIDENTRY(pReqObj,pObj->decDummyReqObj.reqObjDummy));
    pChObj->dummyBitBuf.channelNum = chId;
    pReqObj->InBuf = &pChObj->dummyBitBuf;
    pReqObj->type = DEC_LINK_REQ_OBJECT_TYPE_DUMMY_CHDELETE;
    pReqObj->ivaSwitchSerializer = NULL;
    status = Utils_quePut(&pObj->decProcessTsk[tskId].processQue,
                           pReqObj, BIOS_NO_WAIT);
    UTILS_assert(status == FVID2_SOK);
    Vps_printf("DEC : Delete CH%d, Dummy Object queued !!!\n", chId);

    return (status);
}

static Int32 DecLink_codecFlushNDeleteChannel(DecLink_Obj * pObj, UInt32 chId)
{
    Int32 status = DEC_LINK_S_SUCCESS;
    DecLink_ChObj *pChObj;
    System_FrameInfo *pFrameInfo;
    FVID2_Frame  *fvidFrm;
    FVID2_FrameList freeFrameList;

    freeFrameList.numFrames = 0;
    if (pObj->createArgs.chCreateParams[chId].algCreateStatus == 
                         DEC_LINK_ALG_CREATE_STATUS_CREATE_DONE)
    {
        pChObj = &pObj->chObj[chId];

        switch (pChObj->algObj.algCreateParams.format)
        {
            case IVIDEO_H264BP:
            case IVIDEO_H264MP:
            case IVIDEO_H264HP:
                DecLinkH264_codecFlush(pChObj, 
                                       &pChObj->algObj.u.h264AlgIfObj.inArgs, 
                                       &pChObj->algObj.u.h264AlgIfObj.outArgs, 
                                       &pChObj->algObj.u.h264AlgIfObj.inBufs, 
                                       &pChObj->algObj.u.h264AlgIfObj.outBufs, 
                                       pChObj->algObj.u.h264AlgIfObj.algHandle, 
                                       &freeFrameList, TRUE);
                break;

            case IVIDEO_MPEG4SP:
            case IVIDEO_MPEG4ASP:
                DecLinkMPEG4_codecFlush(pChObj, 
                                        &pChObj->algObj.u.mpeg4AlgIfObj.inArgs, 
                                        &pChObj->algObj.u.mpeg4AlgIfObj.outArgs, 
                                        &pChObj->algObj.u.mpeg4AlgIfObj.inBufs, 
                                        &pChObj->algObj.u.mpeg4AlgIfObj.outBufs, 
                                        pChObj->algObj.u.mpeg4AlgIfObj.algHandle, 
                                        &freeFrameList, TRUE);
                break;

            case IVIDEO_MJPEG:
                break;

            default:
                status = DEC_LINK_E_UNSUPPORTEDCODEC;
                break;
        }
        UTILS_assert(status == DEC_LINK_S_SUCCESS);
    }

    if (freeFrameList.numFrames)
    {
        DecLink_codecFreeProcessedFrames(pObj, &freeFrameList);
    }

    DecLink_codecDeleteChannel(pObj, chId);

    while (Utils_queGetQueuedCount(&pObj->outObj.bufOutQue.emptyQue[chId]))
    {
        status = Utils_queGet(&pObj->outObj.bufOutQue.emptyQue[chId],
                             (Ptr *)&fvidFrm,1,BIOS_NO_WAIT);
        UTILS_assert(status == FVID2_SOK);
        pFrameInfo = (System_FrameInfo *) fvidFrm->appData;
        UTILS_assert(pFrameInfo != NULL);
        UTILS_assert(pFrameInfo->invalidFrame == TRUE);
        pFrameInfo->invalidFrame = FALSE;
        status = Utils_quePut(&pObj->outObj.outChObj[chId].fvidFrmQue, 
                              fvidFrm, BIOS_NO_WAIT);
        UTILS_assert(status == FVID2_SOK);
    }

    status = System_sendLinkCmd(pObj->linkId, DEC_LINK_CMD_LATE_ACK);

    if (UTILS_ISERROR(status))
    {
        UTILS_warn("DECLINK:[%s:%d]:"
                   "System_sendLinkCmd DEC_LINK_CMD_LATE_ACK failed"
                   "errCode = %d", __FILE__, __LINE__, status);
    }

    return (status);
}

Int32 DecLink_resetDecErrorReporting(DecLink_Obj * pObj, DecLink_ChErrorReport * decErrReportCtrl)
{
    Int32 status = DEC_LINK_S_SUCCESS;

    /* Enable/Disable reporting of Decoder error to A8 */
    pObj->chObj[decErrReportCtrl->chId].decErrorMsg.reportA8 = decErrReportCtrl->enableErrReport;

    return (status);
}


static Int32 DecLink_codecCreateIvaMapMutex(DecLink_Obj * pObj)
{
    ti_sysbios_gates_GateMutexPri_Params prms;

    UTILS_assert(pObj->ivaChMapMutex == NULL);

    ti_sysbios_gates_GateMutexPri_Params_init(&prms);
    ti_sysbios_gates_GateMutexPri_construct(&pObj->ivaChMapMutexObj,
                                            &prms);
    pObj->ivaChMapMutex =
    ti_sysbios_gates_GateMutexPri_handle(&pObj->ivaChMapMutexObj);

    return (DEC_LINK_S_SUCCESS);
}

static Int32 DecLink_codecDeleteIvaMapMutex(DecLink_Obj * pObj)
{
    UTILS_assert(pObj->ivaChMapMutex != NULL);

    ti_sysbios_gates_GateMutexPri_destruct(&pObj->ivaChMapMutexObj);
    pObj->ivaChMapMutex = NULL;
    return (DEC_LINK_S_SUCCESS);
}

static Int32 DecLink_codecCreateIvaSwitchSerializerObj(DecLink_Obj * pObj)
{
    Int32 status;
    Int i;

    UTILS_COMPILETIME_ASSERT(UTILS_ARRAYSIZE(pObj->decIVASwitchSerializeObj.freeSerializerQueMem)
                             ==
                             UTILS_ARRAYSIZE(pObj->decIVASwitchSerializeObj.freeSerializerSemMem));
    status =
    Utils_queCreate(&pObj->decIVASwitchSerializeObj.freeSerializerQue,
                    UTILS_ARRAYSIZE(pObj->decIVASwitchSerializeObj.freeSerializerQueMem),
                    pObj->decIVASwitchSerializeObj.freeSerializerQueMem,
                    UTILS_QUE_FLAG_NO_BLOCK_QUE);
    UTILS_assert(status == 0);
    for (i = 0; i < UTILS_ARRAYSIZE(pObj->decIVASwitchSerializeObj.freeSerializerSemMem); i++)
    {
        ti_sysbios_knl_Semaphore_Params prms;

        ti_sysbios_knl_Semaphore_Params_init(&prms);
        ti_sysbios_knl_Semaphore_construct(&pObj->decIVASwitchSerializeObj.freeSerializerSemMem[i],
                                           0,
                                           &prms);
       status = Utils_quePut(&pObj->decIVASwitchSerializeObj.freeSerializerQue,
                             &pObj->decIVASwitchSerializeObj.freeSerializerSemMem[i],
                             ti_sysbios_BIOS_NO_WAIT);
       UTILS_assert(status == 0);
    }

    return (DEC_LINK_S_SUCCESS);
}

static Int32 DecLink_codecDeleteIvaSwitchSerializerObj(DecLink_Obj * pObj)
{
    Int32 status;
    Int i;

    UTILS_COMPILETIME_ASSERT(UTILS_ARRAYSIZE(pObj->decIVASwitchSerializeObj.freeSerializerQueMem)
                             ==
                             UTILS_ARRAYSIZE(pObj->decIVASwitchSerializeObj.freeSerializerSemMem));

    UTILS_assert(Utils_queIsFull(&pObj->decIVASwitchSerializeObj.freeSerializerQue) == TRUE);
    status = Utils_queDelete(&pObj->decIVASwitchSerializeObj.freeSerializerQue);
    UTILS_assert(status == 0);
    for (i = 0; i < UTILS_ARRAYSIZE(pObj->decIVASwitchSerializeObj.freeSerializerSemMem); i++)
    {
        ti_sysbios_knl_Semaphore_destruct(&pObj->decIVASwitchSerializeObj.freeSerializerSemMem[i]);
    }
    return (DEC_LINK_S_SUCCESS);
}


static Void DecLink_ivaMapUpdate(DecLink_Obj * pObj,SystemVideo_Ivahd2ChMap_Tbl* tbl)
{
    IArg key;
    UInt numHDVICP = HDVICP2_GetNumberOfIVAs();
    Int ivaIdx,decChIdx;
    UInt32 decCh;
    Bits32 reconfig_detect_mask[(DEC_LINK_MAX_CH/32) + 1];

    UTILS_assert(numHDVICP <= UTILS_ARRAYSIZE(tbl->ivaMap));
    UTILS_assert(pObj->inQueInfo.numCh <= DEC_LINK_MAX_CH);

    key = GateMutexPri_enter(pObj->ivaChMapMutex);
    memset(reconfig_detect_mask,0,sizeof(reconfig_detect_mask));
    for (ivaIdx = 0; ivaIdx < numHDVICP; ivaIdx++)
    {
        for (decChIdx = 0 ; decChIdx < tbl->ivaMap[ivaIdx].DecNumCh; decChIdx++)
        {
            decCh = tbl->ivaMap[ivaIdx].DecChList[decChIdx];
            if ((decCh < pObj->inQueInfo.numCh)
                &&
                ((reconfig_detect_mask[decCh/32u] & (1u << (decCh % 32))) == 0))
            {
                /* If channel number is valid and has not been previously mapped to
                 * another IVA allow reconfiguration of IVA map
                 */
                pObj->ch2ProcessTskIdNext[decCh] = ivaIdx;
                reconfig_detect_mask[decCh/32u] |= 1u << (decCh % 32);
            }
            else
            {
                if (decCh < pObj->inQueInfo.numCh)
                {
                    UTILS_warn("DECLINK:WARNING!!!. Invalid ch2IVAMap. IVAID:%d,ChId:%d",
                               ivaIdx,decCh);
                }
            }
        }
    }
    GateMutexPri_leave(pObj->ivaChMapMutex,key);
}


static Void DecLink_ivaMapChangeCbFxn(Ptr ctx,SystemVideo_Ivahd2ChMap_Tbl* tbl)
{
   DecLink_Obj * pObj = ctx;

   UTILS_assert((pObj->linkId >= SYSTEM_LINK_ID_VDEC_START)
                &&
                (pObj->linkId <= SYSTEM_LINK_ID_VDEC_END));

   DecLink_ivaMapUpdate(pObj,tbl);
   Utils_tskSendCmd(&pObj->tsk,DEC_LINK_CMD_INTERNAL_IVAMAPCHANGE);
}

static
Void DecLink_codecRegisterIVAMapCb(DecLink_Obj * pObj)
{
    Utils_encdecIVAMapChangeNotifyCbInfo cbInfo;

    DecLink_codecCreateIvaMapMutex(pObj);
    DecLink_codecCreateIvaSwitchSerializerObj(pObj);
    cbInfo.ctx = pObj;
    cbInfo.fxns = &DecLink_ivaMapChangeCbFxn;
    Utils_encdecRegisterIVAMapChangeNotifyCb(&cbInfo);
}

static
Void DecLink_codecUnregisterIVAMapCb(DecLink_Obj * pObj)
{
    Utils_encdecIVAMapChangeNotifyCbInfo cbInfo;

    cbInfo.ctx = pObj;
    cbInfo.fxns = &DecLink_ivaMapChangeCbFxn;
    Utils_encdecUnRegisterIVAMapChangeNotifyCb(&cbInfo);
    DecLink_codecDeleteIvaSwitchSerializerObj(pObj);
    DecLink_codecDeleteIvaMapMutex(pObj);
}


static
Int32 DecLink_codecIVASwitchQueSerializer(DecLink_Obj * pObj,
                                          UInt32 chId,
                                          UInt32 prevIVAId,
                                          UInt32 nextIVAId,
                                          DecLink_ReqObj **pReqObjNextPtr)
{
    DecLink_ReqObj *pReqObjPrevIva;
    DecLink_ReqObj *pReqObjNextIva;
    ti_sysbios_knl_Semaphore_Struct *ivaSerializerMem;
    ti_sysbios_knl_Semaphore_Handle ivaSerializer;
    Int32 status;



    status =
    Utils_queGet(&pObj->decDummyReqObj.reqQueDummy , (Ptr *) & pReqObjPrevIva, 1,
                 ti_sysbios_BIOS_NO_WAIT);
    UTILS_assert(status == 0);
    UTILS_assert(UTILS_ARRAYISVALIDENTRY(pReqObjPrevIva,pObj->decDummyReqObj.reqObjDummy));

    status =
    Utils_queGet(&pObj->decDummyReqObj.reqQueDummy, (Ptr *) & pReqObjNextIva, 1,
                 ti_sysbios_BIOS_NO_WAIT);
    UTILS_assert(status == 0);
    UTILS_assert(UTILS_ARRAYISVALIDENTRY(pReqObjNextIva,pObj->decDummyReqObj.reqObjDummy));

    status = Utils_queGet(&pObj->decIVASwitchSerializeObj.freeSerializerQue,
                          (Ptr *)&ivaSerializerMem,
                          1,
                          ti_sysbios_BIOS_NO_WAIT);
    UTILS_assert(status == 0);
    ivaSerializer = ti_sysbios_knl_Semaphore_handle(ivaSerializerMem);
    pObj->chObj[chId].dummyBitBuf.channelNum = chId;

    pReqObjPrevIva->type = DEC_LINK_REQ_OBJECT_TYPE_DUMMY_IVAMAPCHANGE_PREVIVALAST;
    pReqObjPrevIva->ivaSwitchSerializer = ivaSerializer;
    pReqObjPrevIva->InBuf = &pObj->chObj[chId].dummyBitBuf;


    pReqObjNextIva->type = DEC_LINK_REQ_OBJECT_TYPE_DUMMY_IVAMAPCHANGE_NEXTIVAFIRST;
    pReqObjNextIva->ivaSwitchSerializer = ivaSerializer;
    pReqObjNextIva->InBuf = &pObj->chObj[chId].dummyBitBuf;


    DECLINK_INFO_LOG(pObj->linkId,chId,"Queueing last frame in prev IVA[%d]",prevIVAId);
    status = Utils_quePut(&pObj->decProcessTsk[prevIVAId].processQue,
                           pReqObjPrevIva, ti_sysbios_BIOS_NO_WAIT);
    UTILS_assert(status == FVID2_SOK);

    /* The first frame to be processed in new IVA will wait for semaphore to be posted by
     * last frame of the same channel from previous IVA.
     * This is to ensure when switching iva encode/decode doesnt happen
     * out of order .
     * To avoid deadlock , all the last frames of  previous IVA must
     * be queued in IVA process queue for all channels before
     * the first frames of next IVA are queued.
     */
    UTILS_assert(pReqObjNextPtr != NULL);
    *pReqObjNextPtr = pReqObjNextIva;
    return DEC_LINK_S_SUCCESS;
}

static
Void DecLink_codecIVASwitchQueFirstOfNextIVA(DecLink_Obj * pObj,
                                             DecLink_ReqObj *pNextIvaFirstObjs[],
                                             UInt32 newIvaId[],
                                             UInt32 numCh)
{
    Int i;
    Int32 status;
    UInt32 chId,nextIVAId;
    DecLink_ReqObj *pReqObjNextIva;

    UTILS_assert((pNextIvaFirstObjs != NULL)
                 &&
                 (newIvaId != NULL));
    for (i = 0; i < numCh; i++)
    {
        UTILS_assert((pNextIvaFirstObjs[i] != NULL)
                     &&
                     (pNextIvaFirstObjs[i]->InBuf != NULL));
        pReqObjNextIva = pNextIvaFirstObjs[i];
        chId = pReqObjNextIva->InBuf->channelNum;
        nextIVAId = newIvaId[i];
        DECLINK_INFO_LOG(pObj->linkId,chId,"Queueing first frame in next IVA[%d]",nextIVAId);
        status = Utils_quePut(&pObj->decProcessTsk[nextIVAId].processQue,
                               pReqObjNextIva, ti_sysbios_BIOS_NO_WAIT);
        UTILS_assert(status == FVID2_SOK);
    }
}

Int32 DecLink_codecIVAMapChangeHandler(DecLink_Obj * pObj)
{
    Int32 status = DEC_LINK_S_SUCCESS;
    UInt32 chId;
    UInt32 newIVAID;
    IArg key;
    DecLink_ReqObj *pNextIvaFirstObjs[DEC_LINK_MAX_CH];
    UInt32 newIvaId[DEC_LINK_MAX_CH];
    UInt32 numNextIvaFirstObjs;

    key = GateMutexPri_enter(pObj->ivaChMapMutex);
    numNextIvaFirstObjs = 0;
    for (chId = 0; chId < pObj->inQueInfo.numCh; chId++)
    {
        newIVAID = pObj->ch2ProcessTskIdNext[chId];
        if (newIVAID != pObj->ch2ProcessTskId[chId])
        {
            UTILS_assert(numNextIvaFirstObjs < UTILS_ARRAYSIZE(pNextIvaFirstObjs));
            DecLink_codecIVASwitchQueSerializer(
                                     pObj,
                                     chId,
                                     pObj->ch2ProcessTskId[chId],
                                     newIVAID,
                                     &(pNextIvaFirstObjs[numNextIvaFirstObjs]));
            UTILS_assert(numNextIvaFirstObjs < UTILS_ARRAYSIZE(newIvaId));
            newIvaId[numNextIvaFirstObjs] = newIVAID;
            numNextIvaFirstObjs++;
        }
        pObj->ch2ProcessTskId[chId] = newIVAID;
    }
    DecLink_codecIVASwitchQueFirstOfNextIVA(pObj,
                                            pNextIvaFirstObjs,
                                            newIvaId,
                                            numNextIvaFirstObjs);
    GateMutexPri_leave(pObj->ivaChMapMutex,key);
    return (status);
}

static Int32 DecLink_codecCreateReqObjDummy(DecLink_Obj * pObj)
{
    Int32 status;
    UInt32 reqId;
    struct decDummyReqObj_s *dummyReq = &pObj->decDummyReqObj;

    memset(dummyReq->reqObjDummy, 0, sizeof(dummyReq->reqObjDummy));

    UTILS_COMPILETIME_ASSERT(UTILS_ARRAYSIZE(dummyReq->reqQueMemDummy) ==
                             UTILS_ARRAYSIZE(dummyReq->reqObjDummy));
    status = Utils_queCreate(&dummyReq->reqQueDummy,
                             UTILS_ARRAYSIZE(dummyReq->reqQueMemDummy),
                             dummyReq->reqQueMemDummy,
                             UTILS_QUE_FLAG_NO_BLOCK_QUE);
    UTILS_assert(status == FVID2_SOK);

    for (reqId = 0; reqId < UTILS_ARRAYSIZE(dummyReq->reqObjDummy); reqId++)
    {
        status =
            Utils_quePut(&dummyReq->reqQueDummy, &dummyReq->reqObjDummy[reqId], BIOS_NO_WAIT);
        UTILS_assert(status == FVID2_SOK);
    }

    return DEC_LINK_S_SUCCESS;
}

static Int32 DecLink_codecDeleteReqObjDummy(DecLink_Obj * pObj)
{
    struct decDummyReqObj_s *dummyReq = &pObj->decDummyReqObj;
    Int32 status;

    status = Utils_queDelete(&dummyReq->reqQueDummy);

    UTILS_assert(status == 0);

    return DEC_LINK_S_SUCCESS;
}




