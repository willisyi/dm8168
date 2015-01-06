/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2011 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/



#include <chains_scd_bits_wr.h>		//for scd by guo
#include <mcfw/interfaces/link_api/ipcLink.h>
#define FILE_WRITE_STOPPED  (0)
#define FILE_WRITE_RUNNING  (1)

Scd_Ctrl gScd_ctrl;

Int32 Scd_resetStatistics()
{
    UInt32 chId;
    Scd_ChInfo *pChInfo;

    for(chId=0; chId<VENC_CHN_MAX; chId++)
    {
        pChInfo = &gScd_ctrl.chInfo[chId];

        pChInfo->totalDataSize = 0;
        pChInfo->numFrames = 0;
        pChInfo->maxWidth = 0;
        pChInfo->minWidth = 0xFFF;
        pChInfo->maxHeight= 0;
        pChInfo->minHeight= 0xFFF;
    }

    gScd_ctrl.statsStartTime = OSA_getCurTimeInMsec();

    return 0;
}

Int32 Scd_printStatistics(Bool resetStats)
{
    UInt32 chId;
    Scd_ChInfo *pChInfo;
    float elaspedTime;

    elaspedTime = OSA_getCurTimeInMsec() - gScd_ctrl.statsStartTime;

    elaspedTime /= 1000.0; // in secs

    printf( "\n"
            "\n *** SCD Frame Results Received Statistics *** "
            "\n"
            "\n Elased time = %6.1f secs"
            "\n"
            "\n CH | Bitrate (Kbps) | FPS | Width (max/min) | Height (max/min)"
            "\n ----------------------------------------------------------------------------------",
            elaspedTime
            );

    for(chId=0; chId<VENC_CHN_MAX;chId++)
    {
        pChInfo = &gScd_ctrl.chInfo[chId];

        if(pChInfo->numFrames)
        {
            printf("\n %2d | %14.2f | %3.1f | %5d / %6d | %6d / %6d",
                chId,
                (pChInfo->totalDataSize*8.0/elaspedTime)/1024.0,
                pChInfo->numFrames*1.0/elaspedTime,
                pChInfo->maxWidth,
                pChInfo->minWidth,
                pChInfo->maxHeight,
                pChInfo->minHeight
                );
        }
    }

    printf("\n");

    if(resetStats)
        Scd_resetStatistics();

    return 0;
}

Void Scd_bitsWriteCbFxn(Ptr pPrm)
{
    OSA_semSignal(&gScd_ctrl.wrSem);
}


Int32 Scd_trackCh(UInt32 tCh)
{
    gScd_ctrl.chIdTrack = tCh;
    gScd_ctrl.drawGrid  = TRUE;
    return 0;
}

