/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#ifndef _NULL_SRC_LINK_PRIV_H_
#define _NULL_SRC_LINK_PRIV_H_

#include <mcfw/src_bios6/links_common/system/system_priv_common.h>
#include <mcfw/interfaces/link_api/nullSrcLink.h>

#define NULL_SRC_LINK_OBJ_MAX                     (3)

#define NULL_SRC_LINK_MAX_OUT_FRAMES              (SYSTEM_MAX_CH_PER_OUT_QUE*SYSTEM_LINK_FRAMES_PER_CH)

typedef struct {
    UInt32 tskId;

    Utils_TskHndl tsk;

    NullSrcLink_CreateParams createArgs;

    FVID2_Frame outFrames[NULL_SRC_LINK_MAX_OUT_FRAMES];
    System_FrameInfo frameInfo[NULL_SRC_LINK_MAX_OUT_FRAMES];
    FVID2_Format outFormat;

    Utils_BufHndl bufOutQue;

    Clock_Handle timer;

    /* Video source link info that is returned when queried by next link */
    System_LinkInfo info;

    UInt32 chNextFid[SYSTEM_MAX_CH_PER_OUT_QUE];

} NullSrcLink_Obj;

#endif
