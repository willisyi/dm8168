/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#ifndef _UTILS_TRACE_H_
#define _UTILS_TRACE_H_

#include <xdc/std.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/psp/vps/vps.h>
#include <mcfw/src_bios6/utils/utils.h>

#ifdef  __TI_TMS470_V7M3__
Void Utils_assertHook(Char *file,Int lineNum,Char *assertInfo);
#else
/* Stub out assert Hook for DSP until it is implemented */
#define Utils_assertHook(file,lineNum,assertInfo)
#endif

#define UTILS_COMPILETIME_ASSERT(condition)                                     \
                   do {                                                         \
                       typedef char ErrorCheck[((condition) == TRUE) ? 1 : -1]; \
                   } while(0)

#define UTILS_ISERROR(errCode)                                  ((errCode) < 0)

#ifdef UTILS_ASSERT_ENABLE

/* Use C assert. */
#define UTILS_assert(y)                                                   \
do {                                                                      \
    extern volatile Int g_AssertFailLoop;                                 \
    if (!(y)) {                                                           \
        Vps_rprintf (" %d: Assertion @ Line: %d in %s: %s : failed !!!\n", \
                Utils_getCurTimeInMsec(), __LINE__, __FILE__, #y);         \
        Utils_assertHook(__FILE__,__LINE__,#y);                            \
        while(g_AssertFailLoop);                                          \
    }                                                                     \
} while (0)

#else

#define UTILS_assert(y)

#endif

#ifdef UTILS_ASSERT_ENABLE
#define UTILS_assertError(condition, statusVar, errorCode, linkID, channelID)     \
do {                                                                              \
    if (!(condition)) {                                                           \
        statusVar = errorCode;                                                    \
        Vps_rprintf(" \n%d:ERR::linkID:%x::channelID:%d::"                                            \
                     "errorCode:%d::FileName:%s::linuNum:%d::errorCondition:%s\n",                   \
                     Utils_getCurTimeInMsec(),linkID, channelID, statusVar, __FILE__, __LINE__,#condition);  \
    }                                                                                                \
} while(0)

#else

#define UTILS_assertError(condition, statusVar, errorCode, linkID, channelID)

#endif                                                     /* ifndef
                                                            * UTILS_ASSERT_ENABLE 
                                                            */

#ifdef UTILS_ASSERT_ENABLE
#define UTILS_warn(...)           do { Vps_rprintf("\n%d:WARN",Utils_getCurTimeInMsec());\
                                       Vps_rprintf(__VA_ARGS__);                 \
                                  } while(0)
#else
#define UTILS_warn(...)
#endif

static inline void Utils_printFVIDFrameList(FVID2_FrameList * frameList)
{
    Int i;

    Vps_rprintf("\nFVID2_FrameList:%p", frameList);
    Vps_rprintf("\nframeList->numFrames:%d", frameList->numFrames);

    for (i = 0; i < frameList->numFrames; i++)
    {
        Vps_rprintf("\nframeList->frame[%d].addr:%p", i,
                    frameList->frames[i]->addr[0][0]);
    }
}

static inline void Utils_printFVIDProcessList(FVID2_ProcessList * processList)
{
    Int i;

    Vps_rprintf("\nFVID2_ProcessList:%p", processList);
    Vps_rprintf("\nprocessList->numInLists:%d", processList->numInLists);

    for (i = 0; i < processList->numInLists; i++)
    {
        Vps_rprintf("\nprocessList->inFrameList[%d]:%p", i,
                    processList->inFrameList[i]);
        Utils_printFVIDFrameList(processList->inFrameList[i]);
    }

    Vps_rprintf("\nprocessList->numOutLists:%d", processList->numOutLists);
    for (i = 0; i < processList->numOutLists; i++)
    {
        Vps_rprintf("\nprocessList->outFrameList[%d]:%p", i,
                    processList->outFrameList[i]);
        Utils_printFVIDFrameList(processList->outFrameList[i]);
    }
}

#endif                                                     /* ifndef
                                                            * _UTILS_TRACE_H_ 
                                                            */
