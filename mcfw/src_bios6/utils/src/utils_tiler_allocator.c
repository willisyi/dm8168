/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include <mcfw/src_bios6/utils/utils_tiler.h>
#include <mcfw/src_bios6/utils/utils_tiler_allocator.h>
#include "utils_tiler_priv.h"
#include <mcfw/interfaces/link_api/system_debug.h>
#include <ti/sysbios/heaps/HeapMem.h>

#ifdef  KB
#undef  KB
#endif

#ifdef  MB
#undef  MB
#endif

#define KB                                                             (1024u)
#define MB                                                             (KB*KB)

extern  const UInt32 UTILS_TILER_CNT_8BIT_SIZE;
extern  const UInt32 UTILS_TILER_CNT_16BIT_SIZE;
extern  const UInt32 UTILS_TILER_CNT_32BIT_SIZE;

#define UTILS_TILER_PHYS_MEM_SIZE               ((UTILS_TILER_CNT_8BIT_SIZE) +\
                                                 (UTILS_TILER_CNT_16BIT_SIZE)+\
                                                 (UTILS_TILER_CNT_32BIT_SIZE))

#define UTILS_TILER_MAX_HEIGHT_8BIT   (UTILS_TILER_CNT_8BIT_SIZE/VPSUTILS_TILER_CNT_8BIT_PITCH)
#define UTILS_TILER_MAX_HEIGHT_16BIT  (UTILS_TILER_CNT_16BIT_SIZE/VPSUTILS_TILER_CNT_16BIT_PITCH)
#define UTILS_TILER_MAX_HEIGHT_32BIT  (UTILS_TILER_CNT_32BIT_SIZE/VPSUTILS_TILER_CNT_32BIT_PITCH)

#define UTILS_TILER_DMM_PAT_VIEW_MAP_BASE_REGADDR    (0x4E000460u)
#define UTILS_TILER_DMM_PAT_VIEW_MAP__0_REGADDR      (0x4E000440u)
#define UTILS_TILER_DMM_PAT_VIEW_MAP__1_REGADDR      (0x4E000444u)
#define UTILS_TILER_DMM_PAT_VIEW_MAP__2_REGADDR      (0x4E000448u)
#define UTILS_TILER_DMM_PAT_VIEW_MAP__3_REGADDR      (0x4E00044Cu)

#define UTILS_TILER_DMM_PAT_VIEW_BASEPHYADDR         (0x80000000u)

#define UTILS_TILER_DMM_PAT_VIEW_MASK              (0x80000000u)
#define UTILS_TILER_DMM_PAT_VIEW_MAP_OFFSET_MASK   (0x78000000u)
#define UTILS_TILER_DMM_PAT_VIEW_MAP_OFFSET_SHIFT  (27u)
#define UTILS_TILER_DMM_PAT_TWO_NIBBLE_SHIFT       (8u)

/**************************************************************************\
* Register Overlay Structure
\**************************************************************************/

typedef struct {

    UInt32 curX[UTILS_TILER_CNT_MAX];
    UInt32 curStartY[UTILS_TILER_CNT_MAX];
    UInt32 curEndY[UTILS_TILER_CNT_MAX];
    UInt32 startOffset[UTILS_TILER_CNT_MAX];

    UInt32 enableLog;
    UInt32 initDone;
    UInt32 allocatorDisabled;
    HeapMem_Struct rawMemHeapObj;
    HeapMem_Handle rawMemHeap;

} Utils_TilerObj;

extern UInt8 *gUtils_tilerPhysMem;

Utils_TilerObj gUtils_tilerObj = {.initDone = FALSE ,
                                  #ifdef SYSTEM_USE_TILER
                                    .allocatorDisabled = FALSE
                                  #else
                                    .allocatorDisabled = TRUE
                                  #endif
                                  };

static Int32 Utils_tilerAllocatorGetMaxHeight(UInt32 cntMode,
                                              UInt32 * maxHeight);

static Void  utils_tiler_create_non_tiled_heap(Utils_TilerObj *pObj,
                                               Ptr    heapAddress,
                                               UInt32 heapSize)
{
    HeapMem_Params params;

    UTILS_assert(pObj->rawMemHeap == NULL);
    HeapMem_Params_init(&params);
    params.buf  = heapAddress;
    params.size = heapSize;
    HeapMem_construct(&pObj->rawMemHeapObj,
                      &params);
    pObj->rawMemHeap = HeapMem_handle(&pObj->rawMemHeapObj);
    UTILS_assert(pObj->rawMemHeap != NULL);
}

