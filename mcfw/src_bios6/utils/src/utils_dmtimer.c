/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/
#include <xdc/std.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/Assert.h>
#include <xdc/runtime/Startup.h>
#include <xdc/runtime/Types.h>
#include <xdc/runtime/Timestamp.h>

#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/hal/Hwi.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/timers/dmtimer/Timer.h>
#include <mcfw/src_bios6/utils/utils_dmtimer.h>
#include "utils_dmtimer_regcfg.h"

#define UTILS_DMTIMER_FREQ_CALIBRATION_LOOP_LENGTH          (250000)
#define UTILS_DMTIMER_OVERFLOW_CHECK_PERIOD_MS              (1000)
//#define UTILS_DMTIMER_FREQUENCY_AUTODETECT                  (TRUE)

static
UInt32 Utils_dmTimerBaseAddr[] =
{
    UTILS_DMTIMER_TIMER0_BASE_ADDR,
    UTILS_DMTIMER_TIMER1_BASE_ADDR,
    UTILS_DMTIMER_TIMER2_BASE_ADDR,
    UTILS_DMTIMER_TIMER3_BASE_ADDR,
    UTILS_DMTIMER_TIMER4_BASE_ADDR,
    UTILS_DMTIMER_TIMER5_BASE_ADDR,
    UTILS_DMTIMER_TIMER6_BASE_ADDR
};

#ifndef UTILS_DMTIMER_FREQUENCY_AUTODETECT
static
UInt32 Utils_dmTimerFrequency[] =
{
    UTILS_DMTIMER_TIMER0_FREQUENCY,
    UTILS_DMTIMER_TIMER1_FREQUENCY,
    UTILS_DMTIMER_TIMER2_FREQUENCY,
    UTILS_DMTIMER_TIMER3_FREQUENCY,
    UTILS_DMTIMER_TIMER4_FREQUENCY,
    UTILS_DMTIMER_TIMER5_FREQUENCY,
    UTILS_DMTIMER_TIMER6_FREQUENCY
};
#endif


typedef struct Utils_dmTimerPeriodicObj {
    Clock_Struct clkStruct;
    Clock_Handle clkHandle;
    Bool clkStarted;
} Utils_dmTimerPeriodicObj;


typedef struct Utils_dmTimerElem
{
    Bool   freqSet;
    UInt64 freq;
    UInt32 lastTimerCounter;
    UInt32 rolloverCount;
    UInt32 lastPrdCallbackTime;
    UInt32 prdCallbackIntervalInTicks;
    Utils_dmTimerPeriodicObj periodicObj;
    Utils_dmTimerRegs *timerRegs;
    UInt32 timerId;
} Utils_dmTimerElem;


typedef struct Utils_dmTimerObj
{
    Bool initDone;
    Utils_dmTimerElem timer[UTILS_DMTIMER_NUM_TIMERS];
} Utils_dmTimerObj;

Utils_dmTimerObj gUtils_dmTimerObj =
{
    .initDone = FALSE,
};

#define UTILS_DMTIMER_GETTSDELTA(startTime,endTime)                 \
    (((endTime) > (startTime)) ? ((endTime) - (startTime))           \
                                   : ((((UInt32)~0u)- (startTime)) +     \
                                        (endTime)))

#define UTILS_DMTIMER_READTIMERREG(timerReg)       ((timerReg)->tcrr)
#define UTILS_DMTIMER_TICKS2MS(ticks,freq)         ((ticks)/((freq)/1000))
#define UTILS_DMTIMER_TICKS2MICROSEC(ticks,freq)   ((ticks)/((freq)/1000000))


static UInt32 utils_dmtimer_getcnt32(Utils_dmTimerElem *timerElem);

static Void  utils_dmtimer_prd_calloutfcn(UArg arg)
{
    Utils_dmTimerElem *timerElem = (Utils_dmTimerElem *) arg;
    UInt32 curTime;

    /* Call timer-getcnt periodically to ensure
     * rollover is detected */
    curTime = utils_dmtimer_getcnt32(timerElem);
    if (curTime > timerElem->lastPrdCallbackTime)
    {
        timerElem->prdCallbackIntervalInTicks =
        (curTime - timerElem->lastPrdCallbackTime);
    }
    timerElem->lastPrdCallbackTime = curTime;
}

