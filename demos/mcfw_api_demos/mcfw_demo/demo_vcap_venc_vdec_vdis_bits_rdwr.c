/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2011 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
    \file demo_vcap_venc_vdec_vdis_bits_rdwr.c
    \brief
*/

#include <demo_vcap_venc_vdec_vdis.h>

#define ENABLE_DMA_COPY

VcapVencVdecVdis_IpcBitsCtrl gVcapVencVdecVdis_ipcBitsCtrl =
{
    .fObj.fpWrHdr  = {NULL},
    .fObj.fpWrData = {{NULL}},
    .fObj.fpWrMvData = {NULL},
    .fObj.maxFileSize    = MCFW_IPC_BITS_MAX_FILE_SIZE,
    .fObj.enableFWrite   = MCFW_IPC_BITS_ENABLE_FILE_WRITE,
    .fObj.enableLayerWrite = MCFW_IPC_BITS_ENABLE_FILE_LAYERS_WRITE,
    .fObj.fwriteEnableBitMask = MCFW_IPC_BITS_FWRITE_ENABLE_BITMASK_DEFAULT,
    .fObj.fileExtension = MCFW_IPC_BITS_DEFAULT_FILE_EXTENSION,
    .noNotifyBitsInHLOS  = MCFW_IPC_BITS_NONOTIFYMODE_BITSIN,
    .noNotifyBitsOutHLOS = MCFW_IPC_BITS_NONOTIFYMODE_BITSOUT,
    #ifdef ENABLE_DMA_COPY
    .dmaObj.useDma       = MCFW_IPC_BITS_USE_DMA_FOR_BITBUF_COPY,
    #else
    .dmaObj.useDma       = FALSE,
    #endif
};

VcapVencVdecVdis_FreeBitBufInfoTbl g_FreeBitBufInfoTbl;

static
Void VcapVencVdecVdis_FreeBitBufInit()
{
    Int status,i;

    status = OSA_mutexCreate(&g_FreeBitBufInfoTbl.mutex);
    OSA_assert(status == OSA_SOK);
    OSA_mutexLock(&g_FreeBitBufInfoTbl.mutex);
    for (i = 0; i < (OSA_ARRAYSIZE(g_FreeBitBufInfoTbl.tbl) - 1);i++)
    {
        g_FreeBitBufInfoTbl.tbl[i].nextFreeIndex = (i + 1);
    }
    g_FreeBitBufInfoTbl.tbl[i].nextFreeIndex =
        APP_IPCBITSCTRL_FREE_BITBUFINFO_INVALIDID;
    g_FreeBitBufInfoTbl.freeIndex = 0;
    OSA_mutexUnlock(&g_FreeBitBufInfoTbl.mutex);
}

static
Void VcapVencVdecVdis_FreeBitBufDeInit()
{
    Int status,i;

    status = OSA_mutexDelete(&g_FreeBitBufInfoTbl.mutex);
    OSA_assert(status == OSA_SOK);

    for (i = 0; i < (OSA_ARRAYSIZE(g_FreeBitBufInfoTbl.tbl) - 1);i++)
    {
        g_FreeBitBufInfoTbl.tbl[i].nextFreeIndex = (i + 1);
    }
    g_FreeBitBufInfoTbl.tbl[i].nextFreeIndex =
        APP_IPCBITSCTRL_FREE_BITBUFINFO_INVALIDID;
    g_FreeBitBufInfoTbl.freeIndex = 0;

}

static
VCODEC_BITSBUF_S * VcapVencVdecVdis_FreeBitBufAlloc()
{
    VCODEC_BITSBUF_S *freeBitBufInfo = NULL;
    struct VcapVencVdecVdis_FreeBitBufInfoEntry * entry = NULL;

    OSA_mutexLock(&g_FreeBitBufInfoTbl.mutex);
    OSA_assert((g_FreeBitBufInfoTbl.freeIndex !=
                APP_IPCBITSCTRL_FREE_BITBUFINFO_INVALIDID)
               &&
               (g_FreeBitBufInfoTbl.freeIndex <
                OSA_ARRAYSIZE(g_FreeBitBufInfoTbl.tbl)));
    entry = &g_FreeBitBufInfoTbl.tbl[g_FreeBitBufInfoTbl.freeIndex];
    g_FreeBitBufInfoTbl.freeIndex = entry->nextFreeIndex;
    entry->nextFreeIndex = APP_IPCBITSCTRL_FREE_BITBUFINFO_INVALIDID;
    freeBitBufInfo = &entry->bitBuf;
    OSA_mutexUnlock(&g_FreeBitBufInfoTbl.mutex);

    return freeBitBufInfo;
}

static
Int VcapVencVdecVdis_FreeBitBufGetIndex(VCODEC_BITSBUF_S * bitBufInfo)
{
   Int index;
   struct VcapVencVdecVdis_FreeBitBufInfoEntry *entry = (struct VcapVencVdecVdis_FreeBitBufInfoEntry *)bitBufInfo;

   OSA_COMPILETIME_ASSERT(offsetof(struct VcapVencVdecVdis_FreeBitBufInfoEntry,bitBuf) == 0);

   index = entry - &(g_FreeBitBufInfoTbl.tbl[0]);
   return index;
}


static
Void VcapVencVdecVdis_FreeBitBufFree(VCODEC_BITSBUF_S * bitBufInfo)
{
    Int entryIndex;
    struct VcapVencVdecVdis_FreeBitBufInfoEntry * entry = NULL;

    OSA_mutexLock(&g_FreeBitBufInfoTbl.mutex);
    entryIndex = VcapVencVdecVdis_FreeBitBufGetIndex(bitBufInfo);
    OSA_assert((entryIndex >= 0) &&
               (entryIndex < OSA_ARRAYSIZE(g_FreeBitBufInfoTbl.tbl)));
    entry = &g_FreeBitBufInfoTbl.tbl[entryIndex];
    entry->nextFreeIndex = g_FreeBitBufInfoTbl.freeIndex;
    g_FreeBitBufInfoTbl.freeIndex = entryIndex;
    OSA_mutexUnlock(&g_FreeBitBufInfoTbl.mutex);

}


static
Void VcapVencVdecVdis_ipcBitsGenerateFileName(char *dirPath,
                                 char *fname,
                                 UInt32 chNum,
                                 UInt32 chLayerNum,
                                 char *fsuffix,
                                 char *dstBuf,
                                 UInt32 maxLen);


