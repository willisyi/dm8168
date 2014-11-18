/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2010 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include "remote_debug_if.h"
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/hal/Hwi.h>
#include <string.h>
#include <stdio.h>
#include <xdc/std.h>
#include <xdc/runtime/System.h>

#define REMOTE_DEBUG_SERVER_CURRENT_CORE         REMOTE_DEBUG_CORE_ID_C6XDSP

#define REMOTE_DEBUG_SERVER_PRINT_BUF_LEN        (1024*2)

#define STATIC static

typedef struct {

    unsigned int coreId;

    char printBuf[REMOTE_DEBUG_SERVER_PRINT_BUF_LEN];

} RemoteDebug_ServerObj;

#pragma DATA_SECTION(gRemoteDebug_coreObj,".bss:extMemNonCache:remoteDebugCoreShm");
#pragma DATA_ALIGN(gRemoteDebug_coreObj, 4*1024);
STATIC RemoteDebug_CoreObj gRemoteDebug_coreObj[REMOTE_DEBUG_CORE_ID_MAX];

STATIC RemoteDebug_ServerObj gRemoteDebug_serverObj =
    { REMOTE_DEBUG_SERVER_CURRENT_CORE };

STATIC int RemoteDebug_serverPutString(unsigned int coreId, char *pString)
{
    unsigned int maxBytes, numBytes, copyBytes, serverIdx, clientIdx;
    volatile unsigned char *pDst;

    RemoteDebug_CoreObj *pCoreObj;

    if (coreId >= REMOTE_DEBUG_CORE_ID_MAX)
        return -1;

    pCoreObj = &gRemoteDebug_coreObj[coreId];

    if (pCoreObj->headerTag != REMOTE_DEBUG_HEADER_TAG)
        return -1;

    numBytes = strlen(pString);

    if (numBytes <= 0)
        return -1;

    serverIdx = pCoreObj->serverIdx;
    clientIdx = pCoreObj->clientIdx;

    if (serverIdx < clientIdx)
        maxBytes = clientIdx - serverIdx;
    else
        maxBytes = (REMOTE_DEBUG_LOG_BUF_SIZE - serverIdx) + clientIdx;

    if (numBytes > maxBytes)
        return -1;

    pDst = &pCoreObj->serverLogBuf[0];

    for (copyBytes = 0; copyBytes < numBytes; copyBytes++)
    {
        if (serverIdx >= REMOTE_DEBUG_LOG_BUF_SIZE)
            serverIdx = 0;

        pDst[serverIdx] = *pString++;
        serverIdx++;
    }

    pDst[serverIdx] = 0;
    serverIdx++;

    pCoreObj->serverIdx = serverIdx;

    return 0;
}

int Vps_printf(char *format, ...)
{
    int retVal;
    va_list vaArgPtr;
    char *buf = NULL;
    UInt32 cookie;

    cookie = Hwi_disable();

    buf = &gRemoteDebug_serverObj.printBuf[0];

    va_start(vaArgPtr, format);
    vsnprintf(buf, REMOTE_DEBUG_SERVER_PRINT_BUF_LEN, format, vaArgPtr);
    va_end(vaArgPtr);

    retVal = RemoteDebug_serverPutString(gRemoteDebug_serverObj.coreId, buf);

    Hwi_restore(cookie);

    if (BIOS_getThreadType() == BIOS_ThreadType_Task)
    {
        /* Printf should be called only from Task context as it does pend.
         * Calling from other context will cause exception
         */
        System_printf(buf);
    }

    return (retVal);
}

int Vps_rprintf(char *format, ...)
{
    int retVal;
    va_list vaArgPtr;
    char *buf = NULL;
    UInt32 cookie;

    cookie = Hwi_disable();

    buf = &gRemoteDebug_serverObj.printBuf[0];

    va_start(vaArgPtr, format);
    vsnprintf(buf, REMOTE_DEBUG_SERVER_PRINT_BUF_LEN, format, vaArgPtr);
    va_end(vaArgPtr);

    retVal = RemoteDebug_serverPutString(gRemoteDebug_serverObj.coreId, buf);

    Hwi_restore(cookie);

    return (retVal);
}

int RemoteDebug_putChar(char ch)
{
    volatile RemoteDebug_CoreObj *pCoreObj;
    volatile int coreId;

    coreId = gRemoteDebug_serverObj.coreId;

    if (coreId >= REMOTE_DEBUG_CORE_ID_MAX)
        return -1;

    pCoreObj = &gRemoteDebug_coreObj[coreId];

    if (pCoreObj->headerTag == REMOTE_DEBUG_HEADER_TAG)
    {
        pCoreObj->clientFlags[0] = (REMOTE_DEBUG_FLAG_TYPE_CHAR | ch);
    }

    return 0;
}

int RemoteDebug_getChar(char *pChar, UInt32 timeout)
{
    volatile RemoteDebug_CoreObj *pCoreObj;
    volatile int coreId, value;

    *pChar = 0;

    coreId = gRemoteDebug_serverObj.coreId;

    if (coreId >= REMOTE_DEBUG_CORE_ID_MAX)
        return -1;

    pCoreObj = &gRemoteDebug_coreObj[coreId];

    while (1)
    {
        if (pCoreObj->headerTag != REMOTE_DEBUG_HEADER_TAG)
            return -1;

        value = pCoreObj->serverFlags[0];
        if (value & REMOTE_DEBUG_FLAG_TYPE_CHAR)
        {
            *pChar = value & 0xFF;
            pCoreObj->serverFlags[0] = 0;
            break;
        }
        if (timeout == BIOS_WAIT_FOREVER)
            Task_sleep(10);
        else
            return -1;
    }

    return 0;
}

int RemoteDebug_getString(char *pChar, UInt32 timeout)
{

    volatile RemoteDebug_CoreObj *pCoreObj;
    volatile int coreId, value;
    volatile int length;

    *pChar = 0;

    coreId = gRemoteDebug_serverObj.coreId;

    if (coreId >= REMOTE_DEBUG_CORE_ID_MAX)
        return -1;

    pCoreObj = &gRemoteDebug_coreObj[coreId];

    while (1)
    {
        if (pCoreObj->headerTag != REMOTE_DEBUG_HEADER_TAG)
            return -1;

        value = pCoreObj->serverFlags[0];

        if (value & REMOTE_DEBUG_FLAG_TYPE_STRING)
        {

            length = pCoreObj->serverFlags[1];

            if (length > 0 && length < REMOTE_DEBUG_PARAM_BUF_SIZE)
                strncpy(pChar, (char *) pCoreObj->serverParamBuf, length + 1);

            pCoreObj->serverFlags[1] = 0;
            pCoreObj->serverFlags[0] = 0;
            break;

        }
        if (timeout == BIOS_WAIT_FOREVER)
            Task_sleep(10);
        else
            return -1;
    }

    return 0;
}
