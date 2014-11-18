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
    \file avsync.c
    \brief A8 side function defenitions. Interface functions for Audio-Video sync component.
*/

#include <mcfw/src_linux/links/avsync/avsync_priv.h>
#include <mcfw/interfaces/link_api/ipcLink.h>
#include <mcfw/src_linux/osa/inc/osa_debug.h>
#include <demos/audio_sample/audio.h>
#include <sys/time.h>



/** NewAvsync **/
#include <string.h>
#include <osa.h>
#include <osa_que.h>
#include <mcfw/src_linux/links/system/system_priv_common.h>


AvsyncLinkHLOS_Obj gAvsyncLink_obj =
    {
     .initDone       = FALSE,
     .nsHandleCreate = NULL,
     .nsHandleGetHandle = NULL,
     .wallTimeBase  = 0,
    };

#define AVSYNC_GET_LOG_TBL_HANDLE()             (&(gAvsyncLink_obj.srObj->vidSyncLog[0]))
#define AVSYNC_GET_CAPTURE_TS_HANDLE()          (&(gAvsyncLink_obj.srObj->capTSLog[0]))
#define AVSYNC_GET_IPCBITSOUT_TS_HANDLE()       (&(gAvsyncLink_obj.srObj->ipcTSLog[0]))

static Void avsync_print_smavobj(Avsync_SimpleMovingAvgObj *smaObj)
{
#ifdef AVSYNC_SMA_DEBUG
    Int i;

    OSA_printf("AVSYNC:SMA_OBJ");
    OSA_printf("AVSYNC:SMA_OBJ.avgQueFilled:%d", smaObj->avgQueFilled);
    OSA_printf("AVSYNC:SMA_OBJ.total:%d", smaObj->total);
    OSA_printf("AVSYNC:SMA_OBJ.index:%d", smaObj->index);
    for (i = 0; i < OSA_ARRAYSIZE(smaObj->val);i++)
    {
        OSA_printf("AVSYNC:SMA_OBJ.val[%d]:%d",i,smaObj->val[i]);
    }
#endif /* #ifdef AVSYNC_SMA_DEBUG */
}

static Void  avsync_init_smavgobj(Avsync_SimpleMovingAvgObj *smaObj)
{
    memset(smaObj,0,sizeof(*smaObj));
    smaObj->index = 0;
    smaObj->total = 0;
    smaObj->avgQueFilled = FALSE;
    avsync_print_smavobj(smaObj);
}

static Int32 avsync_get_simple_moving_avg(Avsync_SimpleMovingAvgObj *smaObj,Int32 newVal)
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

static Void avsync_set_player_time_firstaudpts(Avsync_PlayerTimeObj *playerTimeObj,
                                               UInt64                audioPTS)
{
    OSA_assert(AVSYNC_INVALID_PTS == playerTimeObj->firstAudioPTS);
    playerTimeObj->firstAudioPTS = audioPTS;
}


static Void avsync_init_player_time_mediatimebase(Avsync_PlayerTimeObj *playerTimeObj,
                                                  UInt64                mediaTimeBase)
{
    OSA_assert(playerTimeObj->mediaTimeBase == AVSYNC_INVALID_PTS);
    playerTimeObj->mediaTimeBase = mediaTimeBase;
}

static Void avsync_init_player_time_walltimebase(Avsync_PlayerTimeObj *playerTimeObj,
                                                 UInt64                wallTimeBase)
{
    OSA_assert(playerTimeObj->wallTimeBase == AVSYNC_INVALID_PTS);
    playerTimeObj->wallTimeBase = wallTimeBase;
}

static Void avsync_init_player_timer(Avsync_PlayerTimeObj *playerTimeObj)
{
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
}

static Void  avsync_init_refclkobj(Avsync_RefClkObj *refClk)
{
    refClk->numSynchedChannels = 0;
    refClk->masterPlayerTime = NULL;
    refClk->synchMasterChNum = AVSYNC_INVALID_CHNUM;
    avsync_init_smavgobj(&refClk->ptsDiffAvg);
}

static Bool avsync_start_player_time(Avsync_PlayerTimeObj *playerTime)
{
    Bool playerTimeStarted = FALSE;
    UInt64 mediaBaseTime;

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
    return playerTimeStarted;
}

static UInt64 avsync_get_player_timer(Avsync_PlayerTimeObj *playerTimeObj,
                                      UInt64 wallTime)
{
    UInt64 wallTimeElapsed;
    UInt64 playerTime;

    AVSYNC_CRITICAL_BEGIN();
    wallTimeElapsed = wallTime - playerTimeObj->wallTimeBase;
    wallTimeElapsed *= playerTimeObj->scalingFactor;
    playerTime =
      (playerTimeObj->mediaTimeBase + playerTimeObj->refClkAdjustVal)
            + wallTimeElapsed;
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
    UInt64 curSTC;

    AVSYNC_CRITICAL_BEGIN();
    curSTC = Avsync_getWallTime();

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
        *clkAdjustDelta = -1 * (preAdjustPlayerTime - postAdjustPlayerTime);
    }
    AVSYNC_CRITICAL_END();

}

static Void avsync_set_player_time_refclk_delta(Avsync_PlayerTimeObj *playerTimeObj,
                                                Int32 refClkAdjust)
{
    OSA_assert((playerTimeObj->state == AVSYNC_PLAYERTIME_STATE_INIT)
                 ||
                 (playerTimeObj->state == AVSYNC_PLAYERTIME_STATE_RUNNING));
    if (playerTimeObj->state == AVSYNC_PLAYERTIME_STATE_RUNNING)
    {
        playerTimeObj->refClkAdjustVal += refClkAdjust;
    }
}

static Void avsync_open_srobj(AvsyncLinkHLOS_Obj *pObj)
{
    Int status;
#if 0
    UInt16 queryList[] = {AVSYNC_SROBJ_CREATE_PROCID, MultiProc_INVALIDID};
    NameServer_Params nameServerParams;

    OSA_assert (pObj->nsHandleCreate == NULL);
    /* Get the default params for  the Name server */
    NameServer_Params_init(&nameServerParams);
    nameServerParams.maxRuntimeEntries = AVSYNC_MAX_NUM_INSTANCES; /* max instances on this processor */
    nameServerParams.maxValueLen       = sizeof (UInt32);
    /* Create the Name Server instance */
    pObj->nsHandleCreate = NameServer_create (AVSYNC_NAMESERVERNAME_HLOS,
                                        &nameServerParams);
    OSA_assert(pObj->nsHandleCreate != NULL);
    pObj->nsHandleGetHandle = NameServer_getHandle(AVSYNC_NAMESERVERNAME_HLOS);
    OSA_assert(pObj->nsHandleGetHandle != NULL);
    do {
        status =
          NameServer_getUInt32(pObj->nsHandleGetHandle,
                               AVSYNC_SROBJ_NAME,
                               &pObj->srObjSrPtr,
                               queryList);
        if (NameServer_S_SUCCESS != status)
        {
            usleep(AVSYNC_SROBJ_RETRY_INTERVAL_MICROSEC);
            printf("NameServer_getUInt32 failed for :%s",
                   AVSYNC_SROBJ_NAME);
        }
    } while(NameServer_S_SUCCESS != status);
    OSA_assert(NameServer_S_SUCCESS == status);
#else
    {
        Avsync_GetSrObjParams srObjGetParams;

        srObjGetParams.srObjSrPtr = IPC_LINK_INVALID_SRPTR;
        status =
        System_linkControl(SYSTEM_LINK_ID_AVSYNC,
                           AVSYNC_LINK_CMD_GETSROBJ,
                           &srObjGetParams,
                           sizeof(srObjGetParams),
                           TRUE);
        OSA_assert(status == 0);
        pObj->srObjSrPtr = srObjGetParams.srObjSrPtr;
    }
#endif
    pObj->srObj = SharedRegion_getPtr(pObj->srObjSrPtr);
    pObj->gateMPSrMemPtr = SharedRegion_getPtr(pObj->srObj->gateMPAddr);
    status = GateMP_openByAddr(pObj->gateMPSrMemPtr,&pObj->gate);
    OSA_assert(GateMP_S_SUCCESS == status);
}

