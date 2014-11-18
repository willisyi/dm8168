/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include "osdLink_priv.h"


Bool AlgLink_osdAlgIsValidCh(AlgLink_OsdObj * pObj, UInt32 chId)
{
    if(
        chId >= SWOSD_MAX_CHANNELS
            ||
        chId >= ALG_LINK_OSD_MAX_CH
            ||
        chId >= pObj->numValidOsdCh
      )
    {
        return FALSE;
    }

    return TRUE;
}

Int32 AlgLink_osdAlgResetStatistics(AlgLink_OsdObj * pObj)
{
    AlgLink_OsdChObj *pChObj;
    Int32 chId, i;

    for(chId=0; chId<ALG_LINK_OSD_MAX_CH; chId++)
    {
        pChObj = &pObj->chObj[chId];

        pChObj->inFrameRecvCount = 0;
        pChObj->inFrameQueCount = 0;
        pChObj->inFrameRejectCount = 0;
        pChObj->inFrameProcessCount = 0;
    }

    pObj->statsStartTime = Utils_getCurTimeInMsec();

    pObj->processFrameCount = 0;
    pObj->totalTime = 0;

    for(i=0; i<SWOSD_MAX_PROFILE_LOG; i++)
        pObj->profileLog[i] = 0;

    return 0;
}

Int32 AlgLink_osdAlgPrintStatistics(AlgLink_OsdObj * pObj, Bool resetAfterPrint)
{
    UInt32 chId;
    AlgLink_OsdChObj *pChObj;
    UInt32 elaspedTime;
    UInt64 referenceTimeInUsecs;

    elaspedTime = Utils_getCurTimeInMsec() - pObj->statsStartTime; // in msecs
    elaspedTime /= 1000; // convert to secs

    referenceTimeInUsecs = (Utils_getCurTimeInMsec() - pObj->statsStartTime)*1000;

    Vps_printf( " \n"
            " *** (SIMCOP) OSD Statistics *** \n"
            " \n"
            " Elasped Time           : %d secs\n"
            " Total Fields Processed : %d \n"
            " Total Fields FPS       : %d FPS\n"
            " \n"
            " Detailed Internal Profile Log, \n"
            " - OSD ALG Total time   : %10d msecs (%d %% of elasped time) \n"
            " - GATHER  Total time   : %10d msecs (%d %% of elasped time) \n"
            " - BLEND   Total time   : %10d msecs (%d %% of elasped time) \n"
            " - SCATTER Total time   : %10d msecs (%d %% of elasped time) \n"
            " - EDMA    Total time   : %10d msecs (%d %% of elasped time) \n"
            " - SIMCOP  Total time   : %10d msecs (%d %% of elasped time) \n"
            " - SCAT/GATH CPU time   : %10d msecs (%d %% of elasped time) \n"
            " - BLEND     CPU time   : %10d msecs (%d %% of elasped time) \n"
            " \n"
            " \n"
            " CH  | In Recv In Que In Reject In Process \n"
            " Num | FPS     FPS    FPS       FPS        \n"
            " ------------------------------------------\n",
            elaspedTime,
            pObj->processFrameCount,
            pObj->processFrameCount * 100 / (pObj->totalTime / 10),
            (UInt32)(pObj->profileLog[SWOSD_PROFILE_TOTAL_TIME]/1000), (UInt32)(pObj->profileLog[SWOSD_PROFILE_TOTAL_TIME]*100/referenceTimeInUsecs),
            (UInt32)(pObj->profileLog[SWOSD_PROFILE_GATHER_TOTAL_TIME]/1000), (UInt32)(pObj->profileLog[SWOSD_PROFILE_GATHER_TOTAL_TIME]*100/referenceTimeInUsecs),
            (UInt32)(pObj->profileLog[SWOSD_PROFILE_BLEND_TOTAL_TIME]/1000), (UInt32)(pObj->profileLog[SWOSD_PROFILE_BLEND_TOTAL_TIME]*100/referenceTimeInUsecs),
            (UInt32)(pObj->profileLog[SWOSD_PROFILE_SCATTER_TOTAL_TIME]/1000), (UInt32)(pObj->profileLog[SWOSD_PROFILE_SCATTER_TOTAL_TIME]*100/referenceTimeInUsecs),
            (UInt32)(pObj->profileLog[SWOSD_PROFILE_EDMA_TOTAL_TIME]/1000), (UInt32)(pObj->profileLog[SWOSD_PROFILE_EDMA_TOTAL_TIME]*100/referenceTimeInUsecs),
            (UInt32)(pObj->profileLog[SWOSD_PROFILE_SIMCOP_TOTAL_TIME]/1000), (UInt32)(pObj->profileLog[SWOSD_PROFILE_SIMCOP_TOTAL_TIME]*100/referenceTimeInUsecs),
            (UInt32)(pObj->profileLog[SWOSD_PROFILE_SCATGATH_CPU_TIME]/1000), (UInt32)(pObj->profileLog[SWOSD_PROFILE_SCATGATH_CPU_TIME]*100/referenceTimeInUsecs),
            (UInt32)(pObj->profileLog[SWOSD_PROFILE_BLEND_CPU_TIME]/1000), (UInt32)(pObj->profileLog[SWOSD_PROFILE_BLEND_CPU_TIME]*100/referenceTimeInUsecs)
          );

    for (chId = 0; chId < pObj->numValidOsdCh; chId++)
    {
        pChObj = &pObj->chObj[chId];

        Vps_printf( " %3d | %7d %6d %9d %10d\n",
            chId,
            pChObj->inFrameRecvCount/elaspedTime,
            pChObj->inFrameQueCount/elaspedTime,
            pChObj->inFrameRejectCount/elaspedTime,
            pChObj->inFrameProcessCount/elaspedTime
            );
    }

    Vps_printf( " \n");

    if(resetAfterPrint)
    {
        AlgLink_osdAlgResetStatistics(pObj);
    }
    return FVID2_SOK;
}


