/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/
#ifndef _SD_DEMO_COMMON_H_
#define _SD_DEMO_COMMON_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <errno.h>
#include <ctype.h>
#include <sys/wait.h>
#include <sys/types.h>  // For stat().
#include <sys/stat.h>   // For stat().
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <ti_media_std.h>

#include <ti/syslink/utils/IHeap.h>
#include <ti/syslink/utils/Memory.h>
#include <ti/ipc/SharedRegion.h>


#define SD_DEMO_SCD_MAX_TILES                           (9)
#define SD_DEMO_SCD_MAX_CHANNELS                        (16)

/**
    \brief Allocated buffer info
*/
typedef struct {

    UInt8  *physAddr;
    /**< Physical address */

    UInt8  *virtAddr;
    /**< Virtual address */

    UInt32  srPtr;
    /**< Shared region Pointer SRPtr */

} SD_Demo_AllocBufInfo;

typedef Int32 (*VSYS_EVENT_HANDLER_CALLBACK)(UInt32 eventId, Ptr pPrm, Ptr appData);

Int32 Vsys_registerEventHandler(VSYS_EVENT_HANDLER_CALLBACK callback, Ptr appData);


char SD_Demo_getChar();

int  SD_Demo_getChId(char *string, int maxChId);
int  SD_Demo_getIntValue(char *string, int minVal, int maxVal, int defaultVal);

Bool SD_Demo_getFileWriteEnable();
Bool SD_Demo_isPathValid( const char* absolutePath );

int  SD_Demo_getFileWritePath(char *path, char *defaultPath);

Int32 SD_Demo_detectBoard();
Int32 SD_Demo_prfLoadCalcEnable(Bool enable, Bool printStatus, Bool printTskLoad);
Int32 SD_Demo_prfLoadPrint(Bool printTskLoad,Bool resetTskLoad);
Int32 SD_Demo_memPrintHeapStatus();
Int32 SD_Demo_captureGetTamperStatus(Ptr pPrm);
Int32 SD_Demo_captureGetMotionStatus(Ptr pPrm);
Int32 SD_Demo_eventHandler(UInt32 eventId, Ptr pPrm, Ptr appData);

int SD_Demo_enableDisbaleChannel(Bool enableChn);

int SD_Demo_scdTileInit();
int SD_Demo_scdBlockConfigUpdate();

/* API to update Encoder Bitrate */
int SD_Demo_encodeBitrateChange();
/* API to update Encoder Framerate */
int SD_Demo_encodeFramerateChange();

Int32 SD_Demo_allocBuf(UInt32 srRegId, UInt32 bufSize, UInt32 bufAlign, SD_Demo_AllocBufInfo *bufInfo);
Int32 SD_Demo_freeBuf(UInt32 srRegId, UInt8 *virtAddr, UInt32 bufSize);
#endif

