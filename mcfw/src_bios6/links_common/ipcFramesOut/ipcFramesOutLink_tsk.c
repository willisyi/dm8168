/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include "ipcFramesOutLink_priv.h"

#define IPC_FRAMES_IN_ENABLE_PROFILE                                       (TRUE)

#pragma DATA_ALIGN(gIpcFramesOutLink_tskStack, 32)
#pragma DATA_SECTION(gIpcFramesOutLink_tskStack, ".bss:taskStackSection")
UInt8
    gIpcFramesOutLink_tskStack[IPC_FRAMES_OUT_LINK_OBJ_MAX]
    [IPC_LINK_TSK_STACK_SIZE];

IpcFramesOutLink_Obj gIpcFramesOutLink_obj[IPC_FRAMES_OUT_LINK_OBJ_MAX];

static Int32 IpcFramesOutLink_reconfigPrdObj(IpcFramesOutLink_Obj * pObj,
                                           UInt period);
Int32 IpcFramesOutLink_getFullFrames(Utils_TskHndl * pTsk, UInt16 queId,
                                     FVID2_FrameList * pFrameBufList);

Int32 IpcFramesOutLink_putEmptyFrames(Utils_TskHndl * pTsk, UInt16 queId,
                                      FVID2_FrameList * pFrameBufList);



Int32 IpcFramesOutLink_SetFrameRate(IpcFramesOutLink_Obj * pObj, IpcOutM3Link_ChFpsParams * params)
{
    Int32 status = FVID2_SOK;
    IpcFramesOutLink_ChObj *pChObj;

    UTILS_assert(params->chId < (IPC_FRAMESOUT_LINK_MAX_CH));
    pChObj = &pObj->chObj[params->chId];

    pChObj->frameSkipCtx.firstTime = TRUE;
    pChObj->frameSkipCtx.inputFrameRate = params->inputFrameRate;
    pChObj->frameSkipCtx.outputFrameRate = params->outputFrameRate;

    return (status);
}


Int32 IpcFramesOutLink_resetStatistics(IpcFramesOutLink_Obj *pObj)
{
    UInt32 chId;
    IpcFramesOutLink_ChObj *pChObj;

    for (chId = 0; chId < pObj->numCh; chId++)
    {
        pChObj = &pObj->chObj[chId];

        pChObj->inFrameRecvCount = 0;
        pChObj->inFrameUserSkipCount = 0;
        pChObj->inFrameProcessCount = 0;
    }

    pObj->totalFrameCount = 0;

    pObj->statsStartTime = Utils_getCurTimeInMsec();

    return 0;
}


Int32 IpcFramesOutLink_printStatistics (IpcFramesOutLink_Obj *pObj, Bool resetAfterPrint)
{
    UInt32 chId;
    IpcFramesOutLink_ChObj *pChObj;
    UInt32 elaspedTime;

    elaspedTime = Utils_getCurTimeInMsec() - pObj->statsStartTime; // in msecs
    elaspedTime /= 1000; // convert to secs

    Vps_printf( " \n"
            " *** IpcFramesOutRTOS Statistics *** \n"
            " \n"
            " Elasped Time           : %d secs\n"
            " Total Fields Processed : %d \n"
            " Total Fields FPS       : %d FPS\n"
            " \n"
            " \n"
            " CH  | In Recv In Process In Skip\n"
            " Num | FPS     FPS        FPS    \n"
            " --------------------------------\n",
            elaspedTime,
                    pObj->totalFrameCount
            , pObj->totalFrameCount / (elaspedTime)
                    );

    for (chId = 0; chId < pObj->numCh; chId++)
    {
        pChObj = &pObj->chObj[chId];

        Vps_printf( " %3d | %7d %10d %7d\n",
            chId,
            pChObj->inFrameRecvCount/elaspedTime,
            pChObj->inFrameProcessCount/elaspedTime,
            pChObj->inFrameUserSkipCount/elaspedTime
             );
    }

    Vps_printf( " \n");

    if(resetAfterPrint)
    {
        IpcFramesOutLink_resetStatistics(pObj);
    }
    return FVID2_SOK;
}

Void IpcFramesOutLink_notifyCb(Utils_TskHndl * pTsk)
{
    IpcFramesOutLink_Obj *pObj = (IpcFramesOutLink_Obj *) pTsk->appData;

    IpcFramesOutLink_reconfigPrdObj(pObj, IPC_FRAMESOUT_LINK_DONE_PERIOD_MS);
    Utils_tskSendCmd(pTsk, SYSTEM_IPC_CMD_RELEASE_FRAMES);
}

static IpcFramesOutLink_initStats(IpcFramesOutLink_Obj * pObj)
{
    Utils_PrfTsHndl *tsHndlRestore;

    UTILS_assert(pObj->stats.tsHandle != NULL);
    tsHndlRestore = pObj->stats.tsHandle;
    memset(&pObj->stats, 0, sizeof(pObj->stats));
    pObj->stats.tsHandle = tsHndlRestore;
    /* Reset the timestamp counter */
    Utils_prfTsReset(pObj->stats.tsHandle);

}

static Void IpcFramesOutLink_prdCalloutFcn(UArg arg)
{
    IpcFramesOutLink_Obj *pObj = (IpcFramesOutLink_Obj *) arg;
    Int32 status;

    status = System_sendLinkCmd(pObj->tskId, SYSTEM_IPC_CMD_RELEASE_FRAMES);


    if (UTILS_ISERROR(status))
    {
        #ifdef SYSTEM_DEBUG_CMD_ERROR
        UTILS_warn("IPCFRAMEOUTLINK:[%s:%d]:"
                   "System_sendLinkCmd SYSTEM_IPC_CMD_RELEASE_FRAMES failed"
                   "errCode = %d", __FILE__, __LINE__, status);
        #endif
    }

}

