/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include <mcfw/src_bios6/utils/utils_buf_ext.h>
#include <mcfw/src_bios6/links_common/system/system_priv_common.h>

Int32 Utils_bufCreateExt(Utils_BufHndlExt * pHndl, Bool blockOnGet, 
                         Bool blockOnPut, UInt32 numAllocPools)
{
    Int32 status;
    UInt32 flags;
    Int i;

    flags = UTILS_QUE_FLAG_NO_BLOCK_QUE;

    if (blockOnGet)
        flags |= UTILS_QUE_FLAG_BLOCK_QUE_GET;
    if (blockOnPut)
        flags |= UTILS_QUE_FLAG_BLOCK_QUE_PUT;

    for (i = 0; i < numAllocPools; i++)
    {
        status = Utils_queCreate(&pHndl->emptyQue[i],
                                 UTILS_BUF_MAX_QUE_SIZE, 
                                 pHndl->emptyQueMem[i], flags);
        UTILS_assert(status == FVID2_SOK);
    }
    pHndl->numAllocPools = numAllocPools;

    status = Utils_queCreate(&pHndl->fullQue,
                             UTILS_BUF_MAX_QUE_SIZE, pHndl->fullQueMem, flags);
    UTILS_assert(status == FVID2_SOK);

    return status;
}

Int32 Utils_bufDeleteExt(Utils_BufHndlExt * pHndl)
{
    Int i;

    for (i = 0; i < pHndl->numAllocPools; i++)
    {
       Utils_queDelete(&pHndl->emptyQue[i]);
    }
    pHndl->numAllocPools = 0;
    Utils_queDelete(&pHndl->fullQue);

    return FVID2_SOK;
}

Int32 Utils_bufGetEmptyExt(Utils_BufHndlExt * pHndl, 
                           FVID2_FrameList * pFrameList,
                           UInt32 allocPoolID, UInt32 timeout)
{
    UInt32 idx, maxFrames;
    Int32 status;
    System_FrameInfo *pFrameInfo;

    UTILS_assert(pHndl != NULL);
    UTILS_assert(pFrameList != NULL);

    if (timeout == BIOS_NO_WAIT)
        maxFrames = FVID2_MAX_FVID_FRAME_PTR;
    else
        maxFrames = pFrameList->numFrames;

    UTILS_assert(maxFrames <= FVID2_MAX_FVID_FRAME_PTR);
    UTILS_assert(allocPoolID < pHndl->numAllocPools);

    for (idx = 0; idx < maxFrames; idx++)
    {
        status =
            Utils_queGet(&(pHndl->emptyQue[allocPoolID]), 
                         (Ptr *) & pFrameList->frames[idx], 1, timeout);
        if (status != FVID2_SOK)
            break;
        pFrameInfo = (System_FrameInfo *) pFrameList->frames[idx]->appData;
        UTILS_assert(pFrameInfo != NULL);
        pFrameInfo->allocPoolID = allocPoolID;
    }

    pFrameList->numFrames = idx;

    return FVID2_SOK;
}

Int32 Utils_bufGetEmptyFrameExt(Utils_BufHndlExt * pHndl,
                                FVID2_Frame ** pFrame, 
                                UInt32 allocPoolID, UInt32 timeout)
{
    Int32 status;
    System_FrameInfo *pFrameInfo;

    UTILS_assert(pHndl != NULL);
    UTILS_assert(pFrame != NULL);

    *pFrame = NULL;
    UTILS_assert(allocPoolID < pHndl->numAllocPools);
    status = Utils_queGet(&(pHndl->emptyQue[allocPoolID]), 
                         (Ptr *) pFrame, 1, timeout);
    if (status == FVID2_SOK)
    {
        pFrameInfo = (System_FrameInfo *) ((*pFrame)->appData);
        UTILS_assert(pFrameInfo != NULL);
        pFrameInfo->allocPoolID = allocPoolID;
    }

    return status;
}

Int32 Utils_bufPutEmptyExt(Utils_BufHndlExt * pHndl, 
                           FVID2_FrameList * pFrameList)
{
    UInt32 idx;
    Int32 status;
    UInt32 allocPoolID;
    System_FrameInfo *pFrameInfo;
        
    UTILS_assert(pHndl != NULL);
    UTILS_assert(pFrameList != NULL);
    UTILS_assert(pFrameList->numFrames <= FVID2_MAX_FVID_FRAME_PTR);

    for (idx = 0; idx < pFrameList->numFrames; idx++)
    {
        pFrameInfo = (System_FrameInfo *) pFrameList->frames[idx]->appData;
        UTILS_assert(pFrameInfo != NULL);
        allocPoolID = pFrameInfo->allocPoolID;

        UTILS_assert(allocPoolID < pHndl->numAllocPools);
        status =
            Utils_quePut(&(pHndl->emptyQue[allocPoolID]), 
                         pFrameList->frames[idx], BIOS_NO_WAIT);
        UTILS_assert(status == FVID2_SOK);
    }

    return FVID2_SOK;
}

