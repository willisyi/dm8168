/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/
#include <xdc/std.h>
#include <xdc/runtime/IHeap.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/ipc/SharedRegion.h>
#include <mcfw/src_bios6/utils/utils_mem.h>
#include <mcfw/interfaces/link_api/system_debug.h>
#include <mcfw/interfaces/link_api/system_common.h>
#include <mcfw/interfaces/link_api/system_tiler.h>


/* See utils_mem.h for function documentation */

//#define UTILS_MEM_DEBUG

/** \brief Two shared region heaps are allocated in each Ducati core
     First one for frame buffers and other for bit buffer allocation */

#define UTILS_MEM_NUM_SHARED_REGION_HEAP    (2)
#define UTILS_MEM_VID_FRAME_BUF_HEAP        (0)
#define UTILS_MEM_VID_BITS_BUF_HEAP         (1)



/* Memory pool handle */
IHeap_Handle gUtils_heapMemHandle[UTILS_MEM_NUM_SHARED_REGION_HEAP] = {NULL};
Bool         gUtils_memClearBuf[UTILS_MEM_NUM_SHARED_REGION_HEAP] = {FALSE};

/* Used for RPE - ENSURE that this is cached as this is used for alg memtab allocation */
void* Utils_getAlgMemoryHeapHandle (void)
{
    return ((void*) gUtils_heapMemHandle[UTILS_MEM_VID_BITS_BUF_HEAP]);
}

Int32 Utils_memInit()
{
    SharedRegion_Entry srEntry;
    Int                srStatus = SharedRegion_S_SUCCESS;
    UInt32 srId[UTILS_MEM_NUM_SHARED_REGION_HEAP], i;

    srId[UTILS_MEM_VID_FRAME_BUF_HEAP] = SYSTEM_IPC_SR_VIDEO_FRAME;
    srId[UTILS_MEM_VID_BITS_BUF_HEAP]  = SYSTEM_IPC_SR_CACHED;


    for (i=0; i<UTILS_MEM_NUM_SHARED_REGION_HEAP; i++)
    {
        SharedRegion_entryInit(&srEntry);
        SharedRegion_getEntry(srId[i], &srEntry);
        Vps_printf (" %d: MEM: Shared Region %d: Base = 0x%08x, Length = 0x%08x (%d MB) \n",
                    Utils_getCurTimeInMsec(), srId[i],srEntry.base,srEntry.len, srEntry.len/(1024*1024));
        if ((FALSE == srEntry.isValid)
            &&
            (0 != srEntry.len))
        {
            srEntry.isValid     = TRUE;
            do {
                srStatus = SharedRegion_setEntry(srId[i], &srEntry);

                if (srStatus != SharedRegion_S_SUCCESS) {
                    Vps_printf(" %d: MEM: ERROR: SharedRegion_setEntry (%d, 0x%08x) FAILED !!! "
                               " (status=%d) \n", Utils_getCurTimeInMsec(), srId[i], &srEntry, srStatus);
                    Task_sleep(10);
                }
            } while (srStatus != SharedRegion_S_SUCCESS);
        }
        if (srEntry.len)
        {
            gUtils_heapMemHandle[i] = SharedRegion_getHeap(srId[i]);
            UTILS_assert(gUtils_heapMemHandle[i] != NULL);
            gUtils_memClearBuf[i] = FALSE;
        }
    }
    return 0;
}

Int32 Utils_memDeInit()
{
    UInt32 i;

    for (i=0; i<UTILS_MEM_NUM_SHARED_REGION_HEAP; i++)
    {
        /* delete memory pool heap */
        gUtils_heapMemHandle[i] = NULL;
    }
    return 0;
}