static Void  utils_tiler_delete_non_tiled_heap(Utils_TilerObj *pObj)
{
    UTILS_assert(pObj->rawMemHeap ==
                 HeapMem_handle(&pObj->rawMemHeapObj));
    HeapMem_destruct(&pObj->rawMemHeapObj);
    pObj->rawMemHeap = NULL;
}

#ifdef SYSTEM_USE_TILER
int g_DbgPatViewMapVal = 0;
static Int32 utils_tiler_pat_init(Void)
{
    UInt32 v0, v1, v2, v3;
    UInt32 patViewMapVal;
    UInt32 tile8PhyAddr;
    UInt32 tile16PhyAddr;
    UInt32 tile32PhyAddr;
    UInt32 tilePGPhyAddr;
    Int32 status = 0;
    volatile UInt32 *patViewMapBaseReg =
        (volatile UInt32 *) UTILS_TILER_DMM_PAT_VIEW_MAP_BASE_REGADDR;
    volatile UInt32 *patViewMap0Reg =
        (volatile UInt32 *) UTILS_TILER_DMM_PAT_VIEW_MAP__0_REGADDR;
    volatile UInt32 *patViewMap1Reg =
        (volatile UInt32 *) UTILS_TILER_DMM_PAT_VIEW_MAP__1_REGADDR;
    volatile UInt32 *patViewMap2Reg =
        (volatile UInt32 *) UTILS_TILER_DMM_PAT_VIEW_MAP__2_REGADDR;
    volatile UInt32 *patViewMap3Reg =
        (volatile UInt32 *) UTILS_TILER_DMM_PAT_VIEW_MAP__3_REGADDR;

    *patViewMapBaseReg = UTILS_TILER_DMM_PAT_VIEW_BASEPHYADDR;

    if ((NULL != gUtils_tilerPhysMem)
        &&
        (UTILS_TILER_PHYS_MEM_SIZE != 0))
    {
        tile8PhyAddr = (UInt32) (&gUtils_tilerPhysMem[0]);
        /* Assert that physical memory is 12*MB aligned */
        UTILS_assert((tile8PhyAddr & ((128 * MB) - 1)) == 0);
        UTILS_assert(tile8PhyAddr >= UTILS_TILER_DMM_PAT_VIEW_BASEPHYADDR);
        /* Tiler containers physical size should be MB aligned */
        UTILS_assert((UTILS_TILER_CNT_8BIT_SIZE % MB) == 0);
         UTILS_assert((UTILS_TILER_CNT_16BIT_SIZE % MB) == 0);
        UTILS_assert((UTILS_TILER_CNT_32BIT_SIZE % MB) == 0);
        v0 = (tile8PhyAddr - UTILS_TILER_DMM_PAT_VIEW_BASEPHYADDR);
        v0 /= UTILS_TILER_CONTAINER_MAXSIZE;
        v0 &= 0xFF;

        if (UTILS_TILER_PHYS_MEM_SIZE >  (1 * UTILS_TILER_CONTAINER_MAXSIZE))
        {
            tile16PhyAddr = (UInt32)
            (&gUtils_tilerPhysMem[UTILS_TILER_CONTAINER_MAXSIZE]);
        }
        else
        {
            tile16PhyAddr = tile8PhyAddr;
        }


        UTILS_assert(tile16PhyAddr >= UTILS_TILER_DMM_PAT_VIEW_BASEPHYADDR);
        v1 = (tile16PhyAddr - UTILS_TILER_DMM_PAT_VIEW_BASEPHYADDR);
        v1 /= UTILS_TILER_CONTAINER_MAXSIZE;
        v1 &= 0xFF;

        if (UTILS_TILER_PHYS_MEM_SIZE >  (2 * UTILS_TILER_CONTAINER_MAXSIZE))
        {
            tile32PhyAddr = (UInt32)
                (&gUtils_tilerPhysMem[2 * UTILS_TILER_CONTAINER_MAXSIZE]);
        }
        else
        {
            tile32PhyAddr = tile16PhyAddr;
        }


        UTILS_assert(tile32PhyAddr >= UTILS_TILER_DMM_PAT_VIEW_BASEPHYADDR);
        v2 = (tile32PhyAddr - UTILS_TILER_DMM_PAT_VIEW_BASEPHYADDR);
        v2 /= UTILS_TILER_CONTAINER_MAXSIZE;
        v2 &= 0xFF;

        if (UTILS_TILER_PHYS_MEM_SIZE >  (3 * UTILS_TILER_CONTAINER_MAXSIZE))
        {
            tilePGPhyAddr = (UInt32)
                (&gUtils_tilerPhysMem[3 * UTILS_TILER_CONTAINER_MAXSIZE]);
        }
        else
        {
            tilePGPhyAddr = tile32PhyAddr;
        }

        UTILS_assert(tilePGPhyAddr >= UTILS_TILER_DMM_PAT_VIEW_BASEPHYADDR);
        v3 = (tilePGPhyAddr - UTILS_TILER_DMM_PAT_VIEW_BASEPHYADDR);
        v3 /= UTILS_TILER_CONTAINER_MAXSIZE;
        v3 &= 0xFF;

        patViewMapVal = (v0 | (v1 << 8) | (v2 << 16) | (v3 << 24));

        /* Store value for debug purpose */
        g_DbgPatViewMapVal = patViewMapVal;

        *patViewMap0Reg = patViewMapVal;
        *patViewMap1Reg = patViewMapVal;
        *patViewMap2Reg = patViewMapVal;
        *patViewMap3Reg = patViewMapVal;
        status = 0;
    }
    else
    {
        status = -1;
    }
    return status;
}
#endif

