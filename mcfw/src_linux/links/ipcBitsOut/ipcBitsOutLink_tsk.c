/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/
#include <stddef.h>
#include <stdlib.h>
#include "ipcBitsOutLink_priv.h"
#include <ti/syslink/utils/IHeap.h>
#include <ti/syslink/utils/Memory.h>
#include <ti/syslink/utils/Cache.h>
#include <ti/syslink/ProcMgr.h>
#include <mcfw/interfaces/link_api/avsync_hlos.h>

/** @enum EncDec_ResolutionClass
 *  @brief Enumeration of different resolution class.
 */
typedef enum EncDec_ResolutionClass {
    UTILS_ENCDEC_RESOLUTION_CLASS_FIRST = 0,
    UTILS_ENCDEC_RESOLUTION_CLASS_16MP = UTILS_ENCDEC_RESOLUTION_CLASS_FIRST,
    UTILS_ENCDEC_RESOLUTION_CLASS_1080P,
    UTILS_ENCDEC_RESOLUTION_CLASS_720P,
    UTILS_ENCDEC_RESOLUTION_CLASS_D1,
    UTILS_ENCDEC_RESOLUTION_CLASS_CIF,
    UTILS_ENCDEC_RESOLUTION_CLASS_LAST = UTILS_ENCDEC_RESOLUTION_CLASS_CIF,
    UTILS_ENCDEC_RESOLUTION_CLASS_COUNT =
        (UTILS_ENCDEC_RESOLUTION_CLASS_LAST + 1)
} EncDec_ResolutionClass;

#define UTILS_ENCDEC_RESOLUTION_CLASS_16MP_WIDTH                        (4*1024)
#define UTILS_ENCDEC_RESOLUTION_CLASS_16MP_HEIGHT                       (4*1024)

#define UTILS_ENCDEC_RESOLUTION_CLASS_1080P_WIDTH                         (1920)
#define UTILS_ENCDEC_RESOLUTION_CLASS_1080P_HEIGHT                        (1080)

#define UTILS_ENCDEC_RESOLUTION_CLASS_720P_WIDTH                          (1280)
#define UTILS_ENCDEC_RESOLUTION_CLASS_720P_HEIGHT                          (720)

#define UTILS_ENCDEC_RESOLUTION_CLASS_D1_WIDTH                             (720)
#define UTILS_ENCDEC_RESOLUTION_CLASS_D1_HEIGHT                            (576)

#define UTILS_ENCDEC_RESOLUTION_CLASS_CIF_WIDTH                            (352)
#define UTILS_ENCDEC_RESOLUTION_CLASS_CIF_HEIGHT                           (288)

typedef struct ipcBitsOutLinkResClassChannelInfo {
    UInt32 numActiveResClass;
    struct resInfo_s {
        EncDec_ResolutionClass resClass;
        UInt32 width;
        UInt32 height;
        UInt32 numChInResClass;
        UInt32 chIdx[SYSTEM_MAX_CH_PER_OUT_QUE];
    } resInfo[UTILS_ENCDEC_RESOLUTION_CLASS_COUNT];
} ipcBitsOutLinkResClassChannelInfo;

static ipcBitsOutLinkResClassChannelInfo resClassChInfo;


static
Void  IpcBitsOutLink_printBufferStats(IpcBitsOutLink_Obj * pObj)
{
    Int i;

    printf("\r\n IPCBITSOUTLINK:Buffer Statistics");
    printf("\r\n Num Alloc Pools:%d",pObj->outQueInfo.allocPoolInfo.numPools);
    printf("\r\n PoolId | TotalBufCnt | FreeBufCnt | BufSize");
    for (i = 0; i < pObj->outQueInfo.allocPoolInfo.numPools;i++)
    {
        printf("\r\n %7d|%13d|%11d|%8d",
               i,
               pObj->outQueInfo.allocPoolInfo.bufPoolInfo[i].numBufs,
               OSA_queGetQueuedCount(&pObj->listElemQue[i]),
               pObj->outQueInfo.allocPoolInfo.bufPoolInfo[i].bufSize);
    }
    printf("\n");
}

static
Int32 IpcBitsOutLink_putEmptyBufs(IpcBitsOutLink_Obj * pObj,
                                  Bitstream_BufList * pBufList);


#ifdef DDR_MEM_256M
#define UTILS_ENCDEC_BITBUF_SCALING_FACTOR           (2)
#else
#define UTILS_ENCDEC_BITBUF_SCALING_FACTOR           (1)
#endif

/** @enum UTILS_ENCDEC_GET_BITBUF_SIZE
 *  @brief Macro that returns max size of encoded bitbuffer for a given resolution
 *  @todo Factor targetBitRate in bit buf size calculation */
#define UTILS_ENCDEC_GET_BITBUF_SIZE(width,height)   ( (  (width) * (height)  )/UTILS_ENCDEC_BITBUF_SCALING_FACTOR )

static
EncDec_ResolutionClass ipcbitsoutlink_get_resolution_class(UInt32 width,
                                                           UInt32 height)
{
    EncDec_ResolutionClass resClass = UTILS_ENCDEC_RESOLUTION_CLASS_16MP;

    UTILS_assert((width <= UTILS_ENCDEC_RESOLUTION_CLASS_16MP_WIDTH)
                 && (height <= UTILS_ENCDEC_RESOLUTION_CLASS_16MP_HEIGHT));

    if ((width > UTILS_ENCDEC_RESOLUTION_CLASS_1080P_WIDTH) ||
        (height > UTILS_ENCDEC_RESOLUTION_CLASS_1080P_HEIGHT))
    {
        resClass = UTILS_ENCDEC_RESOLUTION_CLASS_16MP;
    }
    else
    {
        if ((width > UTILS_ENCDEC_RESOLUTION_CLASS_720P_WIDTH) ||
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
    }
    return resClass;
}

static
Int ipcbitsoutlink_get_resolution_class_info(EncDec_ResolutionClass resClass,
                                             UInt32 * pWidth, UInt32 * pHeight)
{
    Int status = IPC_BITSOUT_LINK_S_SUCCESS;

    switch (resClass)
    {
        case UTILS_ENCDEC_RESOLUTION_CLASS_16MP:
            *pWidth = UTILS_ENCDEC_RESOLUTION_CLASS_16MP_WIDTH;
            *pHeight = UTILS_ENCDEC_RESOLUTION_CLASS_16MP_HEIGHT;
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
            status = IPC_BITSOUT_E_INT_UNKNOWNRESOLUTIONCLASS;
            OSA_ERROR("Unknown resoltuion class:%d", resClass);
            break;
    }
    return status;
}

static
Int ipcbitsoutlink_add_chinfo_to_resclass(EncDec_ResolutionClass resClass,
                                          UInt32 chID,
                                          ipcBitsOutLinkResClassChannelInfo *
                                          resClassChInfo)
{
    Int i;
    Int status = IPC_BITSOUT_LINK_S_SUCCESS;

    UTILS_assert(resClassChInfo->numActiveResClass <=
                 UTILS_ENCDEC_RESOLUTION_CLASS_COUNT);
    for (i = 0; i < resClassChInfo->numActiveResClass; i++)
    {
        if (resClassChInfo->resInfo[i].resClass == resClass)
        {
            UInt32 curChIdx = resClassChInfo->resInfo[i].numChInResClass;

            UTILS_assert(curChIdx < SYSTEM_MAX_CH_PER_OUT_QUE);
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
            ipcbitsoutlink_get_resolution_class_info(resClass,
                                                     &(resClassChInfo->
                                                       resInfo[resClassIndex].
                                                       width),
                                                     &(resClassChInfo->
                                                       resInfo[resClassIndex].
                                                       height));
        UTILS_assert(status == IPC_BITSOUT_LINK_S_SUCCESS);
        resClassChInfo->resInfo[resClassIndex].numChInResClass = 0;
        resClassChInfo->resInfo[resClassIndex].chIdx[0] = chID;
        resClassChInfo->resInfo[resClassIndex].numChInResClass++;
        resClassChInfo->numActiveResClass++;
    }
    return status;
}

static
Int ipcbitsoutlink_compare_resclass_resolution(const void *resInfoA,
                                               const void *resInfoB)
{
    const struct resInfo_s *resA = resInfoA;
    const struct resInfo_s *resB = resInfoB;

    return ((resA->width * resA->height) - (resB->width * resB->height));
}

static
Void ipcbitsoutlink_merge_resclass_chinfo_entry(struct resInfo_s *entryTo,
                                                struct resInfo_s *entryFrom)
{
    Int i;

    for (i = 0; i < entryFrom->numChInResClass; i++)
    {
        UInt32 curChIdx = entryTo->numChInResClass;

        UTILS_assert(entryTo->numChInResClass < SYSTEM_MAX_CH_PER_OUT_QUE);
        entryTo->chIdx[curChIdx] = entryFrom->chIdx[i];
        entryTo->numChInResClass++;
    }
    entryTo->resClass = entryFrom->resClass;
    entryTo->width = entryFrom->width;
    entryTo->height = entryFrom->height;
}

static
Int ipcbitsoutlink_merge_resclass_chinfo(ipcBitsOutLinkResClassChannelInfo *
                                         resClassChInfo,
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
                  ipcbitsoutlink_compare_resclass_resolution);
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
        ipcbitsoutlink_merge_resclass_chinfo_entry(&resClassChInfo->
                                                   resInfo
                                                   [resolutionToMergeIdx],
                                                   &resClassChInfo->
                                                   resInfo
                                                   [resolutionFromMergeIdx]);
        resClassChInfo->numActiveResClass--;
    }
    return IPC_BITSOUT_LINK_S_SUCCESS;
}