Void VcapVencVdecVdis_ipcBitsInCbFxn (Ptr cbCtx)
{
    VcapVencVdecVdis_IpcBitsCtrl *app_ipcBitsCtrl;
    static Int printInterval;

    OSA_assert(cbCtx = &gVcapVencVdecVdis_ipcBitsCtrl);
    app_ipcBitsCtrl = cbCtx;
    OSA_semSignal(&app_ipcBitsCtrl->thrObj.bitsInNotifySem);
    #ifdef IPC_BITS_DEBUG
    if ((printInterval % MCFW_IPCBITS_INFO_PRINT_INTERVAL) == 0)
    {
        OSA_printf("MCFW_IPCBITS: Callback function:%s",__func__);
    }
    #endif
    printInterval++;

}

static Bool VcapVencVdecVdis_ipcBitsWriteWrap(FILE  * fp,
                                 UInt32 bytesToWrite,
                                 UInt32 maxFileSize)
{
    static Int printStatsIterval = 0;
    Bool wrapOccured = FALSE;

    if (maxFileSize != MCFW_IPC_BITS_MAX_FILE_SIZE_INFINITY)
    {
        if (((ftell(fp)) + bytesToWrite) > maxFileSize)
        {
            if ((printStatsIterval % MCFW_IPCBITS_INFO_PRINT_INTERVAL) == 0)
                OSA_printf("MCFW_IPCBITS:File wrap @ [%ld],MaxFileSize [%d]",
                           ftell(fp),maxFileSize);
            rewind(fp);
            printStatsIterval++;
            wrapOccured = TRUE;
        }
    }
    return wrapOccured;
}


