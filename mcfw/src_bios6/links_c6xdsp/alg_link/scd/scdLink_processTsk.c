/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include "../algLink_priv.h"

#pragma DATA_ALIGN(gScdLink_processTskStack, 32)
#pragma DATA_SECTION(gScdLink_processTskStack, ".bss:taskStackSection")
UInt8 gScdLink_processTskStack[ALG_LINK_OBJ_MAX][VPSS_ALG_LINK_TSK_STACK_SIZE];


Void AlgLink_scdAlgProcessTskMain(struct Utils_TskHndl *pTsk, Utils_MsgHndl * pMsg)
{
    UInt32 cmd = Utils_msgGetCmd(pMsg);
    Bool ackMsg, done = FALSE;
    Int32 status;
    AlgLink_ScdObj *pObj;

    pObj = (AlgLink_ScdObj *) pTsk->appData;

    if (cmd != SYSTEM_CMD_START)
    {
        Utils_tskAckOrFreeMsg(pMsg, FVID2_SOK);
        return;
    }

    Vps_rprintf(" %d: SCD: Process Tsk Started !!!\n",
        Utils_getCurTimeInMsec()
        );

    Utils_tskAckOrFreeMsg(pMsg, FVID2_SOK);

    pObj->processTskState  = ALG_LINK_PROCESS_TASK_STATE_RUNNING;

    while (!done)
    {
        pMsg = NULL;

        status = Utils_tskRecvMsg(pTsk, &pMsg, BIOS_NO_WAIT);

        if(status==FVID2_SOK && pMsg)
        {
            cmd = Utils_msgGetCmd(pMsg);

            switch (cmd)
            {
                case SYSTEM_CMD_STOP:
                    done = TRUE;
                    ackMsg = TRUE;
                    break;

                default:
                    Utils_tskAckOrFreeMsg(pMsg, status);
                    break;
            }
        }

        if(!done)
        {
            #ifdef SYSTEM_DEBUG_SCD_RT
            Vps_rprintf(" %d: SCD: Process Tsk Running !!!\n",
                Utils_getCurTimeInMsec()
                );
            #endif

            status = AlgLink_ScdalgProcessData(pObj);
            if (status == FVID2_SOK)
            {
                /* Send-out the output bitbuffer */
                System_sendLinkCmd(pObj->outQueParams[ALG_LINK_SCD_OUT_QUE].nextLink,
                                   SYSTEM_CMD_NEW_DATA);
            }

        }
    }

    pObj->processTskState  = ALG_LINK_PROCESS_TASK_STATE_STOPPED;

    Vps_rprintf(" %d: SCD: Process Tsk Stopped !!!\n",
        Utils_getCurTimeInMsec()
        );

    if (ackMsg && pMsg != NULL)
        Utils_tskAckOrFreeMsg(pMsg, status);

    return;
}

Int32 AlgLink_scdAlgProcessTskSendCmd(AlgLink_ScdObj *pObj, UInt32 cmd)
{
    Int32 status;


    if(cmd==SYSTEM_CMD_STOP)
    {
        UInt32 oldIntState;

        Vps_rprintf(" %d: SCD: Process Tsk Stopping !!!\n",
            Utils_getCurTimeInMsec()
            );

        oldIntState = Hwi_disable();

        pObj->processTskState  = ALG_LINK_PROCESS_TASK_STATE_STOPPING;
        /* put a NULL frame to unblock waiting process Tsk */
//        Utils_quePut(&pObj->processQ, NULL, BIOS_NO_WAIT);
        /* Unblock process queue to unblock waiting process Tsk */
        Utils_queUnBlock(&pObj->processQ);

        Hwi_restore(oldIntState);
    }

    status = Utils_tskSendMsg(
                pObj->pTsk,
                &pObj->processTsk,
                cmd,
                NULL,
                UTILS_MBX_FLAG_WAIT_ACK
            );

    if(cmd==SYSTEM_CMD_STOP)
    {
      Vps_rprintf(" %d: SCD: Process Tsk Stopping  DONE !!!\n",
          Utils_getCurTimeInMsec()
          );
    }
    UTILS_assert(status==FVID2_SOK);

    return status;
}



Int32 AlgLink_scdAlgProcessTskInit(AlgLink_ScdObj *pObj, UInt32 objId)
{
    Int32 status;

    sprintf(pObj->processTskName, "SCD_PROCESS_TSK%d", objId);

    status = Utils_tskCreate(&pObj->processTsk,
                             AlgLink_scdAlgProcessTskMain,
                             ALG_LINK_TSK_PRI_SCD,
                             gScdLink_processTskStack[objId],
                            ALG_LINK_TSK_STACK_SIZE, pObj, pObj->processTskName);
    UTILS_assert(status == FVID2_SOK);

    return status;
}

Int32 AlgLink_scdAlgProcessTskDeInit(AlgLink_ScdObj *pObj)
{
    Utils_tskDelete(&pObj->processTsk);

    return FVID2_SOK;
}