static
Int ipcbitsoutlink_populate_outbuf_pool_size_info(IpcBitsOutLinkHLOS_CreateParams
                                                  * createArgs,
                                                  System_LinkQueInfo *
                                                  inQueInfo,
                                                  IpcBitsOutLink_OutQueueInfo *
                                                  outQueInfo)
{
    Int i, j;
    Int status = IPC_BITSOUT_LINK_S_SUCCESS;
    EncDec_ResolutionClass resClass;
    UInt32 totalNumOutBufs = 0;

    if (TRUE != createArgs->bufPoolPerCh)
    {
        resClassChInfo.numActiveResClass = 0;
        for (i = 0; i < inQueInfo->numCh; i++)
        {
            resClass =
                ipcbitsoutlink_get_resolution_class(inQueInfo->chInfo[i].width,
                                                    inQueInfo->chInfo[i].height);
            ipcbitsoutlink_add_chinfo_to_resclass(resClass, i, &resClassChInfo);
        }
        if (resClassChInfo.numActiveResClass > IPC_BITS_OUT_MAX_NUM_ALLOC_POOLS)
        {
            ipcbitsoutlink_merge_resclass_chinfo(&resClassChInfo,
                                                 IPC_BITS_OUT_MAX_NUM_ALLOC_POOLS);
        }
        else
        {
            qsort(resClassChInfo.resInfo, resClassChInfo.numActiveResClass,
                  sizeof(struct resInfo_s),
                  ipcbitsoutlink_compare_resclass_resolution);
        }
        outQueInfo->allocPoolInfo.numPools = resClassChInfo.numActiveResClass;
        for (i = 0; i < outQueInfo->allocPoolInfo.numPools; i++)
        {
            outQueInfo->allocPoolInfo.bufPoolInfo[i].numBufs =
                resClassChInfo.resInfo[i].numChInResClass *
                createArgs->numBufPerCh[i];

            totalNumOutBufs += outQueInfo->allocPoolInfo.bufPoolInfo[i].numBufs;
            outQueInfo->allocPoolInfo.bufPoolInfo[i].bufSize =
                UTILS_ENCDEC_GET_BITBUF_SIZE(resClassChInfo.resInfo[i].width,
                                             resClassChInfo.resInfo[i].height);
            for (j = 0; j < resClassChInfo.resInfo[i].numChInResClass; j++)
            {
                UTILS_assert(resClassChInfo.resInfo[i].chIdx[j] <
                             SYSTEM_MAX_CH_PER_OUT_QUE);
                outQueInfo->ch2poolMap[resClassChInfo.resInfo[i].chIdx[j]] = i;
            }
        }
        UTILS_assert(totalNumOutBufs <= SYSTEM_IPC_BITS_MAX_LIST_ELEM);
    }
    else
    {
        outQueInfo->allocPoolInfo.numPools = inQueInfo->numCh;
        for (i = 0; i < outQueInfo->allocPoolInfo.numPools; i++)
        {
            OSA_assert(createArgs->numBufPerCh[i] != 0);
            outQueInfo->allocPoolInfo.bufPoolInfo[i].numBufs =
                createArgs->numBufPerCh[i];
            outQueInfo->allocPoolInfo.bufPoolInfo[i].bufSize =
                UTILS_ENCDEC_GET_BITBUF_SIZE(inQueInfo->chInfo[i].width,
                                             inQueInfo->chInfo[i].height);
            outQueInfo->ch2poolMap[i] = i;
        }
    }
    UTILS_assert(outQueInfo->allocPoolInfo.numPools <=
                 IPC_BITS_OUT_MAX_NUM_ALLOC_POOLS);


    return status;
}

IpcBitsOutLink_Obj gIpcBitsOutLink_obj[IPC_BITS_OUT_LINK_OBJ_MAX];

Void IpcBitsOutLink_notifyCb(OSA_TskHndl * pTsk)
{
    OSA_tskSendMsg(pTsk, NULL, SYSTEM_IPC_CMD_RELEASE_FRAMES, NULL, 0);
}

Void *IpcBitsOutLink_periodicTaskFxn(Void * prm)
{
    IpcBitsOutLink_Obj *pObj = (IpcBitsOutLink_Obj *) prm;
    Int32 status;
    UInt32 printWarnCounter = 0;

    while (FALSE == pObj->prd.exitThread)
    {
        OSA_waitMsecs(IPC_BITS_OUT_PROCESS_PERIOD_MS);
        OSA_mutexLock(&pObj->apiMutex);
        if (pObj->pDeleteBufMsg)
        {
            UInt32 cmd;
            IpcBitsOutHLOSLink_deleteChBufParams *bufDelPrms;
            UInt32 chId;

            cmd = OSA_msgGetCmd(pObj->pDeleteBufMsg);
            bufDelPrms = OSA_msgGetPrm(pObj->pDeleteBufMsg);
            OSA_assert(IPCBITSOUT_LINK_CMD_DELETE_CH_BUFFER == cmd);
            chId = bufDelPrms->chId;
            OSA_assert((pObj->bitBufPoolState[chId] ==
                        IPCBITSOUTHLOS_BUFPOOL_STATE_DELETEINPROGRESS_FLUSH_DONE)
                        ||
                        (pObj->bitBufPoolState[chId] ==
                        IPCBITSOUTHLOS_BUFPOOL_STATE_DELETEINPROGRESS_WAIT_APP_BUF_FREE));
            if (pObj->bitBufPoolState[chId] ==
                IPCBITSOUTHLOS_BUFPOOL_STATE_DELETEINPROGRESS_FLUSH_DONE)
            {
                if (pObj->createArgs.baseCreateParams.noNotifyMode)
                {
                    if (pObj->prd.numPendingCmd < IPC_BITSOUT_MAX_PENDING_RELEASE_FRAMES_CMDS)
                    {
                        status = OSA_tskSendMsg(&pObj->tsk , NULL, SYSTEM_IPC_CMD_RELEASE_FRAMES,NULL,0);
                        pObj->prd.numPendingCmd++;
                    }
                    else
                    {
                        UInt32 curTime = OSA_getCurTimeInMsec();

                        OSA_printf("IPC_BITSOUTLINK:!WARNING!.Commands not being processed by link."
                                   "TimeSinceLastMsgProcess:%d",
                                   (curTime - pObj->delMsgReceiveTime));
                    }
                }
            }
            if ((OSA_getCurTimeInMsec() - pObj->delMsgReceiveTime) >
                IPC_BITSOUT_LINK_DELBUF_CMD_PENDING_WARNING_THRESHOLD_MS)
            {
                if ((printWarnCounter % IPC_BITSOUT_STATS_WARN_INTERVAL) == 0)
                {
                    OSA_printf("IPC_BITSOUTLINK:!WARNING!.IPCBITSOUT_LINK_CMD_DELETE_CH_BUFFER pending for [%d] ms"
                               " DeleteChBufState:%d",
                               (OSA_getCurTimeInMsec() - pObj->delMsgReceiveTime),
                               pObj->bitBufPoolState[chId]);
                }
                printWarnCounter++;
            }
        }
        else
        {
            printWarnCounter = 0;
        }
        OSA_mutexUnlock(&pObj->apiMutex);
    }
    return NULL;
}


static
Int32 IpcBitsOutLink_createPrdObj(IpcBitsOutLink_Obj * pObj)
{
    pObj->prd.numPendingCmd = 0;
    pObj->prd.exitThread = FALSE;
    OSA_thrCreate(&pObj->prd.thrHandle,
                  IpcBitsOutLink_periodicTaskFxn,
                  IPC_LINK_TSK_PRI, IPC_LINK_TSK_STACK_SIZE, pObj);
    return IPC_BITSOUT_LINK_S_SUCCESS;
}

static
Int32 IpcBitsOutLink_deletePrdObj(IpcBitsOutLink_Obj * pObj)
{
    pObj->prd.exitThread = TRUE;
    OSA_thrDelete(&pObj->prd.thrHandle);
    pObj->prd.numPendingCmd = 0;
    return IPC_BITSOUT_LINK_S_SUCCESS;
}

static
Void  ipcbitsoutlink_init_flush_frame_info(Bitstream_Buf *buf,
                                           UInt32 chId)
{
    memset(buf,0,sizeof(*buf));
    buf->addr = NULL;
    buf->phyAddr  = 0;
    buf->bufSize = 0;
    buf->fillLength = 0;
    buf->allocPoolID = IPC_BITSOUT_INVALID_ALLOC_POOL_ID;
    buf->channelNum  = chId;
    buf->flushFrame = TRUE;

}