/*
* When numTemporalLayer is set to VENC_TEMPORAL_LAYERS_1 & enableSVCExtensionFlag is
* set to enabled, then the stream comes as follows IPPPPP.....PPPPPPI 
* and the file dump with complete stream will be VBITS_DATA_0_LAYER_0.h264
* ****************************************************************************
*  When numTemporalLayer is set to VENC_TEMPORAL_LAYERS_2 & enableSVCExtensionFlag is
*  set to enabled, then the stream comes as follows 
*
*     P1    P1    P1    P1    P1
*   I    P0    P0    P0    P0     ......
*
*  and the file dump with complete stream will be VBITS_DATA_0_LAYER_1.h264 while the 
*  file VBITS_DATA_0_LAYER_0.h264 will be the subset stream of lesser frame rate.
*  There for, for any numTemporalLayer set, the complete stream file will be
*  (numTemporalLayer-1) index file
*****************************************************************************
*
*  When numTemporalLayer is set to VENC_TEMPORAL_LAYERS_4 & enableSVCExtensionFlag is
*  set to enabled, then the stream comes as follows 
*
* P3  P3    P3  P3    P3  P3       P3 P3     ....
*    P2         P2          P2           P2        ....
*           P1                       P1               P1    P1   .....
*I                       P0                               P0         ....
*
*
*  and the file dump with complete stream will be 
        VBITS_DATA_0_LAYER_3.h264 -> IP3P2P3P1P3P2P3P0P3P2P3P1....
*  while the  files 
*      VBITS_DATA_0_LAYER_2.h264  -> IP2P1P2P0P2P1P2P0....
*      VBITS_DATA_0_LAYER_1.h264  -> IP1P0P1P0P1P0...
*      VBITS_DATA_0_LAYER_0.h264  -> IP0P0P0P0IP0....
*        will be the subset streams with decreasing order of frame rate.
*
*/
static Void VcapVencVdecVdis_ipcBitsWriteBitsToFile (FILE  * fpHdr[MCFW_IPC_BITS_MAX_NUM_CHANNELS],
                                           FILE  * fpBuf[MCFW_IPC_BITS_MAX_NUM_CHANNELS][MCFW_IPC_BITS_MAX_NUM_CHANNELS_LAYERS],
                                           FILE  *fpMvBuf[MCFW_IPC_BITS_MAX_NUM_CHANNELS],
                                           VCODEC_BITSBUF_LIST_S *bufList,
                                           UInt32 maxFileSize,
                                           UInt32 fwriteEnableChannelBitmask,
                                           Bool enableLayerWrite,
                                           Bool wrapOccuredHdr[],
                                           Bool wrapOccuredBuf[MCFW_IPC_BITS_MAX_NUM_CHANNELS][MCFW_IPC_BITS_MAX_NUM_CHANNELS_LAYERS])
{
    Int i, j;
    VCODEC_BITSBUF_S *pBuf;
    size_t write_cnt;
    static UInt32 maxMvFrameCnt;

    for (i = 0; i < bufList->numBufs;i++)
    {
        UInt32 fileIdx;

        pBuf = &bufList->bitsBuf[i];
        OSA_assert(pBuf->chnId < MCFW_IPC_BITS_MAX_NUM_CHANNELS);
        fileIdx = pBuf->chnId;
    if ((fwriteEnableChannelBitmask & (1 << fileIdx)) || (fileIdx > 31 && (fwriteEnableChannelBitmask & (1 << (fileIdx-32)))))
        {
            /* This file write will be the default case valid for all codecs*/
            if(enableLayerWrite == FALSE || 
              ((enableLayerWrite == TRUE) && (pBuf->codecType != VCODEC_TYPE_H264)))
            {
                if (FALSE == wrapOccuredBuf[fileIdx][MCFW_IPC_BITS_DATA_FILE_DEFAULTLAYER_INDEX])
                {
                     wrapOccuredBuf[fileIdx][MCFW_IPC_BITS_DATA_FILE_DEFAULTLAYER_INDEX] =
                        VcapVencVdecVdis_ipcBitsWriteWrap(fpBuf[fileIdx][MCFW_IPC_BITS_DATA_FILE_DEFAULTLAYER_INDEX],
                                                          pBuf->filledBufSize, 
                                                          maxFileSize);
                }

                if (FALSE == wrapOccuredBuf[fileIdx][MCFW_IPC_BITS_DATA_FILE_DEFAULTLAYER_INDEX])
                {
                    write_cnt = fwrite(pBuf->bufVirtAddr,sizeof(char),
                                       pBuf->filledBufSize,
                                       fpBuf[fileIdx][MCFW_IPC_BITS_DATA_FILE_DEFAULTLAYER_INDEX]);
                    OSA_assert(write_cnt == pBuf->filledBufSize);

                    /*The below check for max frames as 500, means write mv data only for first n frame*/
                    if(pBuf->mvDataFilledSize != 0 && maxMvFrameCnt <= 500)
                    {
                        maxMvFrameCnt++;
                        write_cnt = fwrite(pBuf->bufVirtAddr + pBuf->mvDataOffset,
                                  sizeof(char), pBuf->mvDataFilledSize,fpMvBuf[fileIdx]);

                        OSA_assert(write_cnt == pBuf->mvDataFilledSize);
                    }
                }
            }
            else if(enableLayerWrite == TRUE)
            {
                /*I-Frames should be present in all the h264 temporal layers*/
                if( pBuf->temporalId == 0 && 
                    pBuf->frameType == VCODEC_FRAME_TYPE_I_FRAME )
                {
                    for(j = 0; j < pBuf->numTemporalLayerSetInCodec ; j++)
                    {
                        if (FALSE == wrapOccuredBuf[fileIdx][j])
                        {
                            wrapOccuredBuf[fileIdx][j] =
                            VcapVencVdecVdis_ipcBitsWriteWrap(fpBuf[fileIdx][j],pBuf->filledBufSize, maxFileSize);
                        }

                    if (FALSE == wrapOccuredBuf[fileIdx][j])
                        {
                            write_cnt = fwrite(pBuf->bufVirtAddr,sizeof(char),pBuf->filledBufSize,fpBuf[fileIdx][j]);
                            OSA_assert(write_cnt == pBuf->filledBufSize);
                        }                 
                    }
                }

                /*P-Frames should be present in specific layers based on their temporal Id's
                               ***/
                if( pBuf->temporalId == 0 && 
                     pBuf->frameType == VCODEC_FRAME_TYPE_P_FRAME )
                {
                    for(j = 0; j < pBuf->numTemporalLayerSetInCodec ; j++)
                    {
                           
                        if (FALSE == wrapOccuredBuf[fileIdx][j])
                        {
                            wrapOccuredBuf[fileIdx][j] =
                                VcapVencVdecVdis_ipcBitsWriteWrap(fpBuf[fileIdx][j],pBuf->filledBufSize, maxFileSize);
                        }

                        if (FALSE == wrapOccuredBuf[fileIdx][j])
                        {
                            write_cnt = fwrite(pBuf->bufVirtAddr,sizeof(char),pBuf->filledBufSize,fpBuf[fileIdx][j]);
                            OSA_assert(write_cnt == pBuf->filledBufSize);
                        }
                    }
                }

                if( pBuf->temporalId == 1 && 
                     pBuf->frameType == VCODEC_FRAME_TYPE_P_FRAME )
                {
                    for(j = 1; j < pBuf->numTemporalLayerSetInCodec ; j++)
                    {
                            
                        if (FALSE == wrapOccuredBuf[fileIdx][j])
                        {
                            wrapOccuredBuf[fileIdx][j] =
                                VcapVencVdecVdis_ipcBitsWriteWrap(fpBuf[fileIdx][j],pBuf->filledBufSize, maxFileSize);
                        }

                        if (FALSE == wrapOccuredBuf[fileIdx][j])
                        {
                            write_cnt = fwrite(pBuf->bufVirtAddr,sizeof(char),pBuf->filledBufSize,fpBuf[fileIdx][j]);
                            OSA_assert(write_cnt == pBuf->filledBufSize);
                        }
                    }
                }

                if( pBuf->temporalId == 2 && 
                     pBuf->frameType == VCODEC_FRAME_TYPE_P_FRAME )
                {
                    for(j = 2; j < pBuf->numTemporalLayerSetInCodec ; j++)
                    {
                             
                        if (FALSE == wrapOccuredBuf[fileIdx][j])
                        {
                            wrapOccuredBuf[fileIdx][j] =
                                VcapVencVdecVdis_ipcBitsWriteWrap(fpBuf[fileIdx][j],pBuf->filledBufSize, maxFileSize);
                        }

                        if (FALSE == wrapOccuredBuf[fileIdx][j])
                        {
                            write_cnt = fwrite(pBuf->bufVirtAddr,sizeof(char),pBuf->filledBufSize,fpBuf[fileIdx][j]);
                            OSA_assert(write_cnt == pBuf->filledBufSize);
                        }
                    }
                }
                if( pBuf->temporalId == 3 && 
                     pBuf->frameType == VCODEC_FRAME_TYPE_P_FRAME )
                {
                    for(j = 3; j < pBuf->numTemporalLayerSetInCodec ; j++)
                    {
                           
                        if (FALSE == wrapOccuredBuf[fileIdx][j])
                        {
                            wrapOccuredBuf[fileIdx][j] =
                                VcapVencVdecVdis_ipcBitsWriteWrap(fpBuf[fileIdx][j],pBuf->filledBufSize, maxFileSize);
                        }

                        if (FALSE == wrapOccuredBuf[fileIdx][j])
                        {
                            write_cnt = fwrite(pBuf->bufVirtAddr,sizeof(char),pBuf->filledBufSize,fpBuf[fileIdx][j]);
                            OSA_assert(write_cnt == pBuf->filledBufSize);
                        }                 
                    }
                }                   

                /*The below check for max frames as 500, means write mv data only for first n frame*/
                if(pBuf->mvDataFilledSize != 0 && maxMvFrameCnt <= 500)
                {
                    maxMvFrameCnt++;
                    write_cnt = fwrite(pBuf->bufVirtAddr + pBuf->mvDataOffset,
                              sizeof(char), pBuf->mvDataFilledSize,fpMvBuf[fileIdx]);

                    OSA_assert(write_cnt == pBuf->mvDataFilledSize);
                }
            }

            if (FALSE == wrapOccuredHdr[fileIdx])
            {
                wrapOccuredHdr[fileIdx] =
                VcapVencVdecVdis_ipcBitsWriteWrap(fpHdr[fileIdx],sizeof(*pBuf),maxFileSize);
            }
            if (FALSE == wrapOccuredHdr[fileIdx])
            {
                write_cnt = fwrite(pBuf,sizeof(*pBuf),1,fpHdr[fileIdx]);
                OSA_assert(write_cnt == 1);
            }
        }
    }
}

static Void VcapVencVdecVdis_ipcBitsCopyBitBufInfo (VCODEC_BITSBUF_S *dst,
                                          const VCODEC_BITSBUF_S *src)
{
    dst->chnId = src->chnId;
    dst->codecType = src->codecType;
    dst->filledBufSize = src->filledBufSize;
    dst->bottomFieldBitBufSize = src->bottomFieldBitBufSize;
    dst->inputFileChanged = src->inputFileChanged;
    dst->frameType = src->frameType;
    dst->timestamp  = src->timestamp;
    dst->upperTimeStamp= src->upperTimeStamp;
    dst->lowerTimeStamp= src->lowerTimeStamp;
    dst->seqId         = Demo_displayGetCurSeqId();
}


