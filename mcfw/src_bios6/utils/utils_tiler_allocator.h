/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
    \ingroup UTILS_API
    \defgroup UTILS_TILER_API Tiler allocator API
    @{
*/

#ifndef __UTILS_TILER_ALLOCATOR_H_
#define __UTILS_TILER_ALLOCATOR_H_

#include <mcfw/src_bios6/utils/utils_mem.h>
#include <mcfw/src_bios6/utils/utils_tiler.h>




/*
 * Init tiler memory */
Int32 Utils_tilerAllocatorInit();

/*
 * De-init tiler memory */
Int32 Utils_tilerAllocatorDeInit();

/*
 * Enable/disable debug log for tiler APIs */
Int32 Utils_tilerAllocatorDebugLogEnable(UInt32 enable);

/**
 * \brief Alloc from tiler space.
 *
 * Value can be passed directly to VPDMA
 *
 * \param cntMode [IN] container mode
 * \param width [IN] width in BYTES
 * \param height [IN] height in lines
 *
 * \return Tiler address
 */
UInt32 Utils_tilerAllocatorAlloc(UInt32 cntMode, UInt32 width, UInt32 height);

/**
 * \brief Free all previously allocated tiler frames
 */
Int32 Utils_tilerAllocatorFree(UInt32 tileAddr);

/**
 * \brief Free all previously allocated tiler frames
 */
Int32 Utils_tilerAllocatorFreeAll();

/**
 * \brief Disable tiler allocation completely.
 *
 * This can be used to disable tiler allocation
 * at system initialization time so that
 * the tiler physical memory can be allocated
 * for some other purpose.
 */
Int32 Utils_tilerAllocatorDisable();

/**
 * \brief Enable tiler allocation.
 */
Int32 Utils_tilerAllocatorEnable();

/**
 * \brief Returns flag indicating tiler allocation has been disabled or not
 *
 * If Tiler allocation has been disabled at either build or at run time
 * this function will return TRUE else FALSE
 */
UInt32 Utils_tilerAllocatorIsDisabled();

/**
 * \brief Used to allocate raw (non-tiled) memory.
 *
 * The tiler allocator can be disabled at runtime and
 * the memory allocated for tiled physical memory
 * can be used to allocate raw memory.
 * Tiler allocation must be disabled for this API to work
 * else API will fail and return NULL.
*/
Ptr Utils_tilerAllocatorAllocRaw(UInt32 size, UInt32 align);

/**
 * \brief Used to free raw (non-tiled) memory.
 *
 * The tiler allocator can be disabled at runtime and
 * the memory allocated for tiled physical memory
 * can be used to allocate raw memory.
 * This API is used to free such allocated memory.
*/
Void Utils_tilerAllocatorFreeRaw(Ptr addr,UInt32 size);

/**
 * \brief Used to get free memory size in tiler raw memory
 *
 * return 0 is tiler is enabled
 * else return free size in tiler raw buffer heap in bytes
*/
UInt32 Utils_tilerAllocatorGetFreeSizeRaw();

/**
 * \brief Used to get free memory size in a tiler container
 *
 * return 0 if tiler is disabled
 * else return free size in tiler container in bytes
*/
UInt32 Utils_tilerAllocatorGetFreeSize(UInt32 cntMode);

#endif                                                     /* __UTILS_TILER_ALLOCATOR_H_
                                                            */
