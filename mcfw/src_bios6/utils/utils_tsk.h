/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
    \ingroup UTILS_API
    \defgroup UTILS_TSK_API Task wrapper APIs

    APIs in this file couple a BIOS Task with a mailbox in order to allow
    application to implement a state machine kind of logic, where in state
    change happens based on received message's

    @{
*/

/**
    \file utils_tsk.h
    \brief Task wrapper API
*/

#ifndef _UTILS_TSK_H_
#define _UTILS_TSK_H_

#include <mcfw/src_bios6/utils/utils_mbx.h>
#include <mcfw/src_bios6/utils/utils_prf.h>

struct Utils_TskHndl;

/**
    \brief Task main function

    This function is called when a message is received by the
    task.

    \param pHndl [OUT] Task handle
    \param pMsg  [OUT] Received message

*/
typedef Void(*Utils_TskFuncMain) (struct Utils_TskHndl * pHndl,
                                  Utils_MsgHndl * pMsg);

/**
    \brief Task handle
*/
typedef struct Utils_TskHndl {
    Task_Handle tsk;
    /**< BIOS Task handle */

    Task_Params tskParams;
    /**< BIOS Task create params */

    Utils_MbxHndl mbx;
    /**< Mail box associated with this task */

    UInt8 *stackAddr;
    /**< Task stack address */

    UInt32 stackSize;
    /**< Task stack size */

    UInt32 tskPri;
    /**< Task priority as defined by BIOS */

    Utils_TskFuncMain funcMain;
    /**< Task main,

        Note, this is different from BIOS Task, since this function
        is entered ONLY when a message is received.
      */

    Ptr appData;
    /** Application specific data */

} Utils_TskHndl;

/**
    \brief Create a task

    \param pHndl        [OUT] Task handle
    \param funcMain     [IN]  Task main,
                              Note, this is different from BIOS Task, since
                              this function
                              is entered ONLY when a message is received.
    \param tskPri       [IN]  Task priority as defined by BIOS
    \param stackAddr    [IN]  Task stack address
    \param stackSize    [IN]  Task stack size
    \param appData      [IN]  Application specific data
    \param tskName      [IN]  Task name

    \return FVID2_SOK on success else failure
*/
Int32 Utils_tskCreate(Utils_TskHndl * pHndl,
                      Utils_TskFuncMain funcMain,
                      UInt32 tskPri,
                      UInt8 * stackAddr,
                      UInt32 stackSize, Ptr appData, char *tskName);

/**
    \brief Delete a task

    \param pHndl        [OUT] Task handle

    \return FVID2_SOK on success else failure
*/
Int32 Utils_tskDelete(Utils_TskHndl * pHndl);

/**
    \brief change priority of a task

    \param pHndl        [OUT] Task handle

    \return FVID2_SOK on success else failure
*/
Int32 Utils_tskSetPri(Utils_TskHndl * pHndl, UInt32 newPri);



/**
    \brief Send message from one task to another task

    Refer to Utils_mbxSendMsg() for details
*/
static inline Int32
Utils_tskSendMsg(Utils_TskHndl * pFrom,
                 Utils_TskHndl * pTo, UInt32 cmd, Void * pPrm, UInt32 msgFlags)
{
    return Utils_mbxSendMsg(&pFrom->mbx, &pTo->mbx, cmd, pPrm, msgFlags);
}

/**
    \brief Send 32-bit command to another task

    Refer to Utils_mbxSendCmd() for details
*/
static inline Int32 Utils_tskSendCmd(Utils_TskHndl * pTo, UInt32 cmd)
{
    return Utils_mbxSendCmd(&pTo->mbx, cmd);
}

/**
    \brief Wait for a message to arrive

    Refer to Utils_mbxRecvMsg() for details
*/
static inline Int32
Utils_tskRecvMsg(Utils_TskHndl * pHndl, Utils_MsgHndl ** pMsg, UInt32 timeout)
{
    return Utils_mbxRecvMsg(&pHndl->mbx, pMsg, timeout);
}

/**
    \brief ACK or free received message

    Refer to Utils_mbxAckOrFreeMsg() for details
*/
static inline Int32 Utils_tskAckOrFreeMsg(Utils_MsgHndl * pMsg, Int32 result)
{
    return Utils_mbxAckOrFreeMsg(pMsg, result);
}

/**
    \brief Wait until user specified command is received

    Refer to Utils_mbxWaitCmd() for details
*/
static inline Int32
Utils_tskWaitCmd(Utils_TskHndl * pHndl, Utils_MsgHndl ** pMsg, UInt32 cmdToWait)
{
    return Utils_mbxWaitCmd(&pHndl->mbx, pMsg, cmdToWait);
}

/**
    \brief Peek for a message

    Refer to Utils_mbxRecvMsg() for details
*/
static inline Int32
Utils_tskPeekMsg(Utils_TskHndl * pHndl, Utils_MsgHndl ** pMsg)
{
    return Utils_mbxPeekMsg(&pHndl->mbx, pMsg);
}

Int32 Utils_tskFlushMsg(Utils_TskHndl * pHndl, UInt32 *flushCmdId, UInt32 numCmds);


#endif

/* @} */
