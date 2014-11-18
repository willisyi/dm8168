/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#ifndef _SYSTEM_LINK_PRIV_H_
#define _SYSTEM_LINK_PRIV_H_

#include <mcfw/src_bios6/links_c6xdsp/system/system_priv_c6xdsp.h>
#include <mcfw/interfaces/link_api/systemLink_c6xdsp.h>

typedef struct {
    UInt32 tskId;

    Utils_TskHndl tsk;

} SystemLink_Obj;

#endif
