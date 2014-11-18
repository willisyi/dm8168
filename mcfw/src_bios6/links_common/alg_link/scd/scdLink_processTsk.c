        /*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include "../algLink_priv.h"
#include "scdLink_priv.h"

#pragma DATA_ALIGN(gScdLink_processTskStack, 32)
#pragma DATA_SECTION(gScdLink_processTskStack, ".bss:taskStackSection")
UInt8 gScdLink_processTskStack[ALG_LINK_OBJ_MAX][VPSS_ALG_LINK_TSK_STACK_SIZE];

Int32 AlgLink_scdAlgDoTmpFrameInit(AlgLink_ScdObj * pObj, AlgLink_ScdChObj *pChObj, FVID2_Frame *pFrame)
{
    Int32 status = FVID2_SOK;
    UInt32 curTime;
    UInt32 oldIntState;

    if(pChObj->algReset)
    {
        oldIntState = Hwi_disable();

        pChObj->algReset = FALSE;

        pChObj->algInitMeanVarMHIPrm.inAddr = pFrame->addr[0][0];

        pChObj->algInitMeanVarMHIPrm.width     = pChObj->width;
        pChObj->algInitMeanVarMHIPrm.height    = pChObj->height;

        Hwi_restore(oldIntState);

        #ifdef SYSTEM_DEBUG_SCD_RT
        if(pChObj->rtPrmUpdate)
        {
            Vps_rprintf(" %d: SCD: CH%d: Reseting Alg due to RT Param update !!!\n",
                    Utils_getCurTimeInMsec(),
                    pFrame->channelNum
                );
        }
        else
        {
            Vps_rprintf(" %d: SCD: CH%d: Reseting Alg !!!\n",
                    Utils_getCurTimeInMsec(),
                    pFrame->channelNum
                );

        }
        #endif


        #ifdef SYSTEM_DEBUG_SCD_RT
        Vps_rprintf(" %d: SCD: CH%d: SCD_initMeanVarMHI() ... !!!\n",
                Utils_getCurTimeInMsec(),
                pFrame->channelNum
                );
        #endif

        curTime = Utils_getCurTimeInMsec();

        /* first frame  */
        status = SCD_initMeanVarMHI(&pObj->algObj, &pChObj->algInitMeanVarMHIPrm, 0);

        curTime = Utils_getCurTimeInMsec() - curTime;

        oldIntState = Hwi_disable();

        pObj->totalTime += curTime;

        Hwi_restore(oldIntState);

        #ifdef SYSTEM_DEBUG_SCD_RT
        Vps_rprintf(" %d: SCD: CH%d: SCD_initMeanVarMHI() ... DONE !!!\n",
                Utils_getCurTimeInMsec(),
                pFrame->channelNum
                );
        #endif

        if(status!=SCD_NO_ERROR)
        {
            Vps_printf(" %d: SCD: CH%d: ERROR (%d) in SCD_initMeanVarMHI() !!!\n",
                Utils_getCurTimeInMsec(),
                pFrame->channelNum,
                status
                );
        }
    }

    return status;
}

