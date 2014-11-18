/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include <mcfw/src_bios6/utils/utils_prf.h>

#define UTILS_FLOAT2INT_ROUND(x) ((x)>=0?(long)((x)+0.5):(long)((x)-0.5))

typedef struct {
    Bool isAlloc;
    char name[32];
    Task_Handle pTsk;
    UInt64 totalTskThreadTime;

} Utils_PrfLoadObj;

typedef struct {
    Utils_PrfTsHndl tsObj[UTILS_PRF_MAX_HNDL];
    Utils_PrfLoadObj loadObj[UTILS_PRF_MAX_HNDL];

} Utils_PrfObj;

typedef struct {
    UInt64 totalSwiThreadTime;
    UInt64 totalHwiThreadTime;
    UInt64 totalTime;
    UInt64 totalIdlTskTime;
} Utils_AccPrfLoadObj;

Utils_PrfObj gUtils_prfObj;
Utils_AccPrfLoadObj gUtils_accPrfLoadObj;
UInt32 gUtils_startLoadCalc = 0;

Int32 Utils_prfInit()
{
    memset(&gUtils_prfObj, 0, sizeof(gUtils_prfObj));
    memset(&gUtils_accPrfLoadObj, 0, sizeof(Utils_AccPrfLoadObj));

    return 0;
}

Int32 Utils_prfDeInit()
{

    return 0;
}

Utils_PrfTsHndl *Utils_prfTsCreate(char *name)
{
    UInt32 hndlId;
    Utils_PrfTsHndl *pHndl = NULL;

    UInt32 cookie;

    cookie = Hwi_disable();

    for (hndlId = 0; hndlId < UTILS_PRF_MAX_HNDL; hndlId++)
    {
        pHndl = &gUtils_prfObj.tsObj[hndlId];

        if (pHndl->isAlloc == FALSE)
        {
            strncpy(pHndl->name, name, sizeof(pHndl->name));

            pHndl->isAlloc = TRUE;

            Utils_prfTsReset(pHndl);
            break;
        }
    }

    Hwi_restore(cookie);

    return pHndl;
}

Int32 Utils_prfTsDelete(Utils_PrfTsHndl * pHndl)
{
    pHndl->isAlloc = FALSE;
    return 0;
}

UInt64 Utils_prfTsBegin(Utils_PrfTsHndl * pHndl)
{
    /* Currently thi is not used as 64bit timestamp is not working TODO */
    pHndl->startTs = Utils_prfTsGet64();

    return pHndl->startTs;
}

UInt64 Utils_prfTsEnd(Utils_PrfTsHndl * pHndl, UInt32 numFrames)
{
    return Utils_prfTsDelta(pHndl, pHndl->startTs, numFrames);
}

UInt64 Utils_prfTsDelta(Utils_PrfTsHndl * pHndl, UInt64 startTime,
                        UInt32 numFrames)
{
    UInt64 endTs;
    UInt32 cookie;

    /* Currently thi is not used as 64bit timestamp is not working TODO */
    endTs = Utils_prfTsGet64();

    cookie = Hwi_disable();

    pHndl->totalTs += (endTs - pHndl->startTs);
    pHndl->count++;
    pHndl->numFrames += numFrames;

    Hwi_restore(cookie);

    return endTs;
}

Int32 Utils_prfTsReset(Utils_PrfTsHndl * pHndl)
{
    UInt32 cookie;

    cookie = Hwi_disable();

    pHndl->totalTs = 0;
    pHndl->count = 0;
    pHndl->numFrames = 0;

    Hwi_restore(cookie);

    return 0;
}

UInt64 Utils_prfTsGet64()
{
    Types_Timestamp64 ts64;
    UInt64 curTs;

    Timestamp_get64(&ts64);

    curTs = ((UInt64) ts64.hi << 32) | ts64.lo;

    return curTs;
}

