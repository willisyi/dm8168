/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
    \ingroup AvSync library
*/

/**
    \file avsync_m3vpss.h
    \brief m3 side structures & functions
*/

#ifndef _AVSYNC_M3VPSS_H_
#define _AVSYNC_M3VPSS_H_

#include <mcfw/src_bios6/links_m3vpss/system/system_priv_m3vpss.h>      // for  FVID2_Frame
#include <mcfw/interfaces/link_api/avsync_internal.h>
#include <mcfw/src_bios6/utils/utils.h>

/* For Wall Timer */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/gates/GateMutexPri.h>
#include <ti/sysbios/hal/Timer.h>

/* IPC */
#include <ti/ipc/SharedRegion.h>
#include <ti/ipc/GateMP.h>


/* TIMESTAMP 64bit */
#include <xdc/runtime/Timestamp.h>
#include <xdc/runtime/Types.h>

#include <ti/ipc/NameServer.h>
#include <mcfw/interfaces/link_api/avsync_rtos.h>

#define  AVSYNC_VIDQUE_STATE_INIT                            (0)
#define  AVSYNC_VIDQUE_STATE_CREATED                         (1 << 0)
#define  AVSYNC_VIDQUE_STATE_FIRSTFRMRECEIVED                (1 << 1)
#define  AVSYNC_VIDQUE_STATE_FIRSTFRMPLAYED                  (1 << 2)


#define  AVSYNC_VIDQUE_INIT_STATE(state)                     (state = AVSYNC_VIDQUE_STATE_INIT)
#define  AVSYNC_VIDQUE_SET_STATE(curState,newState)          ((curState) |= (newState))
#define  AVSYNC_VIDQUE_CHECK_STATE(curState,checkState)      ((curState) & (checkState))

#define  AVSYNC_CRITICAL_BEGIN()                          do                                             \
                                                          {                                              \
                                                              IArg key;                                  \
                                                                                                         \
                                                              key = GateMP_enter(gAvsyncLink_obj.gate);

#define  AVSYNC_CRITICAL_END()                                GateMP_leave(gAvsyncLink_obj.gate,key);    \
                                                          }  while(0)

#define  AVSYNC_CFG_MUTEX_ACQUIRE(displayID)              do                                                     \
                                                          {                                                      \
                                                              IArg mutexKey;                                     \
                                                                                                                 \
                                                              UTILS_assert(displayID < AVSYNC_MAX_NUM_DISPLAYS); \
                                                              mutexKey = ti_sysbios_gates_GateMutexPri_enter(    \
                                                              gAvsyncLink_obj.cfgMutex[displayID]);

#define  AVSYNC_CFG_MUTEX_RELEASE(displayID)                  ti_sysbios_gates_GateMutexPri_leave(               \
                                                              gAvsyncLink_obj.cfgMutex[displayID],mutexKey);     \
                                                          }  while(0)


#define AVSYNC_GET_HW_TIME()                              (Utils_getCurTimeInMsec())
//#define AVSYNC_GET_HW_TIME()                              Utils_dmTimerGetCurTimeInMsec(SYSTEM_DMTIMER_ID);

typedef struct AvsyncLink_WallTimerUpdateObj
{
    Clock_Struct clkStruct;
    Clock_Handle clkHandle;
    Bool         clkStarted;
} AvsyncLink_WallTimerUpdateObj;

typedef struct AvsyncLink_Obj
{
    /* Capture link task */
    Utils_TskHndl     tsk;
    Bool              initDone;
    NameServer_Handle nsHandle;
    Ptr               nsKey;
    GateMP_Handle     gate;
    Ptr               gateMPSrMemPtr;
    UInt32            gateMPSrMemSize;
    SharedRegion_SRPtr srObjSrPtr;
    Avsync_SharedObj  *srObj;
    AvsyncLink_WallTimerUpdateObj wallTimerUpdateObj;
    AvsyncLink_DisplayObj displayObj[AVSYNC_MAX_NUM_DISPLAYS];
    UInt32                displayId2vidSyncLinkMap[AVSYNC_MAX_NUM_DISPLAYS];
    UInt32                displayId2audDevMap[AVSYNC_MAX_NUM_DISPLAYS];
    UInt32                syncMasterChNum[AVSYNC_MAX_NUM_DISPLAYS];
    ti_sysbios_gates_GateMutexPri_Handle   cfgMutex[AVSYNC_MAX_NUM_DISPLAYS];
    ti_sysbios_gates_GateMutexPri_Struct   cfgMutexMem[AVSYNC_MAX_NUM_DISPLAYS];
} AvsyncLink_Obj;



#endif

/* @} */