static
Void ipcbitsoutlink_send_flush_frame(IpcBitsOutLink_Obj * pObj,
                                      UInt32 chId)
{
    SystemIpcBits_ListElem *pListElem;
    Int32 status;

    IPCBITSOUTLINK_INFO_LOG(pObj->tskId,"Flush Frame Sent for chId[%d]",chId);
    OSA_assert(chId < pObj->outQueInfo.allocPoolInfo.numPools);
    pObj->bitBufPoolState[chId] =
                            IPCBITSOUTHLOS_BUFPOOL_STATE_DELETEINPROGRESS_FLUSH_DONE;
    status = OSA_queGet(&pObj->listElemFreeQue,(Int32 *)&pListElem,OSA_TIMEOUT_NONE);
    OSA_assert(OSA_SOK == status);
    ipcbitsoutlink_init_flush_frame_info(&pListElem->bitBuf,chId);
    SYSTEM_IPC_BITS_SET_BUFSTATE(pListElem->bufState, IPC_BITBUF_STATE_OUTQUE);
    status = ListMP_putTail(pObj->listMPOutHndl, (ListMP_Elem *) pListElem);
    OSA_assert(status == ListMP_S_SUCCESS);
}

static Int IpcBitsOutLink_start(IpcBitsOutLink_Obj * pObj)
{
    pObj->startProcessing = TRUE;
    return IPC_BITSOUT_LINK_S_SUCCESS;
}

static Int IpcBitsOutLink_stop(IpcBitsOutLink_Obj * pObj)
{
    pObj->startProcessing = FALSE;
    return IPC_BITSOUT_LINK_S_SUCCESS;
}

static Void IpcBitsOutLink_initStats(IpcBitsOutLink_Obj * pObj)
{
    memset(&pObj->stats, 0, sizeof(pObj->stats));
}

static Ptr IpcBitsOutLink_MapUsrVirt2Phy(Ptr usrVirt)
{
    Ptr phyAddr = 0;

    phyAddr =  Memory_translate (usrVirt,
                                 Memory_XltFlags_Virt2Phys);

    OSA_printf("IPCBITSOUTLINK:Translated Addr Virt:%p To Phy:%p",
                usrVirt,phyAddr);
    return phyAddr;
}

Void IpcBitsOutLink_createFreeListElemQue(IpcBitsOutLink_Obj *pObj)
{
    Int status;
    Int i;

    status = OSA_queCreate(&pObj->listElemFreeQue,
                           OSA_ARRAYSIZE(pObj->listElem));
    OSA_assert(status == OSA_SOK);
    for (i = 0; i < OSA_ARRAYSIZE(pObj->listElem);i++)
    {
        status = OSA_quePut(&pObj->listElemFreeQue,
                            (Int32)pObj->listElem[i],
                            OSA_TIMEOUT_NONE);
        OSA_assert(status == OSA_SOK);
    }
}


Void IpcBitsOutLink_deleteFreeListElemQue(IpcBitsOutLink_Obj *pObj)
{
    Int status;

    status = OSA_queDelete(&pObj->listElemFreeQue);
    OSA_assert(status == OSA_SOK);
}

static Int IpcBitsOutLink_createOutObj(IpcBitsOutLink_Obj * pObj)
{
    Int status = OSA_SOK;
    Int32 poolId, elemId, bufId;
    IHeap_Handle srBitBufHeapHandle;
    UInt32 bufSize, numBufs, totBufSize, cacheLineSize;
    const UInt32 srIndex = SYSTEM_IPC_SR_CACHED;
    Ptr   phyAddr;

    ipcbitsoutlink_populate_outbuf_pool_size_info(&pObj->createArgs,
                                                  &pObj->createArgs.inQueInfo,
                                                  &pObj->outQueInfo);
    elemId = 0;
    srBitBufHeapHandle = SharedRegion_getHeap(srIndex);
    OSA_assert(srBitBufHeapHandle != NULL);
    cacheLineSize = SharedRegion_getCacheLineSize(srIndex);

    IpcBitsOutLink_createFreeListElemQue(pObj);
    for (poolId = 0; poolId < pObj->outQueInfo.allocPoolInfo.numPools; poolId++)
    {
        status = OSA_queCreate(&pObj->listElemQue[poolId],
                               SYSTEM_IPC_BITS_MAX_LIST_ELEM);
        OSA_assert(status == OSA_SOK);
        bufSize =
            OSA_align(pObj->outQueInfo.allocPoolInfo.bufPoolInfo[poolId].
                      bufSize, cacheLineSize);
        numBufs = pObj->outQueInfo.allocPoolInfo.bufPoolInfo[poolId].numBufs;
        totBufSize = bufSize * numBufs;
        printf ("###Bit buff of size from the SR # %d : %d\n", srIndex, totBufSize);
        pObj->bitBufPoolPtr[poolId] =
            Memory_alloc(srBitBufHeapHandle, totBufSize, cacheLineSize, NULL);
        OSA_assert(pObj->bitBufPoolPtr[poolId] != NULL);
        OSA_printf("IPC_BITSOUT:BitBuffer Alloc.PoolID:%d,Size:0x%X",
                    poolId,totBufSize);
        phyAddr = IpcBitsOutLink_MapUsrVirt2Phy(pObj->bitBufPoolPtr[poolId]);
        pObj->bitBufPoolSize[poolId] = totBufSize;
        pObj->appAllocBufCnt[poolId] = 0;
        for (bufId = 0; bufId < numBufs; bufId++)
        {
            SystemIpcBits_ListElem *listElem;

            OSA_assert(elemId < SYSTEM_IPC_BITS_MAX_LIST_ELEM);
            status = OSA_queGet(&pObj->listElemFreeQue,(Int32 *)&listElem,OSA_TIMEOUT_NONE);
            OSA_assert(OSA_SOK == status);
            elemId++;
            SYSTEM_IPC_BITS_SET_BUFOWNERPROCID(listElem->bufState);
            SYSTEM_IPC_BITS_SET_BUFSTATE(listElem->bufState,
                                         IPC_BITBUF_STATE_FREE);
            listElem->bitBuf.addr =
                (Ptr) (((UInt32) (pObj->bitBufPoolPtr[poolId])) +
                       (bufSize * bufId));
            if (phyAddr)
            {
                listElem->bitBuf.phyAddr =
                    (UInt32) ((UInt32) (phyAddr) +  (bufSize * bufId));
            }
            listElem->bitBuf.allocPoolID = poolId;
            listElem->bitBuf.bufSize = bufSize;
            listElem->bitBuf.fillLength = 0;
            listElem->bitBuf.mvDataFilledSize = 0;
            listElem->bitBuf.temporalId = 0;
            listElem->bitBuf.numTemporalLayerSetInCodec = 0;
            listElem->bitBuf.startOffset = 0;
            listElem->bitBuf.bottomFieldBitBufSize = 0;
            listElem->bitBuf.doNotDisplay = FALSE;
            listElem->bitBuf.inputFileChanged = FALSE;
            listElem->srBufPtr = SharedRegion_getSRPtr(listElem->bitBuf.addr,
                                                       srIndex);
            OSA_assert(listElem->srBufPtr != IPC_LINK_INVALID_SRPTR);
            status =
                OSA_quePut(&pObj->listElemQue[poolId], (Int32) listElem,
                           OSA_TIMEOUT_NONE);
            OSA_assert(status == OSA_SOK);
        }
    }
    return status;
}

static Int IpcBitsOutLink_deleteOutObj(IpcBitsOutLink_Obj * pObj)
{
    Int status = OSA_SOK;
    Int32 poolId, elemId;
    IHeap_Handle srBitBufHeapHandle;
    const UInt32 srIndex = SYSTEM_IPC_SR_CACHED;

    elemId = 0;
    srBitBufHeapHandle = SharedRegion_getHeap(srIndex);
    OSA_assert(srBitBufHeapHandle != NULL);
    for (poolId = 0; poolId < pObj->outQueInfo.allocPoolInfo.numPools; poolId++)
    {
        if (pObj->bitBufPoolState[poolId] == IPCBITSOUTHLOS_BUFPOOL_STATE_CREATED)
        {
            status = OSA_queDelete(&pObj->listElemQue[poolId]);
            OSA_assert(status == OSA_SOK);
            OSA_assert(pObj->bitBufPoolPtr[poolId] != NULL);
            OSA_assert(pObj->bitBufPoolSize[poolId] != 0);
            Memory_free(srBitBufHeapHandle,
                        pObj->bitBufPoolPtr[poolId], pObj->bitBufPoolSize[poolId]);
            OSA_printf("IPC_BITSOUT:BitBuffer Free.PoolID:%d,Size:0x%X",
                        poolId,pObj->bitBufPoolSize[poolId]);
            pObj->bitBufPoolPtr[poolId] = NULL;
            pObj->bitBufPoolSize[poolId] = 0;
        }
        else
        {
            OSA_assert(pObj->bitBufPoolState[poolId] == IPCBITSOUTHLOS_BUFPOOL_STATE_DELETED);
            OSA_assert(pObj->bitBufPoolPtr[poolId] == NULL);
            OSA_assert(pObj->bitBufPoolSize[poolId] == 0);
        }
    }
    IpcBitsOutLink_deleteFreeListElemQue(pObj);
    return status;
}

