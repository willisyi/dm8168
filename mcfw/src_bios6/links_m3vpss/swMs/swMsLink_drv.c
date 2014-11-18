/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include "swMsLink_priv.h"
#include "mcfw/src_bios6/links_m3vpss/system/system_priv_m3vpss.h"
#include <mcfw/interfaces/common_def/ti_vsys_common_def.h>

static
Int32 SwMsLink_freeFrame(System_LinkInQueParams * pInQueParams,
                         FVID2_FrameList * frameList, FVID2_Frame * pFrame);

Bool  waitingOnDriverCbSWMS[SYSTEM_SW_MS_MAX_INST] = {FALSE};
#define SWMSLINK_DEBUG_BLANK_OUTPUT_BUFFER                              (FALSE)

/*
This function is to fill blank data for a blank input frame
When a CH input frame is not available or a CH is disabled from display
then this blank frame is used as input

The same blank frame is used for all channels.

This initial color of the blank frame is filled inside this function
using CPU memcopy.

This works ONLY on YUV422I input format.
So that the CPU load during blank frame fill
and DDR BW during blank frame read is not high the blank frame
size should be small typically < 400x300

Asserts are put in the function to check these conditions.
*/
static
Int32 SwMsLink_fillBlankDataPattern(FVID2_Format * pFormat,
                               FVID2_Frame * pFrame)
{
    /* using memcpy to make blank frame buffer */
    UInt32 w, h, fillColor32;
    UInt32 *pAddr;

    if ((pFormat->dataFormat == FVID2_DF_YUV420SP_UV)
        ||
        (pFormat->dataFormat == FVID2_DF_YUV420SP_VU))
    {
        /* blank frame buffer is ALWAYS YUV422I and non-tiled */
        Vps_printf(" %d: SWMS: Invalid data format of blank buffer !!!\n",
                Utils_getCurTimeInMsec());
        UTILS_assert(0);
    }

    if(pFormat->height*pFormat->width > 400*300)
    {
        Vps_printf(" %d: SWMS: Blank buffer size of %d x %d is too big !!!\n",
                Utils_getCurTimeInMsec(),
                pFormat->width,
                pFormat->height
            );
        UTILS_assert(0);
    }

    fillColor32 = UTILS_DMA_GENERATE_FILL_PATTERN(0x00, 0x80, 0x80);

    for(h=0; h<pFormat->height; h++)
    {
        pAddr = (UInt32*)((UInt32)pFrame->addr[0][0] + h*pFormat->pitch[0]);
        for(w=0; w<pFormat->width; w+=2) /* +2 since data format is YUV422I */
        {
           *pAddr = fillColor32;

           pAddr++;
        }
    }


    return FVID2_SOK;
}

static
Void  SwMsLink_drvSetBlankOutputFlag(SwMsLink_Obj * pObj)
{
    Int frameId;

    for (frameId = 0; frameId < UTILS_ARRAYSIZE(pObj->outFrameInfo);
         frameId++)
    {
        pObj->outFrameInfo[frameId].swMsBlankOutBuf =
            SWMS_LINK_DO_OUTBUF_BLANKING;
    }
}

static
Int32 SwMsLink_drvDmaCreate(SwMsLink_Obj * pObj)
{
    Int32 status;

    status = Utils_dmaCreateCh(&pObj->dmaObj,
                               UTILS_DMA_DEFAULT_EVENT_Q,
                               SWMS_LINK_DMA_MAX_TRANSFERS, TRUE);
    UTILS_assert(status==FVID2_SOK);

    if(pObj->createArgs.enableLayoutGridDraw == TRUE)
    {
        status = Utils_dmaCreateCh(&pObj->gridDmaObj,
                                   UTILS_DMA_DEFAULT_EVENT_Q,
                                   SWMS_LINK_DMA_GRID_MAX_TRANSFERS, TRUE);
        UTILS_assert(status==FVID2_SOK);
    }

    return status;
}

static
Int32 SwMsLink_drvDmaDelete(SwMsLink_Obj * pObj)
{
    Int32 status;

    status = Utils_dmaDeleteCh(&pObj->dmaObj);
    UTILS_assert(status==FVID2_SOK);

    if(pObj->createArgs.enableLayoutGridDraw == TRUE)
    {
        status = Utils_dmaDeleteCh(&pObj->gridDmaObj);
        UTILS_assert(status==FVID2_SOK);
    }

    return status;
}

Int32 SwMsLink_drvDoDma(SwMsLink_Obj * pObj, FVID2_Frame *pFrame)
{
    Int32 status = 0,i;
    Utils_DmaFill2D lineInfo[SWMS_LINK_DMA_GRID_MAX_TRANSFERS];

    UInt16 thickness, numTx;

    UInt32 pitch;

    thickness = 4;

    pitch = pObj->outFrameFormat.pitch[0];

    numTx=0;

    for(i=0;i<pObj->layoutParams.numWin;i++)
    {

    /*Horizontal Top line*/
    lineInfo[numTx].destAddr[0] = pFrame->addr[0][0];
    lineInfo[numTx].destPitch[0] = pitch;
    lineInfo[numTx].dataFormat  = FVID2_DF_YUV422I_YUYV;
    lineInfo[numTx].width = pObj->layoutParams.winInfo[i].width;
    lineInfo[numTx].height = thickness;
    lineInfo[numTx].fillColorYUYV =
        UTILS_DMA_GENERATE_FILL_PATTERN(SW_MS_GRID_FILL_PIXEL_LUMA ,
                                        SW_MS_GRID_FILL_PIXEL_CHROMA,
                                        SW_MS_GRID_FILL_PIXEL_CHROMA);

    lineInfo[numTx].startX = pObj->layoutParams.winInfo[i].startX;
    lineInfo[numTx].startY = pObj->layoutParams.winInfo[i].startY;
    numTx++;

    /*Horizontal Bottom line*/
    lineInfo[numTx].destAddr[0] = pFrame->addr[0][0];
    lineInfo[numTx].destPitch[0] = pitch;
    lineInfo[numTx].dataFormat  = FVID2_DF_YUV422I_YUYV;
    lineInfo[numTx].width = pObj->layoutParams.winInfo[i].width;
    lineInfo[numTx].height = thickness;
    lineInfo[numTx].fillColorYUYV =
        UTILS_DMA_GENERATE_FILL_PATTERN(SW_MS_GRID_FILL_PIXEL_LUMA ,
                                        SW_MS_GRID_FILL_PIXEL_CHROMA,
                                        SW_MS_GRID_FILL_PIXEL_CHROMA);

    lineInfo[numTx].startX = pObj->layoutParams.winInfo[i].startX;
    lineInfo[numTx].startY = pObj->layoutParams.winInfo[i].startY +
    pObj->layoutParams.winInfo[i].height - thickness;
    numTx++;

    /*Vertical Left side*/
    lineInfo[numTx].destAddr[0] = pFrame->addr[0][0];
    lineInfo[numTx].destPitch[0] = pitch;
    lineInfo[numTx].dataFormat  = FVID2_DF_YUV422I_YUYV;
    lineInfo[numTx].width = thickness;
    lineInfo[numTx].height = pObj->layoutParams.winInfo[i].height;
    lineInfo[numTx].fillColorYUYV =
        UTILS_DMA_GENERATE_FILL_PATTERN(SW_MS_GRID_FILL_PIXEL_LUMA ,
                                        SW_MS_GRID_FILL_PIXEL_CHROMA,
                                        SW_MS_GRID_FILL_PIXEL_CHROMA);

    lineInfo[numTx].startX = pObj->layoutParams.winInfo[i].startX +
    pObj->layoutParams.winInfo[i].width - thickness;
    lineInfo[numTx].startY = pObj->layoutParams.winInfo[i].startY;
    numTx++;

    /*Vertical right side*/
    lineInfo[numTx].destAddr[0] = pFrame->addr[0][0];
    lineInfo[numTx].destPitch[0] = pitch;
    lineInfo[numTx].dataFormat  = FVID2_DF_YUV422I_YUYV;
    lineInfo[numTx].width = thickness;
    lineInfo[numTx].height =  pObj->layoutParams.winInfo[i].height;
    lineInfo[numTx].fillColorYUYV =
        UTILS_DMA_GENERATE_FILL_PATTERN(SW_MS_GRID_FILL_PIXEL_LUMA ,
                                        SW_MS_GRID_FILL_PIXEL_CHROMA,
                                        SW_MS_GRID_FILL_PIXEL_CHROMA);

    lineInfo[numTx].startX = pObj->layoutParams.winInfo[i].startX;
    lineInfo[numTx].startY = pObj->layoutParams.winInfo[i].startY;
    numTx++;

    UTILS_assert(numTx<=SWMS_LINK_DMA_GRID_MAX_TRANSFERS);

    status = Utils_dmaFill2D(&pObj->gridDmaObj, lineInfo, numTx);
    numTx = 0;
    }
    return status;
}


static
Int32 SwMsLink_drvCheckBlankOutputBuffer(SwMsLink_Obj * pObj, FVID2_Frame *pFrame)
{
    Int32 status = 0;
    System_FrameInfo *frameInfo = pFrame->appData;

    if ((NULL != frameInfo)
        &&
        (frameInfo->swMsBlankOutBuf))
    {
        Utils_DmaFill2D blankFrameInfo[SWMS_LINK_DMA_MAX_TRANSFERS];
        UInt16  numTx;
        UInt32 pitch;

        pitch = pObj->outFrameFormat.pitch[0];

        numTx=0;

        blankFrameInfo[numTx].destAddr[0] = pFrame->addr[0][0];
        blankFrameInfo[numTx].destPitch[0] = pitch;
        blankFrameInfo[numTx].dataFormat  = FVID2_DF_YUV422I_YUYV;
        blankFrameInfo[numTx].width = pObj->outFrameFormat.width;
        blankFrameInfo[numTx].height = pObj->outFrameFormat.height;
        blankFrameInfo[numTx].fillColorYUYV =
                UTILS_DMA_GENERATE_FILL_PATTERN(SW_MS_BLANK_FRAME_PIXEL_LUMA ,
                                                SW_MS_BLANK_FRAME_PIXEL_CHROMA,
                                                SW_MS_BLANK_FRAME_PIXEL_CHROMA);
        blankFrameInfo[numTx].startX = 0;
        blankFrameInfo[numTx].startY = 0;
        #if SWMSLINK_DEBUG_BLANK_OUTPUT_BUFFER
            Vps_printf("%d:SWMS: Start Blanking of output buffer: "
                       "BufAddr:[%p] Width:[%d] Height:[%d] Pitch:[%d] Pixel:[0x%X]",
                       Utils_getCurTimeInMsec(),
                       blankFrameInfo[numTx].destAddr[0],
                       blankFrameInfo[numTx].width,
                       blankFrameInfo[numTx].height,
                       blankFrameInfo[numTx].destPitch[0],
                       blankFrameInfo[numTx].fillColorYUYV);
        #endif
        numTx++;

        UTILS_assert(numTx<=SWMS_LINK_DMA_MAX_TRANSFERS);

        status = Utils_dmaFill2D(&pObj->dmaObj, blankFrameInfo, numTx);

        if (UTILS_ISERROR(status))
        {
            Vps_printf("SWMS: Utils_dmaFill2D for output buffer failed!!");
        }
        else
        {
            #if SWMSLINK_DEBUG_BLANK_OUTPUT_BUFFER
                Vps_printf("%d:SWMS: End Blanking of output buffer: "
                       "BufAddr:[%p] ",
                       Utils_getCurTimeInMsec(),
                       blankFrameInfo[0].destAddr[0]);
            #endif
        }
        frameInfo->swMsBlankOutBuf = FALSE;
    }

    return status;
}

static Int32 SwMsLink_drvCreateDupObj(SwMsLink_Obj * pObj)
{
    Int32 status;
    Int i;

    memset(&pObj->dupObj, 0, sizeof(pObj->dupObj));
    status = Utils_queCreate(&pObj->dupObj.dupQue,
                             UTILS_ARRAYSIZE(pObj->dupObj.dupQueMem),
                             pObj->dupObj.dupQueMem,
                             UTILS_QUE_FLAG_BLOCK_QUE_GET);
    UTILS_assertError(!UTILS_ISERROR(status),
                      status,
                      SWMS_LINK_E_DUPOBJ_CREATE_FAILED, pObj->linkId, -1);
    if (!UTILS_ISERROR(status))
    {
        for (i = 0; i < SWMS_LINK_MAX_DUP_FRAMES; i++)
        {
            pObj->dupObj.frameInfo[i].pVdecOrgFrame = NULL;
            pObj->dupObj.frameInfo[i].vdecRefCount = 0;
            pObj->dupObj.dupFrameMem[i].appData = &(pObj->dupObj.frameInfo[i]);
            status = Utils_quePut(&pObj->dupObj.dupQue,
                                  &pObj->dupObj.dupFrameMem[i], BIOS_NO_WAIT);
            UTILS_assert(!UTILS_ISERROR(status));
        }
    }
    return status;
}

static Int32 SwMsLink_drvDeleteDupObj(SwMsLink_Obj * pObj)
{
    Int32 status;

    UTILS_assertError((Utils_queIsFull(&pObj->dupObj.dupQue) == TRUE),
                      status,
                      SWMS_LINK_E_DUPOBJ_DELETE_FAILED, pObj->linkId, -1);
    status = Utils_queDelete(&pObj->dupObj.dupQue);
    UTILS_assertError(!UTILS_ISERROR(status),
                      status,
                      SWMS_LINK_E_DUPOBJ_DELETE_FAILED, pObj->linkId, -1);
    return status;
}

static Int32 SwMsLink_drv_init_outframe(SwMsLink_Obj * pObj,
                                        FVID2_Frame * pFrame)
{
    System_FrameInfo *pFrameInfo;

    pFrameInfo = (System_FrameInfo *) pFrame->appData;
    UTILS_assert((pFrameInfo != NULL)
                 &&
                 UTILS_ARRAYISVALIDENTRY(pFrameInfo, pObj->outFrameInfo));
    pFrameInfo->vdecRefCount = 1;
    pFrameInfo->pVdecOrgFrame = NULL;
    return FVID2_SOK;
}

static Int32 SwMsLink_dupFrame(SwMsLink_Obj * pObj, FVID2_Frame * pOrgFrame,
                               FVID2_Frame ** ppDupFrame)
{
    Int status = FVID2_SOK;
    FVID2_Frame *pFrame;
    System_FrameInfo *pFrameInfo, *pOrgFrameInfo;

    status =
        Utils_queGet(&pObj->dupObj.dupQue, (Ptr *) & pFrame, 1, BIOS_NO_WAIT);
    UTILS_assert(status == FVID2_SOK);
    UTILS_assert(pFrame != NULL);
    pFrameInfo = (System_FrameInfo *) pFrame->appData;
    UTILS_assert(pFrameInfo != NULL);
    while (((System_FrameInfo *) pOrgFrame->appData)->pVdecOrgFrame != NULL)
    {
        pOrgFrame = ((System_FrameInfo *) pOrgFrame->appData)->pVdecOrgFrame;
    }
    pOrgFrameInfo = pOrgFrame->appData;
    memcpy(pFrame, pOrgFrame, sizeof(*pOrgFrame));
    pOrgFrameInfo = pOrgFrame->appData;
    memcpy(pFrameInfo, pOrgFrameInfo, sizeof(*pOrgFrameInfo));

    pFrame->appData = pFrameInfo;
    pFrameInfo->pVdecOrgFrame = pOrgFrame;
    UTILS_assert(pOrgFrameInfo->vdecRefCount <= SWMS_LINK_MAX_DUP_PER_FRAME);
    pOrgFrameInfo->vdecRefCount++;
    *ppDupFrame = pFrame;

    return status;
}

Int32 SwMsLink_drvFreeProcessedFrames(SwMsLink_Obj * pObj,
                                      FVID2_FrameList * freeFrameList)
{
    Int i, status = FVID2_SOK;
    FVID2_Frame *freeFrame;
    FVID2_Frame *origFrame;
    System_FrameInfo *freeFrameInfo;
    UInt cookie;
    UInt32 origFrameIndex;

    cookie = Hwi_disable();

    for (i = 0; i < freeFrameList->numFrames; i++)
    {
        freeFrame = freeFrameList->frames[i];
        UTILS_assert(freeFrame != NULL);
        freeFrameInfo = freeFrame->appData;
        UTILS_assert(freeFrameInfo != NULL);
        if (freeFrameInfo->pVdecOrgFrame)
        {
            UTILS_assert(UTILS_ARRAYISVALIDENTRY(freeFrame,
                                                 pObj->dupObj.dupFrameMem));
            origFrame = freeFrameInfo->pVdecOrgFrame;
            status = Utils_quePut(&pObj->dupObj.dupQue,
                                  freeFrame, BIOS_NO_WAIT);
            UTILS_assert(!UTILS_ISERROR(status));
            freeFrame = origFrame;
            origFrameIndex = UTILS_ARRAYINDEX(freeFrame,pObj->outFrames);
            UTILS_assert(origFrameIndex < UTILS_ARRAYSIZE(pObj->outFrames));
            UTILS_COMPILETIME_ASSERT(UTILS_ARRAYSIZE(pObj->outFrames) ==
                                     UTILS_ARRAYSIZE(pObj->outFrameInfo));
            freeFrameInfo = &pObj->outFrameInfo[origFrameIndex];
        }
        UTILS_assert((freeFrameInfo->pVdecOrgFrame == NULL)
                     && (freeFrameInfo->vdecRefCount > 0));
        freeFrameInfo->vdecRefCount--;
        if (freeFrameInfo->vdecRefCount == 0)
        {
            status = Utils_bufPutEmptyFrame(&pObj->bufOutQue, freeFrame);
            UTILS_assert(!UTILS_ISERROR(status));
        }
    }

    Hwi_restore(cookie);

    return status;
}

