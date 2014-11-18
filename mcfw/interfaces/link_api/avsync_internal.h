/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2012 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/
/** @cond AVSYNC_INTERNAL_DOCUMENTATION_ENABLE */
/**
    \ingroup AvSync library
*/

/**
    \file avsync_internal.h
    \brief Avsync internal datastructure and functions. Not to be included by application
*/

#ifndef _AVSYNC_INTERNAL_H_
#define _AVSYNC_INTERNAL_H_

#include <stdint.h>
#include <mcfw/interfaces/link_api/avsync.h>
#include <mcfw/interfaces/link_api/system_debug.h>
#include <ti/ipc/SharedRegion.h>


/** Enumeration of clock states. */
typedef enum
{
    AVSYNC_TIME_ClockStateRunning,                  /**< Clock running. */
    AVSYNC_TIME_ClockStateWaitingForStartTime,      /**< Clock waiting until the prescribed clients emit their  start time.
                                                      * AVSYNC_TIME_ClockStateStopped,  Clock stopped.*/
    AVSYNC_TIME_ClockStateMax = 0x7FFFFFFF
} AVSYNC_TIME_CLOCKSTATE;


/**<  AVSync Video / Audio Frame Logic return values. AVSync Frame Q / RingIO should interpret & act as per these flags  */
typedef enum
{
    AVSYNC_DropFrame,                               /**<  lag scenario. frame buffer should be recycled */
    AVSYNC_RepeatFrame,                             /**<  lead scenario */
    AVSYNC_RenderFrame,                             /**<  pts & media time match within limits */
    AVSYNC_RenderFrame_LAST = 0x7FFFFFFF
} AVSYNC_FRAME_RENDER_FLAG;



#define AVSYNC_MAX_NUM_INSTANCES                                            (1)
#define AVSYNC_STC_WRAP_WINDOW_MS                                           (100)
#define AVSYNC_STC_WRAP_THRESHOLD_START                                     ((AVSYNC_MAX_STC_VALUE) - (AVSYNC_STC_WRAP_WINDOW_MS))
#define AVSYNC_SROBJ_NAME                                                   "AVSYNC_SROBJ\0"
#define AVSYNC_SROBJ_CREATE_PROCID                                          (SYSTEM_PROC_M3VPSS)
#define AVSYNC_WALLTIMER_UPDATE_PERIOD_MS                                   (1)
#define AVSYNC_WALLTIMER_UPDATE_PERIOD_MAX_MS                               (100)
#define AVSYNC_PAUSE_TIMESCALE                                              (0.0)
#define AVSYNC_GET_PTS_DELTA(systemTime,framePTS)                           (((systemTime) >= (framePTS)) ? \
                                                                             ((systemTime) - (framePTS))  : \
                                                                             (-1 * ((Int32)((framePTS) - (systemTime)))))
#define AVSYNC_SIMPLE_MOVING_AVERAGE_NUM_ENTRIES                            (8)
#define AVSYNC_UNDERRUN_QUE_LEN                                             (1)
#define  AVSYNC_NAMESERVERNAME_HLOS                                         "AVSYNC_NS_HLOS\0"
#define  AVSYNC_NAMESERVERNAME_RTOS                                         "AVSYNC_NS_RTOS\0"
#define  AVSYNC_FIRST_PTS_MAX_DELTA_MS                                      (500)
//#define  AVSYNC_APPLY_PTS_SMOOTHING_FILTER                                  (TRUE)

#define  AVSYNC_IS_MASTER_SYNC_CHANNEL(masterCh,curCh)    ((masterCh) == (curCh))
#define  AVSYNC_MAP_TIMESCALE_TO_PLAYSTATE(timeScale)     ((((timeScale) > 0.9)        \
                                                            &&                         \
                                                            ((timeScale) < 1.1))       \
                                                           ? AVSYNC_PLAYER_STATE_PLAY  \
                                                           : AVSYNC_PLAYER_STATE_TIMESCALE_PLAY)