/**
  Get buffer size based on data format

  pFormat - data format information
  *size - buffer size
  *cOffset - C plane offset for YUV420SP data
*/
Int32 Utils_memFrameGetSize(FVID2_Format * pFormat,
                            UInt32 * size, UInt32 * cOffset)
{
    UInt32 bufferHeight;
    Int32 status = 0;

    bufferHeight = pFormat->height;

    switch (pFormat->dataFormat)
    {
        case FVID2_DF_YUV422I_YUYV:
        case FVID2_DF_YUV422I_YVYU:
        case FVID2_DF_YUV422I_UYVY:
        case FVID2_DF_YUV422I_VYUY:
        case FVID2_DF_YUV444I:
        case FVID2_DF_RGB24_888:
        case FVID2_DF_RAW_VBI:

            /* for single plane data format's */
            *size = pFormat->pitch[0] * bufferHeight;
            break;

        case FVID2_DF_YUV422SP_UV:
        case FVID2_DF_YUV420SP_UV:

            /* for Y plane */
            *size = pFormat->pitch[0] * bufferHeight;

            /* cOffset is at end of Y plane */
            *cOffset = *size;

            if (pFormat->dataFormat == FVID2_DF_YUV420SP_UV)
            {
                /* C plane height is 1/2 of Y plane */
                bufferHeight = bufferHeight / 2;
            }

            /* for C plane */
            *size += pFormat->pitch[1] * bufferHeight;
            break;

        default:
            /* illegal data format */
            status = -1;
            break;
    }

    /* align size to minimum required frame buffer alignment */
    *size = VpsUtils_align(*size, VPS_BUFFER_ALIGNMENT);

    return status;
}

Int32 Utils_memFrameAlloc(FVID2_Format * pFormat,
                          FVID2_Frame * pFrame, UInt16 numFrames)
{
    UInt32 size, cOffset, frameId;
    Int32 status;
    UInt8 *pBaseAddr;

    /* init all FVID2_Frame to 0's */
    memset(pFrame, 0, sizeof(*pFrame)*numFrames);

    /* align height to multiple of 2 */
    pFormat->height = VpsUtils_align(pFormat->height, 2);

    /* get frame size for given pFormat */
    status = Utils_memFrameGetSize(pFormat, &size, &cOffset);

    if (status == 0)
    {
        /* allocate the memory for 'numFrames' */

        /* for all 'numFrames' memory is contigously allocated */
        pBaseAddr = Utils_memAlloc(size * numFrames, VPS_BUFFER_ALIGNMENT);

        if (pBaseAddr == NULL)
        {
            status = -1;                                   /* Error in
                                                            * allocation,
                                                            * exit with error
                                                            */
        }
    }

    if (status == 0)
    {
        /* init memory pointer for 'numFrames' */
        for (frameId = 0; frameId < numFrames; frameId++)
        {

            /* copy channelNum to FVID2_Frame from FVID2_Format */
            pFrame->channelNum = pFormat->channelNum;
            pFrame->addr[0][0] = pBaseAddr;

            switch (pFormat->dataFormat)
            {
                case FVID2_DF_RAW_VBI:
                case FVID2_DF_YUV422I_UYVY:
                case FVID2_DF_YUV422I_VYUY:
                case FVID2_DF_YUV422I_YUYV:
                case FVID2_DF_YUV422I_YVYU:
                case FVID2_DF_YUV444I:
                case FVID2_DF_RGB24_888:
                    break;
                case FVID2_DF_YUV422SP_UV:
                case FVID2_DF_YUV420SP_UV:
                    /* assign pointer for C plane */
                    pFrame->addr[0][1] = (UInt8 *) pFrame->addr[0][0] + cOffset;
                    break;
                default:
                    /* illegal data format */
                    status = -1;
                    break;
            }
            /* go to next frame */
            pFrame++;

            /* increment base address */
            pBaseAddr += size;
        }
    }

    if (status != 0)
    {
        void System_memPrintHeapStatus();

        Vps_printf("Memory allocation failed due to insufficient free memory, requested - %d \n", size * numFrames);
        System_memPrintHeapStatus();

    }
    /* commented out, so please always check the status immediately after the
     * Utils_memFrameAlloc() call and handle accordingly */
    // UTILS_assert(status == 0);

    return status;
}

Int32 Utils_memFrameFree(FVID2_Format * pFormat,
                         FVID2_Frame * pFrame, UInt16 numFrames)
{
    UInt32 size, cOffset;
    Int32 status;

    /* get frame size for given 'pFormat' */
    status = Utils_memFrameGetSize(pFormat, &size, &cOffset);

    if (status == 0)
    {
        /* free the frame buffer memory */

        /* for all 'numFrames' memory is allocated contigously during alloc,
         * so first frame memory pointer points to the complete memory block
         * for all frames */
        Utils_memFree(pFrame->addr[0][0], size * numFrames);
    }

    return 0;
}