Int32 AlgLink_osdAlgAllocMem(AlgLink_OsdObj * pObj)
{
    UInt32 blockId;
    Int32 status;

    status = SWOSD_getMemAllocInfo(&pObj->algObj, &pObj->algMemAllocPrm);
    UTILS_assert(status==FVID2_SOK);

    for(blockId=0; blockId<pObj->algMemAllocPrm.numMemBlocks; blockId++)
    {
        #ifdef SYSTEM_VERBOSE_PRINTS
        Vps_printf(" %d: OSD: MEM REQUEST %d: of size %d B (align=%d)\n",
            Utils_getCurTimeInMsec(),
            blockId,
            pObj->algMemAllocPrm.memBlockSize[blockId],
            pObj->algMemAllocPrm.memBlockAlign[blockId]
            );
        #endif

        pObj->algMemAllocPrm.memBlockAddr[blockId] =
                Utils_memAlloc(
                    pObj->algMemAllocPrm.memBlockSize[blockId],
                    pObj->algMemAllocPrm.memBlockAlign[blockId]
                );
        UTILS_assert(pObj->algMemAllocPrm.memBlockAddr[blockId]!=NULL);

        #ifdef SYSTEM_VERBOSE_PRINTS
        Vps_printf(" %d: OSD: MEM ALLOC %d: @ 0x%08x of size %d B (align=%d)\n",
            Utils_getCurTimeInMsec(),
            blockId,
            pObj->algMemAllocPrm.memBlockAddr[blockId],
            pObj->algMemAllocPrm.memBlockSize[blockId],
            pObj->algMemAllocPrm.memBlockAlign[blockId]
            );
        #endif
    }

    status = SWOSD_setMemAllocInfo(&pObj->algObj, &pObj->algMemAllocPrm);
    UTILS_assert(status==FVID2_SOK);

    return FVID2_SOK;
}

