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
#include <mcfw/src_bios6/utils/utils_mem.h>
#include <mcfw/src_bios6/links_m3video/codec_utils/utils_encdec.h>
#include <mcfw/src_bios6/links_m3video/codec_utils/hdvicp2_config.h>
#include <mcfw/interfaces/link_api/system_tiler.h>
#include "encLink_priv.h"
#include "encLink_h264_priv.h"
#include "encLink_jpeg_priv.h"

typedef struct encLinkResClassChannelInfo {
    UInt32 numActiveResClass;
    struct resInfo_s {
        EncDec_ResolutionClass resClass;
        UInt32 width;
        UInt32 height;
        UInt32 numChInResClass;
        UInt32 chIdx[ENC_LINK_MAX_CH];
    } resInfo[UTILS_ENCDEC_RESOLUTION_CLASS_COUNT];
} encLinkResClassChannelInfo;

EncLink_ChCreateParams ENCLINK_DEFAULTCHCREATEPARAMS_H264 = {
    .format = IVIDEO_H264HP,
    .profile = IH264_HIGH_PROFILE,
    .dataLayout = IVIDEO_FIELD_SEPARATED,
    .fieldMergeEncodeEnable = FALSE,
    .encodingPreset = ENC_LINK_DEFAULT_ALGPARAMS_ENCODINGPRESET,
    .enableAnalyticinfo = ENC_LINK_DEFAULT_ALGPARAMS_ANALYTICINFO,
    .enableWaterMarking = ENC_LINK_DEFAULT_ALGPARAMS_ENABLEWATERMARKING,
    .maxBitRate = ENC_LINK_DEFAULT_ALGPARAMS_MAXBITRATE,
    .rateControlPreset = ENC_LINK_DEFAULT_ALGPARAMS_RATECONTROLPRESET,
    .defaultDynamicParams = 
    {
        .intraFrameInterval = ENC_LINK_DEFAULT_ALGPARAMS_INTRAFRAMEINTERVAL,
        .targetBitRate      = ENC_LINK_DEFAULT_ALGPARAMS_TARGETBITRATE,
        .interFrameInterval = ENC_LINK_DEFAULT_ALGPARAMS_MAXINTERFRAMEINTERVAL,
        .mvAccuracy         = ENC_LINK_DEFAULT_ALGPARAMS_MVACCURACY,
        .inputFrameRate     = ENC_LINK_DEFAULT_ALGPARAMS_INPUTFRAMERATE,
        .qpMin              = ENC_LINK_DEFAULT_ALGPARAMS_QPMIN,
        .qpMax              = ENC_LINK_DEFAULT_ALGPARAMS_QPMAX,
        .qpInit             = ENC_LINK_DEFAULT_ALGPARAMS_QPINIT,        
        .vbrDuration        = ENC_LINK_DEFAULT_ALGPARAMS_VBRDURATION,
        .vbrSensitivity     = ENC_LINK_DEFAULT_ALGPARAMS_VBRSENSITIVITY
    }
};

EncLink_ChCreateParams ENCLINK_DEFAULTCHCREATEPARAMS_JPEG = {
    .format = IVIDEO_MJPEG,
    .profile = 0,
    .dataLayout = IVIDEO_FIELD_SEPARATED,
    .fieldMergeEncodeEnable = FALSE,                       /* For MJPEG */
    .encodingPreset = 0,
    .maxBitRate = 0,
    .rateControlPreset = 0,
    .defaultDynamicParams = 
    {
        .intraFrameInterval = 0,
        .targetBitRate      = 2 * 1000 * 1000,
        .interFrameInterval = 0,
        .mvAccuracy         = ENC_LINK_DEFAULT_ALGPARAMS_MVACCURACY,
        .inputFrameRate     = ENC_LINK_DEFAULT_ALGPARAMS_INPUTFRAMERATE,
        .qpMin              = 0,
        .qpMax              = 0,
        .qpInit             = 0, 
        .vbrDuration        = 0,
        .vbrSensitivity     = 0
    }
};

static
EncDec_ResolutionClass enclink_get_resolution_class(UInt32 width,
                                                    UInt32 height);
static
Int enclink_get_resolution_class_info(EncDec_ResolutionClass resClass,
                                      UInt32 * pWidth, UInt32 * pHeight);
static
Int enclink_add_chinfo_to_resclass(EncDec_ResolutionClass resClass,
                                   UInt32 chID,
                                   encLinkResClassChannelInfo * resClassChInfo);
static
Int enclink_compare_resclass_resolution(const void *resInfoA,
                                        const void *resInfoB);
static
Void enclink_merge_resclass_chinfo_entry(struct resInfo_s *entryTo,
                                         struct resInfo_s *entryFrom);
static
Int enclink_merge_resclass_chinfo(encLinkResClassChannelInfo * resClassChInfo,
                                  UInt32 targetResClassCount);
static
Int enclink_populate_outbuf_pool_size_info(EncLink_CreateParams * createArgs,
                                           System_LinkQueInfo * inQueInfo,
                                           EncLink_OutObj * outQueInfo);
static Int32 EncLink_codecCreateReqObj(EncLink_Obj * pObj);
static Int32 EncLink_codecCreateOutObj(EncLink_Obj * pObj);
static Int32 EncLink_codecCreateChObj(EncLink_Obj * pObj, UInt32 chId);
static Int32 EncLink_codecCreateEncObj(EncLink_Obj * pObj, UInt32 chId);
static Void EncLink_codecProcessTskFxn(UArg arg1, UArg arg2);
static Int32 EncLink_codecCreateProcessTsk(EncLink_Obj * pObj, UInt32 tskId);
static Int32 EncLink_codecDeleteProcessTsk(EncLink_Obj * pObj, UInt32 tskId);
static Int32 EncLink_codecQueueFramesToChQue(EncLink_Obj * pObj);
static Int32 EncLink_codecSubmitData(EncLink_Obj * pObj);
static Int32 EncLink_codecGetProcessedData(EncLink_Obj * pObj);
static Int32 EncLink_codecDynamicResolutionChange(EncLink_Obj * pObj,
                                                  EncLink_ReqObj * pReqObj, 
                                                  UInt32 chId);
                             
static Int32 EncLink_PrepareBatch (EncLink_Obj *pObj, UInt32 tskId, 
                                   EncLink_ReqObj *pReqObj, 
                                   EncLink_ReqBatch *pReqObjBatch);

static Int32 EncLink_SubmitBatch (EncLink_Obj *pObj, UInt32 tskId, 
                                  EncLink_ReqBatch *pReqObjBatch);                            
static
Void EncLink_codecUnregisterIVAMapCb(EncLink_Obj * pObj);

static
Void EncLink_codecRegisterIVAMapCb(EncLink_Obj * pObj);
static Int32 EncLink_codecCreateReqObjDummy(EncLink_Obj * pObj);
static Int32 EncLink_codecDeleteReqObjDummy(EncLink_Obj * pObj);

static
EncDec_ResolutionClass enclink_get_resolution_class(UInt32 width, UInt32 height)
{
    EncDec_ResolutionClass resClass = UTILS_ENCDEC_RESOLUTION_CLASS_16MP;

    UTILS_assert((width <= UTILS_ENCDEC_RESOLUTION_CLASS_16MP_WIDTH)
                 && (height <= UTILS_ENCDEC_RESOLUTION_CLASS_16MP_HEIGHT));
    if ((width > UTILS_ENCDEC_RESOLUTION_CLASS_WUXGA_WIDTH) ||
        (height > UTILS_ENCDEC_RESOLUTION_CLASS_WUXGA_HEIGHT))
    {
        resClass = UTILS_ENCDEC_RESOLUTION_CLASS_16MP;
    }
    if ((width > UTILS_ENCDEC_RESOLUTION_CLASS_1080P_WIDTH) ||
        (height > UTILS_ENCDEC_RESOLUTION_CLASS_1080P_HEIGHT))
    {
        resClass = UTILS_ENCDEC_RESOLUTION_CLASS_WUXGA;
    }
    else if ((width > UTILS_ENCDEC_RESOLUTION_CLASS_720P_WIDTH) ||
        (height > UTILS_ENCDEC_RESOLUTION_CLASS_720P_HEIGHT))
    {
        resClass = UTILS_ENCDEC_RESOLUTION_CLASS_1080P;
    }
    else
    {
        if ((width > UTILS_ENCDEC_RESOLUTION_CLASS_D1_WIDTH) ||
            (height > UTILS_ENCDEC_RESOLUTION_CLASS_D1_HEIGHT))
        {
            resClass = UTILS_ENCDEC_RESOLUTION_CLASS_720P;
        }
        else
        {
            if ((width > UTILS_ENCDEC_RESOLUTION_CLASS_CIF_WIDTH) ||
                (height > UTILS_ENCDEC_RESOLUTION_CLASS_CIF_HEIGHT))
            {
                resClass = UTILS_ENCDEC_RESOLUTION_CLASS_D1;
            }
            else
            {
                resClass = UTILS_ENCDEC_RESOLUTION_CLASS_CIF;
            }
        }
    }
    return resClass;
}

static
Int enclink_get_resolution_class_info(EncDec_ResolutionClass resClass,
                                      UInt32 * pWidth, UInt32 * pHeight)
{
    Int status = ENC_LINK_S_SUCCESS;

    switch (resClass)
    {
        case UTILS_ENCDEC_RESOLUTION_CLASS_16MP:
            *pWidth = UTILS_ENCDEC_RESOLUTION_CLASS_16MP_WIDTH;
            *pHeight = UTILS_ENCDEC_RESOLUTION_CLASS_16MP_HEIGHT;
            break;
        case UTILS_ENCDEC_RESOLUTION_CLASS_WUXGA:
            *pWidth = UTILS_ENCDEC_RESOLUTION_CLASS_WUXGA_WIDTH;
            *pHeight = UTILS_ENCDEC_RESOLUTION_CLASS_WUXGA_HEIGHT;
            break;
        case UTILS_ENCDEC_RESOLUTION_CLASS_1080P:
            *pWidth = UTILS_ENCDEC_RESOLUTION_CLASS_1080P_WIDTH;
            *pHeight = UTILS_ENCDEC_RESOLUTION_CLASS_1080P_HEIGHT;
            break;
        case UTILS_ENCDEC_RESOLUTION_CLASS_720P:
            *pWidth = UTILS_ENCDEC_RESOLUTION_CLASS_720P_WIDTH;
            *pHeight = UTILS_ENCDEC_RESOLUTION_CLASS_720P_HEIGHT;
            break;
        case UTILS_ENCDEC_RESOLUTION_CLASS_D1:
            *pWidth = UTILS_ENCDEC_RESOLUTION_CLASS_D1_WIDTH;
            *pHeight = UTILS_ENCDEC_RESOLUTION_CLASS_D1_HEIGHT;
            break;
        case UTILS_ENCDEC_RESOLUTION_CLASS_CIF:
            *pWidth = UTILS_ENCDEC_RESOLUTION_CLASS_CIF_WIDTH;
            *pHeight = UTILS_ENCDEC_RESOLUTION_CLASS_CIF_HEIGHT;
            break;
        default:
            *pWidth = *pHeight = 0;
            status = UTILS_ENCDEC_E_INT_UNKNOWNRESOLUTIONCLASS;
            ENCLINK_INTERNAL_ERROR_LOG(status,
                                       "Unknown resoltuion class:%d", resClass);
            break;
    }
    return status;
}

static
Int enclink_add_chinfo_to_resclass(EncDec_ResolutionClass resClass,
                                   UInt32 chID,
                                   encLinkResClassChannelInfo * resClassChInfo)
{
    Int i;
    Int status = UTILS_ENCDEC_S_SUCCESS;

    UTILS_assert(resClassChInfo->numActiveResClass <=
                 UTILS_ENCDEC_RESOLUTION_CLASS_COUNT);
    for (i = 0; i < resClassChInfo->numActiveResClass; i++)
    {
        if (resClassChInfo->resInfo[i].resClass == resClass)
        {
            UInt32 curChIdx = resClassChInfo->resInfo[i].numChInResClass;

            UTILS_assert(curChIdx < ENC_LINK_MAX_CH);
            resClassChInfo->resInfo[i].chIdx[curChIdx] = chID;
            resClassChInfo->resInfo[i].numChInResClass++;
            break;
        }
    }
    if (i == resClassChInfo->numActiveResClass)
    {
        Int resClassIndex = resClassChInfo->numActiveResClass;

        /* Need to add a entry for this resolution class */
        UTILS_assert(resClassChInfo->numActiveResClass <
                     UTILS_ENCDEC_RESOLUTION_CLASS_COUNT);
        resClassChInfo->resInfo[resClassIndex].resClass = resClass;
        status =
            enclink_get_resolution_class_info(resClass,
                                              &(resClassChInfo->
                                                resInfo[resClassIndex].width),
                                              &(resClassChInfo->
                                                resInfo[resClassIndex].height));
        UTILS_assert((status == ENC_LINK_S_SUCCESS));
        resClassChInfo->resInfo[resClassIndex].numChInResClass = 0;
        resClassChInfo->resInfo[resClassIndex].chIdx[0] = chID;
        resClassChInfo->resInfo[resClassIndex].numChInResClass++;
        resClassChInfo->numActiveResClass++;
    }
    return status;
}

static
Int enclink_compare_resclass_resolution(const void *resInfoA,
                                        const void *resInfoB)
{
    const struct resInfo_s *resA = resInfoA;
    const struct resInfo_s *resB = resInfoB;

    return ((resA->width * resA->height) - (resB->width * resB->height));
}

static
Void enclink_merge_resclass_chinfo_entry(struct resInfo_s *entryTo,
                                         struct resInfo_s *entryFrom)
{
    Int i;

    for (i = 0; i < entryFrom->numChInResClass; i++)
    {
        UInt32 curChIdx = entryTo->numChInResClass;

        UTILS_assert(entryTo->numChInResClass < ENC_LINK_MAX_CH);
        entryTo->chIdx[curChIdx] = entryFrom->chIdx[i];
        entryTo->numChInResClass++;
    }
    entryTo->resClass = entryFrom->resClass;
    entryTo->width = entryFrom->width;
    entryTo->height = entryFrom->height;
}

static
Int enclink_merge_resclass_chinfo(encLinkResClassChannelInfo * resClassChInfo,
                                  UInt32 targetResClassCount)
{
    Bool sortDone = FALSE;

    UTILS_assert(targetResClassCount > 0);
    while (resClassChInfo->numActiveResClass > targetResClassCount)
    {
        Uint32 resolutionToMergeIdx, resolutionFromMergeIdx;

        if (FALSE == sortDone)
        {
            qsort(resClassChInfo->resInfo, resClassChInfo->numActiveResClass,
                  sizeof(struct resInfo_s),
                  enclink_compare_resclass_resolution);
            sortDone = TRUE;
        }
        UTILS_assert(resClassChInfo->numActiveResClass >= 2);
        resolutionToMergeIdx = resClassChInfo->numActiveResClass - 2;
        resolutionFromMergeIdx = resClassChInfo->numActiveResClass - 1;
        UTILS_assert((resClassChInfo->resInfo[resolutionToMergeIdx].width <=
                      resClassChInfo->resInfo[resolutionFromMergeIdx].width)
                     &&
                     (resClassChInfo->resInfo[resolutionToMergeIdx].height <=
                      resClassChInfo->resInfo[resolutionFromMergeIdx].height));
        enclink_merge_resclass_chinfo_entry(&resClassChInfo->
                                            resInfo[resolutionToMergeIdx],
                                            &resClassChInfo->
                                            resInfo[resolutionFromMergeIdx]);
        resClassChInfo->numActiveResClass--;
    }
    return ENC_LINK_S_SUCCESS;
}