Int32 SwMsLink_drvResetStatistics(SwMsLink_Obj * pObj)
{
    UInt32 winId;

    SwMsLink_OutWinObj *pWinObj;

    for(winId=0; winId<SYSTEM_SW_MS_MAX_WIN; winId++)
    {
        pWinObj = &pObj->winObj[winId];

        pWinObj->framesRecvCount = 0;
        pWinObj->framesInvalidChCount = 0;
        pWinObj->framesRejectCount = 0;
        pWinObj->framesQueRejectCount = 0;
        pWinObj->framesQueuedCount = 0;
        pWinObj->framesRepeatCount = 0;
        pWinObj->framesAccEventCount = 0;
        pWinObj->framesAccMax = 0;
        pWinObj->framesAccMin = 0xFF;
        pWinObj->framesDroppedCount = 0;
        pWinObj->framesUsedCount = 0;
        pWinObj->framesFidInvalidCount = 0;

        pWinObj->minLatency = 0xFF;
        pWinObj->maxLatency = 0;
    }
    pObj->framesOutReqCount = 0;
    pObj->framesOutDropCount = 0;
    pObj->framesOutRejectCount = 0;
    pObj->framesOutCount = 0;
    pObj->prevDoScalingTime  = 0;
    pObj->scalingInterval    = 0;
    pObj->scalingIntervalMin = SW_MS_SCALING_INTERVAL_INVALID;
    pObj->scalingIntervalMax = 0;
    pObj->statsStartTime = Utils_getCurTimeInMsec();

    return 0;
}

Int32 SwMsLink_drvResetAvgStatistics(SwMsLink_Obj * pObj)
{
    UInt32 winId;

    pObj->avgStats.framesOutCount = 0;
    for (winId = 0; winId < SYSTEM_SW_MS_MAX_WIN; winId++)
    {
        pObj->avgStats.framesUsedCount[winId] = 0;
    }

    pObj->avgStats.statsStartTime = Utils_getCurTimeInMsec();

    return 0;
}

Int32 SwMsLink_drvPrintAvgStatistics(SwMsLink_Obj * pObj)
{
    UInt32 elaspedTime;

    elaspedTime = Utils_getCurTimeInMsec() - pObj->avgStats.statsStartTime; // in msecs

    if(elaspedTime < SYSTEM_RT_STATS_LOG_INTERVAL*1000)
        return 0;

    #ifdef SYSTEM_PRINT_RT_AVG_STATS_LOG
    {
        UInt32 winId, outputFPS, totalWinFPS;

        outputFPS = (pObj->avgStats.framesOutCount*10000/elaspedTime);

        totalWinFPS = 0;
        for (winId = 0; winId < pObj->layoutParams.numWin; winId++)
        {
            totalWinFPS += pObj->avgStats.framesUsedCount[winId];
        }

        totalWinFPS = (totalWinFPS*10000)/elaspedTime;

        Vps_rprintf(" %-8s: Output  FPS: %d.%d fps , Total Window FPS: %d.%d fps ... in %d.%d secs\n",
            pObj->name,
            outputFPS/10,
            outputFPS%10,
            totalWinFPS/10,
            totalWinFPS%10,
            elaspedTime/1000,
            (elaspedTime%1000)/100
            );
    }
    #endif

    SwMsLink_drvResetAvgStatistics(pObj);


    return 0;
}

Int32 SwMsLink_drvPrintStatistics(SwMsLink_Obj * pObj, Bool resetAfterPrint)
{
    UInt32 winId;
    SwMsLink_OutWinObj *pWinObj;
    UInt32 elaspedTime;

    elaspedTime = Utils_getCurTimeInMsec() - pObj->statsStartTime; // in msecs
    elaspedTime /= 1000; // convert to secs

    Vps_printf( " \n"
            " *** [%s] Mosaic Statistics *** \n"
            " \n"
            " Elasped Time: %d secs\n"
            " \n"
            " Output Request FPS   : %d fps (%d frames) \n"
            " Output Actual  FPS   : %d fps (%d frames) \n"
            " Output Drop    FPS   : %d fps (%d frames) \n"
            " Output Reject  FPS   : %d fps (%d frames) \n"
            " Scaling Internal     : %d ms \n"
            " Scaling Internal min : %d ms \n"
            " Scaling Internal max : %d ms \n"
            " \n"
            " Win | Window Repeat Drop Recv Que  FID Invlid Acc Event          Invalid   Que Reject Reject Latency  \n"
            " Num | FPS    FPS    FPS  FPS  FPS  FPS        Count (Max/Min)    CH Frames Frames     Frames Min / Max\n"
            " ------------------------------------------------------------------------------------------------------\n",
            pObj->name,
            elaspedTime,
            pObj->framesOutReqCount/elaspedTime,
            pObj->framesOutReqCount,
            pObj->framesOutCount/elaspedTime,
            pObj->framesOutCount,
            pObj->framesOutDropCount/elaspedTime,
            pObj->framesOutDropCount,
            pObj->framesOutRejectCount/elaspedTime,
            pObj->framesOutRejectCount,
            pObj->scalingInterval/pObj->framesOutReqCount,
            pObj->scalingIntervalMin,
            pObj->scalingIntervalMax
            );


    for (winId = 0; winId < pObj->layoutParams.numWin; winId++)
    {
        pWinObj = &pObj->winObj[winId];

        Vps_printf( " %3d | %6d %6d %4d %4d %4d %10d %8d (%3d/%3d) %9d %10d %6d %3d / %3d\n",
            winId,
            #if 1
            pWinObj->framesUsedCount/elaspedTime,
            pWinObj->framesRepeatCount/elaspedTime,
            pWinObj->framesDroppedCount/elaspedTime,
            pWinObj->framesRecvCount/elaspedTime,
            pWinObj->framesQueuedCount/elaspedTime,
            pWinObj->framesFidInvalidCount/elaspedTime,
            #else
            pWinObj->framesUsedCount,
            pWinObj->framesRepeatCount,
            pWinObj->framesDroppedCount,
            pWinObj->framesRecvCount,
            pWinObj->framesQueuedCount,
            pWinObj->framesFidInvalidCount,
            #endif
            pWinObj->framesAccEventCount,
            pWinObj->framesAccMax,
            pWinObj->framesAccMin,
            pWinObj->framesInvalidChCount,
            pWinObj->framesQueRejectCount,
            pWinObj->framesRejectCount,
            pWinObj->minLatency,
            pWinObj->maxLatency
            );
    }

    Vps_printf( " \n");

    SwMsLink_drvPrintLayoutParams(pObj);

    Vps_printf( " \n");

    if(resetAfterPrint)
    {
        SwMsLink_drvResetStatistics(pObj);
    }

    return 0;
}

static
UInt32 SwMsLink_getStartWinIdFromDrvInst(SwMsLink_Obj *pObj, UInt32 drvInst)
{
    return pObj->DrvObj[drvInst].startWin;
}

Int32 SwMsLink_drvModifyFramePointer(SwMsLink_Obj * pObj, SwMsLink_DrvObj *pDrvObj,
                                         Bool addOffset)
{
    Int32 offset[2],frameId, bytesPerPixel,windId;
    System_LinkChInfo *rtChannelInfo;
    UInt32 drvId, startWinId;

    drvId = UTILS_ARRAYINDEX(pDrvObj,pObj->DrvObj);
    UTILS_assert(drvId < UTILS_ARRAYSIZE(pObj->DrvObj));
    startWinId = SwMsLink_getStartWinIdFromDrvInst(pObj,drvId);

    for(frameId = 0; frameId< pDrvObj->inFrameList.numFrames; frameId++)
    {
        windId = pDrvObj->inFrameList.frames[frameId]->channelNum + startWinId;
        if (pObj->layoutParams.winInfo[windId].channelNum <
            pObj->inQueInfo.numCh)
        {
            rtChannelInfo = &pObj->
                 rtChannelInfo[pObj->layoutParams.winInfo[windId].channelNum];

            if(rtChannelInfo->dataFormat == SYSTEM_DF_YUV422I_YUYV)
                bytesPerPixel = 2;
            else
                bytesPerPixel = 1;

            offset[0] = rtChannelInfo->pitch[0]*rtChannelInfo->startY    +
                                       rtChannelInfo->startX*bytesPerPixel;
            offset[1] = rtChannelInfo->pitch[1]*rtChannelInfo->startY/2  +
                                       rtChannelInfo->startX*bytesPerPixel;

            if(addOffset == FALSE)
            {
                offset[0] = -offset[0];
                offset[1] = -offset[1];
            }

            #ifndef SWMS_ENABLE_MODIFY_FRAME_POINTER_ALWAYS
            if ((rtChannelInfo->width + rtChannelInfo->startX) > SW_MS_MAX_WIDTH_SUPPORTED)
            #endif
            {
                /* when appData is NULL frame is blank frame in this case dont update address */
               if(pDrvObj->inFrameList.frames[frameId]->appData!=NULL)
               {
                    pDrvObj->inFrameList.frames[frameId]->addr[0][0] =
                      (Ptr) ((Int32)pDrvObj->inFrameList.frames[frameId]->addr[0][0] + offset[0]);
                   pDrvObj->inFrameList.frames[frameId]->addr[0][1] =
                        (Ptr) ((Int32)pDrvObj->inFrameList.frames[frameId]->addr[0][1] + offset[1]);
               }
            }

        }
    }
    return 0;
}

Void SwMsLink_drvTimerCb(UArg arg)
{
    SwMsLink_Obj *pObj = (SwMsLink_Obj *) arg;

    if(pObj->createArgs.enableProcessTieWithDisplay == FALSE)
    {
        Utils_tskSendCmd(&pObj->tsk, SW_MS_LINK_CMD_DO_SCALING);
    }
}

Void SwMsLink_drvDisplaySyncCb(Ptr arg)
{
    SwMsLink_Obj *pObj = (SwMsLink_Obj *) arg;
    UInt32 curTime, displayInterval;

    if(pObj->createArgs.enableProcessTieWithDisplay == TRUE)
    {
        curTime = Utils_getCurTimeInMsec();
        if (curTime > pObj->lastDisplayCBTS)
        {
            displayInterval = curTime - pObj->lastDisplayCBTS;
        }
        else
        {
            displayInterval = curTime + (0xFFFFFFFF - pObj->lastDisplayCBTS);
        }

        if(pObj->timerPeriod > (displayInterval + SWMS_LINK_DISPLAY_CB_OFFSET))
        {
            pObj->SwmsProcessTieWithDisplayLocalFlag ^= TRUE;
            pObj->enableOuputDupLocalFlag = TRUE;
        }
        else
        {
            pObj->SwmsProcessTieWithDisplayLocalFlag = TRUE;
            pObj->enableOuputDupLocalFlag = FALSE;
        }
        if(pObj->SwmsProcessTieWithDisplayLocalFlag)
        {
            Utils_tskSendCmd(&pObj->tsk, SW_MS_LINK_CMD_DO_SCALING);
        }
    pObj->lastDisplayCBTS = Utils_getCurTimeInMsec();
    }
}

Int32 SwMsLink_drvFvidCb(FVID2_Handle handle, Ptr appData, Ptr reserved)
{
    SwMsLink_DrvObj *pDrvObj = (SwMsLink_DrvObj *) appData;

    Semaphore_post(pDrvObj->complete);

    return FVID2_SOK;
}

Int32 SwMsLink_drvFvidErrCb(FVID2_Handle handle,
                            Ptr appData, Ptr errList, Ptr reserved)
{
    return FVID2_SOK;
}

Int32 SwMsLink_drvGetLayoutParams(SwMsLink_Obj * pObj, SwMsLink_LayoutPrm * layoutParams)
{
    SwMsLink_drvLock(pObj);

    memcpy(layoutParams, &pObj->layoutParams, sizeof(*layoutParams));

    SwMsLink_drvUnlock(pObj);

    return FVID2_SOK;

}

UInt32 SwMsLink_getDrvInstFromWinId(SwMsLink_Obj *pObj, UInt32 winId)
{
    UInt32 i;
    for(i = 0; i < pObj->createArgs.numSwMsInst; i++)
    {
        if(winId <= pObj->DrvObj[i].endWin)
            break;
    }
    return i;
}

Int32 SwMsLink_drvPrintLayoutParams(SwMsLink_Obj * pObj)
{
    UInt32 winId, chNum;
    SwMsLink_OutWinObj *pWinObj;
    char strDataFormat[8];

    Vps_printf( " \n"
            " *** [%s] Mosaic Parameters *** \n"
            " \n"
            " Output FPS: %d\n"
            " \n"
            " Win | Ch  | Input      | Input          | Input         | Input       | Output     |  Output         | Output        | Output      | Low Cost | SWMS | Data  | Blank |\n"
            " Num | Num | Start X, Y | Width x Height | Pitch Y / C   | Memory Type | Start X, Y |  Width x Height | Pitch Y / C   | Memory Type | ON / OFF | Inst | Format| Frame |\n"
            " ----------------------------------------------------------------------------------------------------------------------------------------------------------------------\n",
            pObj->name,
            pObj->layoutParams.outputFPS
            );


    for (winId = 0; winId < pObj->layoutParams.numWin; winId++)
    {
        pWinObj = &pObj->winObj[winId];

        chNum = pObj->layoutParams.winInfo[winId].channelNum;

        if(pWinObj->scRtInFrmPrm.dataFormat==FVID2_DF_YUV422I_YUYV)
            strcpy(strDataFormat, "422I ");
        else
        if(pWinObj->scRtInFrmPrm.dataFormat==FVID2_DF_YUV420SP_UV)
            strcpy(strDataFormat, "420SP");
        else
            strcpy(strDataFormat, "UNKNWN");

        Vps_printf
            (" %3d | %3d | %4d, %4d | %5d x %6d | %5d / %5d | %s | %4d, %4d | %5d x %6d | %5d / %6d | %s | %8s | %4d | %6s | %5s |\n",
                winId,
                chNum,
                pWinObj->scRtCropCfg.cropStartX,
                pWinObj->scRtCropCfg.cropStartY,
                pWinObj->scRtInFrmPrm.width,
                pWinObj->scRtInFrmPrm.height,
                pWinObj->scRtInFrmPrm.pitch[0],
                pWinObj->scRtInFrmPrm.pitch[1],
                gSystem_nameMemoryType[pWinObj->scRtInFrmPrm.memType],
                pObj->layoutParams.winInfo[winId].startX,
                pObj->layoutParams.winInfo[winId].startY,
                pWinObj->scRtOutFrmPrm.width,
                pWinObj->scRtOutFrmPrm.height,
                pWinObj->scRtOutFrmPrm.pitch[0],
                pWinObj->scRtOutFrmPrm.pitch[1],
                gSystem_nameMemoryType[pWinObj->scRtOutFrmPrm.memType],
                gSystem_nameOnOff[pObj->layoutParams.winInfo[winId].bypass],
                SwMsLink_getDrvInstFromWinId(pObj,winId),
                strDataFormat,
                gSystem_nameOnOff[pWinObj->isBlankFrameDisplay]
            );
    }

    Vps_printf( " \n");

    return FVID2_SOK;
}


Int32 SwMsLink_drvUpdateRtChannelInfo(SwMsLink_Obj * pObj)
{
    UInt32 chId;
    System_LinkChInfo *pChInfo;

    SwMsLink_drvLock(pObj);

   for (chId = 0; chId < pObj->inQueInfo.numCh; chId++)
   {
       UTILS_assert (chId < SYSTEM_SW_MS_MAX_CH_ID);
       pChInfo = &pObj->inQueInfo.chInfo[chId];

       pObj->rtChannelInfo[chId].startX = VpsUtils_align(pChInfo->startX, 2);
       pObj->rtChannelInfo[chId].startY = VpsUtils_align(pChInfo->startY, 2);
       pObj->rtChannelInfo[chId].width = VpsUtils_align(pChInfo->width, 2);
       pObj->rtChannelInfo[chId].height = VpsUtils_align(pChInfo->height, 2);
       pObj->rtChannelInfo[chId].pitch[0] = pChInfo->pitch[0];
       pObj->rtChannelInfo[chId].pitch[1] = pChInfo->pitch[1];
       pObj->rtChannelInfo[chId].pitch[2] = pChInfo->pitch[2];
       pObj->rtChannelInfo[chId].memType = pChInfo->memType;
       pObj->rtChannelInfo[chId].dataFormat = pChInfo->dataFormat;
   }

   SwMsLink_drvUnlock(pObj);

   return FVID2_SOK;
}

Int32 SwMsLink_drvGetInputChInfoFromWinId(SwMsLink_Obj * pObj,SwMsLink_WinInfo * pCropInfo)
{
    UInt32  chnl;

    SwMsLink_drvLock(pObj);

    chnl = pObj->layoutParams.winInfo[pCropInfo->winId].channelNum;

    pCropInfo->startX   = pObj->inTskInfo.queInfo[0].chInfo[chnl].startX;
    pCropInfo->startY   = pObj->inTskInfo.queInfo[0].chInfo[chnl].startY;
    pCropInfo->width    = pObj->inTskInfo.queInfo[0].chInfo[chnl].width;
    pCropInfo->height   = pObj->inTskInfo.queInfo[0].chInfo[chnl].height;

    if ((FVID2_SF_PROGRESSIVE == pObj->inQueInfo.chInfo[chnl].scanFormat) &&
        pObj->layoutParams.winInfo[pCropInfo->winId].bypass &&
        VPS_VPDMA_MT_NONTILEDMEM == pObj->inQueInfo.chInfo[chnl].memType
        )
    {
        pCropInfo->height   /= 2;
    }

    SwMsLink_drvUnlock(pObj);
    return FVID2_SOK;
}

