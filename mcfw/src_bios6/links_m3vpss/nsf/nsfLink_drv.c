/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include <xdc/std.h>
#include <mcfw/interfaces/link_api/system_tiler.h>
#include "nsfLink_priv.h"


Int32 NsfLink_resetStatistics(NsfLink_Obj * pObj)
{
    UInt32 chId;
    NsfLink_ChObj *pChObj;

    for (chId = 0; chId < pObj->nsfCreateParams.numCh; chId++)
    {
        pChObj = &pObj->linkChInfo[chId];

        pChObj->inFrameRecvCount = 0;
        pChObj->inFrameRejectCount = 0;
        pChObj->inFrameProcessCount = 0;
        pChObj->outFrameCount = 0;
        pChObj->outFrameUserSkipCount = 0;
        pChObj->outFrameSkipCount     = 0;
    }


    pObj->statsStartTime = Utils_getCurTimeInMsec();

    return 0;
}

Int32 NsfLink_drvPrintStatistics(NsfLink_Obj * pObj, Bool resetAfterPrint)
{
    UInt32 chId;
    NsfLink_ChObj *pChObj;
    UInt32 elaspedTime;

    elaspedTime = Utils_getCurTimeInMsec() - pObj->statsStartTime; // in msecs
    elaspedTime /= 1000; // convert to secs

    Vps_printf( " \n"
            " *** [%s] NSF Statistics *** \n"
            " \n"
            " Elasped Time           : %d secs\n"
            " Total Fields Processed : %d \n"
            " Total Fields FPS       : %d FPS\n"
            " \n"
            " \n"
            " CH  | In Recv In Reject In Process Out User Out Out      \n"
            " Num | FPS     FPS       FPS        FPS Skip FPS Skip FPS \n"
            " ------------------------------------------------\n",
            pObj->name,
            elaspedTime,
            pObj->processFrameCount,
            pObj->processFrameCount * 100 / (pObj->totalTime / 10)
                    );

    for (chId = 0; chId < pObj->inQueInfo.numCh; chId++)
    {
        pChObj = &pObj->linkChInfo[chId];

        Vps_printf( " %3d | %7d %9d %10d %3d %8d %8d\n",
            chId,
            pChObj->inFrameRecvCount/elaspedTime,
            pChObj->inFrameRejectCount/elaspedTime,
            pChObj->inFrameProcessCount/elaspedTime,
            pChObj->outFrameCount/elaspedTime,
            pChObj->outFrameUserSkipCount/elaspedTime,
            pChObj->outFrameSkipCount/elaspedTime
            );
    }

    Vps_printf( " \n");

    if(resetAfterPrint)
    {
        NsfLink_resetStatistics(pObj);
    }
    return FVID2_SOK;
}

Int32 NsfLink_printBufferStatus(NsfLink_Obj * pObj)
{
    Uint8 i, str[256];
    
    Vps_rprintf(
        " \n"
        " *** [%s] NSF Statistics *** \n"
        "%d: NSF: Rcvd from prev = %d, Returned to prev = %d\r\n",
        pObj->name,
        Utils_getCurTimeInMsec(), pObj->inFrameReceiveCount, pObj->inFrameGivenCount);

    for (i=0; i<pObj->createArgs.numOutQue; i++)
//        for (i=0; i<NSF_LINK_MAX_OUT_QUE; i++)
    {
        sprintf ((char *)str, "NSF Out [%d]", i);
        Utils_bufPrintStatus(str, &pObj->bufOutQue[i]);
    }
    return 0;
}


Int32 NsfLink_drvCallback(FVID2_Handle handle, Ptr appData, Ptr reserved)
{
    NsfLink_Obj *pObj = (NsfLink_Obj *) appData;

    Semaphore_post(pObj->complete);

    return FVID2_SOK;
}

Int32 NsfLink_drvFvidErrCb(FVID2_Handle handle, Ptr appData,
                           Ptr errList, Ptr reserved)
{
#ifdef SYSTEM_DEBUG_NSF
    Vps_printf(" %d: NSF: Error Callback !!!\n", Utils_getCurTimeInMsec());
#endif

    return FVID2_SOK;
}

