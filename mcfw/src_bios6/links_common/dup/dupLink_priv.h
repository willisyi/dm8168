/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#ifndef _DUP_LINK_PRIV_H_
#define _DUP_LINK_PRIV_H_

#include <mcfw/src_bios6/links_common/system/system_priv_common.h>
#include <mcfw/interfaces/link_api/dupLink.h>

#define DUP_LINK_OBJ_MAX   (4)

#define DUP_LINK_MAX_FRAMES_PER_OUT_QUE		(SYSTEM_LINK_FRAMES_PER_CH*SYSTEM_MAX_CH_PER_OUT_QUE)

typedef struct DupLink_statsObj {
    UInt32 recvCount;
    UInt32 forwardCount[DUP_LINK_MAX_OUT_QUE];
    UInt32 releaseCount[DUP_LINK_MAX_OUT_QUE];
} DupLink_statsObj;


typedef struct {
    UInt32 tskId;

    Utils_TskHndl tsk;

    DupLink_CreateParams createArgs;

    UInt32 getFrameCount;
    UInt32 putFrameCount;

    System_LinkInfo inTskInfo;

    System_LinkInfo info;

    Utils_BufHndl outFrameQue[DUP_LINK_MAX_OUT_QUE];

    FVID2_Frame frames[DUP_LINK_MAX_OUT_QUE * DUP_LINK_MAX_FRAMES_PER_OUT_QUE];

    System_FrameInfo frameInfo[DUP_LINK_MAX_OUT_QUE *
                               DUP_LINK_MAX_FRAMES_PER_OUT_QUE];

    Semaphore_Handle lock;

    FVID2_FrameList inFrameList;
    FVID2_FrameList outFrameList[DUP_LINK_MAX_OUT_QUE];
    DupLink_statsObj stats;

} DupLink_Obj;

#endif
