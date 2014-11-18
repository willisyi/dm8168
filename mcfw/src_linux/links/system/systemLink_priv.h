/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#ifndef _SYSTEM_LINK_PRIV_H_
#define _SYSTEM_LINK_PRIV_H_

#include "system_priv_common.h"

#include <mcfw/interfaces/ti_vsys.h>

typedef struct
{
    UInt32 tskId;

    OSA_TskHndl tsk;

    VSYS_EVENT_HANDLER_CALLBACK eventHandler;

    Ptr eventHandlerAppData;

} SystemLink_Obj;


/* Core status check timeout in Milli Seconds, Set to 1 Second. */
#define SYSTEM_LINK_CORE_STATUS_CHECK_TIMEOUT_MS      1000

#define SYSTEM_LINK_INTERNAL_EVENT_SLAVE_EXCEPTION_BASE                 (0xDEAD)
#define SYSTEM_LINK_INTERNAL_EVENT_SLAVE_EXCEPTION_DSP                  (SYSTEM_LINK_INTERNAL_EVENT_SLAVE_EXCEPTION_BASE + SYSTEM_PROC_DSP)
#define SYSTEM_LINK_INTERNAL_EVENT_SLAVE_EXCEPTION_VIDEOM3              (SYSTEM_LINK_INTERNAL_EVENT_SLAVE_EXCEPTION_BASE + SYSTEM_PROC_M3VIDEO)
#define SYSTEM_LINK_INTERNAL_EVENT_SLAVE_EXCEPTION_VPSSM3               (SYSTEM_LINK_INTERNAL_EVENT_SLAVE_EXCEPTION_BASE + SYSTEM_PROC_M3VPSS)


#define SYSTEM_LINK_IS_SLAVE_CORE_EXCEPTION(cmd)                        ((((cmd) == SYSTEM_LINK_INTERNAL_EVENT_SLAVE_EXCEPTION_DSP)      || \
                                                                          ((cmd) == SYSTEM_LINK_INTERNAL_EVENT_SLAVE_EXCEPTION_VIDEOM3)  || \
                                                                          ((cmd) == SYSTEM_LINK_INTERNAL_EVENT_SLAVE_EXCEPTION_VPSSM3))  ?  \
                                                                          TRUE                                                           :  \
                                                                          FALSE)

#define SYSTEM_LINK_SLAVE_CORE_EXCEPTION_MAP_CMD_TO_SYSTEMPROCID(cmd)    ((cmd) - SYSTEM_LINK_INTERNAL_EVENT_SLAVE_EXCEPTION_BASE)


#endif
