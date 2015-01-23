/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/
#include <stddef.h>
#include "ipcFramesInLink_priv.h"
#include <ti/syslink/utils/IHeap.h>
#include <ti/syslink/utils/Memory.h>
#include <ti/syslink/utils/Cache.h>

#include <mcfw/interfaces/common_def/ti_vdis_common_def.h>


IpcFramesInLink_Obj gIpcFramesInLink_obj[IPC_FRAMES_IN_LINK_OBJ_MAX];

static
Void IpcFramesInLink_notifyCb(OSA_TskHndl * pTsk)
{
    Int32 status;
    IpcFramesInLink_Obj *pObj = (IpcFramesInLink_Obj *) pTsk->appData;

    if (pObj->prd.numPendingCmd < IPC_FRAMES_IN_MAX_PENDING_NEWDATA_CMDS)
    {
        pObj->prd.numPendingCmd++;
        status = OSA_tskSendMsg(pTsk, NULL, SYSTEM_CMD_NEW_DATA, NULL, 0);
    }
    else
    {
        UInt32 curTime = OSA_getCurTimeInMsec();

        OSA_printf("IPC_FRAMESINLINK:!WARNING!.Commands not being processed by link."
                   "TimeSinceLastAlloc:%d,TimeSinceLastFree:%d",
                  (curTime - pObj->stats.lastGetBufTime),
                  (curTime - pObj->stats.lastFreeBufTime));
    }
}

static Void IpcFramesInLink_initStats(IpcFramesInLink_Obj * pObj)
{
    memset(&pObj->stats, 0, sizeof(pObj->stats));
}

static
Void *IpcFramesInLink_periodicTaskFxn(Void * prm)
{
    IpcFramesInLink_Obj *pObj = (IpcFramesInLink_Obj *) prm;
    Int32 status;

    while (FALSE == pObj->prd.exitThread)
    {
        OSA_waitMsecs(IPC_FRAMES_IN_LINK_PROCESS_PERIOD_MS);
        if (pObj->prd.numPendingCmd < IPC_FRAMES_IN_MAX_PENDING_NEWDATA_CMDS)
        {
            pObj->prd.numPendingCmd++;
            status = System_sendLinkCmd(pObj->tskId, SYSTEM_CMD_NEW_DATA);
        }
        else
        {
            UInt32 curTime = OSA_getCurTimeInMsec();

            OSA_printf("IPC_FRAMESINLINK:!WARNING!.Commands not being processed by link."
                       "TimeSinceLastAlloc:%d,TimeSinceLastFree:%d",
                       (curTime - pObj->stats.lastGetBufTime),
                       (curTime - pObj->stats.lastFreeBufTime));
        }
    }
    return NULL;
}


static Int32 IpcFramesInLink_createPrdObj(IpcFramesInLink_Obj * pObj)
{
    pObj->prd.numPendingCmd = 0;
    pObj->prd.exitThread = FALSE;
    OSA_thrCreate(&pObj->prd.thrHandle,
                  IpcFramesInLink_periodicTaskFxn,
                  IPC_LINK_TSK_PRI, IPC_LINK_TSK_STACK_SIZE, pObj);
    return IPC_FRAMES_IN_LINK_S_SUCCESS;
}

static
Int32 IpcFramesInLink_deletePrdObj(IpcFramesInLink_Obj * pObj)
{
    pObj->prd.exitThread = TRUE;
    OSA_thrDelete(&pObj->prd.thrHandle);
    pObj->prd.numPendingCmd = 0;
    return IPC_FRAMES_IN_LINK_S_SUCCESS;
}


static
Void IpcFramesInLink_setOutQueInfo(IpcFramesInLink_Obj *pObj)
{
    UInt32 totalNumInputChannels = 0;
    Int i;
    UInt32 chPerQueue,chId;
    UInt32 outChId,queueId;
    UInt32 inChId,inQueueId;

    for (i = 0; i < pObj->inQueInfo.numQue;i++)
    {
        totalNumInputChannels += pObj->inQueInfo.queInfo[i].numCh;
    }
    chPerQueue = (totalNumInputChannels / pObj->createArgs.baseCreateParams.numOutQue);
    pObj->info.numQue = pObj->createArgs.baseCreateParams.numOutQue;

    inQueueId = 0;
    inChId    = 0;
    for (chId = 0; chId < totalNumInputChannels;chId++)
    {
        outChId = chId % chPerQueue;
        queueId = chId / chPerQueue;
        OSA_assert(queueId < pObj->info.numQue);

        pObj->info.queInfo[queueId].numCh = chPerQueue;
        OSA_assert((inQueueId < pObj->inQueInfo.numQue)
                      &&
                      (inChId   < pObj->inQueInfo.queInfo[inQueueId].numCh));
        pObj->info.queInfo[queueId].chInfo[outChId] =
            pObj->inQueInfo.queInfo[inQueueId].chInfo[inChId];
        inChId++;
        if (inChId >= pObj->inQueInfo.queInfo[inQueueId].numCh)
        {
            inChId = 0;
            inQueueId++;
        }
    }
}