Int32 AlgLink_scdAlgProcessTskProcessFrame(AlgLink_ScdObj * pObj, AlgLink_ScdChObj *pChObj, FVID2_Frame *pFrame)
{
    Int32 status = FVID2_SOK;
    UInt32 curTime;
    Bool statusChangeDetected = FALSE;

    UInt32 oldIntState;

    oldIntState = Hwi_disable();

    pChObj->algProcessPrm.inAddr = pFrame->addr[0][0];

    if(pChObj->algProcessPrm.prevInAddr==NULL)
        pChObj->algProcessPrm.prevInAddr = pChObj->algProcessPrm.inAddr;

    if(pChObj->rtPrmUpdate)
    {
        pChObj->algProcessPrm.width = pChObj->width;
        pChObj->algProcessPrm.height = pChObj->height;

        pChObj->rtPrmUpdate = FALSE;
    }

    Hwi_restore(oldIntState);

    #ifdef SYSTEM_DEBUG_SCD_RT
    Vps_rprintf(" %d: SCD: CH%d: SCD_process() ... !!!\n",
            Utils_getCurTimeInMsec(),
            pFrame->channelNum
            );
    #endif

    curTime = Utils_getCurTimeInMsec();

    status = SCD_process(&pObj->algObj, &pChObj->algProcessPrm, &pChObj->algProcessStatus, 0);

    curTime = Utils_getCurTimeInMsec() - curTime;

    #ifdef SYSTEM_DEBUG_SCD_RT
    Vps_rprintf(" %d: SCD: CH%d: SCD_process() ... DONE !!!\n",
            Utils_getCurTimeInMsec(),
            pFrame->channelNum
            );
    #endif

    oldIntState = Hwi_disable();

    {
        UInt32 i;

        for(i=0; i<SCD_MAX_PROFILE_LOG; i++)
            pObj->profileLog[i] += pChObj->algProcessPrm.profileLog[i];
    }

    if(pChObj->algProcessStatus.status!=SCD_NO_ERROR)
    {
        #ifdef SYSTEM_VERBOSE_PRINTS
        char errorString[20];

        strncpy(errorString, "Unknown error", sizeof(errorString));
        if(pChObj->algProcessStatus.status==SCD_ERR_INPUT_INVALID)
            strncpy(errorString, "Input Invalid", sizeof(errorString));
        if(pChObj->algProcessStatus.status==SCD_ERR_MEMORY_POINTER_NULL)
            strncpy(errorString, "NULL memory pointer", sizeof(errorString));
        if(pChObj->algProcessStatus.status==SCD_ERR_INTERNAL_FAILURE)
            strncpy(errorString, "Internal Failure", sizeof(errorString));

        Vps_printf(" %d: SCD: CH%d: ERROR (%d [%s]) in SCD_process() !!!\n",
            Utils_getCurTimeInMsec(),
            pFrame->channelNum,
            pChObj->algProcessStatus.status,
            errorString
            );
        #endif
    }

    pChObj->scdStatus = ALG_LINK_SCD_DETECTOR_UNAVAILABLE;

    if(pChObj->algProcessStatus.output==SCD_DETECTOR_NO_CHANGE)
        pChObj->scdStatus = ALG_LINK_SCD_DETECTOR_NO_CHANGE;
    else
    if(pChObj->algProcessStatus.output==SCD_DETECTOR_CHANGE)
        pChObj->scdStatus = ALG_LINK_SCD_DETECTOR_CHANGE;


    if(pChObj->scdStatus==ALG_LINK_SCD_DETECTOR_CHANGE)
    {
        if(pChObj->prevScdStatus!=ALG_LINK_SCD_DETECTOR_CHANGE)
        {
            statusChangeDetected = TRUE;

            Vps_rprintf(" %d: SCD: CH%d: Tamper Detected !!!\n",
                Utils_getCurTimeInMsec(),
                pFrame->channelNum
                );

        }
    }
    if(pChObj->scdStatus==ALG_LINK_SCD_DETECTOR_NO_CHANGE)
    {
        if(pChObj->prevScdStatus!=ALG_LINK_SCD_DETECTOR_NO_CHANGE)
        {
            statusChangeDetected = TRUE;

            Vps_rprintf(" %d: SCD: CH%d: NO Tamper Detected !!!\n",
                Utils_getCurTimeInMsec(),
                pFrame->channelNum
                );
        }
    }

    pChObj->prevScdStatus = pChObj->scdStatus;

    pObj->totalTime += curTime;

    pChObj->algProcessPrm.prevInAddr = pChObj->algProcessPrm.inAddr;

    pChObj->inFrameProcessCount++;
    pObj->processFrameCount++;

    Hwi_restore(oldIntState);

    if(pChObj->pPrevProcessFrame!=NULL)
    {
        Utils_quePut(&pChObj->freeQ, pChObj->pPrevProcessFrame, BIOS_NO_WAIT);
    }

    pChObj->pPrevProcessFrame = pFrame;

    if(statusChangeDetected)
    {
        if(pObj->scdCreateParams.enableTamperNotify)
        {
            AlgLink_ScdChStatus chStatus;

            chStatus.chId = pChObj->chId;
            chStatus.frmResult = pChObj->scdStatus;

            System_linkControl(SYSTEM_LINK_ID_HOST, VSYS_EVENT_TAMPER_DETECT, &chStatus, sizeof(AlgLink_ScdChStatus), FALSE);
        }
    }

    return status;
}

