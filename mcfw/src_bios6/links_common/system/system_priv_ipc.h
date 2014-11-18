/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#ifndef _SYSTEM_PRIV_IPC_H_
#define _SYSTEM_PRIV_IPC_H_

#include <xdc/std.h>
#include <ti/ipc/Ipc.h>
#include <ti/ipc/ListMP.h>
#include <ti/ipc/HeapMemMP.h>
#include <ti/ipc/MultiProc.h>
#include <ti/ipc/Notify.h>
#include <ti/ipc/MessageQ.h>
#include <ti/ipc/GateMP.h>
#include <mcfw/src_bios6/utils/utils_trace.h>
#include <mcfw/src_bios6/links_common/system/system_priv_common.h>
#include <mcfw/interfaces/link_api/ipcLink.h>
#include <mcfw/interfaces/link_api/vidbitstream.h>

typedef struct {

    ListMP_Elem listElem;
    FVID2_Frame *pFrame;

} SystemIpcM3_ListElem;

typedef Void(*System_ipcNotifyCb) (Utils_TskHndl * pTsk);

typedef struct {

    Semaphore_Handle msgQLock;
    Task_Handle msgQTask;
    MessageQ_Handle selfAckMsgQ;
    MessageQ_Handle selfMsgQ;
    MessageQ_QueueId remoteProcMsgQ[SYSTEM_PROC_MAX];
    Bool isOpenRemoteProcMsgQ[SYSTEM_PROC_MAX];
    HeapMemMP_Handle msgQHeapHndl;
    Ptr msgQHeapBaseAddr;

    System_ipcNotifyCb notifyCb[SYSTEM_LINK_ID_MAX];

} System_IpcObj;

extern System_IpcObj gSystem_ipcObj;
extern UInt32 gSystem_ipcEnableProcId[];

Int32 System_ipcInit();
Int32 System_ipcDeInit();

Int32 System_ipcNotifyInit();
Int32 System_ipcNotifyDeInit();

Int32 System_ipcMsgQInit();
Int32 System_ipcMsgQDeInit();

Int32 System_ipcMsgQOpen(UInt32 procId);
Void System_ipcMsgQTaskMain(UArg arg0, UArg arg1);
Int32 System_ipcMsgQSendMsg(UInt32 linkId, UInt32 cmd, Void * pPrm,
                            UInt32 prmSize, Bool waitAck);

Void System_ipcStart();
Void System_ipcStop();


Int32 System_ipcRegisterNotifyCb(UInt32 linkId, System_ipcNotifyCb notifyCb);
Int32 System_ipcSendNotify(UInt32 linkId);

Int32 System_ipcListMPReset(ListMP_Handle pOutHndl, ListMP_Handle pInHndl);
Int32 System_ipcListMPCreate(UInt32 regionId, UInt32 linkId,
                             ListMP_Handle * pOutHndl, ListMP_Handle * pInHndl,
                             GateMP_Handle * pOutHndleGate, GateMP_Handle * pInHndleGate);
Int32 System_ipcListMPOpen(UInt32 linkId, ListMP_Handle * pOutHndl,
                           ListMP_Handle * pInHndl);
Int32 System_ipcListMPClose(ListMP_Handle * pOutHndl, ListMP_Handle * pInHndl);
Int32 System_ipcListMPDelete(ListMP_Handle * pOutHndl, ListMP_Handle * pInHndl,
                             GateMP_Handle * pOutHndleGate, GateMP_Handle * pInHndleGate);

UInt32 System_ipcListMPAllocListElemMem(UInt32 regionId, UInt32 size);
Int32 System_ipcListMPFreeListElemMem(UInt32 regionId, UInt32 shAddr,
                                      UInt32 size);

#endif