Int32 NsfLink_drvCreate(NsfLink_Obj * pObj, NsfLink_CreateParams * pPrm)
{
    Int32 status;
    UInt32 queId;
    Semaphore_Params semParams;
    FVID2_CbParams cbParams;

#ifdef SYSTEM_DEBUG_NSF
    Vps_printf(" %d: NSF: Create in progress !!!\n", Utils_getCurTimeInMsec());
#endif

#ifdef SYSTEM_DEBUG_MEMALLOC
    Vps_printf (" %d: Before Create NSF\n", Utils_getCurTimeInMsec());
    System_memPrintHeapStatus();
#endif

    UTILS_MEMLOG_USED_START();
    /* Store the create params coming from the app */
    memcpy(&pObj->createArgs, pPrm, sizeof(*pPrm));

    pObj->inFrameReceiveCount = 0;
    pObj->inFrameGivenCount = 0;
    pObj->outFrameReceiveCount = 0;
    pObj->outFrameGivenCount = 0;
    pObj->processFrameReqCount = 0;
    pObj->getProcessFrameReqCount = 0;
    pObj->processFrameCount = 0;
    pObj->getFrames = 0;
    pObj->totalTime = 0;
    pObj->curTime = 0;
    pObj->curEvenFieldFrame = 0;

    NsfLink_resetStatistics(pObj);

    /* Info about the NSF link */
    pObj->info.numQue = NSF_LINK_MAX_OUT_QUE;

    for (queId = 0; queId < NSF_LINK_MAX_OUT_QUE; queId++)
    {
        status = Utils_bufCreate(&pObj->bufOutQue[queId], FALSE, FALSE);
        UTILS_assert(status == FVID2_SOK);

        pObj->info.queInfo[queId].numCh = 0;
    }

    status = Utils_bufCreate(&pObj->bufEvenFieldOutQue, FALSE, FALSE);
    UTILS_assert(status == FVID2_SOK);

    pObj->info.queInfo[2].numCh = 0;

    /* Copy previous link (capture) info */
    status = System_linkGetInfo(pPrm->inQueParams.prevLinkId, &pObj->inTskInfo);
    UTILS_assert(status == FVID2_SOK);
    UTILS_assert(pPrm->inQueParams.prevLinkQueId < pObj->inTskInfo.numQue);

    #ifndef SYSTEM_USE_TILER
    if (pObj->createArgs.tilerEnable)
    {
        Vps_printf("NSFLINK:!!WARNING.FORCIBLY DISABLING TILER since tiler is disabled at build time");
        pObj->createArgs.tilerEnable = FALSE;
    }
    #endif

    /* Copy previous link's queue info */
    memcpy(&pObj->inQueInfo,
           &pObj->inTskInfo.queInfo[pPrm->inQueParams.prevLinkQueId],
           sizeof(pObj->inQueInfo));

    /* Create semaphores */
    Semaphore_Params_init(&semParams);
    semParams.mode = Semaphore_Mode_BINARY;
    pObj->complete = Semaphore_create(0u, &semParams, NULL);
    UTILS_assert(pObj->complete != NULL);

    /*
     * Configure the channels, allocate frames for each channel and
     * put those frames in channel specific buffer queue.
     */
    NsfLink_drvInitCh(pObj);

    /* Callback functions */
    memset(&cbParams, 0, sizeof(cbParams));

    cbParams.cbFxn = NsfLink_drvCallback;
    cbParams.appData = pObj;
    cbParams.errCbFxn = NsfLink_drvFvidErrCb;
    cbParams.errList = &pObj->errCbProcessList;

    /* Create NSF handle */
    pObj->fvidHandleNsf = FVID2_create(FVID2_VPS_M2M_NSF_DRV,
                                       VPS_M2M_INST_NF0,
                                       &pObj->nsfCreateParams,
                                       &pObj->nsfCreateStatus, &cbParams);
    UTILS_assert(pObj->fvidHandleNsf != NULL);

    UTILS_MEMLOG_USED_END(pObj->memUsed);
    UTILS_MEMLOG_PRINT("NSF:",
                       pObj->memUsed,
                       UTILS_ARRAYSIZE(pObj->memUsed));

#ifdef SYSTEM_DEBUG_NSF
    Vps_printf(" %d: NSF: Create Done !!!\n", Utils_getCurTimeInMsec());
#endif

#ifdef SYSTEM_DEBUG_MEMALLOC    
    Vps_printf (" %d: After Create NSF\n", Utils_getCurTimeInMsec());
    System_memPrintHeapStatus();
#endif

    return status;
}

/* This function gets called when capture driver has captured frames. This
 * function takes those frames from the capture driver and stores them in the
 * channel specific buf-queue for noise filtering stage. */
