/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2011 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
  \file chains_ipcBits.h
  \brief Chains function related to IPC Bits links
  */


#ifndef _VDEC_VDIS_H_
#define _VDEC_VDIS_H_


#include <demo.h>
#include <libgen.h>
#include "iniparser.h"

//#define IPCBITS_OUT_HOST_DEBUG


#include <osa_que.h>
#include <osa_mutex.h>
#include <osa_thr.h>
#include <osa_sem.h>
#include <sys/time.h>






#define MCFW_IPC_BITS_GENERATE_INI_AUTOMATIC                        0
#define MCFW_IPC_BITS_INI_FILE_NO                                   "file"
#define MCFW_IPC_BITS_INI_FILE_PATH                                 "path"
#define MCFW_IPC_BITS_INI_FILE_WIDTH                                "width"
#define MCFW_IPC_BITS_INI_FILE_HEIGHT                               "height"
#define MCFW_IPC_BITS_INI_FILE_ENABLE                               "enable"
#define MCFW_IPC_BITS_INI_FILE_CODEC                                "codec"
#define MCFW_IPC_BITS_INI_FILE_NUMBUF                               "numbuf"
#define MCFW_IPC_BITS_INI_FILE_DISPLAYDELAY                         "displaydelay"
#define MCFW_IPC_BITS_MAX_FILE_NUM_IN_INI                           100
#define MCFW_IPC_BITS_MAX_RES_NUM                                   10


#define MCFW_IPC_BITS_MAX_FULL_PATH_FILENAME_LENGTH                 (256)

#define MCFW_IPCBITS_SENDFXN_TSK_PRI                                (2)

#define MCFW_IPCBITS_SENDFXN_TSK_STACK_SIZE                         (0) /* 0 means system default will be used */

#define MCFW_IPCBITS_SENDFXN_PERIOD_MS                              (16)

#define MCFW_IPCBITS_INFO_PRINT_INTERVAL                            (1000)

#define MCFW_IPCFRAMES_SENDFXN_TSK_PRI                              (2)

#define MCFW_IPCFRAMES_SENDFXN_TSK_STACK_SIZE                       (0) /* 0 means system default will be used */

#define MCFW_IPCFRAMES_SENDFXN_PERIOD_MS                            (30)

#define MCFW_IPCFRAMES_MAX_NUM_ALLOC_FRAMES                         (4)

#define MCFW_IPCFRAMES_FRAME_INVALID_WIDTH                          (~0u)

#define MCFW_IPCFRAMES_FRAME_INVALID_HEIGHT                         (~0u)

#define MCFW_IPCFRAMES_FRAME_BUF_ALIGN                              (128)

#define MCFW_IPCFRAMES_SRID                                         (0)

#define MCFW_IPCBITS_GET_BITBUF_SIZE(width,height)                  ((width) * (height)/2)
#define MCFW_IPCBITS_GET_YUV422_FRAME_SIZE(width,height)            ((width) * (height) * 2)


typedef struct VdecVdis_AudioSyncCtrl{
    Uint64 timeStamp64;
    Bool   gotAudioTS;
    Bool   doDropFrame;
    Uint64 tsIncPerCh[VDEC_CHN_MAX];

} VdecVdis_AudioSyncCtrl;

typedef struct {

    FILE    *fpRdHdr[MCFW_IPC_BITS_MAX_FILE_NUM_IN_INI];

    int      fdRdData[MCFW_IPC_BITS_MAX_FILE_NUM_IN_INI];

    UInt32 lastId[MCFW_IPC_BITS_MAX_RES_NUM];

    OSA_ThrHndl   thrHandle;
    OSA_SemHndl   thrStartSem;
    volatile Bool thrExit;
    VdecVdis_AudioSyncCtrl audioSyncCtrl;
    Bool    enable64BitTimeStamp;
    volatile Bool switchInputFile;
    int FileIndex;
    int chId;
} VdecVdis_IpcBitsCtrl;


typedef struct {

    char    path[MCFW_IPC_BITS_MAX_FULL_PATH_FILENAME_LENGTH];
    UInt32  width;
    UInt32  height;
    Bool    enable;
    UInt32  validResId;
    char    codec[10];
    Int32  displaydelay;
    UInt32  numbuf;

} VdecVdis_FileInfo;

typedef struct {

    UInt32 width;
    UInt32 height;

} VdecVdis_res;

typedef struct VdecVdis_IpcFramesCtrlThrObj {
    OSA_ThrHndl thrHandleFramesOut;
    OSA_SemHndl   thrStartSem;
    volatile Bool exitFramesOutThread;
} VdecVdis_IpcFramesCtrlThrObj;

typedef struct VdecVdis_IpcFramesBufObj {
    Ptr        bufVirt;
    UInt32     bufPhy;
    UInt32     refCnt;
    UInt32     maxWidth;
    UInt32     maxHeight;
    UInt32     curWidth;
    UInt32     curHeight;
    UInt32     numCh;
} VdecVdis_IpcFramesBufObj;

typedef struct VdecVdis_IpcFramesCtrl {
    VdecVdis_IpcFramesCtrlThrObj  thrObj;
    VdecVdis_IpcFramesBufObj      frameObj[MCFW_IPCFRAMES_MAX_NUM_ALLOC_FRAMES];
} VdecVdis_IpcFramesCtrl;


typedef struct {
    UInt32 channel;
    UInt16 speed;

} VdecVdis_TrickPlayMode;

typedef struct {

    UInt32              fileNum;
    UInt32              numRes; // Numbers of resolution in vaild file
    VdecVdis_res        res[MCFW_IPC_BITS_MAX_RES_NUM];
    UInt32              numChnlInRes[MCFW_IPC_BITS_MAX_RES_NUM]; // file numbers in each valid resolution
    UInt32              resToChnl[MCFW_IPC_BITS_MAX_RES_NUM][MCFW_IPC_BITS_MAX_FILE_NUM_IN_INI]; // chnl IDs array for each res
    VdecVdis_FileInfo   fileInfo[MCFW_IPC_BITS_MAX_FILE_NUM_IN_INI];
    UInt32              frameCnt[MCFW_IPC_BITS_MAX_FILE_NUM_IN_INI];
    VdecVdis_IpcFramesCtrl ipcFrames;
    VdecVdis_TrickPlayMode   gDemo_TrickPlayMode;
}VdecVdis_Config;

extern VdecVdis_Config  gVdecVdis_config;



Int32 VdecVdis_bitsRdInit();
Int32 VdecVdis_bitsRdExit();

Void  VdecVdis_bitsRdStop(void);
Void  VdecVdis_bitsRdStart(void);

Int32 VdecVdis_ipcFramesCreate();
Void  VdecVdis_ipcFramesStart(void);
Void  VdecVdis_ipcFramesStop(void);
Int32 VdecVdis_ipcFramesDelete();
Void  VdecVdis_switchInputFile(UInt32 chId, UInt32 FileIndex);

#endif /* CHAINS_IPCBITS_H_ */