static
Void  IpcFramesInLink_phyAddrMapTblCreate(IpcFramesInLink_Obj * pObj)
{
    Int status;
    Int i;
    List_Params prm;

    status =
        OSA_queCreate(&pObj->phyAddrMap.mapEntryFreeQ,
                      IPC_FRAMES_IN_MAX_UNIQUE_BUFADDR);
    OSA_assert(OSA_SOK == status);
    for (i = 0; i < OSA_ARRAYSIZE(pObj->phyAddrMap.mapEntryMem);i++)
    {
        status = OSA_quePut(&pObj->phyAddrMap.mapEntryFreeQ,
                            (Int32)(&pObj->phyAddrMap.mapEntryMem[i]),
                            OSA_TIMEOUT_NONE);
        OSA_assert(OSA_SOK == status);
    }
    List_Params_init(&prm);
    List_construct(&pObj->phyAddrMap.head,&prm);
}

static
Void  IpcFramesInLink_phyAddrMap (IpcFramesInLink_phyAddrMapEntry *mapEntry,
                                  UInt32                phyAddr,
                                  UInt32                len)
{
    Memory_MapInfo mapInfo;
    Int status;

    mapEntry->actualAddress = phyAddr;
    mapEntry->size          = len;
    mapInfo.src  = mapEntry->actualAddress;
    mapInfo.size = mapEntry->size;
    mapInfo.isCached = FALSE;
    status = Memory_map (&mapInfo);
    if (status < 0)
    {
        OSA_printf("IPC_FRAMES_IN_LINK:!!!WARN.Memory map failed for address:0x%x",
                   mapEntry->actualAddress);
        mapEntry->mappedAddress = (UInt32)NULL;
    }
    else
    {
        mapEntry->mappedAddress = mapInfo.dst;
        OSA_printf("IPC_FRAMES_IN_LINK:Memory map physical address:0x%x to vitual address:0x%x",
                   mapEntry->actualAddress, mapEntry->mappedAddress);
    }
}

static
Void  IpcFramesInLink_phyAddrUnMap (IpcFramesInLink_phyAddrMapEntry *mapEntry)
{
    Memory_UnmapInfo    unmapInfo;
    Int status;

    unmapInfo.addr  = mapEntry->mappedAddress;
    unmapInfo.size = mapEntry->size;
    unmapInfo.isCached = FALSE;
    status = Memory_unmap (&unmapInfo);
    if (status < 0)
    {
        OSA_printf("IPC_FRAMES_IN_LINK:!!!WARN.Memory unmap failed for address:0x%x",
                   mapEntry->mappedAddress);
    }
}


static
IpcFramesInLink_phyAddrMapEntry * IpcFramesInLink_phyAddrAddEntry(
                                       IpcFramesInLink_Obj * pObj,
                                       UInt32                phyAddr,
                                       UInt32                len)
{
    Int status;
    IpcFramesInLink_phyAddrMapEntry *mapEntry;

    status = OSA_queGet(&pObj->phyAddrMap.mapEntryFreeQ,
                        (Int32 *)&mapEntry,
                        OSA_TIMEOUT_NONE);
    OSA_assert(OSA_SOK == status);
    /* Initialize the list element */
    List_elemClear ((List_Elem *) mapEntry);
    IpcFramesInLink_phyAddrMap(mapEntry,phyAddr,len);
    OSA_assert(mapEntry->mappedAddress != (UInt32)NULL);
    pObj->phyAddrMap.numMappedEntries++;
    pObj->phyAddrMap.totalMapSize += mapEntry->size;
    List_put(&pObj->phyAddrMap.head,(List_Elem *) mapEntry);
    return (mapEntry);
}


static
Void IpcFramesInLink_phyAddrRemoveEntry(IpcFramesInLink_Obj * pObj,
                                        IpcFramesInLink_phyAddrMapEntry *mapEntry)
{
    Int status;

    IpcFramesInLink_phyAddrUnMap(mapEntry);

    pObj->phyAddrMap.numMappedEntries--;
    pObj->phyAddrMap.totalMapSize -= mapEntry->size;
    status = OSA_quePut(&pObj->phyAddrMap.mapEntryFreeQ,
                        (Int32 )mapEntry,
                        OSA_TIMEOUT_NONE);
    OSA_assert(OSA_SOK == status);
}


