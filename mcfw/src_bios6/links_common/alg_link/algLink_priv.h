/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#ifndef _ALG_LINK_PRIV_H_
#define _ALG_LINK_PRIV_H_

#include <mcfw/src_bios6/utils/utils.h>
#include <mcfw/src_bios6/links_common/system/system_priv_common.h>
#include <mcfw/interfaces/link_api/algLink.h>
#include <mcfw/src_bios6/links_common/alg_link/swosd/osdLink_priv.h>
#include <mcfw/src_bios6/links_common/alg_link/scd/scdLink_priv.h>

#define ALG_LINK_OBJ_MAX                     (2)

typedef struct AlgLink_Obj {
    UInt32 linkId;

    char name[32];

    Utils_TskHndl tsk;
    System_LinkInfo inTskInfo;
    System_LinkQueInfo inQueInfo;

    AlgLink_CreateParams createArgs;

    System_LinkInfo info;

    AlgLink_OsdObj osdAlg;
    /**< handle to OSD algorithm */

    AlgLink_ScdObj scdAlg;
    /**< handle to SCD algorithm */

    UInt32 memUsed[UTILS_MEM_MAXHEAPS];

    Utils_BufHndl framesOutBufQue;

} AlgLink_Obj;


Int32 AlgLink_algCreate(AlgLink_Obj * pObj, AlgLink_CreateParams * pPrm);
Int32 AlgLink_algProcessData(Utils_TskHndl *pTsk, AlgLink_Obj * pObj);
Int32 AlgLink_algDelete(AlgLink_Obj * pObj);
Int32 AlgLink_getFullFrames(Utils_TskHndl * pTsk, UInt16 queId,
                           FVID2_FrameList * pBufList);
Int32 AlgLink_putEmptyFrames(Utils_TskHndl * pTsk, UInt16 queId,
                           FVID2_FrameList * pBufList);
Int32 AlgLink_getInfo(Utils_TskHndl * pTsk, System_LinkInfo * info);

#endif