Int32 NsfLink_drvProcessData(NsfLink_Obj * pObj)
{
    UInt32 frameId;
    FVID2_FrameList frameList;
    FVID2_Frame *pFrame;
    System_LinkInQueParams *pInQueParams;
    Int32 status;
    UInt32 freeFrameNum;
    NsfLink_ChObj *pChObj;
    UInt32 emptyBufCnt;

    /* Pointer to the input link's queue */
    pInQueParams = &pObj->createArgs.inQueParams;

    /* Get the captured frames from the capture driver */
    System_getLinksFullFrames(pInQueParams->prevLinkId,
                              pInQueParams->prevLinkQueId, &frameList);

    if (frameList.numFrames)
    {
        freeFrameNum = 0;
        pObj->inFrameReceiveCount += frameList.numFrames;

        /* For each captured frame, check the channel no and copy it
         * accordingly in the fullQueue. */
        for (frameId = 0; frameId < frameList.numFrames; frameId++)
        {
            pFrame = frameList.frames[frameId];

            UTILS_assert(pFrame->channelNum < pObj->nsfCreateParams.numCh);

            pChObj = &pObj->linkChInfo[pFrame->channelNum];

            pChObj->inFrameRecvCount++;

            #if (NSF_LINK_DROP_INPUT_ON_OUTBUF_UNAVAILABLE == TRUE)
            emptyBufCnt =
            Utils_bufGetEmptyFrameCount(&pObj->linkChInfo[pFrame->channelNum].bufInQue);
            #else
            emptyBufCnt = 1;
            #endif
/*            if ((pObj->inQueInfo.chInfo[pFrame->channelNum].scanFormat ==
                                               FVID2_SF_PROGRESSIVE) ||
                                               (pChObj->nextFid == 0))
*/
            {
                pChObj->doFrameDrop = Utils_doSkipFrame(&(pChObj->frameSkipCtx));
            }

            if ((pChObj->doFrameDrop == FALSE) && (emptyBufCnt != 0))
            {
                status = Utils_bufPutFullFrame(&pChObj->bufInQue, pFrame);
                UTILS_assert(status == FVID2_SOK);
            }
            else
            {
                pChObj->inFrameRejectCount++;
                frameList.frames[freeFrameNum] = pFrame;
                freeFrameNum++;
                if (pChObj->doFrameDrop == TRUE)
                    pChObj->outFrameUserSkipCount++;
                if (0 == emptyBufCnt)
                    pChObj->outFrameSkipCount++;
            }
        }

        if (freeFrameNum)
        {
            frameList.numFrames = freeFrameNum;

#ifdef SYSTEM_DEBUG_NSF_RT
            Vps_printf(" %d: NF    : Skipped %d IN frames !!!\n",
                       Utils_getCurTimeInMsec(), frameList.numFrames);
#endif
            pObj->inFrameGivenCount += frameList.numFrames;
            System_putLinksEmptyFrames(pInQueParams->prevLinkId,
                                       pInQueParams->prevLinkQueId, &frameList);
        }
    }

    do
    {
        status = NsfLink_drvDoNsfFilter(pObj);
    } while (status == FVID2_SOK);

    return FVID2_SOK;
}