static Void VcapVencVdecVdis_ipcBitsCopyBitBufDataMem2Mem(VCODEC_BITSBUF_S *dstBuf,
                                                VCODEC_BITSBUF_S *srcBuf)
{
    OSA_assert(srcBuf->filledBufSize < dstBuf->bufSize);

    if (FALSE == gVcapVencVdecVdis_ipcBitsCtrl.dmaObj.useDma)
    {
        memcpy(dstBuf->bufVirtAddr,srcBuf->bufVirtAddr,srcBuf->filledBufSize);
    }
    else
    {
        OSA_DmaCopy1D prm;

        prm.srcPhysAddr = (UInt32)srcBuf->bufPhysAddr;
        prm.dstPhysAddr = (UInt32)dstBuf->bufPhysAddr;
        prm.size        = srcBuf->filledBufSize;
        OSA_dmaCopy1D(&gVcapVencVdecVdis_ipcBitsCtrl.dmaObj.dmaChHdnl,
                      &prm,
                      1);

    }
}

static Void VcapVencVdecVdis_ipcBitsQueEmptyBitBufs(VcapVencVdecVdis_res resolution, OSA_QueHndl       *emptyQue)
{
    VCODEC_BITSBUF_LIST_S emptyBufList;
    VCODEC_BITSBUF_S *pBuf;
    VDEC_BUF_REQUEST_S reqInfo;
    Int i;
    Int status;
    UInt32 bitBufSize;

    bitBufSize = MCFW_IPCBITS_GET_BITBUF_SIZE(resolution.width,
                                                resolution.height);
    if (bitBufSize <=0 )
        return;

    emptyBufList.numBufs = 0;
    reqInfo.numBufs = VCODEC_BITSBUF_MAX;
    reqInfo.reqType = VDEC_BUFREQTYPE_BUFSIZE;
    for (i = 0; i < VCODEC_BITSBUF_MAX; i++)
    {
        reqInfo.u[i].minBufSize = bitBufSize;
    }
    Vdec_requestBitstreamBuffer(&reqInfo, &emptyBufList, 0);
    for (i = 0; i < emptyBufList.numBufs; i++)
    {
        pBuf = VcapVencVdecVdis_FreeBitBufAlloc();
        OSA_assert(pBuf != NULL);
        *pBuf = emptyBufList.bitsBuf[i];
        OSA_assert(pBuf->bufSize >= bitBufSize );

        status = OSA_quePut(emptyQue,(Int32)pBuf,OSA_TIMEOUT_NONE);
        OSA_assert(status == 0);
    }
}

static Void VcapVencVdecVdis_ipcBitsSendFullBitBufs(OSA_QueHndl       *fullQue)
{
    VCODEC_BITSBUF_LIST_S fullBufList;
    VCODEC_BITSBUF_S *pBuf;
    Int status;

    fullBufList.numBufs = 0;
    while((status = OSA_queGet(fullQue,(Int32 *)(&pBuf),OSA_TIMEOUT_NONE)) == 0)
    {
        OSA_assert(fullBufList.numBufs < VCODEC_BITSBUF_MAX);
        fullBufList.bitsBuf[fullBufList.numBufs] = *pBuf;
        fullBufList.numBufs++;
        VcapVencVdecVdis_FreeBitBufFree(pBuf);
        if (fullBufList.numBufs == VCODEC_BITSBUF_MAX)
        {
            break;
        }
    }
    if (fullBufList.numBufs)
    {
        gVcapVencVdecVdis_ipcBitsCtrl.totalDecFrames+=fullBufList.numBufs;
        Vdec_putBitstreamBuffer(&fullBufList);
    }
}

static Void *VcapVencVdecVdis_ipcBitsSendFxn(Void * prm)
{
    VcapVencVdecVdis_IpcBitsCtrlThrObj *thrObj = (VcapVencVdecVdis_IpcBitsCtrlThrObj *) prm;
    static Int printStatsInterval = 0, i;

    OSA_printf("MCFW_IPCBITS:%s:Entered...",__func__);
    while (FALSE == thrObj->exitBitsOutThread)
    {
        OSA_waitMsecs(MCFW_IPCBITS_SENDFXN_PERIOD_MS);
        for(i=0; i<MCFW_IPCBITS_RESOLUTION_TYPES; i++)
            VcapVencVdecVdis_ipcBitsQueEmptyBitBufs(thrObj->resolution[i], &thrObj->bufQFreeBufs);
        VcapVencVdecVdis_ipcBitsSendFullBitBufs(&thrObj->bufQFullBufs);

        #ifdef IPC_BITS_DEBUG
        if ((printStatsInterval % MCFW_IPCBITS_INFO_PRINT_INTERVAL) == 0)
        {
            OSA_printf("MCFW_IPCBITS:%s:INFO: periodic print..",__func__);
        }
        #endif
        printStatsInterval++;
    }
    OSA_printf("MCFW_IPCBITS:%s:Leaving...",__func__);
    return NULL;
}

#define DEMO_VCAP_VENC_VDEC_VDIS_REPLACE_DECODER_TIMESTAMPS (TRUE)



#define VDEC_VDIS_FRAME_DURATION_MS (33)

static Void VcapVencVdecVdis_setFrameTimeStamp(VcapVencVdecVdis_ChInfo *decChInfo,
                                               VCODEC_BITSBUF_S *pEmptyBuf)
{
#ifdef DEMO_VCAP_VENC_VDEC_VDIS_REPLACE_DECODER_TIMESTAMPS
    UInt64 curTimeStamp =
      decChInfo->numFrames * VDEC_VDIS_FRAME_DURATION_MS;

    pEmptyBuf->lowerTimeStamp = (UInt32)(curTimeStamp & 0xFFFFFFFF);
    pEmptyBuf->upperTimeStamp = (UInt32)((curTimeStamp >> 32)& 0xFFFFFFFF);
#else
    (Void)pEmptyBuf; //Kill warning
#endif /* DEMO_VCAP_VENC_VDEC_VDIS_REPLACE_DECODER_TIMESTAMPS */
    if (0 == decChInfo->numFrames)
    {
        UInt32 displayChId;

        Vdec_mapDec2DisplayChId(VDIS_DEV_HDMI,pEmptyBuf->chnId,&displayChId);
        Vdis_setFirstVidPTS(VDIS_DEV_HDMI,displayChId,curTimeStamp);
        Vdec_mapDec2DisplayChId(VDIS_DEV_HDCOMP,pEmptyBuf->chnId,&displayChId);
        Vdis_setFirstVidPTS(VDIS_DEV_HDCOMP,displayChId,curTimeStamp);
        Vdec_mapDec2DisplayChId(VDIS_DEV_SD,pEmptyBuf->chnId,&displayChId);
        Vdis_setFirstVidPTS(VDIS_DEV_SD,displayChId,curTimeStamp);
    }

    decChInfo->numFrames += 1;
}