static
Void  IpcFramesInLink_phyAddrMapTblDelete(IpcFramesInLink_Obj * pObj)
{
    Int                     status;
    IpcFramesInLink_phyAddrMapEntry *mapEntry;

    IPCFRAMESINLINK_INFO_LOG(pObj->tskId,"Map of Frame buffers..."
                                         "Count = %d,Size = %d",
                                         pObj->phyAddrMap.numMappedEntries,
                                         pObj->phyAddrMap.totalMapSize);
    while(!List_empty(&pObj->phyAddrMap.head))
    {
        mapEntry = List_get(&pObj->phyAddrMap.head);
        IpcFramesInLink_phyAddrRemoveEntry(pObj,
                                           mapEntry);
    }
    List_destruct(&pObj->phyAddrMap.head);
    status =
        OSA_queDelete(&pObj->phyAddrMap.mapEntryFreeQ);
    OSA_assert(OSA_SOK == status);
    IPCFRAMESINLINK_INFO_LOG(pObj->tskId,"UnMap of Frame buffers..."
                                         "Count = %d,Size = %d",
                                         pObj->phyAddrMap.numMappedEntries,
                                         pObj->phyAddrMap.totalMapSize);
}

static
IpcFramesInLink_phyAddrMapEntry *  IpcFramesInLink_phyAddrMapTblGetEntry(
                                            IpcFramesInLink_Obj * pObj,
                                            UInt32                phyAddr)
{
    List_Elem *             listInfo;
    IpcFramesInLink_phyAddrMapEntry *mapEntry = NULL;
    IpcFramesInLink_phyAddrMapEntry *retEntry = NULL;

    List_traverse(listInfo,&pObj->phyAddrMap.head){
        mapEntry = (IpcFramesInLink_phyAddrMapEntry *)listInfo;
        if (mapEntry->actualAddress == phyAddr)
        {
            retEntry = mapEntry;
            break;
        }
    }
    return retEntry;
}

static
UInt32 IpcFramesInLink_phyAddr2VirtAddr(IpcFramesInLink_Obj * pObj,
                                      UInt32                phyAddr,
                                      UInt32                len)
{
    IpcFramesInLink_phyAddrMapEntry *mappedEntry = NULL;

    mappedEntry = IpcFramesInLink_phyAddrMapTblGetEntry(pObj,phyAddr);
    if (mappedEntry == NULL)
    {
        mappedEntry =  IpcFramesInLink_phyAddrAddEntry(pObj,phyAddr,len);
    }
    OSA_assert(mappedEntry != NULL);
    return  mappedEntry->mappedAddress;
}

static
Int32 IpcFramesInLink_create(IpcFramesInLink_Obj * pObj,
                           IpcFramesInLinkRTOS_CreateParams * pPrm)
{
    Int32 status;
    System_LinkInQueParams *pInQueParams;

#ifdef SYSTEM_DEBUG_IPC
    OSA_printf(" %d: IPC_FRAMES_IN   : Create in progress !!!\n",
               OSA_getCurTimeInMsec());
#endif

    memcpy(&pObj->createArgs, pPrm, sizeof(pObj->createArgs));

    status =
        System_ipcListMPOpen(pObj->createArgs.baseCreateParams.inQueParams.
                             prevLinkId, &pObj->listMPOutHndl,
                             &pObj->listMPInHndl);
    OSA_assert(status == OSA_SOK);
    OSA_assert(pObj->listMPInHndl != NULL);

    while(!ListMP_empty(pObj->listMPInHndl))
    {
        Ptr listElem =
        ListMP_getHead(pObj->listMPInHndl);
        if (listElem != NULL)
        {
            IPCFRAMESINLINK_INFO_LOG(pObj->tskId,
                                   "InList not empty!!!"
                                   "Stale entry:[%p]",
                                   listElem);
        }
    }


    status = OSA_queCreate(&pObj->outFrameBufQue,
                           SYSTEM_IPC_FRAMES_MAX_LIST_ELEM);
    OSA_assert(status == OSA_SOK);

    pInQueParams = &pObj->createArgs.baseCreateParams.inQueParams;

    status = System_linkGetInfo(pInQueParams->prevLinkId, &pObj->inQueInfo);
    OSA_assert(status == OSA_SOK);

    IpcFramesInLink_setOutQueInfo(pObj);
    IpcFramesInLink_initStats(pObj);
    IpcFramesInLink_phyAddrMapTblCreate(pObj);

    if (TRUE == pObj->createArgs.baseCreateParams.noNotifyMode)
    {
        IpcFramesInLink_createPrdObj(pObj);
    }
    pObj->state = IPC_FRAMES_IN_LINK_STATE_ACTIVE;

#ifdef SYSTEM_DEBUG_IPC
    OSA_printf(" %d: IPC_FRAMES_IN   : Create Done !!!\n", OSA_getCurTimeInMsec());
#endif

    return IPC_FRAMES_IN_LINK_S_SUCCESS;
}

