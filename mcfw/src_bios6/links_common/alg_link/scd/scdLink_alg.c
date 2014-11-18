/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include "scdLink_priv.h"

Int32 AlgLink_scdAlgIsValidCh(AlgLink_ScdObj * pObj, UInt32 chId)
{
    if(chId < ALG_LINK_SIMCOP_SCD_MAX_CH
        &&
       chId < pObj->inQueInfo->numCh
        &&
        pObj->chObj[chId].enableScd==TRUE
    )
    {
        return TRUE;
    }

    return FALSE;
}



Int32 AlgLink_scdAlgResetStatistics(AlgLink_ScdObj * pObj)
{
    AlgLink_ScdChObj *pChObj;
    Int32 chId, i;

    for(chId=0; chId<ALG_LINK_SIMCOP_SCD_MAX_CH; chId++)
    {
        pChObj = &pObj->chObj[chId];

        pChObj->inFrameRecvCount = 0;
        pChObj->inFrameSkipCount = 0;
        pChObj->inFrameProcessCount = 0;
    }

    pObj->statsStartTime = Utils_getCurTimeInMsec();

    for(i=0; i<SCD_MAX_PROFILE_LOG; i++)
        pObj->profileLog[i] = 0;

    return 0;
}

Int32 AlgLink_scdAlgPrintStatistics(AlgLink_ScdObj * pObj, Bool resetAfterPrint)
{
    UInt32 chId;
    AlgLink_ScdChObj *pChObj;
    UInt32 elaspedTime;
    UInt64 referenceTimeInUsecs;

    elaspedTime = Utils_getCurTimeInMsec() - pObj->statsStartTime; // in msecs
    elaspedTime /= 1000; // convert to secs

    referenceTimeInUsecs = (Utils_getCurTimeInMsec() - pObj->statsStartTime)*1000;

    Vps_printf( " \n"
            " *** (SIMCOP) SCD Statistics *** \n"
            " \n"
            " Elasped Time           : %d secs\n"
            " Total Fields Processed : %d \n"
            " Total Fields FPS       : %d FPS\n"
            " \n"
            " Detailed Internal Profile Log, \n"
            " - SCD ALG Total time   : %10d msecs (%d %% of elasped time) \n"
            " - SIMCOP  Total time   : %10d msecs (%d %% of elasped time) \n"
            " - SCD ALG   CPU time   : %10d msecs (%d %% of elasped time) \n"

            " \n"
            " \n"
            " CH  | In Recv In Skip In Process \n"
            " Num | FPS     FPS     FPS        \n"
            " ---------------------------------\n",
            elaspedTime,
            pObj->processFrameCount,
            pObj->processFrameCount * 100 / (pObj->totalTime / 10),
            (UInt32)(pObj->profileLog[SCD_PROFILE_TOTAL_TIME]/1000), (UInt32)(pObj->profileLog[SCD_PROFILE_TOTAL_TIME]*100/referenceTimeInUsecs),
            (UInt32)(pObj->profileLog[SCD_PROFILE_SIMCOP_TIME]/1000), (UInt32)(pObj->profileLog[SCD_PROFILE_SIMCOP_TIME]*100/referenceTimeInUsecs),
            (UInt32)(pObj->profileLog[SCD_PROFILE_CPU_TIME]/1000), (UInt32)(pObj->profileLog[SCD_PROFILE_CPU_TIME]*100/referenceTimeInUsecs)
                    );

    for (chId = 0; chId < pObj->inQueInfo->numCh; chId++)
    {
        pChObj = &pObj->chObj[chId];

        Vps_printf( " %3d | %7d %7d %10d\n",
            chId,
            pChObj->inFrameRecvCount/elaspedTime,
            pChObj->inFrameSkipCount/elaspedTime,
            pChObj->inFrameProcessCount/elaspedTime
            );
    }

    Vps_printf( " \n");

    if(resetAfterPrint)
    {
        AlgLink_scdAlgResetStatistics(pObj);
    }
    return FVID2_SOK;
}