Int32 IpcBitsOutLink_create(IpcBitsOutLink_Obj * pObj,
                            IpcBitsOutLinkHLOS_CreateParams * pPrm)
{
    Int32 status;
    UInt32 i;

#ifdef SYSTEM_DEBUG_IPC
    OSA_printf(" %d: IPC_BITS_OUT   : Create in progress !!!\n",
               OSA_getCurTimeInMsec());
#endif

    memcpy(&pObj->createArgs, pPrm, sizeof(pObj->createArgs));

    for (i=0; i<IPC_LINK_BITS_OUT_MAX_NUM_ALLOC_POOLS; i++)
    {
        OSA_assert(i < IPC_BITS_OUT_MAX_NUM_ALLOC_POOLS);
        if(pObj->createArgs.numBufPerCh[i] == 0)
            pObj->createArgs.numBufPerCh[i] =
                  IPC_BITS_OUT_LINK_MAX_OUT_FRAMES_PER_CH;
    }

    status = System_ipcListMPReset(pObj->listMPOutHndl, pObj->listMPInHndl);
    OSA_assert(status == OSA_SOK);

    IpcBitsOutLink_createOutObj(pObj);

    IpcBitsOutLink_initStats(pObj);

    pObj->startProcessing = FALSE;
    pObj->pDeleteBufMsg   = NULL;

    IpcBitsOutLink_createPrdObj(pObj);
#ifdef SYSTEM_DEBUG_IPC
    OSA_printf(" %d: IPC_BITS_OUT   : Create Done !!!\n",
               OSA_getCurTimeInMsec());
#endif

    return IPC_BITSOUT_LINK_S_SUCCESS;
}

Int32 IpcBitsOutLink_delete(IpcBitsOutLink_Obj * pObj)
{
    Int32 status = IPC_BITSOUT_LINK_S_SUCCESS;
#ifdef SYSTEM_DEBUG_IPC
    OSA_printf(" %d: IPC_BITS_OUT   : Delete in progress !!!\n",
               OSA_getCurTimeInMsec());
#endif
    IpcBitsOutLink_deletePrdObj(pObj);
    OSA_assert(NULL == pObj->pDeleteBufMsg);
    IPCBITSOUTLINK_INFO_LOG(pObj->tskId,
                            "RECV:%d\tFREE:%d,DROPPED:%d,AVGLATENCY:%d",
                            pObj->stats.recvCount,
                            pObj->stats.freeCount,
                            pObj->stats.droppedCount,
                    OSA_DIV(pObj->stats.totalRoundTrip ,
                             pObj->stats.freeCount));

    IpcBitsOutLink_deleteOutObj(pObj);
    status = System_ipcListMPReset(pObj->listMPOutHndl, pObj->listMPInHndl);
    OSA_assert(status == OSA_SOK);
#ifdef SYSTEM_DEBUG_IPC
    OSA_printf(" %d: IPC_BITS_OUT   : Delete Done !!!\n",
               OSA_getCurTimeInMsec());
#endif

    return IPC_BITSOUT_LINK_S_SUCCESS;
}

static
Int32 IpcBitsOutLink_doPrePutCacheOp(IpcBitsOutLink_Obj * pObj,
                                     SystemIpcBits_ListElem * pListElem)
{
    if (pListElem->bitBuf.fillLength)
    {
        Cache_wbInv(pListElem->bitBuf.addr,
                    pListElem->bitBuf.fillLength, Cache_Type_ALL, TRUE);
    }
    /* No cache ops done since pListElem is allocated from non-cached memory */
    UTILS_assert(SharedRegion_isCacheEnabled(SharedRegion_getId(pListElem)) ==
                 FALSE);
    return IPC_BITSOUT_LINK_S_SUCCESS;
}

static
Int32 IpcBitsOutLink_doPostGetCacheOp(IpcBitsOutLink_Obj * pObj,
                                      SystemIpcBits_ListElem * pListElem)
{
    /* No cache ops done since pListElem is allocated from non-cached memory */
    UTILS_assert(SharedRegion_isCacheEnabled(SharedRegion_getId(pListElem)) ==
                 FALSE);
    return IPC_BITSOUT_LINK_S_SUCCESS;
}

static
Int32 IpcBitsOutLink_listMPPut(IpcBitsOutLink_Obj * pObj,
                               SystemIpcBits_ListElem * pListElem)
{
    Int32 status = IPC_BITSOUT_LINK_S_SUCCESS;

    SYSTEM_IPC_BITS_SET_BUFSTATE(pListElem->bufState, IPC_BITBUF_STATE_OUTQUE);
    IpcBitsOutLink_doPrePutCacheOp(pObj, pListElem);
    status = ListMP_putTail(pObj->listMPOutHndl, (ListMP_Elem *) pListElem);
    OSA_assert(status == ListMP_S_SUCCESS);
    return IPC_BITSOUT_LINK_S_SUCCESS;
}

static
UInt32 ipcbitsoutlink_map_bufsize2poolid(IpcBitsOutLink_Obj * pObj,
                                         UInt32 minBufSize)
{
    UInt32 allocPoolID = IPC_BITSOUT_INVALID_ALLOC_POOL_ID;
    Int i;

    for (i= 0; i < pObj->outQueInfo.allocPoolInfo.numPools; i++)
    {
        if (pObj->outQueInfo.allocPoolInfo.bufPoolInfo[i].bufSize >=
            minBufSize)
        {
            allocPoolID = i;
            break;
        }
    }

    return (allocPoolID);
}


static
Int32 IpcBitsOutLink_getEmptyBufs(IpcBitsOutLink_Obj * pObj,
                                  Bitstream_BufList * pBufList,
                                  IpcBitsOutLinkHLOS_BitstreamBufReqInfo *reqInfo)
{
    Int32 status = IPC_BITSOUT_LINK_S_SUCCESS;
    Int i;
    SystemIpcBits_ListElem *pListElem;
    Int32 bufId;
    UInt32 allocPoolID;

    OSA_assert(pObj->createArgs.inQueInfo.numCh < SYSTEM_MAX_CH_PER_OUT_QUE);
    bufId = 0;
    for (i = 0;
         (i < reqInfo->numBufs)
         &&
         (bufId < VIDBITSTREAM_MAX_BITSTREAM_BUFS);
         i++)
    {
        if (TRUE != pObj->createArgs.bufPoolPerCh)
        {
            if (reqInfo->reqType != IPC_BITSOUTHLOS_BITBUFREQTYPE_BUFSIZE)
            {
                OSA_printf("IPCBITSOUT:!!!WARNING.!!! "
                           "Buffer Request Type is ChID but "
                           "IpcBitsOut is created with bufPoolPerCh set to FALSE "
                           "Buffer Request Type should be IPC_BITSOUTHLOS_BITBUFREQTYPE_BUFSIZE");
                allocPoolID = IPC_BITSOUT_INVALID_ALLOC_POOL_ID;
            }
            else
            {
                allocPoolID = ipcbitsoutlink_map_bufsize2poolid(pObj,
                                                            reqInfo->u[i].minBufSize);
            }
        }
        else
        {
            if (reqInfo->reqType != IPC_BITSOUTHLOS_BITBUFREQTYPE_CHID)
            {
                OSA_printf("IPCBITSOUT:!!!WARNING.!!! "
                           "Buffer Request Type is bufSize but "
                           "IpcBitsOut is created with bufPoolPerCh set to TRUE "
                           "Buffer Request Type should be IPC_BITSOUTHLOS_BITBUFREQTYPE_CHID");
                allocPoolID = IPC_BITSOUT_INVALID_ALLOC_POOL_ID;
            }
            else
            {
                if(reqInfo->u[i].chNum < pObj->outQueInfo.allocPoolInfo.numPools)
                {
                    allocPoolID = reqInfo->u[i].chNum;
                }
                else
                {
                    OSA_printf("IPCBITSOUT:!!!WARNING.!!! Invalid channel number requested:[%d]",
                               reqInfo->u[i].chNum);
                    allocPoolID = IPC_BITSOUT_INVALID_ALLOC_POOL_ID;
                }
            }
        }
        if (allocPoolID != IPC_BITSOUT_INVALID_ALLOC_POOL_ID)
        {
            OSA_assert(allocPoolID < pObj->outQueInfo.allocPoolInfo.numPools);
            if (IPCBITSOUTHLOS_BUFPOOL_STATE_CREATED == pObj->bitBufPoolState[allocPoolID])
            {
                status =
                        OSA_queGet(&pObj->listElemQue[allocPoolID],
                                   (Int32 *) (&pListElem), OSA_TIMEOUT_NONE);
                if (status == OSA_SOK)
                {
                    OSA_assert(pListElem != NULL);
                    OSA_assert(bufId < VIDBITSTREAM_MAX_BITSTREAM_BUFS);
                    pBufList->bufs[bufId] = &pListElem->bitBuf;
                    pBufList->bufs[bufId]->fillLength = 0;
                    pBufList->bufs[bufId]->inputFileChanged = FALSE;
                    pBufList->bufs[bufId]->mvDataFilledSize = 0;
                    pBufList->bufs[bufId]->numTemporalLayerSetInCodec = 0;
                    pBufList->bufs[bufId]->temporalId = 0;
                    pBufList->bufs[bufId]->bottomFieldBitBufSize = 0;
                    pBufList->bufs[bufId]->allocPoolID = allocPoolID;
                    pBufList->bufs[bufId]->seqId = SYSTEM_DISPLAY_SEQID_DEFAULT;
                    pBufList->bufs[bufId]->addr = SharedRegion_getPtr(pListElem->srBufPtr);
                    pBufList->bufs[bufId]->flushFrame = FALSE;
                    if (TRUE == pObj->createArgs.bufPoolPerCh)
                    {
                        pBufList->bufs[bufId]->channelNum = allocPoolID;
                    }
                    bufId++;
                    pObj->appAllocBufCnt[allocPoolID] += 1;
                }
            }
            else
            {
                //OSA_printf("IPCBITSOUT:!!!WARNING.!!! Trying to allocate buffers from a disabled channel:[%d]",
                //           allocPoolID);
            }
        }
        else
        {
            #ifdef DEBUG_IPC_BITS
            OSA_printf("IPCBITSOUT:!!!WARNING.!!! Requested buf size not supported:[%d]",
                       reqInfo->u[i].minBufSize);
            #endif
        }
    }
    pBufList->numBufs = bufId;

    if (pBufList->numBufs == 0) {
        pObj->stats.numNoFreeBufCount++;
        if ((pObj->stats.numNoFreeBufCount % IPC_BITSOUT_STATS_WARN_INTERVAL) == 0)
        {
            #ifdef DEBUG_IPC_BITS
            OSA_printf("IPCBITSOUT:!!!WARNING.!!! NO FREE BUF AVAILABLE. OCCURENCE COUNT:[%d]",
                       pObj->stats.numNoFreeBufCount);
            #endif
        }
    }
    return IPC_BITSOUT_LINK_S_SUCCESS;
}


