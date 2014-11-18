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
    \file avsync_m3vpss.c
    \brief m3 side function definitions.
*/

#include <xdc/std.h>
#include <mcfw/src_bios6/utils/utils_dmtimer.h>
#include <mcfw/src_bios6/links_m3vpss/avsync/avsync_m3vpss.h>

AvsyncLink_Obj gAvsyncLink_obj;
static const Avsync_SynchConfigParams gAvsyncDisableParam =
{
    .avsyncEnable = FALSE,
};

/* link stack */
#pragma DATA_ALIGN(gAvsyncLink_tskStack, 32)
#pragma DATA_SECTION(gAvsyncLink_tskStack, ".bss:taskStackSection")
UInt8 gAvsyncLink_tskStack[AVSYNC_LINK_TSK_STACK_SIZE];

#if (AVSYNC_WALLTIMER_UPDATE_PERIOD_MS > AVSYNC_WALLTIMER_UPDATE_PERIOD_MAX_MS)
    #error "AVSYNC_WALLTIMER_UPDATE_PERIOD_MAX_MS must be > AVSYNC_WALLTIMER_UPDATE_PERIOD_MS"
#endif

#define AVSYNC_GET_LOG_TBL_HANDLE()             (&(gAvsyncLink_obj.srObj->vidSyncLog[0]))
#define AVSYNC_GET_CAPTURE_TS_HANDLE()          (&(gAvsyncLink_obj.srObj->capTSLog[0]))
#define AVSYNC_IS_FORCE_DISABLE_CFG(cfg)        ((cfg) == &gAvsyncDisableParam)

Void Avsync_vidSynchCallbackFxn(UInt32 displayID)
{
    AvsyncLink_Obj *pObj = &gAvsyncLink_obj;

    UTILS_assert(displayID < UTILS_ARRAYSIZE(pObj->displayObj));
    pObj->displayObj[displayID].vSyncSTC = Avsync_getWallTime();
    if (pObj->displayObj[displayID].cbHookObj.cbFxn)
    {
        pObj->displayObj[displayID].cbHookObj.cbFxn(pObj->displayObj[displayID].cbHookObj.ctx);
    }
}

Void Avsync_vidSynchRegisterCbHook(UInt32 displayID,
                                   AvsyncLink_VidSyncCbHookObj *cbHookObj)
{
    AvsyncLink_Obj *pObj = &gAvsyncLink_obj;
    UInt key = Hwi_disable();

    UTILS_assert(displayID < UTILS_ARRAYSIZE(pObj->displayObj));
    pObj->displayObj[displayID].cbHookObj = *cbHookObj;
    Hwi_restore(key);
}


static Void avsync_update_wall_timer(UArg arg)
{
    Avsync_WallTimerObj *pWallTimerObj = (Avsync_WallTimerObj *)arg;
    UInt32 curSTC = AVSYNC_GET_HW_TIME();

    if (curSTC < pWallTimerObj->lastSTC)
    {

        if (pWallTimerObj->lastSTC >= AVSYNC_STC_WRAP_THRESHOLD_START)
        {
            pWallTimerObj->rollOverCount++;
            Vps_rprintf("\n\nAVSYNC:WallTime Rollover. PrevTs[%d]/CurTs[%d]\n",
                        pWallTimerObj->lastSTC,curSTC);
            pWallTimerObj->curWallTime +=
              ((AVSYNC_MAX_STC_VALUE - pWallTimerObj->lastSTC) + curSTC);
        }
        else
        {
            Vps_rprintf("\n\nAVSYNC:WallTime IGNORE Unexpected Discontinuity.PrevTs[%d]/CurTs[%d]\n",
                        pWallTimerObj->lastSTC,curSTC);
            pWallTimerObj->curWallTime += AVSYNC_WALLTIMER_UPDATE_PERIOD_MS;
        }
    }
    else
    {
        UInt32 stcDelta;

        stcDelta = curSTC - pWallTimerObj->lastSTC;
        if (stcDelta <= AVSYNC_WALLTIMER_UPDATE_PERIOD_MAX_MS)
        {
            pWallTimerObj->curWallTime += stcDelta;
        }
        else
        {
            Vps_rprintf("\n\nAVSYNC:WallTime IGNORE Unexpected Discontinuity.PrevTs[%d]/CurTs[%d]\n",
                        pWallTimerObj->lastSTC,curSTC);
            pWallTimerObj->curWallTime += AVSYNC_WALLTIMER_UPDATE_PERIOD_MS;
        }
    }
    pWallTimerObj->lastSTC = curSTC;
}

static Void avsync_create_prd_obj(AvsyncLink_WallTimerUpdateObj *timerElem,
                                  ti_sysbios_knl_Clock_FuncPtr clockFxn,
                                  UArg          clockFxnArg)
{
    ti_sysbios_knl_Clock_Params clockParams;

    ti_sysbios_knl_Clock_Params_init(&clockParams);
    clockParams.arg = clockFxnArg;
    UTILS_assert(timerElem->clkHandle == NULL);

    ti_sysbios_knl_Clock_construct(&(timerElem->clkStruct),
                    clockFxn,
                    1,
                    &clockParams);
    timerElem->clkHandle = Clock_handle(&timerElem->clkStruct);
    timerElem->clkStarted = FALSE;
}

static Void avsync_delete_prd_obj(AvsyncLink_WallTimerUpdateObj *timerElem)
{
    UTILS_assert(timerElem->clkHandle != NULL);

    /* Stop the clock */
    Clock_stop(timerElem->clkHandle);
    Clock_destruct(&(timerElem->clkStruct));
    timerElem->clkHandle = NULL;
    timerElem->clkStarted = FALSE;

}

static Void avsync_start_prd_obj(AvsyncLink_WallTimerUpdateObj *timerElem,
                                 UInt period)
{
    UTILS_assert(timerElem->clkHandle != NULL);

    if (FALSE == timerElem->clkStarted)
    {
        Clock_setPeriod(timerElem->clkHandle, period);
        Clock_setTimeout(timerElem->clkHandle, period);
        Clock_start(timerElem->clkHandle);
        timerElem->clkStarted = TRUE;
    }

}

static Void avsync_stop_prd_obj(AvsyncLink_WallTimerUpdateObj *timerElem)
{
    UTILS_assert(timerElem->clkHandle != NULL);

    if (TRUE == timerElem->clkStarted)
    {
        Clock_stop(timerElem->clkHandle);
        timerElem->clkStarted = FALSE;
    }
}

static Void avsync_init_wall_timer(AvsyncLink_Obj *pObj,
                                   UInt64 initWallTimeTS)
{
    /* Initialize wall time to
     * init time - AVSYNC_WALLTIMER_UPDATE_PERIOD_MS
     * This is because wall time will start from the
     * PRD callback which will occur after
     * AVSYNC_WALLTIMER_UPDATE_PERIOD_MS ms
     */
    pObj->srObj->wallTimer.curWallTime =
      initWallTimeTS - AVSYNC_WALLTIMER_UPDATE_PERIOD_MS;
    pObj->srObj->wallTimer.lastSTC     = 0;
    pObj->srObj->wallTimer.rollOverCount = 0;
    avsync_create_prd_obj(&pObj->wallTimerUpdateObj,
                          avsync_update_wall_timer,
                          (UArg)&pObj->srObj->wallTimer);
    pObj->srObj->wallTimer.lastSTC = AVSYNC_GET_HW_TIME();
    avsync_start_prd_obj(&pObj->wallTimerUpdateObj,
                         AVSYNC_WALLTIMER_UPDATE_PERIOD_MS);
}

static Void avsync_uninit_wall_timer(AvsyncLink_Obj *pObj)
{
    avsync_stop_prd_obj(&pObj->wallTimerUpdateObj);
    avsync_delete_prd_obj(&pObj->wallTimerUpdateObj);
}

static Void avsync_init_player_timer(Avsync_PlayerTimeObj *playerTimeObj)
{
    AVSYNC_CRITICAL_BEGIN();
    playerTimeObj->mediaTimeBase = AVSYNC_INVALID_PTS;
    playerTimeObj->scalingFactor = 1.0;
    playerTimeObj->wallTimeBase = AVSYNC_INVALID_PTS;
    memset(&playerTimeObj->stats,0,sizeof(playerTimeObj->stats));
    playerTimeObj->firstVideoPTS = AVSYNC_INVALID_PTS;
    playerTimeObj->firstAudioPTS = AVSYNC_INVALID_PTS;
    playerTimeObj->refClkAdjustVal = 0;
    playerTimeObj->lastVidFrmPTS = AVSYNC_INVALID_PTS;
    playerTimeObj->lastAudFrmPTS = AVSYNC_INVALID_PTS;
    playerTimeObj->scanFrameDisplayDuration = 0;
    playerTimeObj->takeNextStep = FALSE;
    playerTimeObj->playState       = AVSYNC_PLAYER_STATE_RESET;
    playerTimeObj->state = AVSYNC_PLAYERTIME_STATE_INIT;
    playerTimeObj->firstFrmInSeqReceived = FALSE;
    playerTimeObj->curSeqId = SYSTEM_DISPLAY_SEQID_DEFAULT;
    AVSYNC_CRITICAL_END();
}

static UInt64 avsync_get_player_timer(Avsync_PlayerTimeObj *playerTimeObj,
                                      UInt64 wallTime)
{
    UInt64 wallTimeElapsed;
    UInt64 playerTime;

    AVSYNC_CRITICAL_BEGIN();
    if (playerTimeObj->state == AVSYNC_PLAYERTIME_STATE_RUNNING)
    {
        wallTimeElapsed = wallTime - playerTimeObj->wallTimeBase;
        wallTimeElapsed *= playerTimeObj->scalingFactor;
        playerTime =
             ((playerTimeObj->mediaTimeBase + playerTimeObj->refClkAdjustVal)
                + wallTimeElapsed);
    }
    else
    {
        playerTime = AVSYNC_INVALID_PTS;
    }
    AVSYNC_CRITICAL_END();
    return playerTime;
}

static Void avsync_set_player_timer(Avsync_PlayerTimeObj *playerTimeObj,
                                    UInt64                mediaTimeBase,
                                    Float                 curScalingFactor,
                                    Int32                 *clkAdjustDelta)
{
    UInt64 preAdjustPlayerTime;
    UInt64 postAdjustPlayerTime;
    UInt64 curSTC = Avsync_getWallTime();

    AVSYNC_CRITICAL_BEGIN();
    preAdjustPlayerTime = avsync_get_player_timer(playerTimeObj,curSTC);
    playerTimeObj->scalingFactor = curScalingFactor;
    playerTimeObj->mediaTimeBase = mediaTimeBase;
    playerTimeObj->wallTimeBase  = Avsync_getWallTime();
    playerTimeObj->refClkAdjustVal = 0;
    postAdjustPlayerTime = avsync_get_player_timer(playerTimeObj,curSTC);
    if (postAdjustPlayerTime > preAdjustPlayerTime)
    {
        playerTimeObj->stats.numClockAdjustPositive++;
        *clkAdjustDelta = postAdjustPlayerTime - preAdjustPlayerTime;
    }
    else
    {
        playerTimeObj->stats.numClockAdjustNegative++;
        *clkAdjustDelta = -1 * (Int32)(preAdjustPlayerTime - postAdjustPlayerTime);
    }
    AVSYNC_CRITICAL_END();
}

static Void avsync_set_timescale(Avsync_PlayerTimeObj *playerTimeObj,
                                 Float newScalingFactor,
                                 UInt32 displaySeqId)
{
    AVSYNC_CRITICAL_BEGIN();
    Int32 clkAdjustDelta;
    UInt64 curPlayerTime = avsync_get_player_timer(playerTimeObj,
                                                   Avsync_getWallTime());

    if (AVSYNC_INVALID_PTS != curPlayerTime)
    {
        avsync_set_player_timer(playerTimeObj,curPlayerTime,newScalingFactor,
                                &clkAdjustDelta);
        playerTimeObj->playState =
             AVSYNC_MAP_TIMESCALE_TO_PLAYSTATE(playerTimeObj->scalingFactor);
        Vps_printf("AVSYNC:avsync_set_timescale:"
                   " Scaling Factor:[%f]"
                   " Clock Adjust delta[%d]\n",
                   newScalingFactor,
                   clkAdjustDelta);
    }
    else
    {
        playerTimeObj->scalingFactor = newScalingFactor;
    }
    playerTimeObj->curSeqId = displaySeqId;
    AVSYNC_CRITICAL_END();
}

static Void avsync_set_player_timer_state_play(Avsync_PlayerTimeObj *playerTimeObj,
                                               UInt32 displaySeqId)
{
    AVSYNC_CRITICAL_BEGIN();
    avsync_set_timescale(playerTimeObj,1.0,displaySeqId);
    playerTimeObj->playState   = AVSYNC_PLAYER_STATE_PLAY;
    AVSYNC_CRITICAL_END();
}

static Void avsync_set_player_timer_state_scan(Avsync_PlayerTimeObj *playerTimeObj,
                                               UInt32 frameDisplayDuration,
                                               UInt32 displaySeqId)
{
    AVSYNC_CRITICAL_BEGIN();
    avsync_set_timescale(playerTimeObj,1.0,displaySeqId);
    playerTimeObj->lastVidFrmPTS = AVSYNC_INVALID_PTS;
    playerTimeObj->scanFrameDisplayDuration = frameDisplayDuration;
    playerTimeObj->playState   = AVSYNC_PLAYER_STATE_SCAN;
    playerTimeObj->curSeqId    = displaySeqId;
    AVSYNC_CRITICAL_END();
}

static Void avsync_set_player_time_firstvidpts(Avsync_PlayerTimeObj *playerTimeObj,
                                               UInt64                videoPTS)
{
    AVSYNC_CRITICAL_BEGIN();
    playerTimeObj->firstVideoPTS = videoPTS;
    AVSYNC_CRITICAL_END();
}

static UInt64 avsync_get_player_time_firstvidpts(Avsync_PlayerTimeObj *playerTimeObj)
{
    UInt64                videoPTS;

    AVSYNC_CRITICAL_BEGIN();
    videoPTS = playerTimeObj->firstVideoPTS;
    AVSYNC_CRITICAL_END();

    return videoPTS;
}

static Void avsync_set_player_time_firstaudpts(Avsync_PlayerTimeObj *playerTimeObj,
                                               UInt64                audioPTS)
{
    UTILS_assert(AVSYNC_INVALID_PTS == playerTimeObj->firstAudioPTS);
    AVSYNC_CRITICAL_BEGIN();
    playerTimeObj->firstAudioPTS = audioPTS;
    AVSYNC_CRITICAL_END();
}

static Void avsync_init_player_time_walltimebase(Avsync_PlayerTimeObj *playerTimeObj,
                                                 UInt64                wallTimeBase)
{
    UTILS_assert(playerTimeObj->wallTimeBase == AVSYNC_INVALID_PTS);
    AVSYNC_CRITICAL_BEGIN();
    playerTimeObj->wallTimeBase = wallTimeBase;
    AVSYNC_CRITICAL_END();
}

static Void avsync_init_player_time_mediatimebase(Avsync_PlayerTimeObj *playerTimeObj,
                                                  UInt64                mediaTimeBase)
{
    UTILS_assert(playerTimeObj->mediaTimeBase == AVSYNC_INVALID_PTS);
    AVSYNC_CRITICAL_BEGIN();
    playerTimeObj->mediaTimeBase = mediaTimeBase;
    AVSYNC_CRITICAL_END();
}