static Void avsync_close_srobj(AvsyncLinkHLOS_Obj *pObj)
{
    Int status;

    GateMP_close(&pObj->gate);
    if (pObj->nsHandleCreate != NULL)
    {
        status = NameServer_delete(&pObj->nsHandleCreate);
        OSA_assert(NameServer_S_SUCCESS == status);
    }
}

static Void avsync_init_hlos(AvsyncLinkHLOS_Obj *pObj)
{
    if (FALSE == pObj->initDone)
    {
        OSA_assert (AVSYNC_SROBJ_CREATE_PROCID != System_getSelfProcId());
        avsync_open_srobj(pObj);
        pObj->initDone = TRUE;
    }
}

static Void avsync_deinit_hlos(AvsyncLinkHLOS_Obj *pObj)
{
    if (TRUE == pObj->initDone)
    {
        OSA_assert (AVSYNC_SROBJ_CREATE_PROCID != System_getSelfProcId());
        avsync_close_srobj(pObj);
        pObj->wallTimeBase = 0;
        pObj->initDone = FALSE;
    }
}

UInt64 Avsync_getWallTime()
{
    OSA_assert(TRUE == gAvsyncLink_obj.initDone);
    return (gAvsyncLink_obj.srObj->wallTimer.curWallTime);
}

static Int avsync_audquecreate_validate_params(AvsyncLink_AudQueCreateParams *cp)
{
    Int status = AVSYNC_S_OK;

    if(!((cp->maxElements > 0)
         &&
         (cp->chNum < AVSYNC_MAX_CHANNELS_PER_DISPLAY)
         &&
         (cp->audioDevID < AVSYNC_MAX_NUM_AUDIO_PLAYOUT_DEVICES)))
    {
        status = AVSYNC_E_INVALIDPARAMS;
    }
    return status;
}

static Void avsync_audstats_reset(Avsync_AudStats *stats)
{
    OSA_assert(stats != NULL);
    memset(stats,0,sizeof(*stats));
}

static Void avsync_audque_create_register(AvsyncLinkHLOS_Obj *pObj)
{
    AVSYNC_CRITICAL_BEGIN();

    pObj->srObj->numAudQueCreated++;

    AVSYNC_CRITICAL_END();
}

Int Avsync_audQueCreate(AvsyncLink_AudQueCreateParams *cp,
                        AvsyncLink_AudQueObj *queObj)
{
    Int status = AVSYNC_S_OK;
    AvsyncLinkHLOS_Obj *pObj = &gAvsyncLink_obj;

    OSA_assert(TRUE == pObj->initDone);
    status = avsync_audquecreate_validate_params(cp);
    if (!OSA_ISERROR(status))
    {
        queObj->cp = *cp;
        status = OSA_queCreate(&queObj->audFrmQ,
                                 queObj->cp.maxElements);
    }
    if (!OSA_ISERROR(status))
    {
        Avsync_MasterSynchInfo masterSynchInfo;
        OSA_assert(pObj->srObj != NULL);

        masterSynchInfo.audioDevId = cp->audioDevID;
        status =
        System_linkControl(SYSTEM_LINK_ID_AVSYNC,
                           AVSYNC_LINK_CMD_GETVIDSYNCHCHINFO,
                           &masterSynchInfo,
                           sizeof(masterSynchInfo),
                           TRUE);
        queObj->videoStreamDisplayID = masterSynchInfo.displayId;
        queObj->syncMasterChnum = masterSynchInfo.synchMasterChNum;
        if (masterSynchInfo.synchMasterChNum == queObj->cp.chNum)
        {
            AVSYNC_CRITICAL_BEGIN();
            queObj->playerTime
              = &pObj->srObj->playerTime[queObj->videoStreamDisplayID][queObj->cp.chNum];
            if (queObj->playerTime->doInit)
            {
                avsync_init_player_timer(queObj->playerTime);
                queObj->playerTime->doInit = FALSE;
            }
            queObj->refClk
                = &pObj->srObj->refClk[queObj->videoStreamDisplayID];
            if (queObj->refClk->doInit)
            {
                avsync_init_refclkobj(queObj->refClk);
                queObj->refClk->doInit = FALSE;
            }
            queObj->stats
              = &pObj->srObj->audstats[queObj->videoStreamDisplayID][queObj->cp.chNum];
            avsync_audstats_reset(queObj->stats);
            queObj->cfg = &pObj->srObj->queCfg[queObj->videoStreamDisplayID][queObj->cp.chNum];
            queObj->playTimerStartTimeout = queObj->cfg->playTimerStartTimeout;
            AVSYNC_CRITICAL_END();
        }
        else
        {
            queObj->cfg = NULL;
            queObj->playerTime = NULL;
            queObj->refClk = NULL;
            queObj->stats = NULL;
        }
        AVSYNC_AUDQUE_INIT_STATE(queObj->state);
        AVSYNC_AUDQUE_SET_STATE(queObj->state,AVSYNC_AUDQUE_STATE_CREATED);
        avsync_audque_create_register(pObj);
    }
    return status;
}

static Void avsync_audque_delete_register(AvsyncLinkHLOS_Obj *pObj)
{
    OSA_assert(pObj->srObj->numAudQueCreated > 0);

    AVSYNC_CRITICAL_BEGIN();
    pObj->srObj->numAudQueCreated--;
    AVSYNC_CRITICAL_END();
}

Int Avsync_audQueDelete(AvsyncLink_AudQueObj *queObj)
{
    Int status = AVSYNC_S_OK;


    status = OSA_queDelete(&queObj->audFrmQ);
    OSA_assert(status == 0);

    AVSYNC_CRITICAL_BEGIN();
    if (queObj->cfg)
    {
        queObj->cfg->chNum = AVSYNC_INVALID_CHNUM;
    }
    if (queObj->playerTime)
    {
        queObj->playerTime->doInit = TRUE;
    }
    if (queObj->refClk)
    {
        queObj->refClk->doInit = TRUE;
    }
    AVSYNC_CRITICAL_END();
    AVSYNC_AUDQUE_INIT_STATE(queObj->state);
    avsync_audque_delete_register(&gAvsyncLink_obj);


    return status;
}


static
UInt64 avsync_get_frame_pts(AudFrm_Buf *frame)
{
    UInt64 pts;

    pts = frame->timestamp;
    return pts;
}

