/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include "helloWorldLink_priv.h"

#pragma DATA_ALIGN(gHelloWorldLink_tskStack, 32)
#pragma DATA_SECTION(gHelloWorldLink_tskStack, ".bss:taskStackSection")
UInt8 gHelloWorldLink_tskStack[HELLOWORLD_LINK_OBJ_MAX][HELLOWORLD_LINK_TSK_STACK_SIZE];

HelloWorldLink_Obj gHelloWorldLink_obj[HELLOWORLD_LINK_OBJ_MAX];


/* ===================================================================
 *  @func     HelloWorldLink_tskMain
 *
 *  @desc     Each link run as independent thread. This is main task
 *            that recevies the command from host/other core and 
 *            act upon them 
 *
 *  @modif    This function modifies the following structures
 *
 *  @inputs   This function takes the following inputs
 *            <Utils_TskHndl>
 *            Handle to the task 
 *            <Utils_MsgHndl>
 *            Handle to the message
 *
 *  @outputs  
 *
 *  @return   None
 *  ==================================================================
 */
Void HelloWorldLink_tskMain(struct Utils_TskHndl *pTsk, Utils_MsgHndl * pMsg)
{
    UInt32 cmd = Utils_msgGetCmd(pMsg);
    Bool ackMsg, done;
    Int32 status;
    HelloWorldLink_Obj *pObj;

    pObj = (HelloWorldLink_Obj *) pTsk->appData;

    if (cmd != SYSTEM_CMD_CREATE)
    {
        Utils_tskAckOrFreeMsg(pMsg, FVID2_EFAIL);
        return;
    }

	/* Create algorithm isntance. At this place allocate memory resource and DMA resource, if any */
    status = HelloWorldLink_create(pObj, Utils_msgGetPrm(pMsg));

    Utils_tskAckOrFreeMsg(pMsg, status);

    if (status != FVID2_SOK)
        return;

    done = FALSE;
    ackMsg = FALSE;

    while (!done)
    {
        status = Utils_tskRecvMsg(pTsk, &pMsg, BIOS_WAIT_FOREVER);
        if (status != FVID2_SOK)
            break;

        cmd = Utils_msgGetCmd(pMsg);

        switch (cmd)
        {
            case SYSTEM_CMD_NEW_DATA:
                  Utils_tskAckOrFreeMsg(pMsg, status);
                  HelloWorldLink_processData(pObj);
                  break;

            case HELLOWORLD_LINK_CMD_PRINT_STATISTICS:
               // HelloWorldLink_printStatistics(&pObj->scdAlg, TRUE);
                Utils_tskAckOrFreeMsg(pMsg, status);
                break;

            case SYSTEM_CMD_DELETE:
                done = TRUE;
                ackMsg = TRUE;
                break;

            /*
			case SYSTEM_CMD_CONFIG_X_PRM
			//Algorithm call to configure/tune run time parameter X
			break;
			*/

            default:
                Utils_tskAckOrFreeMsg(pMsg, status);
                break;
        }
    }

    HelloWorldLink_delete(pObj);

    if (ackMsg && pMsg != NULL)
        Utils_tskAckOrFreeMsg(pMsg, status);

    return;
}

/* ===================================================================
 *  @func     HelloWorldLink_init
 *
 *  @desc     This module creates the HelloWorld link instance
 *
 *  @modif    This function modifies the following structures
 *
 *  @inputs   This function takes no inputs
 *            
 *  @outputs  
 *            
 *
 *  @return   Status
 *			  FVID2_SOK: If HelloWorld link instance created successfuly 
 *  ====================================================================
 */
Int32 HelloWorldLink_init()
{
    Int32 status;
    System_LinkObj linkObj;
    HelloWorldLink_Obj *pObj;
    UInt32 objId;

    for (objId = 0; objId < HELLOWORLD_LINK_OBJ_MAX; objId++)
    {
        pObj = &gHelloWorldLink_obj[objId];

        memset(pObj, 0, sizeof(*pObj));

        pObj->linkId = SYSTEM_LINK_ID_HELLOWORLD_0 + objId;

        linkObj.pTsk = &pObj->tsk;
        linkObj.linkGetFullFrames   = NULL;
        linkObj.linkPutEmptyFrames  = NULL;
        linkObj.linkGetFullBitBufs  = HelloWorldLink_getFullBufs;
        linkObj.linkPutEmptyBitBufs = HelloWorldLink_putEmptyBufs;
        linkObj.getLinkInfo         = HelloWorldLink_getInfo;

        sprintf(pObj->name, "HELLOWORLD%d   ", objId);

        System_registerLink(pObj->linkId, &linkObj);

        status = Utils_tskCreate(&pObj->tsk,
                                 HelloWorldLink_tskMain,
                                 HELLOWORLD_LINK_TSK_PRI,
                                 gHelloWorldLink_tskStack[objId],
                                 HELLOWORLD_LINK_TSK_STACK_SIZE, pObj, pObj->name);
        UTILS_assert(status == FVID2_SOK);
    }

    return status;
}