Int32 NsfLink_drvDoNsfFilter(NsfLink_Obj * pObj)
{
    FVID2_FrameList inFrameList[2], outFrameList;
    FVID2_ProcessList processList;
    FVID2_Frame *pFullFrame;
    FVID2_Frame *pEmptyFrame;
    Int32 status;
    UInt32 chId, frameId, queueId = 0, chPerQueue;
    FVID2_Frame *pFrame;
    UInt32 sendMsgToTsk = 0;
    UInt32 perChCount, numFrames;
    System_FrameInfo *pInFrameInfo;
    System_FrameInfo *pOutFrameInfo;
    NsfLink_ChObj *pChObj;
    Vps_M2mNsfRtParams *rtParams;
    Vps_NsfDataFormat *rtParamDataFmt;
    NsfLink_ChObj *nsfChObj;
    Bool rtParamUpdatePerFrame;

    /* Initialize the process list with different frame lists */
    processList.inFrameList[0] = &inFrameList[0];
    processList.inFrameList[1] = &inFrameList[1];
    processList.outFrameList[0] = &outFrameList;

    processList.numInLists = 2;
    processList.numOutLists = 1;

    processList.drvData = NULL;
    processList.reserved = NULL;

    numFrames = 0;

    pObj->getFrames++;

    /** For all the available channels, look into the respective
     * bufInQue.fullQueue and take the frames out of that queue and add them
     * to the inFrameList.
     * Take the same number of frames from the respective bufInQue.emptyQueue
     * and add them to the outFrameList.
     * This process will make the desired processList, ready for noise
     * filtering.
     */
    for (chId = 0; chId < pObj->nsfCreateParams.numCh; chId++)
    {
        /* While there are captured frames... and less than what NSF could
         * consume per request per channel. */
        perChCount = 0u;
        while (1)
        {
            status = Utils_bufGetFullFrame(&pObj->linkChInfo[chId].bufInQue,
                                           &pFullFrame, BIOS_NO_WAIT);
            if (status != FVID2_SOK)
                break;

            pInFrameInfo = (System_FrameInfo *) pFullFrame->appData;

            /* First check whether it can be accomodated or not */
            status = Utils_bufGetEmptyFrame(&pObj->linkChInfo[chId].bufInQue,
                                            &pEmptyFrame, BIOS_WAIT_FOREVER);
            UTILS_assert(status == FVID2_SOK);

            pOutFrameInfo = (System_FrameInfo *) pEmptyFrame->appData;


            /* Add both the frames in the respective frame lists */
            inFrameList[0].frames[numFrames] = pFullFrame;
            inFrameList[1].frames[numFrames] =
                pObj->linkChInfo[chId].pPrevOutFrame;
            
            pFullFrame->perFrameCfg = NULL;
            rtParamUpdatePerFrame = FALSE;
            UTILS_assert (pFullFrame->channelNum == chId);
            nsfChObj = &pObj->linkChInfo[chId];
            rtParamDataFmt = &nsfChObj->nsfRtParamDataFormat;
            if ((pInFrameInfo != NULL) && (pInFrameInfo->rtChInfoUpdate == TRUE))
            {
                if (pInFrameInfo->rtChInfo.height != rtParamDataFmt->inFrameHeight)
                {
                    rtParamDataFmt->inFrameHeight = pInFrameInfo->rtChInfo.height;
                    rtParamUpdatePerFrame = TRUE;
                }
                if (pInFrameInfo->rtChInfo.width != rtParamDataFmt->inFrameWidth)
                {
                    rtParamDataFmt->inFrameWidth = pInFrameInfo->rtChInfo.width;
                    rtParamUpdatePerFrame = TRUE;
                }
                if (pInFrameInfo->rtChInfo.pitch[0] != rtParamDataFmt->inPitch)
                {
                    rtParamDataFmt->inPitch = pInFrameInfo->rtChInfo.pitch[0];
                    rtParamUpdatePerFrame = TRUE;
                }
            }
            if (rtParamUpdatePerFrame == TRUE)
            {
                rtParams = &nsfChObj->nsfRtParams;
                rtParams->dataFormat = &nsfChObj->nsfRtParamDataFormat;
                rtParams->processingCfg = NULL;
                pFullFrame->perFrameCfg = rtParams;
            }

            pOutFrameInfo->rtChInfo.width    = rtParamDataFmt->inFrameWidth;
            pOutFrameInfo->rtChInfo.height   = rtParamDataFmt->inFrameHeight;
            pOutFrameInfo->rtChInfo.pitch[0] = rtParamDataFmt->outPitch[0];
            pOutFrameInfo->rtChInfo.pitch[1] = rtParamDataFmt->outPitch[1];
            pOutFrameInfo->rtChInfoUpdate    = TRUE;

            outFrameList.frames[numFrames] = pEmptyFrame;

            /* next previous output frame is current output frame */
            pObj->linkChInfo[chId].pPrevOutFrame = pEmptyFrame;

            pEmptyFrame->timeStamp = pFullFrame->timeStamp;
            if (pInFrameInfo != NULL)
                pOutFrameInfo->ts64  = pInFrameInfo->ts64;

            nsfChObj->inFrameProcessCount++;
            numFrames++;
            perChCount++;
            if (perChCount >
                0 /* pObj->nsfCreateStatus.maxFramesPerChInQueue */ )
                break;
        }
    }

    pObj->getFrames--;

    inFrameList[0].numFrames = numFrames;
    inFrameList[1].numFrames = numFrames;
    outFrameList.numFrames = numFrames;

    /* Start noise filtering */
    if (inFrameList[0].numFrames)
    {
#ifdef SYSTEM_DEBUG_NSF_RT
        Vps_printf(" %d: NSF: Noise Filtering %d frames !!!\n",
                   Utils_getCurTimeInMsec(), inFrameList[0].numFrames);
#endif

        pObj->processFrameReqCount++;

        pObj->curTime = Utils_getCurTimeInMsec();

        status = FVID2_processFrames(pObj->fvidHandleNsf, &processList);
        UTILS_assert(status == FVID2_SOK);

        Semaphore_pend(pObj->complete, BIOS_WAIT_FOREVER);

        status = FVID2_getProcessedFrames(pObj->fvidHandleNsf, &processList,
                                          BIOS_NO_WAIT);
        UTILS_assert(status == FVID2_SOK);

        pObj->curTime = Utils_getCurTimeInMsec() - pObj->curTime;

        pObj->totalTime += pObj->curTime;

        pObj->processFrameCount += processList.outFrameList[0]->numFrames;

        pObj->getProcessFrameReqCount++;

#ifdef SYSTEM_DEBUG_NSF_RT
        Vps_printf(" %d: NSF: Noise Filtering of %d frames Done !!!\n",
                   Utils_getCurTimeInMsec(), inFrameList[0].numFrames);
#endif
        /* Put the processed frames in the appropriate output queue */
        for (frameId = 0; frameId < outFrameList.numFrames; frameId++)
        {
            pFrame = outFrameList.frames[frameId];

            if (pFrame)
            {
                pChObj = &pObj->linkChInfo[pFrame->channelNum];
                /** Split the frames into both the output queues,
                 * if they are enabled. Else use output queue 0 only.
                 * Also, if output queue 1 is used, frames sent to this queue
                 * should be modified before submitting so that the
                 * pFrame->channelNum should start with 0 and not with
                 * (pObj->nsfCreateParams.numCh / 2).
                 */
                chPerQueue =
                    (pObj->nsfCreateParams.numCh / pObj->createArgs.numOutQue);
                queueId = (pFrame->channelNum / chPerQueue);
                pFrame->channelNum = pFrame->channelNum % chPerQueue;
                pChObj->outFrameCount++;

                /* Set the flag whether queue is filled or not */
                sendMsgToTsk |= (1 << queueId);
    
                status = Utils_bufPutFullFrame(&pObj->bufOutQue[queueId], pFrame);
                UTILS_assert(status == FVID2_SOK);
            } /* end of if (pFrame) */
        }

        /* Send new data available command to the next link */
        for (queueId = 0; queueId < NSF_LINK_MAX_OUT_QUE; queueId++)
        {
            if (sendMsgToTsk & 0x1)
            {
                pObj->outFrameGivenCount += outFrameList.numFrames;
                /* Send data available message to next task */
                System_sendLinkCmd(pObj->createArgs.outQueParams[queueId].
                                   nextLink, SYSTEM_CMD_NEW_DATA);
            }

            sendMsgToTsk >>= 1;
            if (sendMsgToTsk == 0)
                break;
        }

        pObj->inFrameGivenCount += inFrameList[0].numFrames;

        /* Return frames back to the capture link as well */
        System_putLinksEmptyFrames(pObj->createArgs.inQueParams.prevLinkId,
                                   pObj->createArgs.inQueParams.prevLinkQueId,
                                   &inFrameList[0]);
        status = FVID2_SOK;
    }
    else
    {
#ifdef SYSTEM_DEBUG_NSF_RT
        Vps_printf(" %d: NSF: No frames available for filtering !!!\n",
                   Utils_getCurTimeInMsec());
#endif

        /* no more frame availble so exit the loop outside of this function */
        status = FVID2_EFAIL;
    }

    return status;
}