Int32 SwMsLink_drvSetCropParam(SwMsLink_Obj * pObj,SwMsLink_WinInfo * pCropInfo)
{
    SwMsLink_OutWinObj *pWinObj;

    SwMsLink_drvLock(pObj);

    pWinObj = &pObj->winObj[pCropInfo->winId];

    pWinObj->scRtCropCfg.cropStartX =
        VpsUtils_align(pCropInfo->startX, 2);
    pWinObj->scRtCropCfg.cropStartY =
        VpsUtils_align(pCropInfo->startY, 2);
    pWinObj->scRtCropCfg.cropWidth = VpsUtils_align(pCropInfo->width, 2);
    pWinObj->scRtCropCfg.cropHeight =
        VpsUtils_align(pCropInfo->height, 2);

    pWinObj->scRtInFrmPrm.width =
        pWinObj->scRtCropCfg.cropStartX +
        pWinObj->scRtCropCfg.cropWidth;
    pWinObj->scRtInFrmPrm.height =
        pWinObj->scRtCropCfg.cropStartY +
        pWinObj->scRtCropCfg.cropHeight;

    pWinObj->applyRtPrm = TRUE;



    SwMsLink_drvUnlock(pObj);
    return FVID2_SOK;
}

Int32 SwMsLink_drvSetScCoeffs(SwMsLink_Obj * pObj, FVID2_Handle fvidHandle,
                UInt32 curCoeffId_v, UInt32 curCoeffId_h, Bool isDei)
{
    Int32 retVal = FVID2_SOK;
    Vps_ScCoeffParams coeffPrms;
    static char *scCoeffName[VPS_SC_SET_MAX] =
    {
        "3/16",
        "4/16",
        "5/16",
        "6/16",
        "7/16",
        "8/16",
        "9/16",
        "10/16",
        "11/16",
        "12/16",
        "13/16",
        "14/16",
        "15/16",
        "UPSCALE",
        "1/1",
    };

    if(curCoeffId_h>=VPS_SC_SET_MAX)
        curCoeffId_h = VPS_SC_US_SET;

    if(curCoeffId_v>=VPS_SC_SET_MAX)
        curCoeffId_v = VPS_SC_US_SET;

    Vps_rprintf(" %d: %s    : Loading Vertical Co-effs (%s)x ... \n",
                Utils_getCurTimeInMsec(), pObj->name, scCoeffName[curCoeffId_v]
            );
    Vps_rprintf(" %d: %s    : Loading Horizontal Co-effs (%s)x ... \n",
                Utils_getCurTimeInMsec(), pObj->name, scCoeffName[curCoeffId_h]
            );

    coeffPrms.hScalingSet = curCoeffId_h;
    coeffPrms.vScalingSet = curCoeffId_v;

    coeffPrms.coeffPtr = NULL;
    coeffPrms.scalarId = isDei
        ? VPS_M2M_DEI_SCALAR_ID_DEI_SC : VPS_M2M_SC_SCALAR_ID_DEFAULT;

    /* Program DEI scalar coefficient - Always used */
    retVal = FVID2_control(fvidHandle, IOCTL_VPS_SET_COEFFS, &coeffPrms, NULL);
    UTILS_assert(FVID2_SOK == retVal);

    Vps_rprintf(" %d: SWMS    : Co-effs Loading ... DONE !!!\n",
                Utils_getCurTimeInMsec());

    return (retVal);
}

Int32 SwMsLink_drvSelectScCoeffs (SwMsLink_Obj * pObj)
{
    SwMsLink_OutWinObj *pWinObj;

    UInt32 inH, outH, inW, outW, numerator, denominator;
    Int32  coeffId_v, coeffId_h;
    Bool isDei;
    Int32 instId;

    /* This is an example of selecting scaling coefficients

            Need to apply the below co-effs todo antiflicker with downscaling / upscaling

            VPS_SC_US_SET - 1x1
            VPS_SC_DS_SET_4_16 - for other layouts

            Selecting scaling co-eff based on vertical resolution.
            Assuming horizontal scaling will be proportionate to vertical scaling.

            Selecting co-effs based on SC done for WIN0.
            Assuming WIN0 scaling is same as other window's OR WIN0 is bigger / primary window.
         */

    pWinObj = &pObj->winObj[0];

    inH = pWinObj->scRtCropCfg.cropHeight;
    outH = pWinObj->scRtOutFrmPrm.height;
    inW = pWinObj->scRtCropCfg.cropWidth;
    outW = pWinObj->scRtOutFrmPrm.width;

    /* find best matching scaling co-effs */
    numerator = denominator = 16;
    for(coeffId_v=VPS_SC_US_SET;coeffId_v>=VPS_SC_DS_SET_3_16; coeffId_v--)
    {
        if( inH*numerator <= outH*denominator )
            break;

        numerator--;
    }

    numerator = denominator = 16;
    for(coeffId_h=VPS_SC_US_SET;coeffId_h>=VPS_SC_DS_SET_3_16; coeffId_h--)
    {
        if( inW*numerator <= outW*denominator )
            break;

        numerator--;
    }

    if ((coeffId_h == VPS_SC_US_SET)||(coeffId_v == VPS_SC_US_SET))
    {
        coeffId_h = VPS_SC_US_SET;
        coeffId_v = VPS_SC_US_SET;
    }

    if (coeffId_h != VPS_SC_US_SET)
        coeffId_h++;
    
    instId = 0;
    if(pObj->DrvObj[instId].isDeiDrv == TRUE)
        isDei = TRUE;
    else
        isDei = FALSE;
    if (pObj->DrvObj[instId].fvidHandle)
        SwMsLink_drvSetScCoeffs(pObj, pObj->DrvObj[instId].fvidHandle, 
                                coeffId_v, coeffId_h, isDei);

    instId += SYSTEM_SW_MS_MAX_INST;
    if(pObj->DrvObj[instId].isDeiDrv == TRUE)
        isDei = TRUE;
    else
        isDei = FALSE;

    if (pObj->DrvObj[instId].fvidHandle)
        SwMsLink_drvSetScCoeffs(pObj, pObj->DrvObj[instId].fvidHandle, 
                                coeffId_v, coeffId_h, isDei);

    return FVID2_SOK;
}

Int32 SwMsLink_drvSwitchLayout(SwMsLink_Obj * pObj,
                               SwMsLink_LayoutPrm * layoutParams,
                               Bool isLockAlredayTaken)
{
    UInt32 winId, chNum;
    SwMsLink_OutWinObj *pWinObj;
    System_LinkChInfo *pChInfo;
    UInt32 drvInst;

    if (isLockAlredayTaken == FALSE)
    {
        SwMsLink_drvLock(pObj);
    }

    pObj->switchLayout = TRUE;

    if(layoutParams->onlyCh2WinMapChanged == FALSE)
    {
          SwMsLink_drvResetStatistics(pObj);

          pObj->skipProcessing = SW_MS_SKIP_PROCESSING;
          SwMsLink_drvSetBlankOutputFlag(pObj);
    }

    memcpy(&pObj->layoutParams, layoutParams, sizeof(*layoutParams));

    SwMsLink_updateLayoutParams(&pObj->layoutParams, pObj->outFrameFormat.pitch[0]);

    SwMsLink_drvGetTimerPeriod(pObj, layoutParams);
    SwMsLink_drvClockPeriodReconfigure(pObj);

    for (winId = 0; winId < pObj->layoutParams.numWin; winId++)
    {
        drvInst = SwMsLink_getDrvInstFromWinId(pObj,winId);

        pWinObj = &pObj->winObj[winId];

        pWinObj->scRtOutFrmPrm.width = pObj->layoutParams.winInfo[winId].width;
        pWinObj->scRtOutFrmPrm.height = pObj->layoutParams.winInfo[winId].height;
        pWinObj->scRtOutFrmPrm.pitch[0] = pObj->outFrameFormat.pitch[0];
        pWinObj->scRtOutFrmPrm.pitch[1] = pObj->outFrameFormat.pitch[1];
        pWinObj->scRtOutFrmPrm.pitch[2] = pObj->outFrameFormat.pitch[2];
        pWinObj->scRtOutFrmPrm.memType =
            pObj->info.queInfo[0].chInfo[0].memType;

        pWinObj->scRtOutFrmPrm.dataFormat = pObj->outFrameFormat.dataFormat;

        chNum = pObj->layoutParams.winInfo[winId].channelNum;


        if(!(chNum == SYSTEM_SW_MS_INVALID_ID || chNum < pObj->inQueInfo.numCh))
        {
            Vps_printf(" WARNING: WIN%d: CH%d, Channel ID is INVALID (max allowed CH ID is CH%d), !!!\n", winId, chNum, pObj->inQueInfo.numCh-1);
        }

        if (chNum == SYSTEM_SW_MS_INVALID_ID || chNum >= pObj->inQueInfo.numCh)
        {
            pWinObj->scRtCropCfg.cropStartX = pObj->blankFrameScRtCropCfg.cropStartX;
            pWinObj->scRtCropCfg.cropStartY = pObj->blankFrameScRtCropCfg.cropStartY;
            pWinObj->scRtCropCfg.cropWidth  = pObj->blankFrameScRtCropCfg.cropWidth;
            pWinObj->scRtCropCfg.cropHeight = pObj->blankFrameScRtCropCfg.cropHeight;
            pWinObj->scRtInFrmPrm.width  = pObj->blankFrameScRtInFrmPrm.width;
            pWinObj->scRtInFrmPrm.height = pObj->blankFrameScRtInFrmPrm.height;
            pWinObj->scRtInFrmPrm.pitch[0] = pObj->blankFrameScRtInFrmPrm.pitch[0];
            pWinObj->scRtInFrmPrm.pitch[1] = pObj->blankFrameScRtInFrmPrm.pitch[1];
            pWinObj->scRtInFrmPrm.pitch[2] = pObj->blankFrameScRtInFrmPrm.pitch[2];
            pWinObj->scRtInFrmPrm.memType = pObj->blankFrameScRtInFrmPrm.memType;
            pWinObj->scRtInFrmPrm.dataFormat = pObj->blankFrameScRtInFrmPrm.dataFormat;

        }
        else
        {
            pChInfo = &pObj->rtChannelInfo[chNum];

            pWinObj->scRtCropCfg.cropStartX =
                VpsUtils_align(pChInfo->startX, 2);
            pWinObj->scRtCropCfg.cropStartY =
                VpsUtils_align(pChInfo->startY, 2);
            pWinObj->scRtCropCfg.cropWidth = VpsUtils_align(pChInfo->width, 2);
            pWinObj->scRtCropCfg.cropHeight =
                VpsUtils_align(pChInfo->height, 2);

            #ifdef SWMS_ENABLE_MODIFY_FRAME_POINTER_ALWAYS
            pWinObj->scRtCropCfg.cropStartX = 0;
            pWinObj->scRtCropCfg.cropStartY = 0;
            #endif

            pWinObj->scRtInFrmPrm.width =
                pWinObj->scRtCropCfg.cropStartX +
                pWinObj->scRtCropCfg.cropWidth;
            pWinObj->scRtInFrmPrm.height =
                pWinObj->scRtCropCfg.cropStartY +
                pWinObj->scRtCropCfg.cropHeight;
            pWinObj->scRtInFrmPrm.pitch[0] = pChInfo->pitch[0];
            pWinObj->scRtInFrmPrm.pitch[1] = pChInfo->pitch[1];
            pWinObj->scRtInFrmPrm.pitch[2] = pChInfo->pitch[2];
            pWinObj->scRtInFrmPrm.memType = pChInfo->memType;

            pWinObj->scRtInFrmPrm.dataFormat = pChInfo->dataFormat;
        }

        if(pWinObj->scRtInFrmPrm.width > SW_MS_MAX_WIDTH_SUPPORTED)
        {
            pWinObj->scRtInFrmPrm.width = pWinObj->scRtCropCfg.cropWidth;
            pWinObj->scRtCropCfg.cropStartX = 0;
        }

        if (pObj->DrvObj[drvInst].isDeiDrv)
        {
            memset(&pWinObj->deiRtPrm, 0, sizeof(pWinObj->deiRtPrm));

            pWinObj->deiRtPrm.deiOutFrmPrms = &pWinObj->scRtOutFrmPrm;
            pWinObj->deiRtPrm.deiInFrmPrms = &pWinObj->scRtInFrmPrm;
            pWinObj->deiRtPrm.deiScCropCfg = &pWinObj->scRtCropCfg;
            pWinObj->deiRtPrm.deiRtCfg = &pWinObj->deiRtCfg;

            pWinObj->deiRtCfg.resetDei = FALSE;
            pWinObj->deiRtCfg.fldRepeat = FALSE;

            if(pObj->DrvObj[drvInst].forceBypassDei == TRUE)
            {
                /* In case captured data is progressive and window is bypass
                * (don't care quality so much), SC takes only even lines and
                * make S/W mosaic. This is sometimes needed due to SC
                * performance. */
                if ((FVID2_SF_PROGRESSIVE ==
                     pObj->inQueInfo.chInfo[chNum].scanFormat)
                            &&
                    pObj->layoutParams.winInfo[winId].bypass
                            &&
                    VPS_VPDMA_MT_NONTILEDMEM ==
                            pObj->inQueInfo.chInfo[chNum].memType
                    )
                {
                    pWinObj->scRtCropCfg.cropStartY /= 2;
                    pWinObj->scRtCropCfg.cropHeight /= 2;
                    pWinObj->scRtInFrmPrm.height /= 2;
                    pWinObj->scRtInFrmPrm.pitch[0] *= 2;
                    pWinObj->scRtInFrmPrm.pitch[1] *= 2;
                    pWinObj->scRtInFrmPrm.pitch[2] *= 2;
                }
            }
        }
        else
        {
            memset(&pWinObj->scRtPrm, 0, sizeof(pWinObj->scRtPrm));

            pWinObj->scRtPrm.outFrmPrms = &pWinObj->scRtOutFrmPrm;
            pWinObj->scRtPrm.inFrmPrms = &pWinObj->scRtInFrmPrm;
            pWinObj->scRtPrm.srcCropCfg = &pWinObj->scRtCropCfg;
            pWinObj->scRtPrm.scCfg = NULL;

            /* In case captured data is progressive and window is bypass
             * (don't care quality so much), SC takes only even lines and
             * make S/W mosaic. This is sometimes needed due to SC
             * performance. */
            if ((FVID2_SF_PROGRESSIVE ==
                 pObj->inQueInfo.chInfo[chNum].scanFormat) &&
                pObj->layoutParams.winInfo[winId].bypass   &&
                VPS_VPDMA_MT_NONTILEDMEM ==
                            pObj->inQueInfo.chInfo[chNum].memType)
            {
                pWinObj->scRtCropCfg.cropStartY /= 2;
                pWinObj->scRtCropCfg.cropHeight /= 2;
                pWinObj->scRtInFrmPrm.height /= 2;
                pWinObj->scRtInFrmPrm.pitch[0] *= 2;
                pWinObj->scRtInFrmPrm.pitch[1] *= 2;
                pWinObj->scRtInFrmPrm.pitch[2] *= 2;
            }
        }

        pWinObj->applyRtPrm = TRUE;

        pWinObj->blankFrame.channelNum = winId - pObj->DrvObj[drvInst].startWin;

        pWinObj->curOutFrame.channelNum = winId - pObj->DrvObj[drvInst].startWin;

        pWinObj->isBlankFrameDisplay = FALSE;
    }

    SwMsLink_drvSelectScCoeffs(pObj);

    SwMsLink_drvPrintLayoutParams(pObj);

    if (isLockAlredayTaken == FALSE)
    {
        SwMsLink_drvUnlock(pObj);
    }
    return FVID2_SOK;
}

Int32 SwMsLink_drvCreateOutInfo(SwMsLink_Obj * pObj, UInt32 outRes)
{
    Int32 status;
    System_LinkChInfo *pChInfo;
    UInt32 frameId, bufferPitch;
    UInt32 bufferWidth, bufferHeight;

    memset(&pObj->outFrameDrop, 0, sizeof(pObj->outFrameDrop));

    pObj->info.numQue = 1;
    pObj->info.queInfo[0].numCh = 1;

    pChInfo = &pObj->info.queInfo[0].chInfo[0];

    pChInfo->dataFormat = FVID2_DF_YUV422I_YUYV;
    pChInfo->memType = VPS_VPDMA_MT_NONTILEDMEM;

    System_getOutSize(pObj->createArgs.maxOutRes, &bufferWidth, &bufferHeight);

    bufferPitch = VpsUtils_align(bufferWidth, VPS_BUFFER_ALIGNMENT * 2) * 2;

    if(pObj->createArgs.initOutRes == SYSTEM_STD_INVALID)
    {
        pObj->createArgs.initOutRes = pObj->createArgs.maxOutRes;
    }

    System_getOutSize(pObj->createArgs.initOutRes, &pChInfo->width, &pChInfo->height);

    Vps_printf("pChInfo->width = %d, pChInfo->height = %d\n",pChInfo->width,pChInfo->height);

    pChInfo->pitch[0] = bufferPitch;
    pChInfo->pitch[1] = pChInfo->pitch[2] = 0;

    pChInfo->scanFormat = FVID2_SF_PROGRESSIVE;

    pObj->bufferFrameFormat.channelNum = 0;
    pObj->bufferFrameFormat.width = bufferWidth;
    pObj->bufferFrameFormat.height = bufferHeight;
    pObj->bufferFrameFormat.pitch[0] = pChInfo->pitch[0];
    pObj->bufferFrameFormat.pitch[1] = pChInfo->pitch[1];
    pObj->bufferFrameFormat.pitch[2] = pChInfo->pitch[2];
    pObj->bufferFrameFormat.fieldMerged[0] = FALSE;
    pObj->bufferFrameFormat.fieldMerged[1] = FALSE;
    pObj->bufferFrameFormat.fieldMerged[2] = FALSE;
    pObj->bufferFrameFormat.dataFormat = pChInfo->dataFormat;
    pObj->bufferFrameFormat.scanFormat = pChInfo->scanFormat;
    pObj->bufferFrameFormat.bpp = FVID2_BPP_BITS16;
    pObj->bufferFrameFormat.reserved = NULL;

    status = Utils_bufCreate(&pObj->bufOutQue, TRUE, FALSE);
    UTILS_assert(status == FVID2_SOK);

    /* set actual required width x height */
    pObj->outFrameFormat = pObj->bufferFrameFormat;
    pObj->outFrameFormat.width = pChInfo->width;
    pObj->outFrameFormat.height = pChInfo->height;

    for (frameId = 0; frameId < pObj->createArgs.numOutBuf; frameId++)
    {
        /* alloc buffer of max possible size but use only what is needed for a
         * given resolution */
        status = Utils_memFrameAlloc(&pObj->bufferFrameFormat,
                                     &pObj->outFrames[frameId], 1);
        UTILS_assert(status == FVID2_SOK);

        status = Utils_bufPutEmptyFrame(&pObj->bufOutQue,
                                        &pObj->outFrames[frameId]);
        UTILS_assert(status == FVID2_SOK);
        pObj->outFrameInfo[frameId].swMsBlankOutBuf = SWMS_LINK_DO_OUTBUF_BLANKING;
        pObj->outFrames[frameId].appData = &pObj->outFrameInfo[frameId];
    }

    return FVID2_SOK;
}

