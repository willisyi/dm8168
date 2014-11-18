/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include "grpxLink_priv.h"
#include "grpxLink_coeffs.h"

Int32 GrpxLink_drvFvidCb(FVID2_Handle handle, Ptr appData, Ptr reserved)
{
    GrpxLink_Obj *pObj = (GrpxLink_Obj *) appData;
    UInt32 elaspedTime, curTime;

    Utils_tskSendCmd(&pObj->tsk, GRPX_LINK_CMD_DO_DEQUE);

    pObj->cbCount++;

    curTime = Utils_getCurTimeInMsec();

    if (pObj->cbCount > 10)
    {
        elaspedTime = curTime - pObj->lastCbTime;

        if (elaspedTime > pObj->maxCbTime)
            pObj->maxCbTime = elaspedTime;

        if (elaspedTime < pObj->minCbTime)
            pObj->minCbTime = elaspedTime;
    }

    pObj->lastCbTime = curTime;

    return FVID2_SOK;
}


Int32 GrpxLink_drvFvidCreate(GrpxLink_Obj * pObj)
{
    FVID2_CbParams cbParams;
    UInt32 bytesPerPixel;
    Int32 status;

    memset(&pObj->grpxFormat, 0, sizeof(pObj->grpxFormat));
    memset(&pObj->grpxCreateArgs, 0, sizeof(pObj->grpxCreateArgs));
    memset(&pObj->grpxParams, 0, sizeof(pObj->grpxParams));
    memset(&pObj->grpxRtParams, 0, sizeof(pObj->grpxRtParams));
    memset(&pObj->grpxScParams, 0, sizeof(pObj->grpxScParams));
    memset(&cbParams, 0, sizeof(cbParams));

    cbParams.cbFxn = GrpxLink_drvFvidCb;
    cbParams.appData = pObj;

    pObj->grpxCreateArgs.memType = VPS_VPDMA_MT_NONTILEDMEM;
    pObj->grpxCreateArgs.drvMode = VPS_GRPX_STREAMING_MODE;
    pObj->grpxCreateArgs.periodicCallbackEnable = TRUE;

    pObj->grpxFormat.channelNum     = 0;
    pObj->grpxFormat.width          = pObj->createArgs.dynPrm.inWidth;
    pObj->grpxFormat.height         = pObj->createArgs.dynPrm.inHeight;
    pObj->grpxFormat.fieldMerged[0] = FALSE;

    pObj->grpxFormat.scanFormat     = FVID2_SF_PROGRESSIVE;
    if(pObj->createArgs.bufferInfo.scanFormat==VGRPX_SF_INTERLACED)
        pObj->grpxFormat.scanFormat     = FVID2_SF_INTERLACED;

    if(pObj->grpxFormat.scanFormat==FVID2_SF_INTERLACED)
        pObj->grpxFormat.fieldMerged[0] = TRUE;

    switch(pObj->createArgs.bufferInfo.dataFormat)
    {
        case VGRPX_DATA_FORMAT_ARGB888:
            pObj->grpxFormat.dataFormat = FVID2_DF_ARGB32_8888;
            pObj->grpxFormat.bpp        = FVID2_BPP_BITS32;
            bytesPerPixel = 4;
            break;

        default:
        case VGRPX_DATA_FORMAT_RGB565:
            pObj->grpxFormat.dataFormat = FVID2_DF_RGB16_565;
            pObj->grpxFormat.bpp        = FVID2_BPP_BITS16;
            bytesPerPixel = 2;
            break;
    }
    pObj->grpxFormat.reserved = 0;;

    if(pObj->createArgs.bufferInfo.bufferPitch==0)
    {
        pObj->createArgs.bufferInfo.bufferPitch =
            SystemUtils_align(
                    pObj->createArgs.bufferInfo.bufferWidth*bytesPerPixel,
                    VPS_BUFFER_ALIGNMENT
                    );
    }
    pObj->grpxFormat.pitch[0] = pObj->createArgs.bufferInfo.bufferPitch;

    GrpxLink_drvSetDynamicParams(pObj, &pObj->createArgs.dynPrm);

    pObj->grpxHndl = FVID2_create(
                            FVID2_VPS_DISP_GRPX_DRV,
                            pObj->createArgs.grpxId,
                            &pObj->grpxCreateArgs,
                            &pObj->grpxCreateStatus,
                            &cbParams
                        );
    UTILS_assert(pObj->grpxHndl != NULL);

    status = FVID2_setFormat(
                    pObj->grpxHndl,
                    &pObj->grpxFormat
                );
    UTILS_assert(status==FVID2_SOK);

    status = FVID2_control(
                    pObj->grpxHndl,
                    IOCTL_VPS_SET_GRPX_PARAMS,
                    &pObj->grpxParams,
                    NULL
                );

    pObj->rtParamUpdate = FALSE;

    UTILS_assert(status==FVID2_SOK);

    return FVID2_SOK;
}