Void utils_tiler_init_container_offset(Void)
{
    gUtils_tilerObj.startOffset[UTILS_TILER_CNT_8BIT] = 0;
    if (UTILS_TILER_PHYS_MEM_SIZE > 1 * UTILS_TILER_CONTAINER_MAXSIZE)
    {
        gUtils_tilerObj.startOffset[UTILS_TILER_CNT_16BIT] = 0;
    }
    else
    {
        gUtils_tilerObj.startOffset[UTILS_TILER_CNT_16BIT] =
            gUtils_tilerObj.startOffset[UTILS_TILER_CNT_8BIT] +
            UTILS_TILER_CNT_8BIT_SIZE;
    }
    if (UTILS_TILER_PHYS_MEM_SIZE > 2 * UTILS_TILER_CONTAINER_MAXSIZE)
    {
        gUtils_tilerObj.startOffset[UTILS_TILER_CNT_32BIT] = 0;
    }
    else
    {
        gUtils_tilerObj.startOffset[UTILS_TILER_CNT_32BIT] =
            gUtils_tilerObj.startOffset[UTILS_TILER_CNT_16BIT] +
            UTILS_TILER_CNT_16BIT_SIZE;
    }
}

Int32 Utils_tilerAllocatorInit()
{

    if (gUtils_tilerObj.initDone == FALSE)
    {
        Int32 status = -1;

        #ifdef SYSTEM_USE_TILER
            status = utils_tiler_pat_init();
        #endif
        if (0 == status)
        {
            utils_tiler_init_container_offset();

            Utils_tilerAllocatorFreeAll();

            Utils_tilerAllocatorDebugLogEnable(FALSE);

            gUtils_tilerObj.allocatorDisabled = FALSE;
        }
        else
        {
            gUtils_tilerObj.allocatorDisabled = TRUE;
            if (UTILS_TILER_PHYS_MEM_SIZE != 0)
            {
                utils_tiler_create_non_tiled_heap(&gUtils_tilerObj,
                                                       gUtils_tilerPhysMem,
                                                       UTILS_TILER_PHYS_MEM_SIZE);
            }
        }
        gUtils_tilerObj.initDone = TRUE;
    }
    return 0;
}

Int32 Utils_tilerAllocatorDeInit()
{

    gUtils_tilerObj.initDone = FALSE;
    return 0;
}

Int32 Utils_tilerAllocatorDebugLogEnable(UInt32 enable)
{
    gUtils_tilerObj.enableLog = enable;

    return 0;
}



UInt32 Utils_tilerAllocatorGetAddr(UInt32 cntMode, UInt32 startX, UInt32 startY)
{
    UInt32 tilerAddr, maxPitch, maxHeight, offset;

    UTILS_assert(TRUE == gUtils_tilerObj.initDone);

    Utils_tilerGetMaxPitch(cntMode, &maxPitch);
    Utils_tilerAllocatorGetMaxHeight(cntMode, &maxHeight);
    offset = gUtils_tilerObj.startOffset[cntMode];

    tilerAddr = offset + (startY * maxPitch + startX);
    tilerAddr = UTILS_TILER_PUT_CNT_MODE(tilerAddr, cntMode);

    if (gUtils_tilerObj.enableLog)
    {
        Vps_printf(" [TILER] Tiler Addr = 0x%08x, mode = %d, x,y = %d,%d\n",
                   tilerAddr, cntMode, startX, startY);
    }

    return tilerAddr;
}