static
Int32 SwMsLink_drvCreateChannelObj(SwMsLink_Obj * pObj)
{
    Int i;
    Int status = FVID2_SOK;
    AvsyncLink_VidQueCreateParams cp;

    for (i = 0; i < SYSTEM_SW_MS_MAX_CH_ID; i++)
    {
        Avsync_vidQueCreateParamsInit(&cp);
        cp.chNum     = i;
        cp.maxElements = UTILS_ARRAYSIZE(pObj->chObj[i].inQueMem);
        cp.queueMem    = pObj->chObj[i].inQueMem;
        cp.syncLinkID = pObj->linkId;
        cp.displayID  = Avsync_mapDisplayLinkID2Index(pObj->createArgs.outQueParams.nextLink);
        status =
          Avsync_vidQueCreate(&cp,&pObj->chObj[i].inQue);
        UTILS_assert(status ==0);
        pObj->chObj[i].pCurInFrame = NULL;
        pObj->chObj[i].isPlaybackChannel = FALSE;
        pObj->chObj[i].isInputReceived = FALSE;
    }
    pObj->lastOutBufPtr = NULL;
    pObj->lastDisplayCBTS = 0;
    pObj->SwmsProcessTieWithDisplayLocalFlag = FALSE;
    pObj->enableOuputDupLocalFlag = FALSE;
    if (pObj->createArgs.enableProcessTieWithDisplay)
    {
        AvsyncLink_VidSyncCbHookObj  cbHookObj;

        cbHookObj.cbFxn = SwMsLink_drvDisplaySyncCb;
        cbHookObj.ctx   = pObj;
        Avsync_vidSynchRegisterCbHook(Avsync_vidQueGetDisplayID(pObj->linkId),
                                      &cbHookObj);
    }

    return status;
}

static
Int32 SwMsLink_drvDeleteChannelObj(SwMsLink_Obj * pObj)
{
    Int i;
    Int status = FVID2_SOK;

    for (i = 0; i < SYSTEM_SW_MS_MAX_CH_ID; i++)
    {
        status =
          Avsync_vidQueDelete(&pObj->chObj[i].inQue);
        UTILS_assert(status ==0);
        pObj->chObj[i].pCurInFrame = NULL;
        pObj->chObj[i].isPlaybackChannel = FALSE;
        pObj->chObj[i].isInputReceived = FALSE;
    }
    pObj->lastOutBufPtr = NULL;
    return status;
}

static
Bool  SwMsLink_isPlaybackChannel(FVID2_Frame *frame)
{
    System_FrameInfo *frameInfo;
    Bool isPlaybackChannel = FALSE;

    frameInfo = frame->appData;
    if (frameInfo)
    {
        if (frameInfo->isPlayBackChannel == TRUE)
        {
            isPlaybackChannel = TRUE;
        }
    }
    return isPlaybackChannel;
}

static
Int32 SwMsLink_drvQueueInputFrames(SwMsLink_Obj * pObj,
                                   FVID2_FrameList *frameList)
{
    Int i;
    Int status = 0;
    FVID2_Frame *frame;
    System_LinkInQueParams *pInQueParams;
    UInt32 drvInst;
    UInt32 winId,chIdx;
    SwMsLink_OutWinObj *pWinObj;
    SwMsLink_LayoutWinInfo *pWinInfo;


    pInQueParams = &pObj->createArgs.inQueParams;

    for (i = 0; i < frameList->numFrames; i++)
    {
        frame = frameList->frames[i];
        if (frame == NULL)
        {
            #ifdef SWMS_DEBUG_FRAME_REJECT
              Vps_printf(" SWMS: Frame Reject: NULL frame\n");
            #endif
            continue;
        }
        if (frame->channelNum >= SYSTEM_SW_MS_MAX_CH_ID)
        {
            #ifdef SWMS_DEBUG_FRAME_REJECT
              Vps_printf(" SWMS: Frame Reject: Invalid CH ID (%d)\n", chId);
            #endif
            /* invalid ch ID */
            SwMsLink_freeFrame(pInQueParams, &pObj->freeFrameList, frame);
            continue;
        }
        UTILS_assert(frame->channelNum < UTILS_ARRAYSIZE(pObj->chObj));
        if (!pObj->chObj[frame->channelNum].isPlaybackChannel)
        {
            pObj->chObj[frame->channelNum].isPlaybackChannel =
                SwMsLink_isPlaybackChannel(frame);
        }
        chIdx = frame->channelNum;
        winId = pObj->layoutParams.ch2WinMap[chIdx];
        if (winId < pObj->layoutParams.numWin)
        {
            pWinObj = &pObj->winObj[winId];
            pWinInfo = &pObj->layoutParams.winInfo[winId];
            drvInst = SwMsLink_getDrvInstFromWinId(pObj,winId);
            if (pWinInfo->bypass || !pObj->DrvObj[drvInst].isDeiDrv || pObj->DrvObj[drvInst].forceBypassDei)
            {
                /* window shows channel in bypass mode, then drop odd fields,
            * i.e always expect even fields */
                pWinObj->expectedFid = 0;
            }
            if (frame->fid != pWinObj->expectedFid)
            {
                 pWinObj->framesFidInvalidCount++;

                 /* incoming frame fid does not match required fid */
                 SwMsLink_freeFrame(pInQueParams, &pObj->freeFrameList, frame);
                 frame = NULL;
            }
            if (frame)
            {
                if (pObj->DrvObj[drvInst].isDeiDrv && pObj->DrvObj[drvInst].forceBypassDei==FALSE)
                {
                    pWinObj->expectedFid ^= 1;
                }
            }
        }

        if (frame)
        {
            status = Avsync_vidQuePut(&pObj->chObj[frame->channelNum].inQue,
                              frame);
            UTILS_assert(status == 0);
        }

    }
    return status;
}


static
Int32 SwMsLink_freeFrameList(SwMsLink_Obj * pObj,
                             FVID2_FrameList * freeFrameList)
{
    System_LinkInQueParams *pInQueParams;

    pInQueParams = &pObj->createArgs.inQueParams;

    UTILS_assert (freeFrameList->numFrames < FVID2_MAX_FVID_FRAME_PTR);
    if (freeFrameList->numFrames >= (FVID2_MAX_FVID_FRAME_PTR/4))
    {
        System_putLinksEmptyFrames(pInQueParams->prevLinkId,
                                   pInQueParams->prevLinkQueId, freeFrameList);

        freeFrameList->numFrames = 0;
    }
    return 0;
}


static
Int32 SwMsLink_drvUpdateChCurrentInFrame(SwMsLink_Obj * pObj)
{
    Int chIdx,chCount;
    Int status = 0;
    FVID2_Frame *frame;
    UInt32 winId;
    System_LinkInQueParams *pInQueParams;
    UInt32 drvInst,skipInputFrames;
    SwMsLink_OutWinObj *pWinObj;
    SwMsLink_LayoutWinInfo *pWinInfo;
    UInt32 syncMasterCh;

    pInQueParams = &pObj->createArgs.inQueParams;
    syncMasterCh = Avsync_vidQueGetMasterSynchChannel(pObj->linkId);
    chIdx = syncMasterCh;
    if (chIdx == AVSYNC_INVALID_CHNUM)
    {
        chIdx = 0;
    }

    for (chCount = 0;
         chCount < pObj->inQueInfo.numCh;
         chCount++)
    {
        winId = pObj->layoutParams.ch2WinMap[chIdx];
        pObj->chObj[chIdx].isInputReceived = FALSE;
        if ((winId >= pObj->layoutParams.numWin)
            &&
            (chIdx != syncMasterCh))
        {
            frame = NULL;
            /* channel is not mapped to active window.
             * Free all queued input frames
             */
           Avsync_vidQueFlush(&pObj->chObj[chIdx].inQue,
                              &frame,
                              &pObj->freeFrameList);
           SwMsLink_freeFrameList(pObj,&pObj->freeFrameList);
           if (frame)
           {
               if (pObj->chObj[chIdx].pCurInFrame != NULL)
               {
                   SwMsLink_freeFrame(pInQueParams,&pObj->freeFrameList,
                                      pObj->chObj[chIdx].pCurInFrame);
                   pObj->chObj[chIdx].pCurInFrame = NULL;
               }
               if (pObj->chObj[chIdx].isPlaybackChannel)
               {
                   pObj->chObj[chIdx].pCurInFrame = frame;
                   pObj->chObj[chIdx].isInputReceived = TRUE;
               }
               else
               {
                   SwMsLink_freeFrame(pInQueParams,&pObj->freeFrameList,
                                      frame);
               }
           }
        }
        else
        {
            UInt32 queCnt;

            frame = NULL;
            pWinObj = &pObj->winObj[winId];
            pWinInfo = &pObj->layoutParams.winInfo[winId];
            drvInst = SwMsLink_getDrvInstFromWinId(pObj,winId);
            queCnt = Avsync_vidQueGetQueLength(&pObj->chObj[chIdx].inQue);

            if (SYSTEM_SW_MS_DEFAULT_INPUT_QUE_LEN == pObj->createArgs.maxInputQueLen)
            {
                pObj->createArgs.maxInputQueLen = SW_MS_SKIP_INPUT_FRAMES_SC;
                if(pObj->DrvObj[drvInst].isDeiDrv && pObj->DrvObj[drvInst].forceBypassDei == FALSE)
                {
                    pObj->createArgs.maxInputQueLen = SW_MS_SKIP_INPUT_FRAMES_DEI;
                }
            }
            else
            {
                if (SYSTEM_SW_MS_INVALID_INPUT_QUE_LEN != pObj->createArgs.maxInputQueLen)
                {
                    if(pObj->DrvObj[drvInst].isDeiDrv && pObj->DrvObj[drvInst].forceBypassDei == FALSE)
                    {
                        /* Make sure that if we use DEI we drop even number of fields */
                        if ((pObj->createArgs.maxInputQueLen & 0x1) == 0)
                        {
                            pObj->createArgs.maxInputQueLen++;
                        }
                    }
                }
            }
            skipInputFrames = pObj->createArgs.maxInputQueLen;
            if ((queCnt > skipInputFrames)
                &&
                (!(AVSYNC_VIDQUE_IS_SYNCH_ENABLED(&pObj->chObj[chIdx].inQue))))
            {
                UInt32 numFreeFramePreFlush = pObj->freeFrameList.numFrames;
                pWinObj->framesAccEventCount++;
                if(queCnt>pWinObj->framesAccMax)
                    pWinObj->framesAccMax = queCnt;
                if(queCnt<pWinObj->framesAccMin)
                    pWinObj->framesAccMin = queCnt;
                pWinObj->framesRecvCount    += queCnt;
                Avsync_vidQueFlush(&pObj->chObj[chIdx].inQue,
                                   &frame,
                                   &pObj->freeFrameList);
                pWinObj->framesDroppedCount += pObj->freeFrameList.numFrames -
                                               numFreeFramePreFlush;
                SwMsLink_freeFrameList(pObj,&pObj->freeFrameList);
            }
            else
            {
                UInt32 numFreeFramePreGet = pObj->freeFrameList.numFrames;
                Avsync_vidQueGet(&pObj->chObj[chIdx].inQue,
                                 FALSE,
                                 &frame,
                                 &pObj->freeFrameList);
                pWinObj->framesDroppedCount += pObj->freeFrameList.numFrames -
                                              numFreeFramePreGet;
                SwMsLink_freeFrameList(pObj,&pObj->freeFrameList);
            }
            if (frame != NULL)
            {
                if (pWinInfo->channelNum != frame->channelNum)
                {
                    #ifdef SWMS_DEBUG_FRAME_REJECT
                        Vps_printf
                        (" SWMS: Frame Reject: Win not mapped to this channel (%d)\n",
                         chId);
                    #endif
                   pWinObj->framesInvalidChCount++;

                   /* win is not assigned to this ch, normally this condition
                    * wont happen */
                   SwMsLink_freeFrame(pInQueParams, &pObj->freeFrameList, frame);
                   frame = NULL;
                }
            }

            if (frame)
            {
                if (pObj->chObj[chIdx].pCurInFrame != NULL)
                {
                    SwMsLink_freeFrame(pInQueParams,
                                       &pObj->freeFrameList,
                                       pObj->chObj[chIdx].pCurInFrame);
                }
                pObj->chObj[chIdx].pCurInFrame = frame;
                pObj->chObj[chIdx].isInputReceived = TRUE;
                pWinObj->framesQueuedCount++;
                /* toggle next expected fid */
            }
            else
            {
                pWinObj->framesRepeatCount++;
            }
        }
        chIdx = (chIdx + 1) % SYSTEM_SW_MS_MAX_CH_ID;
    }
    return status;
}

Int32 SwMsLink_makeBlankFrameInfo(SwMsLink_Obj * pObj)
{
    SwMsLink_OutWinObj *pWinObj;
    Int32 status;

    pObj->blankBufferFormat.channelNum      = 0;
    pObj->blankBufferFormat.width           = 320+32;
    pObj->blankBufferFormat.height          = 240+24;
    pObj->blankBufferFormat.fieldMerged[0]  = FALSE;
    pObj->blankBufferFormat.fieldMerged[1]  = FALSE;
    pObj->blankBufferFormat.fieldMerged[2]  = FALSE;
    pObj->blankBufferFormat.pitch[0]        = VpsUtils_align(pObj->blankBufferFormat.width*2, VPS_BUFFER_ALIGNMENT);
    pObj->blankBufferFormat.pitch[1]        = pObj->blankBufferFormat.pitch[0];
    pObj->blankBufferFormat.pitch[2]        = pObj->blankBufferFormat.pitch[0];
    pObj->blankBufferFormat.dataFormat      = FVID2_DF_YUV422I_YUYV;
    pObj->blankBufferFormat.scanFormat      = FVID2_SF_PROGRESSIVE;
    pObj->blankBufferFormat.bpp             = FVID2_BPP_BITS12;
    pObj->blankBufferFormat.reserved        = NULL;

    pObj->blankFrameScRtInFrmPrm.width      = pObj->blankBufferFormat.width;
    pObj->blankFrameScRtInFrmPrm.height     = pObj->blankBufferFormat.height;
    pObj->blankFrameScRtInFrmPrm.pitch[0]   = pObj->blankBufferFormat.pitch[0];
    pObj->blankFrameScRtInFrmPrm.pitch[1]   = pObj->blankBufferFormat.pitch[1];
    pObj->blankFrameScRtInFrmPrm.pitch[2]   = pObj->blankBufferFormat.pitch[2];
    pObj->blankFrameScRtInFrmPrm.memType    = VPS_VPDMA_MT_NONTILEDMEM;
    pObj->blankFrameScRtInFrmPrm.dataFormat = pObj->blankBufferFormat.dataFormat;

    pObj->blankFrameScRtCropCfg.cropStartX  = 0;
    pObj->blankFrameScRtCropCfg.cropStartY  = 0;
    pObj->blankFrameScRtCropCfg.cropWidth   = pObj->blankFrameScRtInFrmPrm.width;
    pObj->blankFrameScRtCropCfg.cropHeight  = pObj->blankFrameScRtInFrmPrm.height;

    pWinObj = &pObj->winObj[0];

    /* alloc buffer of max possible size as input blank buffer */
    status = System_getBlankFrame(&pWinObj->blankFrame);
    UTILS_assert(status == FVID2_SOK);
    pWinObj->blankFrame.addr[0][1] = pWinObj->blankFrame.addr[0][0];
    SwMsLink_fillBlankDataPattern(&pObj->blankBufferFormat,
                             &pWinObj->blankFrame);

    return FVID2_SOK;
}

