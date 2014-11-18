/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include <mcfw/src_bios6/utils/utils_mbx.h>

#define UTILS_MBX_MSG_POOL_MAX   (10240)

static Utils_QueHandle gUtils_mbxMsgPoolFreeQue;
static Ptr gUtils_mbxMsgPoolFreeQueMem[UTILS_MBX_MSG_POOL_MAX];
static Utils_MsgHndl gUtils_mbxMsgPool[UTILS_MBX_MSG_POOL_MAX];

UInt32 Utils_mbxGetFreeMsgCount()
{
    UInt32 cookie;
    UInt32 freeMsg;

    cookie = Hwi_disable();

    freeMsg = gUtils_mbxMsgPoolFreeQue.count;

    Hwi_restore(cookie);

    return freeMsg;
}

Utils_MsgHndl *Utils_mbxAllocMsg(UInt32 timeout)
{
    Utils_MsgHndl *pMsg = NULL;

    Utils_queGet(&gUtils_mbxMsgPoolFreeQue, (Ptr *) & pMsg, 1, timeout);

    return pMsg;
}

Int32 Utils_mbxFreeMsg(Utils_MsgHndl * pMsg, UInt32 timeout)
{
    Int32 status;

    status = Utils_quePut(&gUtils_mbxMsgPoolFreeQue, pMsg, timeout);

    return status;
}

Int32 Utils_mbxInit()
{
    Int32 status, msgId;

    status = Utils_queCreate(&gUtils_mbxMsgPoolFreeQue,
                             UTILS_MBX_MSG_POOL_MAX,
                             gUtils_mbxMsgPoolFreeQueMem,
                             UTILS_QUE_FLAG_BLOCK_QUE);

    if (status != 0)
        return status;

    for (msgId = 0; msgId < UTILS_MBX_MSG_POOL_MAX; msgId++)
    {
        status = Utils_mbxFreeMsg(&gUtils_mbxMsgPool[msgId], BIOS_NO_WAIT);

        if (status != 0)
        {
            Utils_mbxDeInit();
            break;
        }
    }

    return status;
}

Int32 Utils_mbxDeInit()
{
    Int32 status;

    status = Utils_queDelete(&gUtils_mbxMsgPoolFreeQue);

    return status;
}

Int32 Utils_mbxCreate(Utils_MbxHndl * pMbx)
{
    Int32 status;
    Int i;

    /* create queues */

    status = Utils_queCreate(&pMbx->recvQue,
                             UTILS_MBX_RECV_QUE_LEN_MAX,
                             pMbx->memRecvQue, UTILS_QUE_FLAG_BLOCK_QUE);

    if (status != 0)
        return status;

    memset(pMbx->ackQue, 0, sizeof(pMbx->ackQue));
    for (i = 0; i < UTILS_MBX_ACK_QUE_CNT_MAX; i++)
    {
        status = Utils_queCreate(&(pMbx->ackQue[i]),
                                 UTILS_MBX_ACK_QUE_LEN_MAX,
                                 pMbx->memAckQue[i], UTILS_QUE_FLAG_BLOCK_QUE);

        if (status != 0)
        {
            break;
        }
        pMbx->ackQueInUse[i] = FALSE;
    }
    if (i < UTILS_MBX_ACK_QUE_CNT_MAX)
    {
        for (                                              /* i is unmodified
                                                            */ ; i >= 0; i--)
        {
            Utils_queDelete(&pMbx->ackQue[i]);
        }
        Utils_queDelete(&pMbx->recvQue);
    }

    return status;
}

Int32 Utils_mbxDelete(Utils_MbxHndl * pMbx)
{
    Int32 status = 0;
    Int i;

    /* delete queues */

    status |= Utils_queDelete(&pMbx->recvQue);
    for (i = 0; i < UTILS_MBX_ACK_QUE_CNT_MAX; i++)
    {
        status |= Utils_queDelete(&(pMbx->ackQue[i]));
    }
    return status;
}

Int32 Utils_mbxSendCmd(Utils_MbxHndl * pTo, UInt32 cmd)
{
    Utils_MsgHndl *pSentMsg;
    Int32 retVal = 0;

    if (pTo == NULL)
        return -1;

    /* alloc message */
    pSentMsg = Utils_mbxAllocMsg(BIOS_NO_WAIT);
    if (pSentMsg == NULL)
        return -1;

    /* set message fields */
    pSentMsg->pFrom = NULL;
    pSentMsg->flags = 0;
    pSentMsg->cmd = cmd;
    pSentMsg->result = 0;
    pSentMsg->pPrm = NULL;
    pSentMsg->ackQue = NULL;

    /* send message */
    retVal = Utils_quePut(&pTo->recvQue, pSentMsg, BIOS_NO_WAIT);

    if (retVal != 0)
    {
        retVal |= Utils_mbxFreeMsg(pSentMsg, BIOS_NO_WAIT);
    }

    return retVal;
}