Int32 AlgLink_osdAlgBlindWinCreate(AlgLink_OsdObj * pObj)
{
    Int32 status;
    UInt32 channelId;

    for(channelId = 0;channelId<pObj->numValidOsdCh;channelId++)
    {
        pObj->chObj[channelId].numBlindDmaWin = 0;

    }

    // create DMA channel for blind area
    status = Utils_dmaCreateCh(&pObj->dmaObj,
                                   UTILS_DMA_DEFAULT_EVENT_Q,
                                   ALG_LINK_OSD_MAX_WINDOWS, FALSE);
    UTILS_assert(status==FVID2_SOK);

    return status;
}

Int32 AlgLink_osdAlgBlindWinDelete(AlgLink_OsdObj * pObj)
{
    Int32 status;

    status = Utils_dmaDeleteCh(&pObj->dmaObj);
    UTILS_assert(status==FVID2_SOK);

    return status;
}

Int32 AlgLink_osdAlgSetChOsdBlindWinPrm(AlgLink_OsdObj * pObj,
                            AlgLink_OsdChBlindWinParams * params)
{
    Utils_DmaFill2D *pChDmaPrm;
    System_LinkChInfo *pChInfo;
    UInt32 winId, dmaWinId;
    AlgLink_OsdBlindWindowPrm *pWinPrm;

    if( ! AlgLink_osdAlgIsValidCh(pObj, params->chId) )
        return FVID2_EFAIL;

    pChInfo        = &pObj->inQueInfo->chInfo[params->chId];

    if(params->numWindows>ALG_LINK_OSD_MAX_WINDOWS)
        params->numWindows = ALG_LINK_OSD_MAX_WINDOWS;

    dmaWinId = 0;

    for(winId=0; winId<params->numWindows; winId++)
    {
        pWinPrm        = &params->winPrm[winId];

        if(pWinPrm->enableWin)
        {
            pChDmaPrm      = &pObj->chObj[params->chId].blindWinDmaPrm[dmaWinId];

            pChDmaPrm->destAddr[0]   = NULL;
            pChDmaPrm->destAddr[1]   = NULL;
            pChDmaPrm->destPitch[0]  = pChInfo->pitch[0];
            pChDmaPrm->destPitch[1]  = pChInfo->pitch[1];
            pChDmaPrm->dataFormat    = pChInfo->dataFormat;
            pChDmaPrm->startX        = SystemUtils_floor(pWinPrm->startX, 2);
            pChDmaPrm->startY        = SystemUtils_floor(pWinPrm->startY, 2);
            pChDmaPrm->width         = SystemUtils_floor(pWinPrm->width, 2);
            pChDmaPrm->height        = SystemUtils_floor(pWinPrm->height, 2);
            pChDmaPrm->fillColorYUYV = pWinPrm->fillColorYUYV;

            if(pChDmaPrm->startX > pChInfo->width)
                continue;

            if(pChDmaPrm->startY > pChInfo->height)
                continue;

            if(pChDmaPrm->startX + pChDmaPrm->width > pChInfo->width)
                pChDmaPrm->width = pChInfo->width - pChDmaPrm->startX;

            if(pChDmaPrm->startY + pChDmaPrm->height > pChInfo->height)
                pChDmaPrm->height = pChInfo->height - pChDmaPrm->startY;

            if(pChDmaPrm->width < ALG_LINK_OSD_MIN_WIN_WIDTH)
                continue;

            if(pChDmaPrm->height < ALG_LINK_OSD_MIN_WIN_HEIGHT)
                continue;

            dmaWinId++;
        }
    }

    pObj->chObj[params->chId].numBlindDmaWin = dmaWinId;

    return FVID2_SOK;
}