Int32 Utils_prfTsPrint(Utils_PrfTsHndl * pHndl, Bool resetAfterPrint)
{
    UInt32 timeMs, fps, fpc;
    Types_FreqHz cpuHz;

    /* This is not used as 64 bit timestamp is not working TODO */
    UInt32 cpuKhz;

    Timestamp_getFreq(&cpuHz);

    /* Currently thi is not used as 64bit timestamp is not working TODO */
    cpuKhz = cpuHz.lo / 1000;                              /* convert to Khz */

    /* Currently thi is not used as 64bit timestamp is not working TODO */
    timeMs = (pHndl->totalTs) / cpuKhz;

    fps = (pHndl->numFrames * 1000) / timeMs;
    fpc = (pHndl->numFrames) / pHndl->count;

    Vps_printf(" %d: PRF : %s : t: %d ms, c: %d, f: %d, fps: %d, fpc: %d \r\n", Utils_getCurTimeInMsec(), pHndl->name, timeMs,  /* in
                                                                                                                         * msecs
                                                                                                                         */
               pHndl->count, pHndl->numFrames, fps,        /* frames per
                                                            * second */
               fpc                                         /* frames per
                                                            * count */
        );

    if (resetAfterPrint)
        Utils_prfTsReset(pHndl);

    return 0;
}

Int32 Utils_prfTsPrintAll(Bool resetAfterPrint)
{
    UInt32 hndlId;
    Utils_PrfTsHndl *pHndl;

    Vps_printf("\r\n");

    for (hndlId = 0; hndlId < UTILS_PRF_MAX_HNDL; hndlId++)
    {
        pHndl = &gUtils_prfObj.tsObj[hndlId];

        if (pHndl->isAlloc)
        {
            Utils_prfTsPrint(pHndl, resetAfterPrint);
        }
    }

    Vps_printf("\r\n");

    return 0;
}

Int32 Utils_prfLoadRegister(Task_Handle pTsk, char *name)
{
    UInt32 hndlId;
    UInt32 cookie;
    Int32 status = FVID2_EFAIL;
    Utils_PrfLoadObj *pHndl;

    cookie = Hwi_disable();

    for (hndlId = 0; hndlId < UTILS_PRF_MAX_HNDL; hndlId++)
    {
        pHndl = &gUtils_prfObj.loadObj[hndlId];

        if (pHndl->isAlloc == FALSE)
        {
            pHndl->isAlloc = TRUE;
            pHndl->pTsk = pTsk;
            strncpy(pHndl->name, name, sizeof(pHndl->name));
            status = FVID2_SOK;
            break;
        }
    }

    Hwi_restore(cookie);

    return status;
}

char * Utils_prfLoadGetTaskName(Task_Handle pTsk)
{
    UInt32 hndlId;
    UInt32 cookie;
    Utils_PrfLoadObj *pHndl = NULL;
    char *tskName = NULL;

    cookie = Hwi_disable();

    for (hndlId = 0; hndlId < UTILS_PRF_MAX_HNDL; hndlId++)
    {
        pHndl = &gUtils_prfObj.loadObj[hndlId];

        if (pHndl->isAlloc && pHndl->pTsk == pTsk)
        {
            break;
        }
    }

    if (hndlId < UTILS_PRF_MAX_HNDL)
    {
        UTILS_assert(UTILS_ARRAYISVALIDENTRY(pHndl,gUtils_prfObj.loadObj));
        tskName = pHndl->name;
    }
    Hwi_restore(cookie);

    return tskName;
}

Int32 Utils_prfLoadUnRegister(Task_Handle pTsk)
{
    UInt32 hndlId;
    UInt32 cookie;
    Int32 status = FVID2_EFAIL;
    Utils_PrfLoadObj *pHndl;

    cookie = Hwi_disable();

    for (hndlId = 0; hndlId < UTILS_PRF_MAX_HNDL; hndlId++)
    {
        pHndl = &gUtils_prfObj.loadObj[hndlId];

        if (pHndl->isAlloc && pHndl->pTsk == pTsk)
        {
            pHndl->isAlloc = FALSE;
            status = FVID2_SOK;
            break;
        }
    }

    Hwi_restore(cookie);

    return status;
}

