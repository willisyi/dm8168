/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2011 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
    \file  demo_vcap_venc_vdec_vdis_bits_rdwr.h
    \brief Chains function related to IPC Bits links
*/


#ifndef _DEMO_VCAP_VENC_VDEC_VDIS_BITS_RDWR_H_
#define _DEMO_VCAP_VENC_VDEC_VDIS_BITS_RDWR_H_

#include <demo.h>

//#define IPC_BITS_DEBUG

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <errno.h>
#include <ctype.h>
#include <sys/wait.h>
#include <sys/types.h>  // For stat().
#include <sys/stat.h>   // For stat().
#include <sys/statvfs.h>// For statvfs()
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <osa.h>
#include <osa_que.h>
#include <osa_mutex.h>
#include <osa_thr.h>
#include <osa_sem.h>
#include <osa_dma.h>

#define MCFW_IPC_BITS_MAX_FILE_SIZE_INFINITY                          (~(0u))
#ifdef KB
#undef KB
#endif

#ifdef MB
#undef MB
#endif

#ifdef GB
#undef GB
#endif

#define KB                                                               (1024)
#define MB                                                               (KB*KB)
#define GB                                                               (KB*KB*KB)


#define MCFW_IPC_BITS_MAX_FILENAME_LENGTH                                 (64)
#define MCFW_IPC_BITS_MAX_PATH_LENGTH                                     (256)
#define MCFW_IPC_BITS_MAX_FULL_PATH_FILENAME_LENGTH                       (MCFW_IPC_BITS_MAX_PATH_LENGTH+MCFW_IPC_BITS_MAX_FILENAME_LENGTH)
#define MCFW_IPC_BITS_FILE_STORE_DIR                                      "/dev/shm"
#define MCFW_IPC_BITS_HDR_FILE_NAME                                       "VBITS_HDR"
#define MCFW_IPC_BITS_DATA_FILE_NAME                                      "VBITS_DATA"
#define MCFW_IPC_BITS_DEFAULT_FILE_EXTENSION                             "bin"
#define MCFW_IPC_BITS_DATA_FILE_DEFAULTLAYER_INDEX                        0

#define MCFW_IPC_BITS_MAX_NUM_CHANNELS                                    (48)
#define MCFW_IPC_BITS_MAX_NUM_CHANNELS_LAYERS                             (4)
#define MCFW_IPC_BITS_NONOTIFYMODE_BITSIN                                 (TRUE)
#define MCFW_IPC_BITS_NONOTIFYMODE_BITSOUT                                (TRUE)

#define MCFW_IPC_BITS_MAX_DEFAULT_SIZE                                    (2*MB)
#define MCFW_IPC_BITS_FREE_SPACE_RETRY_MS                                 (16)
#define MCFW_IPC_BITS_FREE_SPACE_MAX_RETRY_CNT                            (500)

#define MCFW_IPC_BITS_MAX_BUFCONSUMEWAIT_MS                               (1000)

#define MCFW_IPC_BITS_MAX_FILE_SIZE           (MCFW_IPC_BITS_MAX_FILE_SIZE_INFINITY)

#define MCFW_IPC_BITS_INIT_FILEHANDLE(fp)                                    \
                                                   do {                        \
                                                       if (fp != NULL)         \
                                                       {                       \
                                                           fclose(fp);         \
                                                           fp = NULL;          \
                                                       }                       \
                                                   } while (0)

#define MCFW_IPC_BITS_ENCODER_FPS                      (30)
#define MCFW_IPC_BITS_ENCODER_BITRATE                  (2 * 1024 * 1024)
#define MCFW_IPC_BITS_FILEBUF_SIZE_HDR                 (sizeof(VCODEC_BITSBUF_S) * MCFW_IPC_BITS_ENCODER_FPS)
#define MCFW_IPC_BITS_FILEBUF_SIZE_DATA                (MCFW_IPC_BITS_ENCODER_BITRATE)

#define MCFW_IPCBITS_SENDFXN_TSK_PRI                   (2)
#define MCFW_IPCBITS_RECVFXN_TSK_PRI                   (2)

#define MCFW_IPCBITS_SENDFXN_TSK_STACK_SIZE            (0) /* 0 means system default will be used */
#define MCFW_IPCBITS_RECVFXN_TSK_STACK_SIZE            (0) /* 0 means system default will be used */

#define MCFW_IPCBITS_SENDFXN_PERIOD_MS                 (8)
#define MCFW_IPCBITS_RECVFXN_PERIOD_MS                 (8)

#define MCFW_IPCBITS_INFO_PRINT_INTERVAL               (1000)

/** @enum MCFW_IPCBITS_GET_BITBUF_SIZE
 *  @brief Macro that returns max size of encoded bitbuffer for a given resolution
 */
