/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
    \ingroup UTILS_API
    \defgroup UTILS_MEM_API Frame buffer memory allocator API

    APIs to allocate frame buffer memory from a predefined memory pool

    @{
*/

/**
  \file utils_mem.h
  \brief Frame buffer memory allocator API
*/

#ifndef _UTILS_MEM_H_
#define _UTILS_MEM_H_

#include <mcfw/src_bios6/utils/utils.h>
#include <mcfw/interfaces/link_api/vidbitstream.h>

/** \brief Utility define for Kilobyte, i.e 1024 bytes */
#ifndef KB
#define KB ((UInt32)1024)
#endif

/** \brief Utility define for Megabyte, i.e 1024*1024 bytes */
#ifndef MB
#define MB (KB*KB)
#endif

/**
  \brief One time system init of memory allocator

  Should be called by application before using allocate APIs

  \return 0 on sucess, else failure
*/
Int32 Utils_memInit();

/**
  \brief One time system init of memory allocator

  Should be called by application before using allocate APIs.
  This is same as Utils_memInit(), except that a larger heap size is used.
  Right now only chains app will used this large heap.
  When using large heap a different config.bld needs to be used.
  For chains app this is setup via Makefile's in HDS driver build system.

  Only ONE of Utils_memInit() or Utils_memInit_largeHeap() should be called during system init.

  \return 0 on sucess, else failure
*/
Int32 Utils_memInit_largeHeap();

/**
  \brief One time system de-init of memory allocator

  Should be called by application at system de-init

  \return 0 on sucess, else failure
*/
Int32 Utils_memDeInit();

/**
  \brief Allocate a frame

  Use FVID2_Format to allocate a frame.
  Fill FVID2_Frame fields like channelNum based on FVID2_Format

  \param  pFormat   [IN] Data format information
  \param  pFrame    [OUT] Initialzed FVID2_Frame structure
  \param  numFrames [IN] Number of frames to allocate

  \return 0 on sucess, else failure
*/
Int32 Utils_memFrameAlloc(FVID2_Format * pFormat,
                          FVID2_Frame * pFrame, UInt16 numFrames);

/**
  \brief Free's previously allocate FVID2_Frame's

  \param  pFormat   [IN] Data format information
  \param  pFrame    [IN] FVID2_Frame structure
  \param  numFrames [IN] Number of frames to free

  \return 0 on sucess, else failure
*/
Int32 Utils_memFrameFree(FVID2_Format * pFormat,
                         FVID2_Frame * pFrame, UInt16 numFrames);

/**
  \brief Allocate memory from Frame buffer memory pool

  \param size     [IN] size in bytes
  \param align    [IN] alignment in bytes

  \return NULL or error, else memory pointer
*/
Ptr Utils_memAlloc(UInt32 size, UInt32 align);

/**
  \brief Free previously allocate Frame buffer memory pointer

  \param addr     [IN] memory pointer to free
  \param size     [IN] size of memory pointed to by the memory pointer

  \return 0 on sucess, else failure
*/
Int32 Utils_memFree(Ptr addr, UInt32 size);

/**
    \brief Control if allocated buffer needs to be cleared to 0

    By default allocated buffer will not be cleared to 0

    \param enable   [IN] TRUE: clear allocated buffer, FALSE: do not clear allocated buffer

    \return 0 on sucess, else failure
*/
Int32 Utils_memClearOnAlloc(Bool enable);

/**
 *  \brief Returns the system heap free memory (in bytes)
 */
UInt32 Utils_memGetSystemHeapFreeSpace(void);

/**
 *  \brief Returns the SR 0 heap free memory (in bytes)
 */
UInt32 Utils_memGetSR0HeapFreeSpace(void);

/**
 *  \brief Returns the Frame buffer heap free memory (in bytes)
 */
UInt32 Utils_memGetBufferHeapFreeSpace(void);

/**
 *  \brief Returns the Bitstream buffer heap free memory (in bytes)
 */
UInt32 Utils_memGetBitBufferHeapFreeSpace(void);

/**
 *  \brief Returns the heap handle used for algorithm mem allocation
 */
void* Utils_getAlgMemoryHeapHandle (void);

/**
    \brief Init heap with user specified memory area

    This API is used internally by Utils_memInit() and Utils_memInit_largeHeap()
    so typically user's will NOT use this API directly
*/
Int32 Utils_memInit_internal(UInt32 frmBufSrId, UInt32 bitBufSrId);

#define UTILS_MEM_ENABLE_MEMLOG                                           (TRUE)
#define UTILS_MEM_ENABLE_MEMLOG_SRHEAP                                    (TRUE)

#ifdef UTILS_MEM_ENABLE_MEMLOG
#include <xdc/std.h>
#include <xdc/runtime/Memory.h>
#include <xdc/runtime/IHeap.h>
#include <ti/sysbios/heaps/HeapMem.h>
#    ifdef UTILS_MEM_ENABLE_MEMLOG_SRHEAP
#        include <ti/ipc/SharedRegion.h>
#    endif

#define  UTILS_MEM_MAXHEAPS                                                 (8)

/**
 * @name   utilsmem_memstats_getfreestats
 * @brief  Function to get free memory stats from all the heaps in the system
 * @param  freeSize                 : Pointer to freeSize array to be populated
 * @param  nMaxSize                 : Max size of free size array
 * @return Number of heaps in the system
 */
static Int utilsmem_memstats_getfreestats(UInt32 freeSize[], UInt32 nMaxSize)
{
#   ifdef UTILS_MEM_ENABLE_MEMLOG_SRHEAP
    SharedRegion_Entry srEntry;
#   endif
    Int i;
    Int heapIndex = 0;
    Memory_Stats memstats;
    Int numStaticHeaps = 0;
    Int numDynamicHeaps = 0;
    HeapMem_Handle nextHeap, curHeap;

    numStaticHeaps = HeapMem_Object_count();
    for (i = 0; i < numStaticHeaps; i++)
    {
        IHeap_Handle heapHandle = HeapMem_Handle_upCast(HeapMem_Object_get(NULL,
                                                                           i));

        Memory_getStats(heapHandle, &(memstats));
        UTILS_assert((nMaxSize > heapIndex));
        freeSize[heapIndex++] = memstats.totalFreeSize;
    }
    nextHeap = HeapMem_Object_first();
    do {
        curHeap = nextHeap;
        if (NULL != curHeap)
        {
            IHeap_Handle heapHandle = HeapMem_Handle_upCast(curHeap);

            numDynamicHeaps++;
            Memory_getStats(heapHandle, &(memstats));
            UTILS_assert((nMaxSize > heapIndex));
            freeSize[heapIndex++] = memstats.totalFreeSize;
            nextHeap = HeapMem_Object_next(curHeap);
        }
    } while (nextHeap != NULL);
#   ifdef UTILS_MEM_ENABLE_MEMLOG_SRHEAP
    for (i = 0; i < SharedRegion_getNumRegions(); i++)
    {
        SharedRegion_getEntry(i, &srEntry);
        if (TRUE == srEntry.isValid)
        {
            if (TRUE == srEntry.createHeap)
            {
                IHeap_Handle srHeap = SharedRegion_getHeap(i);

                UTILS_assert((nMaxSize > heapIndex));
                Memory_getStats(srHeap, &(memstats));
                freeSize[heapIndex++] = memstats.totalFreeSize;
            }
        }
    }
#   endif
    return heapIndex;
}

/**
 * @name   utilsmem_memstats_getusedsize
 * @brief  Function to get used memory stats from all the heaps in the system
 * @param  initialfreeSize          : Pointer to initial free size array
 * @param  usedSize                 : Pointer to used size array to be populated
 * @param  numHeaps                 : Number of heaps in the system
 * @return Number of heaps in the system
 */
static Void
utilsmem_memstats_getusedsize(UInt32 initialfreeSize[],
                              UInt32 usedSize[], UInt32 numHeaps)
{
    Int i;
    UInt32 currentfreeSize[UTILS_MEM_MAXHEAPS];
    UInt32 numHeapsEnd;

    UTILS_assert((numHeaps <= UTILS_MEM_MAXHEAPS));
    numHeapsEnd =
        utilsmem_memstats_getfreestats(&(currentfreeSize[0]),
                                       UTILS_MEM_MAXHEAPS);
    UTILS_assert((numHeapsEnd == numHeaps));

    for (i = 0; i < numHeaps; i++)
    {
        UTILS_assert((initialfreeSize[i] >= currentfreeSize[i]));
        usedSize[i] = initialfreeSize[i] - currentfreeSize[i];
    }
}

/**
 * @name   utilsmem_memstats_getfreedsize
 * @brief  Function to get freed memory stats from all the heaps in the system
 * @param  initialfreeSize          : Pointer to initial free size array
 * @param  freedSize                 : Pointer to freed size array to be populated
 * @param  numHeaps                 : Number of heaps in the system
 * @return Number of heaps in the system
 */
static Void
utilsmem_memstats_getfreedsize(UInt32 initialfreeSize[],
                               UInt32 freedSize[], UInt32 numHeaps)
{
    Int i;
    UInt32 currentfreeSize[UTILS_MEM_MAXHEAPS];
    UInt32 numHeapsEnd;

    UTILS_assert((numHeaps <= UTILS_MEM_MAXHEAPS));
    numHeapsEnd =
        utilsmem_memstats_getfreestats(&(currentfreeSize[0]),
                                       UTILS_MEM_MAXHEAPS);
    UTILS_assert((numHeapsEnd == numHeaps));

    for (i = 0; i < numHeaps; i++)
    {
        UTILS_assert((currentfreeSize[i] >= initialfreeSize[i]));
        freedSize[i] = currentfreeSize[i] - initialfreeSize[i];
    }
}

/**
 * @name   utilsmem_memstats_checkleak
 * @brief  Function to check if memory leak occured
 * @param  allocSize                 : Pointer to alloc size array
 * @param  freedSize                 : Pointer to freed size array
 * @param  numHeaps                  : Number of heaps in the system
 * @param  id                        : ID used to identify stage when printing leaks.
 * @return Number of heaps in the system
 */
static Void
utilsmem_memstats_checkleak(UInt32 allocSize[],
                            UInt32 freedSize[], UInt32 numHeaps, UInt32 id)
{
    Int i;

    for (i = 0; i < numHeaps; i++)
    {
        if (allocSize[i] != freedSize[i])
        {
            Vps_printf("MemoryLeak:STAGE:%d\tHEAPNUM:%d\t"
                       "ALLOC=%d\tFREED=%d\n", id, i, allocSize[i],
                       freedSize[i]);
        }
    }
}
#endif

#ifdef UTILS_MEM_ENABLE_MEMLOG
#define UTILS_MEMLOG_USED_START()  do {                                                     \
                                        UInt32 nNumHeaps =      0;                          \
                                        UInt32 freeMemStart[UTILS_MEM_MAXHEAPS];            \
                                                                                            \
                                        nNumHeaps =                                         \
                                        utilsmem_memstats_getfreestats(freeMemStart,        \
                                                                       UTILS_MEM_MAXHEAPS)
#else
#define UTILS_MEMLOG_USED_START()
#endif

#ifdef UTILS_MEM_ENABLE_MEMLOG
#define UTILS_MEMLOG_USED_END(pUsedSize)                                                   \
                                        utilsmem_memstats_getusedsize(freeMemStart,        \
                                                                      pUsedSize,           \
                                                                      nNumHeaps);          \
                                      } while (0)
