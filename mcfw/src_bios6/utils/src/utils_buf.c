/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include <mcfw/src_bios6/utils/utils_buf.h>

Int32 Utils_bufCreate(Utils_BufHndl * pHndl, Bool blockOnGet, Bool blockOnPut)
{
    Int32 status;
    UInt32 flags;

    flags = UTILS_QUE_FLAG_NO_BLOCK_QUE;

    if (blockOnGet)
        flags |= UTILS_QUE_FLAG_BLOCK_QUE_GET;
    if (blockOnPut)
        flags |= UTILS_QUE_FLAG_BLOCK_QUE_PUT;

    status = Utils_queCreate(&pHndl->emptyQue,
                             UTILS_BUF_MAX_QUE_SIZE, pHndl->emptyQueMem, flags);
    UTILS_assert(status == FVID2_SOK);

    status = Utils_queCreate(&pHndl->fullQue,
                             UTILS_BUF_MAX_QUE_SIZE, pHndl->fullQueMem, flags);
    UTILS_assert(status == FVID2_SOK);

    return status;
}

Int32 Utils_bufDelete(Utils_BufHndl * pHndl)
{
    Utils_queDelete(&pHndl->emptyQue);
    Utils_queDelete(&pHndl->fullQue);

    return FVID2_SOK;
}

Int32 Utils_bufGetEmpty(Utils_BufHndl * pHndl, FVID2_FrameList * pFrameList,
                        UInt32 timeout)
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
            Utils_queGet(&pHndl->emptyQue, (Ptr *) & pFrameList->frames[idx], 1,
                         timeout);
        if (status != FVID2_SOK)
            break;
    }

    pFrameList->numFrames = idx;

    return FVID2_SOK;
}

Int32 Utils_bufGetEmptyFrame(Utils_BufHndl * pHndl,
                             FVID2_Frame ** pFrame, UInt32 timeout)
{
    Int32 status;

    UTILS_assert(pHndl != NULL);
    UTILS_assert(pFrame != NULL);

    *pFrame = NULL;

    status = Utils_queGet(&pHndl->emptyQue, (Ptr *) pFrame, 1, timeout);

    return status;
}

UInt32 Utils_bufGetEmptyFrameCount(Utils_BufHndl * pHndl)
{
    UInt32 emptyBufCnt;

    UTILS_assert(pHndl != NULL);
    emptyBufCnt = Utils_queGetQueuedCount(&pHndl->emptyQue);

    return emptyBufCnt;
}


Int32 Utils_bufPutEmpty(Utils_BufHndl * pHndl, FVID2_FrameList * pFrameList)
{
    UInt32 idx;
    Int32 status;

    UTILS_assert(pHndl != NULL);
    UTILS_assert(pFrameList != NULL);
    UTILS_assert(pFrameList->numFrames <= FVID2_MAX_FVID_FRAME_PTR);

    for (idx = 0; idx < pFrameList->numFrames; idx++)
    {
        status =
            Utils_quePut(&pHndl->emptyQue, pFrameList->frames[idx],
                         BIOS_NO_WAIT);
        UTILS_assert(status == FVID2_SOK);
    }

    return FVID2_SOK;
}

Int32 Utils_bufPutEmptyFrame(Utils_BufHndl * pHndl, FVID2_Frame * pFrame)
{
    Int32 status;

    UTILS_assert(pHndl != NULL);

    status = Utils_quePut(&pHndl->emptyQue, pFrame, BIOS_NO_WAIT);
    UTILS_assert(status == FVID2_SOK);

    return FVID2_SOK;
}

Int32 Utils_bufGetFull(Utils_BufHndl * pHndl, FVID2_FrameList * pFrameList,
                       UInt32 timeout)
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

Int32 Utils_bufGetFullFrame(Utils_BufHndl * pHndl,
                            FVID2_Frame ** pFrame, UInt32 timeout)
{
    Int32 status;

    UTILS_assert(pHndl != NULL);
    UTILS_assert(pFrame != NULL);

    *pFrame = NULL;

    status = Utils_queGet(&pHndl->fullQue, (Ptr *) pFrame, 1, timeout);

    return status;
}

Int32 Utils_bufPutFull(Utils_BufHndl * pHndl, FVID2_FrameList * pFrameList)
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

Int32 Utils_bufPutFullFrame(Utils_BufHndl * pHndl, FVID2_Frame * pFrame)
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

Void Utils_bufPrintStatus(UInt8 *str, Utils_BufHndl * pHndl)
{
    Vps_printf("%s Buf Q Status\n", str);
    Vps_printf("Empty Q -> count %d, wrPtr %d, rdPtr %d\n", pHndl->emptyQue.count, pHndl->emptyQue.curWr, pHndl->emptyQue.curRd);
    Vps_printf("Full Q -> count %d, wrPtr %d, rdPtr %d\n", pHndl->fullQue.count, pHndl->fullQue.curWr, pHndl->fullQue.curRd);
}