Int32 AlgLink_scdAlgChCreate(AlgLink_ScdObj * pObj)
{
    AlgLink_ScdChObj *pChObj;
    AlgLink_ScdCreateParams *pCreateArgs;
    AlgLink_ScdChParams *pChPrm;
    System_LinkChInfo *pChInfo;
    UInt32 blockId, chId, i;

    pCreateArgs = &pObj->scdCreateParams;

    for(chId=0; chId<ALG_LINK_SIMCOP_SCD_MAX_CH; chId++)
    {
        pChObj = &pObj->chObj[chId];

        memset(pChObj, 0, sizeof(*pChObj));

        pChObj->enableScd = FALSE;

        pChObj->scdMode = ALG_LINK_SCD_DETECTMODE_DISABLE;
    }

    for(i=0; i<pCreateArgs->numValidChForSCD; i++)
    {
        pChPrm = &pCreateArgs->chDefaultParams[i];

        chId =  pChPrm->chId;

        pChObj = &pObj->chObj[chId];

        pChInfo = &pObj->inQueInfo->chInfo[chId];

        pChObj->enableScd = TRUE;

        pChObj->scdMode = ALG_LINK_SCD_DETECTMODE_MONITOR_FULL_FRAME;

        pChObj->algReset = TRUE;

        pChObj->skipInitialFrames = TRUE;
        pChObj->startTime         = 0;

        pChObj->width   = SystemUtils_floor(pChInfo->width, ALG_LINK_SIMCOP_SCD_WIDTH_ALIGN);
        pChObj->height  = pChInfo->height;
        pChObj->rtPrmUpdate = FALSE;

        if(pChObj->width>pObj->algCreatePrm.maxWidth)
            pChObj->width = pObj->algCreatePrm.maxWidth;

        if(pChObj->height>pObj->algCreatePrm.maxHeight)
            pChObj->height = pObj->algCreatePrm.maxHeight;


        pChObj->frameSkipContext.inputFrameRate = pCreateArgs->inputFrameRate;
        pChObj->frameSkipContext.outputFrameRate = pCreateArgs->outputFrameRate;
        pChObj->frameSkipContext.firstTime = TRUE;

        pChObj->chId = pChPrm->chId;

        pChObj->scdStatus     = ALG_LINK_SCD_DETECTOR_UNAVAILABLE;
        pChObj->prevScdStatus = ALG_LINK_SCD_DETECTOR_UNAVAILABLE;

        pChObj->algProcessPrm.chanId                = chId;

        pChObj->algProcessPrm.width                 = pChObj->width;
        pChObj->algProcessPrm.height                = pChObj->height;
        pChObj->algProcessPrm.pitch                 = SystemUtils_align(pChInfo->width, ALG_LINK_SIMCOP_SCD_WIDTH_ALIGN);
        pChObj->algProcessPrm.mode                  = SCD_DETECTMODE_MONITOR_FULL_FRAME;
        pChObj->algProcessPrm.frmSensitivity        = (SCD_Sensitivity)pChPrm->frmSensitivity;
        pChObj->algProcessPrm.frmIgnoreLightsON     = pChPrm->frmIgnoreLightsON;
        pChObj->algProcessPrm.frmIgnoreLightsOFF    = pChPrm->frmIgnoreLightsOFF;

        pChObj->algProcessPrm.fps                   = SCD_FPS_05;
        pChObj->algProcessPrm.pAlgImageBuf          = &pChObj->algTmpImageBufs;
        pChObj->algProcessPrm.inAddr                = NULL;
        pChObj->algProcessPrm.prevInAddr            = NULL;

        pChObj->algProcessPrm.numSecs2WaitAfterFrmAlert = pCreateArgs->numSecs2WaitAfterFrmAlert;

        pChObj->algInitMeanVarMHIPrm.chanId         = chId;
        pChObj->algInitMeanVarMHIPrm.width          = pChObj->algProcessPrm.width;
        pChObj->algInitMeanVarMHIPrm.height         = pChObj->algProcessPrm.height;
        pChObj->algInitMeanVarMHIPrm.pitch          = pChObj->algProcessPrm.pitch;
        pChObj->algInitMeanVarMHIPrm.pAlgImageBuf   = &pChObj->algTmpImageBufs;
        pChObj->algInitMeanVarMHIPrm.inAddr         = NULL;

        for(blockId=0; blockId<pObj->algPerChMemAllocPrm.numMemBlocks; blockId++)
        {
            #ifdef SYSTEM_VERBOSE_PRINTS
            Vps_printf(" %d: SCD: CH%d: MEM REQUEST %d: of size %d B (align=%d)\n",
                Utils_getCurTimeInMsec(),
                chId,
                blockId,
                pObj->algPerChMemAllocPrm.memBlockSize[blockId],
                pObj->algPerChMemAllocPrm.memBlockAlign[blockId]
                );
            #endif

            pChObj->memBlockAddr[blockId] =
                    Utils_memAlloc(
                        pObj->algPerChMemAllocPrm.memBlockSize[blockId],
                        pObj->algPerChMemAllocPrm.memBlockAlign[blockId]
                    );
            UTILS_assert(pChObj->memBlockAddr[blockId]!=NULL);

            #ifdef SYSTEM_VERBOSE_PRINTS
            Vps_printf(" %d: SCD: CH%d: MEM ALLOC %d: @ 0x%08x of size %d B (align=%d)\n",
                Utils_getCurTimeInMsec(),
                chId,
                blockId,
                pChObj->memBlockAddr[blockId],
                pObj->algPerChMemAllocPrm.memBlockSize[blockId],
                pObj->algPerChMemAllocPrm.memBlockAlign[blockId]
                );
            #endif
        }

        pChObj->algTmpImageBufs.pBkgrdMeanSQ8_7     = pChObj->memBlockAddr[0];
        pChObj->algTmpImageBufs.pBkgrdVarianceSQ12_3= pChObj->memBlockAddr[1];
        pChObj->algTmpImageBufs.pMHIimageUQ8_0      = pChObj->memBlockAddr[2];

        {
            FVID2_Frame *pFrame;
            UInt32 frameId;
            Int32 status;

            pChObj->pPrevProcessFrame = NULL;

            status = Utils_queCreate(&pChObj->freeQ,
                                     ALG_LINK_MAX_PROCESS_FRAMES,
                                     pChObj->freeQMem,
                                     UTILS_QUE_FLAG_NO_BLOCK_QUE
                                        );

            UTILS_assert(status==FVID2_SOK);

            pChObj->processFrameSize = pChObj->algProcessPrm.pitch*pChObj->algProcessPrm.height;

            /* alloc channel process buffer memory */

            for(frameId=0; frameId<ALG_LINK_MAX_PROCESS_FRAMES; frameId++)
            {
                pFrame = &pChObj->processFrames[frameId];

                pFrame->addr[0][0] =
                        Utils_memAlloc(
                            pChObj->processFrameSize,
                            VPS_BUFFER_ALIGNMENT
                        );

                UTILS_assert(pFrame->addr[0][0]!=NULL);

                status = Utils_quePut(&pChObj->freeQ, pFrame, BIOS_NO_WAIT);

                UTILS_assert(status==FVID2_SOK);
            }
        }
    }

    return FVID2_SOK;
}