Int32 GrpxLink_drvAllocAndQueFrame(GrpxLink_Obj * pObj)
{
    FVID2_Frame *pFrame;
    UInt32 frameId;
    Int32 status;

    memset(pObj->grpxFrame, 0, sizeof(pObj->grpxFrame));
    memset(&pObj->frameList, 0, sizeof(pObj->frameList));

    pObj->bufferMemAddr = pObj->createArgs.bufferInfo.bufferPhysAddr;
    pObj->bufferMemSize = pObj->createArgs.bufferInfo.bufferPitch*pObj->createArgs.bufferInfo.bufferHeight;
    pObj->freeBufferMem = FALSE;

    if(pObj->bufferMemAddr==NULL)
    {
        pObj->bufferMemAddr = (UInt32)Utils_memAlloc(pObj->bufferMemSize, GRPX_LINK_BUFFER_ALIGNMENT);
        UTILS_assert(pObj->bufferMemAddr!=NULL);

        pObj->freeBufferMem = TRUE;

        pObj->createArgs.bufferInfo.bufferPhysAddr = pObj->bufferMemAddr;
    }

    for(frameId=0; frameId<GRPX_LINK_MAX_FRAMES; frameId++)
    {
        pFrame = &pObj->grpxFrame[frameId];

        pFrame->addr[0][0] = (Ptr)pObj->bufferMemAddr;
        pFrame->addr[1][0] = (Ptr)(pObj->bufferMemAddr + pObj->createArgs.bufferInfo.bufferPitch);

        pObj->frameList.numFrames = 1;
        pObj->frameList.frames[0] = pFrame;

        status = FVID2_queue(
                    pObj->grpxHndl,
                    &pObj->frameList,
                    0
                  );

        UTILS_assert(status==FVID2_SOK);
    }

    return status;
}

Int32 GrpxLink_drvCreate(GrpxLink_Obj * pObj,
                            VGRPX_CREATE_PARAM_S * pPrm)
{
    Int32 status = FVID2_SOK;

#ifdef SYSTEM_DEBUG_GRPX
    Vps_printf(" %d: GRPX: Create in progress !!!\n", Utils_getCurTimeInMsec());
#endif

    pObj->cbCount = 0;
    pObj->dequeCount = 0;
    pObj->totalTime = 0;
    pObj->minCbTime = 0xFF;
    pObj->maxCbTime = 0;
    pObj->lastCbTime = 0;
    pObj->maxLatency = 0;
    pObj->minLatency = 0xFF;

    memcpy(&pObj->createArgs, pPrm, sizeof(*pPrm));

    GrpxLink_drvFvidCreate(pObj);
    GrpxLink_drvAllocAndQueFrame(pObj);

#ifdef SYSTEM_DEBUG_GRPX
    Vps_printf(" %d: GRPX: Create Done !!!\n", Utils_getCurTimeInMsec());
#endif

    return status;
}

