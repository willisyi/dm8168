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
#include <mcfw/interfaces/link_api/encLink.h>
#include <mcfw/interfaces/link_api/decLink.h>
#include <mcfw/interfaces/link_api/nullLink.h>
#include <mcfw/interfaces/link_api/dupLink.h>
#include <mcfw/interfaces/link_api/mergeLink.h>
#include <mcfw/interfaces/link_api/nullSrcLink.h>
#include <mcfw/interfaces/link_api/ipcLink.h>
#include <mcfw/interfaces/link_api/systemLink_m3vpss.h>
#include <mcfw/interfaces/link_api/systemLink_m3video.h>
#include <mcfw/interfaces/link_api/algLink.h>

/**
 * \brief System Task Descriptor
 *
 */
typedef struct {
    Void *rsv;

} System_VideoObj;

typedef enum System_VideoLinkState {
    SYSTEM_LINK_STATE_START = 0,
    SYSTEM_LINK_STATE_STOP
} System_VideoLinkState;

extern System_VideoObj gSystem_objVideo;

#endif