static
Int32 IpcFramesInLink_freeQueuedInputBufs(IpcFramesInLink_Obj * pObj)
{
    SystemIpcFrames_ListElem *pListElem;
    Int32 status;
    UInt32 freeCount;

    freeCount = 0;
    while (1)
    {
        pListElem = ListMP_getHead(pObj->listMPOutHndl);
        if (pListElem == NULL)
            break;

        OSA_assert(SYSTEM_IPC_FRAMES_GET_BUFSTATE(pListElem->bufState)
                     == IPC_FRAMEBUF_STATE_OUTQUE);
        SYSTEM_IPC_FRAMES_SET_BUFOWNERPROCID(pListElem->bufState);
        SYSTEM_IPC_FRAMES_SET_BUFSTATE(pListElem->bufState,
                                     IPC_FRAMEBUF_STATE_INQUE);
        status = ListMP_putTail(pObj->listMPInHndl, (ListMP_Elem *) pListElem);
        OSA_assert(status == ListMP_S_SUCCESS);
        pObj->stats.droppedCount++;
        freeCount++;
    }

    if (freeCount)
    {
        if(pObj->createArgs.baseCreateParams.notifyPrevLink)
        {
            System_ipcSendNotify(pObj->createArgs.baseCreateParams.inQueParams.
                                 prevLinkId);
        }
    }

    return IPC_FRAMES_IN_LINK_S_SUCCESS;
}

static
Int32 IpcFramesInLink_stop(IpcFramesInLink_Obj * pObj)
{
    pObj->state = IPC_FRAMES_IN_LINK_STATE_INACTIVE;
    IpcFramesInLink_freeQueuedInputBufs(pObj);
    return IPC_FRAMES_IN_LINK_S_SUCCESS;
}

static
Int32 IpcFramesInLink_delete(IpcFramesInLink_Obj * pObj)
{
    Int32 status;

    IPCFRAMESINLINK_INFO_LOG(pObj->tskId,
                           "RECV:%d\tFREE:%d,DROPPED:%d,AVGLATENCY:%d",
                           pObj->stats.recvCount,
                           pObj->stats.freeCount,
                           pObj->stats.droppedCount,
                 OSA_DIV(pObj->stats.totalRoundTrip ,
                            pObj->stats.freeCount));
#ifdef SYSTEM_DEBUG_IPC
    OSA_printf(" %d: IPC_FRAMES_IN   : Delete in progress !!!\n",
               OSA_getCurTimeInMsec());
#endif

    status = System_ipcListMPClose(&pObj->listMPOutHndl, &pObj->listMPInHndl);
    OSA_assert(status == OSA_SOK);

    OSA_queDelete(&pObj->outFrameBufQue);
    if (TRUE == pObj->createArgs.baseCreateParams.noNotifyMode) {
        IpcFramesInLink_deletePrdObj(pObj);
    }

    IpcFramesInLink_phyAddrMapTblDelete(pObj);
#ifdef SYSTEM_DEBUG_IPC
    OSA_printf(" %d: IPC_FRAMES_IN   : Delete Done !!!\n", OSA_getCurTimeInMsec());
#endif

    return IPC_FRAMES_IN_LINK_S_SUCCESS;
}

static UInt32 IpcFrameInLink_mapDataFormat2NumPlanes(UInt32 format)
{
    UInt32 numPlanes = 0;

    switch(format)
    {
        case SYSTEM_DF_YUV422I_UYVY:
        case SYSTEM_DF_YUV422I_YUYV:
        case SYSTEM_DF_YUV422I_YVYU:
        case SYSTEM_DF_YUV422I_VYUY:
        case SYSTEM_DF_YUV444I:
        case SYSTEM_DF_RGB16_565:
        case SYSTEM_DF_ARGB16_1555:
        case SYSTEM_DF_RGBA16_5551:
        case SYSTEM_DF_ARGB16_4444:
        case SYSTEM_DF_RGBA16_4444:
        case SYSTEM_DF_ARGB24_6666:
        case SYSTEM_DF_RGBA24_6666:
        case SYSTEM_DF_RGB24_888:
        case SYSTEM_DF_ARGB32_8888:
        case SYSTEM_DF_RGBA32_8888:
        case SYSTEM_DF_BGR16_565:
        case SYSTEM_DF_ABGR16_1555:
        case SYSTEM_DF_ABGR16_4444:
        case SYSTEM_DF_BGRA16_5551:
        case SYSTEM_DF_BGRA16_4444:
        case SYSTEM_DF_ABGR24_6666:
        case SYSTEM_DF_BGR24_888:
        case SYSTEM_DF_ABGR32_8888:
        case SYSTEM_DF_BGRA24_6666:
        case SYSTEM_DF_BGRA32_8888:
            numPlanes = 1;
            break;
        case SYSTEM_DF_YUV422SP_UV:
        case SYSTEM_DF_YUV422SP_VU:
        case SYSTEM_DF_YUV420SP_UV:
        case SYSTEM_DF_YUV420SP_VU:
            numPlanes = 2;
            break;
        case SYSTEM_DF_YUV422P:
        case SYSTEM_DF_YUV420P:
        case SYSTEM_DF_YUV444P:
            numPlanes = 3;
            break;
        default:
            numPlanes = 0;
            break;
    }
    OSA_assert(numPlanes != 0);
    return numPlanes;
}