Int32 Utils_mbxSendMsg(Utils_MbxHndl * pFrom,
                       Utils_MbxHndl * pTo,
                       UInt32 cmd, Void * pPrm, UInt32 msgFlags)
{
    Utils_MsgHndl *pSentMsg, *pRcvMsg;
    Bool waitAck;
    Int32 retVal = 0;
    Utils_QueHandle *ackQue;
    Int i;
    UInt32 cookie;
    UInt32 ackQIdx = 0;

    if (pTo == NULL)
        return -1;

    /* set ACK que */
    if (pFrom == NULL)
    {
        /* sender mailbox not specified by user */
        if (msgFlags & UTILS_MBX_FLAG_WAIT_ACK)
        {
            /* ERROR: if sender mail box is NULL, then cannot wait for ACK */
            return -1;
        }
        ackQue = NULL;
    }
    else
    {
        ackQue = NULL;
        cookie = Hwi_disable();
        for (i = 0; i < UTILS_MBX_ACK_QUE_CNT_MAX; i++)
        {
            if (pFrom->ackQueInUse[i] == FALSE)
            {
                /* sender mail box */
                ackQue = &(pFrom->ackQue[i]);
                pFrom->ackQueInUse[i] = TRUE;
                break;
            }
        }
        Hwi_restore(cookie);
        if (i == UTILS_MBX_ACK_QUE_CNT_MAX)
        {
            return -1;
        }
        UTILS_assert(ackQue != NULL);
    }

    /* alloc message */
    pSentMsg = Utils_mbxAllocMsg(BIOS_WAIT_FOREVER);
    if (pSentMsg == NULL)
        return -1;

    /* set message fields */
    pSentMsg->pFrom = pFrom;
    pSentMsg->flags = msgFlags;
    pSentMsg->cmd = cmd;
    pSentMsg->result = 0;
    pSentMsg->pPrm = pPrm;
    pSentMsg->ackQue = ackQue;

    /* send message */
    retVal = Utils_quePut(&pTo->recvQue, pSentMsg, BIOS_WAIT_FOREVER);

    if (retVal != 0)
        return retVal;

    if ((msgFlags & UTILS_MBX_FLAG_WAIT_ACK) && ackQue != NULL)
    {
        /* need to wait for ACK */
        waitAck = TRUE;

        do
        {
            /* wait for ACK */
            retVal = Utils_queGet(ackQue,
                                  (Ptr *) & pRcvMsg, 1, BIOS_WAIT_FOREVER);
            if (retVal != 0)
                return retVal;

            if (pRcvMsg == pSentMsg)
            {
                /* ACK received for sent MSG */
                waitAck = FALSE;

                /* copy ACK status to return value */
                retVal = pRcvMsg->result;
            }                                              /* else ACK
                                                            * received for
                                                            * some other
                                                            * message */
            else
            {
                UTILS_warn
                    ("MBX:Received unexpected ack msg. Expected:%p,Received:%p",
                     pSentMsg, pRcvMsg);
            }

            /* free message */
            retVal |= Utils_mbxFreeMsg(pRcvMsg, BIOS_WAIT_FOREVER);

        } while (waitAck);
    }
    if (ackQue)
    {
        cookie = Hwi_disable();
        ackQIdx = ackQue - &(pFrom->ackQue[0]);
        UTILS_assert((ackQIdx < UTILS_MBX_ACK_QUE_CNT_MAX)
                     && (pFrom->ackQueInUse[ackQIdx] == TRUE));
        pFrom->ackQueInUse[ackQIdx] = FALSE;
        Hwi_restore(cookie);
    }

    return retVal;
}

Int32 Utils_mbxRecvMsg(Utils_MbxHndl * pMbxHndl, Utils_MsgHndl ** pMsg,
                       UInt32 timeout)
{
    Int32 retVal;

    /* wait for message to arrive */
    retVal = Utils_queGet(&pMbxHndl->recvQue, (Ptr *) pMsg, 1, timeout);

    return retVal;
}

Int32 Utils_mbxPeekMsg(Utils_MbxHndl * pMbxHndl, Utils_MsgHndl ** pMsg)
{
    Int32 retVal;

    /* wait for message to arrive */
    retVal = Utils_quePeek(&pMbxHndl->recvQue, (Ptr *) pMsg);

    return retVal;
}

Int32 Utils_mbxAckOrFreeMsg(Utils_MsgHndl * pMsg, Int32 ackRetVal)
{
    Int32 retVal = 0;

    if (pMsg == NULL)
        return -1;

    /* check ACK flag */
    if (pMsg->flags & UTILS_MBX_FLAG_WAIT_ACK)
    {
        /* ACK flag is set */

        /* Set ACK status */
        pMsg->result = ackRetVal;

        /* Send ACK to sender */
        if (pMsg->pFrom == NULL)
        {
            retVal = -1;
            retVal |= Utils_mbxFreeMsg(pMsg, BIOS_WAIT_FOREVER);
            return retVal;
        }

        retVal = Utils_quePut(pMsg->ackQue, pMsg, BIOS_WAIT_FOREVER);
    }
    else
    {
        /* ACK flag is not set */

        /* free message */
        retVal = Utils_mbxFreeMsg(pMsg, BIOS_WAIT_FOREVER);
    }

    return retVal;
}

Int32 Utils_mbxWaitCmd(Utils_MbxHndl * pMbxHndl, Utils_MsgHndl ** pMsg,
                       UInt32 waitCmd)
{
    Int32 status = 0;
    Utils_MsgHndl *pRcvMsg;

    while (1)
    {
        /* wait for message */
        status = Utils_mbxRecvMsg(pMbxHndl, &pRcvMsg, BIOS_WAIT_FOREVER);
        if (status != 0)
            return status;

        /* is message command ID same as expected command ID */
        if (Utils_msgGetCmd(pRcvMsg) == waitCmd)
            break;                                         /* yes, exit loop */

        /* no, ACK or free received message */
        status = Utils_mbxAckOrFreeMsg(pRcvMsg, 0);
        if (status != 0)
            return status;
    }

    if (pMsg == NULL)
    {
        /* user does not want to examine the message, so free it here */
        status = Utils_mbxAckOrFreeMsg(pRcvMsg, 0);
    }
    else
    {
        /* user wants to examine the message to return it to user */
        *pMsg = pRcvMsg;
    }

    return status;
}
