/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#ifndef _SYSTEM_COMMON_H_
#define _SYSTEM_COMMON_H_

/**
    \ingroup SYSTEM_LINK_API

    @{
*/

/**
    \file system_common.h
    \brief Common system data structure's and constants

    Common defines, data structure's shared across processor's
    Typically user does not use these in his application directly
*/

#include <stdio.h>
#include <ti/ipc/Ipc.h>
#include <ti/ipc/ListMP.h>
#include <ti/ipc/HeapMemMP.h>
#include <ti/ipc/MultiProc.h>
#include <ti/ipc/Notify.h>
#include <ti/ipc/MessageQ.h>

// #include <ti/psp/vps/vps_displayCtrl.h>
// #include <ti/psp/vps/vps_cfgCsc.h>


/* Frequency values of various cores set explicitly during system init */
#if defined(TI_814X_BUILD)
#define SYSTEM_M3VPSS_FREQ         (240*1000*1000)
#define SYSTEM_M3VIDEO_FREQ        (240*1000*1000)
#define SYSTEM_DSP_FREQ            (750*1000*1000)
#endif

#if defined(TI_8107_BUILD)
#define SYSTEM_M3VPSS_FREQ         (280*1000*1000)
#define SYSTEM_M3VIDEO_FREQ        (SYSTEM_M3VPSS_FREQ)
#define SYSTEM_DSP_FREQ            (0)
#endif

#if defined(TI_816X_BUILD)
#define SYSTEM_M3VPSS_FREQ         (280*1000*1000)
#define SYSTEM_M3VIDEO_FREQ        (280*1000*1000)
#define SYSTEM_DSP_FREQ            (800*1000*1000)
#endif

#define SYSTEM_CMD_CREATE          (0x00000000)
#define SYSTEM_CMD_START           (0x00000001)
#define SYSTEM_CMD_STOP            (0x00000002)
#define SYSTEM_CMD_DELETE          (0x00000003)
#define SYSTEM_CMD_NEW_DATA        (0x00000004)
#define SYSTEM_CMD_GET_INFO        (0x00000005)

#define SYSTEM_IPC_M3_MAX_LIST_ELEM    (1000)

#define SYSTEM_IPC_BITS_MAX_LIST_ELEM  (SYSTEM_IPC_M3_MAX_LIST_ELEM)

#define SYSTEM_IPC_FRAMES_MAX_LIST_ELEM  (SYSTEM_IPC_M3_MAX_LIST_ELEM)
#define SYSTEM_IPC_CMD_RELEASE_FRAMES  (0x2000)
#define SYSTEM_IPC_CMD_SLAVE_CORE_EXCEPTION (0x2001)

#define SYSTEM_IPC_NOTIFY_LINE_ID   (0)
#define SYSTEM_IPC_NOTIFY_EVENT_ID  (15)

#define SYSTEM_IPC_MSGQ_HEAP_NAME               "IPC_MSGQ_MSG_HEAP"
#define SYSTEM_IPC_MSGQ_HEAP_SIZE               (512*1024)
#define SYSTEM_IPC_MSGQ_MSG_SIZE_MAX            (64*1024)
#define SYSTEM_IPC_MSGQ_HEAP                    (0)
#define SYSTEM_IPC_MSGQ_MSG_PAYLOAD_PTR(msg)    (Void*)((UInt32)(msg)+sizeof(SystemIpcMsgQ_Msg))
#define SYSTEM_IPC_MSGQ_OWNER_PROC_ID           SYSTEM_PROC_M3VPSS

#define SYSTEM_IPC_SR_NON_CACHED_DEFAULT     (0)
#define SYSTEM_IPC_SR_M3_LIST_MP             (0)
#define SYSTEM_IPC_SR_CACHED                 (1)
#define SYSTEM_IPC_SR_VIDEO_FRAME            (2)
#define SYSTEM_IPC_SR_AVSYNC                 (0)

/* Dual mode timer to be used in the system */
#define SYSTEM_DMTIMER_ID                    (2)

#ifdef TI816X_2G_DDR
#define SYSTEM_DDR_BASE_ADDR (0x80000000)
#endif

typedef struct {

    MessageQ_MsgHeader msgHead;
    UInt32 linkId;
    UInt32 cmd;
    UInt32 prmSize;
    UInt32 waitAck;
    UInt32 status;

} SystemIpcMsgQ_Msg;

static inline Int32 System_ipcGetMsgQName(UInt32 procId, char *msgQName, char *ackMsgQName)
{
    UTILS_assert(procId< SYSTEM_PROC_MAX);

    sprintf(msgQName, "%s_MSGQ", MultiProc_getName(procId));
    sprintf(ackMsgQName, "%s_ACK_MSGQ", MultiProc_getName(procId));

    return 0;
}

static inline Int32 System_ipcGetListMPName(UInt32 linkId, char *listMPOutName, char *listMPInName)
{
    UInt32 procId;

    procId = SYSTEM_GET_PROC_ID(linkId);
    linkId = SYSTEM_GET_LINK_ID(linkId);

    UTILS_assert(procId< SYSTEM_PROC_MAX);
    UTILS_assert(linkId< SYSTEM_LINK_ID_MAX);

    sprintf(listMPOutName, "%s_IPC_OUT_%d", MultiProc_getName(procId), linkId);
    sprintf(listMPInName , "%s_IPC_IN_%d", MultiProc_getName(procId), linkId);

    return 0;
}

#endif

/* @} */