static Bool avsync_start_player_time(Avsync_PlayerTimeObj *playerTime)
{
    Bool playerTimeStarted = FALSE;
    UInt64 mediaBaseTime = AVSYNC_INVALID_PTS;

    AVSYNC_CRITICAL_BEGIN();
    if (playerTime->mediaTimeBase == AVSYNC_INVALID_PTS)
    {
        if ((playerTime->firstVideoPTS != AVSYNC_INVALID_PTS)
            ||
            (playerTime->firstAudioPTS != AVSYNC_INVALID_PTS))
        {
            if ((playerTime->firstVideoPTS != AVSYNC_INVALID_PTS)
                &&
                (playerTime->firstAudioPTS != AVSYNC_INVALID_PTS))
            {
                mediaBaseTime = AVSYNC_MIN_PTS(playerTime->firstVideoPTS,
                                               playerTime->firstAudioPTS);
            }
            else
            {
                if (playerTime->firstVideoPTS != AVSYNC_INVALID_PTS)
                {
                    mediaBaseTime = playerTime->firstVideoPTS;
                }
                else
                {
                    UTILS_assert(playerTime->firstAudioPTS != AVSYNC_INVALID_PTS);
                    mediaBaseTime = playerTime->firstVideoPTS;
                }
            }
            /* Decrement media base so that play back starts after 1 ms */
            UTILS_assert(mediaBaseTime != AVSYNC_INVALID_PTS);
            if (mediaBaseTime)
                mediaBaseTime--;
            Vps_printf("AVSYNC:Media Time Base:%d,FirstVidPTS:%d,FirstAudPTS:%d\n",
                       (UInt32)mediaBaseTime,
                       (UInt32)playerTime->firstVideoPTS,
                       (UInt32)playerTime->firstAudioPTS);
            avsync_init_player_time_mediatimebase(playerTime,
                                                  mediaBaseTime);
        }
    }
    if ((playerTime->mediaTimeBase != AVSYNC_INVALID_PTS)
        &&
        (playerTime->wallTimeBase != AVSYNC_INVALID_PTS))
    {
        playerTime->state = AVSYNC_PLAYERTIME_STATE_RUNNING;
        playerTimeStarted = TRUE;
    }
    AVSYNC_CRITICAL_END();
    return playerTimeStarted;
}


static Void avsync_set_player_time_refclk_delta(Avsync_PlayerTimeObj *playerTimeObj,
                                                Int32 refClkAdjust)
{
    UTILS_assert((playerTimeObj->state == AVSYNC_PLAYERTIME_STATE_INIT)
                 ||
                 (playerTimeObj->state == AVSYNC_PLAYERTIME_STATE_RUNNING));
    AVSYNC_CRITICAL_BEGIN();
    if (playerTimeObj->state == AVSYNC_PLAYERTIME_STATE_RUNNING)
    {
        playerTimeObj->refClkAdjustVal += refClkAdjust;
    }
    AVSYNC_CRITICAL_END();
}


static Void avsync_pause_player_time(Avsync_PlayerTimeObj *playerTimeObj)
{
    AVSYNC_CRITICAL_BEGIN();
    playerTimeObj->prePauseScalingFactor = playerTimeObj->scalingFactor;
    avsync_set_timescale(playerTimeObj,AVSYNC_PAUSE_TIMESCALE,
                         playerTimeObj->curSeqId);
    playerTimeObj->playState = AVSYNC_PLAYER_STATE_PAUSE;
    AVSYNC_CRITICAL_END();
}

static Void avsync_unpause_player_time(Avsync_PlayerTimeObj *playerTimeObj)
{
    AVSYNC_CRITICAL_BEGIN();
    avsync_set_timescale(playerTimeObj,
                         playerTimeObj->prePauseScalingFactor,
                         playerTimeObj->curSeqId);
    playerTimeObj->playState = AVSYNC_MAP_TIMESCALE_TO_PLAYSTATE(playerTimeObj->scalingFactor);
    AVSYNC_CRITICAL_END();
}

static Void avsync_step_fwd_player_time(Avsync_PlayerTimeObj *playerTimeObj)
{
    AVSYNC_CRITICAL_BEGIN();
    if (playerTimeObj->playState != AVSYNC_PLAYER_STATE_STEP_FWD)
    {
        avsync_pause_player_time(playerTimeObj);
        playerTimeObj->playState = AVSYNC_PLAYER_STATE_STEP_FWD;
    }
    playerTimeObj->takeNextStep = TRUE;
    AVSYNC_CRITICAL_END();
}


#define AVSYNC_RSRC_GATEMP_NONE                               (0)
#define AVSYNC_RSRC_GATEMP_MEMALLOC                           (1 << 0)
#define AVSYNC_RSRC_GATEMP_GATECREATED                        (1 << 1)
#define AVSYNC_RSRC_GATEMP_ALL                                (AVSYNC_RSRC_GATEMP_MEMALLOC |\
                                                               AVSYNC_RSRC_GATEMP_GATECREATED)

static Void avsync_delete_gatemp(AvsyncLink_Obj *pObj,Int rsrcMask)
{
    Int status;

    if (rsrcMask & AVSYNC_RSRC_GATEMP_GATECREATED)
    {
        UTILS_assert(pObj->gate != NULL);
        status = GateMP_delete(&pObj->gate);
        UTILS_assert(status == GateMP_S_SUCCESS);
    }
    else
    {
        /* if only GateSRMemory is allocated without
         * GateMP created, free only the sr memory.
         * If GateMP was created srMemory would be
         * automatically freed at GateMP_delete
         * time
         */
        if (rsrcMask & AVSYNC_RSRC_GATEMP_MEMALLOC)
        {
            IHeap_Handle srHeap;

            srHeap = SharedRegion_getHeap(SYSTEM_IPC_SR_AVSYNC);
            UTILS_assert(srHeap != NULL);
            Memory_free(srHeap, pObj->gateMPSrMemPtr, pObj->gateMPSrMemSize);
        }
    }
}

static Int avsync_create_gatemp(AvsyncLink_Obj *pObj)
{
    GateMP_Params   gateMPParams;
    UInt32          gateMPSize;
    Int status      = AVSYNC_S_OK;
    Int rsrcMask = AVSYNC_RSRC_GATEMP_NONE;

    GateMP_Params_init (&gateMPParams);
    gateMPParams.remoteProtect  = GateMP_RemoteProtect_SYSTEM;
    gateMPParams.localProtect   = GateMP_LocalProtect_TASKLET;
    gateMPSize                  = GateMP_sharedMemReq(&gateMPParams);
    gateMPParams.sharedAddr     = Memory_alloc( SharedRegion_getHeap(SYSTEM_IPC_SR_AVSYNC),
                                                gateMPSize,
                                                SharedRegion_getCacheLineSize(SYSTEM_IPC_SR_AVSYNC),
                                                NULL);
    if (gateMPParams.sharedAddr == NULL)
    {
        Vps_printf("AVSYNC:[%s:%d] AVSync GateMP struct allocation failed\n",
                   __FUNCTION__ , __LINE__);
        status =  AVSYNC_E_INSUFFICIENTRESOURCES;
    }

    if (AVSYNC_S_OK == status)
    {
        rsrcMask |= AVSYNC_RSRC_GATEMP_MEMALLOC;
        pObj->gateMPSrMemPtr  = gateMPParams.sharedAddr;
        pObj->gateMPSrMemSize = gateMPSize;
        pObj->gate = GateMP_create (&gateMPParams);

        if (pObj->gate == NULL)
        {
            Vps_printf("AVSYNC:[%s:%d] AVSync GateMP create failed\n",
                       __FUNCTION__ , __LINE__);
            status =  AVSYNC_E_INSUFFICIENTRESOURCES;
        }
        else
        {
            rsrcMask |= AVSYNC_RSRC_GATEMP_GATECREATED;
        }
    }

    if (status != AVSYNC_S_OK)
    {
        avsync_delete_gatemp(pObj,rsrcMask);
    }

    return status;
}

static Void  avsync_delete_srobj(AvsyncLink_Obj *pObj)
{
    IHeap_Handle  srHeap;
    IArg          key;

    UTILS_assert(pObj->gate != NULL);
    UTILS_assert(pObj->srObj != NULL);
    key = GateMP_enter(pObj->gate);

    avsync_uninit_wall_timer(pObj);

    srHeap = SharedRegion_getHeap(SYSTEM_IPC_SR_AVSYNC);
    UTILS_assert(srHeap != NULL);

    Memory_free(srHeap, pObj->srObj,sizeof(*(pObj->srObj)));
    GateMP_leave(pObj->gate,key);
}


static Int  avsync_create_srobj(AvsyncLink_Obj *pObj)
{
    IHeap_Handle  srHeap;
    Int status = AVSYNC_S_OK;
    Error_Block ebObj;
    Error_Block *eb = &ebObj;

    Error_init(eb);

    UTILS_assert(pObj->gate != NULL);
    AVSYNC_CRITICAL_BEGIN();

    srHeap = SharedRegion_getHeap(SYSTEM_IPC_SR_AVSYNC);
    UTILS_assert(srHeap != NULL);

    pObj->srObj = Memory_calloc(srHeap,
                                sizeof(*(pObj->srObj)),
                                SharedRegion_getCacheLineSize(SYSTEM_IPC_SR_AVSYNC),
                                eb);
    UTILS_assert(NULL != pObj->srObj);
    if (NULL == pObj->srObj)
    {
        Vps_printf("AVSYNC:WARNING!!!!! AVSYNC SROBJ ALLOC FAILED!!!!!");
        status = AVSYNC_E_INSUFFICIENTRESOURCES;
    }
    AVSYNC_CRITICAL_END();
    return status;
}

static Void avsync_print_smavobj(Avsync_SimpleMovingAvgObj *smaObj)
{
#ifdef AVSYNC_DEBUG_SMA
    Int i;

    Vps_rprintf("SMA_OBJ\n");
    Vps_rprintf("SMA_OBJ.avgQueFilled:%d\n", smaObj->avgQueFilled);
    Vps_rprintf("SMA_OBJ.total:%d\n", smaObj->total);
    Vps_rprintf("SMA_OBJ.index:%d\n", smaObj->index);
    for (i = 0; i < UTILS_ARRAYSIZE(smaObj->val);i++)
    {
        Vps_rprintf("SMA_OBJ.val[%d]:%d\n",i,smaObj->val[i]);
    }
#else
    (Void)smaObj;
#endif


}

static Void  avsync_init_smavgobj(Avsync_SimpleMovingAvgObj *smaObj)
{
    memset(smaObj,0,sizeof(*smaObj));
    smaObj->index = 0;
    smaObj->total = 0;
    smaObj->avgQueFilled = FALSE;
    avsync_print_smavobj(smaObj);
}



Int32 avsync_get_simple_moving_avg(Avsync_SimpleMovingAvgObj *smaObj,Int32 newVal)
{
    Int32 avg = 0;

    if (smaObj->avgQueFilled)
    {
        smaObj->total -= smaObj->val[smaObj->index];
        smaObj->total += newVal;
        smaObj->val[smaObj->index] = newVal;
        smaObj->index = (smaObj->index + 1) % AVSYNC_SIMPLE_MOVING_AVERAGE_NUM_ENTRIES;
        avg = smaObj->total / AVSYNC_SIMPLE_MOVING_AVERAGE_NUM_ENTRIES;
    }
    else
    {
        smaObj->total += newVal;
        smaObj->val[smaObj->index] = newVal;
        smaObj->index = (smaObj->index + 1) % AVSYNC_SIMPLE_MOVING_AVERAGE_NUM_ENTRIES;
        if (0 == smaObj->index)
        {
            smaObj->avgQueFilled = TRUE;
            avg = smaObj->total / AVSYNC_SIMPLE_MOVING_AVERAGE_NUM_ENTRIES;
        }
        else
        {
            avg = smaObj->total / smaObj->index;
        }
    }
    return avg;
}



static Void  avsync_init_refclkobj(Avsync_RefClkObj *refClk)
{
    refClk->numSynchedChannels = 0;
    refClk->masterPlayerTime = NULL;
    refClk->synchMasterChNum = AVSYNC_INVALID_CHNUM;
    refClk->lastSynchMasterVidRenderWallTS = AVSYNC_INVALID_PTS;
    avsync_init_smavgobj(&refClk->ptsDiffAvg);
}

static Void avsync_refclk_register_sync_playertime(Avsync_RefClkObj *refClk,
                                                   Avsync_PlayerTimeObj *playerTime)
{
    UTILS_assert(refClk->numSynchedChannels < UTILS_ARRAYSIZE(refClk->synchChannelList));
    refClk->synchChannelList[refClk->numSynchedChannels] = playerTime;
    refClk->numSynchedChannels++;
}

static Void avsync_refclk_unregister_sync_playertime(Avsync_RefClkObj *refClk,
                                                     Avsync_PlayerTimeObj *playerTime)
{
    Int i;
    
    UTILS_assert(refClk != NULL);
    if (refClk->numSynchedChannels)
    {
        for (i = 0; i < refClk->numSynchedChannels; i++)
        {
            if (refClk->synchChannelList[i] == playerTime)
            {
                break;
            }
        }
        UTILS_assert(i < refClk->numSynchedChannels);
        refClk->synchChannelList[i] =
        refClk->synchChannelList[(refClk->numSynchedChannels - 1)];
        refClk->numSynchedChannels--;
    }
    else
    {
        UTILS_warn("AVSYNC:Unregister with refClk called when no synched channels present");
    }
}



static Void avsync_refclk_register_master_playertime(Avsync_RefClkObj *refClk,
                                                     Avsync_PlayerTimeObj *playerTime,
                                                     UInt32 synchMasterChNum)
{
    UTILS_assert(refClk->masterPlayerTime == NULL);
    refClk->masterPlayerTime = playerTime;
    refClk->synchMasterChNum = synchMasterChNum;
}

static Void avsync_refclk_unregister_master_playertime(Avsync_RefClkObj *refClk,
                                                     Avsync_PlayerTimeObj *playerTime)
{
    UTILS_assert(refClk != NULL);
    refClk->masterPlayerTime = NULL;
    refClk->doInit = TRUE;
}

