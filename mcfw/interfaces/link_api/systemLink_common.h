/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2012 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
    \ingroup LINK_API
    \defgroup PROCESSOR_LINK_API Processor Link API
*/

/**
    \ingroup PROCESSOR_LINK_API
    \defgroup SYSTEM_COMMON_LINK_API Processor Link API: Common Interfaces

    @{
*/

/**
    \file systemLink_common.h
    \brief System Common Link Link API
*/


#ifndef _SYSTEM_LINK_COMMON_H_
#define _SYSTEM_LINK_COMMON_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Include's  */
#include <mcfw/interfaces/link_api/system.h>

/**
    \ingroup LINK_API_CMD
    \addtogroup SYSTEM_COMMON_CMD Processor Link API: Common Control Commands

    @{
*/

/**
    \brief System CMD: Start CPU load computation

    \param NONE
*/
#define SYSTEM_COMMON_CMD_CPU_LOAD_CALC_START       (0x9000)

/**
    \brief System CMD: Stop CPU load computation

    \param NONE
*/
#define SYSTEM_COMMON_CMD_CPU_LOAD_CALC_STOP       (0x9001)

/**
    \brief System CMD: Reset CPU load computation

    \param NONE
*/
#define SYSTEM_COMMON_CMD_CPU_LOAD_CALC_RESET      (0x9002)

/**
    \brief System CMD: Print CPU load information

    \param NONE
*/
#define SYSTEM_COMMON_CMD_CPU_LOAD_CALC_PRINT      (0x9003)

/**
    \brief System CMD: Print CPU load, Task Laod and Heap status information

    \param NONE
*/
#define SYSTEM_COMMON_CMD_PRINT_STATUS             (0x9004)

/**
    \brief System CMD: Allocate Tiler memory.

    \param SystemCommon_TilerAlloc * [IN][OUT] Size parameter of requested memory
*/
#define SYSTEM_COMMON_CMD_TILER_ALLOC               (0x9005)

/**
    \brief System CMD: Release/Free Tiler memory.

    \param SystemCommon_TilerFree * [IN] Tiler memory address
*/
#define SYSTEM_COMMON_CMD_TILER_FREE                (0x9006)

/**
    \brief System CMD: Free Tiler memory of all the container modes.

    \param [IN] NONE
*/
#define SYSTEM_COMMON_CMD_TILER_FREE_ALL            (0x9007)

/**
    \brief System CMD: Update channel to IVAHD mapping.

    \param SystemVideo_Ivahd2ChMap_Tbl[] * [IN] IVAHD to Channel Map table
*/
#define SYSTEM_COMMON_CMD_SET_CH2IVAHD_MAP_TBL      (0x9008)

/**
    \brief System CMD: Disable Tiler memory allocation. Tiler heap memory can
    be allocated for other system requirement.

    \param NONE
*/
#define SYSTEM_COMMON_CMD_TILER_DISABLE_ALLOCATOR   (0x9009)

/**
    \brief System CMD: Enable Tiler memory allocation. Tiler heap memory can not
    be allocated for other system requirement.

    \param NONE
*/
#define SYSTEM_COMMON_CMD_TILER_ENABLE_ALLOCATOR    (0x900A)

/**
    \brief System CMD: To know if Tiler memory allocation is disabled or enabled.

    \param NONE
*/
#define SYSTEM_COMMON_CMD_TILER_IS_ALLOCATOR_DISABLED  (0x900B)

/**
    \brief System CMD: Allocate Tiler heap memory if Tiler meory allocation is
    disabled.

    \param SystemCommon_TilerAllocRaw * [IN][OUT] Size parameter
*/
#define SYSTEM_COMMON_CMD_TILER_ALLOC_RAW              (0x900C)

/**
    \brief System CMD: Release/Free Tiler heap memory allocated as Raw memory.

    \param SystemCommon_TilerFreeRaw * [IN] Tiler heap memory address
*/
#define SYSTEM_COMMON_CMD_TILER_FREE_RAW              (0x900D)