#define MCFW_IPCBITS_HD_WIDTH                   (1920)
#define MCFW_IPCBITS_HD_HEIGHT                  (1080)

#define MCFW_IPCBITS_D1_WIDTH                   (720)
#define MCFW_IPCBITS_D1_HEIGHT                  (576)

#define MCFW_IPCBITS_CIF_WIDTH                   (352)
#define MCFW_IPCBITS_CIF_HEIGHT                  (288)

#define MCFW_IPCBITS_GET_BITBUF_SIZE(width,height)   ((width) * (height)/2)

#define MCFW_IPCBITS_RESOLUTION_TYPES               2  /* Bit bufs can be D1 & CIF resolution in some usecases */

#define MCFW_IPCBITS_MAX_PENDING_RECV_SEM_COUNT      (10)

#define MCFW_IPCBITS_MAX_NUM_FREE_BUFS_PER_CHANNEL    (6)
#define MCFW_IPCBITS_FREE_QUE_MAX_LEN                 (MCFW_IPC_BITS_MAX_NUM_CHANNELS * \
                                                         MCFW_IPCBITS_MAX_NUM_FREE_BUFS_PER_CHANNEL)
#define MCFW_IPCBITS_FULL_QUE_MAX_LEN                 (MCFW_IPCBITS_FREE_QUE_MAX_LEN)

#define MCFW_IPC_BITS_ENABLE_FILE_WRITE               (TRUE)
#define MCFW_IPC_BITS_ENABLE_FILE_LAYERS_WRITE        (FALSE)

#define MCFW_IPC_BITS_FWRITE_ENABLE_BITMASK_CHANNEL_0     (1 << 0)
#define MCFW_IPC_BITS_FWRITE_ENABLE_BITMASK_CHANNEL_1     (1 << 1)
#define MCFW_IPC_BITS_FWRITE_ENABLE_BITMASK_CHANNEL_0_CHANNEL16   ((1 << 0) | (1<<16))
#define MCFW_IPC_BITS_FWRITE_ENABLE_BITMASK_ALLCHANNELS   (0xFFFFFFFF)
#define MCFW_IPC_BITS_FWRITE_ENABLE_BITMASK_CHANNEL_0_CHANNEL_1_CHANNEL16 ((1 << 0) | (1<<4) | (1<<16))
#define MCFW_IPC_BITS_FWRITE_ENABLE_BITMASK_CHANNEL_0_CHANNEL4   ((1 << 0) | (1<<4))
#define MCFW_IPC_BITS_FWRITE_ENABLE_BITMASK_CHANNEL_0_CHANNEL4_CHANNEL_8   ((1 << 0) | (1<<4) | (1<<8))
#define MCFW_IPC_BITS_FWRITE_ENABLE_BITMASK_CHANNEL_10_CHANNEL3   ((1 << 10) | (1<< 3))
#define MCFW_IPC_BITS_FWRITE_ENABLE_BITMASK_CHANNEL_3_CHANNEL16   ((1 << 3) | (1<<16))
#define MCFW_IPC_BITS_FWRITE_ENABLE_BITMASK_CHANNEL_0_TO_15  (0xFFFF)

#if defined(TI_814X_BUILD) || defined(TI_8107_BUILD)
#define MCFW_IPC_BITS_FWRITE_ENABLE_BITMASK_DEFAULT   (MCFW_IPC_BITS_FWRITE_ENABLE_BITMASK_CHANNEL_0_CHANNEL4_CHANNEL_8)
#else
#define MCFW_IPC_BITS_FWRITE_ENABLE_BITMASK_DEFAULT   (MCFW_IPC_BITS_FWRITE_ENABLE_BITMASK_CHANNEL_0_CHANNEL16)
#endif
#define MCFW_IPC_BITS_MAX_PARTITION_USAGE_PERCENT     (95)
#define MCFW_IPC_BITS_MAX_SINGLE_FILE_SIZE            (GB + (GB/2))

#define MCFW_IPC_BITS_USE_DMA_FOR_BITBUF_COPY         (TRUE)
#define MCFW_IPC_BITS_DMA_MAX_TRANSFERS               (4)

enum VcapVencVdecVdis_IpcBitsFileType {
    MCFW_IPC_BITS_FILETYPE_HDR,
    MCFW_IPC_BITS_FILETYPE_BUF
} ;

typedef struct {

    UInt32 totalDataSize;
    UInt32 numKeyFrames;
    UInt32 numFrames;
    UInt32 maxWidth;
    UInt32 minWidth;
    UInt32 maxHeight;
    UInt32 minHeight;
    UInt32 maxLatency;
    UInt32 minLatency;

} VcapVencVdecVdis_ChInfo;