static Void  avsync_init_srobj(AvsyncLink_Obj *pObj,
                               Avsync_InitParams *initParams)
{
    UInt16 srId;
    Int    i,j;

    UTILS_assert(pObj->gate != NULL);
    AVSYNC_CRITICAL_BEGIN();

    UTILS_assert(pObj->gateMPSrMemPtr != NULL);
    srId = SharedRegion_getId(pObj->gateMPSrMemPtr);
    UTILS_assert(srId == SYSTEM_IPC_SR_AVSYNC);
    pObj->srObj->gateMPAddr = SharedRegion_getSRPtr(pObj->gateMPSrMemPtr,
                                                    srId);
    for (i = 0; i < UTILS_ARRAYSIZE(pObj->srObj->refClk);i++)
    {
        avsync_init_refclkobj(&pObj->srObj->refClk[i]);
        pObj->srObj->refClk[i].doInit = TRUE;
    }
    avsync_init_wall_timer(pObj,initParams->wallTimeInitTS);
    for (i = 0; i < UTILS_ARRAYSIZE(pObj->srObj->playerTime);i++)
    {
        for (j = 0; j < UTILS_ARRAYSIZE(pObj->srObj->playerTime[i]);j++)
        {
            avsync_init_player_timer(&pObj->srObj->playerTime[i][j]);
            pObj->srObj->playerTime[i][j].doInit = TRUE;
        }
    }
    memset(&pObj->srObj->queCfg,0,sizeof(pObj->srObj->queCfg));
    for (i = 0; i < AVSYNC_MAX_NUM_DISPLAYS;i++)
    {
        for (j = 0; j < AVSYNC_MAX_CHANNELS_PER_DISPLAY;j++)
        {
            pObj->srObj->queCfg[i][j].chNum = AVSYNC_INVALID_CHNUM;
        }
    }
    for (i = 0; i < UTILS_ARRAYSIZE(pObj->srObj->numVidQueCreated);i++)
    {
        pObj->srObj->numVidQueCreated[i] = 0;
    }
    pObj->srObj->numAudQueCreated = 0;
    for (i = 0; i < UTILS_ARRAYSIZE(pObj->srObj->vidSyncLog);i++)
    {
        memset(&pObj->srObj->vidSyncLog[i],0,sizeof(pObj->srObj->vidSyncLog[i]));
    }
    for (i = 0; i < UTILS_ARRAYSIZE(pObj->srObj->capTSLog);i++)
    {
        pObj->srObj->capTSLog[i].logIndex = 0;
    }
    AVSYNC_CRITICAL_END();
}

static Void avsync_unregister_srobj(AvsyncLink_Obj *pObj)
{
    Int status;

    status = NameServer_removeEntry(pObj->nsHandle,
                                    pObj->nsKey);
    UTILS_assert(NameServer_S_SUCCESS == status);

    status = NameServer_delete(&pObj->nsHandle);
    UTILS_assert(NameServer_S_SUCCESS == status);
}

static Int avsync_register_srobj(AvsyncLink_Obj *pObj)
{
    NameServer_Params nameServerParams;
    UInt16 srId;
    Int status = AVSYNC_S_OK;

    /* Get the default params for  the Name server */
    NameServer_Params_init(&nameServerParams);
    nameServerParams.maxRuntimeEntries = AVSYNC_MAX_NUM_INSTANCES; /* max instances on this processor */
    nameServerParams.checkExisting     = TRUE; /* Check if exists */
    nameServerParams.maxNameLen        = NameServer_Params_MAXNAMELEN;
    nameServerParams.maxValueLen       = sizeof (UInt32);
    /* Create the Name Server instance */
    pObj->nsHandle = NameServer_create (AVSYNC_NAMESERVERNAME_RTOS,
                                        &nameServerParams);
    UTILS_assert(pObj->nsHandle != NULL);

    UTILS_assert(pObj->srObj != NULL);
    srId = SharedRegion_getId(pObj->srObj);
    UTILS_assert(srId == SYSTEM_IPC_SR_AVSYNC);
    pObj->srObjSrPtr = SharedRegion_getSRPtr(pObj->srObj,srId);

    pObj->nsKey =
      NameServer_addUInt32(pObj->nsHandle,AVSYNC_SROBJ_NAME,
                           pObj->srObjSrPtr);
    if (pObj->nsKey == NULL)
    {
        status = AVSYNC_E_FAIL;
    }
    return status;
}

static Void avsync_open_srobj(AvsyncLink_Obj *pObj,
                              Avsync_InitParams *initParams)
{
    Int status;
    UInt16 queryList[] = {AVSYNC_SROBJ_CREATE_PROCID, MultiProc_INVALIDID};

    pObj->nsHandle = NameServer_getHandle(AVSYNC_NAMESERVERNAME_RTOS);
    if(pObj->nsHandle == NULL)
    {
        NameServer_Params nameServerParams;

        /* Get the default params for  the Name server */
        NameServer_Params_init(&nameServerParams);
        nameServerParams.maxRuntimeEntries = AVSYNC_MAX_NUM_INSTANCES; /* max instances on this processor */
        nameServerParams.maxValueLen       = sizeof (UInt32);
        /* Create the Name Server instance */
        pObj->nsHandle = NameServer_create (AVSYNC_NAMESERVERNAME_RTOS,
                                            &nameServerParams);
        UTILS_assert(pObj->nsHandle != NULL);
    }
    status =
      NameServer_getUInt32(pObj->nsHandle,
                           AVSYNC_SROBJ_NAME,
                           &pObj->srObjSrPtr,
                           queryList);
    UTILS_assert(NameServer_S_SUCCESS == status);
    pObj->srObj = SharedRegion_getPtr(pObj->srObjSrPtr);
    pObj->gateMPSrMemPtr = SharedRegion_getPtr(pObj->srObj->gateMPAddr);
    status = GateMP_openByAddr(pObj->gateMPSrMemPtr,&pObj->gate);
    UTILS_assert(GateMP_S_SUCCESS == status);
    avsync_init_wall_timer(pObj,initParams->wallTimeInitTS);
}

static Void avsync_close_srobj(AvsyncLink_Obj *pObj)
{
    Int status;

    GateMP_close(&pObj->gate);
    status = NameServer_delete(&pObj->nsHandle);
    UTILS_assert(NameServer_S_SUCCESS == status);
    avsync_uninit_wall_timer(pObj);
}

static Void avsync_init_linkobj(AvsyncLink_Obj *pObj)
{
    Int i;

    for (i = 0; i < UTILS_ARRAYSIZE(pObj->displayId2vidSyncLinkMap); i++)
    {
        pObj->displayId2vidSyncLinkMap[i] = SYSTEM_LINK_ID_INVALID;
    }
    pObj->gate = NULL;
    pObj->gateMPSrMemPtr = NULL;
    pObj->gateMPSrMemSize = 0;
    pObj->nsHandle = NULL;
    pObj->nsKey    = NULL;
    pObj->srObj = NULL;
    pObj->srObjSrPtr = IPC_LINK_INVALID_SRPTR;
    for (i = 0; i < UTILS_ARRAYSIZE(pObj->displayId2audDevMap); i++)
    {
        pObj->displayId2audDevMap[i] = AVSYNC_INVALID_AUDDEVID;
    }
    for (i = 0; i < UTILS_ARRAYSIZE(pObj->displayObj); i++)
    {
        pObj->displayObj[i].vSyncSTC = AVSYNC_INVALID_PTS;
        pObj->displayObj[i].videoBackendDelay = AVSYNC_VIDEO_BACKEND_DELAY_MS;
        pObj->displayObj[i].cbHookObj.cbFxn  = NULL;
        pObj->displayObj[i].cbHookObj.ctx    = NULL;
        
    }
    for (i = 0; i < UTILS_ARRAYSIZE(pObj->syncMasterChNum);i++)
    {
        pObj->syncMasterChNum[i] = AVSYNC_INVALID_CHNUM;
    }
    memset(&pObj->wallTimerUpdateObj,0,sizeof(pObj->wallTimerUpdateObj));
    UTILS_COMPILETIME_ASSERT(UTILS_ARRAYSIZE(pObj->cfgMutex) ==
                             UTILS_ARRAYSIZE(pObj->cfgMutexMem));
    for (i = 0; i < UTILS_ARRAYSIZE(pObj->cfgMutex);i++)
    {
        ti_sysbios_gates_GateMutexPri_Params prms;
        ti_sysbios_gates_GateMutexPri_Params_init(&prms);

        ti_sysbios_gates_GateMutexPri_construct(&pObj->cfgMutexMem[i],
                                                &prms);
        pObj->cfgMutex[i] =
        ti_sysbios_gates_GateMutexPri_handle(&pObj->cfgMutexMem[i]);
    }
}

static Void avsync_deinit_linkobj(AvsyncLink_Obj *pObj)
{
    Int i;

    for (i = 0; i < UTILS_ARRAYSIZE(pObj->cfgMutex);i++)
    {
        pObj->cfgMutex[i] = NULL;
        ti_sysbios_gates_GateMutexPri_destruct(&pObj->cfgMutexMem[i]);
    }
}

static
Int32 avsync_validate_synccfgch_params(Avsync_SynchConfigParams *cfg)
{
    Int32 status = AVSYNC_S_OK;

    UTILS_assertError((cfg->chNum < AVSYNC_MAX_CHANNELS_PER_DISPLAY),
                      status,
                      AVSYNC_E_INVALIDPARAMS,
                      SYSTEM_LINK_ID_AVSYNC,
                      -1);
    return status;
}

static
Int32 avsync_validate_synchcfg_params(AvsyncLink_LinkSynchConfigParams *cfg)
{
    Int32 status = AVSYNC_S_OK;


    UTILS_assertError(((Avsync_mapDisplayLinkID2Index(cfg->displayLinkID) < AVSYNC_MAX_NUM_DISPLAYS)
                      &&
                      (cfg->numCh < AVSYNC_MAX_CHANNELS_PER_DISPLAY)
                      &&
                      (cfg->videoSynchLinkID != SYSTEM_LINK_ID_INVALID)
                      &&
                      ((cfg->syncMasterChnum < AVSYNC_MAX_CHANNELS_PER_DISPLAY)
                       ||
                       (AVSYNC_INVALID_CHNUM == cfg->syncMasterChnum))),
                      status,
                      AVSYNC_E_INVALIDPARAMS,
                      SYSTEM_LINK_ID_AVSYNC,
                      -1);
    if (!UTILS_ISERROR(status))
    {
        Int i;

        for (i = 0; i < cfg->numCh; i++)
        {
            Avsync_SynchConfigParams *vidQueCfg;

            vidQueCfg = &cfg->queCfg[i];
            status = avsync_validate_synccfgch_params(vidQueCfg);
            if (status != AVSYNC_S_OK)
            {
                break;
            }
        }
    }
    return status;
}

static
Int32 avsync_set_avsynccfg(AvsyncLink_Obj *pObj,
                           AvsyncLink_LinkSynchConfigParams *cfg)
{
    Int32 status;
    Int i;
    Avsync_SynchConfigParams *vidQueCfg;

    status = avsync_validate_synchcfg_params(cfg);

    if (status == AVSYNC_S_OK)
    {
         UTILS_assertError(((pObj->srObj->numAudQueCreated == 0)
                            &&
                           (pObj->srObj->numVidQueCreated[Avsync_mapDisplayLinkID2Index(cfg->displayLinkID)] == 0)),
                           status,
                           AVSYNC_E_INVALIDPARAMS,
                           SYSTEM_LINK_ID_AVSYNC,
                           -1);
    }
    if (status == AVSYNC_S_OK)
    {
        AVSYNC_CRITICAL_BEGIN();
        pObj->displayId2vidSyncLinkMap[Avsync_mapDisplayLinkID2Index(cfg->displayLinkID)] =
            cfg->videoSynchLinkID;
        pObj->displayId2audDevMap[Avsync_mapDisplayLinkID2Index(cfg->displayLinkID)] =
            cfg->audioDevId;
        pObj->syncMasterChNum[Avsync_mapDisplayLinkID2Index(cfg->displayLinkID)] =
            cfg->syncMasterChnum;
        for (i = 0; i < cfg->numCh; i++)
        {
            vidQueCfg = &cfg->queCfg[i];
            pObj->srObj->queCfg[Avsync_mapDisplayLinkID2Index(cfg->displayLinkID)][vidQueCfg->chNum] =
                *vidQueCfg;
        }
        AVSYNC_CRITICAL_END();
    }
    return status;
}

Void avsync_init_cfgparams(AvsyncLink_Obj *pObj)
{
    UInt32 devId;
    AvsyncLink_LinkSynchConfigParams synchCfg;

    /* Configure AvsyncConfig with default values */
    for (devId = 0;devId < UTILS_ARRAYSIZE(pObj->srObj->queCfg);devId++)
    {
        synchCfg.audioDevId       = AVSYNC_INVALID_AUDDEVID;
        synchCfg.displayLinkID    = SYSTEM_LINK_ID_DISPLAY_FIRST + devId;
        synchCfg.videoSynchLinkID = SYSTEM_LINK_ID_SW_MS_MULTI_INST_0 +
                                    devId;
        if (synchCfg.videoSynchLinkID >  SYSTEM_LINK_ID_SW_MS_END)
        {
            synchCfg.videoSynchLinkID = SYSTEM_LINK_ID_INVALID;
        }
        synchCfg.numCh = 0;
        synchCfg.syncMasterChnum = AVSYNC_INVALID_CHNUM;
        avsync_set_avsynccfg(pObj,&synchCfg);
    }
}


static Void avsync_init_rtos(AvsyncLink_Obj *pObj,
                             Avsync_InitParams *initParams)
{
    Int status;

    if (FALSE == pObj->initDone)
    {
#ifdef SYSTEM_DEBUG_MEMALLOC
        Vps_printf ("Before AVSYNC Create:\n");
        System_memPrintHeapStatus();
#endif

        avsync_init_linkobj(pObj);
        if (AVSYNC_SROBJ_CREATE_PROCID == System_getSelfProcId())
        {
            /* Create Gate */
            status = avsync_create_gatemp(pObj);
            UTILS_assert(AVSYNC_S_OK == status);
            /* Alloc SrObj */
            status = avsync_create_srobj(pObj);
            UTILS_assert(AVSYNC_S_OK == status);
            /* init SrObj */
            avsync_init_srobj(pObj,initParams);
            //avsync_init_cfgparams(pObj);
            /* Register with Nameserver */
            status = avsync_register_srobj(pObj);
            UTILS_assert(AVSYNC_S_OK == status);
        }
        else
        {
            avsync_open_srobj(pObj,initParams);
        }
        pObj->initDone = TRUE;
#ifdef SYSTEM_DEBUG_MEMALLOC
        Vps_printf ("After AVSYNC Create:\n");
        System_memPrintHeapStatus();
#endif
    }
}

static Void avsync_uninit(AvsyncLink_Obj *pObj)
{
    if (pObj->initDone)
    {
        if (AVSYNC_SROBJ_CREATE_PROCID == System_getSelfProcId())
        {
            /* UnRegister with Nameserver */
            avsync_unregister_srobj(pObj);
            /* Free SrObj */
            avsync_delete_srobj(pObj);
            /* Delete Gate */
            avsync_delete_gatemp(pObj,AVSYNC_RSRC_GATEMP_ALL);
        }
        else
        {
            avsync_close_srobj(pObj);
        }
        avsync_deinit_linkobj(pObj);
        pObj->initDone = FALSE;
    }
}

UInt64 Avsync_getWallTime()
{
    UTILS_assert(TRUE == gAvsyncLink_obj.initDone);
    return (gAvsyncLink_obj.srObj->wallTimer.curWallTime);
}


static Void avsync_log_add_master_ch_info(UInt64 renderTS,UInt64 framePTS,
                                          Int32  clkAdjustDelta,
                                          AVSYNC_FRAME_RENDER_FLAG renderFlag,
                                          UInt32 displayID)
{
#ifdef SYSTEM_DEBUG_AVSYNC_DETAILED_LOGS
    Avsync_logVidSynchTbl *syncTbl = AVSYNC_GET_LOG_TBL_HANDLE();
    Avsync_logVidMasterRecord *masterRec;

    UTILS_assert(syncTbl[displayID].masterRec.index < AVSYNC_LOG_VIDSYNC_INFO_MAX_RECORDS);
    masterRec = &syncTbl[displayID].masterRec.rec[syncTbl[displayID].masterRec.index];
    masterRec->renderPTS = renderTS;
    masterRec->synchMasterPTS = framePTS;
    masterRec->clkAdjustDelta = clkAdjustDelta;
    masterRec->renderFlag     = renderFlag;
    syncTbl[displayID].masterRec.index = (syncTbl[displayID].masterRec.index + 1) %
                                          AVSYNC_LOG_VIDSYNC_INFO_MAX_RECORDS;
#else
    (Void)renderTS;
    (Void)framePTS;
    (Void)clkAdjustDelta;
    (Void)renderFlag;
    (Void)displayID;
#endif
}