Int Avsync_audQuePut(AvsyncLink_AudQueObj *queObj,
                     AudFrm_Buf *frame)
{
    Int status;
    UInt64 framePTS;

    if (queObj->cfg)
    {
        if (!AVSYNC_AUDQUE_CHECK_STATE(queObj->state,AVSYNC_AUDQUE_STATE_FIRSTFRMRECEIVED))
        {
            AVSYNC_CRITICAL_BEGIN();
            if (AVSYNC_PTS_INIT_MODE_AUTO == queObj->cfg->ptsInitMode)
            {
                framePTS = avsync_get_frame_pts(frame);
                if (framePTS != AVSYNC_INVALID_PTS)
                {
                    AVSYNC_AUDQUE_SET_STATE(queObj->state,AVSYNC_AUDQUE_STATE_FIRSTFRMRECEIVED);
                    avsync_set_player_time_firstaudpts(queObj->playerTime,framePTS);
                }
            }
            else
            {
                AVSYNC_AUDQUE_SET_STATE(queObj->state,AVSYNC_AUDQUE_STATE_FIRSTFRMRECEIVED);
            }
            AVSYNC_CRITICAL_END();
        }
    }
    status =
    OSA_quePut(&queObj->audFrmQ,(Int32)frame,OSA_TIMEOUT_NONE);

    OSA_assert(status == 0);

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

static Int32 avsync_adjust_refclk(Avsync_RefClkObj *refClk,UInt64 newMediaBaseTS)
{
    Avsync_PlayerTimeObj *refPlayerTime;
    Int32 clkAdjustDelta;

    refPlayerTime = refClk->masterPlayerTime;
    OSA_assert(refPlayerTime->state == AVSYNC_PLAYERTIME_STATE_RUNNING);
    AVSYNC_CRITICAL_BEGIN();
    avsync_set_player_timer (refPlayerTime,
                             newMediaBaseTS,
                             refPlayerTime->scalingFactor,
                             &clkAdjustDelta);
    OSA_printf("AVSYNC:avsync_adjust_refclk:"
               " Clock Adjust delta[%d]",
               clkAdjustDelta);
    avsync_adjust_synched_clks(refClk,clkAdjustDelta);
    AVSYNC_CRITICAL_END();
    return clkAdjustDelta;
}

static AVSYNC_FRAME_RENDER_FLAG avsync_get_audframe_render_flag(Avsync_PlayerTimeObj *playerTime,
                                                                Avsync_AudStats *audStats,
                                                                Int32 delta,
                                                                Int32 maxLead,
                                                                Int32 maxLag)
{
    AVSYNC_FRAME_RENDER_FLAG renderFlag = AVSYNC_RenderFrame;

    if ((delta < 0) && (delta > -maxLead))
    {
        renderFlag = AVSYNC_RepeatFrame;
        audStats->numFramesReplayed++;
    }
    else
    {
        if ((delta >= 0) && (delta < maxLag))
        {
            renderFlag = AVSYNC_RenderFrame;
            audStats->numFramesPlayed++;
        }
        else
        {
            renderFlag = AVSYNC_DropFrame;
            if (delta > maxLag)
            {
                audStats->numFramesSkippedEarly++;
            }
            else
            {
                audStats->numFramesSkippedLate++;
            }
            //Vps_printf("AVSYNC:FrameSkip:Delta[%d]",delta);
        }
    }
    return renderFlag;
}

#define AVSYNC_ENABLE_LOG     (TRUE)
#ifdef AVSYNC_ENABLE_LOG
Avsync_logAudSynchTbl  gAvsyncLogAudSyncTbl =
{
    .index = 0,
};

static Void avsync_log_print()
{
    Int i;

    for (i = 1; i < AVSYNC_LOG_AUDSYNC_INFO_MAX_RECORDS;i++)
    {
        OSA_printf("AVSYNC:%u,%u,%d",
                    (UInt32)(gAvsyncLogAudSyncTbl.rec[i].curWallTime),
                    (UInt32)(gAvsyncLogAudSyncTbl.rec[i].audFrmPTS),
                    gAvsyncLogAudSyncTbl.rec[i].clkAdjustDelta);
    }
}

static Void avsync_log_add_audsync_info(UInt64 renderTS,UInt64 framePTS,
                                        Int32  clkAdjustDelta)
{
    gAvsyncLogAudSyncTbl.index++;
    if (gAvsyncLogAudSyncTbl.index >= AVSYNC_LOG_AUDSYNC_INFO_MAX_RECORDS)
    {
        gAvsyncLogAudSyncTbl.index = 0;
        avsync_log_print();
    }
    gAvsyncLogAudSyncTbl.rec[gAvsyncLogAudSyncTbl.index].curWallTime = renderTS;
    gAvsyncLogAudSyncTbl.rec[gAvsyncLogAudSyncTbl.index].audFrmPTS = framePTS;
    gAvsyncLogAudSyncTbl.rec[gAvsyncLogAudSyncTbl.index].clkAdjustDelta = clkAdjustDelta;
}


#endif

static AVSYNC_FRAME_RENDER_FLAG avsync_synch_audframe(AvsyncLink_AudQueObj *queObj,
                                                      UInt64 framePTS,
                                                      UInt64 renderTS)
{
    Int32 delta,avgDelta,clkAdjustDelta = 0;
    AVSYNC_FRAME_RENDER_FLAG renderFlag = AVSYNC_RenderFrame;


    OSA_assert(queObj->cfg->avsyncEnable);
    delta = AVSYNC_GET_PTS_DELTA(renderTS,framePTS);

    if (AVSYNC_IS_MASTER_SYNC_CHANNEL(queObj->syncMasterChnum,queObj->cfg->chNum)
        &&
        (queObj->cfg->clkAdjustPolicy.refClkType == AVSYNC_REFCLKADJUST_BYAUDIO))
    {
        if(!AVSYNC_AUDQUE_CHECK_STATE(queObj->state,AVSYNC_AUDQUE_STATE_FIRSTFRMPLAYED))
        {
            if (queObj->cfg->blockOnAudioGet)
            {
                OSA_printf("AVSYNC:Waiting till audio is synched with player time.Duration:%d\n",
                           -delta);
                while (avsync_get_audframe_render_flag(queObj->playerTime,
                                                       queObj->stats,
                                                       delta,
                                                       queObj->cfg->clkAdjustPolicy.clkAdjustLead,
                                                       queObj->cfg->clkAdjustPolicy.clkAdjustLag) == AVSYNC_RepeatFrame)
                {
                    OSA_waitMsecs(AVSYNC_SYNCH_START_CHECK_RETRY_MS);
                }
                OSA_printf("AVSYNC:Waiting till audio is synched with player time..done\n");
            }
            AVSYNC_AUDQUE_SET_STATE(queObj->state,AVSYNC_AUDQUE_STATE_FIRSTFRMPLAYED);
        }
        if (!(AVSYNC_IS_VIDEO_MASTER_SYCH_STATE(queObj->playerTime->playState)))
        {
            avgDelta = avsync_get_simple_moving_avg(&queObj->refClk->ptsDiffAvg,
                                                    delta);
            if ((avgDelta >= queObj->cfg->clkAdjustPolicy.clkAdjustLag)
                ||
                (avgDelta <= -queObj->cfg->clkAdjustPolicy.clkAdjustLead))
            {
                //framePTS += delta - avgDelta;
                OSA_printf("AVSYNC:RefCLk Adjust ."
                           "Delta[%d],AvgDelta[%d],PTS Adjust[%d],MaxLead[%d],MaxLag[%d]",
                           delta,
                           avgDelta,
                           (UInt32)framePTS,
                           queObj->cfg->clkAdjustPolicy.clkAdjustLead,
                           queObj->cfg->clkAdjustPolicy.clkAdjustLag);
                avsync_print_smavobj(&queObj->refClk->ptsDiffAvg);
                clkAdjustDelta = avsync_adjust_refclk(queObj->refClk,framePTS);
            }
            avsync_log_add_audsync_info(renderTS,framePTS,clkAdjustDelta);
        }
        else
        {
            Bool retryRender = FALSE;

            OSA_assert(AVSYNC_IS_VIDEO_MASTER_SYCH_STATE(queObj->playerTime->state));
            do {
                renderFlag =
                avsync_get_audframe_render_flag(queObj->playerTime,
                                                queObj->stats,
                                                delta,
                                                queObj->cfg->clkAdjustPolicy.clkAdjustLead,
                                                queObj->cfg->clkAdjustPolicy.clkAdjustLag);
                if ((AVSYNC_RepeatFrame == renderFlag)
                    &&
                    (queObj->cfg->blockOnAudioGet))
                {
                    OSA_waitMsecs(AVSYNC_AUDIO_GET_RETRY_MS);
                    retryRender = TRUE;
                }
            } while(retryRender);
        }
    }
    return renderFlag;
}


static Bool avsync_handle_player_time_start(AvsyncLink_AudQueObj *queObj,
                                            AudFrm_Buf *frame)
{
    Bool playerTimeStarted = FALSE;

    if (AVSYNC_IS_MASTER_SYNC_CHANNEL(queObj->syncMasterChnum,queObj->cfg->chNum))
    {
        Bool playerTimeStarted;

        OSA_assert(queObj->playerTime == queObj->refClk->masterPlayerTime);
        if ((queObj->cfg->videoPresent) &&
            (queObj->cfg->playStartMode == AVSYNC_PLAYBACK_START_MODE_WAITSYNCH))
        {
            playerTimeStarted = Avsync_audQueWaitForPlayerStart(queObj,queObj->cfg->playTimerStartTimeout);
        }
        if (FALSE == playerTimeStarted)
        {
            Int i;
            UInt64 curWallTime = Avsync_getWallTime();

            AVSYNC_CRITICAL_BEGIN();
            avsync_init_player_time_walltimebase(queObj->playerTime,curWallTime);
            for (i = 0; i < queObj->refClk->numSynchedChannels;i++)
            {
                Avsync_PlayerTimeObj *playerTime = queObj->refClk->synchChannelList[i];
                avsync_init_player_time_walltimebase(playerTime,curWallTime);
                avsync_start_player_time(playerTime);
            }
            playerTimeStarted = avsync_start_player_time(queObj->playerTime);
            AVSYNC_CRITICAL_END();
            OSA_assert(TRUE == playerTimeStarted);
        }
    }
    return playerTimeStarted;
}

Int Avsync_audQueGet(AvsyncLink_AudQueObj *queObj,
                     AudFrm_Buf **framePtr,
                     AudFrm_BufList *freeFrameList,
                     AvsyncLink_AudioBackEndInfo *abeInfo)
{
    Int status = -1;
    UInt64 lastSTCa;
    UInt64 framePTS;
    UInt64 renderTS;

    *framePtr = NULL;

    while (OSA_queGetQueuedCount(&queObj->audFrmQ))
    {
        if ((queObj->cfg)
            &&
            (queObj->cfg->avsyncEnable))
        {
             AudFrm_Buf *frame;

             status = OSA_quePeek(&queObj->audFrmQ,(Int32 *)&frame);
             OSA_assert((frame != NULL) && (status == 0));
             framePTS = avsync_get_frame_pts(frame);
             if (queObj->playerTime->state != AVSYNC_PLAYERTIME_STATE_RUNNING)
             {
                 Bool playerTimeStarted;

                 playerTimeStarted =
                 avsync_handle_player_time_start(queObj,frame);
             }
             if (queObj->playerTime->state == AVSYNC_PLAYERTIME_STATE_RUNNING)
             {
                 AVSYNC_FRAME_RENDER_FLAG renderFlag;

                 lastSTCa = abeInfo->aSyncSTC
                            +
                            abeInfo->audioBackendDelay;
                 renderTS =
                 avsync_get_player_timer(queObj->playerTime,lastSTCa);
                 renderFlag = avsync_synch_audframe(queObj,framePTS,renderTS);
                 if ((AVSYNC_RenderFrame == renderFlag)
                     ||
                     (AVSYNC_DropFrame == renderFlag))
                 {
                    status = OSA_queGet(&queObj->audFrmQ,
                                        (Int32 *)&frame,
                                        OSA_TIMEOUT_NONE);
                    OSA_assert((frame != NULL) && (status == 0));
                    frame->muteFlag =
                    AVSYNC_IS_MUTE_AUDIO_STATE(queObj->playerTime->playState);
                    if (AVSYNC_DropFrame == renderFlag)
                    {
                        OSA_assert(freeFrameList->numFrames <
                                   OSA_ARRAYSIZE(freeFrameList->frames));
                        freeFrameList->frames[freeFrameList->numFrames] =
                            frame;
                        freeFrameList->numFrames++;
                        frame = NULL;
                    }
                }
                if ((AVSYNC_RenderFrame == renderFlag)
                     ||
                     (AVSYNC_RepeatFrame == renderFlag))
                {
                    if (AVSYNC_RenderFrame == renderFlag)
                    {
                        queObj->playerTime->lastAudFrmPTS =
                            framePTS;
                        *framePtr = frame;
                    }
                    break;
                }
            }
            else
            {
                break;
            }
        }
        else
        {
            status =
                OSA_queGet(&queObj->audFrmQ,
                           (Int32 *)framePtr,OSA_TIMEOUT_NONE);
            OSA_assert(status == 0);
            break;
        }
    }
    return status;
}

Int Avsync_audQueFlush(AvsyncLink_AudQueObj *queObj,
                       AudFrm_Buf **framePtr,
                       AudFrm_BufList *freeFrameList)
{
    Int32 status = AVSYNC_S_OK;
    AudFrm_Buf *frame = NULL;

    while (!OSA_queIsEmpty(&queObj->audFrmQ))
    {
        if (frame != NULL)
        {
            OSA_assert(freeFrameList->numFrames <
                       OSA_ARRAYSIZE(freeFrameList->frames));
            freeFrameList->frames[freeFrameList->numFrames] =
                frame;
            freeFrameList->numFrames++;
            frame = NULL;
        }
        status = OSA_queGet(&queObj->audFrmQ,
                           (Int32 *)&frame,
                            OSA_TIMEOUT_NONE);
        OSA_assert(status == 0);
    }
    *framePtr = frame;
    return status;
}

Bool Avsync_audQueWaitForPlayerStart(AvsyncLink_AudQueObj *queObj,
                                     Int32 timeOut)
{
    Bool playerStarted = FALSE;

    if (queObj->cfg)
    {
        while(timeOut > 0)
        {
            if (Avsync_audQueGetPlayerState(queObj) != AVSYNC_PLAYERTIME_STATE_RUNNING)
            {
                OSA_waitMsecs(AVSYNC_PLAYER_STATE_CHECK_RETRY_MS);
                timeOut -= AVSYNC_PLAYER_STATE_CHECK_RETRY_MS;
            }
            else
            {
                break;
            }
        }
        if(Avsync_audQueGetPlayerState(queObj) == AVSYNC_PLAYERTIME_STATE_RUNNING)
        {
            playerStarted = TRUE;
        }
    }
    return playerStarted;
}

Avsync_PlayerTimeState Avsync_audQueGetPlayerState(AvsyncLink_AudQueObj *queObj)
{
    if (queObj->playerTime)
    {
        return ((Avsync_PlayerTimeState)queObj->playerTime->state);
    }
    else
    {
        return AVSYNC_PLAYERTIME_STATE_RUNNING;
    }
}



Int32 AvsyncLink_init()
{
    Int32 status;
    Avsync_InitParams initParams;

    initParams.wallTimeInitTS =
        gAvsyncLink_obj.wallTimeBase;
    status =
    System_linkControl(SYSTEM_LINK_ID_AVSYNC,
                       AVSYNC_LINK_CMD_INIT,
                       &initParams,
                       sizeof(initParams),
                       TRUE);
    OSA_assert(status == 0);
    avsync_init_hlos(&gAvsyncLink_obj);
    return status;
}

Int32 AvsyncLink_deInit()
{
    Int32 status;

    avsync_deinit_hlos(&gAvsyncLink_obj);
    status =
    System_linkControl(SYSTEM_LINK_ID_AVSYNC,
                       AVSYNC_LINK_CMD_DEINIT,
                       NULL,
                       0,
                       TRUE);
    OSA_assert(status == 0);

    return status;
}


Int32 Avsync_configSyncConfigInfo(AvsyncLink_LinkSynchConfigParams *cfg)
{
    Int32 status;

    OSA_assert(TRUE == gAvsyncLink_obj.initDone);
    status =
    System_linkControl(SYSTEM_LINK_ID_AVSYNC,
                       AVSYNC_LINK_CMD_AVSYNCCFG,
                       cfg,
                       sizeof(*cfg),
                       TRUE);
    return status;
}

Int32 Avsync_setPlaybackSpeed(Avsync_TimeScaleParams *timeScaleParams)
{
    Int32 status;

    OSA_assert(TRUE == gAvsyncLink_obj.initDone);
    status =
    System_linkControl(SYSTEM_LINK_ID_AVSYNC,
                       AVSYNC_LINK_CMD_TIMESCALE,
                       timeScaleParams,
                       sizeof(*timeScaleParams),
                       TRUE);
    return status;
}

Int32 Avsync_doPause(Avsync_PauseParams *pauseParams)
{
    Int32 status;

    OSA_assert(TRUE == gAvsyncLink_obj.initDone);
    status =
    System_linkControl(SYSTEM_LINK_ID_AVSYNC,
                       AVSYNC_LINK_CMD_PAUSE,
                       pauseParams,
                       sizeof(*pauseParams),
                       TRUE);
    return status;
}


Int32 Avsync_doUnPause(Avsync_UnPauseParams *unPauseParams)
{
    Int32 status;

    OSA_assert(TRUE == gAvsyncLink_obj.initDone);
    status =
    System_linkControl(SYSTEM_LINK_ID_AVSYNC,
                       AVSYNC_LINK_CMD_UNPAUSE,
                       unPauseParams,
                       sizeof(*unPauseParams),
                       TRUE);
    return status;
}

Int32 Avsync_stepFwd(Avsync_StepFwdParams *stepFwdParams)
{
    Int32 status;

    OSA_assert(TRUE == gAvsyncLink_obj.initDone);
    status =
    System_linkControl(SYSTEM_LINK_ID_AVSYNC,
                       AVSYNC_LINK_CMD_STEP_FWD,
                       stepFwdParams,
                       sizeof(*stepFwdParams),
                       TRUE);
    return status;
}


Int32 Avsync_setFirstAudPTS(Avsync_FirstAudPTSParams *audPTSParams)
{
    Int32 status;

    OSA_assert(TRUE == gAvsyncLink_obj.initDone);
    status =
    System_linkControl(SYSTEM_LINK_ID_AVSYNC,
                       AVSYNC_LINK_CMD_SET_FIRST_AUDPTS,
                       audPTSParams,
                       sizeof(*audPTSParams),
                       TRUE);
    return status;
}

Int32 Avsync_setFirstVidPTS(Avsync_FirstVidPTSParams *vidPTSParams)
{
    Int32 status;

    OSA_assert(TRUE == gAvsyncLink_obj.initDone);
    status =
    System_linkControl(SYSTEM_LINK_ID_AVSYNC,
                       AVSYNC_LINK_CMD_SET_FIRST_VIDPTS,
                       vidPTSParams,
                       sizeof(*vidPTSParams),
                       TRUE);
    return status;
}

Int32 Avsync_resetPlayerTimer(Avsync_ResetPlayerTimerParams *resetParams)
{
    Int32 status;

    OSA_assert(TRUE == gAvsyncLink_obj.initDone);
    status =
    System_linkControl(SYSTEM_LINK_ID_AVSYNC,
                       AVSYNC_LINK_CMD_RESET_PLAYERTIME,
                       resetParams,
                       sizeof(*resetParams),
                       TRUE);
    return status;
}

Int32 Avsync_setVideoBackEndDelay(Avsync_VideoBackendDelayParams *delayParams)
{
    Int32 status;

    OSA_assert(TRUE == gAvsyncLink_obj.initDone);
    status =
    System_linkControl(SYSTEM_LINK_ID_AVSYNC,
                       AVSYNC_LINK_CMD_SET_VIDEO_BACKEND_DELAY,
                       delayParams,
                       sizeof(*delayParams),
                       TRUE);
    return status;
}

Int32 Avsync_doPlay(Avsync_PlayParams *playParams)
{
    Int32 status;

    OSA_assert(TRUE == gAvsyncLink_obj.initDone);
    status =
    System_linkControl(SYSTEM_LINK_ID_AVSYNC,
                       AVSYNC_LINK_CMD_PLAY,
                       playParams,
                       sizeof(*playParams),
                       TRUE);
    return status;
}

Int32 Avsync_seekPlayback(Avsync_SeekParams *seekParams)
{
    Int32 status;

    OSA_assert(TRUE == gAvsyncLink_obj.initDone);
    status =
    System_linkControl(SYSTEM_LINK_ID_AVSYNC,
                       AVSYNC_LINK_CMD_SEEK,
                       seekParams,
                       sizeof(*seekParams),
                       TRUE);
    return status;
}


Int32 Avsync_doScan(Avsync_ScanParams *scanParams)
{
    Int32 status;

    OSA_assert(TRUE == gAvsyncLink_obj.initDone);
    status =
    System_linkControl(SYSTEM_LINK_ID_AVSYNC,
                       AVSYNC_LINK_CMD_SCAN,
                       scanParams,
                       sizeof(*scanParams),
                       TRUE);
    return status;
}

static
Void avsync_get_avsync_enabled_channel_list(AvsyncLinkHLOS_Obj *pObj,
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
Avsync_VidStats * avsync_get_vidstats_obj(AvsyncLinkHLOS_Obj *pObj,
                                          UInt32 displayID,
                                          UInt32 chNum)
{
    Avsync_VidStats *vidstats = NULL;

    if ((displayID < AVSYNC_MAX_NUM_DISPLAYS)
        &&
        (chNum < AVSYNC_MAX_CHANNELS_PER_DISPLAY))
    {
        UTILS_assert(pObj->srObj != NULL);
        vidstats = &pObj->srObj->vidstats[displayID][chNum];
    }
    return vidstats;
}

static
Avsync_AudStats * avsync_get_audstats_obj(AvsyncLinkHLOS_Obj *pObj,
                                          UInt32 displayID,
                                          UInt32 chNum)
{
    Avsync_AudStats *audstats = NULL;

    if ((displayID < AVSYNC_MAX_NUM_DISPLAYS)
        &&
        (chNum < AVSYNC_MAX_CHANNELS_PER_DISPLAY))
    {
        UTILS_assert(pObj->srObj != NULL);
        audstats = &pObj->srObj->audstats[displayID][chNum];
    }
    return audstats;
}

static
Avsync_PlayerTimeObj * avsync_get_player_timeobj(AvsyncLinkHLOS_Obj *pObj,
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



Avsync_SynchConfigParams * avsync_get_avsync_config(AvsyncLinkHLOS_Obj *pObj,
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

static
Void avsync_print_player_timer_stats_header(UInt32 displayID,UInt32 numCh)
{
    printf("\n AVSYNC:PlayerTimer Stats"
           "\n AVSYNC:PlayerTimer Stats DisplayID:%d"
           "\n AVSYNC:PlayerTimer Stats NumCh:%d"
           "\n Chn | Positive Negative  LastVid LastAud"
           "\n Num | ClkAdj   ClkAdj    PTS     PTS    "
           "\n     | Count    Count                    "
           "\n ----------------------------------------",displayID,numCh);
}

static
Void avsync_print_player_timer_stats(Avsync_PlayerTimeObj *playerTime,
                                     UInt32 channel)
{
    printf( "\n %3d| %8d %8d %7d %7d",
           channel,
           playerTime->stats.numClockAdjustPositive,
           playerTime->stats.numClockAdjustNegative,
           (UInt32)playerTime->lastVidFrmPTS,
           (UInt32)playerTime->lastAudFrmPTS);
}


static
Void avsync_print_player_timer_stats_all_ch(AvsyncLinkHLOS_Obj *pObj,
                                            UInt32 displayID)
{
    UInt32 numCh;
    UInt32 channelList[AVSYNC_MAX_CHANNELS_PER_DISPLAY];
    Int i;
    Bool headerPrinted = FALSE;

    avsync_get_avsync_enabled_channel_list(pObj,
                                           displayID,
                                           &numCh,
                                           channelList,
                                           AVSYNC_MAX_CHANNELS_PER_DISPLAY);
    for (i = 0; i < numCh;i++)
    {
        Avsync_PlayerTimeObj *playerTime;

        playerTime =
            avsync_get_player_timeobj(pObj,
                                      displayID,
                                      channelList[i]);
        if (playerTime)
        {
            if ((playerTime->state == AVSYNC_PLAYERTIME_STATE_RUNNING)
                &&
                (playerTime->playState != AVSYNC_PLAYER_STATE_RESET))
            {
                if (!headerPrinted)
                {
                    avsync_print_player_timer_stats_header(displayID,numCh);
                    headerPrinted = TRUE;
                }
                avsync_print_player_timer_stats(playerTime,channelList[i]);
            }
        }
    }
}

static
Void avsync_print_vidstats_header(UInt32 displayID,UInt32 numCh)
{
    printf("\n AVSYNC:Video Stats"
           "\n AVSYNC:Video Stats DisplayID:%d"
           "\n AVSYNC:Video Stats NumCh:%d"
           "\n Chn | Play  Replay Skip  Skip  Underrun Overflow "
           "\n     | Count Count  Early Late                    "
           "\n -------------------------------------------------",displayID,numCh);
}

static
Void avsync_print_vidstats(Avsync_VidStats *vidstats,
                           UInt32 channel)
{
    printf( "\n %3d| %5d %6d %5d %4d %9d %8d",
           channel,
           vidstats->numFramesRendered,
           vidstats->numFramesReplayed,
           vidstats->numFramesSkippedEarly,
           vidstats->numFramesSkippedLate,
           vidstats->numUnderrun,
           vidstats->numOverflow);
}

static
Bool avsync_vidstats_is_valid(Avsync_VidStats *vidstats)
{
    Bool statsValid = FALSE;

    if ((vidstats->numFramesRendered != 0)
        ||
        (vidstats->numFramesReplayed != 0)
        ||
        (vidstats->numFramesSkippedEarly != 0)
        ||
        (vidstats->numFramesSkippedLate != 0))
    {
        statsValid = TRUE;
    }
    return statsValid;
}

static
Void avsync_print_vidstats_stats_all_ch(AvsyncLinkHLOS_Obj *pObj,
                                        UInt32 displayID)
{
    UInt32 numCh;
    UInt32 channelList[AVSYNC_MAX_CHANNELS_PER_DISPLAY];
    Int i;
    Bool headerPrinted = FALSE;

    avsync_get_avsync_enabled_channel_list(pObj,
                                           displayID,
                                           &numCh,
                                           channelList,
                                           AVSYNC_MAX_CHANNELS_PER_DISPLAY);
    for (i = 0; i < numCh;i++)
    {
        Avsync_VidStats *vidstats;
        Avsync_PlayerTimeObj *playerTime;

        playerTime =
            avsync_get_player_timeobj(pObj,
                                      displayID,
                                      channelList[i]);
        vidstats =
            avsync_get_vidstats_obj(pObj,
                                    displayID,
                                    channelList[i]);
        if ((playerTime) && (vidstats))
        {
            if ((playerTime->state == AVSYNC_PLAYERTIME_STATE_RUNNING)
                &&
                (playerTime->playState != AVSYNC_PLAYER_STATE_RESET)
                &&
                avsync_vidstats_is_valid(vidstats))
            {
                if (!headerPrinted)
                {
                    avsync_print_vidstats_header(displayID,numCh);
                    headerPrinted = TRUE;
                }
                avsync_print_vidstats(vidstats,channelList[i]);
            }
        }
    }
}

static
Void avsync_print_audstats_header(UInt32 displayID,UInt32 numCh)
{
    printf("\n AVSYNC:Audio Stats"
           "\n AVSYNC:Audio Stats DisplayID:%d"
           "\n AVSYNC:Audio Stats NumCh:%d"
           "\n Chn | Play  Replay Skip  Skip"
           "\n     | Count Count  Early Late"
           "\n -----------------------------",displayID,numCh);
}

static
Void avsync_print_audstats(Avsync_AudStats *audstats,
                           UInt32 channel)
{
    printf( "\n %3d| %5d %6d %5d %4d",
           channel,
           audstats->numFramesPlayed,
           audstats->numFramesReplayed,
           audstats->numFramesSkippedEarly,
           audstats->numFramesSkippedLate);
}

static
Bool avsync_audstats_is_valid(Avsync_AudStats *audstats)
{
    Bool statsValid = FALSE;

    if ((audstats->numFramesPlayed != 0)
        ||
        (audstats->numFramesReplayed != 0)
        ||
        (audstats->numFramesSkippedEarly != 0)
        ||
        (audstats->numFramesSkippedLate != 0))
    {
        statsValid = TRUE;
    }
    return statsValid;
}

static
Void avsync_print_audstats_stats_all_ch(AvsyncLinkHLOS_Obj *pObj,
                                        UInt32 displayID)
{
    UInt32 numCh;
    UInt32 channelList[AVSYNC_MAX_CHANNELS_PER_DISPLAY];
    Int i;
    Bool headerPrinted = FALSE;

    avsync_get_avsync_enabled_channel_list(pObj,
                                           displayID,
                                           &numCh,
                                           channelList,
                                           AVSYNC_MAX_CHANNELS_PER_DISPLAY);
    for (i = 0; i < numCh;i++)
    {
        Avsync_AudStats *audstats;
        Avsync_PlayerTimeObj *playerTime;

        playerTime =
            avsync_get_player_timeobj(pObj,
                                      displayID,
                                      channelList[i]);
        audstats =
            avsync_get_audstats_obj(pObj,
                                    displayID,
                                    channelList[i]);
        if ((playerTime) && (audstats))
        {
            if ((playerTime->state == AVSYNC_PLAYERTIME_STATE_RUNNING)
                &&
                (playerTime->playState != AVSYNC_PLAYER_STATE_RESET)
                &&
                avsync_audstats_is_valid(audstats))
            {
                if (!headerPrinted)
                {
                    avsync_print_audstats_header(displayID,numCh);
                    headerPrinted = TRUE;
                }
                avsync_print_audstats(audstats,channelList[i]);
            }
        }
    }
}

static
Int32 avsync_get_ts_avg_delta(Int32 numEntries,UInt64 tsLog[])
{
    Int i;
    Int32 totalTsDelta = 0;
    Int32 avgTsDelta;

    for (i = 1; i < numEntries; i++)
    {
        totalTsDelta += tsLog[i] -
                        tsLog[i - 1];
    }
    avgTsDelta = OSA_DIV(totalTsDelta,numEntries);
    return avgTsDelta;
}

static
Void avsync_print_capturetsdelta_header()
{
    printf("\n AVSYNC:Capture TS Avg Delta "
           "\n Chn | Avg                   "
           "\n     | Delta                 "
           "\n ----------------------------");
}

static
Void avsync_print_ipctsdelta_header()
{
    printf("\n AVSYNC:IPCBITSOUT TS Avg Delta"
           "\n Chn | Avg                   "
           "\n     | Delta                 "
           "\n ----------------------------");
}

static
Void avsync_print_tsdelta(Int32 avgTsDelta,
                          UInt32 channel)
{
    printf( "\n %3d| %5d",
           channel,
           avgTsDelta);
}

static
Void avsync_print_capturetsdelta_all_ch(AvsyncLinkHLOS_Obj *pObj)
{
    AvsyncLink_TimestampLog *capTsLog = AVSYNC_GET_CAPTURE_TS_HANDLE();
    Int i;
    Int32 avgTsDelta;
    Bool headerPrinted = FALSE;

    for (i = 0; i < AVSYNC_LOG_CAPTURE_TS_NUMCH;i++)
    {
        avgTsDelta =
          avsync_get_ts_avg_delta(capTsLog[i].logIndex,
                                  capTsLog[i].ts);
        if (!headerPrinted)
        {
            avsync_print_capturetsdelta_header();
            headerPrinted = TRUE;
        }
        avsync_print_tsdelta(avgTsDelta,i);
    }
}

static
Void avsync_print_ipcbitsouttsdelta_all_ch(AvsyncLinkHLOS_Obj *pObj)
{
    AvsyncLink_TimestampLog *ipcTsLog = AVSYNC_GET_IPCBITSOUT_TS_HANDLE();
    Int i;
    Int32 avgTsDelta;
    Bool headerPrinted = FALSE;

    for (i = 0; i < AVSYNC_LOG_CAPTURE_TS_NUMCH;i++)
    {
        avgTsDelta =
          avsync_get_ts_avg_delta(ipcTsLog[i].logIndex,
                                  ipcTsLog[i].ts);
        if (!headerPrinted)
        {
            avsync_print_ipctsdelta_header();
            headerPrinted = TRUE;
        }
        avsync_print_tsdelta(avgTsDelta,i);
    }
}


static
Void avsync_print_vidsynch_master_stats_record_summary(Int32  avgPTSDelta,
                                                UInt32 numFramesPlayed,
                                                UInt32 numFramesSkipped,
                                                UInt32 numFramesReplayed,
                                                UInt32 numClockAdjust)
{
    printf("\n AVGPTSDELTA NUMPLAY NUMSKIP NUMREPLAY NUMCLKADJ");
    printf("\n %11d %7d %7d %9d %9d",
           avgPTSDelta,
           numFramesPlayed,
           numFramesSkipped,
           numFramesReplayed,
           numClockAdjust);
}

static
Void avsync_print_vidsynch_slave_stats_record_summary_header()
{
    printf("\n ChNum | AVGPTSDELTA NUMPLAY NUMSKIP NUMREPLAY NUMUNDERUN");
}

static
Void avsync_print_vidsynch_slave_stats_record_summary(
                                                UInt32 chNum,
                                                Int32  avgPTSDelta,
                                                UInt32 numFramesPlayed,
                                                UInt32 numFramesSkipped,
                                                UInt32 numFramesReplayed,
                                                UInt32 numClockAdjust)
{
    printf("\n %5d | %11d %7d %7d %9d %10d",
           chNum,
           avgPTSDelta,
           numFramesPlayed,
           numFramesSkipped,
           numFramesReplayed,
           numClockAdjust);
}



static
Void avsync_get_vidsynch_masterch_info(Avsync_logVidMasterRecord masterRec[],
                                       UInt32 numRecord,
                                       Int32  *avgPTSDelta,
                                       UInt32 *numFramesPlayed,
                                       UInt32 *numFramesSkipped,
                                       UInt32 *numFramesReplayed,
                                       UInt32 *numClockAdjust)
{
    Int i;

    *avgPTSDelta       = 0;
    *numFramesPlayed   = 0;
    *numFramesSkipped  = 0;
    *numFramesReplayed = 0;
    for (i = 0; i < numRecord; i++)
    {
        switch (masterRec[i].renderFlag)
        {
            case AVSYNC_RenderFrame:
            {
                *numFramesPlayed = *numFramesPlayed + 1;
                *avgPTSDelta +=    masterRec[i].renderPTS -
                           masterRec[i].synchMasterPTS -
                           masterRec[i].clkAdjustDelta;
            }
            break;
            case AVSYNC_RepeatFrame:
            {
                *numFramesReplayed = *numFramesReplayed + 1;
            }
            break;
            case AVSYNC_DropFrame:
            {
                *numFramesSkipped = *numFramesSkipped + 1;
            }
            break;
            default:
            break;
        }
        if (masterRec[i].clkAdjustDelta)
        {
            *numClockAdjust = *numClockAdjust + 1;
        }
    }
    *avgPTSDelta = OSA_DIV(*avgPTSDelta,((Int32)(*numFramesPlayed)));
}

static
Void avsync_get_vidsynch_slavech_info(Avsync_logVidSlaveRecord slaveRec[],
                                      UInt32 numRecord,
                                       Int32  *avgPTSDelta,
                                       UInt32 *numFramesPlayed,
                                       UInt32 *numFramesSkipped,
                                       UInt32 *numFramesReplayed,
                                       UInt32 *numUnderrun)
{
    Int i;

    *avgPTSDelta       = 0;
    *numFramesPlayed   = 0;
    *numFramesSkipped  = 0;
    *numFramesReplayed = 0;
    *numUnderrun       = 0;
    for (i = 0; i < numRecord; i++)
    {
        switch (slaveRec[i].renderFlag)
        {
            case AVSYNC_RenderFrame:
            {
                *numFramesPlayed = *numFramesPlayed + 1;
                if (slaveRec[i].underRun)
                {
                    *numUnderrun = *numUnderrun + 1;
                }
                else
                {
                    *avgPTSDelta +=    slaveRec[i].syncMasterPTSDelta;
                }
            }
            break;
            case AVSYNC_RepeatFrame:
            {
                *numFramesReplayed = *numFramesReplayed + 1;
            }
            break;
            case AVSYNC_DropFrame:
            {
                *numFramesSkipped = *numFramesSkipped + 1;
            }
            break;
            default:
            break;
        }
    }
    *avgPTSDelta = OSA_DIV(*avgPTSDelta,((Int32)(*numFramesPlayed - *numUnderrun)));
}

static
Void avsync_print_vidsynch_stats_record(Avsync_logVidSynchTbl *vidSyncTbl)
{
    Int i;
    Int32  avgPTSDelta;
    UInt32 numFramesPlayed;
    UInt32 numFramesSkipped;
    UInt32 numFramesReplayed;
    UInt32 numClockAdjust;
    UInt32 numUnderrun;
    Bool headerPrinted = FALSE;

    avsync_get_vidsynch_masterch_info(&vidSyncTbl->masterRec.rec[0],
                                      vidSyncTbl->masterRec.index,
                                      &avgPTSDelta,
                                      &numFramesPlayed,
                                      &numFramesSkipped,
                                      &numFramesReplayed,
                                      &numClockAdjust);
    avsync_print_vidsynch_master_stats_record_summary(avgPTSDelta,
                                                      numFramesPlayed,
                                                      numFramesSkipped,
                                                      numFramesReplayed,
                                                      numClockAdjust);
    for (i = 0; i < AVSYNC_LOG_MAX_SLAVE_CHANNELS; i++)
    {
        avsync_get_vidsynch_slavech_info(&(vidSyncTbl->slaveRec[i].rec[0]),
                                          vidSyncTbl->slaveRec[i].index,
                                          &avgPTSDelta,
                                          &numFramesPlayed,
                                          &numFramesSkipped,
                                          &numFramesReplayed,
                                          &numUnderrun);
        if (!headerPrinted)
        {
            avsync_print_vidsynch_slave_stats_record_summary_header();
            headerPrinted = TRUE;
        }
        avsync_print_vidsynch_slave_stats_record_summary(
                                          i,
                                          avgPTSDelta,
                                          numFramesPlayed,
                                          numFramesSkipped,
                                          numFramesReplayed,
                                          numUnderrun);
    }

    #ifdef AVSYNC_LOG_VERBOSE_PRINT
    {
        Avsync_logVidMasterRecord *masterRec;
        Avsync_logVidSlaveRecord *slaveRec;
        Int j;

        printf("\n WALLTIME RF PTS_MAST CLKADJ");
        for (i = 0; i < vidSyncTbl->masterRec.index; i++)
        {
            masterRec = &vidSyncTbl->masterRec.rec[i];
            printf("\n %8d %2d %8d %6d",
                    (UInt32)masterRec->renderPTS,
                    masterRec->renderFlag,
                    (UInt32)masterRec->synchMasterPTS,
                    masterRec->clkAdjustDelta);
        }
        for (i = 0; i < AVSYNC_LOG_MAX_SLAVE_CHANNELS; i++)
        {
            Bool headerPrinted = FALSE;
            for (j = 0; j < vidSyncTbl->slaveRec[i].index;j++)
            {
                if (!headerPrinted)
                {
                    printf("\n RF PTSDEL UNDERRUN");;
                    headerPrinted = TRUE;
                }
                slaveRec = &vidSyncTbl->slaveRec[i].rec[j];
                printf("\n %2d %6d %2d",
                       slaveRec->renderFlag,
                       slaveRec->syncMasterPTSDelta,
                       slaveRec->underRun);
            }
        }
    }
    #endif /* AVSYNC_LOG_VERBOSE_PRINT */
}

static
Void avsync_print_vidsynch_stats_all_ch(AvsyncLinkHLOS_Obj *pObj,
                                        UInt32 displayID)
{
    Avsync_logVidSynchTbl *syncTbl = AVSYNC_GET_LOG_TBL_HANDLE();
    avsync_print_vidsynch_stats_record(&(syncTbl[displayID]));
}


Int32 Avsync_printStats()
{
    AvsyncLinkHLOS_Obj *pObj = &gAvsyncLink_obj;
    UInt32 i;
    Int32 status = AVSYNC_S_OK;

    for (i = 0; i < AVSYNC_MAX_NUM_DISPLAYS;i++)
    {
        avsync_print_player_timer_stats_all_ch(pObj,i);
        avsync_print_vidstats_stats_all_ch(pObj,i);
        avsync_print_audstats_stats_all_ch(pObj,i);
    }
    avsync_print_capturetsdelta_all_ch(pObj);
    avsync_print_ipcbitsouttsdelta_all_ch(pObj);
    for (i = 0; i < AVSYNC_MAX_NUM_DISPLAYS;i++)
    {
        avsync_print_vidsynch_stats_all_ch(pObj,i);
    }

    return status;
}

Int32 Avsync_setWallTimeBase(UInt64 wallTimeBase)
{
    AvsyncLinkHLOS_Obj *pObj = &gAvsyncLink_obj;
    Int32 status = AVSYNC_S_OK;

    if (!pObj->initDone)
    {
        pObj->wallTimeBase = wallTimeBase;
    }
    else
    {
        status = AVSYNC_E_INVALIDPARAMS;
    }
    return status;
}

#ifdef SYSTEM_DEBUG_AVSYNC_DETAILED_LOGS

Void AvsyncLink_logIpcBitsOutTS(UInt32 chNum, UInt64 ts)
{
    AvsyncLink_TimestampLog *ipcBitsTsLog = AVSYNC_GET_IPCBITSOUT_TS_HANDLE();

    if (chNum < AVSYNC_LOG_IPC_TS_NUMCH)
    {
        if (ipcBitsTsLog[chNum].logIndex >= AVSYNC_LOG_TS_MAX_RECORDS)
            ipcBitsTsLog[chNum].logIndex = 0;
        ipcBitsTsLog[chNum].ts[ipcBitsTsLog[chNum].logIndex] = ts;
        ipcBitsTsLog[chNum].logIndex++;
    }
}
#else
Void AvsyncLink_logIpcBitsOutTS(UInt32 chNum, UInt64 ts)
{
    (Void)chNum;
    (Void)ts;
}
#endif