void Scd_windowGrid(UInt32 numberOfWindows, UInt32 chId, Bool flag, UInt32 numHorzBlks)
{
   int scaleRatio;
   scaleRatio=sqrt(numberOfWindows);
   UInt32 winWidth, winHeight;
   UInt32 startX, startY;

   winWidth  = GRPX_PLANE_GRID_WIDTH/scaleRatio;
   winHeight = GRPX_PLANE_GRID_HEIGHT/scaleRatio;

   startX    = winWidth * (chId % scaleRatio);
   startY    = winHeight * (chId / scaleRatio);

#if DEMO_SCD_MOTION_TRACK_DEBUG
   OSA_printf(" [SCD MOTION TRACK]: No. Of Windows %d, ChId %d, Flag  %d\n",
                                         numberOfWindows, chId, flag);
   OSA_printf(" [SCD MOTION TRACK]: WinWidth %d, WinHeight %d, StartX %d StartY %d\n",
                                         winWidth, winHeight, startX, startY);
#endif
   if(flag==TRUE)
   {
      grpx_draw_grid(winWidth, winHeight, startX, startY, numHorzBlks);

   }else
   {
      grpx_draw_grid_exit(winWidth, winHeight, startX, startY, numHorzBlks);
   }
}
#if 0
/* To get the number of windows in case of layout switch*/
Void Scd_trackLayout(Uint32 numberOfWindows, Uint32 startChId)               
{
    UInt32 winIdx , trackChId;

    gScd_ctrl.numberOfWindows = numberOfWindows;

#if DEMO_SCD_MOTION_TRACK_DEBUG
    OSA_printf(" [SCD MOTION TRACK]: Received no of windows for this Mosaic Layout are %d\n",  
                                                               numberOfWindows);
    OSA_printf(" [SCD MOTION TRACK]: Received starting chId for this Mosaic Layout is %d \n",  
                                                                startChId);
#endif

    trackChId = startChId % gDemo_info.maxVcapChannels;   /* to check if channel lies in this mosaic */
    gScd_ctrl.startChId = trackChId;
    if(numberOfWindows == 1 || numberOfWindows == 4 || 
        numberOfWindows == 9 || numberOfWindows == 16)
    {
      for(winIdx = 0; winIdx < gScd_ctrl.numberOfWindows; winIdx++)
      {
        gScd_ctrl.winId[winIdx] = trackChId % gDemo_info.maxVcapChannels;
        trackChId++;
#if DEMO_SCD_MOTION_TRACK_DEBUG
        printf(" [SCD MOTION TRACK]: ChanNo. %d is displyed in Window %d \n",gScd_ctrl.winId[winIdx], winIdx);
#endif
      }
    }
    else
    {
      printf(" [SCD MOTION TRACK]: Disabling Motion Tracking \n");
      printf(" [SCD MOTION TRACK]: Motion Track is not supported in the current Mosaic \n");
      gScd_ctrl.enableMotionTracking = FALSE;
    }
    gScd_ctrl.layoutUpdate = TRUE;
    gScd_ctrl.drawGrid     = TRUE;
}
#endif

Void Scd_enableMotionTracking()
{
    gScd_ctrl.enableMotionTracking = TRUE;
    gScd_ctrl.drawGrid             = TRUE;
}

Void Scd_disableMotionTracking()
{
    gScd_ctrl.enableMotionTracking = FALSE;
    gScd_ctrl.drawGrid             = FALSE;
}

Bool Scd_isMotionTrackingEnabled()
{
    return (gScd_ctrl.enableMotionTracking);
}

static Void Scd_copyBitBufInfoLink2McFw(VALG_FRAMERESULTBUF_S *dstBuf,
                                         Bitstream_Buf    *srcBuf)
{
    dstBuf->reserved       = (UInt32)srcBuf;
    dstBuf->bufVirtAddr    = srcBuf->addr;
    dstBuf->bufSize        = srcBuf->bufSize;
    dstBuf->chnId          = srcBuf->channelNum;
    dstBuf->filledBufSize  = srcBuf->fillLength;
    dstBuf->timestamp      = srcBuf->timeStamp;
    dstBuf->upperTimeStamp = srcBuf->upperTimeStamp;
    dstBuf->lowerTimeStamp = srcBuf->lowerTimeStamp;
    dstBuf->bufPhysAddr    = (Void *)srcBuf->phyAddr;
    dstBuf->frameWidth     = srcBuf->frameWidth;
    dstBuf->frameHeight    = srcBuf->frameHeight;

}
Int32 Scd_getAlgResultBuffer(VALG_FRAMERESULTBUF_LIST_S *pBitsBufList, UInt32 timeout)
{
    Bitstream_BufList ipcBufList;
    Bitstream_Buf *pInBuf;
    VALG_FRAMERESULTBUF_S *pOutBuf;
    UInt32 i;

    pBitsBufList->numBufs = 0;
    ipcBufList.numBufs = 0;

    IpcBitsInLink_getFullVideoBitStreamBufs(gScd_ctrl.ipcBitsInHLOSId,
                                            &ipcBufList);

    pBitsBufList->numBufs = ipcBufList.numBufs;
    for (i = 0; i < ipcBufList.numBufs; i++)
    {
        pOutBuf = &pBitsBufList->bitsBuf[i];
        pInBuf = ipcBufList.bufs[i];

        Scd_copyBitBufInfoLink2McFw(pOutBuf,pInBuf);	/*by guo */
    }

    return 0;
}

