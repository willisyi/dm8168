/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#ifndef _ALG_LINK_PRIV_H_
#define _ALG_LINK_PRIV_H_

#include <mcfw/src_bios6/utils/utils.h>
#include <mcfw/src_bios6/links_c6xdsp/system/system_priv_c6xdsp.h>
#include <mcfw/interfaces/link_api/algLink.h>
#include <mcfw/src_bios6/links_c6xdsp/alg_link/scd/scdLink_priv.h>
#include <mcfw/src_bios6/links_c6xdsp/alg_link/swosd/osdLink_priv.h>

#define ALG_LINK_OBJ_MAX                     (SYSTEM_LINK_ID_ALG_COUNT)

typedef enum AlgLink_State {
    ALG_LINK_STATE_INACTIVE = 0,
    /*<< Alg link state: Alg Link not created or deleted */

    ALG_LINK_STATE_ACTIVE
    /*<< Alg link state: Alg Link created and active */

}AlgLink_State;

typedef struct AlgLink_Obj {
    UInt32 linkId;

    char name[32];

    Bool isCreated;
    /** Link state 
      *  ALG_LINK_STATE_ACTIVE  : Link is created and active 
      *  ALG_LINK_STATE_INACTIVE: Link is deleted and in-active */

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

    UInt32  inFrameGetCount;
} AlgLink_Obj;


Int32 AlgLink_algCreate(AlgLink_Obj * pObj, AlgLink_CreateParams * pPrm);

Int32 AlgLink_algProcessData(AlgLink_Obj * pObj);

Int32 AlgLink_algDelete(AlgLink_Obj * pObj);
Int32 AlgLink_getFullBufs(Utils_TskHndl * pTsk, UInt16 queId,
                           Bitstream_BufList * pBufList);
Int32 AlgLink_putEmptyBufs(Utils_TskHndl * pTsk, UInt16 queId,
                           Bitstream_BufList * pBufList);
Int32 AlgLink_getInfo(Utils_TskHndl * pTsk, System_LinkInfo * info);
#endif