static Void utils_dmtimer_create_prd_obj(Utils_dmTimerElem *timerElem)
{
    Clock_Params clockParams;

    Clock_Params_init(&clockParams);
    clockParams.arg = (UArg) timerElem;
    UTILS_assert(timerElem->periodicObj.clkHandle == NULL);

    Clock_construct(&(timerElem->periodicObj.clkStruct), 
                    utils_dmtimer_prd_calloutfcn,
                    1,
                    &clockParams);
    timerElem->periodicObj.clkHandle = 
                           Clock_handle(&timerElem->periodicObj.clkStruct);
    timerElem->periodicObj.clkStarted = FALSE;
}

static Void utils_dmtimer_delete_prd_obj(Utils_dmTimerElem *timerElem)
{
    UTILS_assert(timerElem->periodicObj.clkHandle != NULL);

    /* Stop the clock */
    Clock_stop(timerElem->periodicObj.clkHandle);
    Clock_destruct(&(timerElem->periodicObj.clkStruct));
    timerElem->periodicObj.clkHandle = NULL;
    timerElem->periodicObj.clkStarted = FALSE;

}

static Void utils_dmtimer_start_prd_obj(Utils_dmTimerElem *timerElem,
                                        UInt period)
{
    UTILS_assert(timerElem->periodicObj.clkHandle != NULL);

    if (FALSE == timerElem->periodicObj.clkStarted)
    {
        Clock_setPeriod(timerElem->periodicObj.clkHandle, period);
        Clock_setTimeout(timerElem->periodicObj.clkHandle, period);
        Clock_start(timerElem->periodicObj.clkHandle);
        timerElem->periodicObj.clkStarted = TRUE;
    }

}

static Void utils_dmtimer_stop_prd_obj(Utils_dmTimerElem *timerElem)
{

    UTILS_assert(timerElem->periodicObj.clkHandle != NULL);

    if (TRUE == timerElem->periodicObj.clkStarted)
    {
        Clock_stop(timerElem->periodicObj.clkHandle);
        timerElem->periodicObj.clkStarted = FALSE;
    }
}


static
Void utils_dmtimer_init(Utils_dmTimerObj *pObj)
{
    if (FALSE == pObj->initDone)
    {
        Int i;
        
        UTILS_COMPILETIME_ASSERT(
                             UTILS_ARRAYSIZE(Utils_dmTimerBaseAddr)
                             ==
                             UTILS_ARRAYSIZE(pObj->timer));

        memset(pObj->timer,0,sizeof(pObj->timer));
        for (i = 0; i < UTILS_ARRAYSIZE(pObj->timer); i++)
        {
            pObj->timer[i].timerId = i;
            pObj->timer[i].timerRegs =
                (Utils_dmTimerRegs *)Utils_dmTimerBaseAddr[i];
            utils_dmtimer_create_prd_obj(&pObj->timer[i]);
        }
        pObj->initDone = TRUE;
    }
}

static
UInt32 utils_dmtimer_getcnt32(Utils_dmTimerElem *timerElem)
{
    UInt32 curTime =
        UTILS_DMTIMER_READTIMERREG(timerElem->timerRegs);

    if (curTime <  timerElem->lastTimerCounter)
    {
        timerElem->rolloverCount++;
    }
    timerElem->lastTimerCounter = curTime;
    return (timerElem->lastTimerCounter);
}

static
UInt64 utils_dmtimer_getcnt64(Utils_dmTimerElem *timerElem)
{
    UInt64 curTs64;

    timerElem->lastTimerCounter =
        UTILS_DMTIMER_READTIMERREG(timerElem->timerRegs);
    curTs64 = timerElem->rolloverCount;
    curTs64 <<= 32;
    curTs64 |= timerElem->lastTimerCounter;
    return (curTs64);
}