Int32 Utils_prfLoadPrintAll(Bool printTskLoad)
{
    Int32 hwiLoad, swiLoad, tskLoad, hndlId, cpuLoad, totalLoad, otherLoad;
    Utils_PrfLoadObj *pHndl;

    hwiLoad = swiLoad = tskLoad = -1;

    hwiLoad = (gUtils_accPrfLoadObj.totalHwiThreadTime * 1000) /
        gUtils_accPrfLoadObj.totalTime;
    swiLoad = (gUtils_accPrfLoadObj.totalSwiThreadTime * 1000) /
        gUtils_accPrfLoadObj.totalTime;
    cpuLoad = 1000 - ((gUtils_accPrfLoadObj.totalIdlTskTime * 1000) /
                     gUtils_accPrfLoadObj.totalTime);

    totalLoad = hwiLoad+swiLoad;

    Vps_printf(" \n");
    Vps_printf(" %d: LOAD: CPU: %d.%d%% HWI: %d.%d%%, SWI:%d.%d%% \n",
                Utils_getCurTimeInMsec(),
                cpuLoad/10, cpuLoad%10,
                hwiLoad/10, hwiLoad%10,
                swiLoad/10, swiLoad%10
                );

    if (printTskLoad)
    {
        Vps_printf(" \n");

        for (hndlId = 0; hndlId < UTILS_PRF_MAX_HNDL; hndlId++)
        {
            pHndl = &gUtils_prfObj.loadObj[hndlId];

            if (pHndl->isAlloc)
            {
                tskLoad = -1;

                tskLoad = (pHndl->totalTskThreadTime * 1000) /
                    gUtils_accPrfLoadObj.totalTime;

                totalLoad += tskLoad;

                if (tskLoad)
                {
                    Vps_printf(" %d: LOAD: TSK: %-20s: %d.%d%% \r\n", Utils_getCurTimeInMsec(),
                           pHndl->name,
                           tskLoad/10,
                           tskLoad%10
                        );
                }
            }
        }

        otherLoad = cpuLoad - totalLoad;

        Vps_printf(" %d: LOAD: TSK: %-20s: %d.%d%% \r\n", Utils_getCurTimeInMsec(),
               "MISC",
               otherLoad/10,
               otherLoad%10
            );
    }
    Vps_printf(" \n");

    return 0;
}

Void Utils_prfLoadCalcStart()
{
    UInt32 cookie;

    cookie = Hwi_disable();
    gUtils_startLoadCalc = TRUE;
    Hwi_restore(cookie);
}

Void Utils_prfLoadCalcStop()
{
    UInt32 cookie;

    cookie = Hwi_disable();
    gUtils_startLoadCalc = FALSE;
    Hwi_restore(cookie);
}

Void Utils_prfLoadCalcReset()
{
    Utils_PrfLoadObj *pHndl;
    UInt32 hndlId;

    gUtils_accPrfLoadObj.totalHwiThreadTime = 0;
    gUtils_accPrfLoadObj.totalSwiThreadTime = 0;
    gUtils_accPrfLoadObj.totalTime = 0;
    gUtils_accPrfLoadObj.totalIdlTskTime = 0;
    /* Reset the performace loads accumulator */
    for (hndlId = 0; hndlId < UTILS_PRF_MAX_HNDL; hndlId++)
    {
        pHndl = &gUtils_prfObj.loadObj[hndlId];

        if (pHndl->isAlloc && pHndl->pTsk != NULL)
        {
            pHndl->totalTskThreadTime = 0;

        }
    }
}

Int32 Utils_prfTsTest(UInt32 count, UInt32 delayInMs)
{
    Utils_PrfTsHndl *pPrf;

    pPrf = Utils_prfTsCreate("TEST");

    while (count--)
    {
        Utils_prfTsBegin(pPrf);
        Task_sleep(delayInMs);
        Utils_prfTsEnd(pPrf, 1);

        Utils_prfTsPrintAll(FALSE);
    }

    Utils_prfTsDelete(pPrf);

    return 0;
}

/* Function called by Loadupdate for each update cycle */
Void Utils_prfLoadUpdate()
{
    Utils_PrfLoadObj *pHndl;
    Load_Stat hwiLoadStat, swiLoadStat, tskLoadStat, idlTskLoadStat;
    Task_Handle idlTskHndl = NULL;
    UInt32 hndlId;

    if (TRUE == gUtils_startLoadCalc)
    {
        idlTskHndl = Task_getIdleTask();
        /* Get the all loads first */
        Load_getGlobalHwiLoad(&hwiLoadStat);
        Load_getGlobalSwiLoad(&swiLoadStat);
        Load_getTaskLoad(idlTskHndl, &idlTskLoadStat);

        gUtils_accPrfLoadObj.totalHwiThreadTime += hwiLoadStat.threadTime;
        gUtils_accPrfLoadObj.totalSwiThreadTime += swiLoadStat.threadTime;
        gUtils_accPrfLoadObj.totalTime += hwiLoadStat.totalTime;
        gUtils_accPrfLoadObj.totalIdlTskTime += idlTskLoadStat.threadTime;

        /* Call the load updated function of each registered task one by one
         * along with the swiLoad, hwiLoad, and Task's own load */
        for (hndlId = 0; hndlId < UTILS_PRF_MAX_HNDL; hndlId++)
        {
            pHndl = &gUtils_prfObj.loadObj[hndlId];

            if (pHndl->isAlloc && pHndl->pTsk != NULL)
            {
                Load_getTaskLoad(pHndl->pTsk, &tskLoadStat);
                pHndl->totalTskThreadTime += tskLoadStat.threadTime;

            }
        }
    }
}