static
Void  IpcBitsOutLink_logTs(Bitstream_Buf *pBitBuf)
{
#ifdef SYSTEM_DEBUG_AVSYNC_DETAILED_LOGS
    UInt64 ts;

    ts = pBitBuf->upperTimeStamp & 0xFFFFFFFF;
    ts <<= 32;
    ts  |= pBitBuf->lowerTimeStamp & 0xFFFFFFFF;
    AvsyncLink_logIpcBitsOutTS(pBitBuf->channelNum,ts);
#else
    (Void)pBitBuf;
#endif
}

static
Int32 IpcBitsOutLink_putFullBufs(IpcBitsOutLink_Obj *pObj,
                                 Bitstream_BufList *pBufList)
{
    SystemIpcBits_ListElem *pListElem;
    Bitstream_Buf *pBitBuf;
    Bitstream_BufList freeBitBufList;
    Bool putDone = FALSE;
    Int32 bufId;
    Bool sendFlushFrame = FALSE;
    UInt32 flushFrameChId = ~(0u);

    freeBitBufList.numBufs = 0;
    for (bufId = 0; bufId < pBufList->numBufs; bufId++)
    {
        pBitBuf = pBufList->bufs[bufId];
        if (IPCBITSOUTHLOS_BUFPOOL_STATE_CREATED == pObj->bitBufPoolState[pBitBuf->allocPoolID])
        {
            IpcBitsOutLink_logTs(pBitBuf);
            OSA_assert(pObj->appAllocBufCnt[pBitBuf->allocPoolID] > 0);
            pObj->appAllocBufCnt[pBitBuf->allocPoolID] -= 1;
            /* Application must not set flush frame to false.
             * Flush frame is only used internally to delete buffer pool
             */
            pBitBuf->flushFrame = FALSE;
            pListElem = (SystemIpcBits_ListElem *)pBitBuf;
            OSA_assert(SharedRegion_getPtr(pListElem->srBufPtr) ==
                       pBitBuf->addr);
            if (0 == pBitBuf->fillLength)
            {
                /* filled length of 0 indicates application
                 * did not fill any data in this buffer.
                 * Free it immediately */
#if SYSTEM_DEBUG_IPC_RT
                OSA_printf(" IPC_OUT: Dropping bitbuf\n");
 #endif
                OSA_assert(freeBitBufList.numBufs <
                           VIDBITSTREAM_MAX_BITSTREAM_BUFS);
                freeBitBufList.bufs[freeBitBufList.numBufs] = pBitBuf;
                freeBitBufList.numBufs++;
                pObj->stats.droppedCount++;
                continue;
            }
            else
            {
                pObj->stats.recvCount++;
                OSA_assert(SYSTEM_IPC_BITS_GET_BUFSTATE(pListElem->bufState)
                           == IPC_BITBUF_STATE_FREE);
                OSA_assert(SYSTEM_IPC_BITS_GET_BUFOWNERPROCID(pListElem->bufState)
                           == System_getSelfProcId());
                SYSTEM_IPC_BITS_SET_BUFSTATE(pListElem->bufState,
                                             IPC_BITBUF_STATE_ALLOCED);
                IpcBitsOutLink_listMPPut(pObj, pListElem);
                putDone = TRUE;
            }
        }
        else
        {
            if (IPCBITSOUTHLOS_BUFPOOL_STATE_DELETEINPROGRESS_WAIT_APP_BUF_FREE ==
                pObj->bitBufPoolState[pBitBuf->allocPoolID])
            {
                OSA_assert(pObj->appAllocBufCnt[pBitBuf->allocPoolID] > 0);
                pObj->appAllocBufCnt[pBitBuf->allocPoolID] -= 1;
                if (0 == pObj->appAllocBufCnt[pBitBuf->allocPoolID])
                {
                    OSA_assert(FALSE == sendFlushFrame);
                    sendFlushFrame = TRUE;
                    flushFrameChId = pBitBuf->allocPoolID;
                }
            }
            else
            {
                IPCBITSOUTLINK_INFO_LOG(pObj->tskId,"Warning!. Trying to putFullBuffer for chId[%d] in invalid state [%d]",
                                        pBitBuf->allocPoolID,
                                        pObj->bitBufPoolState[pBitBuf->allocPoolID]);
            }
            /* filled length of 0 indicates application
             * did not fill any data in this buffer.
             * Free it immediately */
#if SYSTEM_DEBUG_IPC_RT
            OSA_printf(" IPC_OUT: Dropping bitbuf\n");
#endif
            OSA_assert(freeBitBufList.numBufs <
                       VIDBITSTREAM_MAX_BITSTREAM_BUFS);
            freeBitBufList.bufs[freeBitBufList.numBufs] = pBitBuf;
            freeBitBufList.numBufs++;
            pObj->stats.droppedCount++;
        }
    }
    if (freeBitBufList.numBufs)
    {
        IpcBitsOutLink_putEmptyBufs(pObj, &freeBitBufList);
    }
    if (sendFlushFrame)
    {
        OSA_assert(flushFrameChId < pObj->createArgs.inQueInfo.numCh);
        OSA_assert(IPCBITSOUTHLOS_BUFPOOL_STATE_DELETEINPROGRESS_WAIT_APP_BUF_FREE ==
                   pObj->bitBufPoolState[flushFrameChId]);
        ipcbitsoutlink_send_flush_frame(pObj,flushFrameChId);
    }
    if (putDone && (pObj->createArgs.baseCreateParams.notifyNextLink))
    {
        System_ipcSendNotify(pObj->createArgs.baseCreateParams.outQueParams[0].
                             nextLink);
    }
    if (!putDone)
    {
        pObj->stats.numNoFullBufCount++;
        if ((pObj->stats.numNoFullBufCount % IPC_BITSOUT_STATS_WARN_INTERVAL) == 0)
        {
            #ifdef DEBUG_IPC_BITS
            OSA_printf("IPCBITSOUT:!!!WARNING.!!! NO FULL BUF AVAILABLE. OCCURENCE COUNT:[%d]",
                       pObj->stats.numNoFullBufCount);
            #endif
        }
    }
    return IPC_BITSOUT_LINK_S_SUCCESS;
}


static
Int32 IpcBitsOutLink_putEmptyBufs(IpcBitsOutLink_Obj * pObj,
                                  Bitstream_BufList * pBufList)
{
    Int32 status = IPC_BITSOUT_LINK_S_SUCCESS;
    Int32 bufId;
    SystemIpcBits_ListElem *pListElem;

    for (bufId = 0; bufId < pBufList->numBufs; bufId++)
    {
        pListElem = (SystemIpcBits_ListElem *) pBufList->bufs[bufId];
        OSA_assert(SharedRegion_getPtr(pListElem->srBufPtr) ==
                   pBufList->bufs[bufId]->addr);
        /* release ListElem back to queue */
        SYSTEM_IPC_BITS_SET_BUFOWNERPROCID(pListElem->bufState);
        SYSTEM_IPC_BITS_SET_BUFSTATE(pListElem->bufState,
                                     IPC_BITBUF_STATE_FREE);
        OSA_assert(pListElem->bitBuf.allocPoolID <
                   pObj->outQueInfo.allocPoolInfo.numPools);
        status = OSA_quePut(&(pObj->listElemQue[pListElem->bitBuf.allocPoolID]),
                            (Int32) pListElem, OSA_TIMEOUT_NONE);
        OSA_assert(status == OSA_SOK);
    }
    return IPC_BITSOUT_LINK_S_SUCCESS;
}


static
Int32 ipcbitsoutlink_validate_delbuf_params(IpcBitsOutLink_Obj * pObj,
                                            IpcBitsOutHLOSLink_deleteChBufParams *bufDelPrms)
{
    Int32 status;

    OSA_assert(bufDelPrms != NULL);
    if ((TRUE == pObj->createArgs.bufPoolPerCh)
        &&
        (bufDelPrms->chId < pObj->createArgs.inQueInfo.numCh)
        &&
        (bufDelPrms->chId < OSA_ARRAYSIZE(pObj->bitBufPoolState))
        &&
        (pObj->pDeleteBufMsg == NULL)
        &&
        (pObj->bitBufPoolState[bufDelPrms->chId] == IPCBITSOUTHLOS_BUFPOOL_STATE_CREATED))
    {
        status = IPC_BITSOUT_LINK_S_SUCCESS;
    }
    else
    {
        status = IPC_BITSOUT_LINK_E_INVALIDPARAM;
    }
    return status;
}

