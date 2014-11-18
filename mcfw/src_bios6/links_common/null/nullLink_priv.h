/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#ifndef _NULL_LINK_PRIV_H_
#define _NULL_LINK_PRIV_H_

#include <mcfw/src_bios6/links_common/system/system_priv_common.h>
#include <mcfw/interfaces/link_api/nullLink.h>

#define NULL_LINK_OBJ_MAX   (3)

typedef struct {
    UInt32 tskId;

    Utils_TskHndl tsk;

    NullLink_CreateParams createArgs;

    UInt32 receviedFrameCount;

} NullLink_Obj;

#endif