Int32 SwMsLink_drvCreateWinObj(SwMsLink_Obj * pObj, UInt32 winId)
{
    SwMsLink_OutWinObj *pWinObj;
    Int32 status = FVID2_SOK;

    pWinObj = &pObj->winObj[winId];

    /* assume all CHs are of same input size, format, pitch */
    pWinObj->scanFormat =
        (FVID2_ScanFormat) pObj->inQueInfo.chInfo[0].scanFormat;
    pWinObj->expectedFid = 0;
    pWinObj->applyRtPrm = FALSE;

    pWinObj->isBlankFrameDisplay = FALSE;

    if (winId == 0)
    {
        SwMsLink_makeBlankFrameInfo(pObj);
    }
    else
    {
        pWinObj->blankFrame = pObj->winObj[0].blankFrame;
    }

    pWinObj->blankFrame.channelNum = winId;

    memset(&pWinObj->curOutFrame, 0, sizeof(pWinObj->curOutFrame));
    pWinObj->curOutFrame.addr[0][0] = NULL;
    pWinObj->curOutFrame.channelNum = winId;

    return status;
}

Int32 SwMsLink_drvAllocCtxMem(SwMsLink_DrvObj * pObj)
{
    Int32 retVal = FVID2_SOK;
    Vps_DeiCtxInfo deiCtxInfo;
    Vps_DeiCtxBuf deiCtxBuf;
    UInt32 chCnt, bCnt;

    for (chCnt = 0u; chCnt < pObj->cfg.dei.deiCreateParams.numCh; chCnt++)
    {
        /* Get the number of buffers to allocate */
        deiCtxInfo.channelNum = chCnt;
        retVal = FVID2_control(pObj->fvidHandle,
                               IOCTL_VPS_GET_DEI_CTX_INFO, &deiCtxInfo, NULL);
        UTILS_assert(FVID2_SOK == retVal);

        /* Allocate the buffers as requested by the driver */
        for (bCnt = 0u; bCnt < deiCtxInfo.numFld; bCnt++)
        {
            deiCtxBuf.fldBuf[bCnt] = Utils_memAlloc(deiCtxInfo.fldBufSize,
                                                    VPS_BUFFER_ALIGNMENT);
            UTILS_assert(NULL != deiCtxBuf.fldBuf[bCnt]);
        }
        for (bCnt = 0u; bCnt < deiCtxInfo.numMv; bCnt++)
        {
            deiCtxBuf.mvBuf[bCnt] = Utils_memAlloc(deiCtxInfo.mvBufSize,
                                                   VPS_BUFFER_ALIGNMENT);
            UTILS_assert(NULL != deiCtxBuf.mvBuf[bCnt]);
        }
        for (bCnt = 0u; bCnt < deiCtxInfo.numMvstm; bCnt++)
        {
            deiCtxBuf.mvstmBuf[bCnt] = Utils_memAlloc(deiCtxInfo.mvstmBufSize,
                                                      VPS_BUFFER_ALIGNMENT);
            UTILS_assert(NULL != deiCtxBuf.mvstmBuf[bCnt]);
        }

        /* Provided the allocated buffer to driver */
        deiCtxBuf.channelNum = chCnt;
        retVal = FVID2_control(pObj->fvidHandle,
                               IOCTL_VPS_SET_DEI_CTX_BUF, &deiCtxBuf, NULL);
        UTILS_assert(FVID2_SOK == retVal);
    }

    return (retVal);
}

Int32 SwMsLink_drvFreeCtxMem(SwMsLink_DrvObj * pObj)
{
    Int32 retVal = FVID2_SOK;
    Vps_DeiCtxInfo deiCtxInfo;
    Vps_DeiCtxBuf deiCtxBuf;
    UInt32 chCnt, bCnt;

    for (chCnt = 0u; chCnt < pObj->cfg.dei.deiCreateParams.numCh; chCnt++)
    {
        /* Get the number of buffers to allocate */
        deiCtxInfo.channelNum = chCnt;
        retVal = FVID2_control(pObj->fvidHandle,
                               IOCTL_VPS_GET_DEI_CTX_INFO, &deiCtxInfo, NULL);
        UTILS_assert(FVID2_SOK == retVal);

        /* Get the allocated buffer back from the driver */
        deiCtxBuf.channelNum = chCnt;
        retVal = FVID2_control(pObj->fvidHandle,
                               IOCTL_VPS_GET_DEI_CTX_BUF, &deiCtxBuf, NULL);
        UTILS_assert(FVID2_SOK == retVal);

        /* Free the buffers */
        for (bCnt = 0u; bCnt < deiCtxInfo.numFld; bCnt++)
        {
            Utils_memFree(deiCtxBuf.fldBuf[bCnt], deiCtxInfo.fldBufSize);
        }
        for (bCnt = 0u; bCnt < deiCtxInfo.numMv; bCnt++)
        {
            Utils_memFree(deiCtxBuf.mvBuf[bCnt], deiCtxInfo.mvBufSize);
        }
        for (bCnt = 0u; bCnt < deiCtxInfo.numMvstm; bCnt++)
        {
            Utils_memFree(deiCtxBuf.mvstmBuf[bCnt], deiCtxInfo.mvstmBufSize);
        }
    }

    return (retVal);
}

Int32 SwMsLink_drvCreateDeiDrv(SwMsLink_Obj * pObj, SwMsLink_DrvObj *pDrvObj)
{
    Semaphore_Params semParams;
    Vps_M2mDeiChParams *pDrvChPrm;
    UInt32 winId;
    System_LinkChInfo *pChInfo;
    FVID2_CbParams cbParams;

    Semaphore_Params_init(&semParams);

    semParams.mode = Semaphore_Mode_BINARY;

    pDrvObj->complete = Semaphore_create(0u, &semParams, NULL);
    UTILS_assert(pDrvObj->complete != NULL);

    pDrvObj->cfg.dei.deiCreateParams.mode = VPS_M2M_CONFIG_PER_CHANNEL;
    if (pDrvObj->bypassDei)
    {
        pDrvObj->cfg.dei.deiCreateParams.numCh = pDrvObj->endWin - pDrvObj->startWin + 1;
    }
    else
    {
        pDrvObj->cfg.dei.deiCreateParams.numCh = SW_MS_MAX_DEI_CH;
    }
    pDrvObj->cfg.dei.deiCreateParams.deiHqCtxMode = VPS_DEIHQ_CTXMODE_DRIVER_ALL;
    pDrvObj->cfg.dei.deiCreateParams.chParams =
        (const Vps_M2mDeiChParams *) pDrvObj->cfg.dei.deiChParams;

    Vps_rprintf(" %d: SWMS    : VipScReq is %s!!!\n",
                Utils_getCurTimeInMsec(), pDrvObj->cfg.dei.deiCreateParams.isVipScReq == TRUE ? "TRUE" : "FALSE" );

    for (winId = 0; winId < pDrvObj->cfg.dei.deiCreateParams.numCh; winId++)
    {
        pDrvChPrm = &pDrvObj->cfg.dei.deiChParams[winId];

        /* assume all CHs are of same input size, format, pitch */
        pChInfo = &pObj->inQueInfo.chInfo[0];

        pDrvChPrm->inFmt.channelNum = winId;

        if(pChInfo->width > SW_MS_MAX_WIDTH_SUPPORTED)
        {
            pChInfo->width = SW_MS_MAX_WIDTH_SUPPORTED;
        }

        if(pChInfo->height > SW_MS_MAX_HEIGHT_SUPPORTED)
        {
            pChInfo->height = SW_MS_MAX_HEIGHT_SUPPORTED;
        }

        pDrvChPrm->inFmt.width = pChInfo->width;
        pDrvChPrm->inFmt.height = pChInfo->height;
        pDrvChPrm->inFmt.pitch[0] = pChInfo->pitch[0];
        pDrvChPrm->inFmt.pitch[1] = pChInfo->pitch[1];
        pDrvChPrm->inFmt.pitch[2] = pChInfo->pitch[2];
        pDrvChPrm->inFmt.fieldMerged[0] = FALSE;
        pDrvChPrm->inFmt.fieldMerged[1] = FALSE;
        pDrvChPrm->inFmt.fieldMerged[0] = FALSE;
        pDrvChPrm->inFmt.dataFormat = pChInfo->dataFormat;
        if (pDrvObj->bypassDei)
        {
            pDrvChPrm->inFmt.scanFormat = FVID2_SF_PROGRESSIVE;
        }
        else
        {
            pDrvChPrm->inFmt.scanFormat = FVID2_SF_INTERLACED;
        }
        pDrvChPrm->inFmt.bpp = FVID2_BPP_BITS16;

        pDrvChPrm->outFmtDei = &pDrvObj->drvOutFormat[winId];
        pDrvChPrm->outFmtDei->channelNum = winId;
        pDrvChPrm->outFmtDei->width = pObj->outFrameFormat.width;
        pDrvChPrm->outFmtDei->height = pObj->outFrameFormat.height;
        pDrvChPrm->outFmtDei->pitch[0] = pObj->outFrameFormat.pitch[0];
        pDrvChPrm->outFmtDei->pitch[1] = pObj->outFrameFormat.pitch[1];
        pDrvChPrm->outFmtDei->pitch[2] = pObj->outFrameFormat.pitch[2];
        pDrvChPrm->outFmtDei->fieldMerged[0] = FALSE;
        pDrvChPrm->outFmtDei->fieldMerged[1] = FALSE;
        pDrvChPrm->outFmtDei->fieldMerged[0] = FALSE;
        pDrvChPrm->outFmtDei->dataFormat = pObj->outFrameFormat.dataFormat;
        pDrvChPrm->outFmtDei->scanFormat = FVID2_SF_PROGRESSIVE;
        pDrvChPrm->outFmtDei->bpp = pObj->outFrameFormat.bpp;

        pDrvChPrm->inMemType = pChInfo->memType;
        pDrvChPrm->outMemTypeDei = VPS_VPDMA_MT_NONTILEDMEM;
        pDrvChPrm->outMemTypeVip = VPS_VPDMA_MT_NONTILEDMEM;
        pDrvChPrm->drnEnable = FALSE;
        pDrvChPrm->comprEnable = FALSE;

        pDrvChPrm->deiHqCfg = &pDrvObj->cfg.dei.deiHqCfg;
        pDrvChPrm->deiCfg = &pDrvObj->cfg.dei.deiCfg;

        pDrvChPrm->scCfg = &pDrvObj->scCfg;
        pDrvChPrm->deiCropCfg = &pDrvObj->scCropCfg[winId];

        pDrvChPrm->deiHqCfg->bypass = pDrvObj->bypassDei;
        pDrvChPrm->deiHqCfg->inpMode = VPS_DEIHQ_EDIMODE_EDI_LARGE_WINDOW;
        pDrvChPrm->deiHqCfg->tempInpEnable = TRUE;
        pDrvChPrm->deiHqCfg->tempInpChromaEnable = TRUE;
        pDrvChPrm->deiHqCfg->spatMaxBypass = FALSE;
        pDrvChPrm->deiHqCfg->tempMaxBypass = FALSE;
        pDrvChPrm->deiHqCfg->fldMode = VPS_DEIHQ_FLDMODE_5FLD;
        pDrvChPrm->deiHqCfg->lcModeEnable = TRUE;
        pDrvChPrm->deiHqCfg->mvstmEnable = TRUE;
        if (pDrvObj->bypassDei)
        {
            pDrvChPrm->deiHqCfg->tnrEnable = FALSE;
        }
        else
        {
            pDrvChPrm->deiHqCfg->tnrEnable = TRUE;
        }
        pDrvChPrm->deiHqCfg->snrEnable = TRUE;
        pDrvChPrm->deiHqCfg->sktEnable = FALSE;
        pDrvChPrm->deiHqCfg->chromaEdiEnable = TRUE;

        pDrvChPrm->deiCfg->bypass = pDrvObj->bypassDei;
        pDrvChPrm->deiCfg->inpMode = VPS_DEIHQ_EDIMODE_EDI_LARGE_WINDOW;
        pDrvChPrm->deiCfg->tempInpEnable = TRUE;
        pDrvChPrm->deiCfg->tempInpChromaEnable = TRUE;
        pDrvChPrm->deiCfg->spatMaxBypass = FALSE;
        pDrvChPrm->deiCfg->tempMaxBypass = FALSE;

        pDrvChPrm->scCfg->bypass = FALSE;
        pDrvChPrm->scCfg->nonLinear = FALSE;
        pDrvChPrm->scCfg->stripSize = 0;
        pDrvChPrm->scCfg->vsType = VPS_SC_VST_POLYPHASE;

        pDrvChPrm->deiCropCfg->cropStartX = 0;
        pDrvChPrm->deiCropCfg->cropStartY = 0;
        pDrvChPrm->deiCropCfg->cropWidth = pDrvChPrm->inFmt.width;
        if (pDrvObj->bypassDei)
        {
            pDrvChPrm->deiCropCfg->cropHeight = pDrvChPrm->inFmt.height;
        }
        else
        {
            pDrvChPrm->deiCropCfg->cropHeight = pDrvChPrm->inFmt.height * 2;
        }

        /* VIP-SC settings - set some random out width & height as we dont really use the vip sc out */
        if (pDrvObj->cfg.dei.deiCreateParams.isVipScReq == TRUE)
        {
            pDrvChPrm->outFmtVip = &pDrvObj->drvVipOutFormat[winId];
            pDrvChPrm->outFmtVip->channelNum = winId;
            pDrvChPrm->outFmtVip->width = 352;
            pDrvChPrm->outFmtVip->height = 288;
            pDrvChPrm->outFmtVip->pitch[0] = pDrvChPrm->outFmtVip->width*2;
            pDrvChPrm->outFmtVip->pitch[1] = pDrvChPrm->outFmtVip->pitch[0];
            pDrvChPrm->outFmtVip->pitch[2] = 0;
            pDrvChPrm->outFmtVip->fieldMerged[0] = FALSE;
            pDrvChPrm->outFmtVip->fieldMerged[1] = FALSE;
            pDrvChPrm->outFmtVip->fieldMerged[0] = FALSE;
            pDrvChPrm->outFmtVip->dataFormat = pObj->outFrameFormat.dataFormat;
            pDrvChPrm->outFmtVip->scanFormat = FVID2_SF_PROGRESSIVE;
            pDrvChPrm->outFmtVip->bpp = pObj->outFrameFormat.bpp;

            pDrvChPrm->vipScCfg = &pDrvObj->vipScCfg;
            pDrvChPrm->vipCropCfg = &pDrvObj->vipScCropCfg[winId];

            pDrvChPrm->vipScCfg->bypass = TRUE;
            pDrvChPrm->vipScCfg->nonLinear = FALSE;
            pDrvChPrm->vipScCfg->stripSize = 0;
            pDrvChPrm->vipScCfg->vsType = VPS_SC_VST_POLYPHASE;

            pDrvChPrm->vipCropCfg->cropStartX = 0;
            pDrvChPrm->vipCropCfg->cropStartY = 0;
            pDrvChPrm->vipCropCfg->cropWidth = pDrvChPrm->inFmt.width;
            if (pDrvObj->bypassDei)
            {
                pDrvChPrm->vipCropCfg->cropHeight = pDrvChPrm->inFmt.height;
            }
            else
            {
                pDrvChPrm->vipCropCfg->cropHeight = pDrvChPrm->inFmt.height * 2;
            }
        }

        #if defined(TI_814X_BUILD) || defined(TI_8107_BUILD)

        pDrvChPrm->deiHqCfg = NULL;

        /* for SC2 driver deiHqCfg and deiCfg MUST be NULL */
        if ((pDrvObj->drvInstId == VPS_M2M_INST_AUX_SC2_WB1) ||
            (pDrvObj->drvInstId == VPS_M2M_INST_AUX_SC4_VIP1) ||
            (pDrvObj->drvInstId == VPS_M2M_INST_AUX_SC2_SC4_WB1_VIP1))
        {
            pDrvChPrm->deiCfg = NULL;
        }
        #endif

        pDrvChPrm->subFrameParams = NULL;
    }

    memset(&cbParams, 0, sizeof(cbParams));

    cbParams.cbFxn = SwMsLink_drvFvidCb;
    cbParams.errCbFxn = SwMsLink_drvFvidErrCb;
    cbParams.errList = &pDrvObj->errCbProcessList;
    cbParams.appData = pDrvObj;

    pDrvObj->fvidHandle = FVID2_create(FVID2_VPS_M2M_DEI_DRV,
                                       pDrvObj->drvInstId,
                                       &pDrvObj->cfg.dei.deiCreateParams,
                                       &pDrvObj->cfg.dei.deiCreateStatus, &cbParams);
    UTILS_assert(pDrvObj->fvidHandle != NULL);

    if(pDrvObj->bypassDei == FALSE)
        SwMsLink_drvAllocCtxMem(pDrvObj);

    /* load co-effs only once */
    if (
        pDrvObj->bypassDei == FALSE
        ||
        ( /* if DEI is in force bypass mode,
            then DEI in DEI mode driver is not created hence need load co-effs here
            */
            pDrvObj->bypassDei == TRUE
            &&
            pDrvObj->forceBypassDei == TRUE
        )
        )
    {
        SwMsLink_drvSetScCoeffs(pObj, pDrvObj->fvidHandle, 
                                VPS_SC_US_SET, VPS_SC_US_SET, TRUE);
    }

    pDrvObj->processList.numInLists = 1;
    pDrvObj->processList.numOutLists = 1;
    pDrvObj->processList.inFrameList[0] = &pDrvObj->inFrameList;
    pDrvObj->processList.outFrameList[0] = &pDrvObj->outFrameList;

    /* If in dual out mode, initialize VIP-SC outs to dummy frames */
    if (pDrvObj->cfg.dei.deiCreateParams.isVipScReq == TRUE)
    {
        Int32 i;

        pDrvObj->processList.numOutLists = 2;
        pDrvObj->processList.outFrameList[1] = &pObj->outFrameDropList;

        for (i=0; i<FVID2_MAX_FVID_FRAME_PTR; i++)
            pObj->outFrameDropList.frames[i] = &pObj->outFrameDrop;
        Vps_rprintf(" %d: SWMS    : OutFrames List -> %d !!!!!!!\n", Utils_getCurTimeInMsec(), pDrvObj->processList.numOutLists);
    }
    return FVID2_SOK;
}

