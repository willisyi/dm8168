/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#ifndef _UTILS_ENCDEC_PRF_H_
#define _UTILS_ENCDEC_PRF_H_

#include <string.h>
#include <xdc/std.h>
#include <xdc/runtime/Timestamp.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/psp/vps/vps.h>
#include <mcfw/src_bios6/utils/utils.h>

#define UTILS_ENCDEC_HDVICP_PROFILE

#define UTILS_ENCDEC_MAXNUMOFHDVICP2_RESOUCES                               (3)

typedef struct HDVICP_logTbl {
    UInt32 totalAcquire2wait;
    UInt32 totalWait2Isr;
    UInt32 totalIsr2Done;
    UInt32 totalWait2Done;
    UInt32 totalDone2Release;
    UInt32 totalAcquire2Release;
    UInt32 totalAcq2acqDelay;
    UInt32 tempPrevTime;
    UInt32 tempAcquireTime;
    UInt32 tempWaitTime;
    UInt32 startTime;
    UInt32 endTime;
    UInt32 numAccessCnt;
    UInt32 currentBatchSize;
    UInt32 totalIVAHDActivateTime;
} HDVICP_logTbl;

typedef struct HDVICP_tskEnv
{
    UInt32 size;
    UInt32 ivaID;
    Int32 batchSize;
} HDVICP_tskEnv;

extern HDVICP_logTbl g_HDVICP_logTbl[UTILS_ENCDEC_MAXNUMOFHDVICP2_RESOUCES];


static inline UInt32 Utils_encdecGetTime(Void)
{
    return (Utils_getCurTimeInMsec());
}

static Void Utils_encdecHdvicpPrfInit(Void)
{
    memset(&g_HDVICP_logTbl, 0, sizeof(g_HDVICP_logTbl));
    return;
}

static Void Utils_encdecHdvicpPrfPrint(void)
{
    int ivaId;
    UInt32 totalElapsedTime, perCentTotalWait2Isr;

    
#ifdef UTILS_ENCDEC_HDVICP_PROFILE
    for (ivaId = 0; ivaId < UTILS_ENCDEC_MAXNUMOFHDVICP2_RESOUCES; ivaId++)
    {

        Vps_printf("\n\t%d: HDVICP-ID:%d\n", Utils_encdecGetTime(), ivaId);

        totalElapsedTime = (g_HDVICP_logTbl[ivaId].endTime -
                            g_HDVICP_logTbl[ivaId].startTime);
	perCentTotalWait2Isr = (g_HDVICP_logTbl[ivaId].totalWait2Isr * 100) / 
                               totalElapsedTime;
        Vps_printf ("All percentage figures are based off totalElapsedTime\n");
        Vps_printf("\n\t\t totalAcquire2wait :%d %%"
                   "\n\t\t totalWait2Isr :%d %%"
                   "\n\t\t totalIsr2Done :%d %%"
                   "\n\t\t totalWait2Done :%d %%"
                   "\n\t\t totalDone2Release :%d %%"
                   "\n\t\t totalAcquire2Release :%d %%"
                   "\n\t\t totalAcq2acqDelay :%d %%"
                   "\n\t\t totalElapsedTime in msec :%8d"
                   "\n\t\t numAccessCnt:%8d\n",
                   (g_HDVICP_logTbl[ivaId].totalAcquire2wait * 100) / totalElapsedTime ,
                   (g_HDVICP_logTbl[ivaId].totalWait2Isr * 100) / totalElapsedTime,
                   (g_HDVICP_logTbl[ivaId].totalIsr2Done * 100) / totalElapsedTime,
                   (g_HDVICP_logTbl[ivaId].totalWait2Done * 100) / totalElapsedTime,
                   (g_HDVICP_logTbl[ivaId].totalDone2Release * 100) / totalElapsedTime,
                   (g_HDVICP_logTbl[ivaId].totalAcquire2Release * 100 ) / totalElapsedTime,
                   (g_HDVICP_logTbl[ivaId].totalAcq2acqDelay * 100) / totalElapsedTime,
                   totalElapsedTime, g_HDVICP_logTbl[ivaId].numAccessCnt
                   );

        Vps_printf("\n\t\tIVA-FPS :%8d\n",
                   ((g_HDVICP_logTbl[ivaId].numAccessCnt) /
                    (totalElapsedTime/1000)));
        Vps_printf("\n\t\tAverage time spent per frame in microsec:%8d\n",
                   ((totalElapsedTime * 1000/ g_HDVICP_logTbl[ivaId].numAccessCnt) * perCentTotalWait2Isr)/ 100);
    }
#endif
    return;
}

#endif