#ifdef SYSTEM_USE_TILER
UInt32 Utils_tilerAllocatorAlloc(UInt32 cntMode, UInt32 width, UInt32 height)
{
    UInt32 tilerAddr, curX, curStartY, curEndY, maxWidth, maxHeight;

    if (gUtils_tilerObj.initDone == FALSE)
    {
        Utils_tilerAllocatorInit();
    }
    UTILS_assert(gUtils_tilerObj.initDone == TRUE);

    if (FALSE == gUtils_tilerObj.allocatorDisabled)
    {
        width = VpsUtils_align(width, VPS_BUFFER_ALIGNMENT * 2);
        height = VpsUtils_align(height, 2);

        curX = gUtils_tilerObj.curX[cntMode];
        curStartY = gUtils_tilerObj.curStartY[cntMode];
        curEndY = gUtils_tilerObj.curEndY[cntMode];

        Utils_tilerGetMaxPitch(cntMode, &maxWidth);
        Utils_tilerAllocatorGetMaxHeight(cntMode, &maxHeight);

        if ((curX + width) > maxWidth)
        {
            curX = 0;
            curStartY = curEndY;
        }

        if ((curStartY + height) > maxHeight)
        {
            UTILS_warn("\n%d:TILER AllocFailed:CntMode:%d,Width=%d,Height=%d\n",
                       Utils_getCurTimeInMsec(),
                       cntMode,
                       width,
                       height);
            return UTILS_TILER_INVALID_ADDR;
        }

        if ((curStartY + height) > curEndY)
        {
            curEndY = curStartY + height;
        }

        tilerAddr = Utils_tilerAllocatorGetAddr(cntMode, curX, curStartY);

        curX += width;

        gUtils_tilerObj.curX[cntMode] = curX;
        gUtils_tilerObj.curStartY[cntMode] = curStartY;
        gUtils_tilerObj.curEndY[cntMode] = curEndY;

        UTILS_assert(tilerAddr != UTILS_TILER_INVALID_ADDR);
    }
    else
    {
        tilerAddr = UTILS_TILER_INVALID_ADDR;
    }
    return tilerAddr;
}
#else
UInt32 Utils_tilerAllocatorAlloc(UInt32 cntMode, UInt32 width, UInt32 height)
{
    (void)cntMode;
    (void)width;
    (void)height;

    return UTILS_TILER_INVALID_ADDR;
}

#endif


static Int32 Utils_tilerAllocatorGetMaxHeight(UInt32 cntMode,
                                              UInt32 * maxHeight)
{
    *maxHeight = 0;

    switch (cntMode)
    {
        case UTILS_TILER_CNT_8BIT:
            *maxHeight = UTILS_TILER_MAX_HEIGHT_8BIT;
            break;
        case UTILS_TILER_CNT_16BIT:
            *maxHeight = UTILS_TILER_MAX_HEIGHT_16BIT;
            break;
        case UTILS_TILER_CNT_32BIT:
            *maxHeight = UTILS_TILER_MAX_HEIGHT_32BIT;
            break;
        default:
            return -1;
    }

    return 0;
}

Int32 Utils_tilerAllocatorFreeAll()
{
    UInt32 cntMode;

    for (cntMode = 0; cntMode < UTILS_TILER_CNT_MAX; cntMode++)
    {
        gUtils_tilerObj.curX[cntMode] = VPS_BUFFER_ALIGNMENT * 2;
        gUtils_tilerObj.curStartY[cntMode] = 0;
        gUtils_tilerObj.curEndY[cntMode] = 0;
    }
    return 0;
}

Int32 Utils_tilerAllocatorFree(UInt32 tileAddr)
{
    UTILS_assert(gUtils_tilerObj.initDone == TRUE);
    /* Not implemented */
    return -1;
}

Int32 Utils_tilerAllocatorDisable()
{
    Int32 status = 0;
    UInt32 cntMode;

    UTILS_assert(gUtils_tilerObj.initDone == TRUE);
    for (cntMode = 0; cntMode < UTILS_TILER_CNT_MAX; cntMode++)
    {
        /* Runtime disable of tiler allocation is possible
         * only when no tiler memory has been allocated.
         */
        if ((gUtils_tilerObj.curX[cntMode] != VPS_BUFFER_ALIGNMENT * 2)
            ||
            (gUtils_tilerObj.curStartY[cntMode] != 0)
            ||
            (gUtils_tilerObj.curEndY[cntMode]   != 0))
        {
            status = -1;
            Vps_printf("\n%d:TILER ALLOCATOR DISABLE FAILED as allocation has already occured\n",
                       Utils_getCurTimeInMsec());
        }
    }
    if (0 == status)
    {
        gUtils_tilerObj.allocatorDisabled = TRUE;
        utils_tiler_create_non_tiled_heap(&gUtils_tilerObj,
                                          gUtils_tilerPhysMem,
                                          UTILS_TILER_PHYS_MEM_SIZE);

    }
    return status;
}