/**
  Get buffer size based on data format

  pFormat - data format information
  *size - buffer size
  *cOffset - C plane offset for YUV420SP data
*/
static
Int32 IpcFramesInLink_memFrameGetSize(UInt32  dataFormat,
                                      UInt32  pitch[],
                                      UInt32  height,
                                      UInt32 * size,
                                      UInt32 * cOffset)
{
    UInt32 bufferHeight;
    Int32 status = 0;

    bufferHeight = height;

    switch (dataFormat)
    {
        case SYSTEM_DF_YUV422I_YUYV:
        case SYSTEM_DF_YUV422I_YVYU:
        case SYSTEM_DF_YUV422I_UYVY:
        case SYSTEM_DF_YUV422I_VYUY:
        case SYSTEM_DF_YUV444I:
        case SYSTEM_DF_RGB24_888:
        case SYSTEM_DF_RAW_VBI:

            /* for single plane data format's */
            *size = pitch[0] * bufferHeight;
            break;

        case SYSTEM_DF_YUV422SP_UV:
        case SYSTEM_DF_YUV420SP_UV:

            /* for Y plane */
            *size = pitch[0] * bufferHeight;

            /* cOffset is at end of Y plane */
            *cOffset = *size;

            if (dataFormat == SYSTEM_DF_YUV420SP_UV)
            {
                /* C plane height is 1/2 of Y plane */
                bufferHeight = bufferHeight / 2;
            }

            /* for C plane */
            *size += pitch[1] * bufferHeight;
            break;

        default:
            /* illegal data format */
            status = -1;
            break;
    }

    /* align size to minimum required frame buffer alignment */
    *size = OSA_align(*size, SYSTEM_BUFFER_ALIGNMENT);

    return status;
}

static Void IpcFramesInLink_setVirtBufAddr(IpcFramesInLink_Obj * pObj,
                                           VIDFrame_Buf *frame,
                                           volatile SharedRegion_SRPtr srPtr[VIDFRAME_MAX_FIELDS][VIDFRAME_MAX_PLANES])
{
    Int i;
    UInt32 numPlanes;
    Uint32 fieldIdx;
    UInt32 size;
    UInt32 cOffset;
    Int status;
    Ptr bufVirtAddrStart;
    Ptr bufPhyAddrStart;
    System_LinkInQueParams *pInQueParams;
    UInt32 queId;

    pInQueParams = &pObj->createArgs.baseCreateParams.inQueParams;
    queId = pInQueParams->prevLinkQueId;
    /* This logic will not work for tiled memory. It only works when
     * buffers are allocated contiguously for Y/U/V planes
     */
    OSA_assert(SYSTEM_MT_TILEDMEM !=
               pObj->info.queInfo[queId].chInfo[frame->channelNum].memType);
    //UTILS_assert(pObj->inQueInfo.numQue == 1);
    status =
    IpcFramesInLink_memFrameGetSize(
      pObj->inQueInfo.queInfo[queId].chInfo[frame->channelNum].dataFormat,
      pObj->inQueInfo.queInfo[queId].chInfo[frame->channelNum].pitch,
      pObj->inQueInfo.queInfo[queId].chInfo[frame->channelNum].height,
      &size,
      &cOffset);
    OSA_assert(status == 0);
    //if (srPtr[0][0] == IPC_LINK_INVALID_SRPTR)
    if (1)
    {
        bufVirtAddrStart =
                (Ptr)IpcFramesInLink_phyAddr2VirtAddr(pObj,
                                                      (UInt32)(frame->phyAddr[0][0]),
                                                      size);
    }
    else
    {
        bufVirtAddrStart = SharedRegion_getPtr(srPtr[0][0]);
    }
    bufPhyAddrStart = frame->phyAddr[0][0];
    numPlanes =
      IpcFrameInLink_mapDataFormat2NumPlanes(
        pObj->inQueInfo.queInfo[0].chInfo[frame->channelNum].dataFormat);

    for (i = 0; i < numPlanes; i++)
    {
        /* Force field idx to 0 irrespective of top/bot field since
         * other links expect in this format
         */
        fieldIdx = 0;

        if (FALSE == pObj->createArgs.exportOnlyPhyAddr)
        {
            frame->addr[fieldIdx][i] =  bufVirtAddrStart;
            OSA_assert(frame->phyAddr[fieldIdx][i] == bufPhyAddrStart);
            bufVirtAddrStart = (Ptr)((UInt32)bufVirtAddrStart + cOffset);
            bufPhyAddrStart  = (Ptr)((UInt32)bufPhyAddrStart + cOffset);
        }
        else
        {
            frame->addr[fieldIdx][i] = NULL;
        }
    }
}

