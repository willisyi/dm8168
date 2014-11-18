/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include "algLink_priv.h"

#pragma DATA_ALIGN(gAlgLink_tskStack, 32)
#pragma DATA_SECTION(gAlgLink_tskStack, ".bss:taskStackSection")
UInt8 gAlgLink_tskStack[ALG_LINK_OBJ_MAX][ALG_LINK_TSK_STACK_SIZE];

AlgLink_Obj gAlgLink_obj[ALG_LINK_OBJ_MAX];


Void AlgLink_tskMain(struct Utils_TskHndl *pTsk, Utils_MsgHndl * pMsg)
{
    UInt32 cmd = Utils_msgGetCmd(pMsg);
    Bool ackMsg, done;
    Int32 status;
    AlgLink_Obj *pObj;
    UInt32 flushCmds[4];

    pObj = (AlgLink_Obj *) pTsk->appData;

    if (cmd != SYSTEM_CMD_CREATE)
    {
        Utils_tskAckOrFreeMsg(pMsg, FVID2_EFAIL);
        return;
    }

    status = AlgLink_algCreate(pObj, Utils_msgGetPrm(pMsg));

    Utils_tskAckOrFreeMsg(pMsg, status);

    if (status != FVID2_SOK)
        return;
#if 0
    if((pObj->createArgs.enableOSDAlg == 0) && (pObj->createArgs.enableSCDAlg == 1))
    {
       /* If algLink has only SCD algorithm, modify priority of link containing SCD to lower priority .
       * This ensures SCD operation happens in background and
       * is preempted by other algLinks whose algorithms have stringent realtime deadlines
       * link SWOSD
       */

       Vps_printf(" Warning: Changing Tsk priority of %s ALG Link \n",pObj->name);
       status = Utils_tskSetPri(pTsk, ALG_LINK_TSK_PRI_SCD);
       if (status != FVID2_SOK)
          Vps_printf(" Could not change Tsk priority of %s ALG Link \n",pObj->name);
    }
#endif
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
  		          flushCmds[0] = SYSTEM_CMD_NEW_DATA;
                  Utils_tskFlushMsg(pTsk, flushCmds, 1);

                  AlgLink_algProcessData(pObj);
                  break;

            case ALG_LINK_OSD_CMD_SET_CHANNEL_WIN_PRM:
                 {
                   AlgLink_OsdChWinParams *params;

                   params = (AlgLink_OsdChWinParams *) Utils_msgGetPrm(pMsg);
                   AlgLink_OsdalgSetChOsdWinPrm(&pObj->osdAlg, params);
                   Utils_tskAckOrFreeMsg(pMsg, status);
                 }
                break;

            case ALG_LINK_SCD_CMD_SET_CHANNEL_BLOCKCONFIG:
                 {
                   AlgLink_ScdChblkUpdate *params;

                   params = (AlgLink_ScdChblkUpdate *) Utils_msgGetPrm(pMsg);
                   AlgLink_ScdalgSetChblkUpdate(&pObj->scdAlg, params);
                   Utils_tskAckOrFreeMsg(pMsg, status);
                 }
                break;

            case ALG_LINK_SCD_CMD_PRINT_STATISTICS:
                AlgLink_ScdprintStatistics(&pObj->scdAlg, TRUE);
                Utils_tskAckOrFreeMsg(pMsg, status);
                break;


            case ALG_LINK_SCD_CMD_SET_CHANNEL_IGNORELIGHTSON:
                 {
                   AlgLink_ScdChParams *params;

                   params = (AlgLink_ScdChParams *) Utils_msgGetPrm(pMsg);
                   AlgLink_ScdalgSetChScdIgnoreLightsOn(&pObj->scdAlg, params);
                   Utils_tskAckOrFreeMsg(pMsg, status);
                 }
                break;

            case ALG_LINK_SCD_CMD_SET_CHANNEL_IGNORELIGHTSOFF:
                 {
                   AlgLink_ScdChParams *params;

                   params = (AlgLink_ScdChParams *) Utils_msgGetPrm(pMsg);
                   AlgLink_ScdalgSetChScdIgnoreLightsOff(&pObj->scdAlg, params);
                   Utils_tskAckOrFreeMsg(pMsg, status);
                 }
                break;

            case ALG_LINK_SCD_CMD_SET_CHANNEL_MODE:
                 {
                   AlgLink_ScdChParams *params;

                   params = (AlgLink_ScdChParams *) Utils_msgGetPrm(pMsg);
                   AlgLink_ScdalgSetChScdMode(&pObj->scdAlg, params);
                   Utils_tskAckOrFreeMsg(pMsg, status);
                 }
                break;

            case ALG_LINK_SCD_CMD_SET_CHANNEL_SENSITIVITY:
                 {
                   AlgLink_ScdChParams *params;

                   params = (AlgLink_ScdChParams *) Utils_msgGetPrm(pMsg);
                   AlgLink_ScdalgSetChScdSensitivity(&pObj->scdAlg, params);
                   Utils_tskAckOrFreeMsg(pMsg, status);
                 }
                  break;
            case ALG_LINK_SCD_CMD_GET_ALL_CHANNEL_FRAME_STATUS:
                 {
                   AlgLink_ScdAllChFrameStatus *params;

                   params = (AlgLink_ScdAllChFrameStatus *) Utils_msgGetPrm(pMsg);
                   AlgLink_scdAlgGetAllChFrameStatus(&pObj->scdAlg, params);
                   Utils_tskAckOrFreeMsg(pMsg, status);
                 }
                break;
            case ALG_LINK_SCD_CMD_CHANNEL_RESET:
                 {
                   AlgLink_ScdChCtrl *params;

                   params = (AlgLink_ScdChCtrl *) Utils_msgGetPrm(pMsg);
                   AlgLink_ScdalgGetChResetChannel(&pObj->scdAlg, params);
                   Utils_tskAckOrFreeMsg(pMsg, status);
                 }
                  break;
            case SYSTEM_CMD_DELETE:
                flushCmds[0] = SYSTEM_CMD_NEW_DATA;
                Utils_tskFlushMsg(pTsk, flushCmds, 1);
                done = TRUE;
                ackMsg = TRUE;
                break;

            default:
                Utils_tskAckOrFreeMsg(pMsg, status);
                break;
        }
    }

    AlgLink_algDelete(pObj);

    if (ackMsg && pMsg != NULL)
        Utils_tskAckOrFreeMsg(pMsg, status);

    return;
}