Int32 Utils_tilerAllocatorEnable()
{
    UTILS_assert(gUtils_tilerObj.initDone == TRUE);
    if (TRUE == gUtils_tilerObj.allocatorDisabled)
    {
        gUtils_tilerObj.allocatorDisabled = FALSE;
        utils_tiler_delete_non_tiled_heap(&gUtils_tilerObj);
        Utils_tilerAllocatorFreeAll();

    }
    return 0;
}

UInt32 Utils_tilerAllocatorIsDisabled()
{
    return gUtils_tilerObj.allocatorDisabled;

}

Ptr Utils_tilerAllocatorAllocRaw(UInt32 size, UInt32 align)
{
    Ptr addr = NULL;
    Error_Block ebObj;
    Error_Block *eb = &ebObj;

    if (TRUE == gUtils_tilerObj.allocatorDisabled)
    {
        Error_init(eb);

        /* allocate memory */
        UTILS_assert(gUtils_tilerObj.rawMemHeap != NULL);
        addr = Memory_alloc(HeapMem_Handle_upCast(gUtils_tilerObj.rawMemHeap),
                            size,
                            align,
                            eb);
    }
    return addr;
}

Void Utils_tilerAllocatorFreeRaw(Ptr addr,UInt32 size)
{
    Error_Block ebObj;
    Error_Block *eb = &ebObj;

    UTILS_assert(TRUE == gUtils_tilerObj.allocatorDisabled);
    Error_init(eb);

    /* allocate memory */
    UTILS_assert(gUtils_tilerObj.rawMemHeap != NULL);
    /* Check address to be freed falls in tiler physical memoru
     * address range
     */
    UTILS_assert((addr >= gUtils_tilerPhysMem)
                 &&
                 ((((UInt8 *)addr) + size)
                  <=
                  (gUtils_tilerPhysMem + UTILS_TILER_PHYS_MEM_SIZE)));
    Memory_free(HeapMem_Handle_upCast(gUtils_tilerObj.rawMemHeap), addr, size);
}

UInt32 Utils_tilerAllocatorGetFreeSizeRaw()
{
    UInt32 size = 0;
    Memory_Stats stats;

    if (TRUE == gUtils_tilerObj.allocatorDisabled)
    {
        Memory_getStats(HeapMem_Handle_upCast(gUtils_tilerObj.rawMemHeap), &stats);

        size = stats.totalFreeSize;
    }

    return ((UInt32) (size));
}

UInt32 Utils_tilerAllocatorGetFreeSize(UInt32 cntMode)
{
    UInt32 maxHeight, availLines = 0, pitch;

    if (TRUE == gUtils_tilerObj.allocatorDisabled)
        return 0;

    Utils_tilerAllocatorGetMaxHeight(cntMode, &maxHeight);

    if(maxHeight >= gUtils_tilerObj.curEndY[cntMode])
    {
        availLines =  maxHeight - gUtils_tilerObj.curEndY[cntMode];
    }

    switch (cntMode)
    {
        case UTILS_TILER_CNT_8BIT:
            pitch = VPSUTILS_TILER_CNT_8BIT_PITCH;
            break;
        case UTILS_TILER_CNT_16BIT:
            pitch = VPSUTILS_TILER_CNT_16BIT_PITCH;
            break;
        case UTILS_TILER_CNT_32BIT:
            pitch = VPSUTILS_TILER_CNT_32BIT_PITCH;
            break;
        default:
            pitch = 0;
            break;
    }

    #ifdef SYSTEM_DEBUG_MEMALLOC
    // this is useful when we want to reduce tiler size based on free space or left over lines in tiler 2D space
    return availLines*pitch;
    #else
    // this is useful when we want to know free space left in tiler for further memory allocation
    return (availLines*pitch + ((pitch - gUtils_tilerObj.curX[cntMode]) *
    		(gUtils_tilerObj.curEndY[cntMode] - gUtils_tilerObj.curStartY[cntMode])));
    #endif
}