#define  AVSYNC_MIN_PTS(a,b)                             ((((uint64_t)a) < ((uint64_t)b)) ? (a) : (b))

typedef enum Avsync_PlayerTimeState
{
    AVSYNC_PLAYERTIME_STATE_INIT,
    AVSYNC_PLAYERTIME_STATE_RUNNING
} Avsync_PlayerTimeState;

typedef enum Avsync_PlayerState
{
    AVSYNC_PLAYER_STATE_RESET,
    AVSYNC_PLAYER_STATE_PLAY,
    AVSYNC_PLAYER_STATE_PAUSE,
    AVSYNC_PLAYER_STATE_TIMESCALE_PLAY,
    AVSYNC_PLAYER_STATE_STEP_FWD,
    AVSYNC_PLAYER_STATE_SCAN,
    AVSYNC_PLAYER_STATE_LAST = 0x80000000
} Avsync_PlayerState;

typedef struct  Avsync_PlayerTimeObj
{
    UInt32 doInit;
    volatile UInt32 state;
    Avsync_PlayerState playState;
    UInt32 takeNextStep;
    UInt32 scanFrameDisplayDuration;
    UInt32 firstFrmInSeqReceived;
    Int32  refClkAdjustVal;
    UInt32 curSeqId;
    UInt64 mediaTimeBase;
    UInt64 wallTimeBase;
    UInt64 firstAudioPTS;
    UInt64 firstVideoPTS;
    UInt64 lastVidFrmPTS;
    UInt64 lastAudFrmPTS;
    Float  scalingFactor;
    Float  prePauseScalingFactor;
    Avsync_SystemClockStats stats;
}  Avsync_PlayerTimeObj;

typedef struct Avsync_WallTimerObj
{
    UInt64 curWallTime;
    UInt32 lastSTC;
    UInt32 rollOverCount;
} Avsync_WallTimerObj;

typedef struct Avsync_SimpleMovingAvgObj
{
    Bool  avgQueFilled;
    Int32 index;
    Int32  total;
    Int32  val[AVSYNC_SIMPLE_MOVING_AVERAGE_NUM_ENTRIES];
} Avsync_SimpleMovingAvgObj;

typedef struct Avsync_RefClkObj
{
    UInt32                   doInit;
    UInt32                   synchMasterChNum;
    UInt64                   lastSynchMasterVidRenderWallTS;
    Avsync_PlayerTimeObj     *masterPlayerTime;
    UInt32                    numSynchedChannels;
    Avsync_SimpleMovingAvgObj ptsDiffAvg;
    Avsync_PlayerTimeObj     *synchChannelList[AVSYNC_MAX_CHANNELS_PER_DISPLAY];
} Avsync_RefClkObj;

#define AVSYNC_LOG_MAX_SLAVE_CHANNELS                        (32)
#define AVSYNC_LOG_CAPTURE_TS_NUMCH                          (16)
#define AVSYNC_LOG_IPC_TS_NUMCH                              (16)

#ifdef SYSTEM_DEBUG_AVSYNC_DETAILED_LOGS
#define AVSYNC_LOG_VIDSYNC_INFO_MAX_RECORDS                  (1024)
#define AVSYNC_LOG_TS_MAX_RECORDS                            (2048)
#else
#define AVSYNC_LOG_VIDSYNC_INFO_MAX_RECORDS                  (1)
#define AVSYNC_LOG_TS_MAX_RECORDS                            (1)
#endif

//#define AVSYNC_LOG_VERBOSE_PRINT                             (TRUE)


typedef struct Avsync_logVidSlaveRecord
{
    Int32 syncMasterPTSDelta;
    AVSYNC_FRAME_RENDER_FLAG renderFlag;
    Bool  underRun;
} Avsync_logVidSlaveRecord;

typedef struct Avsync_logVidSlaveRecordTbl
{
    UInt32 index;
    Avsync_logVidSlaveRecord rec[AVSYNC_LOG_VIDSYNC_INFO_MAX_RECORDS];
} Avsync_logVidSlaveRecordTbl;

