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
#endif
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
void *Scd_bitsWriteMain(void *pPrm)
{
    Int32                      status, frameId;
    VALG_FRAMERESULTBUF_LIST_S bitsBuf;
    VALG_FRAMERESULTBUF_S      *pBuf;
    Scd_ChInfo                 *pChInfo;
    UInt32                     blkTag[ALG_LINK_SCD_MAX_BLOCKS_IN_FRAME], blkIdx;
    Uint32                     trackChId;
    Uint32                     chTag[VCAP_CHN_MAX], chIdx;       /* To keep track of which channels are being displayed in the current mosaic */

    while(!gScd_ctrl.exitWrThr)
    {
    	
        status = OSA_semWait(&gScd_ctrl.wrSem, OSA_TIMEOUT_FOREVER);
        if(status!=OSA_SOK)
            break;
        trackChId = gScd_ctrl.chIdTrack;             /* so that channel to be displayed/tracked is not changed by parallel threads */

        status = Scd_getAlgResultBuffer(&bitsBuf, TIMEOUT_NO_WAIT);  /*by guo */
	/*
		TMOD:Please apply your app refering to Demo_scd_bits_wr.c
		By:guo8113
	*/
	//grpx_draw_box(500,500,500,500);
	
	//OSA_printf("!!!!!!!Here I wanted!");		
	Scd_releaseAlgResultBuffer(&bitsBuf);	/*by guo */

    }

    gScd_ctrl.isWrThrStopDone = TRUE;
    return NULL;
}



Int32 Scd_bitsWriteCreate(UInt32 demoId,UInt32  ipcBitsInHostId)
{

    Int32 status;
    UInt32 winIdx;

    gScd_ctrl.chIdTrack            = 0;
    /* As default layout is 4x4 widnows */
    gScd_ctrl.numberOfWindows      = SCD_MAX_CHANNELS;
    gScd_ctrl.drawGrid             = FALSE;
    gScd_ctrl.enableMotionTracking = FALSE;
	gScd_ctrl.ipcBitsInHLOSId = ipcBitsInHostId;//added by guo .

#if 1
    /* Making chanOffset zero as in Ne-Prog, SCD channels start from Chan-0 */
    if(demoId == 0)
    {
       OSA_printf("\n Enable SCD block Motion Tracking On Display (Only On ON-CHIP HDMI)\n\n");
       gScd_ctrl.enableMotionTracking = FALSE;//motionTracking Enable or not

    }

     if(gScd_ctrl.enableMotionTracking)
     {
        UInt32 chId;

        chId = 0; //Demo_getChId("Motion Tracking Channel", gDemo_info.maxVcapChannels);

        /* adding offset to get actual SCD channel no. as SCD is enabled in
         * either primary/secondary path */
        gScd_ctrl.chIdTrack = chId;
        gScd_ctrl.drawGrid  = TRUE;

        /* As default layout is 4x4 widnows */
        gScd_ctrl.numberOfWindows = 16;
     }
#endif


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