Int32 AlgLink_osdAlgBlindWinProcessFrames(AlgLink_OsdObj * pObj, FVID2_FrameList *pFrameList)
{
    FVID2_Frame *pFrame;
    AlgLink_OsdChObj *pChObj;
    Utils_DmaFill2D  *pChDmaPrm;
    System_LinkChInfo *pChInfo;
    UInt32 frameId, winId;
    Int32 status = FVID2_SOK;

    for(frameId=0; frameId<pFrameList->numFrames; frameId++)
    {
        pFrame = pFrameList->frames[frameId];

        if( ! AlgLink_osdAlgIsValidCh(pObj, pFrame->channelNum) )
            continue;

        pChObj  = &pObj->chObj[pFrame->channelNum];
        pChInfo = &pObj->inQueInfo->chInfo[pFrame->channelNum];

        if(pChObj->numBlindDmaWin)
        {
            pChDmaPrm = &pChObj->blindWinDmaPrm[0];

            pChDmaPrm->destAddr[0] = pFrame->addr[0][0];
            pChDmaPrm->destAddr[1] = pFrame->addr[0][1];

            if(pChInfo->memType==SYSTEM_MT_TILEDMEM)
            {
                pChDmaPrm->destAddr[0]    = (Ptr)Utils_tilerAddr2CpuAddr( (UInt32)pChDmaPrm->destAddr[0] );
                pChDmaPrm->destAddr[1]    = (Ptr)Utils_tilerAddr2CpuAddr( (UInt32)pChDmaPrm->destAddr[1] );
            }

            for(winId=1; winId<pChObj->numBlindDmaWin; winId++)
            {
                pChDmaPrm = &pChObj->blindWinDmaPrm[winId];

                pChDmaPrm->destAddr[0] = pChObj->blindWinDmaPrm[0].destAddr[0];
                pChDmaPrm->destAddr[1] = pChObj->blindWinDmaPrm[0].destAddr[1];
            }

            #if 1
            status = Utils_dmaFill2D(&pObj->dmaObj, pChObj->blindWinDmaPrm, pChObj->numBlindDmaWin);
            #endif
            UTILS_assert(status == FVID2_SOK);
        }
    }

    return FVID2_SOK;
}