static Int32 IpcFramesOutLink_createPrdObj(IpcFramesOutLink_Obj * pObj)
{
    Clock_Params clockParams;

    Clock_Params_init(&clockParams);
    clockParams.arg = (UArg) pObj;
    UTILS_assert(pObj->prd.clkHandle == NULL);

    Clock_construct(&(pObj->prd.clkStruct),
                    IpcFramesOutLink_prdCalloutFcn, 1, &clockParams);

    pObj->prd.clkHandle = Clock_handle(&pObj->prd.clkStruct);
    pObj->prd.clkStarted = FALSE;

    return IPC_FRAMESOUT_LINK_S_SUCCESS;

}

static Int32 IpcFramesOutLink_deletePrdObj(IpcFramesOutLink_Obj * pObj)
{
    /* Stop the clock */
    Clock_stop(pObj->prd.clkHandle);
    Clock_destruct(&(pObj->prd.clkStruct));
    pObj->prd.clkHandle = NULL;
    pObj->prd.clkStarted = FALSE;

    return IPC_FRAMESOUT_LINK_S_SUCCESS;
}

static Int32 IpcFramesOutLink_startPrdObj(IpcFramesOutLink_Obj * pObj, UInt period,
                                        Bool oneShotMode)
{

    UTILS_assert(pObj->prd.clkHandle != NULL);

    if (FALSE == pObj->prd.clkStarted)
    {
        if (TRUE == oneShotMode)
        {
            Clock_setPeriod(pObj->prd.clkHandle, 0);
        }
        else
        {
            Clock_setPeriod(pObj->prd.clkHandle, period);
        }
        Clock_setTimeout(pObj->prd.clkHandle, period);
        Clock_start(pObj->prd.clkHandle);
        pObj->prd.clkStarted = TRUE;
    }

    return IPC_FRAMESOUT_LINK_S_SUCCESS;
}

static Int32 IpcFramesOutLink_reconfigPrdObj(IpcFramesOutLink_Obj * pObj,
                                           UInt period)
{

    UTILS_assert(pObj->prd.clkHandle != NULL);

    if (TRUE == pObj->prd.clkStarted)
    {
        Clock_stop(pObj->prd.clkHandle);
    }
    Clock_setPeriod(pObj->prd.clkHandle, 0);
    Clock_setTimeout(pObj->prd.clkHandle, period);
    Clock_start(pObj->prd.clkHandle);
    return IPC_FRAMESOUT_LINK_S_SUCCESS;
}

static
Void IpcFramesOutLink_setOutQueInfo(IpcFramesOutLink_Obj *pObj)
{
    UInt32 outChId,queueId;
    UInt32 inChId, i, tempVal, modVal;
    System_LinkQueInfo *pInQueInfo;

    /* get input queue info */
    UTILS_assert(
        pObj->createArgs.baseCreateParams.inQueParams.prevLinkQueId
            <
        pObj->inQueInfo.numQue
        );

    pInQueInfo =
        &pObj->inQueInfo.queInfo[
                pObj->createArgs.baseCreateParams.inQueParams.prevLinkQueId
                ];

    /* set number of output queues  */
    pObj->info.numQue = pObj->createArgs.baseCreateParams.numOutQue;

    /* set number of input / output channels */
    pObj->numCh = pInQueInfo->numCh;

    if (pObj->createArgs.baseCreateParams.equallyDivideChAcrossOutQues == TRUE)
    {
        modVal = pObj->numCh%pObj->info.numQue;
        for (i=0; i<pObj->createArgs.baseCreateParams.numOutQue; i++)
        {
            pObj->createArgs.baseCreateParams.numChPerOutQue[i] = 
                                              pObj->numCh/pObj->info.numQue;
            if(modVal > 0)
            {
                pObj->createArgs.baseCreateParams.numChPerOutQue[i] += 1;
                modVal--;
            }
        }
    }

    /* assign output channels to output info */
    for (inChId = 0; inChId < pInQueInfo->numCh; inChId++)
    {
        tempVal = 0; queueId = 0; outChId = 0;
        for (i=0; i<pObj->createArgs.baseCreateParams.numOutQue; i++)
        {
            tempVal += pObj->createArgs.baseCreateParams.numChPerOutQue[i];
            if (inChId < tempVal)
            {
                queueId = i;
                outChId = inChId - 
                  (tempVal - pObj->createArgs.baseCreateParams.numChPerOutQue[i]);
                break;
            }
        }

        UTILS_assert(inChId < tempVal);
        /* confirm output que ID is within limits */
        UTILS_assert(i < pObj->info.numQue);

        /* set channels per output que */
        pObj->info.queInfo[queueId].numCh = 
              pObj->createArgs.baseCreateParams.numChPerOutQue[queueId];

        /* copy output CH info from input CH info */
        pObj->info.queInfo[queueId].chInfo[outChId] = pInQueInfo->chInfo[inChId];
    }
}