Int32 AlgLink_scdAlgProcessTskRun(AlgLink_ScdObj * pObj)
{
    AlgLink_ScdChObj *pChObj;
    FVID2_Frame *pFrame;

    pFrame = NULL;

    Utils_queGet(&pObj->processQ, (Ptr*)&pFrame, 1, BIOS_WAIT_FOREVER);

    if(pFrame==NULL)
        return FVID2_EFAIL;

    pChObj = &pObj->chObj[pFrame->channelNum];

    AlgLink_scdAlgDoTmpFrameInit(pObj, pChObj, pFrame);

    AlgLink_scdAlgProcessTskProcessFrame(pObj, pChObj, pFrame);

    return FVID2_SOK;
}

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

    #ifdef SYSTEM_DEBUG_SCD_RT
    Vps_rprintf(" %d: SCD: Process Tsk Started !!!\n",
        Utils_getCurTimeInMsec()
        );
    #endif

    Utils_tskAckOrFreeMsg(pMsg, FVID2_SOK);

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

            status = AlgLink_scdAlgProcessTskRun(pObj);

            if(status!=FVID2_SOK)
            {
                done = TRUE;
            }
        }
    }

    #ifdef SYSTEM_DEBUG_SCD_RT
    Vps_rprintf(" %d: SCD: Process Tsk Stopped !!!\n",
        Utils_getCurTimeInMsec()
        );
    #endif

    if (ackMsg && pMsg != NULL)
        Utils_tskAckOrFreeMsg(pMsg, status);

    return;
}

Int32 AlgLink_scdAlgProcessTskSendCmd(AlgLink_ScdObj *pObj, UInt32 cmd)
{
    Int32 status;

    #ifdef SYSTEM_DEBUG_SCD_RT
    Vps_rprintf(" %d: SCD: Process Tsk Stopping !!!\n",
        Utils_getCurTimeInMsec()
        );
    #endif

    if(cmd==SYSTEM_CMD_STOP)
    {
        /* put a NULL frame to unblock waiting process Tsk */
        Utils_quePut(&pObj->processQ, NULL, BIOS_NO_WAIT);
    }

    status = Utils_tskSendMsg(
                pObj->pTsk,
                &pObj->processTsk,
                cmd,
                NULL,
                UTILS_MBX_FLAG_WAIT_ACK
            );

    #ifdef SYSTEM_DEBUG_SCD_RT
    Vps_rprintf(" %d: SCD: Process Tsk Stopping  DONE !!!\n",
        Utils_getCurTimeInMsec()
        );
    #endif

    UTILS_assert(status==FVID2_SOK);

    return status;
}



Int32 AlgLink_scdAlgProcessTskInit(AlgLink_ScdObj *pObj, UInt32 objId)
{
    Int32 status;

    sprintf(pObj->processTskName, "SCD_PROCESS_TSK%d", objId);

    status = Utils_tskCreate(&pObj->processTsk,
                             AlgLink_scdAlgProcessTskMain,
                             VPSS_ALG_LINK_TSK_PRI_SCD,
                             gScdLink_processTskStack[objId],
                             VPSS_ALG_LINK_TSK_STACK_SIZE, pObj, pObj->processTskName);
    UTILS_assert(status == FVID2_SOK);

    return status;
}

Int32 AlgLink_scdAlgProcessTskDeInit(AlgLink_ScdObj *pObj)
{
    Utils_tskDelete(&pObj->processTsk);

    return FVID2_SOK;
}


