/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
    \ingroup UTILS_API
    \defgroup UTILS_MBX_API Message exchange API

    APIs from this file are used to exchange messages between two tasks in the
    links and chains examples.

    A message consists of a 32-bit command and optional 32-bit parameter value.

    The 32-bit command is defined by the user.

    The 32-bit parameter could inturn point to a bigger data structure as defined
    by user.

    The APIs allow a user to send a message and wait for ACK before proceeding
    further.

    Internally message passing is implemented using queue's

    @{
*/

/**
    \file utils_mbx.h
    \brief Message exchange API
*/

#ifndef _UTILS_MBX_H_
#define _UTILS_MBX_H_

#include <mcfw/src_bios6/utils/utils_que.h>

/** \brief Maximum receive que length */
#define UTILS_MBX_RECV_QUE_LEN_MAX (1024)

/** \brief Maximum acknowledgement que length */
#define UTILS_MBX_ACK_QUE_LEN_MAX  (2)

/** \brief Message flag: wait for ACK when this flag is set */
#define UTILS_MBX_FLAG_WAIT_ACK    (0x1)

/** \brief Maximum number of ack queues.This allows multiple tasks to wait
  *        on different ack msgs
  */
#define UTILS_MBX_ACK_QUE_CNT_MAX  (4)

/** \brief Get 32-bit command from message pointer */
#define Utils_msgGetCmd(pMsg)      ((pMsg)->cmd)

/** \brief Get 32-bit parameter pointer from message pointer */
#define Utils_msgGetPrm(pMsg)      ((pMsg)->pPrm)

/**
  \brief Mailbox handle
*/
typedef struct {
    Utils_QueHandle recvQue;
  /**< message receive queue  */

    Utils_QueHandle ackQue[UTILS_MBX_ACK_QUE_CNT_MAX];
  /**< ack message receive queue  */

    Bool ackQueInUse[UTILS_MBX_ACK_QUE_CNT_MAX];
  /**< flag indicating whether this que is expecting ack */

    Ptr memRecvQue[UTILS_MBX_RECV_QUE_LEN_MAX];
  /**< memory for receive queue */

    Ptr memAckQue[UTILS_MBX_ACK_QUE_CNT_MAX][UTILS_MBX_ACK_QUE_LEN_MAX];
  /**< memory for ack queue */

} Utils_MbxHndl;

/**
  \brief Message structure
*/
typedef struct {
    Utils_MbxHndl *pFrom;
  /**< sender mailbox */

    Int32 result;
  /**< result to be sent as part of ACK */

    Void *pPrm;
  /**< parameters sent by sender  */

    UInt32 cmd;
  /**< command sent by sender */

    UInt32 flags;
  /**< message flags set by sender */

    Utils_QueHandle *ackQue;
  /**< ack queue to which msg will be acknowledged */

} Utils_MsgHndl;

/**
  \brief One-time system init for mailbox subsystem
*/
Int32 Utils_mbxInit();

/**
  \brief Free's resources allocated during Utils_mbxInit()
*/
Int32 Utils_mbxDeInit();

/**
  \brief Create's a message box and related resources

  \param pHandle    [OUT] Created handle

  \return FVID2_SOK on success else failure
*/
Int32 Utils_mbxCreate(Utils_MbxHndl * pHandle);

/**
  \brief Free's resources allocated during Utils_mbxCreate()

  \param pHandle    [IN] Mail box handle

  \return FVID2_SOK on success else failure
*/
Int32 Utils_mbxDelete(Utils_MbxHndl * pHandle);

/**
  \brief Send message from one mailbox to another mailbox

  When 'msgFlags' is UTILS_MBX_FLAG_WAIT_ACK,
    the function waits until an ack is received

  When 'msgFlags' is 0,
    the function returns after message is sent to the receiver

  User can use 'pPrm' to pass a parameter pointer to the receiver.
  Its upto user to manage the memory for this parameter pointer.

  'cmd' can be any 32-bit value that is sent to the receiver

  When UTILS_MBX_FLAG_WAIT_ACK is set,
    return value is the value sent via 'result' by the
    receiver when the receiver calls Utils_mbxAckOrFreeMsg()

  When UTILS_MBX_FLAG_WAIT_ACK is not set,
    return value is message send status

  \param pFrom    [IN] Sender Mail box handle
  \param pTo      [IN] Receiver mail box handle
  \param cmd      [IN] 32-bit command
  \param pPrm     [IN] 32-bit parameter pointer
  \param msgFlags [IN] UTILS_MBX_FLAG_xxxx

  \return FVID2_SOK on success else failure

*/
Int32 Utils_mbxSendMsg(Utils_MbxHndl * pFrom,
                       Utils_MbxHndl * pTo,
                       UInt32 cmd, Void * pPrm, Uint32 msgFlags);

/**
  \brief Send command to another mail box

  Same as Utils_mbxSendMsg() except,
  - it can be called from interrupt context as well as task, SWI context
  - ACK message cannot received

  \param pTo      [IN] Receiver mail box handle

  \return FVID2_EFAIL in case message could not be sent
*/
Int32 Utils_mbxSendCmd(Utils_MbxHndl * pTo, UInt32 cmd);

/**
  \brief Waits for a message to arrive

  When 'timeout' is BIOS_WAIT_FOREVER, it waits until atleast one message arrives

  When 'timeout' is BIOS_NO_WAIT, it just checks for any available message.
    if no message is received then it return's with error

  User MUST call Utils_mbxAckOrFreeMsg() for every received message,
  else message will not be free'ed and there will be memory leak.

  \param pMbx       [IN] Receiver mail box
  \param pMsg       [OUT] received message
  \param timeout    [IN] BIOS_WAIT_FOREVER or BIOS_NO_WAIT

  \return FVID2_SOK on success else failure
*/
Int32 Utils_mbxRecvMsg(Utils_MbxHndl * pMbx, Utils_MsgHndl ** pMsg,
                       UInt32 timeout);

Int32 Utils_mbxPeekMsg(Utils_MbxHndl * pMbx, Utils_MsgHndl ** pMsg);

/**
  \brief Acks or frees a message depending on flags set in the message

  if UTILS_MBX_FLAG_WAIT_ACK is set,
    then an ack message is sent to the sender

  if UTILS_MBX_FLAG_WAIT_ACK is not set,
    then it frees the memory associated with this message

  User MUST call this API for every message received using
    Utils_mbxRecvMsg() or Utils_mbxWaitCmd()
  else message will not be free'ed and there will be memory leak.

  \param pMsg       [IN] Message to ACK'ed or free'ed
  \param result     [IN] return code that is sent to the sender if an ack message
                         is sent to the sender

  \return FVID2_SOK on success else failure
*/
Int32 Utils_mbxAckOrFreeMsg(Utils_MsgHndl * pMsg, Int32 result);

/**
  \brief Waits until command of value 'cmdToWait' is received

  if 'pMsg' is NULL it frees the msg internally when it is received and returns

  if 'pMsg' is NOT NULL then it returns the received message to user. User
    needs to free the received message using Utils_mbxAckOrFreeMsg()

  \param pMbx       [IN] Receiver mail box
  \param pMsg       [OUT] received message
  \param cmdToWait  [IN] command to wait for

  \return FVID2_SOK on success else failure
*/
Int32 Utils_mbxWaitCmd(Utils_MbxHndl * pMbx, Utils_MsgHndl ** pMsg,
                       UInt32 cmdToWait);

UInt32 Utils_mbxGetFreeMsgCount();

#endif

/* @} */