Int32 IpcFramesOutLink_create(IpcFramesOutLink_Obj * pObj,
                            IpcFramesOutLinkRTOS_CreateParams * pPrm)
{
    Int32 status, elemId;
    System_LinkInQueParams *pInQueParams;
    UInt32 queId;
    IpcFramesOutLink_ChObj *pChObj;
    UInt32 chId;

#ifdef SYSTEM_DEBUG_IPC
    Vps_printf(" %d: IPC_FRAMES_OUT   : Create in progress !!!\n",
               Utils_getCurTimeInMsec());
#endif
    pObj->totalFrameCount = 0;

    UTILS_MEMLOG_USED_START();
    memcpy(&pObj->createArgs, pPrm, sizeof(pObj->createArgs));

    status = System_ipcListMPReset(pObj->listMPOutHndl, pObj->listMPInHndl);
    UTILS_assert(status == FVID2_SOK);

    status = Utils_queCreate(&pObj->listElemQue,
                             SYSTEM_IPC_FRAMES_MAX_LIST_ELEM,
                             pObj->listElemQueMem,
                             UTILS_QUE_FLAG_BLOCK_QUE_GET);
    UTILS_assert(status == FVID2_SOK);

    for (elemId = 0; elemId < SYSTEM_IPC_FRAMES_MAX_LIST_ELEM; elemId++)
    {
        SYSTEM_IPC_FRAMES_SET_BUFOWNERPROCID(pObj->listElem[elemId]->bufState);
        SYSTEM_IPC_FRAMES_SET_BUFSTATE(pObj->listElem[elemId]->bufState,
                                     IPC_FRAMEBUF_STATE_FREE);
        status =
            Utils_quePut(&pObj->listElemQue, pObj->listElem[elemId],
                         BIOS_NO_WAIT);
        UTILS_assert(status == FVID2_SOK);
        pObj->listElem2FrameBufMap[elemId] = NULL;
    }

    /* Info about the NSF link */
    pObj->inQueInfo.numQue = IPC_FRAMESOUT_LINK_MAX_OUT_QUE;

    for (queId = 0; queId < IPC_FRAMESOUT_LINK_MAX_OUT_QUE; queId++)
    {
        status = Utils_queCreate(&pObj->outFrameBufQue[queId],
                             SYSTEM_IPC_FRAMES_MAX_LIST_ELEM,
                             &(pObj->outFrameBufQueMem[queId][0]),
                             UTILS_QUE_FLAG_BLOCK_QUE_GET);
        UTILS_assert(status == FVID2_SOK);

        pObj->inQueInfo.queInfo[queId].numCh = 0;
    }
    pInQueParams = &pObj->createArgs.baseCreateParams.inQueParams;

    status = System_linkGetInfo(pInQueParams->prevLinkId, &pObj->inQueInfo);
    UTILS_assert(status == FVID2_SOK);
    IpcFramesOutLink_setOutQueInfo(pObj);
    IpcFramesOutLink_initStats(pObj);
    IpcFramesOutLink_createPrdObj(pObj);
    IpcFramesOutLink_resetStatistics(pObj);
    for (chId = 0; chId < pObj->numCh; chId++)
    {
        pChObj = &pObj->chObj[chId];

        pChObj->frameSkipCtx.firstTime = TRUE;
        pChObj->frameSkipCtx.inputFrameRate = pObj->createArgs.baseCreateParams.inputFrameRate;
        pChObj->frameSkipCtx.outputFrameRate = pObj->createArgs.baseCreateParams.outputFrameRate;
    }
    UTILS_MEMLOG_USED_END(pObj->memUsed);
    UTILS_MEMLOG_PRINT("IPC_FRAMES_OUT",
                       pObj->memUsed,
                       UTILS_ARRAYSIZE(pObj->memUsed));

#ifdef SYSTEM_DEBUG_IPC
    Vps_printf(" %d: IPC_FRAMES_OUT   : Create Done !!!\n", Utils_getCurTimeInMsec());
#endif

    return FVID2_SOK;
}

Int32 IpcFramesOutLink_delete(IpcFramesOutLink_Obj * pObj)
{
#ifdef SYSTEM_DEBUG_IPC
    Vps_printf(" %d: IPC_FRAMES_OUT   : Delete in progress !!!\n",
               Utils_getCurTimeInMsec());
#endif

    IPCFRAMESOUTLINK_INFO_LOG(pObj->tskId,
                            "RECV:%d\tFORWARD:%d,DROPPED:%d,AVGLATENCY:%d",
                            pObj->stats.recvCount,
                            pObj->stats.forwardCount,
                            pObj->stats.droppedCount,
                  UTILS_DIV(pObj->stats.totalRoundTrip ,
                            pObj->stats.forwardCount));

#ifdef IPC_FRAMES_IN_ENABLE_PROFILE
    /* Print the timestamp differnce and reset the counter */
    Utils_prfTsPrint(pObj->stats.tsHandle, TRUE);
#endif                                                     /* IPC_FRAMES_IN_ENABLE_PROFILE
                                                            */

    Utils_queDelete(&pObj->listElemQue);
    IpcFramesOutLink_deletePrdObj(pObj);

#ifdef SYSTEM_DEBUG_IPC
    Vps_printf(" %d: IPC_FRAMES_OUT   : Delete Done !!!\n", Utils_getCurTimeInMsec());
#endif

    return FVID2_SOK;
}


static UInt32 IpcFrameOutLink_mapDataFormat2NumPlanes(FVID2_DataFormat format)
{
    UInt32 numPlanes = 0;

    switch(format)
    {
        case FVID2_DF_YUV422I_UYVY:
        case FVID2_DF_YUV422I_YUYV:
        case FVID2_DF_YUV422I_YVYU:
        case FVID2_DF_YUV422I_VYUY:
        case FVID2_DF_YUV444I:
        case FVID2_DF_RGB16_565:
        case FVID2_DF_ARGB16_1555:
        case FVID2_DF_RGBA16_5551:
        case FVID2_DF_ARGB16_4444:
        case FVID2_DF_RGBA16_4444:
        case FVID2_DF_ARGB24_6666:
        case FVID2_DF_RGBA24_6666:
        case FVID2_DF_RGB24_888:
        case FVID2_DF_ARGB32_8888:
        case FVID2_DF_RGBA32_8888:
        case FVID2_DF_BGR16_565:
        case FVID2_DF_ABGR16_1555:
        case FVID2_DF_ABGR16_4444:
        case FVID2_DF_BGRA16_5551:
        case FVID2_DF_BGRA16_4444:
        case FVID2_DF_ABGR24_6666:
        case FVID2_DF_BGR24_888:
        case FVID2_DF_ABGR32_8888:
        case FVID2_DF_BGRA24_6666:
        case FVID2_DF_BGRA32_8888:
            numPlanes = 1;
            break;
        case FVID2_DF_YUV422SP_UV:
        case FVID2_DF_YUV422SP_VU:
        case FVID2_DF_YUV420SP_UV:
        case FVID2_DF_YUV420SP_VU:
            numPlanes = 2;
            break;
        case FVID2_DF_YUV422P:
        case FVID2_DF_YUV420P:
        case FVID2_DF_YUV444P:
            numPlanes = 3;
            break;
        default:
            numPlanes = 0;
    }
    UTILS_assert(numPlanes != 0);
    return numPlanes;
}