static Void VcapVencVdecVdis_ipcBitsProcessFullBufs(VcapVencVdecVdis_IpcBitsCtrlThrObj *thrObj,
                                       VcapVencVdecVdis_IpcBitsCtrlFileObj *fObj,
                                       VcapVencVdecVdis_ChInfo decChInfo[])
{
    VCODEC_BITSBUF_LIST_S fullBufList;
    VCODEC_BITSBUF_S *pFullBuf;
    VCODEC_BITSBUF_S *pEmptyBuf;
    Int i,status;
    static Int printStatsInterval = 0;

    #ifdef IPC_BITS_DEBUG
    if ((printStatsInterval % MCFW_IPCBITS_INFO_PRINT_INTERVAL) == 0)
    {
        OSA_printf("MCFW_IPCBITS:%s:INFO: periodic print..",__func__);
    }
    #endif
    printStatsInterval++;

    Venc_getBitstreamBuffer(&fullBufList, 0);

    for (i = 0; i < fullBufList.numBufs; i++)
    {
        VcapVencVdecVdis_updateStatistics(&fullBufList.bitsBuf[i]);

        if (fullBufList.bitsBuf[i].chnId < Venc_getPrimaryChannels())
        {
            pFullBuf = &fullBufList.bitsBuf[i];
GET_BUF:
            status = OSA_queGet(&thrObj->bufQFreeBufs,(Int32 *)(&pEmptyBuf),
                                OSA_TIMEOUT_FOREVER);
            OSA_assert(status == 0);

            /* If empty buf size not enough to hold rcvd buf <e.g, in multiple resolution scenarios>,
                        * put back the buffer into freeBufs Q
                        */
            if (pFullBuf->filledBufSize > pEmptyBuf->bufSize)
            {
                status = OSA_quePut(&thrObj->bufQFreeBufs,(Int32)pEmptyBuf,
                                    OSA_TIMEOUT_FOREVER);
                OSA_assert(status == 0);
                goto GET_BUF;
            }
            VcapVencVdecVdis_ipcBitsCopyBitBufInfo(pEmptyBuf,pFullBuf);
            VcapVencVdecVdis_ipcBitsCopyBitBufDataMem2Mem(pEmptyBuf,pFullBuf);
            VcapVencVdecVdis_setFrameTimeStamp(&(decChInfo[pEmptyBuf->chnId]),pEmptyBuf);
            status = OSA_quePut(&thrObj->bufQFullBufs,
                                (Int32)pEmptyBuf,OSA_TIMEOUT_NONE);
            OSA_assert(status == 0);
        }
    }
    if (fObj->enableFWrite)
    {
        VcapVencVdecVdis_ipcBitsWriteBitsToFile(fObj->fpWrHdr,
                                      fObj->fpWrData,
                                      fObj->fpWrMvData,
                                      &fullBufList,
                                      fObj->maxFileSize,
                                      fObj->fwriteEnableBitMask,
                                      fObj->enableLayerWrite,
                                      fObj->wrapOccuredHdr,
                                      fObj->wrapOccuredBuf);

    }
    Venc_releaseBitstreamBuffer(&fullBufList);
}

static Void *VcapVencVdecVdis_ipcBitsRecvFxn(Void * prm)
{
    VcapVencVdecVdis_IpcBitsCtrl *ipcBitsCtrl = (VcapVencVdecVdis_IpcBitsCtrl *) prm;
    VcapVencVdecVdis_IpcBitsCtrlThrObj *thrObj = &ipcBitsCtrl->thrObj;
    VcapVencVdecVdis_IpcBitsCtrlFileObj *fObj =  &ipcBitsCtrl->fObj;
    UInt32 printStatsInterval = OSA_getCurTimeInMsec();
    UInt32 elaspedTime;

    while (FALSE == thrObj->exitBitsInThread)
    {
        OSA_semWait(&thrObj->bitsInNotifySem,OSA_TIMEOUT_FOREVER);
        VcapVencVdecVdis_ipcBitsProcessFullBufs(thrObj,
                                      fObj,
                                      ipcBitsCtrl->decInfo);

        elaspedTime = OSA_getCurTimeInMsec() - printStatsInterval;

        if (elaspedTime >= 10000)
        {
            #if 1
            VcapVencVdecVdis_printAvgStatistics(elaspedTime, TRUE);
            #endif

            printStatsInterval = OSA_getCurTimeInMsec();
        }
    }
    return NULL;
}


static Void VcapVencVdecVdis_ipcBitsInitThrObj(VcapVencVdecVdis_IpcBitsCtrlThrObj *thrObj)
{

    OSA_semCreate(&thrObj->bitsInNotifySem,
                  MCFW_IPCBITS_MAX_PENDING_RECV_SEM_COUNT,0);
    thrObj->exitBitsInThread = FALSE;
    thrObj->exitBitsOutThread = FALSE;
    OSA_queCreate(&thrObj->bufQFreeBufs,MCFW_IPCBITS_FREE_QUE_MAX_LEN);
    OSA_queCreate(&thrObj->bufQFullBufs,MCFW_IPCBITS_FULL_QUE_MAX_LEN);
    OSA_thrCreate(&thrObj->thrHandleBitsOut,
                  VcapVencVdecVdis_ipcBitsSendFxn,
                  MCFW_IPCBITS_SENDFXN_TSK_PRI,
                  MCFW_IPCBITS_SENDFXN_TSK_STACK_SIZE,
                  thrObj);

    OSA_thrCreate(&thrObj->thrHandleBitsIn,
                  VcapVencVdecVdis_ipcBitsRecvFxn,
                  MCFW_IPCBITS_RECVFXN_TSK_PRI,
                  MCFW_IPCBITS_RECVFXN_TSK_STACK_SIZE,
                  &gVcapVencVdecVdis_ipcBitsCtrl);

}

static Void VcapVencVdecVdis_ipcBitsDeInitThrObj(VcapVencVdecVdis_IpcBitsCtrlThrObj *thrObj)
{
    thrObj->exitBitsInThread = TRUE;
    thrObj->exitBitsOutThread = TRUE;
    OSA_thrDelete(&thrObj->thrHandleBitsOut);
    OSA_thrDelete(&thrObj->thrHandleBitsIn);
    OSA_semDelete(&thrObj->bitsInNotifySem);
    OSA_queDelete(&thrObj->bufQFreeBufs);
    OSA_queDelete(&thrObj->bufQFullBufs);

}