static Void avsync_log_add_slave_ch_info(UInt32 slaveChNum,
                                         UInt64 framePTS,
                                         UInt64 synchMasterPTS,
                                         AVSYNC_FRAME_RENDER_FLAG renderFlag,
                                         Bool underRun,
                                         UInt32 displayID)
{
#ifdef SYSTEM_DEBUG_AVSYNC_DETAILED_LOGS
    Avsync_logVidSynchTbl *syncTbl = AVSYNC_GET_LOG_TBL_HANDLE();
    Avsync_logVidSlaveRecord *slaveRec;
    UInt32 index;

    slaveChNum %= AVSYNC_LOG_MAX_SLAVE_CHANNELS;
    UTILS_assert(syncTbl[displayID].slaveRec[slaveChNum].index < AVSYNC_LOG_VIDSYNC_INFO_MAX_RECORDS);
    index = syncTbl[displayID].slaveRec[slaveChNum].index;
    slaveRec = &syncTbl[displayID].slaveRec[slaveChNum].rec[index];

    slaveRec->syncMasterPTSDelta = framePTS - synchMasterPTS;
    slaveRec->renderFlag         = renderFlag;
    slaveRec->underRun           = underRun;
    syncTbl[displayID].slaveRec[slaveChNum].index = (syncTbl[displayID].slaveRec[slaveChNum].index + 1) %
                                                     AVSYNC_LOG_VIDSYNC_INFO_MAX_RECORDS;
#else
    (Void)slaveChNum;
    (Void)framePTS;
    (Void)renderFlag;
    (Void)underRun;
    (Void)displayID;
#endif
}

static
UInt32 avsync_map_linkid2displayid(AvsyncLink_Obj *pObj,
                                   UInt32 vidSynchLinkID)
{
    Int i;
    UInt32 displayID = AVSYNC_INVALID_DISPLAY_ID;

    for (i = 0; i < UTILS_ARRAYSIZE(pObj->displayId2vidSyncLinkMap);i++)
    {
        if (pObj->displayId2vidSyncLinkMap[i] == vidSynchLinkID)
        {
            break;
        }
    }
    if (i < UTILS_ARRAYSIZE(pObj->displayId2vidSyncLinkMap))
    {
        displayID = i;
    }
    return displayID;
}

static
UInt32 avsync_map_auddev2displayid(AvsyncLink_Obj *pObj,
                                   UInt32 audioDevID)
{
    Int i;
    UInt32 displayID = AVSYNC_INVALID_DISPLAY_ID;

    for (i = 0; i < UTILS_ARRAYSIZE(pObj->displayId2audDevMap);i++)
    {
        if (pObj->displayId2audDevMap[i] == audioDevID)
        {
            break;
        }
    }
    if (i < UTILS_ARRAYSIZE(pObj->displayId2audDevMap))
    {
        displayID = i;
    }
    return displayID;
}


static Int avsync_vidquecreate_validate_params(AvsyncLink_Obj *pObj,
                                               AvsyncLink_VidQueCreateParams *cp)
{
    Int status = AVSYNC_S_OK;

    UTILS_assertError(((cp->queueMem != NULL)
                       &&
                       (cp->maxElements > 0)
                       &&
                       (cp->chNum < AVSYNC_MAX_CHANNELS_PER_DISPLAY)),
                       status,
                       AVSYNC_E_INVALIDPARAMS,
                       SYSTEM_LINK_ID_AVSYNC,
                       AVSYNC_INVALID_CHNUM);

    return status;
}


static Int avsync_vidquecreate_avsynccfg_validate_params(AvsyncLink_Obj *pObj,
                                                         AvsyncLink_VidQueCreateParams *cp)
{

    Int status = AVSYNC_S_OK;

    UTILS_assertError(((avsync_map_linkid2displayid(pObj,cp->syncLinkID) !=
                         AVSYNC_INVALID_DISPLAY_ID)
                       ||
                       (cp->displayID != AVSYNC_INVALID_DISPLAY_ID)),
                       status,
                       AVSYNC_E_INVALIDPARAMS,
                       SYSTEM_LINK_ID_AVSYNC,
                       AVSYNC_INVALID_CHNUM);
    return status;
}


Void Avsync_vidQueCreateParamsInit(AvsyncLink_VidQueCreateParams *cp)
{
    memset(cp,0,sizeof(*cp));
    cp->displayID = AVSYNC_INVALID_DISPLAY_ID;
}

static Void avsync_vidstats_reset(Avsync_VidStats *stats)
{
    UTILS_assert(stats != NULL);
    memset(stats,0,sizeof(*stats));
}

static Void avsync_vidque_create_register(AvsyncLink_Obj *pObj,
                                          UInt32    displayID)
{
    AVSYNC_CRITICAL_BEGIN();

    pObj->srObj->numVidQueCreated[displayID]++;

    AVSYNC_CRITICAL_END();
}

Int Avsync_vidQueCreate(AvsyncLink_VidQueCreateParams *cp,
                        AvsyncLink_VidQueObj *queObj)
{
    Int status = AVSYNC_S_OK;
    AvsyncLink_Obj *pObj = &gAvsyncLink_obj;
    Bool forceDisableAvsync = FALSE;

    UTILS_assert(TRUE == pObj->initDone);
    status = avsync_vidquecreate_validate_params(pObj,
                                                 cp);
    if (!UTILS_ISERROR(status))
    {
        queObj->cp = *cp;
        status = Utils_queCreate(&queObj->vidFrmQ,
                                 queObj->cp.maxElements,
                                 queObj->cp.queueMem,
                                 UTILS_QUE_FLAG_NO_BLOCK_QUE);
    }

    if (!UTILS_ISERROR(status))
    {
        status = avsync_vidquecreate_avsynccfg_validate_params(pObj,
                                                               cp);
        if (UTILS_ISERROR(status))
        {
            /* Avsync configuration is not done for this link.Default to
             * AVSYNC disabled
             */
            Vps_printf("AVSYNC:WARNING!!!.AVSYNC config invalid for linkID[%x]:chId[%d] "
                       "Will Default to AVSYNC disabled",
                       cp->syncLinkID,
                       cp->chNum);
            queObj->cfg = (Avsync_SynchConfigParams *)&gAvsyncDisableParam;
            queObj->displayID = AVSYNC_INVALID_DISPLAY_ID;
            forceDisableAvsync = TRUE;
        }
    }
    if (!UTILS_ISERROR(status))
    {
        UInt32 displayID = avsync_map_linkid2displayid(pObj,cp->syncLinkID);
        if (cp->displayID != AVSYNC_INVALID_DISPLAY_ID)
        {
            if (cp->displayID != displayID)
            {
                Vps_printf("\n"
                           "AVSYNC:WARNING!! Application wrongly configured"
                           "displayID[%d]. Reseting to correct displayID[%d]\n",
                           displayID,cp->displayID);
                pObj->displayId2vidSyncLinkMap[cp->displayID] = cp->syncLinkID;
                displayID = avsync_map_linkid2displayid(pObj,cp->syncLinkID);
                UTILS_assert(displayID != AVSYNC_INVALID_DISPLAY_ID);
            }
        }

        queObj->cfg = &pObj->srObj->queCfg[displayID][cp->chNum];
        if (AVSYNC_INVALID_CHNUM != queObj->cfg->chNum)
        {
            UTILS_assert(queObj->cfg->chNum == cp->chNum);
            UTILS_assert(pObj->srObj != NULL);
            UTILS_assert(displayID != AVSYNC_INVALID_DISPLAY_ID);
            queObj->displayID = displayID;
            queObj->playerTime
              = &pObj->srObj->playerTime[queObj->displayID][cp->chNum];
            queObj->syncMasterChnum = pObj->syncMasterChNum[queObj->displayID];
            UTILS_assert(queObj->playerTime->state == AVSYNC_PLAYERTIME_STATE_INIT);
            AVSYNC_CRITICAL_BEGIN();
            if (queObj->playerTime->doInit)
            {
                avsync_init_player_timer(queObj->playerTime);
                queObj->playerTime->doInit = FALSE;
            }
            if (queObj->playerTime->playState == AVSYNC_PLAYER_STATE_RESET)
            {
                queObj->playerTime->playState = AVSYNC_PLAYER_STATE_PLAY;
            }
            if (queObj->cfg->clkAdjustPolicy.refClkType == AVSYNC_REFCLKADJUST_NONE)
            {
                queObj->refClk = NULL;
            }
            else
            {
                queObj->refClk     = &pObj->srObj->refClk[queObj->displayID];
                if (queObj->refClk->doInit)
                {
                    avsync_init_refclkobj(queObj->refClk);
                    queObj->refClk->doInit = FALSE;
                }
            }
            AVSYNC_CRITICAL_END();
            queObj->stats
              = &pObj->srObj->vidstats[queObj->displayID][cp->chNum];
            avsync_vidstats_reset(queObj->stats);
        }
        else
        {
            UTILS_assert(displayID != AVSYNC_INVALID_DISPLAY_ID);
            queObj->displayID = displayID;
            queObj->cfg->avsyncEnable = FALSE;
        }
    }
    if (!UTILS_ISERROR(status))
    {
        if (queObj->cfg->avsyncEnable)
        {
            if (AVSYNC_IS_MASTER_SYNC_CHANNEL(queObj->syncMasterChnum,cp->chNum))
            {
                avsync_refclk_register_master_playertime(&pObj->srObj->refClk[queObj->displayID],
                                                         queObj->playerTime,
                                                         queObj->syncMasterChnum);
            }
            else
            {
                if (queObj->cfg->clkAdjustPolicy.refClkType != AVSYNC_REFCLKADJUST_NONE)
                {
                    avsync_refclk_register_sync_playertime(&pObj->srObj->refClk[queObj->displayID],
                                                       queObj->playerTime);
                }
            }
       }
    }
    if (!UTILS_ISERROR(status))
    {
        queObj->displayObj = &pObj->displayObj[queObj->displayID];
        queObj->playTimerStartTimeout = queObj->cfg->playTimerStartTimeout;
        #if (AVSYNC_APPLY_PTS_SMOOTHING_FILTER)
        avsync_init_smavgobj(&queObj->ptsDiffAvg);
        #endif
        AVSYNC_VIDQUE_INIT_STATE(queObj->state);
        AVSYNC_VIDQUE_SET_STATE(queObj->state,AVSYNC_VIDQUE_STATE_CREATED);
        avsync_vidque_create_register(pObj,queObj->displayID);
    }
    if (UTILS_ISERROR(status) && forceDisableAvsync)
    {
        status = AVSYNC_S_OK;
    }
    return status;
}

static Void avsync_vidque_delete_register(AvsyncLink_Obj *pObj,
                                          UInt32 displayID)
{
    UTILS_assert(pObj->srObj->numVidQueCreated[displayID] > 0);

    AVSYNC_CRITICAL_BEGIN();
    pObj->srObj->numVidQueCreated[displayID] -= 1;
    AVSYNC_CRITICAL_END();
}

Int Avsync_vidQueDelete(AvsyncLink_VidQueObj *queObj)
{
    Int status = AVSYNC_S_OK;

    if (queObj->displayID != AVSYNC_INVALID_DISPLAY_ID)
    {
        avsync_vidque_delete_register(&gAvsyncLink_obj,queObj->displayID);
    }

    UTILS_assert(queObj->cfg != NULL);
    if (queObj->cfg->avsyncEnable)
    {
        if (AVSYNC_IS_MASTER_SYNC_CHANNEL(queObj->syncMasterChnum,queObj->cfg->chNum)
            &&
            (queObj->cfg->clkAdjustPolicy.refClkType != AVSYNC_REFCLKADJUST_NONE))
        {
            avsync_refclk_unregister_master_playertime(queObj->refClk,
                                                       queObj->playerTime);
        }
        else
        {
            if (queObj->cfg->clkAdjustPolicy.refClkType != AVSYNC_REFCLKADJUST_NONE)
            {
                avsync_refclk_unregister_sync_playertime(queObj->refClk,
                                                         queObj->playerTime);
            }
        }
    }
    status = Utils_queDelete(&queObj->vidFrmQ);
    UTILS_assert(status == 0);
    queObj->cfg->chNum = AVSYNC_INVALID_CHNUM;
    AVSYNC_VIDQUE_INIT_STATE(queObj->state);
    if (queObj->playerTime)
    {
        queObj->playerTime->doInit = TRUE;
    }

    return status;

}

static
UInt64 avsync_get_frame_pts(FVID2_Frame *frame)
{
    System_FrameInfo *frameInfo = frame->appData;
    UInt64 pts;

    if (frameInfo != NULL)
    {
        pts = frameInfo->ts64;
    }
    else
    {
        pts = AVSYNC_INVALID_PTS;
    }
    return pts;
}

static
Bool avsync_is_seqid_match(FVID2_Frame *frame,UInt32 curSeqId)
{
    System_FrameInfo *frameInfo = frame->appData;
    Bool seqIdMatch = TRUE;

    if (frameInfo != NULL)
    {
        if (frameInfo->seqId != curSeqId)
        {
            seqIdMatch = FALSE;
        }
    }
    return seqIdMatch;
}

Int Avsync_vidQuePut(AvsyncLink_VidQueObj *queObj,
                     FVID2_Frame *frame)
{
    Int status;


    status =
    Utils_quePut(&queObj->vidFrmQ,frame,ti_sysbios_BIOS_NO_WAIT);

    UTILS_assert((status == 0) && (queObj->cfg != NULL));
    if (AVSYNC_INVALID_DISPLAY_ID != queObj->displayID)
    {
        AVSYNC_CFG_MUTEX_ACQUIRE(queObj->displayID);
        if (queObj->cfg->avsyncEnable)
        {
            AVSYNC_VIDQUE_SET_STATE(queObj->state,AVSYNC_VIDQUE_STATE_FIRSTFRMRECEIVED);
            if (!queObj->playerTime->firstFrmInSeqReceived)
            {
                if (AVSYNC_PTS_INIT_MODE_AUTO == queObj->cfg->ptsInitMode)
                {
                    UInt64 pts;

                    pts = avsync_get_frame_pts(frame);
                    if (pts != AVSYNC_INVALID_PTS)
                    {
                        UTILS_assert(avsync_get_player_time_firstvidpts(queObj->playerTime)
                                     ==
                                     AVSYNC_INVALID_PTS);
                        avsync_set_player_time_firstvidpts(queObj->playerTime,pts);
                        queObj->playerTime->firstFrmInSeqReceived = TRUE;
                    }
                }
                else
                {
                    /* If app set PTS then mark first frame as received only if a
                     * PTS within AVSYNC_FIRST_PTS_MAX_DELTA_MS range of
                     * firstVidPTS is received.
                     * This ensures playback starts with the frames marked as
                     * startPTS by application
                     */
                    UInt64 pts;
                    UInt64 startPTS = avsync_get_player_time_firstvidpts(queObj->playerTime);
                    Int32 startPTSDelta;

                    pts = avsync_get_frame_pts(frame);
                    if ((pts != AVSYNC_INVALID_PTS) && (startPTS != AVSYNC_INVALID_PTS))
                    {
                        if (pts > startPTS)
                        {
                            startPTSDelta = pts - startPTS;
                            if (startPTSDelta <= AVSYNC_FIRST_PTS_MAX_DELTA_MS)
                            {
                                queObj->playerTime->firstFrmInSeqReceived = TRUE;
                            }
                        }
                    }
                }
            }
        }
        AVSYNC_CFG_MUTEX_RELEASE(queObj->displayID);
    }
    return status;
}

