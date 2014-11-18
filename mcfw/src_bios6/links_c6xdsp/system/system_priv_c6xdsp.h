/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#ifndef _SYSTEM_PRIV_M3VIDEO_H_
#define _SYSTEM_PRIV_M3VIDEO_H_

#include <mcfw/src_bios6/links_common/system/system_priv_common.h>

#include <mcfw/interfaces/link_api/system.h>
#include <mcfw/interfaces/link_api/nullLink.h>
#include <mcfw/interfaces/link_api/dupLink.h>
#include <mcfw/interfaces/link_api/mergeLink.h>
#include <mcfw/interfaces/link_api/ipcLink.h>
#include <mcfw/interfaces/link_api/selectLink.h>
#include <mcfw/interfaces/link_api/systemLink_c6xdsp.h>

/**
 * \brief System Task Descriptor
 *
 */
typedef struct {
    Void *rsv;

} System_DspObj;

typedef enum System_DspLinkState {
    SYSTEM_LINK_STATE_START = 0,
    SYSTEM_LINK_STATE_STOP
} System_DspLinkState;

extern System_DspObj gSystem_objDsp;

#endif