static
Void VcapVencVdecVdis_ipcBitsGenerateFileName(char *dirPath,
                                    char *fname,
                                    UInt32 chNum,
                                    UInt32 chLayerNum,
                                    char *fsuffix,
                                    char *dstBuf,
                                    UInt32 maxLen)
{
    if(gVcapVencVdecVdis_ipcBitsCtrl.fObj.enableLayerWrite == TRUE)
    {
        snprintf(dstBuf,
             (maxLen - 2),
             "%s/%s_%d_LAYER_%d.%s",
             dirPath,
             fname,
             chNum,
             chLayerNum,
             fsuffix);
    }
    else
    {
        snprintf(dstBuf,
             (maxLen - 2),
             "%s/%s_%d.%s",
             dirPath,
             fname,
             chNum,
             fsuffix);
    }
    dstBuf[(maxLen - 1)] = 0;
}


Void VcapVencVdecVdis_ipcBitsInitSetBitsInNoNotifyMode(Bool noNotifyMode)
{
    gVcapVencVdecVdis_ipcBitsCtrl.noNotifyBitsInHLOS = noNotifyMode;
}

Void VcapVencVdecVdis_ipcBitsInitSetBitsOutNoNotifyMode(Bool noNotifyMode)
{
    gVcapVencVdecVdis_ipcBitsCtrl.noNotifyBitsOutHLOS = noNotifyMode;
}

static
UInt32 VcapVencVdecVdis_ipcBitsGetNumEnabledChannels()
{
    UInt32 numEnabledChannels = 0;
    Int i;

#if (MCFW_IPC_BITS_MAX_NUM_CHANNELS > 48)
    #error "Bitmask for channel fwrite enable supports max 32 channels.Change it"
#endif

    if (FALSE == gVcapVencVdecVdis_ipcBitsCtrl.fObj.enableFWrite)
    {
        numEnabledChannels = 0;
    }
    else
    {
        for (i = 0; i < MCFW_IPC_BITS_MAX_NUM_CHANNELS; i++)
        {
            if ((1 << i) & gVcapVencVdecVdis_ipcBitsCtrl.fObj.fwriteEnableBitMask)
            {
                numEnabledChannels++;
            }

        }
    }
    return (numEnabledChannels);
}

static
unsigned long VcapVencVdecVdis_ipcBitsGetPartitionFreeSpace(char *filePath)
{
    struct statvfs statBuf;
    int status;
    unsigned long freeSize;

    status = statvfs(filePath,&statBuf);
    if (status == 0)
    {
        freeSize = statBuf.f_bsize * statBuf.f_bfree;
        freeSize = (float)freeSize *
                     ((float)MCFW_IPC_BITS_MAX_PARTITION_USAGE_PERCENT/100);
    }
    else
    {
        freeSize = 0;

    }
    OSA_printf("Partition Free Size for Path[%s] = [0x%lX]",
               filePath,freeSize);
    return (freeSize);
}

static
UInt32 VcapVencVdecVdis_ipcBitsGetMaxFileSizePerChannel(char *filePath)
{
    UInt32 maxFileSizePerChannel = 0;
    unsigned long paritionFreeSize;

    paritionFreeSize = VcapVencVdecVdis_ipcBitsGetPartitionFreeSpace(filePath);
    if (0 == paritionFreeSize)
    {
        OSA_printf("Not able to determine partition size.Using default size:%d",
                   MCFW_IPC_BITS_MAX_FILE_SIZE);
        maxFileSizePerChannel = MCFW_IPC_BITS_MAX_FILE_SIZE;
    }
    else
    {
        unsigned long maxFileSizeActual;
        maxFileSizeActual = paritionFreeSize/
                            VcapVencVdecVdis_ipcBitsGetNumEnabledChannels();
        if (maxFileSizeActual > MCFW_IPC_BITS_MAX_SINGLE_FILE_SIZE)
        {
            OSA_printf("Limiting File Size to max[0x%X],actual[0x%lX]",
                       MCFW_IPC_BITS_MAX_SINGLE_FILE_SIZE,
                       maxFileSizeActual);
            maxFileSizeActual = MCFW_IPC_BITS_MAX_SINGLE_FILE_SIZE;
        }
        maxFileSizePerChannel = maxFileSizeActual;
        /* Each channel will have a buffer and a header. So divide by 2 */
        maxFileSizePerChannel /= 2;
        OSA_printf("Max File size per channel:0x%X",maxFileSizePerChannel);
    }
    return (maxFileSizePerChannel);
}


