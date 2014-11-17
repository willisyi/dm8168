
/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2011 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include <demos/link_api_sd_demo/sd_demo/SD_demo.h>
#include <demos/link_api_sd_demo/sd_demo/SD_demo_common.h>
#include <demos/link_api_sd_demo/sd_demo/SD_demo_ipc.h>

#define FILE_WRITE_STOPPED  (0)
#define FILE_WRITE_RUNNING  (1)

SD_Demo_Ipc_Ctrl gSD_Demo_Ipc_Ctrl;

Int32 SD_Demo_resetStatistics()
{
    UInt32 chId;
    SD_Demo_ChInfo *pChInfo;

    for(chId=0; chId<gSD_Demo_ctrl.numCapChannels; chId++)
    {
        pChInfo = &gSD_Demo_Ipc_Ctrl.chInfo[chId];

        pChInfo->totalDataSize = 0;
        pChInfo->numKeyFrames = 0;
        pChInfo->numFrames = 0;
        pChInfo->maxWidth = 0;
        pChInfo->minWidth = 0xFFF;
        pChInfo->maxHeight= 0;
        pChInfo->minHeight= 0xFFF;
        pChInfo->maxLatency= 0;
        pChInfo->minLatency= 0xFFF;

    }

    gSD_Demo_Ipc_Ctrl.statsStartTime = OSA_getCurTimeInMsec();

    return 0;
}

Int32 SD_Demo_printStatistics(Bool resetStats)
{
    UInt32 chId;
    SD_Demo_ChInfo *pChInfo;
    float elaspedTime;

    elaspedTime = OSA_getCurTimeInMsec() - gSD_Demo_Ipc_Ctrl.statsStartTime;

    elaspedTime /= 1000.0; // in secs

    printf( "\n"
            "\n *** Encode Bitstream Received Statistics *** "
            "\n"
            "\n Elased time = %6.1f secs"
            "\n"
            "\n CH | Bitrate (Kbps) | FPS | Key-frame FPS | Width (max/min) | Height (max/min) | Latency (max/min)"
            "\n --------------------------------------------------------------------------------------------------",
            elaspedTime
            );

    for(chId=0; chId<gSD_Demo_ctrl.numCapChannels;chId++)
    {
        pChInfo = &gSD_Demo_Ipc_Ctrl.chInfo[chId];

        if(pChInfo->numFrames)
        {
            printf("\n %2d | %14.2f | %3.1f | %13.1f | %5d / %6d | %6d / %6d  | %6d / %6d",
                chId,
                (pChInfo->totalDataSize*8.0/elaspedTime)/1024.0,
                pChInfo->numFrames*1.0/elaspedTime,
                pChInfo->numKeyFrames*1.0/elaspedTime,
                pChInfo->maxWidth,
                pChInfo->minWidth,
                pChInfo->maxHeight,
                pChInfo->minHeight,
                pChInfo->maxLatency,
                pChInfo->minLatency

                );
        }
    }

    printf("\n");

    if(resetStats)
        SD_Demo_resetStatistics();

    return 0;
}

Void SD_Demo_bitsWriteCbFxn(Ptr pPrm)
{
    OSA_semSignal(&gSD_Demo_Ipc_Ctrl.wrSem);
}

void *SD_Demo_bitsWriteMain(void *pPrm)
{
    Int32 status;
    Int32 frameId;
    Bitstream_BufList bitsBuf;
    Bitstream_Buf *pBuf;

    SD_Demo_ChInfo *pChInfo;
    UInt32 latency;

    FILE *fp = NULL;
    UInt32 fileWriteState = FILE_WRITE_STOPPED;
    UInt32 writeDataSize;

    if(gSD_Demo_Ipc_Ctrl.fileWriteEnable)
    {
        fp = fopen(gSD_Demo_Ipc_Ctrl.fileWriteName, "wb");
        if(fp!=NULL)
        {
            fileWriteState = FILE_WRITE_RUNNING;
            printf(" Opened file [%s] for writing CH%d\n", gSD_Demo_Ipc_Ctrl.fileWriteName, gSD_Demo_Ipc_Ctrl.fileWriteChn);
        }
        else
        {
            printf(" ERROR: File open [%s] for writing CH%d FAILED !!!!\n", gSD_Demo_Ipc_Ctrl.fileWriteName, gSD_Demo_Ipc_Ctrl.fileWriteChn);
        }
    }

    while(!gSD_Demo_Ipc_Ctrl.exitWrThr)
    {
        status = OSA_semWait(&gSD_Demo_Ipc_Ctrl.wrSem, OSA_TIMEOUT_FOREVER);
        if(status!=OSA_SOK)
            break;


        /* Get buffer list from the IPC Link*/
        IpcBitsInLink_getFullVideoBitStreamBufs(gSD_Demo_ctrl.ipcBitsInHLOSId[0],
                                            &bitsBuf);
        if(status== 0 && bitsBuf.numBufs)
        {
            for(frameId=0; frameId<bitsBuf.numBufs; frameId++)
            {
                pBuf = bitsBuf.bufs[frameId];
                if(pBuf->channelNum<gSD_Demo_ctrl.numCapChannels)
                {
                    pChInfo = &gSD_Demo_Ipc_Ctrl.chInfo[pBuf->channelNum];

                    pChInfo->totalDataSize += pBuf->fillLength;
                    pChInfo->numFrames++;

                    latency = pBuf->encodeTimeStamp - pBuf->timeStamp;

                    if(latency > pChInfo->maxLatency)
                        pChInfo->maxLatency = latency;

                    if(latency < pChInfo->minLatency)
                        pChInfo->minLatency = latency;

                    if(pBuf->frameWidth > pChInfo->maxWidth)
                        pChInfo->maxWidth = pBuf->frameWidth;

                    if(pBuf->frameWidth < pChInfo->minWidth)
                        pChInfo->minWidth = pBuf->frameWidth;

                    if(pBuf->frameHeight > pChInfo->maxHeight)
                        pChInfo->maxHeight = pBuf->frameHeight;

                    if(pBuf->frameHeight < pChInfo->minHeight)
                        pChInfo->minHeight = pBuf->frameHeight;

                }
                if(gSD_Demo_Ipc_Ctrl.fileWriteEnable)
                {
                    if(pBuf->channelNum== gSD_Demo_Ipc_Ctrl.fileWriteChn && fileWriteState == FILE_WRITE_RUNNING)
                    {
                        writeDataSize = fwrite(pBuf->addr, 1, pBuf->fillLength, fp);
                        if(writeDataSize!=pBuf->fillLength)
                        {
                            fileWriteState = FILE_WRITE_STOPPED;
                            fclose(fp);
                            printf(" Closing file [%s] for CH%d\n", gSD_Demo_Ipc_Ctrl.fileWriteName, gSD_Demo_Ipc_Ctrl.fileWriteChn);
                        }
                    }
                }
            }

            /* Release buffer list back to the IPC Link*/
           IpcBitsInLink_putEmptyVideoBitStreamBufs(gSD_Demo_ctrl.ipcBitsInHLOSId[0],
                                             &bitsBuf);
        }
    }

    gSD_Demo_Ipc_Ctrl.isWrThrStopDone = TRUE;

    if(gSD_Demo_Ipc_Ctrl.fileWriteEnable)
    {
        if(fileWriteState==FILE_WRITE_RUNNING)
        {
            fclose(fp);
            printf(" Closing file [%s] for CH%d\n", gSD_Demo_Ipc_Ctrl.fileWriteName, gSD_Demo_Ipc_Ctrl.fileWriteChn);
        }
    }

    return NULL;
}