Int32 AlgLink_osdAlgCreate(AlgLink_OsdObj * pObj)
{
	Int32 status, chId;
    AlgLink_OsdChObj *pChObj;
    AlgLink_OsdChWinParams *pChWinPrm;
    SWOSD_CreatePrm *pAlgCreatePrm;
    SWOSD_StaticPrm *pAlgChStaticPrm;
    System_LinkChInfo *pChInfo;

    pObj->numValidOsdCh = pObj->inQueInfo->numCh;

    if(pObj->numValidOsdCh > SWOSD_MAX_CHANNELS)
        pObj->numValidOsdCh = SWOSD_MAX_CHANNELS;

    if(pObj->numValidOsdCh > ALG_LINK_OSD_MAX_VALID_CH)
        pObj->numValidOsdCh = ALG_LINK_OSD_MAX_VALID_CH;

    pAlgCreatePrm = &pObj->algCreatePrm;

    memset(pAlgCreatePrm, 0, sizeof(*pAlgCreatePrm));

    pAlgCreatePrm->numChannels = pObj->numValidOsdCh;

    pAlgCreatePrm->useGlobalAlpha = TRUE;

    pAlgCreatePrm->maxFramePerBlendFrame = pAlgCreatePrm->numChannels;

    /* assuming equal number of primary, secondary, MJPEG channels,
        set pAlgCreatePrm->maxFramePerBlendFrame = number of primary streams
        i.e                                      = pAlgCreatePrm->numChannels/3
    */
    pAlgCreatePrm->maxFramePerBlendFrame /= 3;

    if(pAlgCreatePrm->maxFramePerBlendFrame > SWOSD_MAX_FRAMES_PER_BLEND_FRAME)
        pAlgCreatePrm->maxFramePerBlendFrame = SWOSD_MAX_FRAMES_PER_BLEND_FRAME;

    pAlgCreatePrm->maxWindowsPerCh  = SWOSD_MAX_WINDOWS;

    UTILS_assert(pAlgCreatePrm->numChannels <= SWOSD_MAX_CHANNELS);

    AlgLink_osdAlgResetStatistics(pObj);

    pObj->processFrameCount  = 0;
    pObj->totalTime  = 0;

    for(chId=0; chId<pAlgCreatePrm->numChannels; chId++)
    {
        pChObj = &pObj->chObj[chId];

        pChInfo = &pObj->inQueInfo->chInfo[chId];

        pChWinPrm = &pObj->osdChCreateParams[chId].chDefaultParams;

        pChWinPrm->chId = chId;

        status = Utils_queCreate(
                    &pChObj->inQue,
                    ALG_LINK_OSD_MAX_FRAMES_PER_CH,
                    pChObj->inQueMem,
                    UTILS_QUE_FLAG_NO_BLOCK_QUE
                    );
        UTILS_assert(status==FVID2_SOK);

        status = AlgLink_osdAlgSetChOsdWinPrm(pObj, pChWinPrm, FALSE);

        UTILS_assert(status==FVID2_SOK);

        if(chId==0)
        {
            /* color key setup and transperency based on CH0 color key */
            pAlgCreatePrm->colorKey[0] = pChWinPrm->colorKey[0];
            pAlgCreatePrm->colorKey[1] = pChWinPrm->colorKey[1];
            pAlgCreatePrm->transparencyEnable = pChWinPrm->winPrm[0].transperencyEnable;
            pAlgCreatePrm->globalAlphaValue = AlgLink_osdAlgScaleGlobalAlpha(pChWinPrm->winPrm[0].globalAlpha);
        }

        pAlgCreatePrm->chDynamicPrm[chId] = pChObj->algDynamicPrm;

        pAlgChStaticPrm = &pAlgCreatePrm->chStaticPrm[chId];

        pAlgChStaticPrm->maxWidth = pObj->osdChCreateParams[chId].maxWidth;
        pAlgChStaticPrm->maxHeight = pObj->osdChCreateParams[chId].maxHeight;

        pAlgChStaticPrm->dataFormat = SWOSD_FORMAT_YUV420SP_UV;
        if(pChInfo->dataFormat==SYSTEM_DF_YUV422I_YUYV)
            pAlgChStaticPrm->dataFormat = SWOSD_FORMAT_YUV422I_YUYV;
        if(pChInfo->dataFormat==SYSTEM_DF_YUV422I_UYVY)
            pAlgChStaticPrm->dataFormat = SWOSD_FORMAT_YUV422I_UYVY;

        pAlgChStaticPrm->isTiledMem = FALSE;
        if(pChInfo->memType==SYSTEM_MT_TILEDMEM)
            pAlgChStaticPrm->isTiledMem = TRUE;

        pAlgChStaticPrm->isInterlaced = FALSE;
        if(pChInfo->scanFormat==SYSTEM_SF_INTERLACED)
            pAlgChStaticPrm->isInterlaced = TRUE;

        pAlgChStaticPrm->videoLineOffset[0] = pChInfo->pitch[0];
        pAlgChStaticPrm->videoLineOffset[1] = pChInfo->pitch[1];
    }

    Vps_printf(" %d: OSD: Opening algorithm ... !!!\n",
        Utils_getCurTimeInMsec()
        );

    status = SWOSD_open(&pObj->algObj, pAlgCreatePrm);

    UTILS_assert(status==FVID2_SOK);

    Vps_printf(" %d: OSD: Opening algorithm ... DONE !!!\n",
        Utils_getCurTimeInMsec()
        );

    AlgLink_osdAlgAllocMem(pObj);

    AlgLink_osdAlgBlindWinCreate(pObj);

	return FVID2_SOK;
}

Int32 AlgLink_osdAlgFreeMem(AlgLink_OsdObj * pObj)
{
    UInt32 blockId;
    Int32 status;

    for(blockId=0; blockId<pObj->algMemAllocPrm.numMemBlocks; blockId++)
    {
        status = Utils_memFree(
                pObj->algMemAllocPrm.memBlockAddr[blockId],
                pObj->algMemAllocPrm.memBlockSize[blockId]
                );
        UTILS_assert(status==FVID2_SOK);
    }

    return FVID2_SOK;
}