static Void avsync_adjust_synched_clks(Avsync_RefClkObj *refClk,
                                       Int32 clkAdjustDelta)
{

    Int i;
    Avsync_PlayerTimeObj *synchedChPlayerTime;

    for (i = 0; i < refClk->numSynchedChannels; i++)
    {
        synchedChPlayerTime = refClk->synchChannelList[i];
        avsync_set_player_time_refclk_delta(synchedChPlayerTime,
                                            clkAdjustDelta);
    }
}

#define AVSYNC_ADJUST_REFCLK_WITH_PTS
#ifdef AVSYNC_ADJUST_REFCLK_WITH_PTS
static
Int32 avsync_adjust_refclk(Avsync_RefClkObj *refClk,UInt64 newMediaBaseTS)
{
    Avsync_PlayerTimeObj *refPlayerTime;
    Int32 clkAdjustDelta;

    AVSYNC_CRITICAL_BEGIN();
    refPlayerTime = refClk->masterPlayerTime;
    UTILS_assert(refPlayerTime->state == AVSYNC_PLAYERTIME_STATE_RUNNING);
    avsync_set_player_timer (refPlayerTime,
                             newMediaBaseTS,
                             refPlayerTime->scalingFactor,
                             &clkAdjustDelta);
    Vps_printf("AVSYNC:avsync_adjust_refclk:"
               " Clock Adjust delta[%d]\n",
               clkAdjustDelta);
    avsync_adjust_synched_clks(refClk,clkAdjustDelta);
    AVSYNC_CRITICAL_END();
    return clkAdjustDelta;
}
#else
static
Void avsync_adjust_refclk(Avsync_RefClkObj *refClk,Int32 clkAdjustDelta)
{
    Avsync_PlayerTimeObj *refPlayerTime;
    UInt64 curTime;

    AVSYNC_CRITICAL_BEGIN();
    refPlayerTime = refClk->masterPlayerTime;
    UTILS_assert(refPlayerTime->state == AVSYNC_PLAYERTIME_STATE_RUNNING);
    if (clkAdjustDelta < 0)
    {
        curTime = avsync_get_player_timer(refPlayerTime,Avsync_getWallTime());
        if (curTime < -clkAdjustDelta)
        {
            clkAdjustDelta = -curTime;
            Vps_printf("AVSYNC:avsync_adjust_refclk:"
                       " Clock Adjust delta adjusted to avoid wrap [%d]\n",
                       clkAdjustDelta);
        }
    }

    avsync_set_player_time_refclk_delta(refPlayerTime,clkAdjustDelta);
    Vps_printf("AVSYNC:avsync_adjust_refclk:"
               " Clock Adjust delta[%d]\n",
               clkAdjustDelta);
    avsync_adjust_synched_clks(refClk,clkAdjustDelta);
    AVSYNC_CRITICAL_END();
}
#endif

static AVSYNC_FRAME_RENDER_FLAG avsync_get_vidframe_render_flag(Avsync_PlayerTimeObj *playerTime,
                                                                Avsync_VidStats *vidStats,
                                                                Int32 delta,
                                                                Avsync_vidSchedulePolicy *avsyncPolicy)
{
    AVSYNC_FRAME_RENDER_FLAG renderFlag = AVSYNC_RenderFrame;
    Int32 maxLead;
    Int32 maxLag;
    Int32 playThreshold;

    maxLead = avsyncPolicy->maxReplayLead;
    maxLag  = avsyncPolicy->playMaxLag;
    playThreshold = avsyncPolicy->playMaxLead;

    if (avsyncPolicy->doMarginScaling)

    {
        if (playerTime->scalingFactor > 1.0)
        {
            maxLag *= playerTime->scalingFactor;
            playThreshold *= playerTime->scalingFactor;
        }
    }


    if ((delta < -playThreshold) && (delta > -maxLead))
    {
        renderFlag = AVSYNC_RepeatFrame;
        vidStats->numFramesReplayed++;
    }
    else
    {
        if ((delta >= -playThreshold) && (delta < maxLag))
        {
            renderFlag = AVSYNC_RenderFrame;
            vidStats->numFramesRendered++;
        }
        else
        {
            renderFlag = AVSYNC_DropFrame;
            if (delta > maxLag)
            {
                vidStats->numFramesSkippedLate++;
            }
            else
            {
                vidStats->numFramesSkippedEarly++;
            }
            //Vps_printf("AVSYNC:FrameSkip:Delta[%d]\n",delta);
        }
    }
    return renderFlag;
}

static Bool avsync_is_timebase_shift(Avsync_refClkAdjustPolicy *clkAdjustPolicy,Int32 delta)
{
    Bool timeBaseShifted = FALSE;

    if ((delta < -clkAdjustPolicy->clkAdjustLead)
        ||
        (delta > clkAdjustPolicy->clkAdjustLag))
    {
        timeBaseShifted = TRUE;
    }
    return timeBaseShifted;
}

static AVSYNC_FRAME_RENDER_FLAG avsync_synch_vidframe(AvsyncLink_VidQueObj *queObj,
                                                      UInt64 framePTS,
                                                      UInt64 renderTS)
{
    AVSYNC_FRAME_RENDER_FLAG renderFlag;
    Int32 delta,clkAdjustDelta = 0;
    Bool underRun = FALSE;


    UTILS_assert((queObj->cfg) && (queObj->cfg->avsyncEnable));
    delta = AVSYNC_GET_PTS_DELTA(renderTS,framePTS);
    #if (AVSYNC_APPLY_PTS_SMOOTHING_FILTER)
    delta = avsync_get_simple_moving_avg(&queObj->ptsDiffAvg,delta);
    #endif

    switch(queObj->playerTime->playState)
    {
        case AVSYNC_PLAYER_STATE_PLAY:
        case AVSYNC_PLAYER_STATE_TIMESCALE_PLAY:
        {
            if ((AVSYNC_REFCLKADJUST_NONE == queObj->cfg->clkAdjustPolicy.refClkType)
                &&
                avsync_is_timebase_shift(&queObj->cfg->clkAdjustPolicy,
                                         delta))
            {
                avsync_set_player_timer (queObj->playerTime,
                             framePTS,
                             queObj->playerTime->scalingFactor,
                             &clkAdjustDelta);

                #if (AVSYNC_APPLY_PTS_SMOOTHING_FILTER)
                avsync_init_smavgobj(&queObj->ptsDiffAvg);
                #endif
                renderFlag = AVSYNC_RenderFrame;
                Vps_printf("AVSYNC:RefCLk Adjust .Delta[%d],AjustPTS[%d],MaxLead[%d],MaxLag[%d]\n",
                           delta,
                           ((UInt32)framePTS),
                           queObj->cfg->vidSynchPolicy.maxReplayLead,
                           queObj->cfg->vidSynchPolicy.playMaxLag);
            }
            else
            {
                renderFlag =
                avsync_get_vidframe_render_flag(queObj->playerTime,
                                                queObj->stats,
                                                delta,
                                                &queObj->cfg->vidSynchPolicy);
                if ((AVSYNC_DropFrame == renderFlag)
                    &&
                    (Utils_queGetQueuedCount(&queObj->vidFrmQ) == 1))
                {
                    renderFlag = AVSYNC_RenderFrame;
                    queObj->stats->numUnderrun++;
                    underRun = TRUE;
                    if (AVSYNC_IS_MASTER_SYNC_CHANNEL(queObj->syncMasterChnum,queObj->cfg->chNum)
                        &&
                        (queObj->cfg->clkAdjustPolicy.refClkType == AVSYNC_REFCLKADJUST_BYVIDEO)
                        &&
                        (queObj->playerTime->playState == AVSYNC_PLAYER_STATE_PLAY))
                    {
                        UTILS_assert(queObj->playerTime == queObj->refClk->masterPlayerTime);
                        Vps_printf("AVSYNC:RefCLk Adjust .Delta[%d],AjustPTS[%d],MaxLead[%d],MaxLag[%d]\n",
                                   delta,
                                   ((UInt32)framePTS),
                                   queObj->cfg->vidSynchPolicy.maxReplayLead,
                                   queObj->cfg->vidSynchPolicy.playMaxLag);
                        #ifdef AVSYNC_ADJUST_REFCLK_WITH_PTS
                        clkAdjustDelta =
                        avsync_adjust_refclk(queObj->refClk,framePTS);
                        #else
                        avsync_adjust_refclk(queObj->refClk,delta);
                        clkAdjustDelta = delta;
                        #endif
                    }
                    renderFlag = AVSYNC_RenderFrame;
                }
            }
            break;
        }
        case AVSYNC_PLAYER_STATE_PAUSE:
            renderFlag = AVSYNC_RepeatFrame;
            break;
        case AVSYNC_PLAYER_STATE_STEP_FWD:
            AVSYNC_CRITICAL_BEGIN();
            if (queObj->playerTime->takeNextStep)
            {
                renderFlag = AVSYNC_RenderFrame;
                queObj->playerTime->takeNextStep = FALSE;
                avsync_set_player_timer(queObj->playerTime,
                                        framePTS,
                                        queObj->playerTime->scalingFactor,
                                        &clkAdjustDelta);
            }
            else
            {
                renderFlag = AVSYNC_RepeatFrame;
            }
            AVSYNC_CRITICAL_END();
            break;
        case AVSYNC_PLAYER_STATE_SCAN:
            AVSYNC_CRITICAL_BEGIN();
            if (AVSYNC_INVALID_PTS !=
                queObj->playerTime->lastVidFrmPTS)
            {
                Int32 displayDuration;

                displayDuration =
                AVSYNC_GET_PTS_DELTA(renderTS,
                                     queObj->playerTime->lastVidFrmPTS);
                UTILS_assert(displayDuration > 0);
                if (displayDuration >=
                    queObj->playerTime->scanFrameDisplayDuration)
                {
                    renderFlag = AVSYNC_RenderFrame;
                    avsync_set_player_timer(queObj->playerTime,
                                            framePTS,
                                            queObj->playerTime->scalingFactor,
                                            &clkAdjustDelta);
                }
                else
                {
                    renderFlag = AVSYNC_RepeatFrame;
                }
            }
            else
            {
                renderFlag = AVSYNC_RenderFrame;
                avsync_set_player_timer(queObj->playerTime,
                                        framePTS,
                                        queObj->playerTime->scalingFactor,
                                        &clkAdjustDelta);
            }
            AVSYNC_CRITICAL_END();
            break;
        case AVSYNC_PLAYER_STATE_RESET:
            renderFlag = AVSYNC_DropFrame;
            break;
    }
    if (AVSYNC_IS_MASTER_SYNC_CHANNEL(queObj->syncMasterChnum,queObj->cfg->chNum))
    {
        avsync_log_add_master_ch_info(renderTS,framePTS,clkAdjustDelta,
                                      renderFlag,
                                      queObj->displayID);
    }
    else
    {
        UInt64 synchMasterPTS;

        if (queObj->refClk)
        {
            synchMasterPTS = queObj->refClk->masterPlayerTime->lastVidFrmPTS;
        }
        else
        {
            synchMasterPTS = renderTS;
        }
        avsync_log_add_slave_ch_info(queObj->cfg->chNum,
                             framePTS,
                             synchMasterPTS,
                             renderFlag,
                             underRun,
                             queObj->displayID);
    }
    return renderFlag;
}

static Bool avsync_handle_player_time_start(AvsyncLink_VidQueObj *queObj,
                                            FVID2_Frame *frame)
{
    Bool playerTimeStarted = FALSE;
    UInt64 curWallTime;

    if (AVSYNC_PLAYER_STATE_RESET != queObj->playerTime->playState)
    {
        if ((AVSYNC_IS_MASTER_SYNC_CHANNEL(queObj->syncMasterChnum,queObj->cfg->chNum))
            &&
            (AVSYNC_REFCLKADJUST_NONE != queObj->cfg->clkAdjustPolicy.refClkType))

        {
            Int i;

            UTILS_assert(queObj->playerTime == queObj->refClk->masterPlayerTime);
            if (!(queObj->cfg->audioPresent)
                ||
                ((queObj->cfg->audioPresent) && (0 == queObj->playTimerStartTimeout)))
            {
                Bool playerTimeStarted;

                curWallTime = Avsync_getWallTime();
                avsync_init_player_time_walltimebase(queObj->playerTime,curWallTime);
                playerTimeStarted = avsync_start_player_time(queObj->playerTime);
                UTILS_assert(playerTimeStarted == TRUE);
            }
            for (i = 0; i < queObj->refClk->numSynchedChannels;i++)
            {
                Avsync_PlayerTimeObj *playerTime = queObj->refClk->synchChannelList[i];
                avsync_init_player_time_walltimebase(playerTime,curWallTime);
                avsync_start_player_time(playerTime);
            }
        }
        else
        {
            if (AVSYNC_REFCLKADJUST_NONE == queObj->cfg->clkAdjustPolicy.refClkType)
            {
                curWallTime = Avsync_getWallTime();
                avsync_init_player_time_walltimebase(queObj->playerTime,curWallTime);
            }
            playerTimeStarted = avsync_start_player_time(queObj->playerTime);
        }
    }
    return playerTimeStarted;
}

static
UInt64 avsync_get_render_ts(AvsyncLink_VidQueObj *queObj)
{
    UInt64 renderTS;
    UInt64 lastSTCv;

    if (AVSYNC_IS_MASTER_SYNC_CHANNEL(queObj->syncMasterChnum,
                                       queObj->cp.chNum)
        &&
        (queObj->cfg->clkAdjustPolicy.refClkType != AVSYNC_REFCLKADJUST_NONE))
    {
        if (queObj->cfg->audioPresent)
        {
            lastSTCv = queObj->displayObj->vSyncSTC
                       +
                       queObj->displayObj->videoBackendDelay;
        }
        else
        {
            lastSTCv = Avsync_getWallTime();

        }
        UTILS_assert(queObj->refClk != NULL);
        queObj->refClk->lastSynchMasterVidRenderWallTS = lastSTCv;
    }
    else
    {
        if(AVSYNC_REFCLKADJUST_NONE == queObj->cfg->clkAdjustPolicy.refClkType)
        {
            lastSTCv = Avsync_getWallTime();
        }
        else
        {
            UTILS_assert(AVSYNC_REFCLKADJUST_BYVIDEO == queObj->cfg->clkAdjustPolicy.refClkType);
            lastSTCv = queObj->refClk->lastSynchMasterVidRenderWallTS;
            if (AVSYNC_INVALID_PTS == lastSTCv)
            {
                lastSTCv = Avsync_getWallTime();
            }
        }
    }
    renderTS =
     avsync_get_player_timer(queObj->playerTime,lastSTCv);
    return renderTS;
}