/* This function will be called from the next link in the chain when the
 * frames are no more required. This function restores channel number for
 * each frame from the appData and pushes the frame back to the channel
 * specific bufInQue.emptyQueue. */
Int32 NsfLink_drvPutEmptyFrames(NsfLink_Obj * pObj,
                                FVID2_FrameList * pFrameList)
{
    Int32 status;
    UInt32 frameId;
    FVID2_Frame *pFrame;
    System_FrameInfo *pFrameInfo;

    pObj->outFrameReceiveCount += pFrameList->numFrames;
    if (pFrameList->numFrames)
    {
        for (frameId = 0; frameId < pFrameList->numFrames; frameId++)
        {
            pFrame = pFrameList->frames[frameId];

            /* Channel number might have changed, restore it with the
             * orignial stored value. */
            pFrameInfo = (System_FrameInfo *) pFrame->appData;
            UTILS_assert(pFrameInfo != NULL);
            pFrame->channelNum = pFrameInfo->nsfChannelNum;

            UTILS_assert(pFrame->channelNum < pObj->nsfCreateParams.numCh);

            /* Return each frame to its original channel specific
             * NsfLink_ChObj.bufInQue.emptyQue. */
            status =
                Utils_bufPutEmptyFrame(&pObj->linkChInfo[pFrame->channelNum].
                                       bufInQue, pFrame);
            UTILS_assert(status == FVID2_SOK);
        }

#ifdef SYSTEM_DEBUG_NSF_RT
        Vps_printf(" %d: NSF: Returned %d frames to NF channels !!!\n",
                   Utils_getCurTimeInMsec(), pFrameList->numFrames);
#endif
    }

    return FVID2_SOK;
}

Int32 NsfLink_drvDelete(NsfLink_Obj * pObj)
{
    UInt32 queId, chId;
    NsfLink_ChObj *nsfChObj;

#ifdef SYSTEM_DEBUG_DEI
    Vps_printf(" %d: NSF    : Fields = %d (fps = %d) !!!\n",
               Utils_getCurTimeInMsec(),
               pObj->outFrameGivenCount,
               pObj->outFrameGivenCount * 100 / (pObj->totalTime / 10));
#endif

#ifdef SYSTEM_DEBUG_NSF
    Vps_printf(" %d: NSF: Delete in progress !!!\n", Utils_getCurTimeInMsec());
#endif

    /* NSF handle */
    FVID2_delete(pObj->fvidHandleNsf, NULL);

    /* Free the allocated frames */
    NsfLink_drvFreeFrames(pObj);

    /* Free the NSF link output queues */
    for (queId = 0; queId < NSF_LINK_MAX_OUT_QUE; queId++)
    {
        Utils_bufDelete(&pObj->bufOutQue[queId]);
    }

    Utils_bufDelete(&pObj->bufEvenFieldOutQue);

    /* Free the channel specific buf-queue */
    for (chId = 0; chId < pObj->nsfCreateParams.numCh; chId++)
    {
        nsfChObj = &pObj->linkChInfo[chId];
        Utils_bufDelete(&nsfChObj->bufInQue);
    }

    /* Delete semaphores */
    Semaphore_delete(&pObj->complete);

#ifdef SYSTEM_DEBUG_NSF
    Vps_printf(" %d: NSF: Delete Done !!!\n", Utils_getCurTimeInMsec());
#endif

    return FVID2_SOK;
}