Int32 AlgLink_osdAlgDelete(AlgLink_OsdObj * pObj)
{
    Int32 status;
    UInt32 chId;
    AlgLink_OsdChObj *pChObj;

    AlgLink_osdAlgBlindWinDelete(pObj);

    for(chId=0; chId<pObj->numValidOsdCh; chId++)
    {
        pChObj = &pObj->chObj[chId];

        status = Utils_queDelete(&pChObj->inQue);
        UTILS_assert(status==FVID2_SOK);
    }

    AlgLink_osdAlgFreeMem(pObj);

    status = SWOSD_close(&pObj->algObj);
    UTILS_assert(status==FVID2_SOK);

	return FVID2_SOK;
}

Int32 AlgLink_osdAlgQueFramesToInQue(AlgLink_OsdObj * pObj, FVID2_FrameList *pFrameList)
{
    UInt32 frameId;
    Int32 status;
    FVID2_Frame *pFrame;
    AlgLink_OsdChObj *pChObj;

    for(frameId=0; frameId<pFrameList->numFrames; frameId++)
    {
        pFrame = pFrameList->frames[frameId];

        if(pFrame==NULL)
            continue;

        if(!AlgLink_osdAlgIsValidCh(pObj, pFrame->channelNum))
        {
            continue;
        }

        pChObj = &pObj->chObj[pFrame->channelNum];

        #if 0
        {
            System_LinkChInfo *pChInfo;

            pChInfo = &pObj->inQueInfo->chInfo[pFrame->channelNum];

            /* seeing some artifacts with QCIF OSD is enabled so skipping those CHs right now */
            if(pChInfo->height <= 120 || pChInfo->width <= 176  )
                continue;
        }
        #endif

        pChObj->inFrameRecvCount++;

        status = Utils_quePut(&pChObj->inQue, pFrame, BIOS_NO_WAIT);

        if(status!=FVID2_SOK)
        {
            pChObj->inFrameRejectCount++;

            /* if no free space in queue, then drop the frame, normally this should not happen */
            Vps_rprintf(" %d: OSD: WARNING: Input que full for CH%d !!!\n",
                Utils_getCurTimeInMsec(),
                pFrame->channelNum
                );

            continue;
        }

        pChObj->inFrameQueCount++;
    }

    return FVID2_SOK;
}

Int32 AlgLink_osdAlgMakeBlendFramePrm(AlgLink_OsdObj * pObj)
{
    SWOSD_BlendFramePrm *pAlgBlendFramePrm;
    UInt32 chId;
    Int32 status;
    FVID2_Frame *pFrame;
    AlgLink_OsdChObj *pChObj;
    SWOSD_Frame *pAlgFrame;

    pAlgBlendFramePrm = &pObj->algBlendFramePrm;

    pAlgBlendFramePrm->numFrames = 0;

    for(chId=0; chId<pObj->numValidOsdCh; chId++)
    {
        pChObj = &pObj->chObj[chId];

        status = Utils_queGet(&pChObj->inQue, (Ptr*)&pFrame, 1, BIOS_NO_WAIT);

        if(status==FVID2_SOK && pFrame)
        {
            pChObj->inFrameProcessCount++;

            pAlgFrame = &pAlgBlendFramePrm->frames[pAlgBlendFramePrm->numFrames];

            pAlgFrame->channelNum = pFrame->channelNum;
            pAlgFrame->fid        = pFrame->fid;
            if(pObj->algCreatePrm.chStaticPrm[pFrame->channelNum].isTiledMem)
            {
                pAlgFrame->addr[0]    = (Ptr)Utils_tilerAddr2CpuAddr( (UInt32)pFrame->addr[0][0] );
                pAlgFrame->addr[1]    = (Ptr)Utils_tilerAddr2CpuAddr( (UInt32)pFrame->addr[0][1] );
            }
            else
            {
                pAlgFrame->addr[0]    = pFrame->addr[0][0];
                pAlgFrame->addr[1]    = pFrame->addr[0][1];
            }

            pAlgBlendFramePrm->numFrames++;
        }

        if(pAlgBlendFramePrm->numFrames>=pObj->algCreatePrm.maxFramePerBlendFrame)
            break;
    }

    return FVID2_SOK;
}