Int Avsync_vidQueGet(AvsyncLink_VidQueObj *queObj,
                     Bool  forceGet,
                     FVID2_Frame **framePtr,
                     FVID2_FrameList *freeFrameList)
{
    Int status = -1;
    UInt64 framePTS;
    UInt64 renderTS;

    *framePtr = NULL;
    if (forceGet || AVSYNC_IS_FORCE_DISABLE_CFG(queObj->cfg))
    {
        status = Utils_queGet(&queObj->vidFrmQ,
                              (Ptr *)framePtr,1,ti_sysbios_BIOS_NO_WAIT);
        return status;
    }

    AVSYNC_CFG_MUTEX_ACQUIRE(queObj->displayID);
    while (Utils_queGetQueuedCount(&queObj->vidFrmQ))
    {
        if (queObj->cfg->avsyncEnable)
        {
            FVID2_Frame *frame;
            AVSYNC_FRAME_RENDER_FLAG renderFlag;
            Bool seqIdMatch;
                
            status = Utils_quePeek(&queObj->vidFrmQ,(Ptr *)&frame);
            UTILS_assert((frame != NULL) && (status == 0));
            seqIdMatch = avsync_is_seqid_match(frame,
                                               queObj->playerTime->curSeqId);
            if (queObj->playerTime->state == AVSYNC_PLAYERTIME_STATE_RUNNING)
            {
                renderTS = avsync_get_render_ts(queObj);
                framePTS = avsync_get_frame_pts(frame);
                
                if (TRUE == seqIdMatch)
                {
                    renderFlag = avsync_synch_vidframe(queObj,framePTS,renderTS);
                }
                else
                {
                    renderFlag = AVSYNC_DropFrame;
                }
                
                if ((AVSYNC_RenderFrame == renderFlag)
                    ||
                    (AVSYNC_DropFrame == renderFlag))
                {
                    status = Utils_queGet(&queObj->vidFrmQ,
                                          (Ptr *)&frame,
                                          1,ti_sysbios_BIOS_NO_WAIT);
                    UTILS_assert((frame != NULL) && (status == 0));
                    if (AVSYNC_DropFrame == renderFlag)
                    {
                        UTILS_assert(freeFrameList->numFrames <
                                     UTILS_ARRAYSIZE(freeFrameList->frames));
                        freeFrameList->frames[freeFrameList->numFrames] =
                            frame;
                        freeFrameList->numFrames++;
                        frame = NULL;
                    }
                }
                if ((AVSYNC_RenderFrame == renderFlag)
                    ||
                    (AVSYNC_RepeatFrame == renderFlag)
                    ||
                    (freeFrameList->numFrames ==
                     UTILS_ARRAYSIZE(freeFrameList->frames)))
                {
                    if (AVSYNC_RenderFrame == renderFlag)
                    {
                        queObj->playerTime->lastVidFrmPTS =
                            framePTS;
                        *framePtr = frame;
                    }
                    break;
                }
            }
            else
            {
                Bool playerTimeStarted = FALSE;

                if ((queObj->playerTime->firstFrmInSeqReceived)
                    &&
                    (TRUE == seqIdMatch))
                {
                    playerTimeStarted =
                    avsync_handle_player_time_start(queObj,frame);
                    if (FALSE == playerTimeStarted)
                    {
                        switch(queObj->cfg->playStartMode)
                        {
                            case AVSYNC_PLAYBACK_START_MODE_WAITSYNCH:
                                renderFlag = AVSYNC_RepeatFrame;
                                break;
                            case AVSYNC_PLAYBACK_START_MODE_DROPUNSYNCH:
                                renderFlag = AVSYNC_DropFrame;
                                break;
                            case AVSYNC_PLAYBACK_START_MODE_PLAYUNSYNCH:
                                renderFlag = AVSYNC_RenderFrame;
                                break;
                        }
                    }
                }
                else
                {
                    playerTimeStarted = FALSE;
                    renderFlag = AVSYNC_DropFrame;
                }
                if (FALSE == playerTimeStarted)
                {
                    if ((AVSYNC_RenderFrame == renderFlag)
                        ||
                        (AVSYNC_DropFrame == renderFlag))
                    {
                        status = Utils_queGet(&queObj->vidFrmQ,
                                              (Ptr *)&frame,
                                              1,ti_sysbios_BIOS_NO_WAIT);
                        UTILS_assert((frame != NULL) && (status == 0));
                        if (AVSYNC_DropFrame == renderFlag)
                        {
                            UTILS_assert(freeFrameList->numFrames <
                                         UTILS_ARRAYSIZE(freeFrameList->frames));
                            freeFrameList->frames[freeFrameList->numFrames] =
                                frame;
                            freeFrameList->numFrames++;
                            frame = NULL;
                        }
                        else
                        {
                            *framePtr = frame;
                        }
                    }
                    break;
                }
            }
        }
        else
        {
            status =
                Utils_queGet(&queObj->vidFrmQ,
                             (Ptr *)framePtr,1,ti_sysbios_BIOS_NO_WAIT);
            UTILS_assert(status == 0);
            break;
        }
    }
    AVSYNC_CFG_MUTEX_RELEASE(queObj->displayID);
    return status;
}

static
Int avsync_vidque_do_flush(AvsyncLink_VidQueObj *queObj,
                       FVID2_Frame **framePtr,
                       FVID2_FrameList *freeFrameList)
{
     Int32 status = AVSYNC_S_OK;
     FVID2_Frame *frame = NULL;

     while (!Utils_queIsEmpty(&queObj->vidFrmQ))
     {
         if (frame != NULL)
         {
             UTILS_assert(freeFrameList->numFrames <
                          UTILS_ARRAYSIZE(freeFrameList->frames));
             freeFrameList->frames[freeFrameList->numFrames] =
                 frame;
             freeFrameList->numFrames++;
             frame = NULL;
         }
         status = Utils_queGet(&queObj->vidFrmQ,
                               (Ptr *)&frame,
                                1,
                                ti_sysbios_BIOS_NO_WAIT);
         UTILS_assert(status == 0);
     }
     *framePtr = frame;
     /* handle case of updating firstVidPTS if frames
      * are flushed .This enables to start with
      * correct PTS value instead of stale PTS
      * value when starting the player time
      */
     if (frame)
     {
         UTILS_assert(queObj->cfg != NULL);
         if (queObj->cfg->avsyncEnable)
         {
            if  ((queObj->playerTime->firstFrmInSeqReceived)
                 &&
                 (queObj->playerTime->state  != AVSYNC_PLAYERTIME_STATE_RUNNING))
            {
                UInt64 pts;

                pts = avsync_get_frame_pts(frame);
                if (pts != AVSYNC_INVALID_PTS)
                {
                    avsync_set_player_time_firstvidpts(queObj->playerTime,pts);
                }
            }
         }
     }
     return status;
}


Int Avsync_vidQueFlush(AvsyncLink_VidQueObj *queObj,
                       FVID2_Frame **framePtr,
                       FVID2_FrameList *freeFrameList)
{
     Int32 status = AVSYNC_S_OK;

     if (AVSYNC_INVALID_DISPLAY_ID != queObj->displayID)
     {
         AVSYNC_CFG_MUTEX_ACQUIRE(queObj->displayID);
         status = avsync_vidque_do_flush(queObj,framePtr,freeFrameList);
         AVSYNC_CFG_MUTEX_RELEASE(queObj->displayID);
     }
     else
     {
         status = avsync_vidque_do_flush(queObj,framePtr,freeFrameList);
     }
     return status;
}

UInt32 Avsync_vidQueGetQueLength(AvsyncLink_VidQueObj *queObj)
{
    UInt32 queLen;

    queLen =
    Utils_queGetQueuedCount(&queObj->vidFrmQ);
    return queLen;
}


UInt32 Avsync_VidQueIsEmpty(AvsyncLink_VidQueObj *queObj)
{
    return (Utils_queIsEmpty(&queObj->vidFrmQ));
}

static
Avsync_PlayerTimeObj * avsync_get_player_timeobj(AvsyncLink_Obj *pObj,
                                                 UInt32 displayID,
                                                 UInt32 chNum)
{
    Avsync_PlayerTimeObj *playerTimeObj = NULL;

    if ((displayID < AVSYNC_MAX_NUM_DISPLAYS)
        &&
        (chNum < AVSYNC_MAX_CHANNELS_PER_DISPLAY))
    {
        UTILS_assert(pObj->srObj != NULL);
        playerTimeObj = &pObj->srObj->playerTime[displayID][chNum];
    }
    return playerTimeObj;
}


static
Avsync_SynchConfigParams * avsync_get_avsync_config(AvsyncLink_Obj *pObj,
                                                    UInt32 displayID,
                                                    UInt32 chNum)
{
    Avsync_SynchConfigParams *syncCfg = NULL;

    if ((displayID < AVSYNC_MAX_NUM_DISPLAYS)
        &&
        (chNum < AVSYNC_MAX_CHANNELS_PER_DISPLAY))
    {
        UTILS_assert(pObj->srObj != NULL);
        syncCfg = &pObj->srObj->queCfg[displayID][chNum];
    }
    return syncCfg;
}

UInt32 Avsync_vidQueGetMasterSynchChannel(UInt32  syncLinkID)
{
    AvsyncLink_Obj *pObj = &gAvsyncLink_obj;
    UInt32 displayID = avsync_map_linkid2displayid(pObj,syncLinkID);
    UInt32 synchMasterChNum = AVSYNC_INVALID_CHNUM;

    UTILS_assert(pObj->srObj != NULL);
    if(displayID < UTILS_ARRAYSIZE(pObj->srObj->refClk))
    {
        synchMasterChNum = pObj->srObj->refClk[displayID].synchMasterChNum;
    }
    return synchMasterChNum;
}


UInt32 Avsync_vidQueGetDisplayID(UInt32  syncLinkID)
{
    AvsyncLink_Obj *pObj = &gAvsyncLink_obj;
    UInt32 displayID = avsync_map_linkid2displayid(pObj,syncLinkID);
    return displayID;
}

static
Int32 avsync_map_audiodev2synchinfo(Avsync_MasterSynchInfo *masterSynchInfo)
{
    AvsyncLink_Obj *pObj = &gAvsyncLink_obj;
    UInt32 audioDevId = masterSynchInfo->audioDevId;
    UInt32 displayID = avsync_map_auddev2displayid(pObj,audioDevId);
    UInt32 synchMasterChNum = AVSYNC_INVALID_CHNUM;

    UTILS_assert(pObj->srObj != NULL);
    if(displayID < UTILS_ARRAYSIZE(pObj->srObj->refClk))
    {
        synchMasterChNum = pObj->srObj->refClk[displayID].synchMasterChNum;
    }
    masterSynchInfo->displayId = displayID;
    masterSynchInfo->synchMasterChNum = synchMasterChNum;
    return AVSYNC_S_OK;


}

static
Void avsync_set_video_backend_delay(AvsyncLink_DisplayObj *displayObj,
                                    UInt32 videoBackendDelay)
{
    displayObj->videoBackendDelay = videoBackendDelay;
}

static Int32 avsync_validate_seek_params(Avsync_SynchConfigParams *synchCfg,
                                         UInt64 firstAudPTS,
                                         UInt64 firstVidPTS)
{
    Int32 status = AVSYNC_S_OK;

    UTILS_assertError(
                     ((synchCfg->ptsInitMode == AVSYNC_PTS_INIT_MODE_APP)
                      &&
                      ((firstAudPTS != AVSYNC_INVALID_PTS) ||
                       (firstVidPTS != AVSYNC_INVALID_PTS))),
                      status,
                      AVSYNC_E_INVALIDPARAMS,
                      SYSTEM_LINK_ID_AVSYNC,
                      -1);
    return status;
}

static
Int32 avsync_do_player_seek(Avsync_PlayerTimeObj *playerTime,
                            Avsync_SynchConfigParams *synchCfg,
                            UInt64 firstAudPTS,
                            UInt64 firstVidPTS,
                            UInt32 displaySeqId)
{
    Int32 status;
    Avsync_PlayerTimeObj origPlayState;

    status =
    avsync_validate_seek_params(synchCfg,
                                firstAudPTS,
                                firstVidPTS);
    if (!UTILS_ISERROR(status))
    {
        AVSYNC_CRITICAL_BEGIN();
        /* Store and restore original playstate after seek */
        origPlayState = *playerTime;
        avsync_init_player_timer(playerTime);
        avsync_set_player_time_firstaudpts(playerTime,
                                           firstAudPTS);
        avsync_set_player_time_firstvidpts(playerTime,
                                           firstVidPTS);
        avsync_set_player_timer_state_play(playerTime,displaySeqId);
        /* Now restore the player state so that
         * playback after seek resumes in same state
         * ie. If it was Slow play before seek,
         *     it should be Slow play after seek as well
         */
        playerTime->playState =  origPlayState.playState;
        playerTime->prePauseScalingFactor =  origPlayState.prePauseScalingFactor;
        playerTime->scalingFactor =  origPlayState.scalingFactor;
        playerTime->scanFrameDisplayDuration =  origPlayState.scanFrameDisplayDuration;
        playerTime->takeNextStep = TRUE;
        playerTime->lastVidFrmPTS = AVSYNC_INVALID_PTS;
        AVSYNC_CRITICAL_END();
    }
    return status;
}


static
Void avsync_get_avsync_enabled_channel_list(AvsyncLink_Obj *pObj,
                                           UInt32 displayID,
                                           UInt32 *numCh,
                                           UInt32 channelList[],
                                           UInt32 maxNumCh)
{
    Int i;

    *numCh = 0;
    UTILS_assert(displayID < AVSYNC_MAX_NUM_DISPLAYS);
    for (i = 0; i < AVSYNC_MAX_CHANNELS_PER_DISPLAY;i++)
    {
        Avsync_SynchConfigParams *vidQueCfg =
            &pObj->srObj->queCfg[displayID][i];
        if (vidQueCfg->chNum != AVSYNC_INVALID_CHNUM)
        {
            if (vidQueCfg->avsyncEnable)
            {
                UTILS_assert(*numCh < maxNumCh);
                channelList[*numCh] = i;
                *numCh = *numCh + 1;
            }
        }
    }
}


static
Void avsync_pause_all_channels(AvsyncLink_Obj *pObj,
                              UInt32 displayID)
{
    UInt32 numCh;
    UInt32 channelList[AVSYNC_MAX_CHANNELS_PER_DISPLAY];
    Int i;

    AVSYNC_CRITICAL_BEGIN();
    avsync_get_avsync_enabled_channel_list(pObj,
                                           displayID,
                                           &numCh,
                                           channelList,
                                           AVSYNC_MAX_CHANNELS_PER_DISPLAY);
    for (i = 0; i < numCh; i++)
    {
        Avsync_PlayerTimeObj *playerTime;

        playerTime =
            avsync_get_player_timeobj(pObj,
                                      displayID,
                                      channelList[i]);
        if (playerTime)
        {
            avsync_pause_player_time(playerTime);
        }
    }

    AVSYNC_CRITICAL_END();
}


static
Void avsync_unpause_all_channels(AvsyncLink_Obj *pObj,
                                 UInt32 displayID)
{
    UInt32 numCh;
    UInt32 channelList[AVSYNC_MAX_CHANNELS_PER_DISPLAY];
    Int i;

    AVSYNC_CRITICAL_BEGIN();
    avsync_get_avsync_enabled_channel_list(pObj,
                                           displayID,
                                           &numCh,
                                           channelList,
                                           AVSYNC_MAX_CHANNELS_PER_DISPLAY);
    for (i = 0; i < numCh; i++)
    {
        Avsync_PlayerTimeObj *playerTime;

        playerTime =
            avsync_get_player_timeobj(pObj,
                                      displayID,
                                      channelList[i]);
        if (playerTime)
        {
            avsync_unpause_player_time(playerTime);
        }
    }

    AVSYNC_CRITICAL_END();
}

