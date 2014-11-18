/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
    \ingroup UTILS_API
    \defgroup UTILS_PRF_API Profiling API
    @{
*/

/**
 * - APIs to measure and print elasped time @ 64-bit precision
 *    - Utils_prfTsXxxx
 *
 * - APIs to measure and print CPU load at task, HWI, SWI, global level
 *    - Utils_prfLoadXxxx
 */

#ifndef _UTILS_PRF_H_
#define _UTILS_PRF_H_

#include <mcfw/src_bios6/utils/utils.h>

#define UTILS_PRF_MAX_HNDL     (64)

typedef struct {
    Int32 cpuLoad;
    Int32 hwiLoad;
    Int32 swiLoad;
    Int32 tskLoad;
} Utils_PrfLoad;

/* Typedef for the loadupdate function for the user */
typedef Void(*Utils_loadUpdate) (Utils_PrfLoad *);

typedef struct {

    char name[32];
    Bool isAlloc;
    UInt64 startTs;
    UInt64 totalTs;
    UInt32 count;
    UInt32 numFrames;
} Utils_PrfTsHndl;

Void Utils_prfLoadUpdate();

/* Call this function before using any peformance or Timestamp utils */
Int32 Utils_prfInit();

/* de-Init of the performance and timestamp utils */
Int32 Utils_prfDeInit();

/* Create the handle for the time stamp taking */
Utils_PrfTsHndl *Utils_prfTsCreate(char *name);

/* Delete the handle for the timestamp */
Int32 Utils_prfTsDelete(Utils_PrfTsHndl * pHndl);

/* Start taking the timestamp */
UInt64 Utils_prfTsBegin(Utils_PrfTsHndl * pHndl);

/* Stop taking the timestamp */
UInt64 Utils_prfTsEnd(Utils_PrfTsHndl * pHndl, UInt32 numFrames);

/* Difference between the timestamp */
UInt64 Utils_prfTsDelta(Utils_PrfTsHndl * pHndl, UInt64 startTime,
                        UInt32 numFrames);

/* Reset the timestamp counter for that handle */
Int32 Utils_prfTsReset(Utils_PrfTsHndl * pHndl);

/* Get the 64-bit timer ticks */
UInt64 Utils_prfTsGet64();

/* Print the timestamp differnce and reset the counter thereafter or not */
Int32 Utils_prfTsPrint(Utils_PrfTsHndl * pHndl, Bool resetAfterPrint);

/* Print the timestamp for all the registered handles and reset the counter */
Int32 Utils_prfTsPrintAll(Bool resetAfterPrint);

/* Register a task for load calculation */
Int32 Utils_prfLoadRegister(Task_Handle pTsk, char *name);

/* Un-Register the task for load caculation */
Int32 Utils_prfLoadUnRegister(Task_Handle pTsk);

/* Print loads for all the registered tasks */
Int32 Utils_prfLoadPrintAll(Bool printTskLoad);

/* Gets the task name of a registered task */
Int32 Utils_prfGetTaskName(Task_Handle pTsk, char *name);

/* Start taking the performance load for all the registered tasks */
Void Utils_prfLoadCalcStart();

/* Stop taking the load for all the registered tasks */
Void Utils_prfLoadCalcStop();

/* Reset the load calculation mainly for next cycle of run */
Void Utils_prfLoadCalcReset();

/* Return the task name associated with a task handle */
char * Utils_prfLoadGetTaskName(Task_Handle pTsk);

#endif

/* @} */
