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
    \file avsync_priv.h
    \brief A8 side structures & functions
*/

#ifndef _AVSYNC_PRIV_H_
#define _AVSYNC_PRIV_H_

#include <osa.h>
#include <osa_que.h>
#include <mcfw/interfaces/ti_media_std.h>
#include <mcfw/interfaces/ti_media_error_def.h>
#include <mcfw/interfaces/link_api/avsync_internal.h>
#include <ti/ipc/NameServer.h>
#include <ti/ipc/SharedRegion.h>
#include <ti/ipc/MultiProc.h>
#include <ti/ipc/GateMP.h>
#include <mcfw/interfaces/link_api/avsync.h>
#include <mcfw/interfaces/link_api/avsync_hlos.h>

#define  AVSYNC_AUDQUE_STATE_INIT                            (0)
#define  AVSYNC_AUDQUE_STATE_CREATED                         (1 << 0)
#define  AVSYNC_AUDQUE_STATE_FIRSTFRMRECEIVED                (1 << 1)
#define  AVSYNC_AUDQUE_STATE_FIRSTFRMPLAYED                  (1 << 2)

#define  AVSYNC_AUDQUE_INIT_STATE(state)                     (state = AVSYNC_AUDQUE_STATE_INIT)
#define  AVSYNC_AUDQUE_SET_STATE(curState,newState)          ((curState) |= (newState))
#define  AVSYNC_AUDQUE_CHECK_STATE(curState,checkState)      ((curState) & (checkState))

#define AVSYNC_PLAYER_STATE_CHECK_RETRY_MS                   (2)
#define AVSYNC_SYNCH_START_CHECK_RETRY_MS                    (2)
#define AVSYNC_AUDIO_GET_RETRY_MS                            (30)

#define  AVSYNC_CRITICAL_BEGIN()                          do                                             \
                                                          {                                              \
                                                              IArg key;                                  \
                                                                                                         \
                                                              key = GateMP_enter(gAvsyncLink_obj.gate);

#define  AVSYNC_CRITICAL_END()                                GateMP_leave(gAvsyncLink_obj.gate,key);    \
                                                          }  while(0)

#define AVSYNC_SROBJ_RETRY_INTERVAL_MICROSEC                 (100000)

typedef struct AvsyncLinkHLOS_Obj
{
    /* Capture link task */
    Bool              initDone;
    UInt64            wallTimeBase;
    NameServer_Handle nsHandleCreate;
    NameServer_Handle nsHandleGetHandle;
    GateMP_Handle     gate;
    Ptr               gateMPSrMemPtr;
    UInt32            gateMPSrMemSize;
    SharedRegion_SRPtr srObjSrPtr;
    Avsync_SharedObj  *srObj;
} AvsyncLinkHLOS_Obj;

#define AVSYNC_LOG_MAX_SLAVE_CHANNELS                        (32)
#define AVSYNC_LOG_AUDSYNC_INFO_MAX_RECORDS                  (30*30*1)

typedef struct Avsync_logAudSynchRecord
{
    UInt64 curWallTime;
    UInt64 audFrmPTS;
    Int32  clkAdjustDelta;
} Avsync_logAudSynchRecord;

typedef struct Avsync_logAudSynchTbl
{
    UInt32 index;
    Avsync_logAudSynchRecord rec[AVSYNC_LOG_AUDSYNC_INFO_MAX_RECORDS];
} Avsync_logAudSynchTbl;


#endif

/* @} */

