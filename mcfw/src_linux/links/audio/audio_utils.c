/*******************************************************************************
 *                                                                            
 * Copyright (c) 2012 Texas Instruments Incorporated - http://www.ti.com/      
 *                        ALL RIGHTS RESERVED                                  
 *                                                                            
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include <mcfw/interfaces/ti_media_std.h>
#include <link_api/audioLink.h>

#include <ti/syslink/SysLink.h>
#include <ti/syslink/ProcMgr.h>
#include <ti/ipc/MultiProc.h>
#include <ti/syslink/utils/IHeap.h>
#include <ti/syslink/utils/Memory.h>
#include <ti/ipc/SharedRegion.h>

#define SR_FRAME_BUFFERS_ID     1   /* same as UTILS_MEM_VID_BITS_BUF_HEAP */
/**  ------ Globals  */
ProcMgr_Handle              procMgrHandle;
static Int32 gMallocedBufs = 0;
static Int32 gFreedBufs = 0;
static Int32 gAllocedSRBufs = 0;
static Int32 gFreedSRBufs = 0;


Void Audio_systemProcInit (Void)
{
    Int32   status;
    Int16   procId;
    Int16   state;

    /* 
     * SysLink_setup is the first APIs needs to be called by user side
     * application to use any SysLink functionality 
     */
    SysLink_setup ();

    procId = MultiProc_getId ("DSP");

    status = ProcMgr_open (&procMgrHandle, procId);
    if (0 > status) {
        AUDIO_ERROR_PRINT(("AUDIO: ProcMgr_open() Failed \n"));
        goto EXIT;
    }

    state = ProcMgr_getState (procMgrHandle);

EXIT:
    return;
}


Void Audio_systemProcDeInit (Void)
{
    Int32  state;
    Int32  status;

    state = ProcMgr_getState (procMgrHandle);

    status = ProcMgr_close (&procMgrHandle);

    SysLink_destroy ();
}

Void* Audio_allocMem (Int32 _size)
{
    Void *tmp = NULL; 

    if (_size)
    {

        tmp = malloc (_size);  
        if (tmp)
        {
            gMallocedBufs++;
        }
    }
    return tmp;
}

Void Audio_freeMem (Void *buf)
{
    if (buf)
    {
        free(buf);
        gFreedBufs++;
    }
}


Void *Audio_allocateSharedRegionBuf (Int32 bufSize)
{
    IHeap_Handle    heap = NULL;
    Void *tmp = NULL;

    heap = SharedRegion_getHeap(SR_FRAME_BUFFERS_ID);
    tmp = Memory_alloc (heap, bufSize, 128, NULL);
    if (tmp)
    {
        gAllocedSRBufs++;
    }
    return tmp;
}

Void Audio_freeSharedRegionBuf (Void *buf, Int32 bufSize)
{
    IHeap_Handle    heap = NULL;

    heap = SharedRegion_getHeap(SR_FRAME_BUFFERS_ID);
    if (heap)
    {
        Memory_free(heap, buf, bufSize);
        gFreedSRBufs++;
    }
}

Void Audio_printMemStats (Void)
{
    AUDIO_INFO_PRINT(("\n\nAUDIO: malloc cnt: %d, free cnt: %d\n", gMallocedBufs, gFreedBufs));
    AUDIO_INFO_PRINT(("AUDIO: SR Alloc cnt: %d, SR Free cnt: %d\n\n", gAllocedSRBufs, gFreedSRBufs));
}