typedef struct VcapVencVdecVdis_IpcBitsCtrlFileObj {
    FILE *fpWrHdr[MCFW_IPC_BITS_MAX_NUM_CHANNELS];
    FILE *fpWrData[MCFW_IPC_BITS_MAX_NUM_CHANNELS][MCFW_IPC_BITS_MAX_NUM_CHANNELS_LAYERS];
    FILE *fpWrMvData[MCFW_IPC_BITS_MAX_NUM_CHANNELS];
    char    fileDirPath[MCFW_IPC_BITS_MAX_PATH_LENGTH];
    Bool    wrapOccuredHdr[MCFW_IPC_BITS_MAX_NUM_CHANNELS];
    Bool    wrapOccuredBuf[MCFW_IPC_BITS_MAX_NUM_CHANNELS][MCFW_IPC_BITS_MAX_NUM_CHANNELS_LAYERS];
    UInt32  maxFileSize;
    Bool    enableFWrite;
    Bool    enableLayerWrite;    
    UInt32  fwriteEnableBitMask;
    char    fileExtension[MCFW_IPC_BITS_MAX_FILENAME_LENGTH];
} VcapVencVdecVdis_IpcBitsCtrlFileObj;

typedef struct {

    UInt32 width;
    UInt32 height;

} VcapVencVdecVdis_res;

typedef struct VcapVencVdecVdis_IpcBitsCtrlThrObj {
    OSA_ThrHndl thrHandleBitsIn;
    OSA_ThrHndl thrHandleBitsOut;
    OSA_QueHndl bufQFullBufs;
    OSA_QueHndl bufQFreeBufs;
    OSA_SemHndl bitsInNotifySem;
    volatile Bool exitBitsInThread;
    volatile Bool exitBitsOutThread;
    VcapVencVdecVdis_res  resolution[MCFW_IPCBITS_RESOLUTION_TYPES];
} VcapVencVdecVdis_IpcBitsCtrlThrObj;

typedef struct VcapVencVdecVdis_IpcBitsDmaObj {
    OSA_DmaChHndl dmaChHdnl;
    Bool          useDma;
} VcapVencVdecVdis_IpcBitsDmaObj;

typedef struct VcapVencVdecVdis_IpcBitsCtrl {
    Bool  noNotifyBitsInHLOS;
    Bool  noNotifyBitsOutHLOS;;
    VcapVencVdecVdis_IpcBitsCtrlFileObj fObj;
    VcapVencVdecVdis_IpcBitsCtrlThrObj  thrObj;
    VcapVencVdecVdis_IpcBitsDmaObj      dmaObj;

    VcapVencVdecVdis_ChInfo chInfo[VENC_CHN_MAX];
    VcapVencVdecVdis_ChInfo decInfo[VDEC_CHN_MAX];

    UInt32 statsStartTime;
    UInt32 totalEncFrames;
    UInt32 totalDecFrames;

} VcapVencVdecVdis_IpcBitsCtrl;


#define APP_IPCBITSCTRL_FREE_BITBUFINFO_TBL_SIZE                            (128)
#define APP_IPCBITSCTRL_FREE_BITBUFINFO_INVALIDID                           (~0u)

typedef struct VcapVencVdecVdis_FreeBitBufInfoTbl
{
    OSA_MutexHndl mutex;
    UInt32        freeIndex;
    struct VcapVencVdecVdis_FreeBitBufInfoEntry
    {
        VCODEC_BITSBUF_S bitBuf;
        UInt32           nextFreeIndex;
    } tbl[APP_IPCBITSCTRL_FREE_BITBUFINFO_TBL_SIZE];
} VcapVencVdecVdis_FreeBitBufInfoTbl;

Int32 VcapVencVdecVdis_ipcBitsSetFileWriteMask (UInt32 fileWritemask);
Int32 VcapVencVdecVdis_ipcBitsInit(VcapVencVdecVdis_res resolution[], 
                                         Bool enableFWrite,
                                         Bool enableLayerWrite);
Int32 VcapVencVdecVdis_ipcBitsExit();
Int32 VcapVencVdecVdis_ipcBitsSetFileExtension (char *fileExt);

Void VcapVencVdecVdis_ipcBitsInCbFxn(Ptr cbCtx);
Void  VcapVencVdecVdis_ipcBitsStop(void);

Int32 VcapVencVdecVdis_resetStatistics();
Int32 VcapVencVdecVdis_printStatistics(Bool resetStats, Bool allChs);
Int32 VcapVencVdecVdis_updateStatistics(VCODEC_BITSBUF_S *pBuf);
Int32 VcapVencVdecVdis_resetAvgStatistics();
Int32 VcapVencVdecVdis_printAvgStatistics(UInt32 elaspedTime, Bool resetStats);

#endif