Int32 NsfLink_drvInitCh(NsfLink_Obj * pObj)
{
    Int32 status;
    UInt32 i, chId, frameId, queueId, outChId, chPerQueue;
    System_LinkChInfo *pInChInfo;
    Vps_NsfDataFormat *nsfDataFmt;
    Vps_NsfProcessingCfg *nsfProcCfg;
    FVID2_Frame *frames;
    FVID2_Format format;
    NsfLink_ChObj *nsfChObj;
    UInt32 numFrmsPerCh;

    UTILS_assert(pObj != NULL);

    /* Fill the nsfCreateParams structure */
    /* Find no of channels from input (capture) queue */
    pObj->nsfCreateParams.numCh = pObj->inQueInfo.numCh;

    /*
     * Point to memory for data format structure for each channel
     */
    pObj->nsfCreateParams.dataFormat = &pObj->nsfDataFormat[0];

    /*
     * Point to memory for processing params structure for each channel
     */
    pObj->nsfCreateParams.processingCfg = &pObj->nsfProcCfg[0];

    /*
     * For each channel, do the initialization and
     * FVID2 frame / buffer allocation.
     */
    for (chId = 0; chId < pObj->nsfCreateParams.numCh; chId++)
    {
        pInChInfo = &pObj->inQueInfo.chInfo[chId];
        nsfDataFmt = &pObj->nsfDataFormat[chId];
        nsfProcCfg = &pObj->nsfProcCfg[chId];
        nsfChObj = &pObj->linkChInfo[chId];

        nsfChObj->frameSkipCtx.firstTime = TRUE;
        nsfChObj->frameSkipCtx.inputFrameRate  = pObj->createArgs.inputFrameRate;
        nsfChObj->frameSkipCtx.outputFrameRate = pObj->createArgs.outputFrameRate;
        /* Set the channel number first */
        nsfChObj->channelId = chId;

        nsfChObj->doFrameDrop = FALSE;
        nsfChObj->nextFid = FVID2_FID_TOP;

        /* Initialize the createParams.dataFormat */
        nsfDataFmt->channelNum = chId;
        nsfDataFmt->inMemType = pInChInfo->memType;

        if (pObj->createArgs.tilerEnable)
            nsfDataFmt->outMemType = VPS_VPDMA_MT_TILEDMEM;
        else
            nsfDataFmt->outMemType = VPS_VPDMA_MT_NONTILEDMEM;

        nsfDataFmt->inDataFormat = pInChInfo->dataFormat;
        nsfDataFmt->inFrameWidth = pInChInfo->width;
        nsfDataFmt->inFrameHeight = pInChInfo->height;

        nsfDataFmt->inPitch = pInChInfo->pitch[0];
        /* Only one output format is supported */
        nsfDataFmt->outDataFormat = FVID2_DF_YUV420SP_UV;

        if (nsfDataFmt->outMemType == VPS_VPDMA_MT_TILEDMEM)
        {
            nsfDataFmt->outPitch[0] = VPSUTILS_TILER_CNT_8BIT_PITCH;
            nsfDataFmt->outPitch[1] = VPSUTILS_TILER_CNT_16BIT_PITCH;
        }
        else
        {
            nsfDataFmt->outPitch[0] =
                VpsUtils_align(pInChInfo->width, VPS_BUFFER_ALIGNMENT * 2);
            nsfDataFmt->outPitch[1] =
                VpsUtils_align(pInChInfo->width, VPS_BUFFER_ALIGNMENT * 2);
        }
        nsfDataFmt->outPitch[2] = 0;

        /* Initialize the createParams.processingCfg */
        nsfProcCfg->channelNum = chId;
        /* TBD: Check later on */
        nsfProcCfg->bypassMode = (pObj->createArgs.bypassNsf ?
                                  VPS_NSF_DISABLE_SNF_TNF :
                                  VPS_NSF_DISABLE_NONE);
        nsfProcCfg->frameNoiseAutoCalcEnable = FALSE;
        nsfProcCfg->frameNoiseCalcReset = FALSE;
        nsfProcCfg->subFrameModeEnable = FALSE;
        nsfProcCfg->numLinesPerSubFrame = 128;

        for (i = 0; i < 3; i++)
        {
            nsfProcCfg->staticFrameNoise[i] = 0;
            nsfProcCfg->spatialStrengthLow[i] = VPS_NSF_PROCESSING_CFG_DEFAULT;
            nsfProcCfg->spatialStrengthHigh[i] = VPS_NSF_PROCESSING_CFG_DEFAULT;
        }

        nsfProcCfg->temporalStrength = VPS_NSF_PROCESSING_CFG_DEFAULT;
        nsfProcCfg->temporalTriggerNoise = VPS_NSF_PROCESSING_CFG_DEFAULT;
        nsfProcCfg->noiseIirCoeff = VPS_NSF_PROCESSING_CFG_DEFAULT;
        nsfProcCfg->maxNoise = VPS_NSF_PROCESSING_CFG_DEFAULT;
        nsfProcCfg->pureBlackThres = VPS_NSF_PROCESSING_CFG_DEFAULT;
        nsfProcCfg->pureWhiteThres = VPS_NSF_PROCESSING_CFG_DEFAULT;

        /*
         * Per channel bufInQue structure needs to be created & initialized.
         * bufInQue.fullQue will be populated with captured frames and
         * bufInQue.emptyQuewill be allocated here so that they can be
         * used later on for noise filtering.
         * Frames need to be allocated for bufInQue.emptyQue here.
         */
        /* Create the per channel bufInQue */
        status = Utils_bufCreate(&nsfChObj->bufInQue, TRUE, FALSE);
        UTILS_assert(status == FVID2_SOK);

        /* Fill format with channel specific values to allocate frame */
        format.channelNum = chId;
        format.width = pInChInfo->width;
        format.height =
            VpsUtils_align(pInChInfo->height, VPS_BUFFER_ALIGNMENT * 2);

        if (NSF_LINK_NUM_BUFS_PER_CH_DEFAULT == pObj->createArgs.numBufsPerCh || 
            pObj->createArgs.numBufsPerCh > NSF_LINK_FRAMES_PER_CH_MAX)
        {
            numFrmsPerCh = NSF_LINK_FRAMES_PER_CH_DEFAULT;
        }
        else
        {
            numFrmsPerCh = pObj->createArgs.numBufsPerCh ;
        }

        format.pitch[0] = nsfDataFmt->outPitch[0];
        format.pitch[1] = nsfDataFmt->outPitch[1];
        format.pitch[2] = nsfDataFmt->outPitch[2];
        format.fieldMerged[0] = FALSE;
        format.fieldMerged[1] = FALSE;
        format.fieldMerged[2] = FALSE;
        format.dataFormat = FVID2_DF_YUV420SP_UV;
        format.scanFormat = FVID2_SF_PROGRESSIVE;
        format.bpp = FVID2_BPP_BITS8;                      /* ignored */

        /*
         * Alloc memory based on 'format'
         * Allocated frame info is put in frames[]
         * numFrmsPerCh is the number of frames per channel to
         * allocate.
         */
#ifdef SYSTEM_DEBUG_MEMALLOC

        Vps_printf("NSF:ALLOCINFO:WIDTH[%d]:HEIGHT[%d]:PITCH[%d]:ChId[%d]:MEMTYPE[%d]\n",
                   format.width,
                   format.height,
                   format.pitch[0],
                   chId,
                   nsfDataFmt->outMemType);


#endif
        if (nsfDataFmt->outMemType == VPS_VPDMA_MT_TILEDMEM)
        {
            status = Utils_tilerFrameAlloc(&format, nsfChObj->frames,
                                  numFrmsPerCh);
        }
        else
        {
            status = Utils_memFrameAlloc(&format, nsfChObj->frames,
                                numFrmsPerCh);
        }
        UTILS_assert(status == FVID2_SOK);

        /* Set remaining parameters for the allocated frames */
        for (frameId = 0; frameId < numFrmsPerCh; frameId++)
        {
            /* Point to the frame's array */
            frames = &nsfChObj->frames[frameId];
            frames->perFrameCfg = NULL;
            frames->subFrameInfo = NULL;
            frames->appData = &nsfChObj->frameInfo[frameId];

            nsfChObj->frameInfo[frameId].nsfChannelNum = frames->channelNum;
            nsfChObj->frameInfo[frameId].rtChInfoUpdate = FALSE;
            /* Finally, add this frame to the NsfLink_ChObj.bufInQue.emptyQue
             */
            status = Utils_bufPutEmptyFrame(&nsfChObj->bufInQue, frames);
            UTILS_assert(status == FVID2_SOK);
        }

        /* make initial previous frame point to first frame for first frame
         * of NF this wont be used */
        nsfChObj->pPrevOutFrame = &nsfChObj->frames[0];

        /* Populate the remaining fileds of
         * NsfLink_Obj.System_LinkInfo.System_LinkQueInfo.System_LinkChInfo
         * This information will be used by the next link to configure itself
         * properly. Structure used: info.queInfo[].chInfo[] Channels will be
         * divided equally between output queues */
        chPerQueue = (pObj->nsfCreateParams.numCh / pObj->createArgs.numOutQue);
        outChId = chId % chPerQueue;
        queueId = chId / chPerQueue;
        UTILS_assert(queueId < pObj->info.numQue);

        pObj->info.queInfo[queueId].chInfo[outChId].dataFormat =
            FVID2_DF_YUV420SP_UV;
        pObj->info.queInfo[queueId].chInfo[outChId].memType =
            (Vps_VpdmaMemoryType) nsfDataFmt->outMemType;
        pObj->info.queInfo[queueId].chInfo[outChId].startX = 0;
        pObj->info.queInfo[queueId].chInfo[outChId].startY = 0;
        pObj->info.queInfo[queueId].chInfo[outChId].width =
            nsfDataFmt->inFrameWidth;
        pObj->info.queInfo[queueId].chInfo[outChId].height =
            nsfDataFmt->inFrameHeight;
        pObj->info.queInfo[queueId].chInfo[outChId].pitch[0] =
            nsfDataFmt->outPitch[0];
        pObj->info.queInfo[queueId].chInfo[outChId].pitch[1] =
            nsfDataFmt->outPitch[1];
        pObj->info.queInfo[queueId].chInfo[outChId].pitch[2] =
            nsfDataFmt->outPitch[2];
        pObj->info.queInfo[queueId].chInfo[outChId].scanFormat =
            pInChInfo->scanFormat;

        /* Increase the number of channels now */
        pObj->info.queInfo[queueId].numCh++;
        memcpy (&nsfChObj->nsfRtParamDataFormat, nsfDataFmt, 
                sizeof (Vps_NsfDataFormat));
    }

    return FVID2_SOK;
}