/* ===================================================================
 *  @func     HelloWorldLink_deInit
 *
 *  @desc     This module deletes the HelloWorld link instance
 *
 *  @modif    This function modifies the following structures
 *
 *  @inputs   This function takes no inputs
 *            
 *  @outputs  
 *            
 *
 *  @return   Status
 *			  FVID2_SOK: If instance deletion is successfull
 *  ==================================================================
 */
Int32 HelloWorldLink_deInit()
{
    UInt32 objId;
    HelloWorldLink_Obj *pObj;

    for (objId = 0; objId < HELLOWORLD_LINK_OBJ_MAX; objId++)
    {
        pObj = &gHelloWorldLink_obj[objId];

        Utils_tskDelete(&pObj->tsk);
    }

    return FVID2_SOK;
}

/* ===================================================================
 *  @func     HelloWorldLink_getInfo
 *
 *  @desc     This module is call by next link (that HelloWorld link is 
 *            connected to) to get the output buffer of helloWorld link.
 *            This is input buffer to next link
 *
 *  @modif    This function modifies the following structures
 *
*  @inputs   <Utils_TskHndl>
 *            handle to the task 
 *            <UInt16>
 *            Id of the queue from which this buffer needs to be received
 *            <Bitstream_BufList>
 *            List of buffers received
 *            
 *  @outputs  
 *            
 *
 *  @return   Status
 *			  FVID2_SOK: If buffers were pushed to the out queue 
 *                       successfuly 
 *  ====================================================================
 */
Int32 HelloWorldLink_getInfo(Utils_TskHndl * pTsk, System_LinkInfo * info)
{
    HelloWorldLink_Obj *pObj = (HelloWorldLink_Obj *) pTsk->appData;

    memcpy(info, &pObj->info, sizeof(*info));

    return FVID2_SOK;
}

/* ===================================================================
 *  @func     HelloWorldLink_getFullBufs
 *
 *  @desc     This module is call by next link (that HelloWorld link is 
 *            connected to) to get the output buffer of helloWorld link.
 *            This is input buffer to next link
 *
 *  @modif    This function modifies the following structures
 *
*  @inputs   <Utils_TskHndl>
 *            handle to the task 
 *            <UInt16>
 *            Id of the queue from which this buffer needs to be received
 *            <Bitstream_BufList>
 *            List of buffers received
 *            
 *  @outputs  
 *            
 *
 *  @return   Status
 *			  FVID2_SOK: If buffers were pushed to the out queue 
 *                       successfuly 
 *  ====================================================================
 */
Int32 HelloWorldLink_getFullBufs(Utils_TskHndl * pTsk, UInt16 queId,
                           Bitstream_BufList * pBufList)
{
    HelloWorldLink_Obj *pObj = (HelloWorldLink_Obj *) pTsk->appData;

    UTILS_assert(queId < HELLOWORLD_LINK_MAX_OUT_QUE);

    return Utils_bitbufGetFull(&pObj->outObj[queId].bufOutQue, pBufList, BIOS_NO_WAIT);
}


/* ===================================================================
 *  @func     HelloWorldLink_putEmptyBufs
 *
 *  @desc     This module is call by next link (that HelloWorld link is 
 *            connected to) free the output buffer of helloWorld link 
 *            that it received via it's input queue 
 *
 *  @modif    This function modifies the following structures
 *
 *  @inputs   <Utils_TskHndl>
 *            handle to the task 
 *            <UInt16>
 *            Id of the queue to which this buffer needs to be released
 *            <Bitstream_BufList>
 *            List of buffers to be freed
 *  @outputs  
 *            
 *
 *  @return   Status
 *			  FVID2_SOK: If buffers were pushed to the out queue 
 *                       successfuly 
 *  ====================================================================
 */
Int32 HelloWorldLink_putEmptyBufs(Utils_TskHndl * pTsk, UInt16 queId,
                           Bitstream_BufList * pBufList)
{
    HelloWorldLink_Obj *pObj = (HelloWorldLink_Obj *) pTsk->appData;

    UTILS_assert(queId < HELLOWORLD_LINK_MAX_OUT_QUE);

    return Utils_bitbufPutEmpty(&pObj->outObj[queId].bufOutQue, pBufList);
}