static Void IpcFramesOutLink_copyFrameInfo(IpcFramesOutLink_Obj * pObj,
                                          FVID2_Frame  *src,
                                          VIDFrame_Buf *dst,
                                          volatile SharedRegion_SRPtr srPtr[VIDFRAME_MAX_FIELDS][VIDFRAME_MAX_PLANES])
{
    Int i;
    UInt32 numPlanes;
    Uint32 fieldIdx;
    System_LinkInQueParams *pInQueParams;
    UInt32 queId;
    System_FrameInfo *frmInfo;

    pInQueParams = &pObj->createArgs.baseCreateParams.inQueParams;
    queId = pInQueParams->prevLinkQueId;
    //UTILS_assert(pObj->inQueInfo.numQue == 1);
    UTILS_assert(src->channelNum <
                 pObj->inQueInfo.queInfo[pInQueParams->prevLinkQueId].numCh);
    UTILS_COMPILETIME_ASSERT(VIDFRAME_MAX_FIELDS == FVID2_MAX_FIELDS);
    UTILS_COMPILETIME_ASSERT(VIDFRAME_MAX_PLANES == FVID2_MAX_PLANES);
    numPlanes =
      IpcFrameOutLink_mapDataFormat2NumPlanes((FVID2_DataFormat)
        pObj->inQueInfo.queInfo[queId].chInfo[src->channelNum].dataFormat);
    for (i = 0; i < numPlanes; i++)
    {
        if (src->fid == FVID2_FID_BOTTOM)
        {
            fieldIdx = 1;
        }
        else
        {
            fieldIdx = 0;
        }
        /* force fieldIdx to zero since always addr[0] is only populated */
        fieldIdx = 0;
        dst->phyAddr[fieldIdx][i] = src->addr[fieldIdx][i];
        /*!WARNING. Assume phyiscalAddr == virtualAddr on RTOS side.
         * This will break once we have mmu on RTOS side
         * TODO: Invoke syslink API on RTOS side to map virt2phy
         */
        if (SYSTEM_MT_TILEDMEM ==
            pObj->inQueInfo.queInfo[queId].chInfo[src->channelNum].memType)
        {
            srPtr[fieldIdx][i] = (SharedRegion_SRPtr)IPC_LINK_INVALID_SRPTR;
        }
        else
        {
            UInt16 srId;

            srId = SharedRegion_getId(src->addr[fieldIdx][i]);
            if(srId != SharedRegion_INVALIDREGIONID)
            {
                srPtr[fieldIdx][i] =
                     SharedRegion_getSRPtr(src->addr[fieldIdx][i], srId);
            }
            else
            {
                /* If frame buffer is not allocated from shared region,
                 * set SrPtr to invalid. Only physical address will be
                 * used in this case
                 */
                srPtr[fieldIdx][i] = (SharedRegion_SRPtr)IPC_LINK_INVALID_SRPTR;
            }
        }
    }
    dst->channelNum = src->channelNum;
    dst->fid        = src->fid;
    dst->timeStamp  = src->timeStamp;
    frmInfo         = src->appData;

    /* HACK HACK HACK !!!
       remove the below line "frmInfo->rtChInfoUpdate = FALSE" to force reading
       from inQueInfo once the previous link support dynamic resolution change
     */
    if (frmInfo != NULL)
        ;//frmInfo->rtChInfoUpdate = FALSE;

    if ((frmInfo == NULL) || (frmInfo->rtChInfoUpdate == FALSE))
    {
        dst->frameWidth =
             pObj->inQueInfo.queInfo[queId].chInfo[src->channelNum].width;
        dst->frameHeight =
             pObj->inQueInfo.queInfo[queId].chInfo[src->channelNum].height;
        dst->framePitch[0] =
             pObj->inQueInfo.queInfo[queId].chInfo[src->channelNum].pitch[0];
        dst->framePitch[1] =
             pObj->inQueInfo.queInfo[queId].chInfo[src->channelNum].pitch[1];
    }
    else
    {
        dst->frameWidth = frmInfo->rtChInfo.width;
        dst->frameHeight = frmInfo->rtChInfo.height;
        dst->framePitch[0] = frmInfo->rtChInfo.pitch[0];
        dst->framePitch[1] = frmInfo->rtChInfo.pitch[1];
    }
}

static
Int32 IpcFramesOutLink_copyFrameBufInfo2ListElem(IpcFramesOutLink_Obj * pObj,
                                             SystemIpcFrames_ListElem * pListElem,
                                             FVID2_Frame * pFrameBuf)
{
    UInt32 listIdx;

    IpcFramesOutLink_copyFrameInfo(pObj, pFrameBuf,
                                   &pListElem->frameBuf,
                                   pListElem->srBufPtr);
    /* No cache ops done since pListElem is allocated from non-cached memory */
    UTILS_assert(SharedRegion_isCacheEnabled(SharedRegion_getId(pListElem)) ==
                 FALSE);
    listIdx = pListElem - pObj->listElem[0];
    UTILS_assert(listIdx < UTILS_ARRAYSIZE(pObj->listElem2FrameBufMap));
    UTILS_assert(pObj->listElem2FrameBufMap[listIdx] == NULL);
    pObj->listElem2FrameBufMap[listIdx] = pFrameBuf;
    pListElem->ipcPrivData = pFrameBuf;
    return IPC_FRAMESOUT_LINK_S_SUCCESS;
}

