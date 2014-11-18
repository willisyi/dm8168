/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include "system_priv_common.h"

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

UInt32 SystemTiler_alloc(UInt32 cntMode, UInt32 width, UInt32 height)
{
    UInt32 tileAddr = SYSTEM_TILER_INVALID_ADDR;
    SystemCommon_TilerAlloc tilerAlloc;
    Int32 status;

    tilerAlloc.cntMode = cntMode;
    tilerAlloc.width = width;
    tilerAlloc.height = height;

    status = System_linkControl(SYSTEM_TILER_LINK_ID,
                                SYSTEM_COMMON_CMD_TILER_ALLOC,
                                &tilerAlloc, sizeof(tilerAlloc), TRUE);

    if (UTILS_ISERROR(status))
    {
        tileAddr = SYSTEM_TILER_INVALID_ADDR;
    }
    else
    {
        tileAddr = tilerAlloc.tileAddr;
    }
    return tileAddr;
}

Int32 SystemTiler_free(UInt32 tileAddr)
{
    SystemCommon_TilerFree tilerFree;
    Int32 status;

    tilerFree.tileAddr = tileAddr;

    status = System_linkControl(SYSTEM_TILER_LINK_ID,
                                SYSTEM_COMMON_CMD_TILER_FREE,
                                &tilerFree, sizeof(tilerFree), TRUE);
    return status;
}

Int32 SystemTiler_freeAll(void)
{
    Int32 status;

    status = System_linkControl(SYSTEM_TILER_LINK_ID,
                                SYSTEM_COMMON_CMD_TILER_FREE_ALL,
                                NULL, 0, TRUE);

    return status;
}

UInt32 SystemTiler_isAllocatorDisabled(void)
{
    SystemCommon_TilerIsDisabled tilerAllocatorStatus;
    Int32 status;
    UInt32 isDisabled = TRUE;


    status = System_linkControl(SYSTEM_TILER_LINK_ID,
                                SYSTEM_COMMON_CMD_TILER_IS_ALLOCATOR_DISABLED,
                                &tilerAllocatorStatus,
                                sizeof(tilerAllocatorStatus), TRUE);

    if (!UTILS_ISERROR(status))
    {
        isDisabled = tilerAllocatorStatus.isAllocatorDisabled;
    }
    return isDisabled;
}

Ptr SystemTiler_allocRaw(UInt32 size,UInt32 align)
{
    SystemCommon_TilerAllocRaw tilerAllocRaw;
    Int32 status;
    Ptr allocAddr = NULL;

    tilerAllocRaw.size = size;
    tilerAllocRaw.align = align;
    tilerAllocRaw.allocAddr = NULL;

    status = System_linkControl(SYSTEM_TILER_LINK_ID,
                                SYSTEM_COMMON_CMD_TILER_ALLOC_RAW,
                                &tilerAllocRaw,
                                sizeof(tilerAllocRaw), TRUE);

    if (!UTILS_ISERROR(status))
    {
        allocAddr = (Ptr)tilerAllocRaw.allocAddr;
    }
    return allocAddr;
}

Int32 SystemTiler_freeRaw(Ptr addr, UInt32 size)
{
    SystemCommon_TilerFreeRaw tilerFreeRaw;
    Int32 status;

    tilerFreeRaw.allocAddr = (UInt32)addr;
    tilerFreeRaw.size      = size;

    status = System_linkControl(SYSTEM_TILER_LINK_ID,
                                SYSTEM_COMMON_CMD_TILER_FREE_RAW,
                                &tilerFreeRaw,
                                sizeof(tilerFreeRaw), TRUE);
    return status;
}

Int32 SystemTiler_getFreeSize(SystemCommon_TilerGetFreeSize *pPrm)
{
    Int32 status;

    pPrm->freeSizeRaw = 0;
    pPrm->freeSize8b  = 0;
    pPrm->freeSize16b = 0;
    pPrm->freeSize32b = 0;

    status = System_linkControl(SYSTEM_TILER_LINK_ID,
                                SYSTEM_COMMON_CMD_TILER_GET_FREE_SIZE,
                                pPrm,
                                sizeof(*pPrm), TRUE);

    return status;
}

