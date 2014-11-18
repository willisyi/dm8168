/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#ifndef _SYSTEM_PRIV_COMMON_H_
#define _SYSTEM_PRIV_COMMON_H_

#include <osa_tsk.h>
#include <osa_mutex.h>

#include <ti/syslink/utils/IHeap.h>
#include <ti/syslink/utils/Memory.h>
#include <ti/syslink/utils/Cache.h>

#include <mcfw/interfaces/link_api/system.h>
#include <mcfw/interfaces/link_api/system_common.h>

#define SYSTEM_TSK_PRI                   (2)
#define IPC_LINK_TSK_PRI                 (2)
#define SYSTEM_MSGQ_TSK_PRI              (2)
#define SYSTEM_MSGQ_TSK_PRI_HIGH         (13)

#define SYSTEM_DEFAULT_TSK_STACK_SIZE    (0) /* 0 means system default will be used */
#define SYSTEM_TSK_STACK_SIZE            (SYSTEM_DEFAULT_TSK_STACK_SIZE)
#define IPC_LINK_TSK_STACK_SIZE          (SYSTEM_DEFAULT_TSK_STACK_SIZE)
#define SYSTEM_MSGQ_TSK_STACK_SIZE       (SYSTEM_DEFAULT_TSK_STACK_SIZE)


typedef Int32 (*System_GetLinkInfoCb)        (OSA_TskHndl *pTsk,
                                                System_LinkInfo *info);


/**
 * \brief LINK Instance Info
 *  Each of the LINKs are expected to register with "system" with the following
 *  information.
 *  Using these links the system would form a chain with mutiple LINKs
 */
typedef struct
{
    OSA_TskHndl                *pTsk;

    System_GetLinkInfoCb            getLinkInfo;
    /**<  Function that returns the LINKs output channel configurations */
} System_LinkObj;

/**
 * \brief System Task Descriptor
 *
 */
typedef struct
{
    OSA_MbxHndl             mbx;
    System_LinkObj          linkObj[SYSTEM_LINK_ID_MAX];

} System_CommonObj;


extern System_CommonObj gSystem_objCommon;

Int32 System_sendLinkCmd(UInt32 linkId, UInt32 cmd);

Int32 System_registerLink(UInt32 linkId, System_LinkObj *pTskObj);

OSA_TskHndl *System_getLinkTskHndl(UInt32 linkId);

UInt32 System_getSelfProcId();

Int32 System_linkControl_local(UInt32 linkId, UInt32 cmd, Void *pPrm, UInt32 prmSize, Bool waitAck);
Int32 System_sendLinkCmd_local(UInt32 linkId, UInt32 cmd);
Int32 System_linkGetInfo_local(UInt32 linkId, System_LinkInfo * info);

Int32 System_linkControl_remote(UInt32 linkId, UInt32 cmd, Void *pPrm, UInt32 prmSize, Bool waitAck);

Int32 SystemLink_init();
Int32 SystemLink_deInit();

#endif