static
Void ipcbitsoutlink_do_delete_ch_buffer_pool(IpcBitsOutLink_Obj * pObj,
                                             UInt32 chId)
{
    IHeap_Handle srBitBufHeapHandle;
    const UInt32 srIndex = SYSTEM_IPC_SR_CACHED;
    Int status,i;
    SystemIpcBits_ListElem *pListElem;

    srBitBufHeapHandle = SharedRegion_getHeap(srIndex);
    OSA_assert(srBitBufHeapHandle != NULL);

    OSA_assert(OSA_queGetQueuedCount(&pObj->listElemQue[chId]) ==
               pObj->outQueInfo.allocPoolInfo.bufPoolInfo[chId].numBufs);
    for (i = 0; i < pObj->outQueInfo.allocPoolInfo.bufPoolInfo[chId].numBufs; i++)
    {
        status = OSA_queGet(&pObj->listElemQue[chId],
                            (Int32 *)&pListElem,
                            OSA_TIMEOUT_NONE);
        OSA_assert(OSA_SOK == status);
        status = OSA_quePut(&pObj->listElemFreeQue,
                            (Int32)pListElem,
                            OSA_TIMEOUT_NONE);
        OSA_assert(OSA_SOK == status);
    }
    status = OSA_queDelete(&pObj->listElemQue[chId]);
    OSA_assert(status == OSA_SOK);
    OSA_assert(pObj->bitBufPoolPtr[chId] != NULL);
    OSA_assert(pObj->bitBufPoolSize[chId] != 0);
    Memory_free(srBitBufHeapHandle,
                pObj->bitBufPoolPtr[chId], pObj->bitBufPoolSize[chId]);
    OSA_printf("IPC_BITSOUT:BitBuffer Free.PoolID:%d,Size:0x%X",
                chId,pObj->bitBufPoolSize[chId]);
    pObj->bitBufPoolPtr[chId] = NULL;
    pObj->bitBufPoolSize[chId] = 0;
    pObj->outQueInfo.allocPoolInfo.bufPoolInfo[chId].numBufs = 0;
}

static
Void ipcbitsoutlink_handle_flush_frame_free(IpcBitsOutLink_Obj * pObj,
                                            SystemIpcBits_ListElem *pListElem)

{
    UInt32 chId;
    OSA_MsgHndl *delBufAckMsg;
    Int status;

    OSA_assert(pListElem != NULL);
    IPCBITSOUTLINK_INFO_LOG(pObj->tskId,"Flush Frame Received for chId[%d]",
                            pListElem->bitBuf.channelNum);
    chId = pListElem->bitBuf.channelNum;
    OSA_assert(chId < pObj->outQueInfo.allocPoolInfo.numPools);
    OSA_assert(IPCBITSOUTHLOS_BUFPOOL_STATE_DELETEINPROGRESS_FLUSH_DONE ==
               pObj->bitBufPoolState[chId]);
    OSA_assert(pObj->pDeleteBufMsg != NULL);
    delBufAckMsg = pObj->pDeleteBufMsg;
    pObj->pDeleteBufMsg = NULL;
    pObj->bitBufPoolState[chId] = IPCBITSOUTHLOS_BUFPOOL_STATE_DELETED;
    OSA_assert((pListElem - pObj->listElem[0]) < OSA_ARRAYSIZE(pObj->listElem));
    ipcbitsoutlink_do_delete_ch_buffer_pool(pObj,chId);
    status = OSA_quePut(&pObj->listElemFreeQue,(Int32)pListElem,OSA_TIMEOUT_NONE);
    OSA_assert(status == OSA_SOK);
    IPCBITSOUTLINK_INFO_LOG(pObj->tskId,"Ch buffer delete completed for chId[%d].Total Time[%d]",
                            pListElem->bitBuf.channelNum,
                            (OSA_getCurTimeInMsec() - pObj->delMsgReceiveTime));
    OSA_tskAckOrFreeMsg(delBufAckMsg,IPC_BITSOUT_LINK_S_SUCCESS);
}


static
Int32 IpcBitsOutLink_initateDeleteChBuffer(IpcBitsOutLink_Obj * pObj,
                                           IpcBitsOutHLOSLink_deleteChBufParams *bufDelPrms,
                                           Bool *storeMsgForDelayedAck)
{
    Int32 status = IPC_BITSOUT_LINK_S_SUCCESS;

    status = ipcbitsoutlink_validate_delbuf_params(pObj,bufDelPrms);

    if (IPC_BITSOUT_LINK_S_SUCCESS == status)
    {
        UInt32 chId = bufDelPrms->chId;

        if (0 == pObj->appAllocBufCnt[chId])
        {
            /* As application does not have any allocated buffers, send flush buffer now */
            ipcbitsoutlink_send_flush_frame(pObj,chId);
        }
        else
        {
            pObj->bitBufPoolState[chId] =
              IPCBITSOUTHLOS_BUFPOOL_STATE_DELETEINPROGRESS_WAIT_APP_BUF_FREE;
        }
        *storeMsgForDelayedAck = TRUE;
    }
    else
    {
        *storeMsgForDelayedAck = FALSE;
    }
    return status;
}

static
Int32 ipcbitsoutlink_validate_createbuf_params(IpcBitsOutLink_Obj * pObj,
                                               IpcBitsOutHLOSLink_createChBufParams *bufCreatePrms)
{
    Int32 status;

    OSA_assert(bufCreatePrms != NULL);
    if ((TRUE == pObj->createArgs.bufPoolPerCh)
        &&
        (bufCreatePrms->chId < pObj->createArgs.inQueInfo.numCh)
        &&
        (bufCreatePrms->chId < OSA_ARRAYSIZE(pObj->bitBufPoolState))
        &&
        (pObj->bitBufPoolState[bufCreatePrms->chId] == IPCBITSOUTHLOS_BUFPOOL_STATE_DELETED))
    {
        status = IPC_BITSOUT_LINK_S_SUCCESS;
    }
    else
    {
        status = IPC_BITSOUT_LINK_E_INVALIDPARAM;
    }
    return status;
}

static
UInt32 ipcbitsoutlink_get_bufpool_size(UInt32 width, UInt32 height)
{
    EncDec_ResolutionClass resClass;
    UInt32 resClassWidth,resClassHeight;
    UInt32 bufPoolSize;
    Int32 status;

    resClass =
        ipcbitsoutlink_get_resolution_class(width,
                                            height);
    status =
        ipcbitsoutlink_get_resolution_class_info(resClass,
                                                 &(resClassWidth),
                                                 &(resClassHeight));
        UTILS_assert(status == IPC_BITSOUT_LINK_S_SUCCESS);

    bufPoolSize =
    UTILS_ENCDEC_GET_BITBUF_SIZE(resClassWidth,
                                 resClassHeight);
    return bufPoolSize;
}