typedef struct Avsync_logVidMasterRecord
{
    UInt64 renderPTS;
    UInt64 synchMasterPTS;
    Int32  clkAdjustDelta;
    AVSYNC_FRAME_RENDER_FLAG renderFlag;
} Avsync_logVidMasterRecord;

typedef struct Avsync_logVidMasterRecordTbl
{
    UInt32 index;
    Avsync_logVidMasterRecord rec[AVSYNC_LOG_VIDSYNC_INFO_MAX_RECORDS];
} Avsync_logVidMasterRecordTbl;

typedef struct Avsync_logVidSynchRecord
{
    Avsync_logVidSlaveRecord  synchChInfo[AVSYNC_LOG_MAX_SLAVE_CHANNELS];
} Avsync_logVidSynchRecord;

typedef struct Avsync_logVidSynchTbl
{
    Avsync_logVidMasterRecordTbl masterRec;
    Avsync_logVidSlaveRecordTbl  slaveRec[AVSYNC_LOG_MAX_SLAVE_CHANNELS];
} Avsync_logVidSynchTbl;

typedef struct AvsyncLink_TimestampLog
{
    UInt32 logIndex;
    UInt64 ts[AVSYNC_LOG_TS_MAX_RECORDS];
} AvsyncLink_TimestampLog;

typedef struct  Avsync_SharedObj
{
    SharedRegion_SRPtr   gateMPAddr;
    Avsync_WallTimerObj  wallTimer;
    Avsync_PlayerTimeObj playerTime[AVSYNC_MAX_NUM_DISPLAYS][AVSYNC_MAX_CHANNELS_PER_DISPLAY];
    Avsync_RefClkObj     refClk[AVSYNC_MAX_NUM_DISPLAYS];
    Avsync_VidStats      vidstats[AVSYNC_MAX_NUM_DISPLAYS][AVSYNC_MAX_CHANNELS_PER_DISPLAY];
    Avsync_AudStats      audstats[AVSYNC_MAX_NUM_DISPLAYS][AVSYNC_MAX_CHANNELS_PER_DISPLAY];
    Avsync_SynchConfigParams queCfg[AVSYNC_MAX_NUM_DISPLAYS][AVSYNC_MAX_CHANNELS_PER_DISPLAY];
    UInt32               numAudQueCreated;
    UInt32               numVidQueCreated[AVSYNC_MAX_NUM_DISPLAYS];
    Avsync_logVidSynchTbl   vidSyncLog[AVSYNC_MAX_NUM_DISPLAYS];
    AvsyncLink_TimestampLog capTSLog[AVSYNC_LOG_CAPTURE_TS_NUMCH];
    AvsyncLink_TimestampLog ipcTSLog[AVSYNC_LOG_IPC_TS_NUMCH];
} Avsync_SharedObj;


typedef struct Avsync_MasterSynchInfo
{
    UInt32 audioDevId;
    UInt32 displayId;
    UInt32 synchMasterChNum;
} Avsync_MasterSynchInfo;

static inline Bool AVSYNC_IS_VIDEO_MASTER_SYCH_STATE(Avsync_PlayerState playerState)
{
    Bool isVideoMasterState;

    isVideoMasterState = ((playerState == AVSYNC_PLAYER_STATE_PAUSE)
                          ||
                          (playerState == AVSYNC_PLAYER_STATE_STEP_FWD)
                          ||
                          (playerState == AVSYNC_PLAYER_STATE_TIMESCALE_PLAY))
                         ? TRUE
                         : FALSE;
    return isVideoMasterState;
}

#define AVSYNC_IS_MUTE_AUDIO_STATE(playerState)   (AVSYNC_IS_VIDEO_MASTER_SYCH_STATE(playerState))

#endif

/** @endcond AVSYNC_INTERNAL_DOCUMENTATION_ENABLE */