Int32 Scd_releaseAlgResultBuffer(VALG_FRAMERESULTBUF_LIST_S *pBitsBufList)
{
    VALG_FRAMERESULTBUF_S *pOutBuf;
    Bitstream_BufList ipcBufList;
    UInt32 i;
    Int status = 0;

   // VCAP_TRACE_FXN_ENTRY("Num bufs released:%d",pBitsBufList->numBufs);
    ipcBufList.numBufs = pBitsBufList->numBufs;
    for (i = 0; i < ipcBufList.numBufs; i++)
    {
        pOutBuf = &pBitsBufList->bitsBuf[i];
        ipcBufList.bufs[i] = (Bitstream_Buf*)pBitsBufList->bitsBuf[i].reserved;
    }
    if (ipcBufList.numBufs)
    {
        status =
        IpcBitsInLink_putEmptyVideoBitStreamBufs(gScd_ctrl.ipcBitsInHLOSId,
                                                 &ipcBufList);
    }
   // VCAP_TRACE_FXN_ENTRY("Buf release status:%d",status);
    return 0;
}
#define USE_FBDEV 1
void *Scd_bitsWriteMain(void *pPrm)
{
    Int32                      status, frameId;
    VALG_FRAMERESULTBUF_LIST_S bitsBuf;
    VALG_FRAMERESULTBUF_S      *pBuf;
    Scd_ChInfo                 *pChInfo;
    UInt32                     blkTag[ALG_LINK_SCD_MAX_BLOCKS_IN_FRAME], blkIdx;
    Uint32                     trackChId;
    Uint32                     chTag[VCAP_CHN_MAX], chIdx;       /* To keep track of which channels are being displayed in the current mosaic */


    for(blkIdx = 0; blkIdx < ALG_LINK_SCD_MAX_BLOCKS_IN_FRAME; blkIdx++)
    {
       blkTag[blkIdx] = 0;
    }

    for(chIdx = 0; chIdx < 4; chIdx++)
    {
       chTag[chIdx] = 1;               /* initially all channels are being displayed as default layout is 4*4 */
    }

    while(!gScd_ctrl.exitWrThr)
    {
        status = OSA_semWait(&gScd_ctrl.wrSem, OSA_TIMEOUT_FOREVER);
        if(status!=OSA_SOK)
            break;

        trackChId = gScd_ctrl.chIdTrack;             /* so that channel to be displayed/tracked is not changed by parallel threads */

        status = Scd_getAlgResultBuffer(&bitsBuf, TIMEOUT_NO_WAIT);

        if(status==ERROR_NONE && bitsBuf.numBufs)
        {
            for(frameId=0; frameId<bitsBuf.numBufs; frameId++)
            {
                pBuf = &bitsBuf.bitsBuf[frameId];
                if(pBuf->chnId<VENC_CHN_MAX)
                {
                    pChInfo = &gScd_ctrl.chInfo[pBuf->chnId];

                    pChInfo->totalDataSize += pBuf->filledBufSize;
                    pChInfo->numFrames++;

                    if(pBuf->frameWidth > pChInfo->maxWidth)
                        pChInfo->maxWidth = pBuf->frameWidth;

                    if(pBuf->frameWidth < pChInfo->minWidth)
                        pChInfo->minWidth = pBuf->frameWidth;

                    if(pBuf->frameHeight > pChInfo->maxHeight)
                        pChInfo->maxHeight = pBuf->frameHeight;

                    if(pBuf->frameHeight < pChInfo->minHeight)
                        pChInfo->minHeight = pBuf->frameHeight;

                }


#if DEMO_SCD_ENABLE_MOTION_TRACKING
                {
                    UInt32 numBlkChg;
                    UInt32 monitoredBlk;
                    UInt32 numHorzBlks, numVerBlks, numBlksInFrame, blkHeight;
                    AlgLink_ScdResult *pChResult;
                    UInt32 scdResultBuffChanId;
#if USE_FBDEV
                    UInt32  boxHorOffset, boxVerOffset;
                    UInt32 winWidth, winHeight;
                    UInt32 winStartX, winStartY;
                    UInt32 winIdx = 0;
                    Bool gridErase;
                    UInt32 gridEraseNumWind, gridEraseWinId;

                    int  width, height;
                    int  startX, startY;
                    int  scaleRatio;
#endif
                    pChResult = (AlgLink_ScdResult *)pBuf->bufVirtAddr;

                        /* Not adding Offset  as in Ne-Prog, SCD channels start from Chan-0 */
                        scdResultBuffChanId = pChResult->chId;
                  
                    monitoredBlk = 0;
                    numBlkChg    = 0;

                    numHorzBlks     = ((pBuf->frameWidth + 0x1F ) & (~0x1F)) / 32;  /* Rounding to make divisible by 32 */

                    if((pBuf->frameHeight%ALG_LINK_SCD_BLK_HEIGHT_MIN) == 0)/* For Block height is divisible by 10 */
                       blkHeight = ALG_LINK_SCD_BLK_HEIGHT_MIN;
                    else   /* For Block height is divisible by 12 */
                       blkHeight = ALG_LINK_SCD_BLK_HEIGHT;

                    numVerBlks    = pBuf->frameHeight / blkHeight;

                    numBlksInFrame = numHorzBlks * numVerBlks;

#if USE_FBDEV
                    /* To scale acc to number of windows(assuming square layout) */
                    scaleRatio = sqrt(gScd_ctrl.numberOfWindows);

                    /* Find out where the tracking channel is displayed on current mosaic */
                    for(winIdx=0;winIdx < gScd_ctrl.numberOfWindows; winIdx++)
                    {
                        if(gScd_ctrl.winId[winIdx] == trackChId)
                           break;
                    }

                    if(gScd_ctrl.layoutUpdate)      /* in case of SWITCH LAYOUT */
                    {
                        UInt32 idx;
//                      OSA_printf(" Received starting chId for this layout are %d ",  gScd_ctrl.startChId);
                        for(chIdx = 0; chIdx < VCAP_CHN_MAX; chIdx++)
                        {
                            chTag[chIdx] = 0;
                        }
                        for(idx=0;idx < gScd_ctrl.numberOfWindows; idx++)
                        {
                            chTag[gScd_ctrl.winId[idx]] = 1;

                        }
                        gScd_ctrl.layoutUpdate = FALSE;
                    }


                    boxHorOffset = DEMO_SCD_MOTION_TRACK_GRPX_WIDTH/numHorzBlks;// DEMO_SCD_MOTION_TRACK_BOX_WIDTH_CIF;
                    if(pBuf->frameWidth == 352)
                    {
                         boxVerOffset       = DEMO_SCD_MOTION_TRACK_BOX_HEIGHT_CIF;
                    }
                    else
                    {
                         boxVerOffset       = DEMO_SCD_MOTION_TRACK_BOX_HEIGHT_QCIF;
                    }

                    if(gScd_ctrl.enableMotionTracking)
                    {
                        if((gScd_ctrl.gridPresent ==  TRUE) &&
                            ((chTag[trackChId] != 1) ||                      /*if channel to be tracked does not lie on the current mosaic,*/
                             (gScd_ctrl.prevWinIdTrack != winIdx) ||      /* to disable LMD if channel is switched or different channel is to be tracked */
                                (gScd_ctrl.prevNumberOfWindows != gScd_ctrl.numberOfWindows)))  /* in case of SWITCH LAYOUT, REDRAW GRID(acc to new layout) */
                        {
                           gridErase           =  TRUE;
                           gridEraseNumWind    =  gScd_ctrl.prevNumberOfWindows;
                           gridEraseWinId      =  gScd_ctrl.prevWinIdTrack;
                           OSA_printf("\n\n");
                           if(chTag[trackChId] != 1)
                           {
                               OSA_printf(" [SCD MOTION TRACK]: Grid clean up when Tracking channel is not in Current Mosaic Layout \n");
                               OSA_printf(" [SCD MOTION TRACK]: Starting Channel ID in the Current layout %d Max Channels in the Mosaic %d \n",
                                          gScd_ctrl.startChId, gScd_ctrl.numberOfWindows);
                           }
                           if(gScd_ctrl.prevWinIdTrack != winIdx )
                           {
                               OSA_printf(" [SCD MOTION TRACK]: Grid clean up when Tracking channel changes \n");
                               OSA_printf(" [SCD MOTION TRACK]: WinID of Previously Tracked Chan is %d WinId of Currently Tracked Chan is %d \n",
                                          gScd_ctrl.prevWinIdTrack, winIdx);
                           }
                           if(gScd_ctrl.prevNumberOfWindows != gScd_ctrl.numberOfWindows )
                           {
                                 OSA_printf(" [SCD MOTION TRACK]: Grid clean up when Num Win changes \n");
                                 OSA_printf(" [SCD MOTION TRACK]: Previous Num of Win %d Current Num of Win %d \n",
                                          gScd_ctrl.prevNumberOfWindows, gScd_ctrl.numberOfWindows);
                           }
                           scaleRatio =  sqrt(gridEraseNumWind);
                           winWidth  = GRPX_PLANE_GRID_WIDTH/scaleRatio;
                           winHeight = GRPX_PLANE_GRID_HEIGHT/scaleRatio;
                        }
                        else
                        {
                            gridErase          = FALSE;;
                            gridEraseNumWind    =  gScd_ctrl.prevNumberOfWindows;
                            gridEraseWinId      =  gScd_ctrl.prevWinIdTrack;

                            scaleRatio =  sqrt(gScd_ctrl.numberOfWindows);
                            winWidth  = GRPX_PLANE_GRID_WIDTH/scaleRatio;
                            winHeight = GRPX_PLANE_GRID_HEIGHT/scaleRatio;
                        }

                        if((gridErase ==  TRUE) && (gScd_ctrl.gridPresent == TRUE))
                        {
                            Scd_windowGrid(gridEraseNumWind, gridEraseWinId, FALSE, numHorzBlks);        /* undraw grid acc to previous no of windows */

                            winStartX    = winWidth * (gridEraseWinId % scaleRatio);
                            winStartY    = winHeight * (gridEraseWinId / scaleRatio);

                            for(blkIdx = 0; blkIdx < numBlksInFrame; blkIdx++)
                            {
                              if(blkTag[blkIdx] == 1 )                /* in case of channel switch,all previously drawn boxes are erased */
                              {
                                  OSA_printf(" [SCD MOTION TRACK]: Box clean up when Tracking channel changes \n");
                                    width       = ((DEMO_SCD_MOTION_TRACK_GRPX_WIDTH/scaleRatio)/numHorzBlks) - 3;//  Margin to avoid Grid overlap,116 - 2 - 2;
                                    height      = ((DEMO_SCD_MOTION_TRACK_GRPX_HEIGHT/scaleRatio)/numVerBlks) - 4;// Margin to avoid Grid overlap, 30 - 2 - 2;
                                    startX      = winStartX + ((((blkIdx%numHorzBlks) * boxHorOffset))/scaleRatio) + 1;
                                    startY      = winStartY + ((((blkIdx/numHorzBlks) * boxVerOffset))/scaleRatio) + 1;
                                  grpx_draw_box_exit(width,height,startX, startY);
                                  blkTag[blkIdx] = 0;
                              }
                            }
                            OSA_printf(" [SCD MOTION TRACK]: Grid Erase Finished \n");
                            gScd_ctrl.drawGrid    = TRUE;
                            gScd_ctrl.gridPresent =  FALSE;
                        }

                        if((chTag[trackChId] != 1) && (gScd_ctrl.drawGrid))                      /*if channel to be tracked does not lie on the current mosaic,*/
                        {
                          gScd_ctrl.drawGrid = FALSE;
                          OSA_printf(" [SCD MOTION TRACK]: Disabling Grid Draw as Channel is not Presnt in the Current layout \n");
                        }
                        if((gScd_ctrl.gridPresent == FALSE) && (gScd_ctrl.drawGrid) && (scdResultBuffChanId == trackChId))
                        {
                            OSA_printf(" [SCD MOTION TRACK]: Fresh Grid Drawing \n");
                            Scd_windowGrid(gScd_ctrl.numberOfWindows, winIdx, TRUE, numHorzBlks);
                            gScd_ctrl.drawGrid    =  FALSE;
                            gScd_ctrl.gridPresent =  TRUE;
                        }
                     }
                     else
                     {
                       int  prevScaleRatio;

                       if((gScd_ctrl.drawGrid  ==  FALSE) && (gScd_ctrl.gridPresent == TRUE))
                       {
                           OSA_printf(" [SCD MOTION TRACK]: Grid clean up when Motion Tracking is disabled \n");
                           Scd_windowGrid(gScd_ctrl.prevNumberOfWindows, gScd_ctrl.prevWinIdTrack, FALSE, numHorzBlks);
                           prevScaleRatio=sqrt(gScd_ctrl.prevNumberOfWindows);
                                                             /* CLEAN-UP( erase grid drawn acc to previous layout) */
                           winWidth  = GRPX_PLANE_GRID_WIDTH/prevScaleRatio;
                           winHeight = GRPX_PLANE_GRID_HEIGHT/prevScaleRatio;

                           winStartX    = winWidth * (gScd_ctrl.prevWinIdTrack % prevScaleRatio);
                           winStartY    = winHeight * (gScd_ctrl.prevWinIdTrack / prevScaleRatio);

                           for(blkIdx = 0; blkIdx < numBlksInFrame; blkIdx++)
                           {
                               if(blkTag[blkIdx] == 1 )                    /* in case of channel/layout switch,all previously drawn boxes are erased */
                               {
                                  OSA_printf(" [SCD MOTION TRACK]: Box clean up when Motion Tracking is disabled \n");
                                  width       = ((DEMO_SCD_MOTION_TRACK_GRPX_WIDTH/scaleRatio)/numHorzBlks) - 3;//  Margin to avoid Grid overlap,116 - 2 - 2;
                                  height      = ((DEMO_SCD_MOTION_TRACK_GRPX_HEIGHT/scaleRatio)/numVerBlks) - 4;// Margin to avoid Grid overlap, 30 - 2 - 2;
                                  startX      = winStartX + ((((blkIdx%numHorzBlks) * boxHorOffset))/scaleRatio) + 1;
                                  startY      = winStartY + ((((blkIdx/numHorzBlks) * boxVerOffset))/scaleRatio) + 1;
                                  grpx_draw_box_exit(width,height,startX, startY);
                                  blkTag[blkIdx] = 0;
                               }
                           }
                       }
                       gScd_ctrl.drawGrid    = TRUE;
                       gScd_ctrl.gridPresent =  FALSE;
                     }

                     scaleRatio =  sqrt(gScd_ctrl.numberOfWindows);
                     winWidth  = GRPX_PLANE_GRID_WIDTH/scaleRatio;
                     winHeight = GRPX_PLANE_GRID_HEIGHT/scaleRatio;

                     scaleRatio =  sqrt(gScd_ctrl.numberOfWindows);
                     winStartX    = winWidth * (winIdx % scaleRatio);
                     winStartY    = winHeight * (winIdx / scaleRatio);
#endif

                    /* Logic  to see how many blocks of total enabled blocks experienced change.
                     * For each block, algorithm returns no. of pixels changed in the current
                     * frame. This is compared against thresold determined using SCD sensitivity .
                     * if changed pixels are more than the calculated thresold, block is marked as changed
                     * i.e. motion is detected in the block.
                     * Minimum value of thresold is 20% and then it is incrmented by 10% for
                     * each sensitivity level change. Thresold can vary from 20% - 100%
                     * At max sensitivity, thresold would be 20%. That means if 20% pixels
                     * are changed block is marked as changed.
                     * At minimu sensitivity, thresold would be 100%. That means if 100% pixels
                     * are changed block is marked as changed */
                     for(blkIdx = 0; blkIdx < numBlksInFrame; blkIdx++)
                     {
                        AlgLink_ScdblkChngConfig * blockConfig;

                        blockConfig = &pChResult->blkConfig[blkIdx];
                                                                                                       
                        if(blockConfig->monitored == 1)
                        {
                            UInt32 threshold;

                            monitoredBlk++;
                            threshold = DEMO_SCD_MOTION_DETECTION_SENSITIVITY(ALG_LINK_SCD_BLK_WIDTH, blkHeight) +
                                              (DEMO_SCD_MOTION_DETECTION_SENSITIVITY_STEP * (ALG_LINK_SCD_SENSITIVITY_MAX - blockConfig->sensitivity));

                            if(pChResult->blkResult[blkIdx].numPixelsChanged > threshold)
                            {
                                numBlkChg++;
#if USE_FBDEV

                                if((gScd_ctrl.enableMotionTracking) && gScd_ctrl.gridPresent && (scdResultBuffChanId == trackChId))
                                {
                                    width       = ((DEMO_SCD_MOTION_TRACK_GRPX_WIDTH/scaleRatio)/numHorzBlks) - 3;//  Margin to avoid Grid overlap,116 - 2 - 2;
                                    height      = ((DEMO_SCD_MOTION_TRACK_GRPX_HEIGHT/scaleRatio)/numVerBlks) - 4;// Margin to avoid Grid overlap, 30 - 2 - 2;
                                    startX      = winStartX + ((((blkIdx%numHorzBlks) * boxHorOffset))/scaleRatio) + 1;
                                    startY      = winStartY + ((((blkIdx/numHorzBlks) * boxVerOffset))/scaleRatio) + 1;
                                    if(blkTag[blkIdx] == 0)
                                    {
                                      grpx_draw_box(width, height, startX, startY);
                                    }
                                    blkTag[blkIdx] = 1;
                                 }
#endif
                            }
#if USE_FBDEV
                            else
                            {
                                if((gScd_ctrl.enableMotionTracking) && gScd_ctrl.gridPresent && (scdResultBuffChanId==trackChId))
                                {

                                   if(blkTag[blkIdx] == 1 )
                                   {
                                     width       = ((DEMO_SCD_MOTION_TRACK_GRPX_WIDTH/scaleRatio)/numHorzBlks) - 3;//  Margin to avoid Grid overlap,116 - 2 - 2;
                                     height      = ((DEMO_SCD_MOTION_TRACK_GRPX_HEIGHT/scaleRatio)/numVerBlks) - 4;// Margin to avoid Grid overlap, 30 - 2 - 2;
                                     startX      = winStartX + ((((blkIdx%numHorzBlks) * boxHorOffset))/scaleRatio) + 1;
                                     startY      = winStartY + ((((blkIdx/numHorzBlks) * boxVerOffset))/scaleRatio) + 1;
                                     grpx_draw_box_exit(width,height,startX, startY);
                                     blkTag[blkIdx] = 0;
                                   }

                                }
                            }
#endif
                        }
                    }
#if USE_FBDEV

                    gScd_ctrl.prevWinIdTrack      = winIdx;              // update prevWinIdTrack
#endif
                    gScd_ctrl.prevNumberOfWindows = gScd_ctrl.numberOfWindows;        // update prevWindows

                    if((monitoredBlk > 0) && (numBlkChg > 0))
                    {
                        int newChNum = scdResultBuffChanId;

                        if(newChNum >= Vcap_getNumChannels())
                        {
                            newChNum -=   Vcap_getNumChannels();
                        }
                        OSA_printf(" [MOTION DETECTED] %d: SCD CH <%d> CAP CH = %d \n",
                                 OSA_getCurTimeInMsec(), pChResult->chId, newChNum);
                    }
                }
#endif
            }

             Scd_releaseAlgResultBuffer(&bitsBuf);
        }
    }

    gScd_ctrl.isWrThrStopDone = TRUE;
    return NULL;
}