/*
 * Free allocated frames
 *
 * pObj - NSF driver information */
Int32 NsfLink_drvFreeFrames(NsfLink_Obj * pObj)
{
    UInt16 chId;
    FVID2_Format format;
    UInt32 tilerUsed = FALSE;
    Vps_NsfDataFormat *nsfDataFmt;
    NsfLink_ChObj *nsfChObj;
    UInt32 numFrmsPerCh;

    for (chId = 0; chId < pObj->nsfCreateParams.numCh; chId++)
    {
        nsfDataFmt = &pObj->nsfDataFormat[chId];
        nsfChObj = &pObj->linkChInfo[chId];

        /* fill format with channel specific values */
        format.channelNum = chId;
        format.width = nsfDataFmt->inFrameWidth;
        format.height =
            VpsUtils_align(nsfDataFmt->inFrameHeight, VPS_BUFFER_ALIGNMENT * 2);
        format.pitch[0] = nsfDataFmt->outPitch[0];
        format.pitch[1] = nsfDataFmt->outPitch[1];
        format.pitch[2] = nsfDataFmt->outPitch[2];
        format.fieldMerged[0] = FALSE;
        format.fieldMerged[1] = FALSE;
        format.fieldMerged[2] = FALSE;
        format.dataFormat = FVID2_DF_YUV420SP_UV;
        format.scanFormat = FVID2_SF_PROGRESSIVE;
        format.bpp = FVID2_BPP_BITS8;                      /* ignored */

        if (NSF_LINK_NUM_BUFS_PER_CH_DEFAULT == pObj->createArgs.numBufsPerCh || 
            pObj->createArgs.numBufsPerCh > NSF_LINK_FRAMES_PER_CH_MAX)
        {
            numFrmsPerCh = NSF_LINK_FRAMES_PER_CH_DEFAULT;
        }
        else
        {
            numFrmsPerCh = pObj->createArgs.numBufsPerCh ;
        }

        if (nsfDataFmt->outMemType == VPS_VPDMA_MT_TILEDMEM)
        {
            /*
             * Cannot free tiled frames
             */
            tilerUsed = TRUE;
        }
        else
        {
            /*
             * Free frames for this channel, based on format
             */
            Utils_memFrameFree(&format, nsfChObj->frames,
                               numFrmsPerCh);
        }
    }

    if (tilerUsed)
    {
        SystemTiler_freeAll();
    }

    return FVID2_SOK;
}

Int32 NsfLink_SetFrameRate(NsfLink_Obj * pObj, NsfLink_ChFpsParams * params)
{
    Int32 status = FVID2_SOK;
    NsfLink_ChObj *pChObj;

    if (params->chId < NSF_LINK_MAX_CH_PER_QUE)
    {
        pChObj = &pObj->linkChInfo[params->chId];

        pChObj->frameSkipCtx.firstTime = TRUE;
        pChObj->frameSkipCtx.inputFrameRate = params->inputFrameRate;
        pChObj->frameSkipCtx.outputFrameRate = params->outputFrameRate;
    }
    return (status);
}