Int32 SwMsLink_drvCreateScDrv(SwMsLink_Obj * pObj, SwMsLink_DrvObj *pDrvObj)
{
    Semaphore_Params semParams;
    Vps_M2mScChParams *pDrvChPrm;
    UInt32 winId;
    System_LinkChInfo *pChInfo;
    FVID2_CbParams cbParams;

    Semaphore_Params_init(&semParams);
    semParams.mode = Semaphore_Mode_BINARY;
    pDrvObj->complete = Semaphore_create(0u, &semParams, NULL);
    UTILS_assert(pDrvObj->complete != NULL);

    pDrvObj->cfg.sc.scCreateParams.mode = VPS_M2M_CONFIG_PER_CHANNEL;
    pDrvObj->cfg.sc.scCreateParams.numChannels = SYSTEM_SW_MS_MAX_WIN_PER_SC;
    pDrvObj->cfg.sc.scCreateParams.chParams
        = (Vps_M2mScChParams *) pDrvObj->cfg.sc.scChParams;

    for (winId = 0; winId < pDrvObj->cfg.sc.scCreateParams.numChannels; winId++)
    {
        pDrvChPrm = &pDrvObj->cfg.sc.scChParams[winId];

        /* assume all CHs are of same input size, format, pitch */
        pChInfo = &pObj->inQueInfo.chInfo[0];

        pDrvChPrm->inFmt.channelNum = winId;

        if(pChInfo->width > SW_MS_MAX_WIDTH_SUPPORTED)
        {
            pChInfo->width = SW_MS_MAX_WIDTH_SUPPORTED;
        }

        if(pChInfo->height > SW_MS_MAX_HEIGHT_SUPPORTED)
        {
            pChInfo->height = SW_MS_MAX_HEIGHT_SUPPORTED;
        }

        pDrvChPrm->inFmt.width = pChInfo->width;
        pDrvChPrm->inFmt.height = pChInfo->height;
        pDrvChPrm->inFmt.pitch[0] = pChInfo->pitch[0];
        pDrvChPrm->inFmt.pitch[1] = pChInfo->pitch[1];
        pDrvChPrm->inFmt.pitch[2] = pChInfo->pitch[2];
        pDrvChPrm->inFmt.fieldMerged[0] = FALSE;
        pDrvChPrm->inFmt.fieldMerged[1] = FALSE;
        pDrvChPrm->inFmt.fieldMerged[0] = FALSE;
        pDrvChPrm->inFmt.dataFormat = pChInfo->dataFormat;
        pDrvChPrm->inFmt.scanFormat = FVID2_SF_PROGRESSIVE;
        pDrvChPrm->inFmt.bpp = FVID2_BPP_BITS16;

        pDrvChPrm->outFmt.channelNum = winId;
        pDrvChPrm->outFmt.width = pObj->outFrameFormat.width;
        pDrvChPrm->outFmt.height = pObj->outFrameFormat.height;
        pDrvChPrm->outFmt.pitch[0] = pObj->outFrameFormat.pitch[0];
        pDrvChPrm->outFmt.pitch[1] = pObj->outFrameFormat.pitch[1];
        pDrvChPrm->outFmt.pitch[2] = pObj->outFrameFormat.pitch[2];
        pDrvChPrm->outFmt.fieldMerged[0] = FALSE;
        pDrvChPrm->outFmt.fieldMerged[1] = FALSE;
        pDrvChPrm->outFmt.fieldMerged[0] = FALSE;
        pDrvChPrm->outFmt.dataFormat = pObj->outFrameFormat.dataFormat;
        pDrvChPrm->outFmt.scanFormat = FVID2_SF_PROGRESSIVE;
        pDrvChPrm->outFmt.bpp = pObj->outFrameFormat.bpp;

        pDrvChPrm->inMemType = pChInfo->memType;
        pDrvChPrm->outMemType = VPS_VPDMA_MT_NONTILEDMEM;

        pDrvChPrm->scCfg = &pDrvObj->scCfg;
        pDrvChPrm->srcCropCfg = &pDrvObj->scCropCfg[winId];

        pDrvChPrm->scCfg->bypass = FALSE;
        pDrvChPrm->scCfg->nonLinear = FALSE;
        pDrvChPrm->scCfg->stripSize = 0;
        pDrvChPrm->scCfg->vsType = VPS_SC_VST_POLYPHASE;

        pDrvChPrm->srcCropCfg->cropStartX = 0;
        pDrvChPrm->srcCropCfg->cropStartY = 0;
        pDrvChPrm->srcCropCfg->cropWidth = pDrvChPrm->inFmt.width;
        pDrvChPrm->srcCropCfg->cropHeight = pDrvChPrm->inFmt.height;
    }

    memset(&cbParams, 0, sizeof(cbParams));
    cbParams.cbFxn = SwMsLink_drvFvidCb;
    cbParams.errCbFxn = SwMsLink_drvFvidErrCb;
    cbParams.errList = &pDrvObj->errCbProcessList;
    cbParams.appData = pDrvObj;

    pDrvObj->fvidHandle = FVID2_create(FVID2_VPS_M2M_SC_DRV,
                                       pDrvObj->drvInstId,
                                       &pDrvObj->cfg.sc.scCreateParams,
                                       &pDrvObj->cfg.sc.scCreateStatus, &cbParams);
    UTILS_assert(pDrvObj->fvidHandle != NULL);

    SwMsLink_drvSetScCoeffs(pObj, pDrvObj->fvidHandle, 
                            VPS_SC_US_SET, VPS_SC_US_SET, TRUE);

    pDrvObj->processList.numInLists = 1;
    pDrvObj->processList.numOutLists = 1;
    pDrvObj->processList.inFrameList[0] = &pDrvObj->inFrameList;
    pDrvObj->processList.outFrameList[0] = &pDrvObj->outFrameList;

    return FVID2_SOK;
}

Int32 SwMsLink_drvCreate(SwMsLink_Obj * pObj, SwMsLink_CreateParams * pPrm)
{
    Semaphore_Params semParams;
    Clock_Params clockParams;
    UInt32 winId;
    Int32 status;
    UInt32 i;
    Bool vip0Exist = FALSE;
    Bool sc5Exist  = FALSE;
    Vps_PlatformCpuRev cpuRev;
    UInt32 chId;

#ifdef SYSTEM_DEBUG_SWMS
    Vps_printf(" %d: SWMS: Create in progress !!!\n", Utils_getCurTimeInMsec());
#endif


    UTILS_MEMLOG_USED_START();
    SwMsLink_drvResetAvgStatistics(pObj);
    pObj->inFramePutCount = 0;
    pObj->inFrameGetCount = 0;

    pObj->frameCount = 0;
    pObj->totalTime = 0;

    pObj->skipProcessing = 0;
    pObj->switchLayout = FALSE;
    pObj->rtParamUpdate = FALSE;
    pObj->vipLockRequired = FALSE;

    memcpy(&pObj->createArgs, pPrm, sizeof(*pPrm));

    if(pObj->createArgs.numOutBuf==0)
        pObj->createArgs.numOutBuf = SW_MS_LINK_MAX_OUT_FRAMES_DEFAULT;

    if(pObj->createArgs.numOutBuf>SW_MS_LINK_MAX_OUT_FRAMES)
        pObj->createArgs.numOutBuf = SW_MS_LINK_MAX_OUT_FRAMES;

    memset(pObj->winObj, 0, sizeof(pObj->winObj));

    for(i = 0; i < (2 * SYSTEM_SW_MS_MAX_INST); i++)
    {
        memset(&pObj->DrvObj[i], 0, sizeof(pObj->DrvObj[i]));
    }

    status = System_linkGetInfo(pPrm->inQueParams.prevLinkId, &pObj->inTskInfo);
    UTILS_assert(status == FVID2_SOK);
    UTILS_assert(pPrm->inQueParams.prevLinkQueId < pObj->inTskInfo.numQue);

    memcpy(&pObj->inQueInfo,
           &pObj->inTskInfo.queInfo[pPrm->inQueParams.prevLinkQueId],
           sizeof(pObj->inQueInfo));

    cpuRev = Vps_platformGetCpuRev();

    for (chId = 0; chId < pObj->inQueInfo.numCh; chId++)
    {
        if (cpuRev < VPS_PLATFORM_CPU_REV_2_0)
        {
            if((pObj->inQueInfo.chInfo[chId].width >
                DEI_SC_DRV_422FMT_MAX_WIDTH_LIMIT_BEFORE_CPU_REV_2_0) &&
               ((pObj->inQueInfo.chInfo[chId].dataFormat != FVID2_DF_YUV420SP_UV) &&
                (pObj->inQueInfo.chInfo[chId].dataFormat != FVID2_DF_YUV420SP_VU) &&
                (pObj->inQueInfo.chInfo[chId].dataFormat != FVID2_DF_YUV420P)))
            {

                Vps_printf(" %u: Warning: This CPU Revision [%s] does not"
                             "support current set width %d\n",\
                    Utils_getCurTimeInMsec(), gVpss_cpuVer[cpuRev],\
                                              pObj->inQueInfo.chInfo[chId].width);
                Vps_printf(" %u: Warning: Limiting Input width to 960\n",
                          Utils_getCurTimeInMsec());
                {
                    pObj->inQueInfo.chInfo[chId].width =
                          DEI_SC_DRV_422FMT_MAX_WIDTH_LIMIT_BEFORE_CPU_REV_2_0;
                }
            }
        }
    }

    SwMsLink_drvGetTimerPeriod(pObj, &pPrm->layoutPrm);
    SwMsLink_drvCreateDupObj(pObj);

    Semaphore_Params_init(&semParams);
    semParams.mode = Semaphore_Mode_BINARY;
    pObj->lock = Semaphore_create(1u, &semParams, NULL);
    UTILS_assert(pObj->lock != NULL);

    Clock_Params_init(&clockParams);
    clockParams.period = pObj->timerPeriod;
    clockParams.arg = (UArg) pObj;

    pObj->timer = Clock_create(SwMsLink_drvTimerCb,
                               pObj->timerPeriod, &clockParams, NULL);
    UTILS_assert(pObj->timer != NULL);

    SwMsLink_drvCreateOutInfo(pObj, pPrm->maxOutRes);

    status = SwMsLink_drvDmaCreate(pObj);
    UTILS_assert(0 == status);

    for (winId = 0; winId < SYSTEM_SW_MS_MAX_WIN; winId++)
    {
        SwMsLink_drvCreateWinObj(pObj, winId);
    }

    for (i = 0; i < pObj->createArgs.numSwMsInst; i++)
    {
        if (pObj->createArgs.swMsInstId[i] == SYSTEM_SW_MS_SC_INST_VIP0_SC)
            vip0Exist = TRUE;
        if (pObj->createArgs.swMsInstId[i] == SYSTEM_SW_MS_SC_INST_SC5)
            sc5Exist = TRUE;
        if(vip0Exist && sc5Exist)
        {
            Vps_printf("Didn't support SC5 and VIP0 both exist now !!!\n");
            UTILS_assert(0);
        }
    }

    pObj->maxWinId = 0;
    for (i = 0; i < pObj->createArgs.numSwMsInst; i++)
    {
        pObj->DrvObj[i].forceBypassDei = FALSE;

        if (pObj->createArgs.numSwMsInst != 1)
            pObj->DrvObj[i].startWin = pObj->createArgs.swMsInstStartWin[i];
        else
            pObj->DrvObj[i].startWin = 0;

        if( i != (pObj->createArgs.numSwMsInst - 1))
            pObj->DrvObj[i].endWin = pObj->createArgs.swMsInstStartWin[i+1] - 1;
        else
        {
            if (SYSTEM_SW_MS_MAX_WIN > pObj->DrvObj[i].startWin)
                pObj->DrvObj[i].endWin = SYSTEM_SW_MS_MAX_WIN - 1;
            else
                pObj->DrvObj[i].endWin = pObj->DrvObj[i].startWin + 1;
        }

        if ((pObj->DrvObj[i].startWin + SYSTEM_SW_MS_MAX_WIN_PER_SC - 1) < pObj->DrvObj[i].endWin)
        {
            pObj->DrvObj[i].endWin = pObj->DrvObj[i].startWin + SYSTEM_SW_MS_MAX_WIN_PER_SC - 1;
        }

        pObj->maxWinId = pObj->DrvObj[i].endWin;

        Vps_printf("SWMS: instance %d, sc id %d, start win %d end win %d\n", i,
            pObj->createArgs.swMsInstId[i], pObj->DrvObj[i].startWin, pObj->DrvObj[i].endWin);
        switch (pObj->createArgs.swMsInstId[i])
        {
            case SYSTEM_SW_MS_SC_INST_DEIHQ_SC_NO_DEI:
                pObj->DrvObj[i].forceBypassDei = TRUE;
                /* rest of DrvObj setup would be same as DEI SW MS setup, hence no 'break' */

            case SYSTEM_SW_MS_SC_INST_DEIHQ_SC:
                pObj->DrvObj[i].isDeiDrv = TRUE;
                pObj->DrvObj[i].bypassDei = FALSE;
#if defined(TI_814X_BUILD) || defined(TI_8107_BUILD)
                /* Use DEI-SC1 + VIP0-SC3 dual out mode only for 814x
                    * Mainly used in some Ce usecases where DEI link as well swMSLink opens same
                    * DrvInstances. 2 different driver instances - 1 using both single out mode & other in dual mode>
                    * sharing same SC intance is not supported, we open in dual out mode in swMS as well.
                    * We use VIP outframes as null frames <memset to 0> in order to not generate any output for VIP-SC
                    * So, effectively we get single output
                    */
                pObj->DrvObj[i].drvInstId = VPS_M2M_INST_MAIN_DEI_SC1_SC3_WB0_VIP0;
                pObj->DrvObj[i].cfg.dei.deiCreateParams.isVipScReq = TRUE;
                pObj->vipLockRequired = TRUE;
                pObj->vipInstId = SYSTEM_VIP_0;
#else
                pObj->DrvObj[i].drvInstId = VPS_M2M_INST_MAIN_DEIH_SC1_WB0;
                pObj->DrvObj[i].cfg.dei.deiCreateParams.isVipScReq = FALSE;
#endif
                if(pObj->DrvObj[i].forceBypassDei==FALSE)
                {
                    /* create DEI driver in DEI mode only if force DEI bypass is FALSE */
                    SwMsLink_drvCreateDeiDrv(pObj, &pObj->DrvObj[i]);
                }
                pObj->DrvObj[i + SYSTEM_SW_MS_MAX_INST].startWin = pObj->DrvObj[i].startWin;
                pObj->DrvObj[i + SYSTEM_SW_MS_MAX_INST].endWin = pObj->DrvObj[i].endWin;
                pObj->DrvObj[i + SYSTEM_SW_MS_MAX_INST].isDeiDrv = TRUE;
                pObj->DrvObj[i + SYSTEM_SW_MS_MAX_INST].bypassDei = TRUE;
                pObj->DrvObj[i + SYSTEM_SW_MS_MAX_INST].forceBypassDei = pObj->DrvObj[i].forceBypassDei;
#if defined(TI_814X_BUILD) || defined(TI_8107_BUILD)
                /* Use DEI-SC1 + VIP0-SC3 dual out mode only for 814x */
                pObj->DrvObj[i + SYSTEM_SW_MS_MAX_INST].drvInstId = VPS_M2M_INST_MAIN_DEI_SC1_SC3_WB0_VIP0;
                pObj->DrvObj[i + SYSTEM_SW_MS_MAX_INST].cfg.dei.deiCreateParams.isVipScReq = TRUE;
                pObj->vipLockRequired = TRUE;
                pObj->vipInstId = SYSTEM_VIP_0;
#else
                pObj->DrvObj[i + SYSTEM_SW_MS_MAX_INST].drvInstId = VPS_M2M_INST_MAIN_DEIH_SC1_WB0;
                pObj->DrvObj[i + SYSTEM_SW_MS_MAX_INST].cfg.dei.deiCreateParams.isVipScReq = FALSE;
#endif
                SwMsLink_drvCreateDeiDrv(pObj, &pObj->DrvObj[i + SYSTEM_SW_MS_MAX_INST]);
                break;

            case SYSTEM_SW_MS_SC_INST_DEI_SC_NO_DEI:
                pObj->DrvObj[i].forceBypassDei = TRUE;
                /* rest of DrvObj setup would be same as DEI SW MS setup, hence no 'break' */

            case SYSTEM_SW_MS_SC_INST_DEI_SC:
                pObj->DrvObj[i].isDeiDrv = TRUE;
                pObj->DrvObj[i].bypassDei = FALSE;

#if defined(TI_814X_BUILD) || defined(TI_8107_BUILD)
                /* Use DEI-SC2 + VIP1-SC4 dual out mode only for 814x
                    * Mainly used in some Ce usecases where DEI link as well swMSLink opens same
                    * DrvInstances. 2 different driver instances - 1 using both single out mode & other in dual mode>
                    * sharing same SC intance is not supported, we open in dual out mode in swMS as well.
                    * We use VIP outframes as null frames <memset to 0> in order to not generate any output for VIP-SC
                    * So, effectively we get single output
                    */
                pObj->DrvObj[i].drvInstId = VPS_M2M_INST_AUX_SC2_SC4_WB1_VIP1;
                pObj->DrvObj[i].cfg.dei.deiCreateParams.isVipScReq = TRUE;
                pObj->vipLockRequired = TRUE;
                pObj->vipInstId = SYSTEM_VIP_1;
#else
                pObj->DrvObj[i].drvInstId = VPS_M2M_INST_AUX_DEI_SC2_WB1;
                pObj->DrvObj[i].cfg.dei.deiCreateParams.isVipScReq = FALSE;
#endif
                if(pObj->DrvObj[i].forceBypassDei==FALSE)
                {
                    SwMsLink_drvCreateDeiDrv(pObj, &pObj->DrvObj[i]);
                }
                pObj->DrvObj[i + SYSTEM_SW_MS_MAX_INST].startWin = pObj->DrvObj[i].startWin;
                pObj->DrvObj[i + SYSTEM_SW_MS_MAX_INST].endWin = pObj->DrvObj[i].endWin;
                pObj->DrvObj[i + SYSTEM_SW_MS_MAX_INST].isDeiDrv = TRUE;
                pObj->DrvObj[i + SYSTEM_SW_MS_MAX_INST].bypassDei = TRUE;
                pObj->DrvObj[i + SYSTEM_SW_MS_MAX_INST].forceBypassDei = pObj->DrvObj[i].forceBypassDei;

#if defined(TI_814X_BUILD) || defined(TI_8107_BUILD)
                /* Use DEI-SC2 + VIP1-SC4 dual out mode only for 814x */
                pObj->DrvObj[i + SYSTEM_SW_MS_MAX_INST].drvInstId = VPS_M2M_INST_AUX_SC2_SC4_WB1_VIP1;
                pObj->DrvObj[i + SYSTEM_SW_MS_MAX_INST].cfg.dei.deiCreateParams.isVipScReq = TRUE;
                pObj->vipLockRequired = TRUE;
                pObj->vipInstId = SYSTEM_VIP_1;
#else
                pObj->DrvObj[i + SYSTEM_SW_MS_MAX_INST].drvInstId = VPS_M2M_INST_AUX_DEI_SC2_WB1;
                pObj->DrvObj[i + SYSTEM_SW_MS_MAX_INST].cfg.dei.deiCreateParams.isVipScReq = FALSE;
#endif
                SwMsLink_drvCreateDeiDrv(pObj, &pObj->DrvObj[i + SYSTEM_SW_MS_MAX_INST]);
                break;

            case SYSTEM_SW_MS_SC_INST_VIP0_SC:
                pObj->DrvObj[i].isDeiDrv = FALSE;
                pObj->DrvObj[i].drvInstId = VPS_M2M_INST_SEC0_SC3_VIP0;
                SwMsLink_drvCreateScDrv(pObj,&pObj->DrvObj[i]);
                break;
            case SYSTEM_SW_MS_SC_INST_VIP1_SC:
                pObj->DrvObj[i].isDeiDrv = FALSE;
                pObj->DrvObj[i].drvInstId = VPS_M2M_INST_SEC1_SC4_VIP1;
                SwMsLink_drvCreateScDrv(pObj,&pObj->DrvObj[i]);
                break;
            case SYSTEM_SW_MS_SC_INST_SC5:
                pObj->DrvObj[i].isDeiDrv = FALSE;
                pObj->DrvObj[i].drvInstId = VPS_M2M_INST_SEC0_SC5_WB2;
                SwMsLink_drvCreateScDrv(pObj,&pObj->DrvObj[i]);
                break;
            default:
                UTILS_assert(0);
                break;
        }
    }
    SwMsLink_drvCreateChannelObj(pObj);
    SwMsLink_drvUpdateRtChannelInfo(pObj);
    SwMsLink_drvSwitchLayout(pObj, &pObj->createArgs.layoutPrm, FALSE);
    UTILS_MEMLOG_USED_END(pObj->memUsed);
    UTILS_MEMLOG_PRINT("SWMS",pObj->memUsed,UTILS_ARRAYSIZE(pObj->memUsed));

#ifdef SYSTEM_DEBUG_SWMS
    Vps_printf(" %d: SWMS: Create Done !!!\n", Utils_getCurTimeInMsec());
#endif

    return FVID2_SOK;
}

