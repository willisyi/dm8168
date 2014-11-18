/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#ifndef _SYSTEM_PRIV_IPC_H_
#define _SYSTEM_PRIV_IPC_H_

#include "system_priv_common.h"


#include <ti/syslink/IpcHost.h>
#include <ti/syslink/ProcMgr.h>
#include <ti/syslink/inc/_MultiProc.h>


typedef Void (*System_ipcNotifyCb)(OSA_TskHndl *pTsk);

typedef struct {

    OSA_MutexHndl    msgQLock;
    OSA_ThrHndl      msgQTask;
    MessageQ_Handle  selfAckMsgQ;
    MessageQ_Handle  selfMsgQ;
    MessageQ_QueueId remoteProcMsgQ[SYSTEM_PROC_MAX];
    HeapMemMP_Handle msgQHeapHndl;
    Ptr              msgQHeapBaseAddr;

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

Void  *System_ipcMsgQTaskMain(Void *arg);
Int32 System_ipcMsgQSendMsg(UInt32 linkId, UInt32 cmd, Void *pPrm, UInt32 prmSize, Bool waitAck, UInt32 timeout);

Int32 System_ipcRegisterNotifyCb(UInt32 linkId, System_ipcNotifyCb notifyCb);
Int32 System_ipcSendNotify(UInt32 linkId);

Int32 System_ipcListMPReset(ListMP_Handle pOutHndl, ListMP_Handle pInHndl);
Int32 System_ipcListMPCreate(UInt32 regionId, UInt32 linkId, ListMP_Handle *pOutHndl, ListMP_Handle *pInHndl,
                             GateMP_Handle * pOutHndleGate, GateMP_Handle * pInHndleGate);
Int32 System_ipcListMPOpen(UInt32 linkId, ListMP_Handle *pOutHndl, ListMP_Handle *pInHndl);
Int32 System_ipcListMPClose(ListMP_Handle *pOutHndl, ListMP_Handle *pInHndl);
Int32 System_ipcListMPDelete(ListMP_Handle *pOutHndl, ListMP_Handle *pInHndl,
                             GateMP_Handle * pOutHndleGate, GateMP_Handle * pInHndleGate);

UInt32 System_ipcListMPAllocListElemMem(UInt32 regionId, UInt32 size);
Int32 System_ipcListMPFreeListElemMem(UInt32 regionId, UInt32 shAddr, UInt32 size);

Int32 System_ipcGetSlaveSymbolAddress(char *symbolName,
                                UInt16 multiProcId,
                                Ptr *symAddressPtr);
Int32 System_ipcCopySlaveCoreSymbolContents(char *symbolName,
                                      UInt16 multiProcId,
                                      Ptr  dstPtr,
                                      UInt32 copySize);

#endif
