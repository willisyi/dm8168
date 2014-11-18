/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#ifndef _MERGE_LINK_PRIV_H_
#define _MERGE_LINK_PRIV_H_

#include <mcfw/src_bios6/links_common/system/system_priv_common.h>
#include <mcfw/interfaces/link_api/mergeLink.h>

#define MERGE_LINK_OBJ_MAX   (5)

#define MERGE_LINK_MAX_CH_PER_IN_QUE    (SYSTEM_MAX_CH_PER_OUT_QUE)

typedef struct MergeLink_statsObj {
    UInt32 recvCount[MERGE_LINK_MAX_IN_QUE];
    UInt32 forwardCount;
    UInt32 releaseCount[MERGE_LINK_MAX_IN_QUE];
} MergeLink_statsObj;

typedef struct {
    UInt32 tskId;

    Utils_TskHndl tsk;

    MergeLink_CreateParams createArgs;

    System_LinkInfo inTskInfo[MERGE_LINK_MAX_IN_QUE];

    System_LinkInfo info;

    Semaphore_Handle lock;

    Utils_BufHndl outFrameQue;

    FVID2_FrameList inFrameList;
    FVID2_FrameList freeFrameList[MERGE_LINK_MAX_IN_QUE];

    /* max channel number possible in a input que */
    UInt32 inQueMaxCh[MERGE_LINK_MAX_IN_QUE];

    /* incoming channel number to outgoing channel number map */
    UInt32 inQueChNumMap[MERGE_LINK_MAX_IN_QUE][MERGE_LINK_MAX_CH_PER_IN_QUE];

    /* outgoing channel number to input que ID map */
    UInt32 outQueChToInQueMap[MERGE_LINK_MAX_IN_QUE *
                              MERGE_LINK_MAX_CH_PER_IN_QUE];

    /* outgoing channel number to incoming channel number map, reverse of
     * inQueChNumMap[] */
    UInt32 outQueChMap[MERGE_LINK_MAX_IN_QUE * MERGE_LINK_MAX_CH_PER_IN_QUE];

    MergeLink_statsObj stats;
} MergeLink_Obj;

#endif
