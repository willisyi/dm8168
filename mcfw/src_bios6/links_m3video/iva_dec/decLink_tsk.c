/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include "decLink_priv.h"

#pragma DATA_ALIGN(gDecLink_tskStack, 32)
#pragma DATA_SECTION(gDecLink_tskStack, ".bss:taskStackSection")
UInt8 gDecLink_tskStack[DEC_LINK_OBJ_MAX][DEC_LINK_TSK_STACK_SIZE];

#pragma DATA_ALIGN(gDecLink_obj, 32)
#pragma DATA_SECTION(gDecLink_obj, ".bss:gDecLink_objSection")
DecLink_Obj gDecLink_obj[DEC_LINK_OBJ_MAX];


Void DecLink_tskMain(struct Utils_TskHndl *pTsk, Utils_MsgHndl * pMsg)
{
    UInt32 cmd = Utils_msgGetCmd(pMsg);
    Bool ackMsg, done;
    Int32 status;
    DecLink_Obj *pObj;
    UInt32 flushCmds[2];
    UInt32 originalCmd;

    pObj = (DecLink_Obj *) pTsk->appData;

    if (cmd != SYSTEM_CMD_CREATE)
    {
        #ifndef DEC_LINK_SUPRESS_ERROR_AND_RESET
        DECLINK_INTERNAL_ERROR_LOG(DEC_LINK_E_INVALIDCMD,
                                   "Link create should be first cmd."
                                   "Received Cmd:%d", cmd);
        #endif
        Utils_tskAckOrFreeMsg(pMsg, FVID2_EFAIL);
        return;
    }

    status = DecLink_codecCreate(pObj, Utils_msgGetPrm(pMsg));

    Utils_tskAckOrFreeMsg(pMsg, status);

    if (status != FVID2_SOK)
        return;

    Utils_encdecHdvicpPrfInit();
    done = FALSE;
    ackMsg = FALSE;
    pObj->pMsgTmp = NULL;
    pObj->lateAckStatus = FVID2_SOK;

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

                flushCmds[0] = SYSTEM_CMD_NEW_DATA;
                Utils_tskFlushMsg(pTsk, flushCmds, 1);

                DecLink_codecProcessData(pObj);
                break;

            case DEC_LINK_CMD_GET_PROCESSED_DATA:
                Utils_tskAckOrFreeMsg(pMsg, status);

                flushCmds[0] = DEC_LINK_CMD_GET_PROCESSED_DATA;
                Utils_tskFlushMsg(pTsk, flushCmds, 1);

                DecLink_codecGetProcessedDataMsgHandler(pObj);
                break;

            case DEC_LINK_CMD_PRINT_IVAHD_STATISTICS:
                Utils_tskAckOrFreeMsg(pMsg, status);

                Utils_encdecHdvicpPrfPrint();
                break;

            case DEC_LINK_CMD_PRINT_STATISTICS:
                DecLink_printStatistics(pObj, TRUE);
                Utils_tskAckOrFreeMsg(pMsg, status);
                break;

            case DEC_LINK_CMD_PRINT_BUFFER_STATISTICS:
                Utils_tskAckOrFreeMsg(pMsg, status);
                DecLink_printBufferStatus(pObj);
                break;

            case DEC_LINK_CMD_CREATE_CHANNEL:
                {
                    DecLink_addChannelInfo *params;

                    params = (DecLink_addChannelInfo*) Utils_msgGetPrm(pMsg);
                    if (DEC_LINK_S_SUCCESS == 
                        DecLink_codecCreateChannelHandler(pObj, params))
                    {
                        Utils_tskAckOrFreeMsg(pMsg, status);
                    }
                    else
                    {
                        UTILS_assert(pObj->pMsgTmp == NULL);
                        pObj->pMsgTmp = pMsg;
                        pObj->lateAckStatus = FVID2_EFAIL;
                    }
                }
                break;

            case DEC_LINK_CMD_DELETE_CHANNEL:
                {
                    DecLink_ChannelInfo *params;

                    params = (DecLink_ChannelInfo *) Utils_msgGetPrm(pMsg);
                    DecLink_codecDeleteChannelHandler(pObj, params);
                    UTILS_assert(pObj->pMsgTmp == NULL);
                    pObj->pMsgTmp = pMsg;
                    pObj->lateAckStatus = FVID2_SOK;
                }
                break;
                
            case DEC_LINK_CMD_LATE_ACK:
                originalCmd = Utils_msgGetCmd(pObj->pMsgTmp);
                Utils_tskAckOrFreeMsg(pMsg, status);
                UTILS_assert((originalCmd == DEC_LINK_CMD_DELETE_CHANNEL) || 
                             ((originalCmd == DEC_LINK_CMD_CREATE_CHANNEL) && 
                              (pObj->lateAckStatus == FVID2_EFAIL)));
                if (pObj->pMsgTmp != NULL)
                {
                    Utils_tskAckOrFreeMsg(pObj->pMsgTmp, pObj->lateAckStatus);
                }
                pObj->pMsgTmp = NULL;
                break;

            case DEC_LINK_CMD_DISABLE_CHANNEL:
                {
                    DecLink_ChannelInfo *params;

                    params = (DecLink_ChannelInfo *) Utils_msgGetPrm(pMsg);
                    DecLink_codecDisableChannel(pObj, params);
                    Utils_tskAckOrFreeMsg(pMsg, status);
                }
                break;
            case DEC_LINK_CMD_ENABLE_CHANNEL:
                {
                    DecLink_ChannelInfo *params;

                    params = (DecLink_ChannelInfo *) Utils_msgGetPrm(pMsg);
                    DecLink_codecEnableChannel(pObj, params);
                    Utils_tskAckOrFreeMsg(pMsg, status);
                }
                break;

            case DEC_LINK_CMD_SET_TRICKPLAYCONFIG:
                {
                    DecLink_TPlayConfig * params;
                    params = (DecLink_TPlayConfig *) Utils_msgGetPrm(pMsg);
                    DecLink_setTPlayConfig(pObj, params);
                    Utils_tskAckOrFreeMsg(pMsg, status);
                }
                break;

            case DEC_LINK_CMD_RESET_DEC_ERR_REPORTING: 
                {
                    DecLink_ChErrorReport * params;
                    params = (DecLink_ChErrorReport *) Utils_msgGetPrm(pMsg);
                    DecLink_resetDecErrorReporting(pObj, params);
                    Utils_tskAckOrFreeMsg(pMsg, status);
                }
                break;
            case SYSTEM_CMD_STOP:
                DecLink_codecStop(pObj);
                Utils_tskAckOrFreeMsg(pMsg, status);
                break;

            case SYSTEM_CMD_DELETE:
                DecLink_codecStop(pObj);
                done = TRUE;
                ackMsg = TRUE;
                break;
            case DEC_LINK_CMD_GET_BUFFER_STATISTICS:
            {
                DecLink_BufferStats * params;

                params = (DecLink_BufferStats *) Utils_msgGetPrm(pMsg);
                DecLink_getBufferStatus(pObj, params);
                Utils_tskAckOrFreeMsg(pMsg, status);
                break;
            }
            case DEC_LINK_CMD_INTERNAL_IVAMAPCHANGE:
            {
                DecLink_codecIVAMapChangeHandler(pObj);
                Utils_tskAckOrFreeMsg(pMsg, DEC_LINK_S_SUCCESS);
                break;
            }
            default:
                Utils_tskAckOrFreeMsg(pMsg, status);
                break;
        }
    }

    DecLink_codecDelete(pObj);

    if (ackMsg && pMsg != NULL)
        Utils_tskAckOrFreeMsg(pMsg, status);

    return;
}