Int32 SD_Demo_bitsWriteCreate()
{

    Int32 status;

    gSD_Demo_Ipc_Ctrl.fileWriteChn = 0;

    gSD_Demo_Ipc_Ctrl.fileWriteEnable = SD_Demo_getFileWriteEnable();

    if(gSD_Demo_Ipc_Ctrl.fileWriteEnable)
    {
        char path[256];

        SD_Demo_getFileWritePath(path, "/dev/shm");

        gSD_Demo_Ipc_Ctrl.fileWriteChn = SD_Demo_getChId("File Write", gSD_Demo_ctrl.numCapChannels);

        sprintf(gSD_Demo_Ipc_Ctrl.fileWriteName, "%s/VID_CH%02d.h264", path, gSD_Demo_Ipc_Ctrl.fileWriteChn);
    }

    gSD_Demo_Ipc_Ctrl.exitWrThr = FALSE;
    gSD_Demo_Ipc_Ctrl.isWrThrStopDone = FALSE;


    status = OSA_semCreate(&gSD_Demo_Ipc_Ctrl.wrSem, 1, 0);
    OSA_assert(status==OSA_SOK);

    status = OSA_thrCreate(
        &gSD_Demo_Ipc_Ctrl.wrThrHndl,
        SD_Demo_bitsWriteMain,
        OSA_THR_PRI_DEFAULT,
        0,
        &gSD_Demo_Ipc_Ctrl
        );

    OSA_assert(status==OSA_SOK);

    OSA_waitMsecs(100); // allow for print to complete
    return OSA_SOK;
}

Int32 SD_Demo_bitsWriteDelete()
{

    gSD_Demo_Ipc_Ctrl.exitWrThr = TRUE;
    OSA_semSignal(&gSD_Demo_Ipc_Ctrl.wrSem);


    while(!gSD_Demo_Ipc_Ctrl.isWrThrStopDone)
    {
        OSA_waitMsecs(10);
    }

    OSA_thrDelete(&gSD_Demo_Ipc_Ctrl.wrThrHndl);
    OSA_semDelete(&gSD_Demo_Ipc_Ctrl.wrSem);

    return OSA_SOK;
}

Void SD_Demo_ipcBitsInitCreateParams_BitsInHLOS(IpcBitsInLinkHLOS_CreateParams *cp)
{
    cp->baseCreateParams.noNotifyMode = TRUE;
    cp->cbFxn = SD_Demo_bitsWriteCbFxn;
    cp->cbCtx = &gSD_Demo_Ipc_Ctrl;
    cp->baseCreateParams.notifyNextLink = FALSE;
    /* Previous link of bitsInHLOS is bitsOutRTOS. So, notifyPrevLink
     * should be set to false if bitsInHLOS is to operate in
     * NO_NOTIFY_MODE
     */
    cp->baseCreateParams.notifyPrevLink = FALSE;
}

Void SD_Demo_ipcBitsInitCreateParams_BitsOutRTOS(IpcBitsOutLinkRTOS_CreateParams *cp,
                                                Bool notifyPrevLink)
{
    /* Next link of bitsOutRTOS is bitsInHLOS. So, notifyPrevLink
     * should be set to false if bitsInHLOS is to operate in
     * NO_NOTIFY_MODE
     */
    cp->baseCreateParams.noNotifyMode = TRUE;
    cp->baseCreateParams.notifyNextLink = FALSE;
    cp->baseCreateParams.notifyPrevLink = notifyPrevLink;
}