static
Int32 IpcBitsOutLink_doChBufCreate(IpcBitsOutLink_Obj * pObj,
                                   IpcBitsOutHLOSLink_createChBufParams *bufCreatePrms)
{
    IHeap_Handle srBitBufHeapHandle;
    const UInt32 srIndex = SYSTEM_IPC_SR_CACHED;
    Int status = OSA_SOK;
    UInt32 bufSize, numBufs, totBufSize, cacheLineSize;
    UInt32 bufId;
    UInt32 chId = bufCreatePrms->chId;
    Ptr   phyAddr;
    Memory_Stats memstats;

    pObj->createArgs.numBufPerCh[chId] = bufCreatePrms->numBufs;
    if(pObj->createArgs.numBufPerCh[chId] == 0)
        pObj->createArgs.numBufPerCh[chId] =
              IPC_BITS_OUT_LINK_MAX_OUT_FRAMES_PER_CH;

    pObj->outQueInfo.allocPoolInfo.bufPoolInfo[chId].numBufs
      = pObj->createArgs.numBufPerCh[chId];
    pObj->outQueInfo.allocPoolInfo.bufPoolInfo[chId].bufSize
      = ipcbitsoutlink_get_bufpool_size(bufCreatePrms->maxWidth,
                                        bufCreatePrms->maxHeight);
    cacheLineSize = SharedRegion_getCacheLineSize(srIndex);

    bufSize =
        OSA_align(pObj->outQueInfo.allocPoolInfo.bufPoolInfo[chId].
                  bufSize, cacheLineSize);
    numBufs = pObj->outQueInfo.allocPoolInfo.bufPoolInfo[chId].numBufs;
    totBufSize = bufSize * numBufs;
    printf ("###Bit buff of size from the SR # %d : %d\n", srIndex, totBufSize);
    srBitBufHeapHandle = SharedRegion_getHeap(srIndex);
    OSA_assert(srBitBufHeapHandle != NULL);
    Memory_getStats(srBitBufHeapHandle,&memstats);
    if (memstats.largestFreeSize >= totBufSize)
    {
        pObj->bitBufPoolPtr[chId] =
            Memory_alloc(srBitBufHeapHandle, totBufSize, cacheLineSize, NULL);
        OSA_assert(pObj->bitBufPoolPtr[chId] != NULL);
        OSA_printf("IPC_BITSOUT:BitBuffer Alloc.PoolID:%d,Size:0x%X",
                    chId,totBufSize);
        phyAddr = IpcBitsOutLink_MapUsrVirt2Phy(pObj->bitBufPoolPtr[chId]);
        pObj->bitBufPoolSize[chId] = totBufSize;
        pObj->appAllocBufCnt[chId] = 0;
        status = OSA_queCreate(&pObj->listElemQue[chId],
                               SYSTEM_IPC_BITS_MAX_LIST_ELEM);
        OSA_assert(status == OSA_SOK);

        for (bufId = 0; bufId < numBufs; bufId++)
        {
            SystemIpcBits_ListElem *listElem;

            status = OSA_queGet(&pObj->listElemFreeQue,(Int32 *)&listElem,OSA_TIMEOUT_NONE);
            OSA_assert(OSA_SOK == status);
            SYSTEM_IPC_BITS_SET_BUFOWNERPROCID(listElem->bufState);
            SYSTEM_IPC_BITS_SET_BUFSTATE(listElem->bufState,
                                         IPC_BITBUF_STATE_FREE);
            listElem->bitBuf.addr =
                (Ptr) (((UInt32) (pObj->bitBufPoolPtr[chId])) +
                       (bufSize * bufId));
            if (phyAddr)
            {
                listElem->bitBuf.phyAddr =
                    (UInt32) ((UInt32) (phyAddr) +  (bufSize * bufId));
            }
            listElem->bitBuf.allocPoolID = chId;
            listElem->bitBuf.bufSize = bufSize;
            listElem->bitBuf.fillLength = 0;
            listElem->bitBuf.mvDataFilledSize = 0;
            listElem->bitBuf.temporalId = 0;
            listElem->bitBuf.numTemporalLayerSetInCodec = 0;
            listElem->bitBuf.startOffset = 0;
            listElem->bitBuf.bottomFieldBitBufSize = 0;
            listElem->bitBuf.doNotDisplay = FALSE;
            listElem->bitBuf.inputFileChanged = FALSE;
            listElem->srBufPtr = SharedRegion_getSRPtr(listElem->bitBuf.addr,
                                                       srIndex);
            OSA_assert(listElem->srBufPtr != IPC_LINK_INVALID_SRPTR);
            status =
                OSA_quePut(&pObj->listElemQue[chId], (Int32) listElem,
                           OSA_TIMEOUT_NONE);
            OSA_assert(status == OSA_SOK);
        }
    }
    else
    {
        IPCBITSOUTLINK_INFO_LOG(pObj->tskId,"WARNING:Channel Create Failed.!!!! "
                                            "ChId[%d],RequiredMemory:[%d],AvailableMemory:[%d]",
                                            bufCreatePrms->chId,
                                            totBufSize,
                                            memstats.largestFreeSize);
        status = OSA_EFAIL;
    }
    status = (OSA_SOK == status) ? IPC_BITSOUT_LINK_S_SUCCESS : IPC_BITSOUT_LINK_E_INVALIDPARAM;

    return status;
}

static
Int32 IpcBitsOutLink_CreateChBuffer(IpcBitsOutLink_Obj * pObj,
                                    IpcBitsOutHLOSLink_createChBufParams *bufCreatePrms)
{
    Int32 status = IPC_BITSOUT_LINK_S_SUCCESS;

    status = ipcbitsoutlink_validate_createbuf_params(pObj,bufCreatePrms);

    if (IPC_BITSOUT_LINK_S_SUCCESS == status)
    {
        UInt32 chId = bufCreatePrms->chId;

        status = IpcBitsOutLink_doChBufCreate(pObj,bufCreatePrms);
        if (IPC_BITSOUT_LINK_S_SUCCESS == status)
        {
            pObj->bitBufPoolState[chId] = IPCBITSOUTHLOS_BUFPOOL_STATE_CREATED;
        }
    }
    return status;
}



Int32 IpcBitsOutLink_releaseBitBufs(IpcBitsOutLink_Obj * pObj)
{
    SystemIpcBits_ListElem *pListElem = NULL;
    Int32 status;
    UInt32 curTime, roundTripTime;

    curTime = OSA_getCurTimeInMsec();

    do
    {
        pListElem = ListMP_getHead(pObj->listMPInHndl);
        if (pListElem != NULL)
        {
            IpcBitsOutLink_doPostGetCacheOp(pObj, pListElem);
            if(SYSTEM_IPC_BITS_GET_BUFSTATE(pListElem->bufState)
               != IPC_BITBUF_STATE_INQUE)
            {
                OSA_printf("!!!WARNING.Bad ListElem\n");
                OSA_printf("ListElem:%p,State:%d,SharedRegionID:%d,SrPtr:%p",
                            pListElem,
                            SYSTEM_IPC_BITS_GET_BUFSTATE(pListElem->bufState),
                            SharedRegion_getId(pListElem),
                            (void *)pListElem->srBufPtr);
                OSA_printf("Stats:Recv:%d,Free:%d,Dropped:%d",
                           pObj->stats.recvCount,
                           pObj->stats.freeCount,
                           pObj->stats.droppedCount);
                OSA_assert(SYSTEM_IPC_BITS_GET_BUFSTATE(pListElem->bufState)
                           == IPC_BITBUF_STATE_INQUE);
            }
            if (curTime > ((UInt32) pListElem->ipcPrivData))
            {
                roundTripTime = curTime - ((UInt32) pListElem->ipcPrivData);
                pObj->stats.totalRoundTrip += roundTripTime;
            }
            pObj->stats.freeCount++;
            /* release ListElem back to queue */
            SYSTEM_IPC_BITS_SET_BUFOWNERPROCID(pListElem->bufState);
            SYSTEM_IPC_BITS_SET_BUFSTATE(pListElem->bufState,
                                         IPC_BITBUF_STATE_FREE);
            if (pListElem->bitBuf.flushFrame)
            {
                ipcbitsoutlink_handle_flush_frame_free(pObj,pListElem);
            }
            else
            {

                OSA_assert(pListElem->bitBuf.allocPoolID <
                           pObj->outQueInfo.allocPoolInfo.numPools);
                status =
                    OSA_quePut(&(pObj->listElemQue[pListElem->bitBuf.allocPoolID]),
                               (Int32) pListElem, OSA_TIMEOUT_NONE);
                OSA_assert(status == OSA_SOK);
            }

        }                                                  /* if (pListElem
                                                            * != NULL) */
    } while (pListElem != NULL);

    return IPC_BITSOUT_LINK_S_SUCCESS;
}

Int32 IpcBitsOutLink_getLinkInfo(OSA_TskHndl * pTsk, System_LinkInfo * info)
{
    System_LinkInfo linkInfo;
    IpcBitsOutLink_Obj *pObj = (IpcBitsOutLink_Obj *) pTsk->appData;

    linkInfo.numQue = 1;
    linkInfo.queInfo[0] = pObj->createArgs.inQueInfo;
    memcpy(info, &linkInfo, sizeof(*info));

    return IPC_BITSOUT_LINK_S_SUCCESS;
}

Int IpcBitsOutLink_tskMain(struct OSA_TskHndl * pTsk, OSA_MsgHndl * pMsg,
                           Uint32 curState)
{
    UInt32 cmd = OSA_msgGetCmd(pMsg);
    Bool ackMsg, done;
    Int32 status = IPC_BITSOUT_LINK_S_SUCCESS;
    IpcBitsOutLink_Obj *pObj = (IpcBitsOutLink_Obj *) pTsk->appData;

    if (cmd != SYSTEM_CMD_CREATE)
    {
        OSA_tskAckOrFreeMsg(pMsg, OSA_EFAIL);
        return status;
    }

    status = IpcBitsOutLink_create(pObj, OSA_msgGetPrm(pMsg));

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

        OSA_mutexLock(&pObj->apiMutex);
        switch (cmd)
        {
            case SYSTEM_CMD_DELETE:
                done = TRUE;
                ackMsg = TRUE;
                break;

            case SYSTEM_IPC_CMD_RELEASE_FRAMES:
                OSA_tskAckOrFreeMsg(pMsg, status);

#ifdef SYSTEM_DEBUG_IPC_RT
                OSA_printf(" %d: IPC_BITS_OUT   : Received Notify !!!\n",
                           OSA_getCurTimeInMsec());
#endif
                if (pObj->createArgs.baseCreateParams.noNotifyMode)
                {
                    pObj->prd.numPendingCmd--;
                }
                IpcBitsOutLink_releaseBitBufs(pObj);
                break;

            case SYSTEM_CMD_START:
                IpcBitsOutLink_start(pObj);
                OSA_tskAckOrFreeMsg(pMsg, status);
                break;

            case SYSTEM_CMD_STOP:
                IpcBitsOutLink_stop(pObj);
                OSA_tskAckOrFreeMsg(pMsg, status);
                break;
            case IPCBITSOUT_LINK_CMD_PRINT_BUFFER_STATISTICS:
                IpcBitsOutLink_printBufferStats(pObj);
                OSA_tskAckOrFreeMsg(pMsg, status);
                break;
            case IPCBITSOUT_LINK_CMD_DELETE_CH_BUFFER:
            {
                Bool storeMsgForDelayedAck;

                status = IpcBitsOutLink_initateDeleteChBuffer(
                                                       pObj,
                                                       OSA_msgGetPrm(pMsg),
                                                       &storeMsgForDelayedAck);
                if (storeMsgForDelayedAck)
                {
                    OSA_assert(pObj->pDeleteBufMsg == NULL);
                    pObj->delMsgReceiveTime = OSA_getCurTimeInMsec();
                    pObj->pDeleteBufMsg = pMsg;
                }
                else
                {
                    OSA_tskAckOrFreeMsg(pMsg, status);
                }
                break;
            }
            case IPCBITSOUT_LINK_CMD_CREATE_CH_BUFFER:
            {
                status = IpcBitsOutLink_CreateChBuffer(
                                                      pObj,
                                                      OSA_msgGetPrm(pMsg));
                OSA_tskAckOrFreeMsg(pMsg, status);
                break;
            }
            default:
                OSA_tskAckOrFreeMsg(pMsg, status);
                break;
        }
        OSA_mutexUnlock(&pObj->apiMutex);
    }

    IpcBitsOutLink_delete(pObj);