Int32 DecLink_init()
{
    Int32 status;
    System_LinkObj linkObj;
    DecLink_Obj *pObj;
    char name[32];
    UInt32 objId;

    for (objId = 0; objId < DEC_LINK_OBJ_MAX; objId++)
    {
        pObj = &gDecLink_obj[objId];

        memset(pObj, 0, sizeof(*pObj));
        pObj->linkId = SYSTEM_LINK_ID_VDEC_0 + objId;

        linkObj.pTsk = &pObj->tsk;
        linkObj.linkGetFullFrames = DecLink_getFullFrames;
        linkObj.linkPutEmptyFrames = DecLink_putEmptyFrames;
        linkObj.linkGetFullBitBufs = NULL;
        linkObj.linkPutEmptyBitBufs = NULL;
        linkObj.getLinkInfo = DecLink_getInfo;

        UTILS_SNPRINTF(name, "DEC%d   ", objId);

        System_registerLink(pObj->linkId, &linkObj);

        status = Utils_tskCreate(&pObj->tsk,
                                 DecLink_tskMain,
                                 DEC_LINK_TSK_PRI,
                                 gDecLink_tskStack[objId],
                                 DEC_LINK_TSK_STACK_SIZE, pObj, name);
        UTILS_assert(status == FVID2_SOK);
    }

    return status;
}

Int32 DecLink_deInit()
{
    UInt32 objId;
    DecLink_Obj *pObj;

    for (objId = 0; objId < DEC_LINK_OBJ_MAX; objId++)
    {
        pObj = &gDecLink_obj[objId];

        Utils_tskDelete(&pObj->tsk);
    }

    return FVID2_SOK;
}

Int32 DecLink_getInfo(Utils_TskHndl * pTsk, System_LinkInfo * info)
{
    DecLink_Obj *pObj = (DecLink_Obj *) pTsk->appData;

    memcpy(info, &pObj->info, sizeof(*info));

    return FVID2_SOK;
}

Int32 DecLink_getFullFrames(Utils_TskHndl * pTsk, UInt16 queId,
                            FVID2_FrameList * pFrameList)
{
    DecLink_Obj *pObj = (DecLink_Obj *) pTsk->appData;

    UTILS_assert(queId < DEC_LINK_MAX_OUT_QUE);

    return Utils_bufGetFullExt(&pObj->outObj.bufOutQue, pFrameList,
                               BIOS_NO_WAIT);
}

Int32 DecLink_putEmptyFrames(Utils_TskHndl * pTsk, UInt16 queId,
                             FVID2_FrameList * pFrameList)
{
    DecLink_Obj *pObj = (DecLink_Obj *) pTsk->appData;
    Int freeStatus;//,status;

    UTILS_assert(queId < DEC_LINK_MAX_OUT_QUE);


    freeStatus =  DecLink_codecFreeProcessedFrames(pObj, pFrameList);
    return (freeStatus);
}
