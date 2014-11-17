/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2011 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/
#include <demos/link_api_sd_demo/sd_demo/SD_demo.h>
#include <demos/link_api_sd_demo/sd_demo/SD_demo_common.h>
#include <demos/link_api_sd_demo/sd_demo/SD_demo_scd_bits_wr.h>

#define FILE_WRITE_STOPPED  (0)
#define FILE_WRITE_RUNNING  (1)
SD_Demo_Scd_Ctrl gSD_Demo_Scd_ctrl;

int SD_Demo_ScdResetStatistics()
{
    UInt32 chId;
    SD_Demo_Scd_ChInfo *pChInfo;

    for(chId=0; chId<gSD_Demo_ctrl.numCapChannels; chId++)
    {
        pChInfo = &gSD_Demo_Scd_ctrl.chInfo[chId];

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

    gSD_Demo_Scd_ctrl.statsStartTime = OSA_getCurTimeInMsec();

    return 0;
}

int SD_Demo_ScdPrintStatistics(Bool resetStats)
{
    UInt32 chId;
    SD_Demo_Scd_ChInfo *pChInfo;
    float elaspedTime;

    elaspedTime = OSA_getCurTimeInMsec() - gSD_Demo_Scd_ctrl.statsStartTime;

    elaspedTime /= 1000.0; // in secs

    printf( "\n"
            "\n *** SCD Bitstream Received Statistics *** "
            "\n"
            "\n Elased time = %6.1f secs"
            "\n"
            "\n CH | Bitrate (Kbps) | FPS | Key-frame FPS | Width (max/min) | Height (max/min) | Latency (max/min)"
            "\n --------------------------------------------------------------------------------------------------",
            elaspedTime
            );

    for(chId=0; chId<gSD_Demo_ctrl.numCapChannels;chId++)
    {
        pChInfo = &gSD_Demo_Scd_ctrl.chInfo[chId];

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
        SD_Demo_ScdResetStatistics();

    return 0;
}

Void SD_Demo_ScdBitsWriteCbFxn(Ptr pPrm)
{
    OSA_semSignal(&gSD_Demo_Scd_ctrl.wrSem);
}

void *SD_Demo_ScdBitsWriteMain(void *pPrm)
{
    Int32 status;
    Int32 frameId;
    Bitstream_BufList bitsBuf;
    Bitstream_Buf *pBuf;

    SD_Demo_Scd_ChInfo *pChInfo;
    UInt32 latency;

    FILE *fp = NULL;
    UInt32 fileWriteState = FILE_WRITE_STOPPED;
    UInt32 writeDataSize;

    if(gSD_Demo_Scd_ctrl.fileWriteEnable)
    {
        fp = fopen(gSD_Demo_Scd_ctrl.fileWriteName, "wb");
        if(fp!=NULL)
        {
            fileWriteState = FILE_WRITE_RUNNING;
            printf(" Opened file [%s] for writing CH%d\n", gSD_Demo_Scd_ctrl.fileWriteName, gSD_Demo_Scd_ctrl.fileWriteChn);
        }
        else
        {
            printf(" ERROR: File open [%s] for writing CH%d FAILED !!!!\n", gSD_Demo_Scd_ctrl.fileWriteName, gSD_Demo_Scd_ctrl.fileWriteChn);
        }
    }

    while(!gSD_Demo_Scd_ctrl.exitWrThr)
    {
        status = OSA_semWait(&gSD_Demo_Scd_ctrl.wrSem, OSA_TIMEOUT_FOREVER);
        if(status!=OSA_SOK)
            break;

         /* Get buffer list from the IPC Link*/
        IpcBitsInLink_getFullVideoBitStreamBufs(gSD_Demo_ctrl.ipcBitsInHLOSId[1],
                                            &bitsBuf);
        if(status== 0 && bitsBuf.numBufs)
        {
            for(frameId=0; frameId<bitsBuf.numBufs; frameId++)
            {
                pBuf = bitsBuf.bufs[frameId];
                if(pBuf->channelNum<16)
                {
                    pChInfo = &gSD_Demo_Scd_ctrl.chInfo[pBuf->channelNum];

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

                if(gSD_Demo_Scd_ctrl.fileWriteEnable);
                {
                    if(pBuf->channelNum == gSD_Demo_Scd_ctrl.chId && fileWriteState == FILE_WRITE_RUNNING)
                    {
                        writeDataSize = fwrite(pBuf->addr, 1, pBuf->fillLength, fp);
                        if(writeDataSize!=pBuf->fillLength)
                        {
                            fileWriteState = FILE_WRITE_STOPPED;
                            fclose(fp);
                            printf(" Closing file [%s] for CH%d\n", gSD_Demo_Scd_ctrl.fileWriteName, gSD_Demo_Scd_ctrl.fileWriteChn);
                        }
                    }
                }
            }

            /* Release buffer list back to the IPC Link*/
           IpcBitsInLink_putEmptyVideoBitStreamBufs(gSD_Demo_ctrl.ipcBitsInHLOSId[1],
                                             &bitsBuf);
        }

    }

    gSD_Demo_Scd_ctrl.isWrThrStopDone = TRUE;

    if(gSD_Demo_Scd_ctrl.fileWriteEnable)
    {
        if(fileWriteState==FILE_WRITE_RUNNING)
        {
            fclose(fp);
            printf(" Closing file [%s] for CH%d\n", gSD_Demo_Scd_ctrl.fileWriteName, gSD_Demo_Scd_ctrl.fileWriteChn);
        }
    }

    return NULL;
}

int SD_Demo_ScdBitsWriteCreate()
{
    Int32 status;

    gSD_Demo_Scd_ctrl.fileWriteChn = 0;
    printf("\nEnable SCD block metadata Write\n\n");
    gSD_Demo_Scd_ctrl.fileWriteEnable = SD_Demo_getFileWriteEnable();

    if(gSD_Demo_Scd_ctrl.fileWriteEnable)
    {
        char path[256];

        SD_Demo_getFileWritePath(path, "/dev/shm");

        gSD_Demo_Scd_ctrl.fileWriteChn = SD_Demo_getChId("File Write", gSD_Demo_ctrl.numCapChannels);

        sprintf(gSD_Demo_Scd_ctrl.fileWriteName, "%s/VID_CH%02d.bin", path, gSD_Demo_Scd_ctrl.fileWriteChn);

        gSD_Demo_Scd_ctrl.chId = gSD_Demo_Scd_ctrl.fileWriteChn;
    }

    gSD_Demo_Scd_ctrl.exitWrThr = FALSE;
    gSD_Demo_Scd_ctrl.isWrThrStopDone = FALSE;

    status = OSA_semCreate(&gSD_Demo_Scd_ctrl.wrSem, 1, 0);
    OSA_assert(status==OSA_SOK);

    status = OSA_thrCreate(
        &gSD_Demo_Scd_ctrl.wrThrHndl,
        SD_Demo_ScdBitsWriteMain,
        OSA_THR_PRI_DEFAULT,
        0,
        &gSD_Demo_Scd_ctrl
        );

    OSA_assert(status==OSA_SOK);

    OSA_waitMsecs(100); // allow for print to complete
    return OSA_SOK;
}

Void SD_Demo_ScdBitsWriteStop()
{
    gSD_Demo_Scd_ctrl.exitWrThr = TRUE;
}
int SD_Demo_ScdBitsWriteDelete()
{

    gSD_Demo_Scd_ctrl.exitWrThr = TRUE;
    OSA_semSignal(&gSD_Demo_Scd_ctrl.wrSem);


    while(!gSD_Demo_Scd_ctrl.isWrThrStopDone)
    {
        OSA_waitMsecs(10);
    }

    OSA_thrDelete(&gSD_Demo_Scd_ctrl.wrThrHndl);
    OSA_semDelete(&gSD_Demo_Scd_ctrl.wrSem);

    return OSA_SOK;
}

Void SD_Demo_ipcBitsInitCreateParams_BitsInHLOSVcap(IpcBitsInLinkHLOS_CreateParams *cp)
{
    cp->baseCreateParams.noNotifyMode = TRUE;
    cp->cbFxn = SD_Demo_ScdBitsWriteCbFxn;
    cp->cbCtx = &gSD_Demo_Scd_ctrl;

    cp->baseCreateParams.notifyNextLink = FALSE;
    /* Previous link of bitsInHLOS is bitsOutRTOS. So, notifyPrevLink
     * should be set to false if bitsInHLOS is to operate in
     * NO_NOTIFY_MODE
     */
    cp->baseCreateParams.notifyPrevLink = FALSE;
}