static Int32 IpcFramesInLink_getFrameBuf(IpcFramesInLink_Obj * pObj,
                                     SystemIpcFrames_ListElem * pListElem,
                                     VIDFrame_Buf ** pFrameBufPtr)
{
    OSA_assert(pListElem != NULL);
    /* No cache ops done since pListElem is allocated from non-cached memory */
    OSA_assert(SharedRegion_isCacheEnabled(SharedRegion_getId(pListElem)) ==
                 FALSE);
    if (FALSE == pObj->createArgs.exportOnlyPhyAddr)
    {
        IpcFramesInLink_setVirtBufAddr(pObj,&pListElem->frameBuf,
                                       pListElem->srBufPtr);
    }
    else
    {
        pListElem->frameBuf.addr[0][0] = NULL;
        pListElem->frameBuf.addr[0][1] = NULL;
    }
    *pFrameBufPtr = &pListElem->frameBuf;
    return IPC_FRAMES_IN_LINK_S_SUCCESS;
}

static
Int32 IpcFramesInLink_processFrameBufs(IpcFramesInLink_Obj * pObj)
{
    VIDFrame_Buf *pFrameBuf;
    SystemIpcFrames_ListElem *pListElem;
    UInt32 numFrameBufs;
    Int32 status;
    UInt32 curTime;

    numFrameBufs = 0;
    curTime = OSA_getCurTimeInMsec();
    while (1)
    {
        pListElem = ListMP_getHead(pObj->listMPOutHndl);
        if (pListElem == NULL)
            break;

        IpcFramesInLink_getFrameBuf(pObj, pListElem, &pFrameBuf);
        OSA_assert(SYSTEM_IPC_FRAMES_GET_BUFSTATE(pListElem->bufState)
                     == IPC_FRAMEBUF_STATE_OUTQUE);
        pListElem->timeStamp = curTime;
        pListElem->frameBuf.linkPrivate = (Ptr)pListElem;
        SYSTEM_IPC_FRAMES_SET_BUFOWNERPROCID(pListElem->bufState);
        SYSTEM_IPC_FRAMES_SET_BUFSTATE(pListElem->bufState,
                                     IPC_FRAMEBUF_STATE_DEQUEUED);
        pObj->stats.recvCount++;
        status = OSA_quePut(&pObj->outFrameBufQue,
                            (Int32)pFrameBuf, OSA_TIMEOUT_NONE);
        OSA_assert(status == OSA_SOK);

        numFrameBufs++;
    }

#ifdef SYSTEM_DEBUG_IPC_RT
    OSA_printf(" %d: IPC_FRAMES_IN   : Recevived %d framebufs !!!\n",
               OSA_getCurTimeInMsec(), numFrameBufs);
#endif

    if (numFrameBufs)
    {
        if (pObj->createArgs.cbFxn)
        {
            pObj->createArgs.cbFxn(pObj->createArgs.cbCtx);
        }
    }

    return IPC_FRAMES_IN_LINK_S_SUCCESS;
}

static
Int32 IpcFramesInLink_getFullFrames(IpcFramesInLink_Obj * pObj,
                                    VIDFrame_BufList * pFrameBufList)
{
    UInt32 idx;
    Int32 status;
    VIDFrame_Buf *pFrame;

    for (idx = 0; idx < VIDFRAME_MAX_FRAME_BUFS; idx++)
    {
        status =
            OSA_queGet(&pObj->outFrameBufQue, (Int32 *) & pFrame,
                       OSA_TIMEOUT_NONE);
        if (status != OSA_SOK)
            break;
        pFrameBufList->frames[idx] = *pFrame;
    }

    pFrameBufList->numFrames = idx;

    return IPC_FRAMES_IN_LINK_S_SUCCESS;
}