Ptr Utils_memAlloc(UInt32 size, UInt32 align)
{
    Ptr addr;
    Error_Block ebObj;
    Error_Block *eb = &ebObj;

    Error_init(eb);

    #ifdef UTILS_MEM_DEBUG
    Vps_printf(" UTILS: MEM: Alloc'ing from FRAME BUF. (required size = %d B, free space = %d B)\n",
        size,
        Utils_memGetBufferHeapFreeSpace()
        );
    #endif

    /* allocate memory */
    addr = Memory_alloc(gUtils_heapMemHandle[UTILS_MEM_VID_FRAME_BUF_HEAP], size, align, eb);

    if(addr==NULL)
    {
        /* if memory allocation in Frame buf heap failed, then try in tiler buf heap
            But tiler needs to be disabled for this to work
        */
        if (SystemTiler_isAllocatorDisabled())
        {
            #ifdef UTILS_MEM_DEBUG
            Vps_printf(" UTILS: MEM: Alloc'ing from TILER BUF. (required size = %d B, free space = %d B)\n",
                size,
                SystemTiler_getFreeSize()
                );
            #endif

            Error_init(eb);
            addr = SystemTiler_allocRaw(size, align);
        }
        else
        {
            #ifdef UTILS_MEM_DEBUG
            Vps_printf(" UTILS: MEM: TILER is ENABLED, will try BITS BUF (required size = %d B)\n", size);
            #endif
        }

        if(addr==NULL)
        {
            #ifdef UTILS_MEM_DEBUG
            Vps_printf(" UTILS: MEM: Alloc'ing from BITS BUF. (required size = %d B, free space = %d B)\n",
                size,
                Utils_memGetBitBufferHeapFreeSpace()
                );
            #endif

            Error_init(eb);
            addr = Memory_alloc(gUtils_heapMemHandle[UTILS_MEM_VID_BITS_BUF_HEAP], size, align, eb);
        }

    }

    #ifdef UTILS_MEM_DEBUG
    Vps_printf(" UTILS: MEM: FRAME ALLOC, addr = 0x%08x, size = %d bytes \n",
                addr, size
               );
    #endif

    if (!Error_check(eb)
        &&
        (addr != NULL)
        &&
        gUtils_memClearBuf[UTILS_MEM_VID_FRAME_BUF_HEAP])
    {
        memset(addr, 0x80, size);
    }

    return addr;
}

Int32 Utils_memFree(Ptr addr, UInt32 size)
{
    SharedRegion_Entry srEntry;
    Int32 status;

    SharedRegion_getEntry(SYSTEM_IPC_SR_VIDEO_FRAME, &srEntry);

    if((UInt32)addr >= (UInt32)srEntry.base && (UInt32)addr < ((UInt32)srEntry.base + srEntry.len) )
    {
        /* if address falls in frame buffer  heap then free from that heap */
        #ifdef UTILS_MEM_DEBUG
        Vps_printf(" UTILS: MEM: FRAME FREE, addr = 0x%08x, size = %d bytes, heap = [FRAME BUF]\n", addr, size);
        #endif

        /* free previously allocated memory */
        Memory_free(gUtils_heapMemHandle[UTILS_MEM_VID_FRAME_BUF_HEAP], addr, size);
    }
    else
    {

        SharedRegion_getEntry(SYSTEM_IPC_SR_CACHED, &srEntry);

        if((UInt32)addr >= (UInt32)srEntry.base && (UInt32)addr < ((UInt32)srEntry.base + srEntry.len) )
        {
            /* if address falls in bitstream buffer  heap then free from that heap */
            #ifdef UTILS_MEM_DEBUG
            Vps_printf(" UTILS: MEM: FRAME FREE, addr = 0x%08x, size = %d bytes, heap = [BITS BUF]\n", addr, size);
            #endif

            /* free previously allocated memory */
            Memory_free(gUtils_heapMemHandle[UTILS_MEM_VID_BITS_BUF_HEAP], addr, size);

        }
        else
        {
            #ifdef UTILS_MEM_DEBUG
            Vps_printf(" UTILS: MEM: FRAME FREE, addr = 0x%08x, size = %d bytes, heap = [TILER BUF]\n", addr, size);
            #endif

            /* else address falls in tiler memory area */
            status = SystemTiler_freeRaw(addr,size);
            UTILS_assert(status == 0);
        }
    }

    return 0;
}

Int32 Utils_memClearOnAlloc(Bool enable)
{
    UInt32 i;

    for (i=0; i<UTILS_MEM_NUM_SHARED_REGION_HEAP; i++)
    {
        gUtils_memClearBuf[i] = enable;
    }
    return 0;
}

