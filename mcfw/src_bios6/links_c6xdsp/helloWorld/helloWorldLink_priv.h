/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#ifndef _HELLOWORLD_LINK_PRIV_H_
#define _HELLOWORLD_LINK_PRIV_H_

#include <mcfw/src_bios6/utils/utils.h>
#include <mcfw/src_bios6/links_c6xdsp/system/system_priv_c6xdsp.h>
#include <mcfw/interfaces/link_api/helloWorldLink.h>

#define HELLOWORLD_LINK_OBJ_MAX  (SYSTEM_LINK_ID_HELLOWORLD_COUNT)

#define HELLOWORLD_LINK_MAX_OUT_FRAMES_PER_CH       (SYSTEM_LINK_FRAMES_PER_CH)

#define HELLOWORLD_LINK_MAX_OUT_FRAMES             (HELLOWORLD_LINK_OBJ_MAX * \
	                                          HELLOWORLD_LINK_MAX_OUT_FRAMES_PER_CH)

#define HELLOWORLD_LINK_OUT_BUF_SIZE         100
#define HELLOWORLD_BUFFER_ALIGNMENT          128

typedef struct HelloWorldLink_OutObj {
    Utils_BitBufHndl bufOutQue;
    UInt32           numAllocPools;
    Bitstream_Buf    outBufs[HELLOWORLD_LINK_MAX_OUT_FRAMES];
    UInt32           outNumBufs[UTILS_BITBUF_MAX_ALLOC_POOLS];
    UInt32           bufSize[UTILS_BITBUF_MAX_ALLOC_POOLS];
    UInt32           ch2poolMap[HELLOWORLD_LINK_MAX_CH];
} HelloWorldLink_OutObj;

typedef struct HelloWorldLink_Obj {
    UInt32 linkId;
    
    char name[32];
    
    Utils_TskHndl tsk;
    System_LinkInfo inTskInfo;
    System_LinkQueInfo inQueInfo;

    HelloWorldLink_CreateParams createArgs;

    System_LinkInfo info;

    HelloWorldLink_OutObj outObj[HELLOWORLD_LINK_MAX_OUT_QUE];
    
	/* Algorithm object */
    void *algHndl;
} HelloWorldLink_Obj;


Int32 HelloWorldLink_create(HelloWorldLink_Obj * pObj, HelloWorldLink_CreateParams * pPrm);
Int32 HelloWorldLink_processData(HelloWorldLink_Obj * pObj);
Int32 HelloWorldLink_delete(HelloWorldLink_Obj * pObj);
Int32 HelloWorldLink_getFullBufs(Utils_TskHndl * pTsk, UInt16 queId,
                           Bitstream_BufList * pBufList);
Int32 HelloWorldLink_putEmptyBufs(Utils_TskHndl * pTsk, UInt16 queId,
                           Bitstream_BufList * pBufList);
Int32 HelloWorldLink_getInfo(Utils_TskHndl * pTsk, System_LinkInfo * info);

#endif