static
Int32 IpcFramesInLink_putEmptyFrames(IpcFramesInLink_Obj * pObj,
                                     VIDFrame_BufList * pFrameBufList)
{
    UInt32 bufId;
    Int32 status;
    SystemIpcFrames_ListElem *pListElem = NULL;
    UInt32 curTime;

#ifdef SYSTEM_DEBUG_IPC_RT
    OSA_printf(" %d: IPC_FRAMES_IN   : Releasing %d framebufs !!!\n",
               OSA_getCurTimeInMsec(), pFrameBufList->numFrames);
#endif

    if (pFrameBufList->numFrames)
    {
        OSA_assert(pFrameBufList->numFrames <= VIDFRAME_MAX_FRAME_BUFS);
        curTime = OSA_getCurTimeInMsec();
        pObj->stats.freeCount += pFrameBufList->numFrames;
        for (bufId = 0; bufId < pFrameBufList->numFrames; bufId++)
        {
            pListElem =
            (SystemIpcFrames_ListElem *) pFrameBufList->frames[bufId].linkPrivate;
            if (curTime > pListElem->timeStamp)
            {
                pObj->stats.totalRoundTrip += (curTime - pListElem->timeStamp);
            }
            SYSTEM_IPC_FRAMES_SET_BUFSTATE(pListElem->bufState,
                                         IPC_FRAMEBUF_STATE_INQUE);
            status =
                ListMP_putTail(pObj->listMPInHndl, (ListMP_Elem *) pListElem);
            OSA_assert(status == ListMP_S_SUCCESS);
        }

        if (pObj->createArgs.baseCreateParams.notifyPrevLink)
        {
            System_ipcSendNotify(pObj->createArgs.baseCreateParams.inQueParams.
                                 prevLinkId);
        }
    }

    return IPC_FRAMES_IN_LINK_S_SUCCESS;
}

static
Int32 IpcFramesInLink_getLinkInfo(OSA_TskHndl * pTsk, System_LinkInfo * info)
{
    IpcFramesInLink_Obj *pObj = (IpcFramesInLink_Obj *) pTsk->appData;

    memcpy(info, &pObj->info, sizeof(*info));

    return IPC_FRAMES_IN_LINK_S_SUCCESS;
}

static
Int IpcFramesInLink_tskMain(struct OSA_TskHndl * pTsk, OSA_MsgHndl * pMsg,
                            Uint32 curState)
{
    UInt32 cmd = OSA_msgGetCmd(pMsg);
    Bool ackMsg, done;
    Int status = IPC_FRAMES_IN_LINK_S_SUCCESS;
    IpcFramesInLink_Obj *pObj = (IpcFramesInLink_Obj *) pTsk->appData;

    OSA_printf("%s:Entered", __func__);

    if (cmd != SYSTEM_CMD_CREATE)
    {
        OSA_tskAckOrFreeMsg(pMsg, OSA_EFAIL);
        return status;
    }

    status = IpcFramesInLink_create(pObj, OSA_msgGetPrm(pMsg));

    OSA_tskAckOrFreeMsg(pMsg, status);

    if (status != OSA_SOK)
        return status;

    done = FALSE;
    ackMsg = FALSE;

    while (!done)
    {
        status = OSA_tskWaitMsg(pTsk, &pMsg);
        if (status != OSA_SOK)
            break;

        cmd = OSA_msgGetCmd(pMsg);

        switch (cmd)
        {
            case SYSTEM_CMD_DELETE:
                done = TRUE;
                ackMsg = TRUE;
                break;
            case SYSTEM_CMD_NEW_DATA:
                OSA_tskAckOrFreeMsg(pMsg, status);
                OSA_assert(pObj->prd.numPendingCmd > 0);
                pObj->prd.numPendingCmd--;
                IpcFramesInLink_processFrameBufs(pObj);
                break;
            case SYSTEM_CMD_STOP:
                IpcFramesInLink_stop(pObj);
                OSA_tskAckOrFreeMsg(pMsg, status);
                break;
            default:
                OSA_tskAckOrFreeMsg(pMsg, status);
                break;
        }
    }

    IpcFramesInLink_delete(pObj);

#ifdef SYSTEM_DEBUG_IPC_FRAMES_IN
    OSA_printf(" %d: IPC_FRAMES_IN   : Delete Done !!!\n", OSA_getCurTimeInMsec());
#endif

    if (ackMsg && pMsg != NULL)
        OSA_tskAckOrFreeMsg(pMsg, status);

    return IPC_FRAMES_IN_LINK_S_SUCCESS;
}
//[guo]
Void myNotifyCb(OSA_TskHndl *pTsk)
{
	printf("!!![%u]Notify Callback run!\n",OSA_getCurTimeInUsec());
	 // OSA_SNPRINTF(tskName, "IPC_FRAMES_IN%d", ipcFramesInId);
}
Int32 IpcFramesInLink_init()
{
    Int32 status;
    System_LinkObj linkObj;
    UInt32 ipcFramesInId;
    IpcFramesInLink_Obj *pObj;
    char tskName[32];
    UInt32 procId = System_getSelfProcId();

    OSA_COMPILETIME_ASSERT(offsetof(SystemIpcFrames_ListElem, frameBuf) == 0);
    OSA_COMPILETIME_ASSERT(offsetof(VIDFrame_Buf, reserved) == 0);
    OSA_COMPILETIME_ASSERT(sizeof(((SystemIpcFrames_ListElem *) 0)->frameBuf.reserved) ==
                             sizeof(ListMP_Elem));

    for (ipcFramesInId = 0; ipcFramesInId < IPC_FRAMES_IN_LINK_OBJ_MAX; ipcFramesInId++)
    {
        pObj = &gIpcFramesInLink_obj[ipcFramesInId];

        memset(pObj, 0, sizeof(*pObj));

        pObj->tskId =
            SYSTEM_MAKE_LINK_ID(procId,
                                SYSTEM_LINK_ID_IPC_FRAMES_IN_0) + ipcFramesInId;

        pObj->state = IPC_FRAMES_IN_LINK_STATE_INACTIVE;
        linkObj.pTsk = &pObj->tsk;
        linkObj.getLinkInfo = IpcFramesInLink_getLinkInfo;

        System_registerLink(pObj->tskId, &linkObj);

        OSA_SNPRINTF(tskName, "IPC_FRAMES_IN%d", ipcFramesInId);
//[guo_t]
		int HostNull1Id=HOST_LINK(SYSTEM_LINK_ID_NULL_1);
		System_ipcRegisterNotifyCb(HostNull1Id, myNotifyCb);
		  status = Notify_registerEvent(procId,
                                          SYSTEM_IPC_NOTIFY_LINE_ID,
                                          SYSTEM_IPC_NOTIFY_EVENT_ID,
                                          myNotifyCb, NULL);
		OSA_assert(status == Notify_S_SUCCESS);

		int ii=10;
		while(ii--){
		printf("!!![%u]Send notify to[%s][%d]\n",OSA_getCurTimeInUsec(),
			MultiProc_getName(SYSTEM_GET_PROC_ID(HostNull1Id)),SYSTEM_GET_LINK_ID(HostNull1Id));
		System_ipcSendNotify(HostNull1Id);//guo
		}
	 	//OSA_assert(0);//guo
	 	
//[guo_t]
        System_ipcRegisterNotifyCb(pObj->tskId, IpcFramesInLink_notifyCb);
        status = OSA_tskCreate(&pObj->tsk,
                               IpcFramesInLink_tskMain,
                               IPC_LINK_TSK_PRI,
                               IPC_LINK_TSK_STACK_SIZE, 0, pObj);
        OSA_assert(status == OSA_SOK);
    }
		
    return status;
}