static
Int   VcapVencVdecVdis_ipcBitsOpenFileHandles()
{
    Int status = OSA_SOK;
    Int i,j;
    char fileNameHdr[128];
    char fileNameBuffer[128];


    for (i = 0; i < MCFW_IPC_BITS_MAX_NUM_CHANNELS; i++)
    {
        VcapVencVdecVdis_ipcBitsGenerateFileName(gVcapVencVdecVdis_ipcBitsCtrl.fObj.fileDirPath,
                                       MCFW_IPC_BITS_HDR_FILE_NAME,
                                       i,
                                       0,
                                       MCFW_IPC_BITS_DEFAULT_FILE_EXTENSION,
                                       fileNameHdr,
                                       sizeof(fileNameHdr));
        gVcapVencVdecVdis_ipcBitsCtrl.fObj.fpWrHdr[i] = fopen(fileNameHdr,"wb");
        OSA_assert(gVcapVencVdecVdis_ipcBitsCtrl.fObj.fpWrHdr[i] != NULL);
        if (0 == i)
        {
            gVcapVencVdecVdis_ipcBitsCtrl.fObj.maxFileSize =
                VcapVencVdecVdis_ipcBitsGetMaxFileSizePerChannel(fileNameHdr);
        }
        status =  setvbuf(gVcapVencVdecVdis_ipcBitsCtrl.fObj.fpWrHdr[i],
                          NULL,
                          _IOFBF,
                          MCFW_IPC_BITS_FILEBUF_SIZE_HDR);
        OSA_assert(status != -1);
        gVcapVencVdecVdis_ipcBitsCtrl.fObj.wrapOccuredHdr[i] = FALSE;

        if(gVcapVencVdecVdis_ipcBitsCtrl.fObj.enableLayerWrite == TRUE)
        {
            for (j = 0; j < MCFW_IPC_BITS_MAX_NUM_CHANNELS_LAYERS; j++)
            {
                VcapVencVdecVdis_ipcBitsGenerateFileName(gVcapVencVdecVdis_ipcBitsCtrl.fObj.fileDirPath,
                                           MCFW_IPC_BITS_DATA_FILE_NAME,
                                           i,
                                           j,
                                           gVcapVencVdecVdis_ipcBitsCtrl.fObj.fileExtension,
                                           fileNameBuffer,
                                           sizeof(fileNameBuffer));

                gVcapVencVdecVdis_ipcBitsCtrl.fObj.fpWrData[i][j] = fopen(fileNameBuffer,"wb");
                OSA_assert(gVcapVencVdecVdis_ipcBitsCtrl.fObj.fpWrData[i][j] != NULL);
                status =  setvbuf(gVcapVencVdecVdis_ipcBitsCtrl.fObj.fpWrData[i][j],
                                  NULL,
                                  _IOFBF,
                                  MCFW_IPC_BITS_FILEBUF_SIZE_DATA);
                OSA_assert(status != -1);
                gVcapVencVdecVdis_ipcBitsCtrl.fObj.wrapOccuredBuf[i][j] = FALSE;
            }
        }
        else
        {
             /* NOTE:
                       * Based on enableLayerWrite flag which is specific to h264 bit-stream when disabled will do 
                       * default file write[MCFW_IPC_BITS_DATA_FILE_DEFAULTLAYER_INDEX is 0], 
                       * all other codecs also fall in this case and for when the flag is enabled 
                       * will create multiple file handles per temporal layer */
            VcapVencVdecVdis_ipcBitsGenerateFileName(gVcapVencVdecVdis_ipcBitsCtrl.fObj.fileDirPath,
                                       MCFW_IPC_BITS_DATA_FILE_NAME,
                                       i,
                                       0,
                                       gVcapVencVdecVdis_ipcBitsCtrl.fObj.fileExtension,
                                       fileNameBuffer,
                                       sizeof(fileNameBuffer));

            gVcapVencVdecVdis_ipcBitsCtrl.fObj.fpWrData[i][MCFW_IPC_BITS_DATA_FILE_DEFAULTLAYER_INDEX] 
                                                    = fopen(fileNameBuffer,"wb");
            OSA_assert(gVcapVencVdecVdis_ipcBitsCtrl.fObj.fpWrData[i][MCFW_IPC_BITS_DATA_FILE_DEFAULTLAYER_INDEX] != NULL);
            status =  setvbuf(gVcapVencVdecVdis_ipcBitsCtrl.fObj.fpWrData[i][MCFW_IPC_BITS_DATA_FILE_DEFAULTLAYER_INDEX],
                              NULL,
                              _IOFBF,
                              MCFW_IPC_BITS_FILEBUF_SIZE_DATA);
            OSA_assert(status != -1);
            gVcapVencVdecVdis_ipcBitsCtrl.fObj.wrapOccuredBuf[i][MCFW_IPC_BITS_DATA_FILE_DEFAULTLAYER_INDEX] 
                                                    = FALSE;
                
        }
        VcapVencVdecVdis_ipcBitsGenerateFileName(gVcapVencVdecVdis_ipcBitsCtrl.fObj.fileDirPath,
                                       MCFW_IPC_BITS_DATA_FILE_NAME,
                                       i,
                                       0,
                                       gVcapVencVdecVdis_ipcBitsCtrl.fObj.fileExtension,
                                       fileNameBuffer,
                                       sizeof(fileNameBuffer));
        strncat(fileNameBuffer,".mv",3);

        gVcapVencVdecVdis_ipcBitsCtrl.fObj.fpWrMvData[i] = fopen(fileNameBuffer,"wb");
        OSA_assert(gVcapVencVdecVdis_ipcBitsCtrl.fObj.fpWrMvData[i] != NULL);
        status =  setvbuf(gVcapVencVdecVdis_ipcBitsCtrl.fObj.fpWrMvData[i],
                          NULL,
                          _IOFBF,
                          MCFW_IPC_BITS_FILEBUF_SIZE_DATA);
        OSA_assert(status != -1);
    }
    return status;
}

static
Int   VcapVencVdecVdis_ipcBitsInitFileHandles()
{
    Int i,j;

    for (i = 0; i < MCFW_IPC_BITS_MAX_NUM_CHANNELS; i++)
    {
        MCFW_IPC_BITS_INIT_FILEHANDLE (gVcapVencVdecVdis_ipcBitsCtrl.fObj.fpWrHdr[i]);
        for (j = 0; j < MCFW_IPC_BITS_MAX_NUM_CHANNELS_LAYERS; j++)
        {
            MCFW_IPC_BITS_INIT_FILEHANDLE (gVcapVencVdecVdis_ipcBitsCtrl.fObj.fpWrData[i][j]);
        }
        MCFW_IPC_BITS_INIT_FILEHANDLE (gVcapVencVdecVdis_ipcBitsCtrl.fObj.fpWrMvData[i]);
    }
    return OSA_SOK;
}

static Int32 VcapVencVdecVdis_ipcBitsInitFObj()
{
    static Bool fileDirInputDone = FALSE;

    VcapVencVdecVdis_ipcBitsInitFileHandles();

    if (!fileDirInputDone)
    {
        Demo_getFileWritePath(
                gVcapVencVdecVdis_ipcBitsCtrl.fObj.fileDirPath,
                MCFW_IPC_BITS_FILE_STORE_DIR
                );

        fileDirInputDone = TRUE;
    }

    VcapVencVdecVdis_ipcBitsOpenFileHandles();
    return OSA_SOK;
}


static Int32 VcapVencVdecVdis_ipcBitsInitDmaObj()
{
    Int32 status = OSA_SOK;

    if (gVcapVencVdecVdis_ipcBitsCtrl.dmaObj.useDma)
    {
        status = OSA_dmaInit();
        OSA_assert(status == OSA_SOK);

        status =
        OSA_dmaOpen(&gVcapVencVdecVdis_ipcBitsCtrl.dmaObj.dmaChHdnl,
                    OSA_DMA_MODE_NORMAL,
                    MCFW_IPC_BITS_DMA_MAX_TRANSFERS);
        OSA_assert(status == OSA_SOK);

    }
    if (OSA_SOK != status)
    {
        OSA_printf("DEMO_MCFW_IPC_BITS_CTRL:Disabling DMA as channel open failed[%d]",
                   status);
        gVcapVencVdecVdis_ipcBitsCtrl.dmaObj.useDma = FALSE;
    }
    return status;
}

static Int32 VcapVencVdecVdis_ipcBitsDeInitDmaObj()
{
    Int32 status = OSA_SOK;

    if (gVcapVencVdecVdis_ipcBitsCtrl.dmaObj.useDma)
    {

        status =
        OSA_dmaClose(&gVcapVencVdecVdis_ipcBitsCtrl.dmaObj.dmaChHdnl);
        OSA_assert(status == OSA_SOK);

        status = OSA_dmaExit();
        OSA_assert(status == OSA_SOK);
    }
    return status;
}

Int32 VcapVencVdecVdis_ipcBitsSetFileWriteMask (UInt32 fileWritemask)
{
    gVcapVencVdecVdis_ipcBitsCtrl.fObj.fwriteEnableBitMask = fileWritemask;
    return 0;
}