static
Int32 IpcFramesOutLink_mapListElem2FrameBuf(IpcFramesOutLink_Obj * pObj,
                                        SystemIpcFrames_ListElem * pListElem,
                                        FVID2_Frame ** pFrameBufPtr)
{
    UInt32 listIdx;

    *pFrameBufPtr = NULL;
    listIdx = pListElem - pObj->listElem[0];
    UTILS_assert(listIdx < UTILS_ARRAYSIZE(pObj->listElem2FrameBufMap));
    UTILS_assert(pObj->listElem2FrameBufMap[listIdx] != NULL);
    *pFrameBufPtr = pObj->listElem2FrameBufMap[listIdx];
    UTILS_assert(((Ptr) (*pFrameBufPtr)) == pListElem->ipcPrivData);
    pObj->listElem2FrameBufMap[listIdx] = NULL;
    return IPC_FRAMESOUT_LINK_S_SUCCESS;
}

Int32 IpcFramesOutLink_processFrameBufs(IpcFramesOutLink_Obj * pObj)
{
    System_LinkInQueParams *pInQueParams;
    FVID2_FrameList bufList;
    FVID2_Frame *pFrameBuf = NULL;
    SystemIpcFrames_ListElem *pListElem;
    Int32 status;
    Int32 bufId;
    UInt32 curTime, tempVal;
    FVID2_FrameList freeFrameBufList;
    UInt8 queId, i;
    UInt32 sendMsgToTsk = 0;
    IpcFramesOutLink_ChObj *pChObj;

    pInQueParams = &pObj->createArgs.baseCreateParams.inQueParams;

    bufList.numFrames = 0;
    System_getLinksFullFrames(pInQueParams->prevLinkId,
                            pInQueParams->prevLinkQueId, &bufList);

    freeFrameBufList.numFrames = 0;
    curTime = Utils_getCurTimeInMsec();
    if (bufList.numFrames)
    {
#ifdef SYSTEM_DEBUG_IPC_RT
        Vps_printf(" %d: IPC_FRAMES_OUT   : Received %d framebufs !!!\n",
                   Utils_getCurTimeInMsec(), bufList.numFrames);
#endif

        UTILS_assert(bufList.numFrames <= FVID2_MAX_FVID_FRAME_PTR);
        pObj->stats.recvCount += bufList.numFrames;
        sendMsgToTsk = 0;

#ifdef IPC_FRAMES_IN_ENABLE_PROFILE
        Utils_prfTsBegin(pObj->stats.tsHandle);
#endif                                                     /* IPC_FRAMES_IN_ENABLE_PROFILE
                                                            */
        pObj->totalFrameCount += bufList.numFrames;
        for (bufId = 0; bufId < bufList.numFrames; bufId++)
        {
            Bool          doFrameDrop;

            pFrameBuf = bufList.frames[bufId];
            UTILS_assert(pFrameBuf != NULL);

            pChObj = &pObj->chObj[pFrameBuf->channelNum];
            pChObj->inFrameRecvCount++;
            doFrameDrop = Utils_doSkipFrame(&(pChObj->frameSkipCtx));

            /* frame skipped due to user setting */
            if(doFrameDrop)
            {
                pChObj->inFrameUserSkipCount++;
                UTILS_assert(freeFrameBufList.numFrames <
                             FVID2_MAX_FVID_FRAME_PTR);
                freeFrameBufList.frames[freeFrameBufList.numFrames] =
                    pFrameBuf;
                freeFrameBufList.numFrames++;
                pObj->stats.droppedCount++;
                pObj->stats.releaseCount++;
                continue;
            }

            tempVal = 0; queId = 0;
            for (i=0; i<pObj->createArgs.baseCreateParams.numOutQue; i++)
            {
                tempVal += pObj->createArgs.baseCreateParams.numChPerOutQue[i];
                if (pFrameBuf->channelNum < tempVal)
                {
                    queId = i;
                    break;
                }
            }
            /* confirm output que ID is within limits */
            UTILS_assert(i < pObj->createArgs.baseCreateParams.numOutQue);
            
            status =
                Utils_queGet(&pObj->listElemQue, (Ptr *) & pListElem, 1,
                             BIOS_NO_WAIT);
            UTILS_assert(!UTILS_ISERROR(status));
            if (status != FVID2_SOK)
            {
                /* normally this condition should not happen, if it happens
                 * return the framebuf back to its generator */
#if 0
                Vps_printf(" IPC_OUT: Dropping framebuf\n");
#endif

                UTILS_assert(freeFrameBufList.numFrames <
                             FVID2_MAX_FVID_FRAME_PTR);
                freeFrameBufList.frames[freeFrameBufList.numFrames] =
                    pFrameBuf;
                freeFrameBufList.numFrames++;
                pObj->stats.droppedCount++;
                pChObj->inFrameUserSkipCount++;
                continue;
            }
            UTILS_assert(SYSTEM_IPC_FRAMES_GET_BUFSTATE(pListElem->bufState)
                         == IPC_FRAMEBUF_STATE_FREE);
            UTILS_assert(SYSTEM_IPC_FRAMES_GET_BUFOWNERPROCID(pListElem->bufState)
                         == System_getSelfProcId());
            SYSTEM_IPC_FRAMES_SET_BUFSTATE(pListElem->bufState,
                                         IPC_FRAMEBUF_STATE_ALLOCED);
            IpcFramesOutLink_copyFrameBufInfo2ListElem(pObj, pListElem, pFrameBuf);
            pFrameBuf->timeStamp = curTime;
            SYSTEM_IPC_FRAMES_SET_BUFSTATE(pListElem->bufState,
                                         IPC_FRAMEBUF_STATE_OUTQUE);
            sendMsgToTsk |= (1 << queId);
            status =
                ListMP_putTail(pObj->listMPOutHndl, (ListMP_Elem *) pListElem);
            UTILS_assert(status == ListMP_S_SUCCESS);
            pChObj->inFrameProcessCount++;
        }

#ifdef IPC_FRAMES_IN_ENABLE_PROFILE
        Utils_prfTsEnd(pObj->stats.tsHandle, bufList.numFrames);
#endif                                                     /* IPC_FRAMES_IN_ENABLE_PROFILE
                                                            */

        if (freeFrameBufList.numFrames)
        {
            System_putLinksEmptyFrames(pInQueParams->prevLinkId,
                                     pInQueParams->prevLinkQueId,
                                     &freeFrameBufList);
        }

        /* ProcessLink enable, send the notification to processLink else send to next Link */
        if (pObj->createArgs.baseCreateParams.processLink != SYSTEM_LINK_ID_INVALID)
        {
            if (pObj->createArgs.baseCreateParams.notifyProcessLink)
            {
                System_ipcSendNotify(pObj->createArgs.baseCreateParams.processLink);
            }
        }
        else
        {
            for (queId = 0; queId < pObj->createArgs.baseCreateParams.numOutQue; queId++)
            {
                if ((pObj->createArgs.baseCreateParams.notifyNextLink) && (sendMsgToTsk & 0x1))
                {
                    System_ipcSendNotify(pObj->createArgs.baseCreateParams.outQueParams[queId].
                        nextLink);
                }
                sendMsgToTsk >>= 1;
                if (sendMsgToTsk == 0)
                    break;
            }
        }

        if (pObj->createArgs.baseCreateParams.noNotifyMode)
        {
            if (FALSE == pObj->prd.clkStarted)
            {
                IpcFramesOutLink_startPrdObj(pObj,
                                           IPC_FRAMESOUT_LINK_DONE_PERIOD_MS,
                                           FALSE);
            }
        }
    }

    return FVID2_SOK;
}