static
Int32 SwMsLink_freeFrame(System_LinkInQueParams * pInQueParams,
                         FVID2_FrameList * frameList, FVID2_Frame * pFrame)
{
    frameList->frames[frameList->numFrames] = pFrame;
    frameList->numFrames++;

    UTILS_assert (frameList->numFrames < FVID2_MAX_FVID_FRAME_PTR);
    if (frameList->numFrames >= (FVID2_MAX_FVID_FRAME_PTR/4))
    {
        System_putLinksEmptyFrames(pInQueParams->prevLinkId,
                                   pInQueParams->prevLinkQueId, frameList);

        frameList->numFrames = 0;
    }

    return FVID2_SOK;
}

Int32 SwMsLink_drvProcessData(SwMsLink_Obj * pObj)
{
    System_LinkInQueParams *pInQueParams;
    FVID2_FrameList frameList;

    pInQueParams = &pObj->createArgs.inQueParams;

    System_getLinksFullFrames(pInQueParams->prevLinkId,
                              pInQueParams->prevLinkQueId, &frameList);

    pObj->freeFrameList.numFrames = 0;

    if (frameList.numFrames)
    {
        SwMsLink_drvLock(pObj);
        SwMsLink_drvQueueInputFrames(pObj,&frameList);
        SwMsLink_drvUnlock(pObj);

        if (pObj->freeFrameList.numFrames)
        {
            pObj->inFramePutCount += pObj->freeFrameList.numFrames;
            System_putLinksEmptyFrames(pInQueParams->prevLinkId,
                                       pInQueParams->prevLinkQueId,
                                       &pObj->freeFrameList);
        }
    }

    return FVID2_SOK;
}

Int32 SwMsLink_dmaCopyWinFromPrevOutBuf (SwMsLink_Obj *pObj)
{
    Utils_DmaCopy2D dmaPrm;
    Int32 status, i;
    UInt32 bufferPitch;

#ifdef SYSTEM_DEBUG_SWMS_RT
    Vps_printf(" %d: SWMS: Dma begin !!!\n", Utils_getCurTimeInMsec());
#endif

    bufferPitch = pObj->info.queInfo[0].chInfo[0].pitch[0];

    for (i=0; i<pObj->winCopyObj.numWindows; i++)
    {
        dmaPrm.destAddr[0]  = pObj->winCopyObj.cpObj[i].pDstWindow;
        dmaPrm.srcAddr[0]  = pObj->winCopyObj.cpObj[i].pSrcWindow;
        dmaPrm.width      = pObj->winCopyObj.cpObj[i].winWidth;
        dmaPrm.height     = pObj->winCopyObj.cpObj[i].winHeight;
        dmaPrm.destPitch[0] = bufferPitch;
        dmaPrm.srcPitch[0] = bufferPitch;

        dmaPrm.dataFormat = FVID2_DF_YUV422I_YUYV;
        dmaPrm.srcStartX  = 0;
        dmaPrm.srcStartY  = 0;
        dmaPrm.destStartX = 0;
        dmaPrm.destStartY = 0;

        status = Utils_dmaCopy2D(&pObj->dmaObj, &dmaPrm, 1);
        UTILS_assert(status==FVID2_SOK);
    }

    pObj->winCopyObj.numWindows = 0;

#ifdef SYSTEM_DEBUG_SWMS_RT
    Vps_printf(" %d: SWMS: Dma end !!!\n", Utils_getCurTimeInMsec());
#endif

    return FVID2_SOK;
}

Int32 SwMsLink_dmaWinCopyObjUpdate (SwMsLink_Obj *pObj,
                                    FVID2_Frame *pOutFrame,
                                    SwMsLink_LayoutWinInfo *pWinInfo)
{

    UTILS_assert(pObj->winCopyObj.numWindows < UTILS_ARRAYSIZE(pObj->winCopyObj.cpObj));

    FVID2_Frame *lastOutBufPtr = pObj->lastOutBufPtr;
    pObj->winCopyObj.cpObj[pObj->winCopyObj.numWindows].pDstWindow =
        (Ptr) ((UInt32) pOutFrame->addr[0][0] + pWinInfo->bufAddrOffset);
    pObj->winCopyObj.cpObj[pObj->winCopyObj.numWindows].pSrcWindow =
        (Ptr) ((UInt32) lastOutBufPtr->addr[0][0] + pWinInfo->bufAddrOffset);
    pObj->winCopyObj.cpObj[pObj->winCopyObj.numWindows].winWidth =
                                                        pWinInfo->width;
    pObj->winCopyObj.cpObj[pObj->winCopyObj.numWindows].winHeight =
                                                        pWinInfo->height;
    pObj->winCopyObj.numWindows++;

    return FVID2_SOK;
}

Int32 SwMsLink_drvMakeFrameLists(SwMsLink_Obj * pObj, FVID2_Frame * pOutFrame)
{
    UInt32 winId;
    SwMsLink_LayoutWinInfo *pWinInfo;
    SwMsLink_OutWinObj *pWinObj;
    SwMsLink_DrvObj *pDrvObj;
    FVID2_Frame *pInFrame;
    Bool repeatFld;
    System_LinkInQueParams *pInQueParams;
    UInt32                  i, drvInst;
    Bool rtParamUpdatePerFrame;
    System_FrameInfo *pFrameInfo;
    System_LinkChInfo *rtChannelInfo;
    UInt32  maxChnl;
    Bool isInputReceived;

    pInQueParams = &pObj->createArgs.inQueParams;

    for (i = 0; i < pObj->createArgs.numSwMsInst; i++)
    {
        pObj->DrvObj[i].inFrameList.numFrames = 0;
        pObj->DrvObj[i].outFrameList.numFrames = 0;
        if (pObj->DrvObj[i].isDeiDrv)
        {
            pObj->DrvObj[i + SYSTEM_SW_MS_MAX_INST].inFrameList.numFrames = 0;
            pObj->DrvObj[i + SYSTEM_SW_MS_MAX_INST].outFrameList.numFrames = 0;
        }
    }

    pObj->rtParamUpdate = FALSE;
    pObj->freeFrameList.numFrames = 0;
    pObj->winCopyObj.numWindows = 0;
    SwMsLink_drvCheckBlankOutputBuffer(pObj,pOutFrame);

    maxChnl = pObj->maxWinId + 1;

    if (maxChnl > SYSTEM_SW_MS_MAX_WIN)
        maxChnl = SYSTEM_SW_MS_MAX_WIN;

    SwMsLink_drvUpdateChCurrentInFrame(pObj);
    for (winId = 0; winId < maxChnl; winId++)
    {
        pWinObj = &pObj->winObj[winId];
        pWinInfo = &pObj->layoutParams.winInfo[winId];
        if (winId >= pObj->layoutParams.numWin)
        {
            continue;
        }

        drvInst = SwMsLink_getDrvInstFromWinId(pObj, winId);
        UTILS_assert(drvInst < pObj->createArgs.numSwMsInst );

        repeatFld = FALSE;

        pInFrame = NULL;

        if(pWinInfo->channelNum != SYSTEM_SW_MS_INVALID_ID)
        {
            UTILS_assert(pWinInfo->channelNum < UTILS_ARRAYSIZE(pObj->chObj));
            pInFrame = pObj->chObj[pWinInfo->channelNum].pCurInFrame;
        }
        if (NULL == pInFrame)
        {
            pInFrame = &pWinObj->blankFrame;

            if(pWinObj->isBlankFrameDisplay == FALSE)
            {
                /* last Frame display was non-blank frame, set RT params of blank frame */
                pWinObj->deiRtPrm.deiInFrmPrms = &pObj->blankFrameScRtInFrmPrm;
                pWinObj->deiRtPrm.deiScCropCfg = &pObj->blankFrameScRtCropCfg;

                pWinObj->scRtPrm.inFrmPrms = &pObj->blankFrameScRtInFrmPrm;
                pWinObj->scRtPrm.srcCropCfg = &pObj->blankFrameScRtCropCfg;

                pWinObj->applyRtPrm = TRUE;
            }

            pWinObj->isBlankFrameDisplay = TRUE;

        }
        else
        {
            UInt32 latency;

            pWinObj->framesUsedCount++;

            pObj->avgStats.framesUsedCount[winId]++;

            latency = Utils_getCurTimeInMsec() - pInFrame->timeStamp;

            if(latency>pWinObj->maxLatency)
                pWinObj->maxLatency = latency;
            if(latency<pWinObj->minLatency)
                pWinObj->minLatency = latency;

            if(pWinObj->isBlankFrameDisplay == TRUE)
            {
                /* last Frame display was blank frame, set RT params of current frame */
                pWinObj->deiRtPrm.deiInFrmPrms = &pWinObj->scRtInFrmPrm;
                pWinObj->deiRtPrm.deiScCropCfg = &pWinObj->scRtCropCfg;

                pWinObj->scRtPrm.inFrmPrms  = &pWinObj->scRtInFrmPrm;
                pWinObj->scRtPrm.srcCropCfg = &pWinObj->scRtCropCfg;

                pWinObj->applyRtPrm = TRUE;
            }

            pWinObj->isBlankFrameDisplay = FALSE;
        }

        pDrvObj = &pObj->DrvObj[drvInst];
        if (pObj->DrvObj[drvInst].isDeiDrv)
        {
            if (pWinInfo->bypass || pObj->DrvObj[drvInst].forceBypassDei == TRUE)
            {

                pDrvObj = &pObj->DrvObj[drvInst + SYSTEM_SW_MS_MAX_INST];
                /* [TODO] This is for avoiding bypass DEI driver getting FID
                 * = 1. Not clear why FID = 1 is passed to bypass DEI without
                 * this workaround. */
                if (FVID2_FID_TOP != pInFrame->fid)
                {
                    pInFrame->fid = FVID2_FID_TOP;
                }
            }
        }

        if (pObj->switchLayout)
        {
            pObj->lastOutBufPtr = NULL;
        }

        if( pInFrame == &pWinObj->blankFrame)
        {
            isInputReceived = FALSE;
        }
        else
        {
            isInputReceived = pObj->chObj[pWinInfo->channelNum].isInputReceived;
        }

        if((isInputReceived == TRUE) || (pObj->lastOutBufPtr == NULL) )
        {
            pDrvObj->inFrameList.frames[pDrvObj->inFrameList.numFrames]
                = pInFrame;
            pDrvObj->inFrameList.numFrames++;
            pDrvObj->outFrameList.frames[pDrvObj->outFrameList.numFrames]
                = &pWinObj->curOutFrame;
            pDrvObj->outFrameList.numFrames++;
        }
        else
        {
            SwMsLink_dmaWinCopyObjUpdate (pObj, pOutFrame, pWinInfo);
        }

        rtParamUpdatePerFrame = FALSE;
        pFrameInfo = (System_FrameInfo *) pInFrame->appData;
        if (pFrameInfo != NULL)
        {
            if (pFrameInfo->rtChInfoUpdate == TRUE)
            {
                UTILS_assert(pInFrame->channelNum<SYSTEM_SW_MS_MAX_CH_ID);

                rtChannelInfo = &pObj->rtChannelInfo[pInFrame->channelNum];

                if(pFrameInfo->rtChInfo.width > SW_MS_MAX_WIDTH_SUPPORTED)
                {
                    pFrameInfo->rtChInfo.width = SW_MS_MAX_WIDTH_SUPPORTED;
                }

                if(pFrameInfo->rtChInfo.height > SW_MS_MAX_HEIGHT_SUPPORTED)
                {
                    pFrameInfo->rtChInfo.height = SW_MS_MAX_HEIGHT_SUPPORTED;
                }

                if (pFrameInfo->rtChInfo.startX != rtChannelInfo->startX)
                {
                    rtChannelInfo->startX = pFrameInfo->rtChInfo.startX;
                    rtParamUpdatePerFrame = TRUE;
                }
                if (pFrameInfo->rtChInfo.startY != rtChannelInfo->startY)
                {
                    rtChannelInfo->startY = pFrameInfo->rtChInfo.startY;
                    rtParamUpdatePerFrame = TRUE;
                }
                if (pFrameInfo->rtChInfo.height != rtChannelInfo->height)
                {
                    rtChannelInfo->height = pFrameInfo->rtChInfo.height;
                    rtParamUpdatePerFrame = TRUE;
                }
                if (pFrameInfo->rtChInfo.width != rtChannelInfo->width)
                {
                    rtChannelInfo->width = pFrameInfo->rtChInfo.width;
                    rtParamUpdatePerFrame = TRUE;
                }
                
                /* Right now, only MPSCLR link updates the dataformat, runtime
                    Ensure this */
                if (pFrameInfo->isDataTypeChange)
                {
                    if (pFrameInfo->rtChInfo.dataFormat != 
                        rtChannelInfo->dataFormat)
                    {
                        rtChannelInfo->dataFormat = 
                            pFrameInfo->rtChInfo.dataFormat;
                        rtParamUpdatePerFrame = TRUE;
                    }
                    pFrameInfo->isDataTypeChange = FALSE;
                }
                if (pFrameInfo->rtChInfo.pitch[0] != rtChannelInfo->pitch[0])
                {
                    rtChannelInfo->pitch[0] = pFrameInfo->rtChInfo.pitch[0];
                    rtParamUpdatePerFrame = TRUE;
                }
                if (pFrameInfo->rtChInfo.pitch[1] != rtChannelInfo->pitch[1])
                {
                    rtChannelInfo->pitch[1] = pFrameInfo->rtChInfo.pitch[1];
                    rtParamUpdatePerFrame = TRUE;
                }
                pFrameInfo->rtChInfoUpdate = FALSE;
            }
        }
        pInFrame->channelNum = winId - pDrvObj->startWin;
        pInFrame->perFrameCfg = NULL;
        pWinObj->curOutFrame.perFrameCfg = NULL;

        pWinObj->curOutFrame.addr[0][0] =
            (Ptr) ((UInt32) pOutFrame->addr[0][0] + pWinInfo->bufAddrOffset);

        if (pWinObj->applyRtPrm || repeatFld || rtParamUpdatePerFrame)
        {
            if (pObj->DrvObj[drvInst].isDeiDrv)
            {
                pInFrame->perFrameCfg = &pWinObj->deiRtPrm;
                pWinObj->curOutFrame.perFrameCfg = &pWinObj->deiRtPrm;
            }
            else
            {
                pInFrame->perFrameCfg = &pWinObj->scRtPrm;
                pWinObj->curOutFrame.perFrameCfg = &pWinObj->scRtPrm;
            }
            pWinObj->applyRtPrm = FALSE;

            if (repeatFld && pObj->DrvObj[drvInst].isDeiDrv && pObj->DrvObj[drvInst].forceBypassDei == FALSE)
            {
                pWinObj->deiRtCfg.fldRepeat = TRUE;
            }
            if (rtParamUpdatePerFrame == TRUE)
            {
                pObj->rtParamUpdate = TRUE;
            }
        }
    }

    if (pObj->rtParamUpdate == TRUE)
    {
        Vps_printf(" SWMS: *** UPDATING RT Params ***\n");
        pObj->layoutParams.onlyCh2WinMapChanged = TRUE;
        SwMsLink_drvSwitchLayout(pObj, &pObj->layoutParams, TRUE);
    }
    pObj->rtParamUpdate = FALSE;
    pObj->switchLayout = FALSE;

    if (pObj->freeFrameList.numFrames)
    {
        pObj->inFramePutCount += pObj->freeFrameList.numFrames;
        System_putLinksEmptyFrames(pInQueParams->prevLinkId,
                                   pInQueParams->prevLinkQueId,
                                   &pObj->freeFrameList);
    }

    return FVID2_SOK;
}