Int32 VcapVencVdecVdis_ipcBitsSetFileExtension (char *fileExt)
{
    strcpy(gVcapVencVdecVdis_ipcBitsCtrl.fObj.fileExtension, fileExt);
    return 0;
}


Int32 VcapVencVdecVdis_ipcBitsInit(VcapVencVdecVdis_res resolution[], 
                                         Bool enableFWrite,
                                         Bool enableLayerWrite)

{
    VENC_CALLBACK_S callback;
    Int i;

    VcapVencVdecVdis_resetAvgStatistics();
    VcapVencVdecVdis_resetStatistics();

    gVcapVencVdecVdis_ipcBitsCtrl.fObj.enableFWrite = enableFWrite;
    gVcapVencVdecVdis_ipcBitsCtrl.fObj.enableLayerWrite = enableLayerWrite;

    callback.newDataAvailableCb = VcapVencVdecVdis_ipcBitsInCbFxn;
    /* Register call back with encoder */
    Venc_registerCallback(&callback,
                         (Ptr)&gVcapVencVdecVdis_ipcBitsCtrl);

    VcapVencVdecVdis_FreeBitBufInit();
    if (gVcapVencVdecVdis_ipcBitsCtrl.fObj.enableFWrite == TRUE){
        VcapVencVdecVdis_ipcBitsInitFObj();
    }
    VcapVencVdecVdis_ipcBitsInitDmaObj();

    for (i=0; i<MCFW_IPCBITS_RESOLUTION_TYPES; i++)
        gVcapVencVdecVdis_ipcBitsCtrl.thrObj.resolution[i] = resolution[i];

    VcapVencVdecVdis_ipcBitsInitThrObj(&gVcapVencVdecVdis_ipcBitsCtrl.thrObj);
    memset (&gVcapVencVdecVdis_ipcBitsCtrl.decInfo,0,sizeof(gVcapVencVdecVdis_ipcBitsCtrl.decInfo));
    return OSA_SOK;
}

Void VcapVencVdecVdis_ipcBitsStop(void)
{
    gVcapVencVdecVdis_ipcBitsCtrl.thrObj.exitBitsInThread = TRUE;
    gVcapVencVdecVdis_ipcBitsCtrl.thrObj.exitBitsOutThread = TRUE;
}

Int32 VcapVencVdecVdis_ipcBitsExit()
{
    OSA_printf("Entered:%s...",__func__);
    VcapVencVdecVdis_FreeBitBufDeInit();
    if (gVcapVencVdecVdis_ipcBitsCtrl.fObj.enableFWrite == TRUE){
        VcapVencVdecVdis_ipcBitsInitFileHandles();
    }
    VcapVencVdecVdis_ipcBitsDeInitDmaObj();
    VcapVencVdecVdis_ipcBitsDeInitThrObj(&gVcapVencVdecVdis_ipcBitsCtrl.thrObj);
    OSA_printf("Leaving:%s...",__func__);
    return OSA_SOK;
}


Int32 VcapVencVdecVdis_resetStatistics()
{
    UInt32 chId;
    VcapVencVdecVdis_ChInfo *pChInfo;

    for(chId=0; chId<VENC_CHN_MAX; chId++)
    {
        pChInfo = &gVcapVencVdecVdis_ipcBitsCtrl.chInfo[chId];

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

    gVcapVencVdecVdis_ipcBitsCtrl.statsStartTime = OSA_getCurTimeInMsec();

    return 0;
}

Int32 VcapVencVdecVdis_resetAvgStatistics()
{
    gVcapVencVdecVdis_ipcBitsCtrl.totalEncFrames = 0;
    gVcapVencVdecVdis_ipcBitsCtrl.totalDecFrames = 0;

    return 0;
}

Int32 VcapVencVdecVdis_printAvgStatistics(UInt32 elaspedTime, Bool resetStats)
{
    UInt32 totalFrames;

    totalFrames = gVcapVencVdecVdis_ipcBitsCtrl.totalEncFrames+gVcapVencVdecVdis_ipcBitsCtrl.totalDecFrames;

    /* show total average encode FPS */
    printf( " ## AVERAGE: ENCODE [%6.1f] FPS, DECODE [%6.1f] FPS, ENC+DEC [%6.1f] FPS ... in %4.1f secs, \n",
            gVcapVencVdecVdis_ipcBitsCtrl.totalEncFrames*1000.0/elaspedTime,
            gVcapVencVdecVdis_ipcBitsCtrl.totalDecFrames*1000.0/elaspedTime,
            totalFrames*1000.0/elaspedTime,
            elaspedTime/1000.0
            );

    if(resetStats)
        VcapVencVdecVdis_resetAvgStatistics();

    return 0;
}

Int32 VcapVencVdecVdis_printStatistics(Bool resetStats, Bool allChs)
{
    UInt32 chId;
    VcapVencVdecVdis_ChInfo *pChInfo;
    float elaspedTime;

    elaspedTime = OSA_getCurTimeInMsec() - gVcapVencVdecVdis_ipcBitsCtrl.statsStartTime;

    elaspedTime /= 1000.0; // in secs

    if(allChs)
    {
        printf( "\n"
            "\n *** Encode Bitstream Received Statistics *** "
            "\n"
            "\n Elased time = %6.1f secs"
            "\n"
            "\n CH | Bitrate (Kbps) | FPS | Key-frame FPS | Width (max/min) | Height (max/min) | Latency (max/min)"
            "\n --------------------------------------------------------------------------------------------------",
            elaspedTime
            );

        for(chId=0; chId<VENC_CHN_MAX;chId++)
        {
            pChInfo = &gVcapVencVdecVdis_ipcBitsCtrl.chInfo[chId];

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
            VcapVencVdecVdis_resetStatistics();
    }
    else
    {
    }

    return 0;
}

Int32 VcapVencVdecVdis_updateStatistics(VCODEC_BITSBUF_S *pBuf)
{
    VcapVencVdecVdis_ChInfo *pChInfo;
    UInt32 latency;

    if(pBuf->chnId<VENC_CHN_MAX)
    {
        pChInfo = &gVcapVencVdecVdis_ipcBitsCtrl.chInfo[pBuf->chnId];

        pChInfo->totalDataSize += pBuf->filledBufSize;
        pChInfo->numFrames++;
        gVcapVencVdecVdis_ipcBitsCtrl.totalEncFrames++;
        if(pBuf->frameType==VCODEC_FRAME_TYPE_I_FRAME)
        {
            pChInfo->numKeyFrames++;
        }

        latency = pBuf->encodeTimestamp - pBuf->timestamp;

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
    return 0;
}