Int32 IpcFramesOutLink_releaseFrameBufs(IpcFramesOutLink_Obj * pObj)
{
    System_LinkInQueParams *pInQueParams;
    SystemIpcFrames_ListElem *pListElem = NULL;
    Int32 status;
    UInt32 curTime, roundTripTime;
    FVID2_Frame *pFrameBuf = NULL;
    FVID2_FrameList freeFrameBufList;
    UInt8 queId;
    UInt32 tempVal;

    pInQueParams = &pObj->createArgs.baseCreateParams.inQueParams;

    do
    {
        freeFrameBufList.numFrames = 0;

        curTime = Utils_getCurTimeInMsec();
        while (freeFrameBufList.numFrames < FVID2_MAX_FVID_FRAME_PTR)
        {
            pListElem = ListMP_getHead(pObj->listMPInHndl);
            if (pListElem == NULL)
                break;
            UTILS_assert(SYSTEM_IPC_FRAMES_GET_BUFSTATE(pListElem->bufState)
                         == IPC_FRAMEBUF_STATE_INQUE);
            IpcFramesOutLink_mapListElem2FrameBuf(pObj, pListElem, &pFrameBuf);
            UTILS_assert(pFrameBuf != NULL);
            if (curTime > pFrameBuf->timeStamp)
            {
                roundTripTime = curTime - pFrameBuf->timeStamp;
                pObj->stats.totalRoundTrip += roundTripTime;
            }
            /* Restore the original timestamp as it may be used by next link */
            pFrameBuf->timeStamp = pListElem->frameBuf.timeStamp;
            freeFrameBufList.frames[freeFrameBufList.numFrames] = pFrameBuf;
            freeFrameBufList.numFrames++;
            UTILS_assert(freeFrameBufList.numFrames <=
                         FVID2_MAX_FVID_FRAME_PTR);
            /* release ListElem back to queue */
            SYSTEM_IPC_FRAMES_SET_BUFOWNERPROCID(pListElem->bufState);
            SYSTEM_IPC_FRAMES_SET_BUFSTATE(pListElem->bufState,
                                         IPC_FRAMEBUF_STATE_FREE);
            status = Utils_quePut(&pObj->listElemQue, pListElem, BIOS_NO_WAIT);
            UTILS_assert(status == FVID2_SOK);
        }

#ifdef SYSTEM_DEBUG_IPC_RT
        Vps_printf(" %d: IPC_FRAMES_OUT   : Releasing %d framebufs !!!\n",
                   Utils_getCurTimeInMsec(), freeFrameBufList.numFrames);
#endif

        if (freeFrameBufList.numFrames)
        {

            /* If the buffer is released via processing link, pass it to next link */
            /* NextLink will take care to release the buffer to privLink */
            if(pObj->createArgs.baseCreateParams.processLink != SYSTEM_LINK_ID_INVALID)
            {
                Int i, j;
                UInt32 sendMsgToTsk = 0;

                for (i = 0; i < freeFrameBufList.numFrames;i++)
                {
                    pObj->stats.forwardCount++;

                    /** Split the frames into both the output queues,
                    * if they are enabled. Else use output queue 0 only.
                    * Also, if output queue 1 is used, frames sent to this queue
                    * should be modified before submitting so that the
                    * pFrame->channelNum should start with 0 and not with
                    * (pObj->nsfCreateParams.numCh / 2).
                    */
                    pFrameBuf = freeFrameBufList.frames[i];

                    tempVal = 0; queId = 0;
                    for (j=0; j<pObj->createArgs.baseCreateParams.numOutQue; j++)
                    {
                        tempVal += pObj->createArgs.baseCreateParams.numChPerOutQue[j];
                        if (pFrameBuf->channelNum < tempVal)
                        {
                            queId = j;
                            pFrameBuf->channelNum = pFrameBuf->channelNum - 
                              (tempVal - pObj->createArgs.baseCreateParams.numChPerOutQue[j]);
                            break;
                        }
                    }
                    /* confirm output que ID is within limits */
                    UTILS_assert(j < pObj->createArgs.baseCreateParams.numOutQue);

                    status =  Utils_quePut(&pObj->outFrameBufQue[queId],
                                           freeFrameBufList.frames[i],
                                           BIOS_NO_WAIT);
                    UTILS_assert(!UTILS_ISERROR(status));
                    sendMsgToTsk |= (1 << queId);
                }
                if (pObj->createArgs.baseCreateParams.notifyNextLink)
                {
                    for (i = 0; i < pObj->createArgs.baseCreateParams.numOutQue;i++)
                    {
                        if (sendMsgToTsk & 0x1)
                        {
                            System_sendLinkCmd(pObj->createArgs.baseCreateParams.outQueParams[i].
                                nextLink, SYSTEM_CMD_NEW_DATA);
                        }
                        sendMsgToTsk >>= 1;
                        if (sendMsgToTsk == 0)
                            break;
                    }
                }
            }
            else
            {
                System_putLinksEmptyFrames(pInQueParams->prevLinkId,
                                           pInQueParams->prevLinkQueId,
                                           &freeFrameBufList);
            }
        }
    } while (pListElem != NULL);

    return FVID2_SOK;
}