Int32 Utils_prfGetTaskName(Task_Handle pTsk, char *name)
{
    UInt32 hndlId;
    UInt32 cookie;
    Int32 status = FVID2_EFAIL;
    Utils_PrfLoadObj *pHndl;

    cookie = Hwi_disable();

    for (hndlId = 0; hndlId < UTILS_PRF_MAX_HNDL; hndlId++)
    {
        pHndl = &gUtils_prfObj.loadObj[hndlId];

        if (pHndl->isAlloc == TRUE)
        {
            if (pHndl->pTsk == pTsk)
            {
                strncpy(name, pHndl->name, sizeof(pHndl->name));
                status = FVID2_SOK;
                break;
            }
        }
    }

    Hwi_restore(cookie);

    return status;
}


#include <xdc/std.h>
#include <xdc/runtime/Timestamp.h>
#include <xdc/runtime/Types.h>
#include <ti/sysbios/knl/Task.h>
// #include <ti/sysbios/family/arm/m3/Hwi.h>

#define UTILS_INTLATENCY_LATE_IRP_COUNT                                     (32)
#define UTILS_FREQPERMICROSEC_DIV_FACTOR                       (1 * 1000 * 1000)

typedef struct Utils_IntLatencyMeasure {
    Bool start;
    UInt32 expectedInterruptInterval;
    UInt32 maxAllowedLatency;
    UInt32 prevIntTime;
    UInt32 numLateInts;
    UInt32 lateIntIrp[UTILS_INTLATENCY_LATE_IRP_COUNT];
    UInt32 timerFreqPerMicroSec;
    Hwi_Handle hHwi;
} Utils_IntLatencyMeasure;

Void Utils_IntLatencyCalculate(Utils_IntLatencyMeasure * latencyMeasure,
                               UInt intId)
{
    if (latencyMeasure->start)
    {
        UInt32 curTime = Timestamp_get32();
        UInt32 tsDelta;

        if ((latencyMeasure->prevIntTime != 0)
            && (latencyMeasure->prevIntTime < curTime))
        {
            tsDelta = (curTime - latencyMeasure->prevIntTime) /
                latencyMeasure->timerFreqPerMicroSec;
            if (tsDelta > (latencyMeasure->expectedInterruptInterval +
                           latencyMeasure->maxAllowedLatency))
            {
                UInt32 lateIntIdx =
                    latencyMeasure->numLateInts %
                    UTILS_INTLATENCY_LATE_IRP_COUNT;

                latencyMeasure->lateIntIrp[lateIntIdx] = (UInt32) Task_self();
                latencyMeasure->numLateInts++;
            }
        }
        else
        {
            if (latencyMeasure->prevIntTime == 0)
            {
                Types_FreqHz freq;
                Bits64 freqInMicrosec;

                Timestamp_getFreq(&freq);
                freqInMicrosec = freq.hi;
                freqInMicrosec <<= 32;
                freqInMicrosec |= freq.lo;
                freqInMicrosec /= UTILS_FREQPERMICROSEC_DIV_FACTOR;
                latencyMeasure->timerFreqPerMicroSec = (UInt32) freqInMicrosec;
                latencyMeasure->numLateInts = 0;
                // latencyMeasure->hHwi = Hwi_getHandle(intId);
            }

        }
        latencyMeasure->prevIntTime = curTime;
    }
}

Utils_IntLatencyMeasure g_ClockLatency = {
    .expectedInterruptInterval = 1000,
    .maxAllowedLatency = 1000,
    .prevIntTime = 0,
    .numLateInts = 0,
    .start = FALSE,
};

Void Clock_IntLatencyCalculate()
{
    Utils_IntLatencyCalculate(&g_ClockLatency, 0xf);
}
