/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2011 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
    \file chains_ipcBufAlloc.c
    \brief
*/
#include <mcfw/interfaces/ti_media_std.h>
#include <ti/syslink/utils/IHeap.h>
#include <ti/syslink/utils/Memory.h>
#include <ti/ipc/SharedRegion.h>
#include <mcfw/src_linux/osa/inc/osa_debug.h>
#include <string.h>

Void Chains_fillBuf(UInt8 *bufPtr, Int8 *fileName, UInt32 fSize)
{
	FILE *fp = NULL;

	fp = fopen(fileName, "rb");

	OSA_assert(fp != NULL);

	OSA_assert(fread(bufPtr, 1, fSize, fp) == fSize);

	fclose(fp);
}

Void Chains_createBuf(Ptr *phyAddr, Ptr *bufPtr, Ptr *srBufPtr,
                      UInt32 bufSize, UInt32 srIndex)
{
    IHeap_Handle srBitBufHeapHandle;
    UInt32  cacheLineSize = 128;

    srBitBufHeapHandle = SharedRegion_getHeap(srIndex);
    OSA_assert(srBitBufHeapHandle != NULL);
    /* if(srIndex == SYSTEM_IPC_SR_CACHED)
    {
        cacheLineSize = SharedRegion_getCacheLineSize(srIndex);
    } */

    bufSize = OSA_align(bufSize, cacheLineSize);

    *bufPtr = (Ptr)
    Memory_alloc(srBitBufHeapHandle, bufSize, cacheLineSize, NULL);
    OSA_assert(*bufPtr != NULL);

	
    *phyAddr =  Memory_translate (*bufPtr,
    Memory_XltFlags_Virt2Phys);

    *srBufPtr = (Ptr)SharedRegion_getSRPtr(*bufPtr,srIndex);
    OSA_printf("CHAINS_IPCALLOCBUF:Translated Addr Virt:%p To Phy:%p Shared:%p",
    *bufPtr,*phyAddr,*srBufPtr);

    OSA_assert(*srBufPtr != 0);
}

Void Chains_deleteBuf(Ptr bufPtr, UInt32 bufSize, UInt32 srIndex)
{
	IHeap_Handle srBitBufHeapHandle;

	srBitBufHeapHandle = SharedRegion_getHeap(srIndex);
	OSA_assert(srBitBufHeapHandle != NULL);

	OSA_assert(bufPtr != NULL);

	Memory_free(srBitBufHeapHandle, bufPtr, bufSize);
}



