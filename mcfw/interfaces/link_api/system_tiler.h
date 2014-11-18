/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2012 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
    \ingroup LINK_API

    \defgroup SYSTEM_TILER_API Tiler allocator API

    @{
*/

/**
    \file system_tiler.h
    \brief  Tiler allocator API
*/

#ifndef _SYSTEM_TILER_H_
#define _SYSTEM_TILER_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Include's  */
#include <mcfw/interfaces/link_api/system.h>
#include <mcfw/interfaces/link_api/systemLink_common.h>

/* Define's */

/* @{ */

/*
    \param First container mode of Tiler memory.
*/
#define SYSTEM_TILER_CNT_FIRST  (0)
/*
    \param Tiler memory 8-bit container mode
*/
#define SYSTEM_TILER_CNT_8BIT   (SYSTEM_TILER_CNT_FIRST)
/*
    \param Tiler memroy 16-bit container mode.
*/
#define SYSTEM_TILER_CNT_16BIT  (1)
/*
    \param tiler meory 32-bit container mode.
*/
#define SYSTEM_TILER_CNT_32BIT  (2)
/*
    \param Last container mode of Tiler memory.
*/
#define SYSTEM_TILER_CNT_LAST   (SYSTEM_TILER_CNT_32BIT)
/*
    \param NONE
*/
#define SYSTEM_TILER_CNT_MAX    (SYSTEM_TILER_CNT_LAST + 1)
/*
    \param Invalid address returned when Tiler memory allocation fails.
*/
#define SYSTEM_TILER_INVALID_ADDR              ((UInt32)~(0u))
/*
    \param Tiler memory allocator core Id.
*/
#define SYSTEM_TILER_ALLOCATOR_PROC_ID              (SYSTEM_PROC_M3VPSS)
/*@}*/


/* function's */

/**
    \brief Allocate system tiler memory in a given container mode of size 
           calculated from agruments WIDTH and HEIGHT.

    \return FVID2_SOK on success
*/
UInt32  SystemTiler_alloc(UInt32 cntMode, UInt32 width, UInt32 height);

/**
    \brief Free system tiler memory at given buffer address.

    \return FVID2_SOK on success
*/
Int32  SystemTiler_free(UInt32 tileAddr);

/**
    \brief Free system tiler memory of all the container modes.

    \return FVID2_SOK on success
*/
Int32  SystemTiler_freeAll(void);

/**
    \brief Returns TRUE if allocation of system tiler memory is disbled.

    \return TRUE if Tiler allocation disable, FALSE if Tiler allocation enabled.
*/
UInt32  SystemTiler_isAllocatorDisabled();

/**
    \brief Disable system tiler memory allocation

    \return FVID2_SOK on success
*/
Int32  SystemTiler_disableAllocator(void);

/**
    \brief Enable system tiler memory allocation.

    \return FVID2_SOK on success
*/
Int32  SystemTiler_enableAllocator(void);

/**
    \brief Allocate memory from Tiler heap memory region if Tiler allocation 
           is disabled. It should be called only if SystemTiler_isAllocatorDisabled()
           returns TRUE.

    \return FVID2_SOK on success
*/
Ptr SystemTiler_allocRaw(UInt32 size,UInt32 align);

/**
    \brief Free Tiler heap memory that was allocated as RAW memory.

    \return FVID2_SOK on success
*/
Int32 SystemTiler_freeRaw(Ptr addr, UInt32 size);

/**
    \brief Returns available memory in Tiler heap memory region.

    \return FVID2_SOK on success
*/
Int32 SystemTiler_getFreeSize(SystemCommon_TilerGetFreeSize *pPrm);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif/*_SYSTEM_TILER_H_*/

/*@}*/