Int32 GrpxLink_drvSetScCoeffs(GrpxLink_Obj * pObj, Grpx_ScScaleSet horScaleSet, Grpx_ScScaleSet verScaleSet)
{
    UInt16 *hor_coeff;
    UInt16 *ver_coeff;

    if (GRPX_SC_US_SET == horScaleSet)
        hor_coeff = &GrpxScHorzUpScaleCoeff[0][0];
    else if (GRPX_SC_AF == horScaleSet)
        hor_coeff = &GrpxScHorzAFCoeff[0][0];
    else
        hor_coeff = &GrpxScHorzDownScaleCoeff[horScaleSet][0][0];

    if (GRPX_SC_US_SET == verScaleSet)
        ver_coeff = &GrpxScVertUpScaleCoeff[0][0];
    else if (GRPX_SC_AF == verScaleSet)
        ver_coeff = &GrpxScVertAFCoeff[0][0];
    else
        ver_coeff = &GrpxScVertDownScaleCoeff[verScaleSet][0][0];


#ifdef SYSTEM_DEBUG_GRPX
    Vps_printf(" %d: GRPX: Applying SC co-eff set, horzSet = [%d], vertSet = [%d] !!!\n",
               Utils_getCurTimeInMsec(),
               horScaleSet,
               verScaleSet
            );
#endif

    memcpy(pObj->grpxScCoeff.hsCoeff, hor_coeff, sizeof(pObj->grpxScCoeff.hsCoeff));
    memcpy(pObj->grpxScCoeff.vsCoeff, ver_coeff, sizeof(pObj->grpxScCoeff.vsCoeff));

    return FVID2_SOK;
}