Int32 Scd_bitsWriteCreate(UInt32 demoId,UInt32  ipcBitsInHostId)
{

    Int32 status;
    UInt32 winIdx;

    gScd_ctrl.chIdTrack            = 0;
    /* As default layout is 1x1 widnows */
    gScd_ctrl.numberOfWindows      = 1;//SCD_MAX_CHANNELS;
    gScd_ctrl.drawGrid             = FALSE;
    gScd_ctrl.enableMotionTracking = FALSE;
	gScd_ctrl.ipcBitsInHLOSId = ipcBitsInHostId;//added by guo .




    /* Making chanOffset zero as in Ne-Prog, SCD channels start from Chan-0 */
    if(demoId == 0)
    {
       OSA_printf("\n Enable SCD block Motion Tracking On Display (Only On ON-CHIP HDMI)\n\n");
       gScd_ctrl.enableMotionTracking =FALSE;//motionTracking Enable or not

    }

#if 1  //[guo] for test
	gScd_ctrl.enableMotionTracking = TRUE;
   	 gScd_ctrl.drawGrid  = TRUE;
#endif
     if(gScd_ctrl.enableMotionTracking)
     {
        UInt32 chId;

        chId = 0; //Demo_getChId("Motion Tracking Channel", gDemo_info.maxVcapChannels);

        /* adding offset to get actual SCD channel no. as SCD is enabled in
         * either primary/secondary path */
        gScd_ctrl.chIdTrack = chId;
        gScd_ctrl.drawGrid  = TRUE;

        /* As default layout is 4x4 widnows ,but guo's is 1*/
        gScd_ctrl.numberOfWindows = 1;
     }



#if 0
    OSA_printf("\n Enable SCD block metadata Write\n\n");
    gScd_ctrl.fileWriteEnable = 0;//Demo_getFileWriteEnable();

    if(gScd_ctrl.fileWriteEnable)
    {
        char path[256];
        UInt32 chanOffset;

        chanOffset = gDemo_info.maxVcapChannels;

        /* Making chanOffset zero as in Ne-Prog, SCD channels start from Chan-0 */
        if(demoId == DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE)
        {
             chanOffset = 0;
        }

        Demo_getFileWritePath(path, "/dev/shm");

        gScd_ctrl.fileWriteChn = Demo_getChId("File Write", gDemo_info.maxVcapChannels);

        sprintf(gScd_ctrl.fileWriteName, "%s/VID_CH%02d.bin", path, gScd_ctrl.fileWriteChn);

        /* adding offset to get actual SCD channel no. as SCD is enabled in
         * either primary/secondary path */
        gScd_ctrl.chId = gScd_ctrl.fileWriteChn + chanOffset;
    }
#endif

    gScd_ctrl.exitWrThr              = FALSE;
    gScd_ctrl.isWrThrStopDone        = FALSE;
    gScd_ctrl.prevWinIdTrack         = gScd_ctrl.chIdTrack ;
    gScd_ctrl.prevNumberOfWindows    = gScd_ctrl.numberOfWindows;

    gScd_ctrl.startChId              = 0;
    gScd_ctrl.layoutUpdate           = FALSE;
    gScd_ctrl.gridPresent            = FALSE;

    gbitscallbackFxn.newDataAvailableCb = Scd_bitsWriteCbFxn;

    /* Initializing window to channel map for SCD as 4x4 Layout  */
    for(winIdx = 0; winIdx < gScd_ctrl.numberOfWindows; winIdx++)
        gScd_ctrl.winId[winIdx] = winIdx;

    /* Register call back with encoder ,need to realize*/
	
   // Vcap_registerBitsCallback(&callback,(Ptr)&gScd_ctrl);
   	
	
    status = OSA_semCreate(&gScd_ctrl.wrSem, 1, 0);
    OSA_assert(status==OSA_SOK);

    OSA_assert(status == OSA_SOK);

    status = OSA_thrCreate(
        &gScd_ctrl.wrThrHndl,
        Scd_bitsWriteMain,
        OSA_THR_PRI_DEFAULT,
        0,
        &gScd_ctrl
        );

    OSA_assert(status==OSA_SOK);

    OSA_waitMsecs(100); // allow for print to complete
    return OSA_SOK;
}