Int32 IpcFramesOutLink_getLinkInfo(Utils_TskHndl * pTsk, System_LinkInfo * info)
{
    IpcFramesOutLink_Obj *pObj = (IpcFramesOutLink_Obj *) pTsk->appData;


    memcpy(info, &pObj->info, sizeof(*info));

    return FVID2_SOK;
}

Void IpcFramesOutLink_tskMain(struct Utils_TskHndl * pTsk, Utils_MsgHndl * pMsg)
{
    UInt32 cmd = Utils_msgGetCmd(pMsg);
    Bool ackMsg, done;
    Int32 status;
    IpcFramesOutLink_Obj *pObj = (IpcFramesOutLink_Obj *) pTsk->appData;

    if (cmd != SYSTEM_CMD_CREATE)
    {
        Utils_tskAckOrFreeMsg(pMsg, FVID2_EFAIL);
        return;
    }

    status = IpcFramesOutLink_create(pObj, Utils_msgGetPrm(pMsg));

    Utils_tskAckOrFreeMsg(pMsg, status);

    if (status != FVID2_SOK)
        return;

    done = FALSE;
    ackMsg = FALSE;

    while (!done)
    {
        status = Utils_tskRecvMsg(pTsk, &pMsg, BIOS_WAIT_FOREVER);
        if (status != FVID2_SOK)
            break;

        cmd = Utils_msgGetCmd(pMsg);

        switch (cmd)
        {
            case SYSTEM_CMD_DELETE:
                done = TRUE;
                ackMsg = TRUE;
                break;
            case SYSTEM_CMD_NEW_DATA:
                Utils_tskAckOrFreeMsg(pMsg, status);

                IpcFramesOutLink_processFrameBufs(pObj);
                IpcFramesOutLink_releaseFrameBufs(pObj);
                break;

            case IPCFRAMESOUTRTOS_LINK_CMD_SET_FRAME_RATE:
                {
                    IpcOutM3Link_ChFpsParams *params;

                    params = (IpcOutM3Link_ChFpsParams *) Utils_msgGetPrm(pMsg);
                    IpcFramesOutLink_SetFrameRate(pObj, params);
                    Utils_tskAckOrFreeMsg(pMsg, status);
                }
                break;

            case IPCFRAMESOUTRTOS_LINK_CMD_PRINT_STATISTICS:
                IpcFramesOutLink_printStatistics(pObj, TRUE);
                Utils_tskAckOrFreeMsg(pMsg, status);
                break;


            case SYSTEM_IPC_CMD_RELEASE_FRAMES:
                Utils_tskAckOrFreeMsg(pMsg, status);

#ifdef SYSTEM_DEBUG_IPC_RT
                Vps_printf(" %d: IPC_FRAMES_OUT   : Received Notify !!!\n",
                           Utils_getCurTimeInMsec());
#endif

                IpcFramesOutLink_releaseFrameBufs(pObj);
                break;

            default:
                Utils_tskAckOrFreeMsg(pMsg, status);
                break;
        }
    }

    IpcFramesOutLink_delete(pObj);

#ifdef SYSTEM_DEBUG_IPC_FRAMES_OUT
    Vps_printf(" %d: IPC_FRAMES_OUT   : Delete Done !!!\n", Utils_getCurTimeInMsec());
#endif

    if (ackMsg && pMsg != NULL)
        Utils_tskAckOrFreeMsg(pMsg, status);

    return;
}

Int32 IpcFramesOutLink_allocListElem(IpcFramesOutLink_Obj * pObj)
{
    UInt32 shAddr;
    UInt32 elemId;

    shAddr = System_ipcListMPAllocListElemMem(SYSTEM_IPC_SR_NON_CACHED_DEFAULT,
                                              sizeof(SystemIpcFrames_ListElem) *
                                              SYSTEM_IPC_FRAMES_MAX_LIST_ELEM);

    for (elemId = 0; elemId < SYSTEM_IPC_FRAMES_MAX_LIST_ELEM; elemId++)
    {
        pObj->listElem[elemId] =
            (SystemIpcFrames_ListElem *) (shAddr +
                                        elemId *
                                        sizeof(SystemIpcFrames_ListElem));
    }

    return FVID2_SOK;
}

Int32 IpcFramesOutLink_freeListElem(IpcFramesOutLink_Obj * pObj)
{
    System_ipcListMPFreeListElemMem(SYSTEM_IPC_SR_NON_CACHED_DEFAULT,
                                    (UInt32) pObj->listElem[0],
                                    sizeof(SystemIpcFrames_ListElem) *
                                    SYSTEM_IPC_FRAMES_MAX_LIST_ELEM);

    return FVID2_SOK;
}

Int32 IpcFramesOutLink_initListMP(IpcFramesOutLink_Obj * pObj)
{
    Int32 status;

    status = System_ipcListMPCreate(SYSTEM_IPC_SR_NON_CACHED_DEFAULT,
                                    pObj->tskId,
                                    &pObj->listMPOutHndl, &pObj->listMPInHndl,
                                    &pObj->gateMPOutHndl, &pObj->gateMPInHndl);
    UTILS_assert(status == FVID2_SOK);

    IpcFramesOutLink_allocListElem(pObj);

    return status;
}

Int32 IpcFramesOutLink_deInitListMP(IpcFramesOutLink_Obj * pObj)
{
    Int32 status;

    status = System_ipcListMPDelete(&pObj->listMPOutHndl, &pObj->listMPInHndl,
                                    &pObj->gateMPOutHndl, &pObj->gateMPInHndl);
    UTILS_assert(status == FVID2_SOK);

    IpcFramesOutLink_freeListElem(pObj);

    return status;
}