static
Void  utils_dmtimer_setfreq(Utils_dmTimerElem *timerElem)
{
    Types_FreqHz cpuHz;
    UInt32 startTimeTs,startTimeDmTimer;
    UInt32 endTimeTs,endTimeDmTimer;
    UInt32 elapsedTimeTs,elapsedTimeDmTimer;
    float elapsedRatio;
    UInt64 tsTimerFreq;
    UInt64 dmTimerFreq;

#ifndef UTILS_DMTIMER_FREQUENCY_AUTODETECT
    UTILS_assert(timerElem->timerId <
                 UTILS_ARRAYSIZE(Utils_dmTimerFrequency));
    timerElem->freq = Utils_dmTimerFrequency[timerElem->timerId];
#else
    timerElem->freq = UTILS_DMTIMER_FREQ_INVALID;
#endif

    if (UTILS_DMTIMER_FREQ_INVALID == timerElem->freq)
    {
        Timestamp_getFreq(&cpuHz);

        startTimeDmTimer = utils_dmtimer_getcnt32(timerElem);
        startTimeTs = Timestamp_get32();

        ti_sysbios_timers_dmtimer_Timer_spinLoop__I(UTILS_DMTIMER_FREQ_CALIBRATION_LOOP_LENGTH);

        endTimeDmTimer = utils_dmtimer_getcnt32(timerElem);
        endTimeTs = Timestamp_get32();

        elapsedTimeDmTimer = UTILS_DMTIMER_GETTSDELTA(startTimeDmTimer,endTimeDmTimer);
        elapsedTimeTs      = UTILS_DMTIMER_GETTSDELTA(startTimeTs,endTimeTs);

        elapsedRatio = (float)elapsedTimeDmTimer/elapsedTimeTs;
        tsTimerFreq =    cpuHz.hi;
        tsTimerFreq <<=  32;
        tsTimerFreq |=   cpuHz.lo;
        dmTimerFreq = elapsedRatio * tsTimerFreq;
        timerElem->freq = dmTimerFreq;
        timerElem->rolloverCount = 0;
    }
    utils_dmtimer_start_prd_obj(timerElem,
                                UTILS_DMTIMER_OVERFLOW_CHECK_PERIOD_MS);
    Vps_rprintf(" \n");
    Vps_rprintf(" *** UTILS: DM_TIMER[%d] KHz = %d Khz ***\n", timerElem->timerId,
               ((timerElem->freq & ((UInt32)~0u))/1000));
    Vps_rprintf(" \n");
}

/*
 *  ======== Utils_dmTimerGetCount ========
 */
UInt32 Utils_dmTimerGetCount(UInt32 dmTimerId)
{
    UInt32 timer_val = 0;
    Utils_dmTimerObj *pObj = &gUtils_dmTimerObj;

    if (dmTimerId < UTILS_ARRAYSIZE(Utils_dmTimerBaseAddr))
    {
        utils_dmtimer_init(pObj);
        timer_val = utils_dmtimer_getcnt32(&pObj->timer[dmTimerId]);
    }
    return timer_val;
}

/*
 *  ======== Timer_getFreq ========
 */
Void Utils_dmTimerGetFreq(UInt32 dmTimerId, Types_FreqHz *freq)
{
    Utils_dmTimerObj *pObj = &gUtils_dmTimerObj;

    if (dmTimerId < UTILS_ARRAYSIZE(pObj->timer))
    {
        utils_dmtimer_init(pObj);
        if (pObj->timer[dmTimerId].freqSet == FALSE)
        {
            utils_dmtimer_setfreq(&pObj->timer[dmTimerId]);
            pObj->timer[dmTimerId].freqSet = TRUE;
        }
        freq->lo = pObj->timer[dmTimerId].freq & ((UInt32)~0u);
        freq->hi = pObj->timer[dmTimerId].freq >> 32;
    }
}


UInt32 Utils_dmTimerGetCurTimeInMsec(UInt32 dmTimerId)
{
    UInt64 curTs = 0;
    Utils_dmTimerObj *pObj = &gUtils_dmTimerObj;

    if (dmTimerId < UTILS_ARRAYSIZE(pObj->timer))
    {
        Utils_dmTimerElem *timer = &pObj->timer[dmTimerId];
        utils_dmtimer_init(pObj);
        if (timer->freqSet == FALSE)
        {
            utils_dmtimer_setfreq(timer);
            timer->freqSet = TRUE;
        }
        curTs = utils_dmtimer_getcnt64(timer);
        return (UInt32)(UTILS_DMTIMER_TICKS2MS(curTs,timer->freq));
    }
    else
    {
        return 0;
    }
}

Void Utils_dmTimerInit()
{
    Utils_dmTimerObj *pObj = &gUtils_dmTimerObj;
    utils_dmtimer_init(pObj);
}


Void Utils_dmTimerDeInit()
{
    Utils_dmTimerObj *pObj = &gUtils_dmTimerObj;

    if (pObj->initDone)
    {
        Int i;

        for (i = 0; i < UTILS_ARRAYSIZE(pObj->timer);i++)
        {
            Utils_dmTimerElem *timer = &pObj->timer[i];

            if (timer->periodicObj.clkStarted)
            {
                 utils_dmtimer_stop_prd_obj(timer);
            }
            if (timer->periodicObj.clkHandle)
            {
                 utils_dmtimer_delete_prd_obj(timer);
            }
        }
    }
}