#ifdef SYSTEM_DEBUG_IPC_BITS_OUT
    OSA_printf(" %d: IPC_BITS_OUT   : Delete Done !!!\n",
               OSA_getCurTimeInMsec());
#endif

    if (ackMsg && pMsg != NULL)
        OSA_tskAckOrFreeMsg(pMsg, status);

    return IPC_BITSOUT_LINK_S_SUCCESS;
}

Int32 IpcBitsOutLink_allocListElem(IpcBitsOutLink_Obj * pObj)
{
    UInt32 shAddr;
    UInt32 elemId;

    shAddr = System_ipcListMPAllocListElemMem(SYSTEM_IPC_SR_NON_CACHED_DEFAULT,
                                              sizeof(SystemIpcBits_ListElem) *
                                              SYSTEM_IPC_BITS_MAX_LIST_ELEM);

    for (elemId = 0; elemId < SYSTEM_IPC_BITS_MAX_LIST_ELEM; elemId++)
    {
        pObj->listElem[elemId] =
            (SystemIpcBits_ListElem *) (shAddr +
                                        elemId *
                                        sizeof(SystemIpcBits_ListElem));
    }

    return IPC_BITSOUT_LINK_S_SUCCESS;
}

Int32 IpcBitsOutLink_freeListElem(IpcBitsOutLink_Obj * pObj)
{
    System_ipcListMPFreeListElemMem(SYSTEM_IPC_SR_NON_CACHED_DEFAULT,
                                    (UInt32) pObj->listElem[0],
                                    sizeof(SystemIpcBits_ListElem) *
                                    SYSTEM_IPC_BITS_MAX_LIST_ELEM);

    return IPC_BITSOUT_LINK_S_SUCCESS;
}

Int32 IpcBitsOutLink_initListMP(IpcBitsOutLink_Obj * pObj)
{
    Int32 status;

    status = System_ipcListMPCreate(SYSTEM_IPC_SR_NON_CACHED_DEFAULT,
                                    pObj->tskId,
                                    &pObj->listMPOutHndl, &pObj->listMPInHndl,
                                    &pObj->gateMPOutHndl, &pObj->gateMPInHndl);
    OSA_assert(status == OSA_SOK);

    IpcBitsOutLink_allocListElem(pObj);

    return status;
}

Int32 IpcBitsOutLink_deInitListMP(IpcBitsOutLink_Obj * pObj)
{
    Int32 status;

    status = System_ipcListMPDelete(&pObj->listMPOutHndl, &pObj->listMPInHndl,
                                    &pObj->gateMPOutHndl, &pObj->gateMPInHndl);
    OSA_assert(status == OSA_SOK);

    IpcBitsOutLink_freeListElem(pObj);

    return status;
}


Int32 IpcBitsOutLink_init()
{
    Int32 status;
    System_LinkObj linkObj;
    UInt32 ipcBitsOutId;
    IpcBitsOutLink_Obj *pObj;
    char tskName[32];
    UInt32 procId = System_getSelfProcId();

    OSA_COMPILETIME_ASSERT(offsetof(SystemIpcBits_ListElem, bitBuf) == 0);
    OSA_COMPILETIME_ASSERT(offsetof(Bitstream_Buf, reserved) == 0);
    OSA_COMPILETIME_ASSERT(sizeof(((Bitstream_Buf *) 0)->reserved) ==
                           sizeof(ListMP_Elem));
    for (ipcBitsOutId = 0; ipcBitsOutId < IPC_BITS_OUT_LINK_OBJ_MAX;
         ipcBitsOutId++)
    {
        pObj = &gIpcBitsOutLink_obj[ipcBitsOutId];

        memset(pObj, 0, sizeof(*pObj));

        pObj->tskId =
            SYSTEM_MAKE_LINK_ID(procId,
                                SYSTEM_LINK_ID_IPC_BITS_OUT_0) + ipcBitsOutId;

        linkObj.pTsk = &pObj->tsk;
        linkObj.getLinkInfo = IpcBitsOutLink_getLinkInfo;

        System_registerLink(pObj->tskId, &linkObj);

        OSA_SNPRINTF(tskName, "IPC_BITS_OUT%d", ipcBitsOutId);

        System_ipcRegisterNotifyCb(pObj->tskId, IpcBitsOutLink_notifyCb);

        IpcBitsOutLink_initListMP(pObj);

        status = OSA_mutexCreate(&pObj->apiMutex);
        OSA_assert(status == OSA_SOK);

        status = OSA_tskCreate(&pObj->tsk,
                               IpcBitsOutLink_tskMain,
                               IPC_LINK_TSK_PRI,
                               IPC_LINK_TSK_STACK_SIZE, 0, pObj);
        OSA_assert(status == OSA_SOK);
    }

    return status;
}

Int32 IpcBitsOutLink_deInit()
{
    UInt32 ipcBitsOutId;
    IpcBitsOutLink_Obj *pObj;
    Int32 status;

    for (ipcBitsOutId = 0; ipcBitsOutId < IPC_BITS_OUT_LINK_OBJ_MAX;
         ipcBitsOutId++)
    {
        pObj = &gIpcBitsOutLink_obj[ipcBitsOutId];

        OSA_tskDelete(&pObj->tsk);

        IpcBitsOutLink_deInitListMP(pObj);

        status = OSA_mutexDelete(&pObj->apiMutex);
        OSA_assert(status == OSA_SOK);

    }
    return IPC_BITSOUT_LINK_S_SUCCESS;
}

Int32 IpcBitsOutLink_getEmptyVideoBitStreamBufs(UInt32 linkId,
                                                Bitstream_BufList *bufList,
                                                IpcBitsOutLinkHLOS_BitstreamBufReqInfo *reqInfo)
{
    OSA_TskHndl * pTsk;
    IpcBitsOutLink_Obj * pObj;
    Int status;

    OSA_assert(bufList != NULL);
    if (!((linkId  >= SYSTEM_HOST_LINK_ID_IPC_BITS_OUT_0)
          &&
          (linkId  < (SYSTEM_HOST_LINK_ID_IPC_BITS_OUT_0 + IPC_BITS_OUT_LINK_OBJ_MAX))))
    {
        return IPC_BITSOUT_LINK_E_INVALIDLINKID;
    }
    pTsk = System_getLinkTskHndl(linkId);
    pObj = pTsk->appData;
    OSA_mutexLock(&pObj->apiMutex);
    bufList->numBufs = 0;
    if (pObj->startProcessing)
    {
        IpcBitsOutLink_releaseBitBufs(pObj);
        status = IpcBitsOutLink_getEmptyBufs(pObj,bufList,reqInfo);
    }
    else
    {
        status = IPC_BITSOUT_LINK_S_SUCCESS;
    }
    OSA_mutexUnlock(&pObj->apiMutex);
    return status;
}


Int32 IpcBitsOutLink_putFullVideoBitStreamBufs(UInt32 linkId,
                                               Bitstream_BufList *bufList)
{
    OSA_TskHndl * pTsk;
    IpcBitsOutLink_Obj * pObj;
    Int status;

    OSA_assert(bufList != NULL);
    if (!((linkId  >= SYSTEM_HOST_LINK_ID_IPC_BITS_OUT_0)
          &&
          (linkId  < (SYSTEM_HOST_LINK_ID_IPC_BITS_OUT_0 + IPC_BITS_OUT_LINK_OBJ_MAX))))
    {
        return IPC_BITSOUT_LINK_E_INVALIDLINKID;
    }
    pTsk = System_getLinkTskHndl(linkId);
    pObj = pTsk->appData;
    OSA_mutexLock(&pObj->apiMutex);
    status = IpcBitsOutLink_putFullBufs(pObj,bufList);
    OSA_mutexUnlock(&pObj->apiMutex);
    return status;
}

Int32 IpcBitsOutLink_getInQueInfo(UInt32 linkId,
                                  System_LinkQueInfo *inQueInfo)
{
    OSA_TskHndl * pTsk;
    IpcBitsOutLink_Obj * pObj;
    Int status = IPC_BITSOUT_LINK_S_SUCCESS;

    OSA_assert(inQueInfo != NULL);
    if (!((linkId  >= SYSTEM_HOST_LINK_ID_IPC_BITS_OUT_0)
          &&
          (linkId  < (SYSTEM_HOST_LINK_ID_IPC_BITS_OUT_0 + IPC_BITS_OUT_LINK_OBJ_MAX))))
    {
        return IPC_BITSOUT_LINK_E_INVALIDLINKID;
    }
    pTsk = System_getLinkTskHndl(linkId);
    pObj = pTsk->appData;
    if (pObj->startProcessing)
    {
        *inQueInfo = pObj->createArgs.inQueInfo;
    }
    else
    {
        inQueInfo->numCh = 0;
    }
    return status;
}