#else
#define UTILS_MEMLOG_USED_END(pUsedSize)
#endif

#ifdef UTILS_MEM_ENABLE_MEMLOG
#define UTILS_MEMLOG_FREE_START()  do {                                                   \
                                        UInt32 nNumHeaps =      0;                        \
                                        UInt32 freeMemStart[UTILS_MEM_MAXHEAPS];          \
                                        UInt32 freedSize[UTILS_MEM_MAXHEAPS];             \
                                                                                          \
                                        nNumHeaps =                                       \
                                        utilsmem_memstats_getfreestats(freeMemStart,      \
                                                                       UTILS_MEM_MAXHEAPS)
#else
#define UTILS_MEMLOG_FREE_START()
#endif

#ifdef UTILS_MEM_ENABLE_MEMLOG
#define UTILS_MEMLOG_FREE_END(pAllocSize,id)                                              \
                                        utilsmem_memstats_getfreedsize(freeMemStart,      \
                                                                          freedSize,      \
                                                                          nNumHeaps);     \
                                        utilsmem_memstats_checkleak(pAllocSize,           \
                                                                    freedSize,            \
                                                                    nNumHeaps,            \
                                                                    id);                  \
                                   } while (0)
#else
#define UTILS_MEMLOG_FREE_END(pAllocSize,id)
#endif