static
UInt32 enclink_get_num_outbuf_per_ch(EncLink_ChCreateParams *chCreateParams,
                                     UInt32                  chNum,
                                     UInt32                  defaultNumOutBufs)
{
    static Bool warningPrintDone = FALSE;
    UInt32 numOutBufsPerCh = defaultNumOutBufs;

    /* Calculate the number of output buffers needed for this channel based
     * on create args.
     * !!!!WARNING.READ THIS:
     * Ideally the number of output buffers should be determined by
     * ratio of max output frame rate of that channel and the inputFrameRate.
     * However this parameter is not available at create time.
     * So the number of output buffers is fixed based on coding type.
     * This is a very bad hack . To remove the hack we need to modify
     * the create API for encoder link.
     */
    if (IVIDEO_MJPEG == chCreateParams->format)
    {
        numOutBufsPerCh = 1;
        if (FALSE == warningPrintDone)
        {
            Vps_printf("ENCLINK:INFO: !!!Number of output buffers for ch[%d] set to [%d]",
                        chNum,numOutBufsPerCh);
            warningPrintDone = TRUE;
        }
    }
    return numOutBufsPerCh;
}

static
Int enclink_populate_outbuf_pool_size_info(EncLink_CreateParams * createArgs,
                                           System_LinkQueInfo * inQueInfo,
                                           EncLink_OutObj * outQueInfo)
{
    Int i, j;
    Int status = ENC_LINK_S_SUCCESS;
    Int mvFlag = 0;
    Int mvWidth = 0;
    Int mvHeight = 0;
    EncDec_ResolutionClass resClass;
    static encLinkResClassChannelInfo resClassChInfo;

     /** <  resClassChInfo is made static to avoid blowing up the stack
      **    as this structure is large. This requires that all access
      **    to this data structure be executed in critical section
      */
    UInt key;
    UInt32 totalNumOutBufs = 0;

    key = Hwi_disable();
    resClassChInfo.numActiveResClass = 0;
    for (i = 0; i < inQueInfo->numCh; i++)
    {
        resClass = enclink_get_resolution_class(inQueInfo->chInfo[i].width,
                                                inQueInfo->chInfo[i].height);
        enclink_add_chinfo_to_resclass(resClass, i, &resClassChInfo);
    }
    if (resClassChInfo.numActiveResClass > UTILS_BITBUF_MAX_ALLOC_POOLS)
    {
        enclink_merge_resclass_chinfo(&resClassChInfo,
                                      UTILS_BITBUF_MAX_ALLOC_POOLS);
    }
    outQueInfo->numAllocPools = resClassChInfo.numActiveResClass;
    UTILS_assert(outQueInfo->numAllocPools <= UTILS_BITBUF_MAX_ALLOC_POOLS);

    for (i = 0; i < outQueInfo->numAllocPools; i++)
    {
        Int32 maxBitRate = 0;
        Int32 maxFrameRate = 0;
        mvFlag = 0;
        
        outQueInfo->outNumBufs[i] = 0;
        for (j = 0; j < resClassChInfo.resInfo[i].numChInResClass; j++)
        {
            UInt32 chId;
            Int32 chBitRate;
            Int32 chFrameRate;
            EncLink_ChCreateParams *chCreateParams;

            UTILS_assert(resClassChInfo.resInfo[i].chIdx[j] < ENC_LINK_MAX_CH);
            outQueInfo->ch2poolMap[resClassChInfo.resInfo[i].chIdx[j]] = i;
            chId = resClassChInfo.resInfo[i].chIdx[j];
            chCreateParams = &createArgs->chCreateParams[chId];
            if(chCreateParams->enableAnalyticinfo == 1)
            {
                /*Here Assuming now that all channels will be of same scanFormat type*/
                 mvFlag = 1;
                 mvWidth = inQueInfo->chInfo[chId].width;
                 mvHeight = inQueInfo->chInfo[chId].height;
            }

            chBitRate = chCreateParams->defaultDynamicParams.targetBitRate;
            if (chBitRate > maxBitRate)
            {
                maxBitRate = chBitRate;
            }
            chFrameRate = chCreateParams->defaultDynamicParams.inputFrameRate;
            if (chFrameRate > maxFrameRate)
            {
                maxFrameRate = chFrameRate;
            }
            outQueInfo->outNumBufs[i] +=
              enclink_get_num_outbuf_per_ch(chCreateParams,
                                            chId,
                                            createArgs->numBufPerCh[i]);
        }
        outQueInfo->buf_size[i] =
            UTILS_ENCDEC_GET_BITBUF_SIZE(resClassChInfo.resInfo[i].width,
                                         resClassChInfo.resInfo[i].height,
                                         maxBitRate,
                                         maxFrameRate);
        if(mvFlag)
        {
            outQueInfo->buf_size[i] += ENC_LINK_GET_MVBUF_SIZE(mvWidth, mvHeight);
            mvFlag = 0;
        }

        /* align size to minimum required frame buffer alignment */
        outQueInfo->buf_size[i] = VpsUtils_align(outQueInfo->buf_size[i], IVACODEC_VDMA_BUFFER_ALIGNMENT);
        totalNumOutBufs += outQueInfo->outNumBufs[i];
    }
    UTILS_assert(totalNumOutBufs <= ENC_LINK_MAX_OUT_FRAMES);
    Hwi_restore(key);
    return status;
}

static Int32 EncLink_codecCreateReqObj(EncLink_Obj * pObj)
{
    Int32 status;
    UInt32 reqId;

    memset(pObj->reqObj, 0, sizeof(pObj->reqObj));

    status = Utils_queCreate(&pObj->reqQue,
                             ENC_LINK_MAX_REQ,
                             pObj->reqQueMem, UTILS_QUE_FLAG_BLOCK_QUE_GET);
    UTILS_assert(status == FVID2_SOK);

    pObj->isReqPend = FALSE;

    for (reqId = 0; reqId < ENC_LINK_MAX_REQ; reqId++)
    {
        status =
            Utils_quePut(&pObj->reqQue, &pObj->reqObj[reqId], BIOS_NO_WAIT);
        UTILS_assert(status == FVID2_SOK);
    }

    return FVID2_SOK;
}

static Void EncLink_codecPrdCalloutFcn(UArg arg)
{
    EncLink_Obj *pObj = (EncLink_Obj *) arg;
    Int32 status;

    status = System_sendLinkCmd(pObj->linkId, ENC_LINK_CMD_GET_PROCESSED_DATA);


    if (UTILS_ISERROR(status))
    {
        #ifdef SYSTEM_DEBUG_CMD_ERROR
        UTILS_warn("ENCLINK:[%s:%d]:"
                   "System_sendLinkCmd ENC_LINK_CMD_GET_PROCESSED_DATA failed"
                   "errCode = %d", __FILE__, __LINE__, status);
        #endif
    }

}

static Int32 EncLink_codecCreatePrdObj(EncLink_Obj * pObj)
{
    Clock_Params clockParams;

    Clock_Params_init(&clockParams);
    clockParams.arg = (UArg) pObj;
    UTILS_assert(pObj->periodicObj.clkHandle == NULL);

    Clock_construct(&(pObj->periodicObj.clkStruct), EncLink_codecPrdCalloutFcn,
                    1, &clockParams);

    pObj->periodicObj.clkHandle = Clock_handle(&pObj->periodicObj.clkStruct);
    pObj->periodicObj.clkStarted = FALSE;

    return ENC_LINK_S_SUCCESS;

}

static Int32 EncLink_codecDeletePrdObj(EncLink_Obj * pObj)
{
    /* Stop the clock */
    Clock_stop(pObj->periodicObj.clkHandle);
    Clock_destruct(&(pObj->periodicObj.clkStruct));
    pObj->periodicObj.clkHandle = NULL;
    pObj->periodicObj.clkStarted = FALSE;

    return ENC_LINK_S_SUCCESS;
}

static Int32 EncLink_codecStartPrdObj(EncLink_Obj * pObj, UInt period)
{

    UTILS_assert(pObj->periodicObj.clkHandle != NULL);

    if (FALSE == pObj->periodicObj.clkStarted)
    {
        Clock_setPeriod(pObj->periodicObj.clkHandle, period);
        Clock_setTimeout(pObj->periodicObj.clkHandle, period);
        Clock_start(pObj->periodicObj.clkHandle);
        pObj->periodicObj.clkStarted = TRUE;
    }

    return ENC_LINK_S_SUCCESS;
}

static Int32 EncLink_codecStopPrdObj(EncLink_Obj * pObj)
{

    UTILS_assert(pObj->periodicObj.clkHandle != NULL);

    if (TRUE == pObj->periodicObj.clkStarted)
    {
        Clock_stop(pObj->periodicObj.clkHandle);
        pObj->periodicObj.clkStarted = FALSE;
    }
    return ENC_LINK_S_SUCCESS;
}

static Int32 EncLink_codecCreateOutObj(EncLink_Obj * pObj)
{
    EncLink_OutObj *pOutObj;
    Int32 status;
    UInt32 chId, bufIdx, outId;
    System_LinkChInfo *pOutChInfo;
    Int i;
    UInt32 totalBufCnt;

    enclink_populate_outbuf_pool_size_info(&pObj->createArgs, &pObj->inQueInfo,
                                           &pObj->outObj);

    pOutObj = &pObj->outObj;
    status = Utils_bitbufCreate(&pOutObj->bufOutQue, TRUE, FALSE,
                                pObj->outObj.numAllocPools);
    UTILS_assert(status == FVID2_SOK);

    status = Utils_queCreate(&pObj->processDoneQue,
                             ENC_LINK_MAX_OUT_FRAMES,
                             pObj->processDoneQueMem,
                             (UTILS_QUE_FLAG_BLOCK_QUE_GET |
                              UTILS_QUE_FLAG_BLOCK_QUE_PUT));
    UTILS_assert(status == FVID2_SOK);

    totalBufCnt = 0;
    for (i = 0; i < pOutObj->numAllocPools; i++)
    {
        status = Utils_memBitBufAlloc(&(pOutObj->outBufs[totalBufCnt]),
                                      pOutObj->buf_size[i],
                                      pOutObj->outNumBufs[i]);
        UTILS_assert(status == FVID2_SOK);

        for (bufIdx = 0; bufIdx < pOutObj->outNumBufs[i]; bufIdx++)
        {
            UTILS_assert((bufIdx + totalBufCnt) < ENC_LINK_MAX_OUT_FRAMES);
            pOutObj->outBufs[bufIdx + totalBufCnt].allocPoolID = i;
            pOutObj->outBufs[bufIdx + totalBufCnt].appData =
                &(pOutObj->frameInfo[bufIdx + totalBufCnt]);
            pOutObj->outBufs[bufIdx + totalBufCnt].doNotDisplay =
                FALSE;
            status =
                Utils_bitbufPutEmptyBuf(&pOutObj->bufOutQue,
                                        &pOutObj->outBufs[bufIdx +
                                                          totalBufCnt]);
            UTILS_assert(status == FVID2_SOK);
        }
        totalBufCnt += pOutObj->outNumBufs[i];
    }
    pObj->info.numQue = ENC_LINK_MAX_OUT_QUE;
    for (outId = 0u; outId < ENC_LINK_MAX_OUT_QUE; outId++)
    {
        pObj->info.queInfo[outId].numCh = pObj->inQueInfo.numCh;
    }

    for (chId = 0u; chId < pObj->inQueInfo.numCh; chId++)
    {
        for (outId = 0u; outId < ENC_LINK_MAX_OUT_QUE; outId++)
        {
            pOutChInfo = &pObj->info.queInfo[outId].chInfo[chId];
            pOutChInfo->bufType = SYSTEM_BUF_TYPE_VIDBITSTREAM;
            pOutChInfo->codingformat =
                pObj->createArgs.chCreateParams[chId].format;
            pOutChInfo->memType = VPS_VPDMA_MT_NONTILEDMEM;
            pOutChInfo->scanFormat = pObj->inQueInfo.chInfo[chId].scanFormat;
            pOutChInfo->width = pObj->inQueInfo.chInfo[chId].width;
            pOutChInfo->height = pObj->inQueInfo.chInfo[chId].height;
        }
    }

    return (status);
}