Int32 Utils_bufPutEmptyFrameExt(Utils_BufHndlExt * pHndl, FVID2_Frame * pFrame)
{
    Int32 status;
    System_FrameInfo *pFrameInfo;

    UTILS_assert(pHndl != NULL);
    pFrameInfo = (System_FrameInfo *) pFrame->appData;
    UTILS_assert(pFrameInfo != NULL);
    UTILS_assert(pFrameInfo->allocPoolID < pHndl->numAllocPools);

    status = Utils_quePut(&(pHndl->emptyQue[pFrameInfo->allocPoolID]), 
                         pFrame, BIOS_NO_WAIT);
    UTILS_assert(status == FVID2_SOK);

    return FVID2_SOK;
}

Int32 Utils_bufGetFullExt(Utils_BufHndlExt * pHndl,
                          FVID2_FrameList * pFrameList, UInt32 timeout)
{
    UInt32 idx, maxFrames;
    Int32 status;

    UTILS_assert(pHndl != NULL);
    UTILS_assert(pFrameList != NULL);

    if (timeout == BIOS_NO_WAIT)
        maxFrames = FVID2_MAX_FVID_FRAME_PTR;
    else
        maxFrames = pFrameList->numFrames;

    UTILS_assert(maxFrames <= FVID2_MAX_FVID_FRAME_PTR);

    for (idx = 0; idx < maxFrames; idx++)
    {
        status =
            Utils_queGet(&pHndl->fullQue, (Ptr *) & pFrameList->frames[idx], 1,
                         timeout);
        if (status != FVID2_SOK)
            break;
    }

    pFrameList->numFrames = idx;

    return FVID2_SOK;
}

Int32 Utils_bufGetFullFrameExt(Utils_BufHndlExt * pHndl,
                            FVID2_Frame ** pFrame, UInt32 timeout)
{
    Int32 status;

    UTILS_assert(pHndl != NULL);
    UTILS_assert(pFrame != NULL);

    *pFrame = NULL;

    status = Utils_queGet(&pHndl->fullQue, (Ptr *) pFrame, 1, timeout);

    return status;
}

Int32 Utils_bufPutFullExt(Utils_BufHndlExt * pHndl, 
                          FVID2_FrameList * pFrameList)
{
    UInt32 idx;
    Int32 status;

    UTILS_assert(pHndl != NULL);
    UTILS_assert(pFrameList != NULL);
    UTILS_assert(pFrameList->numFrames <= FVID2_MAX_FVID_FRAME_PTR);

    for (idx = 0; idx < pFrameList->numFrames; idx++)
    {
        status =
            Utils_quePut(&pHndl->fullQue, pFrameList->frames[idx],
                         BIOS_NO_WAIT);
        UTILS_assert(status == FVID2_SOK);
    }

    return FVID2_SOK;
}

Int32 Utils_bufPutFullFrameExt(Utils_BufHndlExt * pHndl, FVID2_Frame * pFrame)
{
    Int32 status;

    UTILS_assert(pHndl != NULL);

    status = Utils_quePut(&pHndl->fullQue, pFrame, BIOS_NO_WAIT);
    if (status != FVID2_SOK)
    {
#if 0
        Vps_rprintf
            ("%d: ERROR: In Utils_bufPutFullFrame(), Utils_quePut() failed !!!\n",
             Utils_getCurTimeInMsec());
#endif
    }

    return status;
}

Void Utils_bufExtPrintStatus(UInt8 *str, Utils_BufHndlExt * pHndl)
{
    Uint8 i;

    Vps_printf("%s BufExt Q Status\n", str);
    for (i=0; i<pHndl->numAllocPools; i++)
    {
        Vps_printf("Empty Q %d -> count %d, wrPtr %d, rdPtr %d\n", i, pHndl->emptyQue[i].count, pHndl->emptyQue[i].curWr, pHndl->emptyQue[i].curRd);
    }
    Vps_printf("Full Q -> count %d, wrPtr %d, rdPtr %d\n", pHndl->fullQue.count, pHndl->fullQue.curWr, pHndl->fullQue.curRd);
}