/**
    \brief System CMD: Get available/free Tiler memory in each container mode.

    \param SystemCommon_TilerGetFreeSize  * [OUT] Free memory in each container mode
*/
#define SYSTEM_COMMON_CMD_TILER_GET_FREE_SIZE         (0x900E)

/**
    \brief System CMD: System command to check core status.

    \param [IN]  None   *
*/
#define SYSTEM_COMMON_CMD_CORE_STATUS                 (0x900F)

/**
    \brief System CMD: Get current channel to IVAHD mapping.

    \param SystemVideo_Ivahd2ChMap_Tbl[] * [OUT] IVAHD to Channel Map table
*/
#define SYSTEM_COMMON_CMD_GET_CH2IVAHD_MAP_TBL        (0x9010)

/* @} */

/**
    \brief System Common: Print CPU load, Task Load and Heap Staus..

    Defines System Common parameters that are passed when system statistics
    print is requested..
*/

typedef struct {

    UInt32 printCpuLoad;
    /**< [IN] Print CPU load of each core */

    UInt32 printTskLoad;
    /**< [IN] Print CPU loading of each Task*/

    UInt32 printHeapStatus;
    /**< [IN] Print Current Heap status of core*/

} SystemCommon_PrintStatus;

/**
    \brief System Tiler: Allocate Tiler memory.

    Defines System Tiler parameters that are passed when Tiler memory allocation
    is requested. The address of allocated Tiler memory is updated accordingly.
*/

typedef struct {

    UInt32 cntMode;
    /**< [IN] Tiler container mode 8/16/32/PG */

    UInt32 width;
    /**< [IN] Frame width */

    UInt32 height;
    /**< [IN] Frame height */

    UInt32 tileAddr;
    /**< [OUT] Tiled buf addr */

} SystemCommon_TilerAlloc;

/**
\brief System Tiler release Tiler memory.

    Defines System Tiler parameters that is passed when Tiler memory has to be
    released/free'd.
*/

typedef struct {

    UInt32 tileAddr;
    /**< [IN] Tiled buf addr to be freed */

} SystemCommon_TilerFree;

/**
    \brief System Tiler allocation disabled/enabled.

    Defines System Tiler parameters that keeps the record of system tiler memory
    allocation. TRUE if allocation disable, FALSE if allocation is enabled.
    in all the container modes.
*/

typedef struct SystemCommon_TilerIsDisabled {

    UInt32 isAllocatorDisabled;
    /**< [OUT] Flag indicating if tiler allocator is disabled */

} SystemCommon_TilerIsDisabled;

/**
    \brief System Tiler heap memory allocate.

    Defines System Tiler parameters that are passed when request is placed
    to allocate memory from Tiler heap memory. Address of allocated memory is
    updated accordingly.
*/

typedef struct SystemCommon_TilerAllocRaw {

    UInt32 size;
    /**< [IN] Size in bytes to be alloced */

    UInt32 align;
    /**< [IN] Alignment bytes */

    UInt32 allocAddr;
    /**< [OUT] Allocated address */

} SystemCommon_TilerAllocRaw;

/**
    \brief System Tiler memory release.

    Defines System Tiler parameters that are required when Tiler heap memory
    is released/free'd
*/

typedef struct SystemCommon_TilerFreeRaw {

    UInt32 size;
    /**< [IN] Size in bytes to be alloced */

    UInt32 allocAddr;
    /**< [IN] Allocated address */

} SystemCommon_TilerFreeRaw;

/**
    \brief System Tiler available memory in all the conatiner moder.

    Defines System Tiler parameters that contains the free available memory
    in all the container modes.
*/
typedef struct SystemCommon_TilerGetFreeSize {

    UInt32 freeSize8b;
    /**<  [IN] Free Size in bytes for 8-bit container,  when tiler is Enabled */

    UInt32 freeSize16b;
    /**< [IN] Free Size in bytes for 16-bit container, when tiler is Enabled */

    UInt32 freeSize32b;
    /**< [IN] Free Size in bytes for 24-bit container, when tiler is Enabled */

    UInt32 freeSizeRaw;
    /**< [IN] Free Size in bytes for tiler heap mem, when tiler is Disabled */

} SystemCommon_TilerGetFreeSize;


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

/*@}*/