Int32 AlgLink_init()
{
    Int32 status;
    System_LinkObj linkObj;
    AlgLink_Obj *pObj;
    UInt32 objId;

    for (objId = 0; objId < ALG_LINK_OBJ_MAX; objId++)
    {
        pObj = &gAlgLink_obj[objId];

        memset(pObj, 0, sizeof(*pObj));

        pObj->linkId = SYSTEM_LINK_ID_ALG_0 + objId;

        linkObj.pTsk = &pObj->tsk;
        linkObj.linkGetFullFrames   = NULL;
        linkObj.linkPutEmptyFrames  = NULL;
        linkObj.linkGetFullBitBufs  = AlgLink_getFullBufs;
        linkObj.linkPutEmptyBitBufs = AlgLink_putEmptyBufs;
        linkObj.getLinkInfo         = AlgLink_getInfo;

        sprintf(pObj->name, "ALG%d   ", objId);

        pObj->isCreated = ALG_LINK_STATE_INACTIVE;

        pObj->isCreated = ALG_LINK_STATE_INACTIVE;

        System_registerLink(pObj->linkId, &linkObj);

        status = Utils_tskCreate(&pObj->tsk,
                                 AlgLink_tskMain,
                                 ALG_LINK_TSK_PRI,
                                 gAlgLink_tskStack[objId],
                                 ALG_LINK_TSK_STACK_SIZE, pObj, pObj->name);
        UTILS_assert(status == FVID2_SOK);

        pObj->scdAlg.pTsk = &pObj->tsk;

        AlgLink_scdAlgProcessTskInit(&pObj->scdAlg, objId);

    }

    return status;
}


Int32 AlgLink_deInit()
{
    UInt32 objId;
    AlgLink_Obj *pObj;

    for (objId = 0; objId < ALG_LINK_OBJ_MAX; objId++)
    {
        pObj = &gAlgLink_obj[objId];

        AlgLink_scdAlgProcessTskDeInit(&pObj->scdAlg);

        Utils_tskDelete(&pObj->tsk);
    }

    return FVID2_SOK;
}

Int32 AlgLink_getInfo(Utils_TskHndl * pTsk, System_LinkInfo * info)
{
    AlgLink_Obj *pObj = (AlgLink_Obj *) pTsk->appData;

    memcpy(info, &pObj->info, sizeof(*info));

    return FVID2_SOK;
}
Int32 AlgLink_getFullBufs(Utils_TskHndl * pTsk, UInt16 queId,
                           Bitstream_BufList * pBufList)
{
    AlgLink_Obj *pObj = (AlgLink_Obj *) pTsk->appData;

    UTILS_assert(queId < ALG_LINK_MAX_OUT_QUE);

    if(pObj->isCreated == ALG_LINK_STATE_ACTIVE)
    {
       /* If Alg link is active then pass buffer list to next link */
       return Utils_bitbufGetFull(&pObj->scdAlg.outObj[queId].bufOutQue, pBufList, BIOS_NO_WAIT);
    }
    else
    {
       /* If Alg link is in-active don't do anything */
       pBufList->numBufs = 0;
       return FVID2_SOK;
    }
}

Int32 AlgLink_putEmptyBufs(Utils_TskHndl * pTsk, UInt16 queId,
                           Bitstream_BufList * pBufList)
{
    AlgLink_Obj *pObj = (AlgLink_Obj *) pTsk->appData;

    UTILS_assert(queId < ALG_LINK_MAX_OUT_QUE);

    if(pObj->isCreated == ALG_LINK_STATE_ACTIVE)
       return Utils_bitbufPutEmpty(&pObj->scdAlg.outObj[queId].bufOutQue, pBufList);
    else
       return FVID2_SOK;
}