Int32 IpcFramesInLink_deInit()
{
    UInt32 ipcFramesInId;

    for (ipcFramesInId = 0; ipcFramesInId < IPC_FRAMES_IN_LINK_OBJ_MAX; ipcFramesInId++)
    {
        OSA_tskDelete(&gIpcFramesInLink_obj[ipcFramesInId].tsk);
    }
    return IPC_FRAMES_IN_LINK_S_SUCCESS;
}

Int32 IpcFramesInLink_getFullVideoFrames(UInt32 linkId,
                                         VIDFrame_BufList *bufList)
{
    OSA_TskHndl * pTsk;
    IpcFramesInLink_Obj * pObj;
    Int status;

    OSA_assert(bufList != NULL);
    if (!((linkId  >= SYSTEM_HOST_LINK_ID_IPC_FRAMES_IN_0)
          &&
          (linkId  < (SYSTEM_HOST_LINK_ID_IPC_FRAMES_IN_0 + IPC_FRAMES_IN_LINK_OBJ_MAX))))
    {
        return IPC_FRAMES_IN_LINK_E_INVALIDLINKID;
    }
    pTsk = System_getLinkTskHndl(linkId);
    pObj = pTsk->appData;
    bufList->numFrames = 0;
    status = IpcFramesInLink_getFullFrames(pObj,bufList);
    return status;
}


Int32 IpcFramesInLink_putEmptyVideoFrames(UInt32 linkId,
                                          VIDFrame_BufList *bufList)
{
    OSA_TskHndl * pTsk;
    IpcFramesInLink_Obj * pObj;
    Int status;

    OSA_assert(bufList != NULL);
    if (!((linkId  >= SYSTEM_HOST_LINK_ID_IPC_FRAMES_IN_0)
          &&
          (linkId  < (SYSTEM_HOST_LINK_ID_IPC_FRAMES_IN_0 + IPC_FRAMES_IN_LINK_OBJ_MAX))))
    {
        return IPC_FRAMES_IN_LINK_E_INVALIDLINKID;
    }
    pTsk = System_getLinkTskHndl(linkId);
    pObj = pTsk->appData;
    status = IpcFramesInLink_putEmptyFrames(pObj,bufList);
    return status;
}