Int32 SwMsLink_DrvProcessFrames(SwMsLink_Obj * pObj)
{
    Int32 status = FVID2_SOK;
    UInt32 curTime;
    SwMsLink_DrvObj *pDrvObj[2*SYSTEM_SW_MS_MAX_INST];
    UInt32 i,drvId;

    for (i = 0; i < SYSTEM_SW_MS_MAX_INST; i++)
    {
        pDrvObj[i] = NULL;
        pDrvObj[i + SYSTEM_SW_MS_MAX_INST] = NULL;
    }

    drvId = 0;
    for (i = 0; i < pObj->createArgs.numSwMsInst; i++)
    {
        if (pObj->DrvObj[i].isDeiDrv)
        {
            if((pObj->DrvObj[i].forceBypassDei == FALSE) &&(pObj->DrvObj[i].inFrameList.numFrames))
            {
                pDrvObj[drvId++] = &pObj->DrvObj[i];
            }

            if (pObj->DrvObj[i + SYSTEM_SW_MS_MAX_INST].inFrameList.numFrames)
            {
                pDrvObj[drvId++] = &pObj->DrvObj[i + SYSTEM_SW_MS_MAX_INST];
            }
        }
        else
        {
            if(pObj->DrvObj[i].inFrameList.numFrames)
                pDrvObj[drvId++] = &pObj->DrvObj[i];
        }
    }

    //now we'll begin process framelist in parallel
    if (drvId)
    {
        curTime = Utils_getCurTimeInMsec();
    }
    else
    {
        if (pObj->winCopyObj.numWindows)
        {
            SwMsLink_dmaCopyWinFromPrevOutBuf(pObj);
        }
        return status;
    }

    /* VIP locking should be done in dual out mode as
       *  VIP access happens at capture during reset & DEI processing
       */
    if (pObj->vipLockRequired == TRUE)
    {
        System_lockVip(pObj->vipInstId);
    }

    for (i = 0; i < drvId; i++)
    {
        /* If in dual out mode; initialize VIP-SC numOutFrames */
        if (pDrvObj[i]->processList.numOutLists == 2)
        {
            pObj->outFrameDropList.numFrames = pDrvObj[i]->outFrameList.numFrames;
        }

        SwMsLink_drvModifyFramePointer(pObj, pDrvObj[i], 1);
        status = FVID2_processFrames(pDrvObj[i]->fvidHandle, &pDrvObj[i]->processList);
        UTILS_assert(status == FVID2_SOK);
        waitingOnDriverCbSWMS[i] = TRUE;
    }

    if (pObj->winCopyObj.numWindows)
    {
        SwMsLink_dmaCopyWinFromPrevOutBuf(pObj);
    }

    for (i = 0; i < drvId; i++)
    {
        Semaphore_pend(pDrvObj[i]->complete, BIOS_WAIT_FOREVER);
        waitingOnDriverCbSWMS[i] = FALSE;
        status = FVID2_getProcessedFrames(pDrvObj[i]->fvidHandle,
                                          &pDrvObj[i]->processList, BIOS_NO_WAIT);
        UTILS_assert(status == FVID2_SOK);

        SwMsLink_drvModifyFramePointer(pObj, pDrvObj[i], 0);
        pObj->frameCount += pDrvObj[i]->inFrameList.numFrames;
    }
    if (pObj->vipLockRequired == TRUE)
    {
        System_unlockVip(pObj->vipInstId);
    }

    curTime = Utils_getCurTimeInMsec() - curTime;
    pObj->totalTime += curTime;


    return status;
}

Int32 SwMsLink_drvDoScaling(SwMsLink_Obj * pObj)
{
    FVID2_Frame *pOutFrame;
    FVID2_Frame *pDupedOutFrame;
    Int32 status;
    UInt32 curTime = Utils_getCurTimeInMsec();
    Bool enableOuputDupLocalFlag = FALSE;
    UInt key;

    if (pObj->skipProcessing)
    {
        pObj->skipProcessing--;
        return FVID2_SOK;
    }

    if (pObj->prevDoScalingTime != 0)
    {
        if (curTime > pObj->prevDoScalingTime)
        {
            UInt32 curScalingInterval;

            curScalingInterval    = curTime - pObj->prevDoScalingTime;
            pObj->scalingInterval += curScalingInterval;
            if (curScalingInterval < pObj->scalingIntervalMin)
            {
                pObj->scalingIntervalMin = curScalingInterval;
            }
            if (curScalingInterval > pObj->scalingIntervalMax)
            {
                pObj->scalingIntervalMax = curScalingInterval;
            }
        }
    }
    pObj->prevDoScalingTime = curTime;
    pObj->framesOutReqCount++;

    status = Utils_bufGetEmptyFrame(&pObj->bufOutQue, &pOutFrame, BIOS_NO_WAIT);
    if (status != FVID2_SOK)
    {
        pObj->framesOutDropCount++;
        return status;
    }
    UTILS_assert(pOutFrame != NULL);
    SwMsLink_drv_init_outframe(pObj, pOutFrame);

    pOutFrame->timeStamp = Utils_getCurTimeInMsec();

    SwMsLink_drvLock(pObj);

    SwMsLink_drvMakeFrameLists(pObj, pOutFrame);

    SwMsLink_DrvProcessFrames(pObj);

    SwMsLink_drvUnlock(pObj);

    if(pObj->createArgs.enableLayoutGridDraw == TRUE)
    {
        SwMsLink_drvDoDma(pObj,pOutFrame);
    }

    key = Hwi_disable();
    enableOuputDupLocalFlag = pObj->enableOuputDupLocalFlag;
    Hwi_restore(key);

    if (enableOuputDupLocalFlag && pObj->createArgs.enableOuputDup)
    {
        status = SwMsLink_dupFrame(pObj, pOutFrame, &pDupedOutFrame);
        status = Utils_bufPutFullFrame(&pObj->bufOutQue, pDupedOutFrame);
        UTILS_assert (status == FVID2_SOK);
    }

    pObj->lastOutBufPtr = pOutFrame;
    status = Utils_bufPutFullFrame(&pObj->bufOutQue, pOutFrame);
    UTILS_assert (status  == FVID2_SOK);
    pObj->framesOutCount++;
    pObj->avgStats.framesOutCount++;

    SwMsLink_drvPrintAvgStatistics(pObj);

    System_sendLinkCmd(pObj->createArgs.outQueParams.nextLink,
                       SYSTEM_CMD_NEW_DATA);

    return status;
}

Int32 SwMsLink_drvDeleteDrv(SwMsLink_Obj * pObj, SwMsLink_DrvObj *pDrvObj)
{
	Int32 status;

    if (pDrvObj->isDeiDrv && pDrvObj->bypassDei == FALSE)
    {
        SwMsLink_drvFreeCtxMem(pDrvObj);
    }

    status = FVID2_delete(pDrvObj->fvidHandle, NULL);
    UTILS_assert(FVID2_SOK == status);

    Semaphore_delete(&pDrvObj->complete);

    return FVID2_SOK;
}

Int32 SwMsLink_drvDelete(SwMsLink_Obj * pObj)
{
    UInt32 i;
    System_MemoryType memType;
    Int32 status;

#ifdef SYSTEM_DEBUG_SWMS
    Vps_printf(" %d: SWMS: Frames = %d (fps = %d) !!!\n",
               Utils_getCurTimeInMsec(),
               pObj->frameCount,
               pObj->frameCount * 100 / (pObj->totalTime / 10));
#endif

#ifdef SYSTEM_DEBUG_SWMS
    Vps_printf(" %d: SWMS: Delete in progress !!!\n", Utils_getCurTimeInMsec());
#endif

    memType = (System_MemoryType)pObj->inQueInfo.chInfo[0].memType;

    /* Unregister display callback tied DO_SCALING invocation on SwMs delete */
    if (pObj->createArgs.enableProcessTieWithDisplay)
    {
        AvsyncLink_VidSyncCbHookObj  cbHookObj;

        cbHookObj.cbFxn = NULL;
        cbHookObj.ctx   = NULL;
        Avsync_vidSynchRegisterCbHook(Avsync_vidQueGetDisplayID(pObj->linkId),
                                      &cbHookObj);
    }

    for (i = 0; i < pObj->createArgs.numSwMsInst; i++)
    {
        if(pObj->DrvObj[i].isDeiDrv)
        {
            if(pObj->DrvObj[i].forceBypassDei==FALSE)
            {   /* DEI driver in DEI mode is deleted only force DEI bypass is FALSE */
                SwMsLink_drvDeleteDrv(pObj, &pObj->DrvObj[i]);
            }
            SwMsLink_drvDeleteDrv(pObj, &pObj->DrvObj[i + SYSTEM_SW_MS_MAX_INST]);
        }
        else
        {
            SwMsLink_drvDeleteDrv(pObj, &pObj->DrvObj[i]);
        }
    }

    status = SwMsLink_drvDmaDelete(pObj);
    UTILS_assert(0 == status);

    Utils_bufDelete(&pObj->bufOutQue);

    Semaphore_delete(&pObj->lock);
    Clock_delete(&pObj->timer);
    SwMsLink_drvDeleteDupObj(pObj);
    SwMsLink_drvDeleteChannelObj(pObj);

    for(i=0; i<pObj->createArgs.numOutBuf; i++)
    {
        Utils_memFrameFree(&pObj->bufferFrameFormat,
                           &pObj->outFrames[i], 1);
    }

    if(memType==SYSTEM_MT_NONTILEDMEM)
    {
        /* blank frame need not be freed, its freed at system de-init */
        }
    else
    {
        // free tiler buffer
        SystemTiler_freeAll();
    }

#ifdef SYSTEM_DEBUG_SWMS
    Vps_printf(" %d: SWMS: Delete Done !!!\n", Utils_getCurTimeInMsec());
#endif

    return FVID2_SOK;
}

Int32 SwMsLink_drvStart(SwMsLink_Obj * pObj)
{
#ifdef SYSTEM_DEBUG_SWMS
    Vps_printf(" %d: SWMS: Start in Progress !!!\n", Utils_getCurTimeInMsec());
#endif

    SwMsLink_drvResetStatistics(pObj);
    Clock_start(pObj->timer);

#ifdef SYSTEM_DEBUG_SWMS
    Vps_printf(" %d: SWMS: Start Done !!!\n", Utils_getCurTimeInMsec());
#endif

    return FVID2_SOK;
}

Int32 SwMsLink_drvStop(SwMsLink_Obj * pObj)
{
#ifdef SYSTEM_DEBUG_SWMS
    Vps_printf(" %d: SWMS: Stop in Progress !!!\n", Utils_getCurTimeInMsec());
#endif

    Clock_stop(pObj->timer);

#ifdef SYSTEM_DEBUG_SWMS
    Vps_printf(" %d: SWMS: Stop Done !!!\n", Utils_getCurTimeInMsec());
#endif

    return FVID2_SOK;
}

Int32 SwMsLink_drvLock(SwMsLink_Obj * pObj)
{
    return Semaphore_pend(pObj->lock, BIOS_WAIT_FOREVER);
}

Int32 SwMsLink_drvUnlock(SwMsLink_Obj * pObj)
{
    Semaphore_post(pObj->lock);

    return FVID2_SOK;
}

Int32 SwMsLink_drvClockPeriodReconfigure(SwMsLink_Obj * pObj)
{
    UInt32 timerPeriod;

    Vps_rprintf(" %d: SWMS    : ******* Configuring clock %d secs... \n",
                Utils_getCurTimeInMsec(), pObj->timerPeriod);

    timerPeriod = pObj->timerPeriod;

    Clock_stop(pObj->timer);
    Clock_setPeriod(pObj->timer, timerPeriod);
    Clock_setTimeout(pObj->timer, timerPeriod);
    Clock_start(pObj->timer);

    return FVID2_SOK;
}

Int32 SwMsLink_drvGetTimerPeriod(SwMsLink_Obj * pObj,
                                 SwMsLink_LayoutPrm * layoutParams)
{
    if (layoutParams->outputFPS == 0 || layoutParams->outputFPS > 200)
    {
        pObj->timerPeriod = SW_MS_LINK_TIMER_DEFAULT_PERIOD;
    }
    else
    {
        pObj->timerPeriod =
              //(1000/(layoutParams->outputFPS+(layoutParams->outputFPS/10)));
              (1000/(layoutParams->outputFPS));
    }
    return FVID2_SOK;
}

Int32 SwMsLink_printBufferStatus (SwMsLink_Obj * pObj)
{
    Uint8 str[256];

    Vps_rprintf
        (" \n"
          " *** [%s] Mosaic Statistics *** \n"
          "%d: SWMS: Rcvd from prev = %d, Returned to prev = %d\r\n",
          pObj->name,
          Utils_getCurTimeInMsec(), pObj->inFrameGetCount, pObj->inFramePutCount);

    sprintf ((char *)str, "SWMS Out ");
    Utils_bufPrintStatus(str, &pObj->bufOutQue);
    return 0;
}

static
Void  SwMsLink_channelFlushBuffers(SwMsLink_Obj * pObj,
                                   UInt32 chNum,
                                   UInt32 holdLastFrame,
                                   FVID2_FrameList *freeFrameList)
{
    System_LinkInQueParams *pInQueParams;
    FVID2_Frame *frame = NULL;

    pInQueParams = &pObj->createArgs.inQueParams;

    Avsync_vidQueFlush(&pObj->chObj[chNum].inQue,
                       &frame,
                       freeFrameList);
    if (FALSE == holdLastFrame)
    {
        if (frame)
        {
            SwMsLink_freeFrame(pInQueParams,
                               freeFrameList,
                               frame);
        }
        if (pObj->chObj[chNum].pCurInFrame)
        {
            SwMsLink_freeFrame(pInQueParams,
                               freeFrameList,
                               pObj->chObj[chNum].pCurInFrame);
            pObj->chObj[chNum].pCurInFrame = NULL;
        }
    }
    else
    {
        if (frame)
        {
            if (pObj->chObj[chNum].pCurInFrame)
            {
                SwMsLink_freeFrame(pInQueParams,
                                   freeFrameList,
                                   pObj->chObj[chNum].pCurInFrame);
                pObj->chObj[chNum].pCurInFrame = NULL;
            }
            SwMsLink_freeFrame(pInQueParams,
                               freeFrameList,
                               frame);
        }
    }
    SwMsLink_freeFrameList(pObj,freeFrameList);
}

Int32 SwMsLink_flushBuffers(SwMsLink_Obj * pObj,SwMsLink_FlushParams *prm)
{
    Int32 status = FVID2_SOK;
    Int i;

    SwMsLink_drvLock(pObj);
    pObj->freeFrameList.numFrames = 0;
    if (prm->chNum == SYSTEM_SW_MS_ALL_CH_ID)
    {
        for (i = 0; i < pObj->inQueInfo.numCh; i++)
        {
            SwMsLink_channelFlushBuffers(pObj,i,
                                         prm->holdLastFrame,
                                         &pObj->freeFrameList);
        }
    }
    else
    {
        if (prm->chNum < pObj->inQueInfo.numCh)
        {
            SwMsLink_channelFlushBuffers(pObj,prm->chNum,
                                         prm->holdLastFrame,
                                         &pObj->freeFrameList);
        }
        else
        {
            Vps_printf("SWMS:!WARNING.Flush invoked for invalid channel number:%d",
                       prm->chNum);
            status = FVID2_EFAIL;
        }
    }

    if (pObj->freeFrameList.numFrames)
    {
        System_LinkInQueParams *pInQueParams;

        pInQueParams = &pObj->createArgs.inQueParams;
        pObj->inFramePutCount += pObj->freeFrameList.numFrames;
        System_putLinksEmptyFrames(pInQueParams->prevLinkId,
                                   pInQueParams->prevLinkQueId,
                                   &pObj->freeFrameList);
    }
    SwMsLink_drvUnlock(pObj);
    return status;
}