static
Void avsync_stepfwd_all_channels(AvsyncLink_Obj *pObj,
                                 UInt32 displayID)
{
    UInt32 numCh;
    UInt32 channelList[AVSYNC_MAX_CHANNELS_PER_DISPLAY];
    Int i;

    AVSYNC_CRITICAL_BEGIN();
    avsync_get_avsync_enabled_channel_list(pObj,
                                           displayID,
                                           &numCh,
                                           channelList,
                                           AVSYNC_MAX_CHANNELS_PER_DISPLAY);
    for (i = 0; i < numCh; i++)
    {
        Avsync_PlayerTimeObj *playerTime;

        playerTime =
            avsync_get_player_timeobj(pObj,
                                      displayID,
                                      channelList[i]);
        if (playerTime)
        {
            avsync_step_fwd_player_time(playerTime);
        }
    }

    AVSYNC_CRITICAL_END();
}

static
Void avsync_timescale_all_channels(AvsyncLink_Obj *pObj,
                                   UInt32 displayID,
                                   Float newScalingFactor,
                                   UInt32 displaySeqId)
{
    UInt32 numCh;
    UInt32 channelList[AVSYNC_MAX_CHANNELS_PER_DISPLAY];
    Int i;

    AVSYNC_CRITICAL_BEGIN();
    avsync_get_avsync_enabled_channel_list(pObj,
                                           displayID,
                                           &numCh,
                                           channelList,
                                           AVSYNC_MAX_CHANNELS_PER_DISPLAY);
    for (i = 0; i < numCh; i++)
    {
        Avsync_PlayerTimeObj *playerTime;

        playerTime =
            avsync_get_player_timeobj(pObj,
                                      displayID,
                                      channelList[i]);
        if (playerTime)
        {
            avsync_set_timescale(playerTime,newScalingFactor,
                                 displaySeqId);
        }
    }

    AVSYNC_CRITICAL_END();
}

static
Void avsync_do_scan_all_channels(AvsyncLink_Obj *pObj,
                                 UInt32 displayID,
                                 UInt32 displayDuration,
                                 UInt32 displaySeqId)
{
    UInt32 numCh;
    UInt32 channelList[AVSYNC_MAX_CHANNELS_PER_DISPLAY];
    Int i;

    AVSYNC_CRITICAL_BEGIN();
    avsync_get_avsync_enabled_channel_list(pObj,
                                           displayID,
                                           &numCh,
                                           channelList,
                                           AVSYNC_MAX_CHANNELS_PER_DISPLAY);
    for (i = 0; i < numCh; i++)
    {
        Avsync_PlayerTimeObj *playerTime;

        playerTime =
            avsync_get_player_timeobj(pObj,
                                      displayID,
                                      channelList[i]);
        if (playerTime)
        {
            avsync_set_player_timer_state_scan(playerTime,
                                               displayDuration,
                                               displaySeqId);
        }
    }

    AVSYNC_CRITICAL_END();
}


static
Void avsync_do_player_seek_all_channels(AvsyncLink_Obj *pObj,
                                        UInt32 displayID,
                                        UInt64 seekAudPTS,
                                        UInt64 seekVidPTS,
                                        UInt32 displaySeqId)
{
    UInt32 numCh;
    UInt32 channelList[AVSYNC_MAX_CHANNELS_PER_DISPLAY];
    Int i;

    AVSYNC_CRITICAL_BEGIN();
    avsync_get_avsync_enabled_channel_list(pObj,
                                           displayID,
                                           &numCh,
                                           channelList,
                                           AVSYNC_MAX_CHANNELS_PER_DISPLAY);
    for (i = 0; i < numCh; i++)
    {
        Avsync_PlayerTimeObj *playerTime;
        Avsync_SynchConfigParams *synchCfg;

        playerTime =
            avsync_get_player_timeobj(pObj,
                                      displayID,
                                      channelList[i]);
        synchCfg =
            avsync_get_avsync_config(pObj,
                                     displayID,
                                     channelList[i]);
        if ((playerTime)
            &&
            (synchCfg)
            &&
            (AVSYNC_PTS_INIT_MODE_APP == synchCfg->ptsInitMode))
        {
            avsync_do_player_seek(playerTime,
                                  synchCfg,
                                  seekAudPTS,
                                  seekVidPTS,
                                  displaySeqId);
        }
        else
        {
            Vps_printf("AVSYNC:AVSYNC_LINK_CMD_SEEK failed"
                       "due to invalid cfg\n"
                       "Config:DisplayID[%d],ChID[%d]\n",
                       displayID,
                       channelList[i]);
        }
    }

    AVSYNC_CRITICAL_END();
}

static
Void avsync_play_all_channels(AvsyncLink_Obj *pObj,
                              UInt32 displayID,
                              UInt32 displaySeqId)
{
    UInt32 numCh;
    UInt32 channelList[AVSYNC_MAX_CHANNELS_PER_DISPLAY];
    Int i;

    AVSYNC_CRITICAL_BEGIN();
    avsync_get_avsync_enabled_channel_list(pObj,
                                           displayID,
                                           &numCh,
                                           channelList,
                                           AVSYNC_MAX_CHANNELS_PER_DISPLAY);
    for (i = 0; i < numCh; i++)
    {
        Avsync_PlayerTimeObj *playerTime;

        playerTime =
            avsync_get_player_timeobj(pObj,
                                      displayID,
                                      channelList[i]);
        if (playerTime)
        {
            avsync_set_player_timer_state_play(playerTime,displaySeqId);
        }
    }

    AVSYNC_CRITICAL_END();
}

static
Void avsync_reset_all_channels(AvsyncLink_Obj *pObj,
                               UInt32 displayID)
{
    UInt32 numCh;
    UInt32 channelList[AVSYNC_MAX_CHANNELS_PER_DISPLAY];
    Int i;

    AVSYNC_CRITICAL_BEGIN();
    avsync_get_avsync_enabled_channel_list(pObj,
                                           displayID,
                                           &numCh,
                                           channelList,
                                           AVSYNC_MAX_CHANNELS_PER_DISPLAY);
    for (i = 0; i < numCh; i++)
    {
        Avsync_PlayerTimeObj *playerTime;

        playerTime =
            avsync_get_player_timeobj(pObj,
                                      displayID,
                                      channelList[i]);
        if (playerTime)
        {
            avsync_init_player_timer(playerTime);
        }
    }

    AVSYNC_CRITICAL_END();
}