Int32 AlgLink_scdAlgAllocMem(AlgLink_ScdObj * pObj)
{
    UInt32 blockId;
    Int32 status;

    status = SCD_getAllocInfo(&pObj->algObj, &pObj->algMemAllocPrm, &pObj->algPerChMemAllocPrm);
    UTILS_assert(status==FVID2_SOK);

    for(blockId=0; blockId<pObj->algMemAllocPrm.numMemBlocks; blockId++)
    {

        #ifdef SYSTEM_VERBOSE_PRINTS
        Vps_printf(" %d: SCD: MEM REQUEST %d: of size %d B (align=%d)\n",
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
        Vps_printf(" %d: SCD: MEM ALLOC %d: @ 0x%08x of size %d B (align=%d)\n",
            Utils_getCurTimeInMsec(),
            blockId,
            pObj->algMemAllocPrm.memBlockAddr[blockId],
            pObj->algMemAllocPrm.memBlockSize[blockId],
            pObj->algMemAllocPrm.memBlockAlign[blockId]
            );
        #endif
    }

    status = SCD_setAllocInfo(&pObj->algObj, &pObj->algMemAllocPrm);
    UTILS_assert(status==FVID2_SOK);

    return FVID2_SOK;
}

Int32 AlgLink_scdAlgCreate(AlgLink_ScdObj * pObj)
{
	Int32 status;
    SCD_CreatePrm *pAlgCreatePrm;

    pAlgCreatePrm = &pObj->algCreatePrm;

    memset(pAlgCreatePrm, 0, sizeof(*pAlgCreatePrm));

    AlgLink_scdAlgResetStatistics(pObj);

    pObj->processFrameCount  = 0;
    pObj->totalTime  = 0;

    pAlgCreatePrm->maxWidth  = SystemUtils_align(pObj->scdCreateParams.maxStride, ALG_LINK_SIMCOP_SCD_WIDTH_ALIGN);
    pAlgCreatePrm->maxHeight = pObj->scdCreateParams.maxHeight;
    pAlgCreatePrm->maxPitch  = pAlgCreatePrm->maxWidth;
    
    Vps_printf(" %d: SCD: Opening algorithm ... !!!\n",
        Utils_getCurTimeInMsec()
        );

    status = SCD_open(&pObj->algObj, pAlgCreatePrm);

    UTILS_assert(status==FVID2_SOK);

    Vps_printf(" %d: SCD: Opening algorithm ... DONE !!!\n",
        Utils_getCurTimeInMsec()
        );

    AlgLink_scdAlgAllocMem(pObj);
    AlgLink_scdAlgChCreate(pObj);

    status = Utils_dmaCreateCh(&pObj->dmaCh, UTILS_DMA_DEFAULT_EVENT_Q, 1, TRUE);

    UTILS_assert(status==FVID2_SOK);

    status = Utils_queCreate(&pObj->processQ,
                             ALG_LINK_MAX_PROCESS_FRAMES*ALG_LINK_SIMCOP_SCD_MAX_CH,
                             pObj->processQMem,
                             UTILS_QUE_FLAG_BLOCK_QUE_GET
                            );
    UTILS_assert(status==FVID2_SOK);

    AlgLink_scdAlgProcessTskSendCmd(pObj, SYSTEM_CMD_START);

	return FVID2_SOK;
}

Int32 AlgLink_scdAlgFreeMem(AlgLink_ScdObj * pObj)
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

Int32 AlgLink_scdAlgChDelete(AlgLink_ScdObj * pObj)
{
    AlgLink_ScdChObj *pChObj;
    UInt32 blockId, chId;
    Int32 status;

    for(chId=0; chId<ALG_LINK_SIMCOP_SCD_MAX_CH; chId++)
    {
        pChObj = &pObj->chObj[chId];

        if(pChObj->enableScd==FALSE)
            continue;

        for(blockId=0; blockId<pObj->algPerChMemAllocPrm.numMemBlocks; blockId++)
        {
            status = Utils_memFree(
                    pChObj->memBlockAddr[blockId],
                    pObj->algPerChMemAllocPrm.memBlockSize[blockId]
                    );
            UTILS_assert(status==FVID2_SOK);
        }

        {
            FVID2_Frame *pFrame;
            UInt32 frameId;


            status = Utils_queDelete(&pChObj->freeQ);

            UTILS_assert(status==FVID2_SOK);

            /* free channel process buffer memory */

            for(frameId=0; frameId<ALG_LINK_MAX_PROCESS_FRAMES; frameId++)
            {
                pFrame = &pChObj->processFrames[frameId];

                status = Utils_memFree(
                            pFrame->addr[0][0], pChObj->processFrameSize
                                );

                UTILS_assert(status==FVID2_SOK);
            }
        }
    }

    return FVID2_SOK;
}

Int32 AlgLink_scdAlgDelete(AlgLink_ScdObj * pObj)
{
    Int32 status;

    AlgLink_scdAlgProcessTskSendCmd(pObj, SYSTEM_CMD_STOP);

    status = Utils_queDelete(&pObj->processQ);
    UTILS_assert(status==FVID2_SOK);

    status = Utils_dmaDeleteCh(&pObj->dmaCh);

    UTILS_assert(status==FVID2_SOK);

    status = SCD_close(&pObj->algObj);
    UTILS_assert(status==FVID2_SOK);

    AlgLink_scdAlgChDelete(pObj);
    AlgLink_scdAlgFreeMem(pObj);

	return FVID2_SOK;
}

Int32 AlgLink_scdAlgRtPrmUpdate(AlgLink_ScdObj * pObj, AlgLink_ScdChObj *pChObj, FVID2_Frame *pFrame)
{
    System_FrameInfo *pFrameInfo;
    System_LinkChInfo *pChInfo;
    UInt32 oldIntState;

    pFrameInfo = (System_FrameInfo *)pFrame->appData;

    if(pFrameInfo==NULL)
        return FVID2_EFAIL;

    if(pFrameInfo->rtChInfoUpdate==FALSE)
        return FVID2_SOK;

    pChInfo = &pFrameInfo->rtChInfo;

    oldIntState = Hwi_disable();

    if(pChInfo->width != pChObj->width
        ||
       pChInfo->height != pChObj->height
    )
    {
        pChObj->rtPrmUpdate           = TRUE;
        pChObj->algReset              = TRUE;
    }

    pChObj->width                 = SystemUtils_floor(pChInfo->width, ALG_LINK_SIMCOP_SCD_WIDTH_ALIGN);
    pChObj->height                = pChInfo->height;

    if(pChObj->width>pObj->algCreatePrm.maxWidth)
        pChObj->width = pObj->algCreatePrm.maxWidth;

    if(pChObj->height>pObj->algCreatePrm.maxHeight)
        pChObj->height = pObj->algCreatePrm.maxHeight;

    Hwi_restore(oldIntState);

    return FVID2_SOK;
}

Int32 AlgLink_scdAlgSubmitFrame(AlgLink_ScdObj * pObj, AlgLink_ScdChObj *pChObj, FVID2_Frame *pFrame)
{
    FVID2_Frame *pEmptyFrame;
    System_LinkChInfo *pChInfo;
    Utils_DmaCopy2D dmaPrm;
    Int32 status;

    pEmptyFrame = NULL;

    Utils_queGet(&pChObj->freeQ, (Ptr*)&pEmptyFrame, 1, BIOS_NO_WAIT);

    if(pEmptyFrame==NULL)
    {
        /* no free available for SCD, so skipping this frame */
        pChObj->inFrameSkipCount++;
        return FVID2_SOK;
    }

    /* found a free frame, do DMA copy of Y data to this frame */
    pChInfo = &pObj->inQueInfo->chInfo[pFrame->channelNum];

    dmaPrm.destAddr[0]  = pEmptyFrame->addr[0][0];
    dmaPrm.destPitch[0] = pChObj->algProcessPrm.pitch;

    dmaPrm.srcAddr[0]  = pFrame->addr[0][0];
    dmaPrm.srcPitch[0] = pChInfo->pitch[0];

    /* need to transfer only Y data, so set data format as 422I and set width = actual width / 2 */
    dmaPrm.dataFormat = FVID2_DF_YUV422I_YUYV;
    dmaPrm.srcStartX  = 0;
    dmaPrm.srcStartY  = 0;
    dmaPrm.destStartX = 0;
    dmaPrm.destStartY = 0;
    dmaPrm.width      = pChObj->width/2;
    dmaPrm.height     = pChObj->height;


    status = Utils_dmaCopy2D(&pObj->dmaCh, &dmaPrm, 1);
    UTILS_assert(status==FVID2_SOK);

    pEmptyFrame->channelNum = pFrame->channelNum;

    status = Utils_quePut(&pObj->processQ, pEmptyFrame, BIOS_NO_WAIT);

    if(status!=FVID2_SOK)
    {
        /* cannot submit frame now process queue is full, release frame to free Q */
        pChObj->inFrameSkipCount++;

        status = Utils_quePut(&pChObj->freeQ, pEmptyFrame, BIOS_NO_WAIT);

        /* this assert should never occur */
        UTILS_assert(status==FVID2_SOK);

        return FVID2_SOK;
    }

    return FVID2_SOK;
}

Bool AlgLink_scdAlgSkipInitialFrames(AlgLink_ScdObj * pObj, AlgLink_ScdChObj *pChObj)
{
    if(pChObj->skipInitialFrames)
    {
        UInt32 elaspedTime;

        if(pChObj->startTime==0)
        {
            pChObj->startTime = Utils_getCurTimeInMsec();
        }

        elaspedTime = Utils_getCurTimeInMsec() - pChObj->startTime;

        if(elaspedTime >= pObj->scdCreateParams.numSecs2WaitB4Init*1000)
            pChObj->skipInitialFrames = FALSE;
    }

    return pChObj->skipInitialFrames;
}

Int32 AlgLink_scdAlgProcessFrames(Utils_TskHndl *pTsk, AlgLink_ScdObj * pObj, FVID2_FrameList *pFrameList)
{
    Int32 status = FVID2_SOK;
    UInt32 frameId;
    FVID2_Frame *pFrame;
    AlgLink_ScdChObj *pChObj;
    Bool skipFrame;

    if(pObj->processFrameCount==0)
    {
        AlgLink_scdAlgResetStatistics(pObj);
    }

    for(frameId=0; frameId<pFrameList->numFrames; frameId++)
    {
        pFrame = pFrameList->frames[frameId];

        if(pFrame==NULL)
            continue;

        if(!AlgLink_scdAlgIsValidCh(pObj, pFrame->channelNum))
        {
            continue;
        }

        pChObj = &pObj->chObj[pFrame->channelNum];

        pChObj->inFrameRecvCount++;

        AlgLink_scdAlgRtPrmUpdate(pObj, pChObj, pFrame);

        skipFrame = Utils_doSkipFrame(&pChObj->frameSkipContext);

        if(skipFrame)
        {
            pChObj->inFrameSkipCount++;
            continue;
        }

        if(pChObj->scdMode != ALG_LINK_SCD_DETECTMODE_MONITOR_FULL_FRAME)
        {
            /* SCD is disabled for this channel */
            pChObj->inFrameSkipCount++;

            /* reset the alg so that next it starts from initial condition */
            pChObj->algReset = TRUE;
            continue;
        }

        if(AlgLink_scdAlgSkipInitialFrames(pObj, pChObj))
            continue;

        AlgLink_scdAlgSubmitFrame(pObj, pChObj, pFrame);
    }

    return status;
}


Int32 AlgLink_scdAlgGetAllChFrameStatus(AlgLink_ScdObj * pObj, AlgLink_ScdAllChFrameStatus *pPrm)
{
    AlgLink_ScdChObj *pChObj;
    UInt32 chId;
    UInt32 oldIntState;

    pPrm->numCh = 0;

    for(chId=0; chId<ALG_LINK_SIMCOP_SCD_MAX_CH; chId++)
    {
        pChObj = &pObj->chObj[chId];

        if(pChObj->enableScd==TRUE)
        {
            pPrm->chanFrameResult[pPrm->numCh].chId = pChObj->chId;

            oldIntState = Hwi_disable();

            pPrm->chanFrameResult[pPrm->numCh].frmResult = pChObj->scdStatus;

            if(pChObj->scdMode==ALG_LINK_SCD_DETECTMODE_DISABLE)
                pPrm->chanFrameResult[pPrm->numCh].frmResult = (UInt32)ALG_LINK_SCD_DETECTOR_UNAVAILABLE;

            Hwi_restore(oldIntState);

            pPrm->numCh++;

            if(pPrm->numCh >= ALG_LINK_SCD_MAX_CH)
            {
                break;
            }
        }
    }

    return FVID2_SOK;
}

Int32 AlgLink_scdAlgSetChMode(AlgLink_ScdObj * pObj, AlgLink_ScdChParams *pPrm)
{
    AlgLink_ScdChObj *pChObj;

    if(AlgLink_scdAlgIsValidCh(pObj, pPrm->chId)==FALSE)
        return FVID2_EFAIL;

    pChObj = &pObj->chObj[pPrm->chId];

    if(pPrm->mode == ALG_LINK_SCD_DETECTMODE_MONITOR_FULL_FRAME)
        pChObj->scdMode = ALG_LINK_SCD_DETECTMODE_MONITOR_FULL_FRAME;
    else
        pChObj->scdMode = ALG_LINK_SCD_DETECTMODE_DISABLE;

    return FVID2_SOK;
}

Int32 AlgLink_scdAlgSetChIgnoreLightsOn(AlgLink_ScdObj * pObj, AlgLink_ScdChParams *pPrm)
{
    AlgLink_ScdChObj *pChObj;

    if(AlgLink_scdAlgIsValidCh(pObj, pPrm->chId)==FALSE)
        return FVID2_EFAIL;

    pChObj = &pObj->chObj[pPrm->chId];

    pChObj->algProcessPrm.frmIgnoreLightsON     = pPrm->frmIgnoreLightsON;

    return FVID2_SOK;
}

Int32 AlgLink_scdAlgSetChIgnoreLightsOff(AlgLink_ScdObj * pObj, AlgLink_ScdChParams *pPrm)
{
    AlgLink_ScdChObj *pChObj;

    if(AlgLink_scdAlgIsValidCh(pObj, pPrm->chId)==FALSE)
        return FVID2_EFAIL;

    pChObj = &pObj->chObj[pPrm->chId];

    pChObj->algProcessPrm.frmIgnoreLightsOFF     = pPrm->frmIgnoreLightsOFF;

    return FVID2_SOK;
}

Int32 AlgLink_scdAlgResetCh(AlgLink_ScdObj * pObj, AlgLink_ScdChCtrl *pPrm)
{
    AlgLink_ScdChObj *pChObj;

    if(AlgLink_scdAlgIsValidCh(pObj, pPrm->chId)==FALSE)
        return FVID2_EFAIL;

    pChObj = &pObj->chObj[pPrm->chId];

    pChObj->algReset = TRUE;

    return FVID2_SOK;
}

Int32 AlgLink_scdAlgSetChSensitivity(AlgLink_ScdObj * pObj, AlgLink_ScdChParams *pPrm)
{
    AlgLink_ScdChObj *pChObj;

    if(AlgLink_scdAlgIsValidCh(pObj, pPrm->chId)==FALSE)
        return FVID2_EFAIL;

    pChObj = &pObj->chObj[pPrm->chId];

    pChObj->algProcessPrm.frmSensitivity     = pPrm->frmSensitivity;

    if(pChObj->algProcessPrm.frmSensitivity > SCD_SENSITIVITY_VERYHIGH)
        pChObj->algProcessPrm.frmSensitivity = SCD_SENSITIVITY_VERYHIGH;

    return FVID2_SOK;
}
