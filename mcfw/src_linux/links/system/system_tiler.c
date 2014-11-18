/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2011 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
    \file  system_tiler.c
    \brief HLOS interface to tiler
*/

/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include "system_priv_common.h"
#include <mcfw/interfaces/link_api/system_tiler.h>
#include <mcfw/interfaces/link_api/systemLink_common.h>

#if(SYSTEM_TILER_ALLOCATOR_PROC_ID == SYSTEM_PROC_DSP)
#    define  SYSTEM_TILER_LINK_ID                           (SYSTEM_LINK_ID_DSP)
#else
#    if(SYSTEM_TILER_ALLOCATOR_PROC_ID == SYSTEM_PROC_M3VIDEO)
#        define  SYSTEM_TILER_LINK_ID                   (SYSTEM_LINK_ID_M3VIDEO)
#    else
#        if (SYSTEM_TILER_ALLOCATOR_PROC_ID == SYSTEM_PROC_M3VPSS)
#            define  SYSTEM_TILER_LINK_ID                (SYSTEM_LINK_ID_M3VPSS)
#        else
#            if (SYSTEM_TILER_ALLOCATOR_PROC_ID == SYSTEM_PROC_HOSTA8)
#                define  SYSTEM_TILER_LINK_ID               (SYSTEM_PROC_HOSTA8)
#            else
#                error "Unknown PROC ID:"##SYSTEM_TILER_ALLOCATOR_PROC_ID
#            endif
#        endif
#    endif
#endif

UInt32 SystemTiler_isAllocatorDisabled(void)
{
    SystemCommon_TilerIsDisabled tilerAllocatorStatus;
    Int32 status;
    UInt32 isDisabled = TRUE;


    status = System_linkControl(SYSTEM_TILER_LINK_ID,
                                SYSTEM_COMMON_CMD_TILER_IS_ALLOCATOR_DISABLED,
                                &tilerAllocatorStatus,
                                sizeof(tilerAllocatorStatus), TRUE);

    if (!OSA_ISERROR(status))
    {
        isDisabled = tilerAllocatorStatus.isAllocatorDisabled;
    }
    return isDisabled;
}


Int32 SystemTiler_disableAllocator(void)
{
    Int32 status;

    status = System_linkControl(SYSTEM_TILER_LINK_ID,
                                SYSTEM_COMMON_CMD_TILER_DISABLE_ALLOCATOR,
                                NULL, 0, TRUE);

    return status;
}


Int32 SystemTiler_enableAllocator(void)
{
    Int32 status;

    status = System_linkControl(SYSTEM_TILER_LINK_ID,
                                SYSTEM_COMMON_CMD_TILER_ENABLE_ALLOCATOR,
                                NULL, 0, TRUE);

    return status;
}