UInt32 Utils_memGetSystemHeapFreeSpace(void)
{
    Memory_Stats stats;
    extern const IHeap_Handle Memory_defaultHeapInstance;

    Memory_getStats(Memory_defaultHeapInstance, &stats);

    return ((UInt32) (stats.totalFreeSize));
}

UInt32 Utils_memGetSR0HeapFreeSpace(void)
{
    UInt32 size;
    Memory_Stats stats;

    Memory_getStats(SharedRegion_getHeap(SYSTEM_IPC_SR_NON_CACHED_DEFAULT), &stats);

    size = stats.totalFreeSize;
    return ((UInt32) (size));
}

UInt32 Utils_memGetBufferHeapFreeSpace(void)
{
    UInt32 size;
    Memory_Stats stats;

    Memory_getStats(gUtils_heapMemHandle[UTILS_MEM_VID_FRAME_BUF_HEAP], &stats);

    size = stats.totalFreeSize;

    return ((UInt32) (size));
}

UInt32 Utils_memGetBitBufferHeapFreeSpace(void)
{
    Memory_Stats stats;

    Memory_getStats(gUtils_heapMemHandle[UTILS_MEM_VID_BITS_BUF_HEAP], &stats);

    return ((UInt32) (stats.totalFreeSize));
}

Int32 Utils_memBitBufAlloc(Bitstream_Buf * pBuf, UInt32 size, UInt16 numFrames)
{
    UInt32 bufId;
    Int32 status = 0;
    UInt8 *pBaseAddr;
    Error_Block ebObj;
    Error_Block *eb = &ebObj;

    Error_init(eb);

    /* init Bitstream_Buf to 0's */
    memset(pBuf, 0, sizeof(*pBuf));

    /* align size to minimum required frame buffer alignment */
    size = VpsUtils_align(size, IVACODEC_VDMA_BUFFER_ALIGNMENT);

    /* allocate the memory for 'numFrames' */
    /* for all 'numFrames' memory is contigously allocated */
    pBaseAddr = Memory_alloc(gUtils_heapMemHandle[UTILS_MEM_VID_BITS_BUF_HEAP],
                    (size * numFrames), IVACODEC_VDMA_BUFFER_ALIGNMENT, eb);

    #ifdef UTILS_MEM_DEBUG
    Vps_printf(" UTILS: MEM: BITS ALLOC, addr = 0x%08x, size = %d bytes\n", pBaseAddr, (size * numFrames));
    #endif

    if (!Error_check(eb)
        &&
        (pBaseAddr != NULL)
        &&
        gUtils_memClearBuf[UTILS_MEM_VID_BITS_BUF_HEAP])
    {
        memset(pBaseAddr, 0x80, (size * numFrames));
    }

    if (pBaseAddr == NULL)
    {
        status = -1;                                       /* Error in
                                                            * allocation,
                                                            * exit with error
                                                            */
    }

    if (!UTILS_ISERROR(status))
    {
        /* init memory pointer for 'numFrames' */
        for (bufId = 0; bufId < numFrames; bufId++)
        {

            /* copy channelNum to Bitstream_Buf from FVID2_Format */
            pBuf->channelNum = bufId;
            pBuf->addr = pBaseAddr;
            /* On the RTOS side, we assume Virtual addr == PhyAddr */
            pBuf->phyAddr = (UInt32)pBaseAddr;
            pBuf->bufSize = size;
            /* go to next frame */
            pBuf++;

            /* increment base address */
            pBaseAddr += size;
        }
    }

    UTILS_assert(status == 0);

    return status;
}

Int32 Utils_memBitBufFree(Bitstream_Buf * pBuf, UInt16 numFrames)
{
    UInt32 size;

    size = pBuf->bufSize;

    /* free the frame buffer memory */

    #ifdef UTILS_MEM_DEBUG
    Vps_printf(" UTILS: MEM: BITS FREE, addr = 0x%08x, size = %d bytes\n", pBuf->addr, (size * numFrames));
    #endif


    /* for all 'numFrames' memory is allocated contigously during alloc, so
     * first frame memory pointer points to the complete memory block for all
     * frames */
    Memory_free(gUtils_heapMemHandle[UTILS_MEM_VID_BITS_BUF_HEAP],
                pBuf->addr, (size * numFrames));

    return 0;
}