Int32 AlgLink_osdAlgProcessFrames(Utils_TskHndl *pTsk, AlgLink_OsdObj * pObj, FVID2_FrameList *pFrameList)
{
    Int32 status = FVID2_SOK;

    if(pObj->processFrameCount==0)
    {
        AlgLink_osdAlgResetStatistics(pObj);
    }

    AlgLink_osdAlgQueFramesToInQue(pObj, pFrameList);

    AlgLink_osdAlgBlindWinProcessFrames(pObj, pFrameList);

    do {
        AlgLink_osdAlgMakeBlendFramePrm(pObj);

        if(pObj->algBlendFramePrm.numFrames)
        {
            UInt32 curTime;

            curTime = Utils_getCurTimeInMsec();

            pObj->processFrameCount += pObj->algBlendFramePrm.numFrames;

            #if 1
            status = SWOSD_blendFrames(
                            &pObj->algObj,
                            &pObj->algBlendFramePrm,
                            0
                        );
            if(status!=FVID2_SOK)
            {
                Vps_rprintf(" %d: OSD: ERROR during SWOSD_blendFrames() !!!\n",
                    Utils_getCurTimeInMsec()
                    );
            }

            {
                UInt32 i;

                for(i=0; i<SWOSD_MAX_PROFILE_LOG; i++)
                    pObj->profileLog[i] += pObj->algBlendFramePrm.profileLog[i];
            }

            #endif

            pObj->totalTime += Utils_getCurTimeInMsec() - curTime;
        }

    } while(pObj->algBlendFramePrm.numFrames);

    return status;
}