Int32 IpcFramesOutLink_init()
{
    Int32 status;
    System_LinkObj linkObj;
    UInt32 ipcFramesOutId;
    IpcFramesOutLink_Obj *pObj;
    char tskName[32];
    UInt32 procId = System_getSelfProcId();

    //UTILS_COMPILETIME_ASSERT(offsetof(SystemIpcFrames_ListElem, frameBuf) == 0);
    UTILS_COMPILETIME_ASSERT(offsetof(VIDFrame_Buf, reserved) == 0);
    UTILS_COMPILETIME_ASSERT(offsetof(SystemIpcFrames_ListElem, frameBuf) == 0);
    UTILS_COMPILETIME_ASSERT(sizeof(((SystemIpcFrames_ListElem *) 0)->frameBuf.reserved) ==
                             sizeof(ListMP_Elem));
    for (ipcFramesOutId = 0; ipcFramesOutId < IPC_FRAMES_OUT_LINK_OBJ_MAX;
         ipcFramesOutId++)
    {
        pObj = &gIpcFramesOutLink_obj[ipcFramesOutId];

        memset(pObj, 0, sizeof(*pObj));

        pObj->tskId =
            SYSTEM_MAKE_LINK_ID(procId,
                                SYSTEM_LINK_ID_IPC_FRAMES_OUT_0) + ipcFramesOutId;

        linkObj.pTsk = &pObj->tsk;
        linkObj.linkGetFullFrames = IpcFramesOutLink_getFullFrames;
        linkObj.linkPutEmptyFrames = IpcFramesOutLink_putEmptyFrames;
        linkObj.getLinkInfo = IpcFramesOutLink_getLinkInfo;

        System_registerLink(pObj->tskId, &linkObj);

        UTILS_SNPRINTF(tskName, "IPC_FRAMES_OUT%d", ipcFramesOutId);

        System_ipcRegisterNotifyCb(pObj->tskId, IpcFramesOutLink_notifyCb);

        IpcFramesOutLink_initListMP(pObj);

        pObj->stats.tsHandle = Utils_prfTsCreate(tskName);

        UTILS_assert(pObj->stats.tsHandle != NULL);

        status = Utils_tskCreate(&pObj->tsk,
                                 IpcFramesOutLink_tskMain,
                                 IPC_LINK_TSK_PRI,
                                 gIpcFramesOutLink_tskStack[ipcFramesOutId],
                                 IPC_LINK_TSK_STACK_SIZE, pObj, tskName);
        UTILS_assert(status == FVID2_SOK);
    }

    return status;
}

Int32 IpcFramesOutLink_deInit()
{
    UInt32 ipcFramesOutId;
    IpcFramesOutLink_Obj *pObj;

    for (ipcFramesOutId = 0; ipcFramesOutId < IPC_FRAMES_OUT_LINK_OBJ_MAX;
         ipcFramesOutId++)
    {
        pObj = &gIpcFramesOutLink_obj[ipcFramesOutId];

        Utils_tskDelete(&pObj->tsk);

        IpcFramesOutLink_deInitListMP(pObj);

        UTILS_assert(pObj->stats.tsHandle != NULL);
        Utils_prfTsDelete(pObj->stats.tsHandle);
    }
    return FVID2_SOK;
}

Int32 IpcFramesOutLink_getFullFrames(Utils_TskHndl * pTsk, UInt16 queId,
                                     FVID2_FrameList * pFrameBufList)
{
    IpcFramesOutLink_Obj *pObj = (IpcFramesOutLink_Obj *) pTsk->appData;
    UInt32 idx;
    Int32 status;

    for (idx = 0; idx < FVID2_MAX_FVID_FRAME_PTR; idx++)
    {
        status =
            Utils_queGet(&pObj->outFrameBufQue[queId], (Ptr *) & pFrameBufList->frames[idx],
                         1, BIOS_NO_WAIT);
        if (status != FVID2_SOK)
            break;
    }

    pFrameBufList->numFrames = idx;

    return IPC_FRAMESOUT_LINK_S_SUCCESS;
}

Int32 IpcFramesOutLink_putEmptyFrames(Utils_TskHndl * pTsk, UInt16 queId,
                                      FVID2_FrameList * pFrameBufList)
{
    IpcFramesOutLink_Obj *pObj = (IpcFramesOutLink_Obj *) pTsk->appData;
    System_LinkInQueParams *pInQueParams;

#ifdef SYSTEM_DEBUG_IPC_RT
    Vps_printf(" %d: IPC_FRAMES_OUT   : Releasing %d framebufs !!!\n",
               Utils_getCurTimeInMsec(), pFrameBufList->numFrames);
#endif

    if (pFrameBufList->numFrames)
    {
        Int i;
        UInt32 tempVal;

        tempVal = 0;
        UTILS_assert(queId < pObj->createArgs.baseCreateParams.numOutQue);
        for (i=0; i<(queId+1); i++)
        {
            tempVal += pObj->createArgs.baseCreateParams.numChPerOutQue[i];
        }

        UTILS_assert(pFrameBufList->numFrames <= FVID2_MAX_FVID_FRAME_PTR);
        for (i = 0; i < pFrameBufList->numFrames;i++)
        {
            FVID2_Frame *pFrameBuf = pFrameBufList->frames[i];
            pFrameBuf->channelNum +=
              (tempVal - pObj->createArgs.baseCreateParams.numChPerOutQue[queId]);
        }
        pObj->stats.releaseCount += pFrameBufList->numFrames;
        pInQueParams = &pObj->createArgs.baseCreateParams.inQueParams;
        System_putLinksEmptyFrames(pInQueParams->prevLinkId,
                                   pInQueParams->prevLinkQueId,
                                   pFrameBufList);
    }
    return IPC_FRAMESOUT_LINK_S_SUCCESS;
}