#ifdef UTILS_MEM_ENABLE_MEMLOG
#define UTILS_MEMLOG_PRINT(str,pUsedSizeArray,sizeOfArray)                                \
                                  do {                                                    \
                                      Int i;                                              \
                                                                                          \
                                      Vps_printf ("\n");                                  \
                                      for (i = 0; i < sizeOfArray; i++) {                 \
                                          if (pUsedSizeArray[i] != 0) {                   \
                                              Vps_printf ("%s:HEAPID:%d\tUSED:%d\n",      \
                                                          str,i, pUsedSizeArray[i]);      \
                                          }                                               \
                                      }                                                   \
                                  } while (0)
#else
#define UTILS_MEMLOG_PRINT(str,pUsedSizeArray,sizeOfArray)
#endif

/**
  \brief Allocate a Bit Buffer
  \return 0 on sucess, else failure
*/
Int32 Utils_memBitBufAlloc(Bitstream_Buf * pBuf, UInt32 size, UInt16 numFrames);

/**
  \brief Free a Bit Buffer
  \return 0 on sucess, else failure
*/
Int32 Utils_memBitBufFree(Bitstream_Buf * pBuf, UInt16 numFrames);

/**
  Get buffer size based on data format

  pFormat - data format information
  *size - buffer size
  *cOffset - C plane offset for YUV420SP data
*/
Int32 Utils_memFrameGetSize(FVID2_Format * pFormat,
                            UInt32 * size, UInt32 * cOffset);


/* @} */
#endif
