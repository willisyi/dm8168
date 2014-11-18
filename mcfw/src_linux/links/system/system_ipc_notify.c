/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include "system_priv_common.h"
#include "system_priv_ipc.h"

Int32 System_ipcRegisterNotifyCb(UInt32 linkId, System_ipcNotifyCb notifyCb)
{
    linkId = SYSTEM_GET_LINK_ID(linkId);
    UTILS_assert(linkId< SYSTEM_LINK_ID_MAX);

    gSystem_ipcObj.notifyCb[linkId] = notifyCb;

    return OSA_SOK;
}

Void System_ipcNotifyHandler(UInt16 procId, UInt16 lineId,
                UInt32 eventId, UArg arg, UInt32 payload)
{
    UInt32 linkId, linkProcId;
    OSA_TskHndl *pTsk;

    if(lineId!=SYSTEM_IPC_NOTIFY_LINE_ID)
        return;

    if(eventId!=SYSTEM_IPC_NOTIFY_EVENT_ID)
        return;

    linkProcId = SYSTEM_GET_PROC_ID(payload);
    linkId     = SYSTEM_GET_LINK_ID(payload);

    if(linkId>=SYSTEM_LINK_ID_MAX)
        return;

    UTILS_assert(
        System_getSelfProcId() ==
        linkProcId
        );

    if(gSystem_ipcObj.notifyCb[linkId])
    {
        pTsk = System_getLinkTskHndl(linkId);

        gSystem_ipcObj.notifyCb[linkId](pTsk);
    }
}

Int32 System_ipcSendNotify(UInt32 linkId)
{
    Int32 status;
    UInt32 procId = SYSTEM_GET_PROC_ID(linkId);

    UTILS_assert(procId< SYSTEM_PROC_MAX);

    status = Notify_sendEvent(
                procId,
                SYSTEM_IPC_NOTIFY_LINE_ID,
                SYSTEM_IPC_NOTIFY_EVENT_ID,
                linkId,
                TRUE
                );

    if(status!=Notify_S_SUCCESS)
    {
        printf(" %u: NOTIFY: Send Event to [%s][%d] failed !!! (status = %d)\n",
            OSA_getCurTimeInMsec(),
            MultiProc_getName(SYSTEM_GET_PROC_ID(linkId)),
            SYSTEM_GET_LINK_ID(linkId),
            status
                );

        UTILS_assert(status==Notify_S_SUCCESS);
    }

    return OSA_SOK;
}

Int32 System_ipcNotifyInit()
{
    UInt32 procId, i;
    Int32 status=OSA_SOK;

    memset(gSystem_ipcObj.notifyCb, 0, sizeof(gSystem_ipcObj.notifyCb));

    i=0;
    while(gSystem_ipcEnableProcId[i]!=SYSTEM_PROC_MAX)
    {
        procId = gSystem_ipcEnableProcId[i];
        if ((procId != System_getSelfProcId()) && (procId != SYSTEM_PROC_INVALID))
        {
            printf(" %u: SYSTEM: Notify register to [%s] line %d, event %d ... \n",
                OSA_getCurTimeInMsec(),
                MultiProc_getName(procId),
                SYSTEM_IPC_NOTIFY_LINE_ID,
                SYSTEM_IPC_NOTIFY_EVENT_ID
                );

            if(Notify_intLineRegistered(procId, SYSTEM_IPC_NOTIFY_LINE_ID)==FALSE)
            {
                UTILS_assert(0);
            }
            if(Notify_eventAvailable(procId, SYSTEM_IPC_NOTIFY_LINE_ID, SYSTEM_IPC_NOTIFY_EVENT_ID)==FALSE)
            {
                UTILS_assert(0);
            }

            status = Notify_registerEvent(
                        procId,
                        SYSTEM_IPC_NOTIFY_LINE_ID,
                        SYSTEM_IPC_NOTIFY_EVENT_ID,
                        System_ipcNotifyHandler,
                        NULL
                      );

            UTILS_assert(status==Notify_S_SUCCESS);
        }
        i++;
    }

    return status;
}

Int32 System_ipcNotifyDeInit()
{
    Int32 status=OSA_SOK;
    UInt32 i, procId;

    i=0;

    while(gSystem_ipcEnableProcId[i]!=SYSTEM_PROC_MAX)
    {
        procId = gSystem_ipcEnableProcId[i];
        if ((procId != System_getSelfProcId()) && (procId != SYSTEM_PROC_INVALID))
        {
            status = Notify_unregisterEvent(
                    procId,
                    SYSTEM_IPC_NOTIFY_LINE_ID,
                    SYSTEM_IPC_NOTIFY_EVENT_ID,
                    System_ipcNotifyHandler,
                    NULL
                    );

            UTILS_assert(status==Notify_S_SUCCESS);
        }
        i++;
    }
    return status;
}