Int32 GrpxLink_drvSetDynamicParams(GrpxLink_Obj * pObj, VGRPX_DYNAMIC_PARAM_S *pPrm)
{
    Grpx_ScScaleSet horScaleSet;
    Grpx_ScScaleSet verScaleSet;

    UInt32 cookie;

    /*
     * disable interrupts
     */
    cookie = Hwi_disable();

    memcpy(&pObj->createArgs.dynPrm, pPrm, sizeof(*pPrm));

    pObj->grpxParams.numRegions     = 1;
    pObj->grpxParams.gParams        = &pObj->grpxRtParams;
    pObj->grpxParams.scParams       = &pObj->grpxScParams;
    pObj->grpxParams.clutPtr        = NULL;

    pObj->grpxRtParams.regId        = 0;
    pObj->grpxRtParams.format       = pObj->grpxFormat.dataFormat;
    pObj->grpxRtParams.pitch[0]     = pObj->grpxFormat.pitch[0];
    pObj->grpxRtParams.rotation     = VPS_MEM_0_ROTATION;
    pObj->grpxRtParams.scParams     = &pObj->grpxScParams;
    pObj->grpxRtParams.stenPtr      = NULL;
    pObj->grpxRtParams.stenPitch    = 0;

    pObj->grpxRtParams.regParams.regionWidth        = pObj->createArgs.dynPrm.inWidth;
    pObj->grpxRtParams.regParams.regionHeight       = pObj->createArgs.dynPrm.inHeight;
    pObj->grpxRtParams.regParams.regionPosX         = pObj->createArgs.dynPrm.displayStartX;
    pObj->grpxRtParams.regParams.regionPosY         = pObj->createArgs.dynPrm.displayStartY;
    pObj->grpxRtParams.regParams.dispPriority       = 0;
    pObj->grpxRtParams.regParams.firstRegion        = 1;
    pObj->grpxRtParams.regParams.lastRegion         = 1;
    pObj->grpxRtParams.regParams.scEnable           = pObj->createArgs.dynPrm.scaleEnable;
    pObj->grpxRtParams.regParams.stencilingEnable   = FALSE;
    pObj->grpxRtParams.regParams.bbEnable           = FALSE;
    pObj->grpxRtParams.regParams.bbAlpha            = 0;
    pObj->grpxRtParams.regParams.blendAlpha         = 40;
    pObj->grpxRtParams.regParams.blendType          = VPS_GRPX_BLEND_NO;
    pObj->grpxRtParams.regParams.transEnable        = pObj->createArgs.dynPrm.transperencyEnable;
    pObj->grpxRtParams.regParams.transType          = VPS_GRPX_TRANS_NO_MASK;
    pObj->grpxRtParams.regParams.transColorRgb24    = pObj->createArgs.dynPrm.transperencyColor;

    pObj->grpxScParams.inWidth          = pObj->createArgs.dynPrm.inWidth;
    pObj->grpxScParams.inHeight         = pObj->createArgs.dynPrm.inHeight;
    pObj->grpxScParams.outWidth         = pObj->createArgs.dynPrm.displayWidth - GRPX_SC_MARGIN_OFFSET;
    pObj->grpxScParams.outHeight        = pObj->createArgs.dynPrm.displayHeight - GRPX_SC_MARGIN_OFFSET;
    pObj->grpxScParams.horFineOffset    = 0;
    pObj->grpxScParams.verFineOffset    = 0;
    pObj->grpxScParams.scCoeff          = &pObj->grpxScCoeff;

    pObj->rtParamUpdate                 = TRUE;

    if(pObj->grpxScParams.inWidth < pObj->grpxScParams.outWidth)
        horScaleSet = GRPX_SC_US_SET;
    else
    if(pObj->grpxScParams.inWidth > pObj->grpxScParams.outWidth)
        horScaleSet = GRPX_SC_DS_SET_0;
    else
        horScaleSet = GRPX_SC_AF;

    if(pObj->grpxScParams.inHeight < pObj->grpxScParams.outHeight)
        verScaleSet = GRPX_SC_US_SET;
    else
    if(pObj->grpxScParams.inHeight > pObj->grpxScParams.outHeight)
        verScaleSet = GRPX_SC_DS_SET_0;
    else
        verScaleSet = GRPX_SC_AF;

    GrpxLink_drvSetScCoeffs(pObj, horScaleSet, verScaleSet);

    /*
     * restore interrupts
     */
    Hwi_restore(cookie);

    return FVID2_SOK;
}

Int32 GrpxLink_drvGetBufferInfo(GrpxLink_Obj * pObj, VGRPX_BUFFER_INFO_S *pPrm)
{
    memcpy(pPrm, &pObj->createArgs.bufferInfo, sizeof(*pPrm));

    return FVID2_SOK;
}

Int32 GrpxLink_drvProcessFrames(GrpxLink_Obj * pObj)
{
    FVID2_Frame *pFrame;
    Int32 status;

    do
    {
        // dequeue queued buffer's
        status = FVID2_dequeue(pObj->grpxHndl, &pObj->frameList, 0, BIOS_NO_WAIT);

        if(status!=FVID2_SOK)
            break;

        pObj->dequeCount++;

        pFrame = pObj->frameList.frames[0];

        pFrame->perFrameCfg = NULL;

        pObj->frameList.perListCfg = NULL;

        if(pObj->rtParamUpdate)
        {
            pObj->rtParamUpdate = FALSE;

            pFrame->perFrameCfg = &pObj->grpxRtParams;
            pObj->frameList.perListCfg = &pObj->grpxParams;
        }

        // queue back the same buffer
        status = FVID2_queue(pObj->grpxHndl, &pObj->frameList, BIOS_NO_WAIT);

    } while (1);

    return FVID2_SOK;
}