/* IDLE and READY state implementation */
static
Void AvsyncLink_tskMain(struct Utils_TskHndl * pTsk, Utils_MsgHndl * pMsg)
{
    UInt32 cmd = Utils_msgGetCmd(pMsg);
    Bool ackMsg, done;
    Int32 status;
    AvsyncLink_Obj *pObj;

    /* IDLE state */

    pObj = (AvsyncLink_Obj *) pTsk->appData;
    UTILS_assert(pObj != NULL);
    if (cmd != AVSYNC_LINK_CMD_INIT)
    {
        /* invalid command recived in IDLE status, be in IDLE state and ACK
         * with error status */
        Utils_tskAckOrFreeMsg(pMsg, FVID2_EFAIL);
        return;
    }

    avsync_init_rtos(pObj,Utils_msgGetPrm(pMsg));

    /* ACK based on create status */
    Utils_tskAckOrFreeMsg(pMsg, FVID2_SOK);

    /* init success, entering READY state */

    done = FALSE;
    ackMsg = FALSE;

    /* READY state loop */
    while (!done)
    {
        /* wait for message */
        status = Utils_tskRecvMsg(pTsk, &pMsg, ti_sysbios_BIOS_WAIT_FOREVER);
        if (status != FVID2_SOK)
            break;

        /* extract message command from message */
        cmd = Utils_msgGetCmd(pMsg);

        switch (cmd)
        {
            case AVSYNC_LINK_CMD_PAUSE:
            {
                Avsync_PauseParams *pauseParams = Utils_msgGetPrm(pMsg);
                Avsync_PlayerTimeObj *playerTime;

                pauseParams->displayLinkID = Avsync_mapDisplayLinkID2Index(pauseParams->displayLinkID);
                if (pauseParams->displayLinkID < AVSYNC_MAX_NUM_DISPLAYS)
                {
                    AVSYNC_CFG_MUTEX_ACQUIRE(pauseParams->displayLinkID);
                    if (AVSYNC_ALL_CHANNEL_ID == pauseParams->chNum)
                    {
                        avsync_pause_all_channels(pObj,
                                                  pauseParams->displayLinkID);
                        status = AVSYNC_S_OK;
                    }
                    else
                    {
                        playerTime =
                            avsync_get_player_timeobj(pObj,
                                                      pauseParams->displayLinkID,
                                                      pauseParams->chNum);
                        if (playerTime)
                        {
                            avsync_pause_player_time(playerTime);
                            status = AVSYNC_S_OK;
                        }
                        else
                        {
                            status = AVSYNC_E_INVALIDPARAMS;
                        }
                    }
                    AVSYNC_CFG_MUTEX_RELEASE(pauseParams->displayLinkID);
                }
                else
                {
                    status = AVSYNC_E_INVALIDPARAMS;
                }
                Utils_tskAckOrFreeMsg(pMsg, status);
                break;
            }
            case AVSYNC_LINK_CMD_UNPAUSE:
            {
                Avsync_UnPauseParams *unpauseParams = Utils_msgGetPrm(pMsg);
                Avsync_PlayerTimeObj *playerTime;

                unpauseParams->displayLinkID = Avsync_mapDisplayLinkID2Index(unpauseParams->displayLinkID);
                if (unpauseParams->displayLinkID < AVSYNC_MAX_NUM_DISPLAYS)
                {
                    AVSYNC_CFG_MUTEX_ACQUIRE(unpauseParams->displayLinkID);

                    if (AVSYNC_ALL_CHANNEL_ID == unpauseParams->chNum)
                    {
                        avsync_unpause_all_channels(pObj,
                                                    unpauseParams->displayLinkID);
                        status = AVSYNC_S_OK;
                    }
                    else
                    {
                        playerTime =
                            avsync_get_player_timeobj(pObj,
                                                      unpauseParams->displayLinkID,
                                                      unpauseParams->chNum);
                        if (playerTime)
                        {
                            avsync_unpause_player_time(playerTime);
                            status = AVSYNC_S_OK;
                        }
                        else
                        {
                            status = AVSYNC_E_INVALIDPARAMS;
                        }
                    }
                    AVSYNC_CFG_MUTEX_RELEASE(unpauseParams->displayLinkID);
                }
                else
                {
                    status = AVSYNC_E_INVALIDPARAMS;
                }
                Utils_tskAckOrFreeMsg(pMsg, status);
                break;
            }
            case AVSYNC_LINK_CMD_TIMESCALE:
            {
                Avsync_TimeScaleParams *timeScaleParams = Utils_msgGetPrm(pMsg);
                Avsync_PlayerTimeObj *playerTime;

                timeScaleParams->displayLinkID = Avsync_mapDisplayLinkID2Index(timeScaleParams->displayLinkID);
                if (timeScaleParams->displayLinkID < AVSYNC_MAX_NUM_DISPLAYS)
                {
                    AVSYNC_CFG_MUTEX_ACQUIRE(timeScaleParams->displayLinkID);

                    if (AVSYNC_ALL_CHANNEL_ID == timeScaleParams->chNum)
                    {
                        avsync_timescale_all_channels(pObj,
                                                      timeScaleParams->displayLinkID,
                                                      ((float)timeScaleParams->timeScaleX1000)/1000,
                                                      timeScaleParams->displaySeqId);
                        status = AVSYNC_S_OK;
                    }
                    else
                    {
                        playerTime =
                            avsync_get_player_timeobj(pObj,
                                                      timeScaleParams->displayLinkID,
                                                      timeScaleParams->chNum);
                        if (playerTime)
                        {
                            avsync_set_timescale(playerTime,
                                                 ((float)timeScaleParams->timeScaleX1000)/1000,
                                                 timeScaleParams->displaySeqId);
                            status = AVSYNC_S_OK;
                        }
                        else
                        {
                            status = AVSYNC_E_INVALIDPARAMS;
                        }
                    }
                    AVSYNC_CFG_MUTEX_RELEASE(timeScaleParams->displayLinkID);
                }
                else
                {
                    status = AVSYNC_E_INVALIDPARAMS;
                }
                Utils_tskAckOrFreeMsg(pMsg, status);
                break;
            }
            case AVSYNC_LINK_CMD_STEP_FWD:
            {
                Avsync_StepFwdParams *stepFwdParams = Utils_msgGetPrm(pMsg);
                Avsync_PlayerTimeObj *playerTime;

                stepFwdParams->displayLinkID = Avsync_mapDisplayLinkID2Index(stepFwdParams->displayLinkID);
                if (stepFwdParams->displayLinkID < AVSYNC_MAX_NUM_DISPLAYS)
                {
                    AVSYNC_CFG_MUTEX_ACQUIRE(stepFwdParams->displayLinkID);

                    if (AVSYNC_ALL_CHANNEL_ID == stepFwdParams->chNum)
                    {
                        avsync_stepfwd_all_channels(pObj,
                                                    stepFwdParams->displayLinkID);
                        status = AVSYNC_S_OK;
                    }
                    else
                    {
                        playerTime =
                            avsync_get_player_timeobj(pObj,
                                                      stepFwdParams->displayLinkID,
                                                      stepFwdParams->chNum);
                        if (playerTime)
                        {
                            avsync_step_fwd_player_time(playerTime);
                            status = AVSYNC_S_OK;
                        }
                        else
                        {
                            status = AVSYNC_E_INVALIDPARAMS;
                        }
                    }
                    AVSYNC_CFG_MUTEX_RELEASE(stepFwdParams->displayLinkID);
                }
                else
                {
                    status = AVSYNC_E_INVALIDPARAMS;
                }

                Utils_tskAckOrFreeMsg(pMsg, status);
                break;
            }
            case AVSYNC_LINK_CMD_AVSYNCCFG:
            {
                AvsyncLink_LinkSynchConfigParams *syncCfg = Utils_msgGetPrm(pMsg);

                status = avsync_set_avsynccfg(pObj,syncCfg);
                Utils_tskAckOrFreeMsg(pMsg, status);
                break;
            }
            case AVSYNC_LINK_CMD_GETSROBJ:
            {
                Avsync_GetSrObjParams *getSrObjParams = Utils_msgGetPrm(pMsg);

                getSrObjParams->srObjSrPtr = pObj->srObjSrPtr;
                status = AVSYNC_S_OK;
                Utils_tskAckOrFreeMsg(pMsg, status);
                break;
            }
            case AVSYNC_LINK_CMD_GETVIDSYNCHCHINFO:
            {
                Avsync_MasterSynchInfo *masterSynchChInfo = Utils_msgGetPrm(pMsg);

                status = avsync_map_audiodev2synchinfo(masterSynchChInfo);
                Utils_tskAckOrFreeMsg(pMsg, status);
                break;
            }
            case AVSYNC_LINK_CMD_SET_FIRST_AUDPTS:
            {
                Avsync_FirstAudPTSParams *audPTSParams = Utils_msgGetPrm(pMsg);
                Avsync_PlayerTimeObj *playerTime;
                Avsync_SynchConfigParams *synchCfg;

                audPTSParams->displayLinkID = Avsync_mapDisplayLinkID2Index(audPTSParams->displayLinkID);
                if (audPTSParams->displayLinkID < AVSYNC_MAX_NUM_DISPLAYS)
                {
                    AVSYNC_CFG_MUTEX_ACQUIRE(audPTSParams->displayLinkID);

                    playerTime =
                        avsync_get_player_timeobj(pObj,
                                                  audPTSParams->displayLinkID,
                                                  audPTSParams->chNum);
                    synchCfg =
                        avsync_get_avsync_config(pObj,
                                                 audPTSParams->displayLinkID,
                                                 audPTSParams->chNum);
                    if ((playerTime)
                        &&
                        (synchCfg)
                        &&
                        (AVSYNC_PTS_INIT_MODE_APP == synchCfg->ptsInitMode))
                    {
                        avsync_set_player_time_firstaudpts(playerTime,
                                               audPTSParams->firstAudPTS);
                        status = AVSYNC_S_OK;
                    }
                    else
                    {
                        Vps_printf("AVSYNC:AVSYNC_LINK_CMD_SET_FIRST_AUDPTS failed"
                                   "due to invalid cfg\n"
                                   "Config:DisplayID[%d],ChID[%d]\n",
                                   audPTSParams->displayLinkID,
                                   audPTSParams->chNum);
                        status = AVSYNC_E_INVALIDPARAMS;
                    }
                    AVSYNC_CFG_MUTEX_RELEASE(audPTSParams->displayLinkID);
                }
                else
                {
                    status = AVSYNC_E_INVALIDPARAMS;
                }

                Utils_tskAckOrFreeMsg(pMsg, status);
                break;
            }
            case AVSYNC_LINK_CMD_SET_FIRST_VIDPTS:
            {
                Avsync_FirstVidPTSParams *vidPTSParams = Utils_msgGetPrm(pMsg);
                Avsync_PlayerTimeObj *playerTime;
                Avsync_SynchConfigParams *synchCfg;

                vidPTSParams->displayLinkID = Avsync_mapDisplayLinkID2Index(vidPTSParams->displayLinkID);
                if (vidPTSParams->displayLinkID < AVSYNC_MAX_NUM_DISPLAYS)
                {
                    AVSYNC_CFG_MUTEX_ACQUIRE(vidPTSParams->displayLinkID);

                    playerTime =
                        avsync_get_player_timeobj(pObj,
                                                  vidPTSParams->displayLinkID,
                                                  vidPTSParams->chNum);
                    synchCfg =
                        avsync_get_avsync_config(pObj,
                                                 vidPTSParams->displayLinkID,
                                                 vidPTSParams->chNum);
                    if ((playerTime)
                        &&
                        (synchCfg)
                        &&
                        (AVSYNC_PTS_INIT_MODE_APP == synchCfg->ptsInitMode))
                    {
                        avsync_set_player_time_firstvidpts(playerTime,
                                               vidPTSParams->firstVidPTS);
                        Vps_printf("AVSYNC:AVSYNC_LINK_CMD_SET_FIRST_VIDPTS success"
                                   "Config:DisplayID[%d],ChID[%d],FirstPTS[%d]\n",
                                   vidPTSParams->displayLinkID,
                                   vidPTSParams->chNum,
                                   (UInt32)vidPTSParams->firstVidPTS);
                        status = AVSYNC_S_OK;
                    }
                    else
                    {
                        Vps_printf("AVSYNC:AVSYNC_LINK_CMD_SET_FIRST_VIDPTS failed"
                                   "due to invalid cfg\n"
                                   "Config:DisplayID[%d],ChID[%d]\n",
                                   vidPTSParams->displayLinkID,
                                   vidPTSParams->chNum);
                        status = AVSYNC_E_INVALIDPARAMS;
                    }
                    AVSYNC_CFG_MUTEX_RELEASE(vidPTSParams->displayLinkID);
                }
                else
                {
                    status = AVSYNC_E_INVALIDPARAMS;
                }
                Utils_tskAckOrFreeMsg(pMsg, status);
                break;
            }
            case AVSYNC_LINK_CMD_RESET_PLAYERTIME:
            {
                Avsync_ResetPlayerTimerParams *resetPlayerParams = Utils_msgGetPrm(pMsg);
                Avsync_PlayerTimeObj *playerTime;

                resetPlayerParams->displayLinkID = Avsync_mapDisplayLinkID2Index(resetPlayerParams->displayLinkID);
                if (resetPlayerParams->displayLinkID < AVSYNC_MAX_NUM_DISPLAYS)
                {
                    AVSYNC_CFG_MUTEX_ACQUIRE(resetPlayerParams->displayLinkID);

                    if (AVSYNC_ALL_CHANNEL_ID == resetPlayerParams->chNum)
                    {
                        avsync_reset_all_channels(pObj,
                                                  resetPlayerParams->displayLinkID);
                        status = AVSYNC_S_OK;
                    }
                    else
                    {
                        playerTime =
                            avsync_get_player_timeobj(pObj,
                                                      resetPlayerParams->displayLinkID,
                                                      resetPlayerParams->chNum);
                        if (playerTime)
                        {
                            avsync_init_player_timer(playerTime);
                            status = AVSYNC_S_OK;
                        }
                        else
                        {
                            status = AVSYNC_E_INVALIDPARAMS;
                        }
                    }
                    AVSYNC_CFG_MUTEX_RELEASE(resetPlayerParams->displayLinkID);
                }
                else
                {
                    status = AVSYNC_E_INVALIDPARAMS;
                }
                Utils_tskAckOrFreeMsg(pMsg, status);
                break;
            }
            case AVSYNC_LINK_CMD_PLAY:
            {
                Avsync_PlayParams *playParams = Utils_msgGetPrm(pMsg);
                Avsync_PlayerTimeObj *playerTime;

                playParams->displayLinkID = Avsync_mapDisplayLinkID2Index(playParams->displayLinkID);

                if (playParams->displayLinkID < AVSYNC_MAX_NUM_DISPLAYS)
                {
                    AVSYNC_CFG_MUTEX_ACQUIRE(playParams->displayLinkID);

                    if (AVSYNC_ALL_CHANNEL_ID == playParams->chNum)
                    {
                        avsync_play_all_channels(pObj,
                                                 playParams->displayLinkID,
                                                 playParams->displaySeqId);
                        status = AVSYNC_S_OK;
                    }
                    else
                    {
                        playerTime =
                            avsync_get_player_timeobj(pObj,
                                                      playParams->displayLinkID,
                                                      playParams->chNum);
                        if (playerTime)
                        {
                            avsync_set_player_timer_state_play(playerTime,
                                                               playParams->displaySeqId);
                            status = AVSYNC_S_OK;
                        }
                        else
                        {
                            status = AVSYNC_E_INVALIDPARAMS;
                        }
                    }
                    AVSYNC_CFG_MUTEX_RELEASE(playParams->displayLinkID);
                }
                else
                {
                    status = AVSYNC_E_INVALIDPARAMS;
                }
                Utils_tskAckOrFreeMsg(pMsg, status);
                break;
            }
            case AVSYNC_LINK_CMD_SCAN:
            {
                Avsync_ScanParams *scanParams = Utils_msgGetPrm(pMsg);
                Avsync_PlayerTimeObj *playerTime;

                scanParams->displayLinkID = Avsync_mapDisplayLinkID2Index(scanParams->displayLinkID);
                if (scanParams->displayLinkID < AVSYNC_MAX_NUM_DISPLAYS)
                {
                    AVSYNC_CFG_MUTEX_ACQUIRE(scanParams->displayLinkID);

                    if (AVSYNC_ALL_CHANNEL_ID == scanParams->chNum)
                    {
                        avsync_do_scan_all_channels(pObj,
                                                    scanParams->displayLinkID,
                                                    scanParams->frameDisplayDurationMS,
                                                    scanParams->displaySeqId);
                        status = AVSYNC_S_OK;
                    }
                    else
                    {
                        playerTime =
                            avsync_get_player_timeobj(pObj,
                                                      scanParams->displayLinkID,
                                                      scanParams->chNum);
                        if (playerTime)
                        {
                            avsync_set_player_timer_state_scan(playerTime,
                                       scanParams->frameDisplayDurationMS,
                                       scanParams->displaySeqId);
                            status = AVSYNC_S_OK;
                        }
                        else
                        {
                            status = AVSYNC_E_INVALIDPARAMS;
                        }
                   }
                    AVSYNC_CFG_MUTEX_RELEASE(scanParams->displayLinkID);
                }
                else
                {
                    status = AVSYNC_E_INVALIDPARAMS;
                }
                Utils_tskAckOrFreeMsg(pMsg, status);
                break;
            }
            case AVSYNC_LINK_CMD_SET_VIDEO_BACKEND_DELAY:
            {
                Avsync_VideoBackendDelayParams *backendDelay =
                    Utils_msgGetPrm(pMsg);

                backendDelay->displayLinkID = Avsync_mapDisplayLinkID2Index(backendDelay->displayLinkID);
                if (backendDelay->displayLinkID < AVSYNC_MAX_NUM_DISPLAYS)
                {
                    AVSYNC_CFG_MUTEX_ACQUIRE(backendDelay->displayLinkID);

                    if (backendDelay->displayLinkID < AVSYNC_MAX_NUM_DISPLAYS)
                    {
                        avsync_set_video_backend_delay(
                          &pObj->displayObj[backendDelay->displayLinkID],
                          backendDelay->backendDelayMS);
                        status = AVSYNC_S_OK;
                    }
                    else
                    {
                        status = AVSYNC_E_INVALIDPARAMS;
                    }
                    AVSYNC_CFG_MUTEX_RELEASE(backendDelay->displayLinkID);
                }
                else
                {
                    status = AVSYNC_E_INVALIDPARAMS;
                }
                Utils_tskAckOrFreeMsg(pMsg, status);
                break;
            }
            case AVSYNC_LINK_CMD_SEEK:
            {
                Avsync_SeekParams *seekParams = Utils_msgGetPrm(pMsg);
                Avsync_PlayerTimeObj *playerTime;
                Avsync_SynchConfigParams *synchCfg;

                seekParams->displayLinkID = Avsync_mapDisplayLinkID2Index(seekParams->displayLinkID);
                if (seekParams->displayLinkID < AVSYNC_MAX_NUM_DISPLAYS)
                {
                    AVSYNC_CFG_MUTEX_ACQUIRE(seekParams->displayLinkID);

                    if (AVSYNC_ALL_CHANNEL_ID == seekParams->chNum)
                    {
                        avsync_do_player_seek_all_channels(pObj,
                                                    seekParams->displayLinkID,
                                                    seekParams->seekAudPTS,
                                                    seekParams->seekVidPTS,
                                                    seekParams->displaySeqId);
                        status = AVSYNC_S_OK;
                    }
                    else
                    {
                        playerTime =
                            avsync_get_player_timeobj(pObj,
                                                      seekParams->displayLinkID,
                                                      seekParams->chNum);
                        synchCfg =
                            avsync_get_avsync_config(pObj,
                                                     seekParams->displayLinkID,
                                                     seekParams->chNum);
                        if ((playerTime)
                            &&
                            (synchCfg)
                            &&
                            (AVSYNC_PTS_INIT_MODE_APP == synchCfg->ptsInitMode))
                        {
                            avsync_do_player_seek(playerTime,
                                                  synchCfg,
                                                  seekParams->seekAudPTS,
                                                  seekParams->seekVidPTS,
                                                  seekParams->displaySeqId);
                            status = AVSYNC_S_OK;
                        }
                        else
                        {
                            Vps_printf("AVSYNC:AVSYNC_LINK_CMD_SET_FIRST_AUDPTS failed"
                                       "due to invalid cfg\n"
                                       "Config:DisplayID[%d],ChID[%d]\n",
                                       seekParams->displayLinkID,
                                       seekParams->chNum);
                            status = AVSYNC_E_INVALIDPARAMS;
                        }
                    }
                    AVSYNC_CFG_MUTEX_RELEASE(seekParams->displayLinkID);
                }
                else
                {
                    status = AVSYNC_E_INVALIDPARAMS;
                }
                Utils_tskAckOrFreeMsg(pMsg, status);
                break;
            }
            case AVSYNC_LINK_CMD_DEINIT:
                /* exit READY state */
                done = TRUE;
                ackMsg = TRUE;
                break;

            default:
                /* invalid command for this state ACK it and continue READY
                 * loop */
                Utils_tskAckOrFreeMsg(pMsg, status);
                break;
        }
    }

    /* exiting READY state, delete driver */
    avsync_uninit(pObj);

    /* ACK message if not previously ACK'ed */
    if (ackMsg && pMsg != NULL)
        Utils_tskAckOrFreeMsg(pMsg, status);

    /* entering IDLE state */
    return;
}

Int32 AvsyncLink_init()
{
    Int32 status;
    System_LinkObj linkObj;
    AvsyncLink_Obj *pObj;

    /* register link with system API */

    pObj = &gAvsyncLink_obj;

    memset(pObj, 0, sizeof(*pObj));

    linkObj.pTsk = &pObj->tsk;
    linkObj.linkGetFullFrames = NULL;
    linkObj.linkPutEmptyFrames = NULL;
    linkObj.getLinkInfo = NULL;

    System_registerLink(SYSTEM_LINK_ID_AVSYNC, &linkObj);

    /** Create link task, task remains in IDLE state
        AvsyncLink_tskMain is called when a message command is received
    */

    status = Utils_tskCreate(&pObj->tsk,
                             AvsyncLink_tskMain,
                             AVSYNC_LINK_TSK_PRI,
                             gAvsyncLink_tskStack,
                             AVSYNC_LINK_TSK_STACK_SIZE, pObj, "AVSYNC");
    UTILS_assert(status == FVID2_SOK);

    return status;
}

Int32 AvsyncLink_deInit()
{
    AvsyncLink_Obj *pObj;

    pObj = &gAvsyncLink_obj;

    /*
     * Delete link task */
    Utils_tskDelete(&pObj->tsk);

    return FVID2_SOK;
}

#ifdef SYSTEM_DEBUG_AVSYNC_DETAILED_LOGS

Void AvsyncLink_logCaptureTS(UInt32 chNum, UInt64 ts)
{
    AvsyncLink_TimestampLog *capTsLog = AVSYNC_GET_CAPTURE_TS_HANDLE();

    if (chNum < AVSYNC_LOG_CAPTURE_TS_NUMCH)
    {
        if (capTsLog[chNum].logIndex >= UTILS_ARRAYSIZE(capTsLog[chNum].ts))
            capTsLog[chNum].logIndex = 0;
        capTsLog[chNum].ts[capTsLog[chNum].logIndex] = ts;
        capTsLog[chNum].logIndex++;
    }
}
#else
Void AvsyncLink_logCaptureTS(UInt32 chNum, UInt64 ts)
{
    (Void)chNum;
    (Void)ts;
}
#endif

UInt32 Avsync_mapDisplayLinkID2Index(UInt32 linkID)
{
    UInt32 displayIndex = AVSYNC_INVALID_DISPLAY_ID;

    if ((linkID >= SYSTEM_LINK_ID_DISPLAY_FIRST)
        &&
        (linkID <= SYSTEM_LINK_ID_DISPLAY_LAST))
    {
        displayIndex = linkID - SYSTEM_LINK_ID_DISPLAY_FIRST;
    }
    return displayIndex;
}

/*<  END -  AvSync Logic functions - END  >*/