static Int32 EncLink_codecCreateChObj(EncLink_Obj * pObj, UInt32 chId)
{
    EncLink_ChObj *pChObj;
    Int32 status;
    ti_sysbios_knl_Semaphore_Params semPrms;

    pChObj = &pObj->chObj[chId];

    status = Utils_queCreate(&pChObj->inQue, ENC_LINK_MAX_REQ,
                             pChObj->inFrameMem, UTILS_QUE_FLAG_BLOCK_QUE_GET);
    UTILS_assert(status == FVID2_SOK);

    pChObj->nextFid = FVID2_FID_TOP;
    pChObj->totalInFrameCnt = 0;

    pChObj->inputFrameRate =
		pObj->createArgs.chCreateParams[chId].defaultDynamicParams.inputFrameRate;

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

static Int32 enclink_codec_set_ch_alg_create_params(EncLink_Obj * pObj,
                                                    UInt32 chId)
{
    EncLink_ChObj *pChObj;
    System_LinkChInfo *pInChInfo;

    EncLink_ChCreateParams  *pChCreatePrm;
    EncLink_ChDynamicParams *pChDynPrm;

    EncLink_AlgCreateParams     *pChAlgCreatePrm;

    pChObj = &pObj->chObj[chId];
    pInChInfo = &pObj->inQueInfo.chInfo[chId];

    pChCreatePrm    = &pObj->createArgs.chCreateParams[chId];
    pChDynPrm       = &pChCreatePrm->defaultDynamicParams;

    pChAlgCreatePrm = &pChObj->algObj.algCreateParams;

    pChAlgCreatePrm->format             = (IVIDEO_Format) pChCreatePrm->format;
    pChAlgCreatePrm->dataLayout         = (IVIDEO_VideoLayout) pChCreatePrm->dataLayout;
    pChAlgCreatePrm->singleBuf          = pChCreatePrm->fieldMergeEncodeEnable;
    pChAlgCreatePrm->maxWidth           = pInChInfo->width;
    pChAlgCreatePrm->maxHeight          = pInChInfo->height;
    pChAlgCreatePrm->maxInterFrameInterval = pChDynPrm->interFrameInterval;

    pChAlgCreatePrm->inputContentType =
        Utils_encdecMapFVID2XDMContentType(pInChInfo->scanFormat);

    pChAlgCreatePrm->inputChromaFormat  =
        Utils_encdecMapFVID2XDMChromaFormat(pInChInfo->dataFormat);

    pChAlgCreatePrm->profile            = pChCreatePrm->profile;

    Utils_encdecGetCodecLevel(pChAlgCreatePrm->format,
                              pChAlgCreatePrm->maxWidth,
                              pChAlgCreatePrm->maxHeight,
                              ENC_LINK_DEFAULT_ALGPARAMS_REFFRAMERATEX1000,
                              pChDynPrm->targetBitRate,
                              &(pChAlgCreatePrm->level),
                              TRUE);

    pChAlgCreatePrm->tilerEnable = FALSE;

    if (pInChInfo->memType == VPS_VPDMA_MT_TILEDMEM)
    {
        pChAlgCreatePrm->tilerEnable = TRUE;
    }

    #ifndef SYSTEM_USE_TILER
    UTILS_assert(FALSE == pChAlgCreatePrm->tilerEnable);
    #endif

    pChAlgCreatePrm->enableAnalyticinfo = pChCreatePrm->enableAnalyticinfo;

    if(pChAlgCreatePrm->enableAnalyticinfo == 1)
    {
        pChAlgCreatePrm->mvDataSize =
                ENC_LINK_GET_MVBUF_SIZE(
                 pInChInfo->width, pInChInfo->height);
    }

    pChAlgCreatePrm->maxBitRate        = pChCreatePrm->maxBitRate;
    pChAlgCreatePrm->encodingPreset    = pChCreatePrm->encodingPreset;
    pChAlgCreatePrm->rateControlPreset = pChCreatePrm->rateControlPreset;
    pChAlgCreatePrm->enableHighSpeed   = pChCreatePrm->enableHighSpeed;
    pChAlgCreatePrm->numTemporalLayer  = pChCreatePrm->numTemporalLayer;
    pChAlgCreatePrm->enableSVCExtensionFlag = pChCreatePrm->enableSVCExtensionFlag;
    pChAlgCreatePrm->enableWaterMarking = pChCreatePrm->enableWaterMarking;

    return ENC_LINK_S_SUCCESS;
}

static Int32 enclink_codec_set_ch_alg_default_dynamic_params(EncLink_Obj * pObj,
                                                             UInt32 chId)
{
    EncLink_ChObj *pChObj;
    System_LinkChInfo *pInChInfo;

    EncLink_ChCreateParams  *pChCreatePrm;
    EncLink_ChDynamicParams *pChDynPrm;

    EncLink_AlgCreateParams     *pChAlgCreatePrm;
    EncLink_AlgDynamicParams    *pChAlgDynPrm;

    pChObj = &pObj->chObj[chId];
    pInChInfo       = &pObj->inQueInfo.chInfo[chId];
 
    pChCreatePrm    = &pObj->createArgs.chCreateParams[chId];
    pChDynPrm       = &pChCreatePrm->defaultDynamicParams;

    pChAlgCreatePrm = &pChObj->algObj.algCreateParams;
    pChAlgDynPrm    = &pChObj->algObj.algDynamicParams;

    pChAlgDynPrm->startX        = ENC_LINK_DEFAULT_ALGPARAMS_STARTX;
    pChAlgDynPrm->startY        = ENC_LINK_DEFAULT_ALGPARAMS_STARTY;
    pChAlgDynPrm->inputWidth    = pInChInfo->width;
    pChAlgDynPrm->inputHeight   = pInChInfo->height;
    pChAlgDynPrm->inputPitch    = pInChInfo->pitch[0];

    pChAlgDynPrm->targetBitRate         = pChDynPrm->targetBitRate;

    if(pChDynPrm->inputFrameRate > 0 && pChDynPrm->targetFrameRate > 0)
    {
        pChAlgDynPrm->targetFrameRate   = pChDynPrm->targetFrameRate * 1000;
        pChAlgDynPrm->refFrameRate      = pChDynPrm->inputFrameRate * 1000;
    }
    else if(pChAlgCreatePrm->format == IVIDEO_MJPEG)
    {
        pChAlgDynPrm->targetFrameRate = ENC_LINK_MJPEG_DEFAULT_ALGPARAMS_TARGETFRAMERATEX1000;
        pChAlgDynPrm->refFrameRate    = ENC_LINK_MJPEG_DEFAULT_ALGPARAMS_REFFRAMERATEX1000;
    }
    else
    if (
        (pChAlgCreatePrm->format ==  IVIDEO_H264BP) ||
        (pChAlgCreatePrm->format ==  IVIDEO_H264MP) ||
        (pChAlgCreatePrm->format ==  IVIDEO_H264HP)
      )
    {
        pChAlgDynPrm->targetFrameRate   = ENC_LINK_DEFAULT_ALGPARAMS_TARGETFRAMERATEX1000;
        pChAlgDynPrm->refFrameRate      = ENC_LINK_DEFAULT_ALGPARAMS_REFFRAMERATEX1000;
    }
    else
    {
        UTILS_assert(FALSE); //Format is not supported
    }

    pChAlgDynPrm->intraFrameInterval    = pChDynPrm->intraFrameInterval;
    pChAlgDynPrm->interFrameInterval    = pChDynPrm->interFrameInterval;
    pChAlgDynPrm->rcAlg                 = pChDynPrm->rcAlg;
    pChAlgDynPrm->qpMinI                = pChDynPrm->qpMin;
    pChAlgDynPrm->qpMaxI                = pChDynPrm->qpMax;
    pChAlgDynPrm->qpInitI               = pChDynPrm->qpInit;
    pChAlgDynPrm->qpMinP                = pChDynPrm->qpMin;
    pChAlgDynPrm->qpMaxP                = pChDynPrm->qpMax;
    pChAlgDynPrm->qpInitP               = pChDynPrm->qpInit;
    pChAlgDynPrm->forceFrame            = FALSE;
    pChAlgDynPrm->vbrDuration           = pChDynPrm->vbrDuration;
    pChAlgDynPrm->vbrSensitivity        = pChDynPrm->vbrSensitivity;
    pChAlgDynPrm->forceFrameStatus      = FALSE;
    pChAlgDynPrm->mvAccuracy            = pChDynPrm->mvAccuracy;
    pChAlgDynPrm->qualityFactor         = pChCreatePrm->profile;

    return ENC_LINK_S_SUCCESS;
}

static Int32 EncLink_codecCreateEncObj(EncLink_Obj * pObj, UInt32 chId)
{
    Int retVal;
    EncLink_ChObj *pChObj;
    Int scratchGroupID;
    UInt32 contentType;

    pChObj = &pObj->chObj[chId];
    scratchGroupID = -1;

    contentType =
        Utils_encdecMapFVID2XDMContentType(pObj->inQueInfo.chInfo[chId].
                                           scanFormat);

    enclink_codec_set_ch_alg_create_params(pObj, chId);
    enclink_codec_set_ch_alg_default_dynamic_params(pObj, chId);
    
    if ((contentType == IVIDEO_INTERLACED) &&
        (pChObj->algObj.algCreateParams.singleBuf == FALSE))
    {
        pChObj->numReqObjPerProcess = 2;
    }
    else
    {
        pChObj->numReqObjPerProcess = 1;
    }
    switch (pChObj->algObj.algCreateParams.format)
    {
        case IVIDEO_H264BP:
        case IVIDEO_H264MP:
        case IVIDEO_H264HP:
            retVal = EncLinkH264_algCreate(&pChObj->algObj.u.h264AlgIfObj,
                                           &pChObj->algObj.algCreateParams,
                                           &pChObj->algObj.algDynamicParams,
                                           pObj->linkId, chId, scratchGroupID);
            break;

        case IVIDEO_MJPEG:
            retVal = EncLinkJPEG_algCreate(&pChObj->algObj.u.jpegAlgIfObj,
                                           &pChObj->algObj.algCreateParams,
                                           &pChObj->algObj.algDynamicParams,
                                           pObj->linkId, chId, scratchGroupID);
            break;
        default:
            retVal = ENC_LINK_E_UNSUPPORTEDCODEC;
            break;

    }
    UTILS_assert(retVal == ENC_LINK_S_SUCCESS);

    return retVal;
}

static Int32 EncLink_codecHandleDummyReqObj(EncLink_Obj  *pObj,
                                            EncLink_ReqObj *pReqObj,
                                            UInt32 ivaID)
{
    Int32 status = ENC_LINK_S_SUCCESS;

    switch (pReqObj->type)
    {
        case ENC_LINK_REQ_OBJECT_TYPE_DUMMY_IVAMAPCHANGE_PREVIVALAST:
        {
            UTILS_assert(pReqObj->OutBuf != NULL);
            UTILS_assert((pReqObj->ivaSwitchSerializer != NULL)
                         &&
                         (UTILS_ARRAYISVALIDENTRY(ti_sysbios_knl_Semaphore_struct(pReqObj->ivaSwitchSerializer),
                          pObj->encIVASwitchSerializeObj.freeSerializerSemMem)));
            ENCLINK_INFO_LOG(pObj->linkId,pReqObj->OutBuf->channelNum,
                             "Iva Map Change Serialization: Last Frame of Prev IVA [%d] received", ivaID);
            ti_sysbios_knl_Semaphore_post(pReqObj->ivaSwitchSerializer);
            break;
        }

        case ENC_LINK_REQ_OBJECT_TYPE_DUMMY_IVAMAPCHANGE_NEXTIVAFIRST:
        {
            Bool semStatus;

            UTILS_assert(pReqObj->OutBuf != NULL);
            UTILS_assert((pReqObj->ivaSwitchSerializer != NULL)
                         &&
                         (UTILS_ARRAYISVALIDENTRY(ti_sysbios_knl_Semaphore_struct(pReqObj->ivaSwitchSerializer),
                          pObj->encIVASwitchSerializeObj.freeSerializerSemMem)));
            ENCLINK_INFO_LOG(pObj->linkId,pReqObj->OutBuf->channelNum,
                             "Iva Map Change Serialization: First Frame of Next IVA [%d] received", ivaID);
            semStatus = ti_sysbios_knl_Semaphore_pend(pReqObj->ivaSwitchSerializer,ti_sysbios_BIOS_WAIT_FOREVER);
            UTILS_assert(semStatus == TRUE);
            ENCLINK_INFO_LOG(pObj->linkId,pReqObj->OutBuf->channelNum,
                             "Iva Map Change Serialization: First Frame of Next IVA [%d] received. Serialization complete!!", ivaID);
            status = Utils_quePut(&pObj->encIVASwitchSerializeObj.freeSerializerQue,
                                  Semaphore_struct(pReqObj->ivaSwitchSerializer),
                                  ti_sysbios_BIOS_NO_WAIT);
            UTILS_assert(status == 0);
            break;
        }
        default:
            /* Unsupported reqObjType.*/
            UTILS_assert(0);
            break;
    }
    pReqObj->type = ENC_LINK_REQ_OBJECT_TYPE_REGULAR;
    pReqObj->ivaSwitchSerializer = NULL;
    UTILS_assert(UTILS_ARRAYISVALIDENTRY(pReqObj,pObj->encDummyReqObj.reqObjDummy));
    status = Utils_quePut(&pObj->encDummyReqObj.reqQueDummy, pReqObj, ti_sysbios_BIOS_NO_WAIT);
    UTILS_assert(status == FVID2_SOK);
    return status;
}

static Void EncLink_codecProcessTskFxn(UArg arg1, UArg arg2)
{
    Int32 status, i;
    EncLink_Obj *pObj;
    EncLink_ReqObj *pReqObj;
    UInt32 tskId;

    pObj = (EncLink_Obj *) arg1;
    tskId = (UInt32) arg2;
    
    memset (&pObj->reqObjBatch[tskId], 0, sizeof(EncLink_ReqBatch));

    while (pObj->state != SYSTEM_LINK_STATE_STOP)
    {
        pObj->reqObjBatch[tskId].numReqObjsInBatch = 0;
        status = ENC_LINK_S_SUCCESS;

        pReqObj = NULL;

        status = Utils_queGet(&pObj->encProcessTsk[tskId].processQue,
                              (Ptr *) & pReqObj, 1, BIOS_WAIT_FOREVER);

        if (!UTILS_ISERROR(status))
        {

          if (ENC_LINK_REQ_OBJECT_TYPE_REGULAR != pReqObj->type)
          {
              EncLink_codecHandleDummyReqObj(pObj,pReqObj,tskId);
              continue;
          }
          /*Prepare a batch of ReqObjs which can be used together to create a
            processList. A Batch has the following constraints:
            1. It should have only one type of codec. i.e. All H.264 or all
               MJPEG
            2. All the req Objs in the Batch should have unique channel numbers
               i.e. no channel number should get repeated within the same Batch
               Avoids maitaining multiple copies of InArgs, OutArgs etc while 
               preparing the processList.
            3. The Batch size is limited to 24 (Comes from codec limitation)
            */
          UTILS_assert(pReqObj->type == ENC_LINK_REQ_OBJECT_TYPE_REGULAR);
          status = EncLink_PrepareBatch (pObj, tskId, pReqObj, 
                                           &pObj->reqObjBatch[tskId]);
          if (UTILS_ISERROR(status))
          {
              UTILS_warn("ENCLINK:ERROR in "
                         "EncLink_PrepareBatch.Status[%d]", status);
          }

        }

        if (pObj->reqObjBatch[tskId].numReqObjsInBatch)
        {
          /*Submit the previously created Batch to the IVAHD by translating it
            into a ProcessList*/
          #ifdef SYSTEM_DEBUG_MULTI_CHANNEL_ENC
          Vps_printf ("ENC : IVAHDID : %d Submitting Batch\n", tskId);
          #endif
          status = EncLink_SubmitBatch (pObj, tskId, &pObj->reqObjBatch[tskId]);
          if (UTILS_ISERROR(status))
          {
              UTILS_warn("ENC : IVAHDID : %d ENCLINK:ERROR in "
                         "EncLink_SubmitBatch.Status[%d]", tskId, status);
          }
          else {
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

      for (i = 0; i < pObj->reqObjBatch[tskId].numReqObjsInBatch; i++)
      {
          /*Return the processed ReqObjects to the Enc Link via the ProcessDone
            Queue*/
          pReqObj = pObj->reqObjBatch[tskId].pReqObj[i];
          /**TODO: Remove this hack of setting timestamp.
           * Timestamp should be system time logged in capture component
          */      
          pReqObj->OutBuf->timeStamp =  pReqObj->InFrameList.frames[0]->timeStamp;
          
          status = Utils_quePut(&pObj->processDoneQue, pReqObj,
                                BIOS_NO_WAIT);
          UTILS_assert(status == FVID2_SOK);      
      }
 
    }

    return;
}

#pragma DATA_ALIGN(gEncProcessTskStack, 32)
#pragma DATA_SECTION(gEncProcessTskStack, ".bss:taskStackSection:enc_process")
UInt8 gEncProcessTskStack[NUM_HDVICP_RESOURCES][ENC_LINK_PROCESS_TSK_SIZE];

static Int32 EncLink_codecCreateProcessTsk(EncLink_Obj * pObj, UInt32 tskId)
{
    Int32 status = ENC_LINK_S_SUCCESS;
    Task_Params tskParams;
    Error_Block ebObj;
    Error_Block *eb = &ebObj;

    Error_init(eb);

    Task_Params_init(&tskParams);

    snprintf(pObj->encProcessTsk[tskId].name,
             (sizeof(pObj->encProcessTsk[tskId].name) - 1),
             "ENC_PROCESS_TSK_%d ", tskId);
    pObj->encProcessTsk[tskId].
          name[(sizeof(pObj->encProcessTsk[tskId].name) - 1)] = 0;
    tskParams.priority = IVASVR_TSK_PRI;
    tskParams.stack = &gEncProcessTskStack[tskId][0];
    tskParams.stackSize = ENC_LINK_PROCESS_TSK_SIZE;
    tskParams.arg0 = (UArg) pObj;
    tskParams.arg1 = (UArg) tskId;
    tskParams.instance->name = pObj->encProcessTsk[tskId].name;

    Task_construct(&pObj->encProcessTsk[tskId].tskStruct,
                   EncLink_codecProcessTskFxn, &tskParams, eb);
    UTILS_assertError((Error_check(eb) == FALSE), status,
                      ENC_LINK_E_TSKCREATEFAILED, pObj->linkId, tskId);
    if (ENC_LINK_S_SUCCESS == status)
    {
        pObj->encProcessTsk[tskId].tsk =
              Task_handle(&pObj->encProcessTsk[tskId].tskStruct);
    }
    pObj->encProcessTsk[tskId].tskEnv.size = sizeof(pObj->encProcessTsk[tskId].tskEnv);
    pObj->encProcessTsk[tskId].tskEnv.ivaID = tskId;
    Task_setEnv(pObj->encProcessTsk[tskId].tsk,
                &pObj->encProcessTsk[tskId].tskEnv);
    Utils_prfLoadRegister(pObj->encProcessTsk[tskId].tsk,
                          pObj->encProcessTsk[tskId].name);
    return status;
}

static Int32 EncLink_codecDeleteProcessTsk(EncLink_Obj * pObj, UInt32 tskId)
{
    Int32 status = ENC_LINK_S_SUCCESS;

    Utils_queUnBlock(&pObj->encProcessTsk[tskId].processQue);
    Utils_queUnBlock(&pObj->processDoneQue);
    while (Task_getMode(pObj->encProcessTsk[tskId].tsk) != Task_Mode_TERMINATED)
    {
        Task_sleep(ENC_LINK_TASK_POLL_DURATION_MS);
    }
    Utils_prfLoadUnRegister(pObj->encProcessTsk[tskId].tsk);
    Task_destruct(&pObj->encProcessTsk[tskId].tskStruct);
    pObj->encProcessTsk[tskId].tsk = NULL;
    return status;
}


static Int32 EncLink_codecMapCh2ProcessTskId(EncLink_Obj * pObj)
{
    UInt32 chId;
    Int32 status = ENC_LINK_S_SUCCESS;

    for (chId = 0; chId < pObj->inQueInfo.numCh; chId++)
    {
        pObj->ch2ProcessTskId[chId] = Utils_encdecGetEncoderIVAID (chId);
    }

    EncLink_codecRegisterIVAMapCb(pObj);
    return status;
}

static void EncLink_codecCreateInitStats(EncLink_Obj * pObj)
{
    Int32 chId;
    EncLink_ChObj *pChObj;

    pObj->inFrameGetCount = 0;
    pObj->inFramePutCount = 0;

    for (chId = 0; chId < pObj->inQueInfo.numCh; chId++)
    {
        pChObj = &pObj->chObj[chId];

        pChObj->prevFrmRecvTime = 0;
        pChObj->totalProcessTime = 0;
        pChObj->totalFrameIntervalTime = 0;
        pChObj->totalInFrameCnt = 0;
        pChObj->inFrameQueCount = 0;
        pChObj->processReqestCount = 0;
        pChObj->getProcessedFrameCount = 0;
        pChObj->inFrameSkipCount = 0;
        
        pChObj->disableChn = FALSE;


        pChObj->curFrameNum = 0;
        pChObj->inputFrameRate = 30;
        pChObj->algObj.setConfigBitMask = 0;
        pChObj->algObj.getConfigFlag = FALSE;
        pChObj->forceAvoidSkipFrame = FALSE;
        pChObj->forceDumpFrame = FALSE;
        pChObj->frameStatus.firstTime = TRUE;
        pChObj->frameStatus.inCnt = 0;
        pChObj->frameStatus.outCnt = 0;
        pChObj->frameStatus.multipleCnt = 0;

    }

    return;
}

Int32 EncLink_codecCreate(EncLink_Obj * pObj, EncLink_CreateParams * pPrm)
{
    Int32 status;
    Int32 chId, tskId, i;
    EncLink_ChObj *pChObj;

    #ifdef SYSTEM_DEBUG_ENC
    Vps_printf(" %d: ENCODE: Create in progress ... !!!\n",
            Utils_getCurTimeInMsec()
        );
    #endif
    
    #ifdef SYSTEM_DEBUG_MEMALLOC
    Vps_printf ("Before ENC Create:\n");
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
    UTILS_assert(pObj->inQueInfo.numCh <= ENC_LINK_MAX_CH);

    for (i=0; i<ENC_LINK_MAX_BUF_ALLOC_POOLS; i++)
    {
        UTILS_assert(i < UTILS_BITBUF_MAX_ALLOC_POOLS);
        if(pObj->createArgs.numBufPerCh[i] == 0)
            pObj->createArgs.numBufPerCh[i] = ENC_LINK_MAX_OUT_FRAMES_PER_CH;

        if(pObj->createArgs.numBufPerCh[i] > ENC_LINK_MAX_OUT_FRAMES_PER_CH)
        {
            Vps_printf("\n ENCLINK: WARNING: User is asking for %d buffers per CH. But max allowed is %d. \n"
                " Over riding user requested with max allowed \n\n",
                pObj->createArgs.numBufPerCh[i], ENC_LINK_MAX_OUT_FRAMES_PER_CH
                );

            pObj->createArgs.numBufPerCh[i] = ENC_LINK_MAX_OUT_FRAMES_PER_CH;

        }
    }

#if ENCLINK_UPDATE_CREATEPARAMS_LOCALLY
    /* ENABLE the define ENCLINK_UPDATE_CREATEPARAMS_LOCALLY if App is not
     * configuring the create time paramters */
    for (chId = 0; chId < pObj->inQueInfo.numCh; chId++)
    {
        if(pObj->createArgs.chCreateParams[chId].format ==  IVIDEO_H264HP)
        {
        pObj->createArgs.chCreateParams[chId] = ENCLINK_DEFAULTCHCREATEPARAMS_H264;
        }
        else if(pObj->createArgs.chCreateParams[chId].format ==  IVIDEO_MJPEG)
        {
           pObj->createArgs.chCreateParams[chId] = ENCLINK_DEFAULTCHCREATEPARAMS_JPEG;
        }
    }
#endif

    EncLink_codecCreateInitStats(pObj);
    EncLink_codecMapCh2ProcessTskId(pObj);
    EncLink_codecCreateOutObj(pObj);
    EncLink_codecCreateReqObj(pObj);
    EncLink_codecCreateReqObjDummy(pObj);
    pObj->state = SYSTEM_LINK_STATE_START;

    for (chId = 0; chId < pObj->inQueInfo.numCh; chId++)
    {
        pChObj = &pObj->chObj[chId];

        #ifdef SYSTEM_DEBUG_ENC
        Vps_printf(" %d: ENCODE: Creating CH%d of %d x %d, pitch = (%d, %d) [%s] [%s], bitrate = %d Kbps ... \n",
                Utils_getCurTimeInMsec(),
                chId,
                pObj->inQueInfo.chInfo[chId].width,
                pObj->inQueInfo.chInfo[chId].height,
                pObj->inQueInfo.chInfo[chId].pitch[0],
                pObj->inQueInfo.chInfo[chId].pitch[1],
                gSystem_nameScanFormat[pObj->inQueInfo.chInfo[chId].scanFormat],
                gSystem_nameMemoryType[pObj->inQueInfo.chInfo[chId].memType],
                pObj->createArgs.chCreateParams[chId].defaultDynamicParams.targetBitRate/1000
            );
        #endif

        pChObj->nextFid = FVID2_FID_TOP;
        pChObj->inFrameQueCount = 0;
        pChObj->processReqestCount = 0;
        pChObj->getProcessedFrameCount = 0;
        EncLink_codecCreateChObj(pObj, chId);
        EncLink_codecCreateEncObj(pObj, chId);
    }

    #ifdef SYSTEM_DEBUG_ENC
    Vps_printf(" %d: ENCODE: All CH Create ... DONE !!!\n",
            Utils_getCurTimeInMsec()
        );
    #endif

    for (tskId = 0; tskId < NUM_HDVICP_RESOURCES; tskId++)
    {
        status = Utils_queCreate(&pObj->encProcessTsk[tskId].processQue,
                                 UTILS_ARRAYSIZE(pObj->encProcessTsk[tskId].processQueMem),
                                 pObj->encProcessTsk[tskId].processQueMem,
                                 (UTILS_QUE_FLAG_BLOCK_QUE_GET |
                                  UTILS_QUE_FLAG_BLOCK_QUE_PUT));
        UTILS_assert(status == FVID2_SOK);
        EncLink_codecCreateProcessTsk(pObj, tskId);
    }

    EncLink_codecCreatePrdObj(pObj);
    EncLink_codecStartPrdObj(pObj, ENC_LINK_PROCESS_DONE_PERIOD_MS);
    UTILS_MEMLOG_USED_END(pObj->memUsed);
    UTILS_MEMLOG_PRINT("ENCLINK",
                       pObj->memUsed,
                       UTILS_ARRAYSIZE(pObj->memUsed));

    #ifdef SYSTEM_DEBUG_ENC
    Vps_printf(" %d: ENCODE: Create ... DONE !!!\n",
            Utils_getCurTimeInMsec()
        );
    #endif
    
    #ifdef SYSTEM_DEBUG_MEMALLOC
    Vps_printf ("After ENC Create:\n");
    System_memPrintHeapStatus();    
    #endif
    return FVID2_SOK;
}

static Int32 EncLink_codecQueueFramesToChQue(EncLink_Obj * pObj)
{
    UInt32 frameId, freeFrameNum;
    FVID2_Frame *pFrame;
    System_LinkInQueParams *pInQueParams;
    FVID2_FrameList frameList;
    EncLink_ChObj *pChObj;
    Int32 status;
    UInt32 curTime;
    Bool skipFrame;

    pInQueParams = &pObj->createArgs.inQueParams;

    System_getLinksFullFrames(pInQueParams->prevLinkId,
                              pInQueParams->prevLinkQueId, &frameList);

    if (frameList.numFrames)
    {
        pObj->inFrameGetCount += frameList.numFrames;

        freeFrameNum = 0;
        curTime = Utils_getCurTimeInMsec();

        for (frameId = 0; frameId < frameList.numFrames; frameId++)
        {
            pFrame = frameList.frames[frameId];

            pChObj = &pObj->chObj[pFrame->channelNum];

            pChObj->inFrameRecvCount++;

            if (( FALSE == pChObj->algObj.algCreateParams.singleBuf) ||
                (pObj->inQueInfo.chInfo[pFrame->channelNum].scanFormat ==
                                               FVID2_SF_PROGRESSIVE))
            {
                pChObj->nextFid = pFrame->fid;
            }

            skipFrame = FALSE;
            if(pChObj->forceDumpFrame == FALSE)
            {
                skipFrame = EncLink_doSkipFrame(pChObj, pFrame->channelNum);

                if (pChObj->forceAvoidSkipFrame == TRUE)
                    skipFrame = FALSE;
            }
            else
            {
                pChObj->forceDumpFrame = FALSE;
            }

            pChObj->curFrameNum++;

            /* frame skipped due to user setting */
            if(skipFrame || pChObj->disableChn)
                pChObj->inFrameUserSkipCount++;

            /* frame skipped due to framework */
            if(pChObj->nextFid != pFrame->fid && pFrame->fid != FVID2_FID_FRAME)
                pChObj->inFrameRejectCount++;

            if (((pChObj->nextFid == pFrame->fid) ||
                (pFrame->fid == FVID2_FID_FRAME)) &&
                (pChObj->disableChn != TRUE) && (skipFrame == FALSE))
            {
                // frame is of the expected FID use it, else drop it
                pChObj->totalInFrameCnt++;
                if (pChObj->totalInFrameCnt > ENC_LINK_STATS_START_THRESHOLD)
                {
                    pChObj->totalFrameIntervalTime +=
                        (curTime - pChObj->prevFrmRecvTime);

                    /* reserved field in FVID2_Frame used as place holder
                        for current time of submission to encode

                        timeStamp has original capture stamp so that should not be modified
                    */
                    pFrame->reserved = (Ptr)curTime;
                }
                else
                {
                    pChObj->totalFrameIntervalTime = 0;
                    pChObj->totalProcessTime = 0;

                    EncLink_resetStatistics(pObj);
                }
                pChObj->prevFrmRecvTime = curTime;

                status = Utils_quePut(&pChObj->inQue, pFrame, BIOS_NO_WAIT);
                UTILS_assert(status == FVID2_SOK);
                pChObj->inFrameQueCount++;
                pChObj->nextFid ^= 1;                      // toggle to next
                                                           // required FID
            }
            else
            {
                pChObj->inFrameSkipCount++;

                // frame is not of expected FID, so release frame
                frameList.frames[freeFrameNum] = pFrame;
                freeFrameNum++;
                if (pChObj->nextFid == pFrame->fid) 
                {
                    pChObj->nextFid ^= 1;  // toggle to next
                }
            }
        }

        if (freeFrameNum)
        {
            frameList.numFrames = freeFrameNum;
            System_putLinksEmptyFrames(pInQueParams->prevLinkId,
                                       pInQueParams->prevLinkQueId, &frameList);
            pObj->inFramePutCount += freeFrameNum;
        }
    }

    return FVID2_SOK;
}

static Int32 EncLink_codecSubmitData(EncLink_Obj * pObj)
{
    EncLink_ReqObj *pReqObj;
    EncLink_ChObj *pChObj;
    UInt32 chId, numProcessCh;
    FVID2_Frame *pInFrame;
    Bitstream_Buf *pOutBuf;
    Int32 status = FVID2_EFAIL, numReqObjPerProcess;
    UInt32 freeFrameNum, tskId, i;
    FVID2_FrameList frameList;
    System_LinkInQueParams *pInQueParams;
    System_FrameInfo *pInFrameInfo;

    freeFrameNum = 0;
    numProcessCh = 0;
    for (chId = 0; chId < pObj->inQueInfo.numCh; chId++)
    {
      numReqObjPerProcess = 0;
      pChObj = &pObj->chObj[chId];
      while(numReqObjPerProcess < pChObj->numReqObjPerProcess) {
        numReqObjPerProcess++;
        status =
            Utils_queGet(&pObj->reqQue, (Ptr *) & pReqObj, 1,
                         BIOS_NO_WAIT);
        if (UTILS_ISERROR(status)) {
            break;
        }

        tskId = pObj->ch2ProcessTskId[chId];
        pReqObj->type = ENC_LINK_REQ_OBJECT_TYPE_REGULAR;
        pReqObj->ivaSwitchSerializer = NULL;
        if (pChObj->algObj.algCreateParams.singleBuf)
        {
            pReqObj->InFrameList.numFrames = 2;
        }
        else
        {
            pReqObj->InFrameList.numFrames = 1;
        }
        if ((status == FVID2_SOK) &&
            (pChObj->inFrameQueCount >= pReqObj->InFrameList.numFrames) &&
            (!Utils_queIsFull(&pObj->encProcessTsk[tskId].processQue)))
        {
            pOutBuf = NULL;
            UTILS_assert(chId < ENC_LINK_MAX_CH);
            status = Utils_bitbufGetEmptyBuf(&pObj->outObj.bufOutQue,
                                             &pOutBuf,
                                             pObj->outObj.ch2poolMap[chId],
                                             BIOS_NO_WAIT);
            if ((status == FVID2_SOK) && (pOutBuf))
            {

                for (i=0; i<pReqObj->InFrameList.numFrames; i++)
                {
                    status = Utils_queGet(&pChObj->inQue, (Ptr *) & pInFrame, 1,
                                          BIOS_NO_WAIT);
                    UTILS_assert(status == FVID2_SOK);
                    pReqObj->InFrameList.frames[i] = pInFrame;
                    pChObj->inFrameQueCount--;
                }

                pInFrameInfo = (System_FrameInfo *) pInFrame->appData;
                pOutBuf->lowerTimeStamp = (UInt32)(pInFrameInfo->ts64 & 0xFFFFFFFF);
                pOutBuf->upperTimeStamp = (UInt32)(pInFrameInfo->ts64 >> 32);

                pOutBuf->channelNum = pInFrame->channelNum;
                pReqObj->OutBuf = pOutBuf;
                numProcessCh++;
                pChObj->forceAvoidSkipFrame = FALSE;

                status =
                    Utils_quePut(&pObj->encProcessTsk[tskId].processQue,
                                 pReqObj, BIOS_NO_WAIT);
                UTILS_assert(status == FVID2_SOK);
                pChObj->processReqestCount++;
            }
            else
            {
                /* Free the input frame if output buffer is not available */
                for (i=0; i<pReqObj->InFrameList.numFrames; i++)
                {
                    status = Utils_queGet(&pChObj->inQue, (Ptr *) & pInFrame, 1,
                                          BIOS_NO_WAIT);
                    UTILS_assert(status == FVID2_SOK);
                    UTILS_assert(freeFrameNum < UTILS_ARRAYSIZE(frameList.frames));
                    frameList.frames[freeFrameNum] = pInFrame;
                    pChObj->inFrameQueCount--;
                    pChObj->inFrameSkipCount++;
                    pChObj->inFrameRejectCount++;
                    freeFrameNum++;
                    pChObj->forceAvoidSkipFrame = TRUE;
                }

                status = Utils_quePut(&pObj->reqQue, pReqObj, BIOS_NO_WAIT);
                UTILS_assert(status == FVID2_SOK);
                status = FVID2_EFAIL;
                continue;
            }
        }
        else
        {
            /* Free the input frame if processQue is full */
            if (pChObj->inFrameQueCount >= pReqObj->InFrameList.numFrames)
            {
                for (i=0; i<pReqObj->InFrameList.numFrames; i++)
                {
                    status = Utils_queGet(&pChObj->inQue, (Ptr *) & pInFrame, 1,
                                          BIOS_NO_WAIT);
                    UTILS_assert(status == FVID2_SOK);
                    UTILS_assert(freeFrameNum < UTILS_ARRAYSIZE(frameList.frames));
                    frameList.frames[freeFrameNum] = pInFrame;
                    pChObj->inFrameQueCount--;
                    pChObj->inFrameSkipCount++;
                    pChObj->inFrameRejectCount++;
                    freeFrameNum++;
                    pChObj->forceAvoidSkipFrame = TRUE;
                }
            }

            status = Utils_quePut(&pObj->reqQue, pReqObj, BIOS_NO_WAIT);
            UTILS_assert(status == FVID2_SOK);
            status = FVID2_EFAIL;
        }
      }
    }

    if (freeFrameNum)
    {
        /* Free input frames */
        pInQueParams = &pObj->createArgs.inQueParams;
        frameList.numFrames = freeFrameNum;
        System_putLinksEmptyFrames(pInQueParams->prevLinkId,
                                   pInQueParams->prevLinkQueId, &frameList);
        pObj->inFramePutCount += freeFrameNum;
    }

    return status;
}

Int32 EncLink_codecProcessData(EncLink_Obj * pObj)
{
    Int32 status;

    EncLink_codecQueueFramesToChQue(pObj);

    do
    {
        status = EncLink_codecSubmitData(pObj);
    } while (status == FVID2_SOK);

    return FVID2_SOK;
}

static Int32 EncLink_codecGetProcessedData(EncLink_Obj * pObj)
{
    FVID2_FrameList inFrameList;
    UInt32 chId, i, latency;
    Bitstream_BufList outBitBufList;
    System_LinkInQueParams *pInQueParams;
    EncLink_ChObj *pChObj;
    Int32 status = FVID2_EFAIL;
    EncLink_ReqObj *pReqObj;
    Bool sendNotify = FALSE;
    UInt32 curTime;

    outBitBufList.numBufs = 0;
    outBitBufList.appData = NULL;
    inFrameList.numFrames = 0;
    curTime = Utils_getCurTimeInMsec();

    while (!Utils_queIsEmpty(&pObj->processDoneQue)
           &&
           (inFrameList.numFrames < (FVID2_MAX_FVID_FRAME_PTR - 1))
           &&
           (outBitBufList.numBufs < (VIDBITSTREAM_MAX_BITSTREAM_BUFS - 1)))
    {
        status = Utils_queGet(&pObj->processDoneQue, (Ptr *) & pReqObj, 1,
                              BIOS_NO_WAIT);
        if (status != FVID2_SOK)
        {
            break;
        }
        chId = pReqObj->OutBuf->channelNum;
        pChObj = &pObj->chObj[chId];
        if (pChObj->totalInFrameCnt > ENC_LINK_STATS_START_THRESHOLD)
        {
            /* reserved field in FVID2_Frame used as place holder
                for current time of submission to encode

                timeStamp has original capture stamp so that should not be modified
            */

            pChObj->totalProcessTime +=
                (curTime - (UInt32)pReqObj->InFrameList.frames[0]->reserved);
        }

        pChObj->outFrameCount++;
        pChObj->getProcessedFrameCount++;
        for (i = 0; i < pReqObj->InFrameList.numFrames; i++)
        {
            if (chId != pReqObj->InFrameList.frames[i]->channelNum)
            {
                Vps_printf("ENC: Error !!! ChId %d,  Req channelNum - %d.....\n",
                        chId, pReqObj->InFrameList.frames[i]->channelNum);
            }
            UTILS_assert(chId == pReqObj->InFrameList.frames[i]->channelNum);

            inFrameList.frames[inFrameList.numFrames] =
                                           pReqObj->InFrameList.frames[i];
            inFrameList.numFrames++;
        }

        outBitBufList.bufs[outBitBufList.numBufs] = pReqObj->OutBuf;
        outBitBufList.numBufs++;

        pReqObj->OutBuf->encodeTimeStamp = Utils_getCurTimeInMsec();

        latency = pReqObj->OutBuf->encodeTimeStamp - pReqObj->OutBuf->timeStamp;

        if(latency>pChObj->maxLatency)
            pChObj->maxLatency = latency;
        if(latency<pChObj->minLatency)
            pChObj->minLatency = latency;

        status = Utils_quePut(&pObj->reqQue, pReqObj, BIOS_NO_WAIT);
        UTILS_assert(status == FVID2_SOK);
    }

    if (outBitBufList.numBufs)
    {
        status = Utils_bitbufPutFull(&pObj->outObj.bufOutQue,
                                     &outBitBufList);
        UTILS_assert(status == FVID2_SOK);
        sendNotify = TRUE;
    }

    if (inFrameList.numFrames)
    {
        /* Free input frames */
        pInQueParams = &pObj->createArgs.inQueParams;
        System_putLinksEmptyFrames(pInQueParams->prevLinkId,
                                   pInQueParams->prevLinkQueId,
                                   &inFrameList);
        pObj->inFramePutCount += inFrameList.numFrames;
    }


    if (sendNotify)
    {
        /* Send-out the output bitbuffer */
        System_sendLinkCmd(pObj->createArgs.outQueParams.nextLink,
                           SYSTEM_CMD_NEW_DATA);
    }

    return FVID2_SOK;
}

Int32 EncLink_codecGetProcessedDataMsgHandler(EncLink_Obj * pObj)
{
    Int32 status;

    status = EncLink_codecGetProcessedData(pObj);
    UTILS_assert(status == FVID2_SOK);

    return ENC_LINK_S_SUCCESS;
}

static
Int32 EncLink_codecFreeInQueuedBufs(EncLink_Obj * pObj)
{
    System_LinkInQueParams *pInQueParams;
    FVID2_FrameList frameList;

    pInQueParams = &pObj->createArgs.inQueParams;
    System_getLinksFullFrames(pInQueParams->prevLinkId,
                              pInQueParams->prevLinkQueId, &frameList);
    if (frameList.numFrames)
    {
        pObj->inFrameGetCount += frameList.numFrames;
        /* Free input frames */
        System_putLinksEmptyFrames(pInQueParams->prevLinkId,
                                   pInQueParams->prevLinkQueId, &frameList);
        pObj->inFramePutCount += frameList.numFrames;
    }
    return ENC_LINK_S_SUCCESS;
}

Int32 EncLink_codecStop(EncLink_Obj * pObj)
{
    Int32 rtnValue = FVID2_SOK;
    UInt32 tskId;

    ENCLINK_INFO_LOG(pObj->linkId, -1, "Stop in progress !!!\n");
    for (tskId = 0; tskId < NUM_HDVICP_RESOURCES; tskId++)
    {
        Utils_queUnBlock(&pObj->encProcessTsk[tskId].processQue);
    }
    while (Utils_queGetQueuedCount(&pObj->reqQue) != ENC_LINK_MAX_REQ)
    {
        Utils_tskWaitCmd(&pObj->tsk, NULL, ENC_LINK_CMD_GET_PROCESSED_DATA);
        EncLink_codecGetProcessedDataMsgHandler(pObj);
    }

    EncLink_codecFreeInQueuedBufs(pObj);
    ENCLINK_INFO_LOG(pObj->linkId, -1, "Stop done !!!\n");

    return (rtnValue);
}

Int32 EncLink_getFrameRate(EncLink_ChObj *pChObj)
{
    return pChObj->inputFrameRate;
}


Int32 EncLink_codecDelete(EncLink_Obj * pObj)
{
    UInt32 outId, chId, tskId;
    EncLink_ChObj *pChObj;
    EncLink_OutObj *pOutObj;
    Bool tilerUsed = FALSE;
    Int i, bitbuf_index;

    ENCLINK_INFO_LOG(pObj->linkId, -1, " ENC    : Delete in progress !!!\n");
    EncLink_codecStopPrdObj(pObj);
    EncLink_codecDeletePrdObj(pObj);
    pObj->state = SYSTEM_LINK_STATE_STOP;

    for (tskId = 0; tskId < NUM_HDVICP_RESOURCES; tskId++)
    {
        EncLink_codecDeleteProcessTsk(pObj, tskId);
        Utils_queDelete(&pObj->encProcessTsk[tskId].processQue);
    }

    EncLink_codecUnregisterIVAMapCb(pObj);
    for (chId = 0; chId < pObj->inQueInfo.numCh; chId++)
    {
        pChObj = &pObj->chObj[chId];

        if (pChObj->algObj.algCreateParams.tilerEnable)
        {
            tilerUsed = TRUE;
        }
        switch (pChObj->algObj.algCreateParams.format)
        {
            case IVIDEO_H264BP:
            case IVIDEO_H264MP:
            case IVIDEO_H264HP:
                EncLinkH264_algDelete(&pChObj->algObj.u.h264AlgIfObj);
                break;

            case IVIDEO_MJPEG:
                EncLinkJPEG_algDelete(&pChObj->algObj.u.jpegAlgIfObj);
                break;
            default:
                UTILS_assert(FALSE);
        }

        Utils_queDelete(&pChObj->inQue);

        if (pChObj->totalInFrameCnt > ENC_LINK_STATS_START_THRESHOLD)
        {
            pChObj->totalInFrameCnt -= ENC_LINK_STATS_START_THRESHOLD;
        }

        #ifdef SYSTEM_VERBOSE_PRINTS
        Vps_printf(" %d: ENCODE: CH%d: "
                    "FrameNum : %8d, "
                    "Processed Frames : %8d, "
                    "Total Process Time : %8d, "
                    "Total Frame Interval: %8d, "
                    "Dropped Frames: %8d, "
                    "FPS: %8d (Required FPS: %8d)\n",
                    Utils_getCurTimeInMsec(),
                    chId,
                    pChObj->curFrameNum,
                    pChObj->totalInFrameCnt,
                    pChObj->totalProcessTime,
                    pChObj->totalFrameIntervalTime,
                    pChObj->inFrameSkipCount,
                    (pChObj->curFrameNum - pChObj->inFrameSkipCount) /
                    (pChObj->totalFrameIntervalTime/1000),
                    EncLink_getFrameRate(pChObj)
                 );
        #endif

        pChObj->totalInFrameCnt = 0;
        ti_sysbios_knl_Semaphore_destruct(&pChObj->codecProcessMutexMem);
        UTILS_assert(pChObj->codecProcessMutex != NULL);
        pChObj->codecProcessMutex = NULL;
    }

    Utils_queDelete(&pObj->processDoneQue);

    for (outId = 0; outId < ENC_LINK_MAX_OUT_QUE; outId++)
    {
        {
            pOutObj = &pObj->outObj;

            Utils_bitbufDelete(&pOutObj->bufOutQue);
            bitbuf_index = 0;
            for (i = 0; i < pOutObj->numAllocPools; i++)
            {
                UTILS_assert((pOutObj->outBufs[bitbuf_index].bufSize ==
                              pOutObj->buf_size[i]));
                Utils_memBitBufFree(&pOutObj->outBufs[bitbuf_index],
                                    pOutObj->outNumBufs[i]);
                bitbuf_index += pOutObj->outNumBufs[i];
            }
        }
    }

    Utils_queDelete(&pObj->reqQue);
    EncLink_codecDeleteReqObjDummy(pObj);

    if (tilerUsed)
    {
        SystemTiler_freeAll();
    }

    ENCLINK_INFO_LOG(pObj->linkId, -1, " ENC    : Delete done !!!\n");
    return FVID2_SOK;
}


Int32 EncLink_resetStatistics(EncLink_Obj * pObj)
{
    UInt32 chId, tskId;
    EncLink_ChObj *pChObj;

    for (chId = 0; chId < pObj->inQueInfo.numCh; chId++)
    {
        pChObj = &pObj->chObj[chId];

        pChObj->inFrameRecvCount = 0;
        pChObj->inFrameRejectCount = 0;
        pChObj->inFrameUserSkipCount = 0;
        pChObj->outFrameCount = 0;
        pChObj->minLatency = 0xFF;
        pChObj->maxLatency = 0;
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

Int32 EncLink_printStatistics (EncLink_Obj * pObj, Bool resetAfterPrint)
{
    UInt32 chId, ivaId;
    EncLink_ChObj *pChObj;
    UInt32 elaspedTime;

    elaspedTime = Utils_getCurTimeInMsec() - pObj->statsStartTime; // in msecs
    elaspedTime /= 1000; // convert to secs

    Vps_printf( " \n"
            " *** ENCODE Statistics *** \n"
            " \n"
            " Elasped Time           : %d secs\n"
            " \n"
            " \n"
            " CH  | In Recv In Skip In User  Out Latency  \n"
            " Num | FPS     FPS     Skip FPS FPS Min / Max\n"
            " --------------------------------------------\n",
            elaspedTime
            );

    for (chId = 0; chId < pObj->inQueInfo.numCh; chId++)
    {
        pChObj = &pObj->chObj[chId];

        Vps_printf( " %3d | %7d %7d %8d %3d %3d / %3d\n",
                    chId,
            pChObj->inFrameRecvCount/elaspedTime,
            pChObj->inFrameRejectCount/elaspedTime,
            pChObj->inFrameUserSkipCount/elaspedTime,
            pChObj->outFrameCount/elaspedTime,
            pChObj->minLatency,
            pChObj->maxLatency
                 );
    }

    Vps_printf( " \n");
    Vps_printf("Multi Channel Encode Average Submit Batch Size \n");
    Vps_printf("Max Submit Batch Size : %d\n", ENC_LINK_GROUP_SUBM_MAX_SIZE);
    
    for (ivaId = 0; ivaId < NUM_HDVICP_RESOURCES; ivaId++)
    {
      Vps_printf ("IVAHD_%d Average Batch Size : %d\n", 
                  ivaId, pObj->batchStatistics[ivaId].averageBatchSize);
      Vps_printf ("IVAHD_%d Max achieved Batch Size : %d\n", 
                  ivaId, pObj->batchStatistics[ivaId].maxAchievedBatchSize);                  
    }

    Vps_printf( " \n");
    
    Vps_printf("Multi Channel Encode Batch break Stats \n");
    
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
    

    if(resetAfterPrint)
    {
        EncLink_resetStatistics(pObj);
    }

    return FVID2_SOK;
}

Int32 EncLink_codecGetDynParams(EncLink_Obj * pObj,
                              EncLink_GetDynParams * params)
{
    Int32 status = ENC_LINK_S_SUCCESS;
    EncLink_ChObj *pChObj;
/*    EncLink_AlgDynamicParams algDynamicParams;*/
    UInt key;

    key = Hwi_disable();
    pChObj = &pObj->chObj[params->chId];

    pChObj->algObj.getConfigFlag = TRUE;

    Hwi_restore(key);

    return (status);
}

Int32 EncLink_codecSetBitrate(EncLink_Obj * pObj,
                              EncLink_ChBitRateParams * params)
{
    Int32 status = ENC_LINK_S_SUCCESS;
    EncLink_ChObj *pChObj;
    UInt key;

    key = Hwi_disable();
    pChObj = &pObj->chObj[params->chId];
    pChObj->algObj.algDynamicParams.targetBitRate = params->targetBitRate;
    pChObj->algObj.setConfigBitMask |= (1 << ENC_LINK_SETCONFIG_BITMASK_BITRARATE);
    Hwi_restore(key);

    return (status);
}

Int32 EncLink_codecSetFps(EncLink_Obj * pObj, EncLink_ChFpsParams * params)
{
    Int32 status = ENC_LINK_S_SUCCESS;
    EncLink_ChObj *pChObj;
    UInt key;

    key = Hwi_disable();
    pChObj = &pObj->chObj[params->chId];

    pChObj->frameStatus.firstTime = TRUE;

    pChObj->algObj.algDynamicParams.targetFrameRate = params->targetFps;

    if(params->targetBitRate != 0)
    {
        pChObj->algObj.algDynamicParams.targetBitRate = params->targetBitRate;
        pChObj->algObj.setConfigBitMask |= (1 << ENC_LINK_SETCONFIG_BITMASK_BITRARATE);
    }

    pChObj->algObj.setConfigBitMask |= (1 << ENC_LINK_SETCONFIG_BITMASK_FPS);
    Hwi_restore(key);

    return (status);
}

Int32 EncLink_codecInputSetFps(EncLink_Obj * pObj, EncLink_ChInputFpsParam * params)
{
    Int32 status = ENC_LINK_S_SUCCESS;
    EncLink_ChObj *pChObj;
    UInt key;

    key = Hwi_disable();
    pChObj = &pObj->chObj[params->chId];

    pChObj->inputFrameRate = params->inputFps;
    pChObj->frameStatus.firstTime = TRUE;

    Hwi_restore(key);

    return (status);
}

Int32 EncLink_codecSetIntraIRate(EncLink_Obj * pObj,
                              EncLink_ChIntraFrIntParams * params)
{
    Int32 status = ENC_LINK_S_SUCCESS;
    EncLink_ChObj *pChObj;
    UInt key;

    key = Hwi_disable();
    pChObj = &pObj->chObj[params->chId];
    pChObj->algObj.algDynamicParams.intraFrameInterval = params->intraFrameInterval;
    pChObj->algObj.setConfigBitMask |= (1 << ENC_LINK_SETCONFIG_BITMASK_INTRAI);
    Hwi_restore(key);

    return (status);
}

Int32 EncLink_codecSetForceIDR(EncLink_Obj * pObj,
                              EncLink_ChForceIFrParams * params)
{
    Int32 status = ENC_LINK_S_SUCCESS;
    EncLink_ChObj *pChObj;
    UInt key;

    key = Hwi_disable();
    pChObj = &pObj->chObj[params->chId];
    pChObj->algObj.setConfigBitMask |= (1 << ENC_LINK_SETCONFIG_BITMASK_FORCEI);
    Hwi_restore(key);

    return (status);
}

Int32 EncLink_codecSetrcAlg(EncLink_Obj * pObj, EncLink_ChRcAlgParams* params)
{
    Int32 status = ENC_LINK_S_SUCCESS;
    EncLink_ChObj *pChObj;
    UInt key;

    key = Hwi_disable();
    pChObj = &pObj->chObj[params->chId];
    pChObj->algObj.algDynamicParams.rcAlg = params->rcAlg;
    pChObj->algObj.setConfigBitMask |= (1 << ENC_LINK_SETCONFIG_BITMASK_RCALGO);
    Hwi_restore(key);

    return (status);
}


Int32 EncLink_codecSetqpParamI(EncLink_Obj * pObj,
                              EncLink_ChQPParams * params)
{
    Int32 status = ENC_LINK_S_SUCCESS;
    EncLink_ChObj *pChObj;
    UInt key;

    key = Hwi_disable();
    pChObj = &pObj->chObj[params->chId];
    pChObj->algObj.algDynamicParams.qpMinI = params->qpMin;
    pChObj->algObj.algDynamicParams.qpMaxI = params->qpMax;
    pChObj->algObj.algDynamicParams.qpInitI = params->qpInit;
    pChObj->algObj.setConfigBitMask |= (1 << ENC_LINK_SETCONFIG_BITMASK_QPI);
    Hwi_restore(key);

    return (status);
}

Int32 EncLink_codecSetqpParamP(EncLink_Obj * pObj,
                              EncLink_ChQPParams * params)
{
    Int32 status = ENC_LINK_S_SUCCESS;
    EncLink_ChObj *pChObj;
    UInt key;

    key = Hwi_disable();
    pChObj = &pObj->chObj[params->chId];
    pChObj->algObj.algDynamicParams.qpMinP = params->qpMin;
    pChObj->algObj.algDynamicParams.qpMaxP = params->qpMax;
    pChObj->algObj.algDynamicParams.qpInitP = params->qpInit;
    pChObj->algObj.setConfigBitMask |= (1 << ENC_LINK_SETCONFIG_BITMASK_QPP);
    Hwi_restore(key);

    return (status);
}

Int32 EncLink_codecForceDumpFrame(EncLink_Obj * pObj,
                              EncLink_ChannelInfo * params)
{
    Int32 status = ENC_LINK_S_SUCCESS;
    EncLink_ChObj *pChObj;
    UInt key;

    key = Hwi_disable();
    pChObj = &pObj->chObj[params->chId];
    pChObj->forceDumpFrame = TRUE;
    Hwi_restore(key);

    return (status);
}

Int32 EncLink_codecSetVBRDuration(EncLink_Obj * pObj, EncLink_ChCVBRDurationParams *params)
{
    Int32 status = ENC_LINK_S_SUCCESS;
    EncLink_ChObj *pChObj;
    UInt key;

    key = Hwi_disable();
    pChObj = &pObj->chObj[params->chId];
    pChObj->algObj.algDynamicParams.vbrDuration = params->vbrDuration;

    pChObj->algObj.setConfigBitMask |= (1 << ENC_LINK_SETCONFIG_BITMASK_VBRD);
    Hwi_restore(key);

    return (status);
}
Int32 EncLink_codecSetVBRSensitivity(EncLink_Obj * pObj, EncLink_ChCVBRSensitivityParams *params)
{
    Int32 status = ENC_LINK_S_SUCCESS;
    EncLink_ChObj *pChObj;
    UInt key;

    key = Hwi_disable();
    pChObj = &pObj->chObj[params->chId];
    pChObj->algObj.algDynamicParams.vbrSensitivity = params->vbrSensitivity;

    pChObj->algObj.setConfigBitMask |= (1 << ENC_LINK_SETCONFIG_BITMASK_VBRS);
    Hwi_restore(key);

    return (status);
}

Int32 EncLink_codecSetROIPrms(EncLink_Obj * pObj, EncLink_ChROIParams * params)
{
    Int32 status = ENC_LINK_S_SUCCESS;

    EncLink_ChObj *pChObj;

    UInt key;

    key = Hwi_disable();
    pChObj = &pObj->chObj[params->chId];
    int i = 0;

    if(params->numOfRegion > 4)
    {
        printf("Warning!! Maximum only 4 ROIs are allowed, defaulting to 4");
        params->numOfRegion = ENC_LINK_CURRENT_MAX_ROI;                
    }

    pChObj->algObj.algDynamicParams.roiParams.roiNumOfRegion = params->numOfRegion;

    for (i = 0; i < params->numOfRegion; i++)
    {
        pChObj->algObj.algDynamicParams.roiParams.roiStartX[i] = params->startX[i];
        pChObj->algObj.algDynamicParams.roiParams.roiStartY[i] = params->startY[i];
        pChObj->algObj.algDynamicParams.roiParams.roiWidth[i] = params->width[i];
        pChObj->algObj.algDynamicParams.roiParams.roiHeight[i] = params->height[i];
        pChObj->algObj.algDynamicParams.roiParams.roiType[i] = params->type[i];                   
       /*
              *  roiPriority: Valid values include all integers between -8 and 8, inclusive. 
              *  A higher value means that more importance will be given to the ROI compared to other regions.
              *  This parameter holds the mask color information if ROI is of type privacy mask. 
              **/

        pChObj->algObj.algDynamicParams.roiParams.roiPriority[i] = params->roiPriority[i];

        /*Checks for out of bounds entered values*/
        if((pChObj->algObj.algDynamicParams.roiParams.roiStartX[i]
            + pChObj->algObj.algDynamicParams.roiParams.roiWidth[i]) 
            > pObj->inQueInfo.chInfo[params->chId].width)
        {
            pChObj->algObj.algDynamicParams.roiParams.roiWidth[i] = 
                pObj->inQueInfo.chInfo[params->chId].width 
                - pChObj->algObj.algDynamicParams.roiParams.roiStartX[i];
        }
        if((pChObj->algObj.algDynamicParams.roiParams.roiStartY[i]
            + pChObj->algObj.algDynamicParams.roiParams.roiHeight[i]) 
            > pObj->inQueInfo.chInfo[params->chId].height)
        {
            pChObj->algObj.algDynamicParams.roiParams.roiHeight[i] = 
               pObj->inQueInfo.chInfo[params->chId].height 
               - pChObj->algObj.algDynamicParams.roiParams.roiStartY[i];
        }

        if((pChObj->algObj.algDynamicParams.roiParams.roiStartX[i] < 0) ||
           (pChObj->algObj.algDynamicParams.roiParams.roiStartY[i] < 0) ||
           ((pChObj->algObj.algDynamicParams.roiParams.roiStartX[i]
            + pChObj->algObj.algDynamicParams.roiParams.roiWidth[i]) < 0) ||
            ((pChObj->algObj.algDynamicParams.roiParams.roiStartY[i]
            + pChObj->algObj.algDynamicParams.roiParams.roiHeight[i]) < 0))
        {
            Vps_printf("Warning! Out of Bounds ROI parameters. ROI Privacy mask Disabled");
            pChObj->algObj.algDynamicParams.roiParams.roiNumOfRegion = 0;
        }

        if((pChObj->algObj.algDynamicParams.roiParams.roiStartX[i] 
           >= pObj->inQueInfo.chInfo[params->chId].width) ||
           (pChObj->algObj.algDynamicParams.roiParams.roiStartY[i]
           >= pObj->inQueInfo.chInfo[params->chId].height)||
           (pChObj->algObj.algDynamicParams.roiParams.roiWidth[i]
           > pObj->inQueInfo.chInfo[params->chId].width)||
           (pChObj->algObj.algDynamicParams.roiParams.roiHeight[i]
           > pObj->inQueInfo.chInfo[params->chId].height))
        {
            Vps_printf("Warning! Out of Bounds ROI parameters. ROI Privacy mask Disabled");
            pChObj->algObj.algDynamicParams.roiParams.roiNumOfRegion = 0;
        }
    }
    
    pChObj->algObj.setConfigBitMask |= (1 << ENC_LINK_SETCONFIG_BITMASK_ROI);
    Hwi_restore(key);

    return (status);
}


Int32 EncLink_codecDisableChannel(EncLink_Obj * pObj,
                              EncLink_ChannelInfo* params)
{
    Int32 status = ENC_LINK_S_SUCCESS;
    EncLink_ChObj *pChObj;
    UInt key;

    key = Hwi_disable();
    pChObj = &pObj->chObj[params->chId];
    pChObj->disableChn = TRUE;
    Hwi_restore(key);

    return (status);
}

Int32 EncLink_codecEnableChannel(EncLink_Obj * pObj,
                              EncLink_ChannelInfo* params)
{
    Int32 status = ENC_LINK_S_SUCCESS;
    EncLink_ChObj *pChObj;
    UInt key;

    key = Hwi_disable();
    pChObj = &pObj->chObj[params->chId];
    pChObj->disableChn = FALSE;
    Hwi_restore(key);

    return (status);
}

Bool  EncLink_doSkipFrame(EncLink_ChObj *pChObj, Int32 chId)
{
    /*if the target framerate has changed, first time case needs to be visited?*/
    if(pChObj->frameStatus.firstTime)
    {
        pChObj->frameStatus.outCnt = 0;
        pChObj->frameStatus.inCnt = 0;

        pChObj->frameStatus.multipleCnt = pChObj->inputFrameRate *
            (pChObj->algObj.algDynamicParams.targetFrameRate/1000);

        pChObj->frameStatus.firstTime = FALSE;
        #ifdef SYSTEM_VERBOSE_PRINTS
        Vps_printf("\r\n Channel:%d inputframerate:%d targetfps:%d",
            chId,
            pChObj->inputFrameRate,
            (pChObj->algObj.algDynamicParams.targetFrameRate/1000));
        #endif
    }

    if (pChObj->frameStatus.inCnt > pChObj->frameStatus.outCnt)
    {
                pChObj->frameStatus.outCnt +=
                    (pChObj->algObj.algDynamicParams.targetFrameRate/1000);
                /*skip this frame, return true*/
                return TRUE;
    }

    // out will also be multiple
    if (pChObj->frameStatus.inCnt == pChObj->frameStatus.multipleCnt)
    {
        // reset to avoid overflow
        pChObj->frameStatus.inCnt = pChObj->frameStatus.outCnt = 0;
    }

    pChObj->frameStatus.inCnt += pChObj->inputFrameRate;
    pChObj->frameStatus.outCnt += (pChObj->algObj.algDynamicParams.targetFrameRate/1000);
    
    if(pChObj->algObj.algDynamicParams.targetFrameRate == 0)
        return TRUE;

    /*display this frame, hence return false*/
    return FALSE;
}

/*
  ASSEMPTION!!!
  EncLink_codecDynamicResolutionChnage() Assumes only one input frame
  or two fileds (in case of feild merge mode) present in each reqObj
*/
static Int32 EncLink_codecDynamicResolutionChange(EncLink_Obj * pObj,
                                 EncLink_ReqObj * reqObj, UInt32 chId)
{
    Int32 status = ENC_LINK_S_SUCCESS;
    EncLink_ChObj * pChObj;
    System_FrameInfo *pFrameInfo;
    EncLink_AlgDynamicParams *chDynamicParams;
    EncLink_ChForceIFrParams IdrParams;
    Bool rtParamUpdatePerFrame;

    pChObj = &pObj->chObj[chId];
    rtParamUpdatePerFrame = FALSE;
    
    UTILS_assert (chId == reqObj->OutBuf->channelNum);
    
    pFrameInfo = (System_FrameInfo *)
                  reqObj->InFrameList.frames[0]->appData;
    reqObj->OutBuf->inputFileChanged = FALSE;

    if ((pFrameInfo != NULL) && (pFrameInfo->rtChInfoUpdate == TRUE))
    {
        chDynamicParams = &pChObj->algObj.algDynamicParams;
        if (pFrameInfo->rtChInfo.height != chDynamicParams->inputHeight)
        {
            chDynamicParams->inputHeight = pFrameInfo->rtChInfo.height;
            rtParamUpdatePerFrame = TRUE;
        }
        if (pFrameInfo->rtChInfo.width != chDynamicParams->inputWidth)
        {
            chDynamicParams->inputWidth = pFrameInfo->rtChInfo.width;
            rtParamUpdatePerFrame = TRUE;
        }
        if (pFrameInfo->rtChInfo.pitch[0] != chDynamicParams->inputPitch)
        {
            chDynamicParams->inputPitch = pFrameInfo->rtChInfo.pitch[0];
            rtParamUpdatePerFrame = TRUE;
        }

        if (rtParamUpdatePerFrame == TRUE)
        {
            switch (pChObj->algObj.algCreateParams.format)
            {
                case IVIDEO_H264BP:
                case IVIDEO_H264MP:
                case IVIDEO_H264HP:
                    status = EncLinkH264_algDynamicParamUpdate(
                                         &pChObj->algObj.u.h264AlgIfObj,
                                         &pChObj->algObj.algCreateParams,
                                         &pChObj->algObj.algDynamicParams);
                    if (UTILS_ISERROR(status))
                    {
                        UTILS_warn("ENCLINK:ERROR in "
                        "EncLinkH264_algDynamicParamUpdate.Status[%d]", status);
                    }
                    IdrParams.chId = chId;
                    EncLink_codecSetForceIDR(pObj, &IdrParams);
                    break;

                case IVIDEO_MJPEG:
                    status = EncLinkJPEG_algDynamicParamUpdate(
                                         &pChObj->algObj.u.jpegAlgIfObj,
                                         &pChObj->algObj.algCreateParams,
                                         &pChObj->algObj.algDynamicParams);
                    if (UTILS_ISERROR(status))
                    {
                        UTILS_warn("ENCLINK:ERROR in "
                        "EncLinkJPEG_algDynamicParamUpdate.Status[%d]", status);
                    }

                    /** Note: The below call to SetForceIDR has been called 
                              just to make a control call to the JPEG encoder, 
                              so that Dynamic resolution takes effect **/

                    IdrParams.chId = chId;
                    EncLink_codecSetForceIDR(pObj, &IdrParams);
                    break;
                default:
                    UTILS_assert(FALSE);
            }

            reqObj->OutBuf->inputFileChanged = TRUE;
         }
        pFrameInfo->rtChInfoUpdate = FALSE;
    }

    return (status);
}

static Int32 EncLink_PrepareBatch (EncLink_Obj *pObj, UInt32 tskId, 
                                   EncLink_ReqObj *pReqObj, 
                                   EncLink_ReqBatch *pReqObjBatch)
{
  Int32 channelId, newObjChannelId, codecClassSwitch = 0;
  Int32 status = FVID2_SOK;
  UInt32 contentType;  
#ifdef SYSTEM_DEBUG_MULTI_CHANNEL_ENC
  Int32 i;
#endif

  Bool batchPreperationDone = FALSE;
  EncLink_ReqObj *pNewReqObj;
  EncLink_ChObj *pChObj;
  Int32 inputFrameWidth, maxBatchSize = 0;
  

  
  /*Reset the submitted flag at the start of batch preperation*/
  pReqObjBatch->channelSubmittedFlag = 0x0;
  pReqObjBatch->codecSubmittedFlag = 0x0;
  pReqObjBatch->numReqObjsInBatch = 0;

  pReqObjBatch->pReqObj[pReqObjBatch->numReqObjsInBatch++] = pReqObj;

  channelId = pReqObj->OutBuf->channelNum;

  contentType =
           Utils_encdecMapFVID2XDMContentType(pObj->inQueInfo.chInfo[channelId].
                                              scanFormat);  
  pChObj = &pObj->chObj[channelId];
  inputFrameWidth = pObj->inQueInfo.chInfo[channelId].width;
 
  /*Since this is the first ReqList in the Batch, the channel submit and codec 
     submit bits wont have been set a-priori.*/
  pReqObjBatch->channelSubmittedFlag = pReqObjBatch->channelSubmittedFlag | 
                                        (0x1 << channelId);
  
  
  if ((UTILS_ENCDEC_RESOLUTION_CLASS_1080P_WIDTH >= inputFrameWidth) &&
      (UTILS_ENCDEC_RESOLUTION_CLASS_720P_WIDTH < inputFrameWidth))
  {
    /*If the element has width greateer than 1920, 
      only 1 channels possible per Batch*/
    maxBatchSize = 1;
  }
  else if ((UTILS_ENCDEC_RESOLUTION_CLASS_720P_WIDTH >= inputFrameWidth) &&
           (UTILS_ENCDEC_RESOLUTION_CLASS_D1_WIDTH < inputFrameWidth))
  {
    /*If the element has width between 1280 and (including )1920, Batch can have 
      only 2 channels*/
    maxBatchSize = MIN (2, ENC_LINK_GROUP_SUBM_MAX_SIZE);
  }
  else if (UTILS_ENCDEC_RESOLUTION_CLASS_D1_WIDTH >= inputFrameWidth)
  {
    maxBatchSize = ENC_LINK_GROUP_SUBM_MAX_SIZE;
  }
  else
  {
    UTILS_assert (FALSE);
  }
  
  /*Set the flag for which codec class this REqObject belongs to.*/
  switch (pChObj->algObj.algCreateParams.format)
  {
      case IVIDEO_H264BP:
      case IVIDEO_H264MP:
      case IVIDEO_H264HP:
          pReqObjBatch->codecSubmittedFlag = pReqObjBatch->codecSubmittedFlag |
                                              (0x1 << 
                                               ENC_LINK_GROUP_CODEC_CLASS_H264);
          break;

      case IVIDEO_MJPEG:
          pReqObjBatch->codecSubmittedFlag = pReqObjBatch->codecSubmittedFlag |
                                              (0x1 << 
                                               ENC_LINK_GROUP_CODEC_CLASS_JPEG);
          break;
      default:
          UTILS_assert(FALSE);
  }
                                        
  while (FALSE == batchPreperationDone)  
  {
    if (pReqObjBatch->numReqObjsInBatch >= maxBatchSize)
    {
      /*The number of Request Lists has exceeded the maximum batch size 
        supported be the codec.*/
      #ifdef SYSTEM_DEBUG_MULTI_CHANNEL_ENC    
      Vps_printf ("ENC : IVAHDID : %d Number of Req Objs exceeded limit: %d\n", tskId,
                  maxBatchSize);
      #endif        
      batchPreperationDone = TRUE;
      pObj->debugBatchPrepStats[tskId].numReasonSizeExceeded++;
      continue;
    }
    if (Utils_queIsEmpty (&pObj->encProcessTsk[tskId].processQue))
    {
      /*There are no more request Objects to be dequeued. Batch generation done*/
      #ifdef SYSTEM_DEBUG_MULTI_CHANNEL_ENC      
      Vps_printf ("ENC : IVAHDID : %d Incoming Queue is empty. Batch generation complete!!",
                  tskId);
      #endif
      batchPreperationDone = TRUE;
      pObj->debugBatchPrepStats[tskId].numReasonReqObjQueEmpty++;
      continue;

    }
    
    /*Peek at the next Request Obj in the Process Queue*/    
    status = Utils_quePeek(&pObj->encProcessTsk[tskId].processQue,
                           (Ptr *) &pNewReqObj);
    
    if (status != FVID2_SOK)
      return status;                           
    
    if (pNewReqObj->type != ENC_LINK_REQ_OBJECT_TYPE_REGULAR)
    {
        batchPreperationDone = TRUE;
        continue;
    }

    
    newObjChannelId = pNewReqObj->OutBuf->channelNum;
    inputFrameWidth = pObj->inQueInfo.chInfo[newObjChannelId].width;
    
    if (UTILS_ENCDEC_RESOLUTION_CLASS_720P_WIDTH < inputFrameWidth)
    {
      /*If the Input Frame Width is greater than 1280, the number of elements in 
        one Batch cannot be more than one. Since atleast one element has already 
        been pushed, we will be able to submit this element only as a part of a 
        new Batch  */
      #ifdef SYSTEM_DEBUG_MULTI_CHANNEL_ENC      
      Vps_printf ("ENC : IVAHDID : %d An element with width greater than 1280 encountered!!\n",
                  tskId);
      #endif          
      batchPreperationDone = TRUE;
      pObj->debugBatchPrepStats[tskId].numReasonResoultionClass++;
      continue;
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
      #ifdef SYSTEM_DEBUG_MULTI_CHANNEL_ENC      
      Vps_printf ("ENC : IVAHDID : %d Channel repeated within Batch!!\n", tskId);
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
                                                ENC_LINK_GROUP_CODEC_CLASS_H264);
            break;

        case IVIDEO_MJPEG:
            codecClassSwitch = pReqObjBatch->codecSubmittedFlag &
                                                (0x1 << 
                                                ENC_LINK_GROUP_CODEC_CLASS_JPEG);
            break;
        default:
            UTILS_assert(FALSE);
    }    
    
    if (! codecClassSwitch)
    {
      /*A codec switch from JPEG to H264 or vice-versa has happened and this 
        Request object cannot be part of the batch to be submitted. 
        Batch generation done.*/
      #ifdef SYSTEM_DEBUG_MULTI_CHANNEL_ENC      
      Vps_printf ("ENC : IVAHDID : %d Codec Switch occured!!. Batch generation complete!!",
                  tskId);
      #endif         
      batchPreperationDone = TRUE;
      pObj->debugBatchPrepStats[tskId].numReasonCodecSwitch++;
      continue;      
    }
    
    /*Now that the Request Obj is eligible to be part of the Batch, include it.
     */
    status = Utils_queGet(&pObj->encProcessTsk[tskId].processQue,
                          (Ptr *) &pNewReqObj, 1, BIOS_WAIT_FOREVER);     
    
    pReqObjBatch->pReqObj[pReqObjBatch->numReqObjsInBatch++] = pNewReqObj;
      
  }
  
  if (FVID2_SOK == status)
  {
    /*Print Debug details of the Batch created*/
    #ifdef SYSTEM_DEBUG_MULTI_CHANNEL_ENC
    Vps_printf("ENC : IVAHDID : %d Batch creation ... DONE !!!\n", tskId);
    Vps_printf("ENC : IVAHDID : %d Number of Req Objs in Batch : %d\n", 
               tskId, pReqObjBatch->numReqObjsInBatch);
    Vps_printf ("ENC : IVAHDID : %d Channels included in Batch:\n", tskId);
    for ( i = 0; i < pReqObjBatch->numReqObjsInBatch; i++)
    {
      Vps_printf ("ENC : IVAHDID : %d %d\n", tskId, 
                  pReqObjBatch->reqObj[i]->OutBuf->channelNum);
    }

    #endif    
  }

  pObj->debugBatchPrepStats[tskId].numBatchCreated++;

  return status;
}


static Int32 EncLink_SubmitBatch (EncLink_Obj *pObj, UInt32 tskId, 
                                  EncLink_ReqBatch *pReqObjBatch)
{

  Int32 status = FVID2_SOK, i, chId;
  EncLink_ReqObj *pReqObj;
  EncLink_ChObj *pChObj;
  IVIDEO_Format format = IVIDEO_H264BP;
  HDVICP_tskEnv  *tskEnv;
    
#ifdef SYSTEM_DEBUG_MULTI_CHANNEL_ENC
  Vps_printf ("ENC : IVAHDID : %d Entering EncLink_SubmitBatch.\n", tskId);
#endif

  /*Make sure that the req Object Batch is not empty*/
  UTILS_assert (pReqObjBatch->numReqObjsInBatch > 0);

  /*For every batch submitted, send the batch size to FC so that total
  number of channels processed can be logged.*/
  tskEnv = Task_getEnv(Task_self());
  UTILS_assert(tskEnv!= NULL);
  tskEnv->batchSize = pReqObjBatch->numReqObjsInBatch;
  
  /*Update Runtime Parameters for the encoder codecs, one channel at a time*/
  for (i = 0; i < pReqObjBatch->numReqObjsInBatch; i++)
  {
    pReqObj = pReqObjBatch->pReqObj[i];
    chId = pReqObj->OutBuf->channelNum;
    pChObj = &pObj->chObj[chId];
    
    Semaphore_pend(pChObj->codecProcessMutex,ti_sysbios_BIOS_WAIT_FOREVER);
    EncLink_codecDynamicResolutionChange (pObj, pReqObj, chId);
    format = pChObj->algObj.algCreateParams.format;
    /*Submit the batch to either H264 encoder or JPEG encoder for processing 
      Enclink_h264EncodeFrameBatch will utilize the .processMulti call and 
      Enclink_jpegEncodeFrame will utilize the process*/

    switch (format)
    {
        case IVIDEO_H264BP:
        case IVIDEO_H264MP:
        case IVIDEO_H264HP:
            /*As SetConfig and GetConfig are moved inside the SubmitBatch fucntion
              to minimize aquire and release calls required. */
           
            /*Submit for Encoding after all the channels have been configured*/
            if (UTILS_ISERROR(status))
            {
                UTILS_warn("ENCLINK:ERROR in "
                           "Enclink_h264EncodeFrame.Status[%d]", status);
            }
            break;

        case IVIDEO_MJPEG:
   
            
            status = EncLinkJPEG_algSetConfig(&pChObj->algObj);
            status = EncLinkJPEG_algGetConfig(&pChObj->algObj);
            UTILS_assert(ENC_LINK_REQ_OBJECT_TYPE_REGULAR == pReqObj->type);
            status = Enclink_jpegEncodeFrame(pChObj, pReqObj);
            
    
            if (UTILS_ISERROR(status))
            {
                UTILS_warn("ENCLINK:ERROR in "
                           "Enclink_JPEGEncodeFrame.Status[%d]", status);
            }
            break;
        default:
            UTILS_assert(FALSE);
    }
    
  }
  /*Process all the available H.264 channels in one submission.*/
  
    switch (format)
    {
        case IVIDEO_H264BP:
        case IVIDEO_H264MP:
        case IVIDEO_H264HP:
            status = Enclink_h264EncodeFrameBatch(pObj, pReqObjBatch, tskId);
            if (UTILS_ISERROR(status))
            {
                UTILS_warn("ENCLINK:ERROR in "
                           "Enclink_h264EncodeFrameBatch.Status[%d] for IVAHD_%d", 
                           status, tskId);
            }
            break;

        case IVIDEO_MJPEG:
            /*Nothing to be done. Process call already exerted.*/
            break;
        default:
            UTILS_assert(FALSE);
    }

  /*Update Runtime Parameters for the encoder codecs, one channel at a time*/
  for (i = 0; i < pReqObjBatch->numReqObjsInBatch; i++)
  {
    pReqObj = pReqObjBatch->pReqObj[i];
    chId = pReqObj->OutBuf->channelNum;
    pChObj = &pObj->chObj[chId];

    Semaphore_post(pChObj->codecProcessMutex);
  }
#ifdef SYSTEM_DEBUG_MULTI_CHANNEL_ENC
  Vps_printf ("ENC : IVAHDID : %d Leaving EncLink_SubmitBatch with status %d\n"
              ,tskId, status);
#endif  
  return status;
}                                  
Int32 EncLink_printBufferStatus (EncLink_Obj * pObj)
{
    Uint8 str[256];
    
    Vps_rprintf(
        " \n"
        " *** Encode Statistics *** \n"
        "\n  %d: ENC: Rcvd from prev = %d, Returned to prev = %d\r\n",
        Utils_getCurTimeInMsec(), pObj->inFrameGetCount, pObj->inFramePutCount);

    sprintf ((char *)str, "ENC Out ");
    Utils_bitbufPrintStatus(str, &pObj->outObj.bufOutQue);
    return 0;
}



static Int32 EncLink_codecCreateIvaMapMutex(EncLink_Obj * pObj)
{
    ti_sysbios_gates_GateMutexPri_Params prms;

    UTILS_assert(pObj->ivaChMapMutex == NULL);

    ti_sysbios_gates_GateMutexPri_Params_init(&prms);
    ti_sysbios_gates_GateMutexPri_construct(&pObj->ivaChMapMutexObj,
                                            &prms);
    pObj->ivaChMapMutex =
    ti_sysbios_gates_GateMutexPri_handle(&pObj->ivaChMapMutexObj);

    return (ENC_LINK_S_SUCCESS);
}

static Int32 EncLink_codecDeleteIvaMapMutex(EncLink_Obj * pObj)
{
    UTILS_assert(pObj->ivaChMapMutex != NULL);

    ti_sysbios_gates_GateMutexPri_destruct(&pObj->ivaChMapMutexObj);
    pObj->ivaChMapMutex = NULL;
    return (ENC_LINK_S_SUCCESS);
}

static Int32 EncLink_codecCreateIvaSwitchSerializerObj(EncLink_Obj * pObj)
{
    Int32 status;
    Int i;

    UTILS_COMPILETIME_ASSERT(UTILS_ARRAYSIZE(pObj->encIVASwitchSerializeObj.freeSerializerQueMem)
                             ==
                             UTILS_ARRAYSIZE(pObj->encIVASwitchSerializeObj.freeSerializerSemMem));
    status =
    Utils_queCreate(&pObj->encIVASwitchSerializeObj.freeSerializerQue,
                    UTILS_ARRAYSIZE(pObj->encIVASwitchSerializeObj.freeSerializerQueMem),
                    pObj->encIVASwitchSerializeObj.freeSerializerQueMem,
                    UTILS_QUE_FLAG_NO_BLOCK_QUE);
    UTILS_assert(status == 0);
    for (i = 0; i < UTILS_ARRAYSIZE(pObj->encIVASwitchSerializeObj.freeSerializerSemMem); i++)
    {
        ti_sysbios_knl_Semaphore_Params prms;

        ti_sysbios_knl_Semaphore_Params_init(&prms);
        ti_sysbios_knl_Semaphore_construct(&pObj->encIVASwitchSerializeObj.freeSerializerSemMem[i],
                                           0,
                                           &prms);
       status = Utils_quePut(&pObj->encIVASwitchSerializeObj.freeSerializerQue,
                             &pObj->encIVASwitchSerializeObj.freeSerializerSemMem[i],
                             ti_sysbios_BIOS_NO_WAIT);
       UTILS_assert(status == 0);
    }

    return (ENC_LINK_S_SUCCESS);
}

static Int32 EncLink_codecDeleteIvaSwitchSerializerObj(EncLink_Obj * pObj)
{
    Int32 status;
    Int i;

    UTILS_COMPILETIME_ASSERT(UTILS_ARRAYSIZE(pObj->encIVASwitchSerializeObj.freeSerializerQueMem)
                             ==
                             UTILS_ARRAYSIZE(pObj->encIVASwitchSerializeObj.freeSerializerSemMem));

    UTILS_assert(Utils_queIsFull(&pObj->encIVASwitchSerializeObj.freeSerializerQue) == TRUE);
    status = Utils_queDelete(&pObj->encIVASwitchSerializeObj.freeSerializerQue);
    UTILS_assert(status == 0);
    for (i = 0; i < UTILS_ARRAYSIZE(pObj->encIVASwitchSerializeObj.freeSerializerSemMem); i++)
    {
        ti_sysbios_knl_Semaphore_destruct(&pObj->encIVASwitchSerializeObj.freeSerializerSemMem[i]);
    }
    return (ENC_LINK_S_SUCCESS);
}


static Void EncLink_ivaMapUpdate(EncLink_Obj * pObj,SystemVideo_Ivahd2ChMap_Tbl* tbl)
{
    IArg key;
    UInt numHDVICP = HDVICP2_GetNumberOfIVAs();
    Int ivaIdx,encChIdx;
    UInt32 encCh;
    Bits32 reconfig_detect_mask[(ENC_LINK_MAX_CH/32) + 1];

    UTILS_assert(numHDVICP <= UTILS_ARRAYSIZE(tbl->ivaMap));
    UTILS_assert(pObj->inQueInfo.numCh <= ENC_LINK_MAX_CH);

    key = GateMutexPri_enter(pObj->ivaChMapMutex);
    memset(reconfig_detect_mask,0,sizeof(reconfig_detect_mask));
    for (ivaIdx = 0; ivaIdx < numHDVICP; ivaIdx++)
    {
        for (encChIdx = 0 ; encChIdx < tbl->ivaMap[ivaIdx].EncNumCh; encChIdx++)
        {
            encCh = tbl->ivaMap[ivaIdx].EncChList[encChIdx];
            if ((encCh < pObj->inQueInfo.numCh)
                &&
                ((reconfig_detect_mask[encCh/32u] & (1u << (encCh % 32))) == 0))
            {
                /* If channel number is valid and has not been previously mapped to
                 * another IVA allow reconfiguration of IVA map
                 */
                pObj->ch2ProcessTskIdNext[encCh] = ivaIdx;
                reconfig_detect_mask[encCh/32u] |= 1u << (encCh % 32);
            }
            else
            {
                if (encCh < pObj->inQueInfo.numCh)
                {
                    UTILS_warn("ENCLINK:WARNING!!!. Invalid ch2IVAMap. IVAID:%d,ChId:%d",
                               ivaIdx,encCh);
                }
            }
        }
    }
    GateMutexPri_leave(pObj->ivaChMapMutex,key);
}


static Void EncLink_ivaMapChangeCbFxn(Ptr ctx,SystemVideo_Ivahd2ChMap_Tbl* tbl)
{
   EncLink_Obj * pObj = ctx;

   UTILS_assert((pObj->linkId >= SYSTEM_LINK_ID_VENC_START)
                &&
                (pObj->linkId <= SYSTEM_LINK_ID_VENC_END));

   EncLink_ivaMapUpdate(pObj,tbl);
   Utils_tskSendCmd(&pObj->tsk,ENC_LINK_CMD_INTERNAL_IVAMAPCHANGE);
}

static
Void EncLink_codecRegisterIVAMapCb(EncLink_Obj * pObj)
{
    Utils_encdecIVAMapChangeNotifyCbInfo cbInfo;

    EncLink_codecCreateIvaMapMutex(pObj);
    EncLink_codecCreateIvaSwitchSerializerObj(pObj);
    cbInfo.ctx = pObj;
    cbInfo.fxns = &EncLink_ivaMapChangeCbFxn;
    Utils_encdecRegisterIVAMapChangeNotifyCb(&cbInfo);
}

static
Void EncLink_codecUnregisterIVAMapCb(EncLink_Obj * pObj)
{
    Utils_encdecIVAMapChangeNotifyCbInfo cbInfo;

    cbInfo.ctx = pObj;
    cbInfo.fxns = &EncLink_ivaMapChangeCbFxn;
    Utils_encdecUnRegisterIVAMapChangeNotifyCb(&cbInfo);
    EncLink_codecDeleteIvaSwitchSerializerObj(pObj);
    EncLink_codecDeleteIvaMapMutex(pObj);
}


static
Int32 EncLink_codecIVASwitchQueSerializer(EncLink_Obj * pObj,
                                          UInt32 chId,
                                          UInt32 prevIVAId,
                                          UInt32 nextIVAId,
                                          EncLink_ReqObj **pReqObjNextPtr)
{
    EncLink_ReqObj *pReqObjPrevIva;
    EncLink_ReqObj *pReqObjNextIva;
    ti_sysbios_knl_Semaphore_Struct *ivaSerializerMem;
    ti_sysbios_knl_Semaphore_Handle ivaSerializer;
    Int32 status;


    status =
    Utils_queGet(&pObj->encDummyReqObj.reqQueDummy,
                 (Ptr *) & pReqObjPrevIva, 1,
                 ti_sysbios_BIOS_NO_WAIT);
    UTILS_assert(status == 0);

    UTILS_assert(UTILS_ARRAYISVALIDENTRY(pReqObjPrevIva,pObj->encDummyReqObj.reqObjDummy));
    status =

    Utils_queGet(&pObj->encDummyReqObj.reqQueDummy,
                 (Ptr *) & pReqObjNextIva, 1,
                 ti_sysbios_BIOS_NO_WAIT);
    UTILS_assert(status == 0);
    UTILS_assert(UTILS_ARRAYISVALIDENTRY(pReqObjNextIva,pObj->encDummyReqObj.reqObjDummy));

    status = Utils_queGet(&pObj->encIVASwitchSerializeObj.freeSerializerQue,
                          (Ptr *)&ivaSerializerMem,
                          1,
                          ti_sysbios_BIOS_NO_WAIT);
    UTILS_assert(status == 0);
    ivaSerializer = ti_sysbios_knl_Semaphore_handle(ivaSerializerMem);
    pObj->chObj[chId].dummyBitBuf.channelNum = chId;

    pReqObjPrevIva->type = ENC_LINK_REQ_OBJECT_TYPE_DUMMY_IVAMAPCHANGE_PREVIVALAST;
    pReqObjPrevIva->ivaSwitchSerializer = ivaSerializer;
    pReqObjPrevIva->OutBuf = &pObj->chObj[chId].dummyBitBuf;


    pReqObjNextIva->type = ENC_LINK_REQ_OBJECT_TYPE_DUMMY_IVAMAPCHANGE_NEXTIVAFIRST;
    pReqObjNextIva->ivaSwitchSerializer = ivaSerializer;
    pReqObjNextIva->OutBuf = &pObj->chObj[chId].dummyBitBuf;


    ENCLINK_INFO_LOG(pObj->linkId,chId,"Queueing last frame in prev IVA[%d]",prevIVAId);
    status = Utils_quePut(&pObj->encProcessTsk[prevIVAId].processQue,
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
    return ENC_LINK_S_SUCCESS;
}

static
Void EncLink_codecIVASwitchQueFirstOfNextIVA(EncLink_Obj * pObj,
                                             EncLink_ReqObj *pNextIvaFirstObjs[],
                                             UInt32 newIvaId[],
                                             UInt32 numCh)
{
    Int i;
    Int32 status;
    UInt32 chId,nextIVAId;
    EncLink_ReqObj *pReqObjNextIva;

    UTILS_assert((pNextIvaFirstObjs != NULL)
                 &&
                 (newIvaId != NULL));
    for (i = 0; i < numCh; i++)
    {
        UTILS_assert((pNextIvaFirstObjs[i] != NULL)
                     &&
                     (pNextIvaFirstObjs[i]->OutBuf != NULL));
        pReqObjNextIva = pNextIvaFirstObjs[i];
        chId = pReqObjNextIva->OutBuf->channelNum;
        nextIVAId = newIvaId[i];
        ENCLINK_INFO_LOG(pObj->linkId,chId,"Queueing first frame in next IVA[%d]",nextIVAId);
        status = Utils_quePut(&pObj->encProcessTsk[nextIVAId].processQue,
                               pReqObjNextIva, ti_sysbios_BIOS_NO_WAIT);
        UTILS_assert(status == FVID2_SOK);
    }
}

Int32 EncLink_codecIVAMapChangeHandler(EncLink_Obj * pObj)
{
    Int32 status = ENC_LINK_S_SUCCESS;
    UInt32 chId;
    UInt32 newIVAID;
    IArg key;
    EncLink_ReqObj *pNextIvaFirstObjs[ENC_LINK_MAX_CH];
    UInt32 newIvaId[ENC_LINK_MAX_CH];
    UInt32 numNextIvaFirstObjs;

    key = GateMutexPri_enter(pObj->ivaChMapMutex);
    numNextIvaFirstObjs = 0;
    for (chId = 0; chId < pObj->inQueInfo.numCh; chId++)
    {
        newIVAID = pObj->ch2ProcessTskIdNext[chId];
        if (newIVAID != pObj->ch2ProcessTskId[chId])
        {
            UTILS_assert(numNextIvaFirstObjs < UTILS_ARRAYSIZE(pNextIvaFirstObjs));
            EncLink_codecIVASwitchQueSerializer(
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
    EncLink_codecIVASwitchQueFirstOfNextIVA(pObj,
                                            pNextIvaFirstObjs,
                                            newIvaId,
                                            numNextIvaFirstObjs);
    GateMutexPri_leave(pObj->ivaChMapMutex,key);
    return (status);
}


static Int32 EncLink_codecCreateReqObjDummy(EncLink_Obj * pObj)
{
    Int32 status;
    UInt32 reqId;
    struct encDummyReqObj_s *dummyReq = &pObj->encDummyReqObj;

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

    return ENC_LINK_S_SUCCESS;
}

static Int32 EncLink_codecDeleteReqObjDummy(EncLink_Obj * pObj)
{
    struct encDummyReqObj_s *dummyReq = &pObj->encDummyReqObj;
    Int32 status;

    status = Utils_queDelete(&dummyReq->reqQueDummy);

    UTILS_assert(status == 0);

    return ENC_LINK_S_SUCCESS;
}