Int32 GrpxLink_drvPrintRtStatus(GrpxLink_Obj * pObj, UInt32 elaspedTime)
{
    char *grpxName[] = { "HDDAC(BP0) ", "DVO2(BP1)  ", "SDDAC(SEC1)" };

    Vps_rprintf(" %d: GRPX: %s: %d fps, Latency (Min / Max) = ( %d / %d ), Callback Interval (Min / Max) = ( %d / %d ) !!! \r\n",
         Utils_getCurTimeInMsec(),
         grpxName[pObj->createArgs.grpxId],
        pObj->cbCount*1000/elaspedTime,
        pObj->minLatency,
        pObj->maxLatency,
        pObj->minCbTime,
        pObj->maxCbTime
        );

#if 1
    /* reset max time */
    pObj->maxCbTime = 0;
    pObj->minCbTime = 0xFF;
    pObj->maxLatency = 0;
    pObj->minLatency = 0xFF;
#endif

    return 0;
}

Int32 GrpxLink_drvPrintStatistics(GrpxLink_Obj * pObj)
{
    UInt32 elaspedTime;

    elaspedTime = Utils_getCurTimeInMsec() - pObj->startTime;

    GrpxLink_drvPrintRtStatus(pObj, elaspedTime);

    return 0;
}

Int32 GrpxLink_drvDelete(GrpxLink_Obj * pObj)
{
    Int32 status;

#ifdef SYSTEM_DEBUG_GRPX
    Vps_printf(" %d: GRPX: Frames = %d (fps = %d) !!!\n",
               Utils_getCurTimeInMsec(),
               pObj->dequeCount,
               pObj->dequeCount * 100 / (pObj->totalTime / 10));
#endif

#ifdef SYSTEM_DEBUG_GRPX
    Vps_printf(" %d: GRPX: Delete in progress !!!\n", Utils_getCurTimeInMsec());
#endif

    do
    {
        // dequeue queued buffer's
        status = FVID2_dequeue(pObj->grpxHndl, &pObj->frameList, 0, BIOS_NO_WAIT);
    } while (status == FVID2_SOK);

    status = FVID2_delete(pObj->grpxHndl, NULL);
    UTILS_assert(status==FVID2_SOK);

    if(pObj->freeBufferMem && pObj->bufferMemAddr)
    {
        Utils_memFree((Ptr)pObj->bufferMemAddr, pObj->bufferMemSize);

        pObj->freeBufferMem = FALSE;
    }

#ifdef SYSTEM_DEBUG_GRPX
    Vps_printf(" %d: GRPX: Delete Done !!!\n", Utils_getCurTimeInMsec());
#endif

    return FVID2_SOK;
}

Int32 GrpxLink_drvStart(GrpxLink_Obj * pObj)
{
    Int32 status = FVID2_SOK;

#ifdef SYSTEM_DEBUG_GRPX
    Vps_printf(" %d: GRPX: Start in progress !!!\n", Utils_getCurTimeInMsec());
#endif

    pObj->lastCbTime = Utils_getCurTimeInMsec();
    pObj->startTime = Utils_getCurTimeInMsec();
    pObj->prevTime = pObj->startTime;

    status = FVID2_start(pObj->grpxHndl, NULL);
    UTILS_assert(status == FVID2_SOK);

    pObj->totalTime = Utils_getCurTimeInMsec();

#ifdef SYSTEM_DEBUG_GRPX
    Vps_printf(" %d: GRPX: Start Done !!!\n", Utils_getCurTimeInMsec());
#endif

    return status;
}

Int32 GrpxLink_drvStop(GrpxLink_Obj * pObj)
{
    Int32 status = FVID2_SOK;

#ifdef SYSTEM_DEBUG_GRPX
    Vps_printf(" %d: GRPX: Stop in progress !!!\n", Utils_getCurTimeInMsec());
#endif

    pObj->totalTime = Utils_getCurTimeInMsec() - pObj->totalTime;

    status = FVID2_stop(pObj->grpxHndl, NULL);
    UTILS_assert(status == FVID2_SOK);

#ifdef SYSTEM_DEBUG_GRPX
    Vps_printf(" %d: GRPX: Stop Done !!!\n", Utils_getCurTimeInMsec());
#endif

    return status;
}