Void Scd_bitsWriteStop()
{
    gScd_ctrl.exitWrThr = TRUE;
}
Int32 Scd_bitsWriteDelete()
{

    gScd_ctrl.exitWrThr = TRUE;
    OSA_semSignal(&gScd_ctrl.wrSem);

    while(!gScd_ctrl.isWrThrStopDone)
    {
        OSA_waitMsecs(10);
    }

    OSA_thrDelete(&gScd_ctrl.wrThrHndl);
    OSA_semDelete(&gScd_ctrl.wrSem);

    return OSA_SOK;
}
Void Chains_ipcBitsInitCreateParams_BitsInHLOSVcap(IpcBitsInLinkHLOS_CreateParams *cp)
{
    VCAP_CALLBACK_S *callback;
    Ptr cbCtx;

    cp->baseCreateParams.noNotifyMode = TRUE;

    cbCtx =  (Ptr)(&gScd_ctrl);
    cp->cbFxn = gbitscallbackFxn.newDataAvailableCb;
    cp->cbCtx = cbCtx;
    cp->baseCreateParams.notifyNextLink = FALSE;
    /* Previous link of bitsInHLOS is bitsOutRTOS. So, notifyPrevLink
     * should be set to false if bitsInHLOS is to operate in
     * NO_NOTIFY_MODE
     */
    cp->baseCreateParams.notifyPrevLink = FALSE;
}