Int32 AlgLink_osdAlgSetChOsdWinPrm(AlgLink_OsdObj * pObj,
                            AlgLink_OsdChWinParams * params,
                            Bool callAlgApi)
{
	Int32 status = FVID2_SOK, winId, algWinId;

    AlgLink_OsdChObj *pChObj;
    AlgLink_OsdChCreateParams *pChCreatePrm;
    AlgLink_OsdWindowPrm    *pWinPrm;
    System_LinkChInfo *pChInfo;

    SWOSD_DynamicPrm *pChAlgDynPrm;
    SWOSD_WindowPrm *pAlgWinPrm;



    if( ! AlgLink_osdAlgIsValidCh(pObj, params->chId) )
        return FVID2_EFAIL;

    pChObj = &pObj->chObj[params->chId];

    pChInfo = &pObj->inQueInfo->chInfo[params->chId];

    pChCreatePrm = &pObj->osdChCreateParams[params->chId];

    pChAlgDynPrm = &pChObj->algDynamicPrm;

    UTILS_assert(SWOSD_MAX_WINDOWS<=ALG_LINK_OSD_MAX_WINDOWS);

    if(params->numWindows > SWOSD_MAX_WINDOWS)
    {
        /* truncating number of windows to MAX windows */
        params->numWindows = SWOSD_MAX_WINDOWS;
    }

    for(winId=0, algWinId=0; winId<params->numWindows; winId++)
    {
        pAlgWinPrm = &pChAlgDynPrm->winPrm[algWinId];

        pWinPrm = &params->winPrm[winId];

        pAlgWinPrm->startX              = SystemUtils_floor(pWinPrm->startX , 2);
        pAlgWinPrm->startY              = SystemUtils_floor(pWinPrm->startY , 2);
        pAlgWinPrm->width               = SystemUtils_floor(pWinPrm->width  , 2);
        pAlgWinPrm->height              = SystemUtils_floor(pWinPrm->height , 2);
        pAlgWinPrm->alpha               = AlgLink_osdAlgScaleGlobalAlpha(pWinPrm->globalAlpha);


        /*
            pAlgWinPrm->transperencyEnable  IS SET on ALL CH basis during create time NOT on per CH basis
            pWinPrm->format NOT USED - set once during create time
            pWinPrm->globalAlpha - NOT USED - win[0] global alpha value is used ALWAYS
        */

        pAlgWinPrm->lineOffset[0]       = pWinPrm->lineOffset;
        pAlgWinPrm->lineOffset[1]       = pWinPrm->lineOffset;

        pAlgWinPrm->graphicsWindowAddr[0] = pWinPrm->addr[0][0];
        pAlgWinPrm->graphicsWindowAddr[1] = pWinPrm->addr[0][1];

        if(!pWinPrm->enableWin)
            continue;

        /* window parameters out of range, do not include window for blending or clip the window WxH */
        if(pWinPrm->width > pChCreatePrm->maxWidth)
            continue;

        if(pWinPrm->height > pChCreatePrm->maxHeight)
            continue;

        if(pAlgWinPrm->startX > pChInfo->width)
            continue;

        if(pAlgWinPrm->startY > pChInfo->height)
            continue;

        if(pAlgWinPrm->startX + pAlgWinPrm->width > pChInfo->width)
            pAlgWinPrm->width = pChInfo->width - pAlgWinPrm->startX;

        if(pAlgWinPrm->startY + pAlgWinPrm->height > pChInfo->height)
            pAlgWinPrm->height = pChInfo->height - pAlgWinPrm->startY;

        if(pAlgWinPrm->width < ALG_LINK_OSD_MIN_WIN_WIDTH)
            continue;

        if(pAlgWinPrm->height < ALG_LINK_OSD_MIN_WIN_HEIGHT)
            continue;

        #if 0
        Vps_rprintf(" %d: OSD: CH%d: WIN%d: start=(%d, %d), %d x %d \n",
            Utils_getCurTimeInMsec(),
            params->chId,
            algWinId,
            pAlgWinPrm->startX, pAlgWinPrm->startY,
            pAlgWinPrm->width, pAlgWinPrm->height
            );
        #endif

        algWinId++;
    }

    pChAlgDynPrm->numWindows = algWinId;

    /*
        params->colorKey[] - NOT USED - set once during create time
    */


    if(status==FVID2_SOK && callAlgApi)
    {
        /* Call Alg API */
        status = SWOSD_setDynamicPrm(&pObj->algObj, params->chId, pChAlgDynPrm);

        if(status!=FVID2_SOK)
        {
            Vps_rprintf(" %d: OSD: ERROR during SWOSD_setDynamicPrm( CH%d ) !!!\n",
                Utils_getCurTimeInMsec(),
                params->chId
                );
        }

        if(params->chId==0)
        {
            UInt8 colorKey[SWOSD_MAX_PLANES];
            Int32 i;
            /* update transperency flag and color key based on CH0 info ONLY
                transperency and clorKey is NOT on per CH basis
            */

            for(i=0; i<SWOSD_MAX_PLANES; i++)
                colorKey[i] = params->colorKey[i];

            status = SWOSD_updateTransparencyEnableFlag(
                        &pObj->algObj,
                        params->winPrm[0].transperencyEnable,
                        colorKey
                        );

            if(status!=FVID2_SOK)
            {
                Vps_rprintf(" %d: OSD: ERROR during SWOSD_updateTransparencyEnableFlag( CH%d ) !!!\n",
                    Utils_getCurTimeInMsec(),
                    params->chId
                    );
            }

            status = SWOSD_updateGlobalAlpha(
                        &pObj->algObj,
                        pObj->algCreatePrm.useGlobalAlpha,
                        AlgLink_osdAlgScaleGlobalAlpha(params->winPrm[0].globalAlpha)
                        );

            if(status!=FVID2_SOK)
            {
                Vps_rprintf(" %d: OSD: ERROR during SWOSD_updateTransparencyEnableFlag( CH%d ) !!!\n",
                    Utils_getCurTimeInMsec(),
                    params->chId
                    );
            }

        }
    }

	return (status);
}






