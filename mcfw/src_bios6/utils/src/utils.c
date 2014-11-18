/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include <mcfw/src_bios6/utils/utils_buf.h>
#include <mcfw/src_bios6/links_common/system/system_priv_common.h>

UInt32 System_getSelfProcId();

volatile int g_AssertFailLoop = TRUE;

#define ENABLE_REMOTE_GET_CHAR

int RemoteDebug_getChar(char *pChar, UInt32 timeout);
int RemoteDebug_putChar(char ch);
int RemoteDebug_getString(char *pChar, UInt32 timeout);

int Utils_getChar(char *pChar, UInt32 timeout)
{
    int status = 0;

    if (timeout == BIOS_WAIT_FOREVER)
    {
#ifdef ENABLE_REMOTE_GET_CHAR
        status = RemoteDebug_getChar(pChar, timeout);
#else
        *pChar = getchar();
        fflush(stdin);
#endif

        return status;
    }

    status = RemoteDebug_getChar(pChar, BIOS_NO_WAIT);

    return status;
}

int Utils_getString(char *pChar, UInt32 timeout)
{
    int status = 0;

    if (timeout == BIOS_WAIT_FOREVER)
    {
#ifdef ENABLE_REMOTE_GET_CHAR
        status = RemoteDebug_getString(pChar, timeout);
#else
        gets(pChar);
        fflush(stdin);
#endif

        return status;
    }

    status = RemoteDebug_getString(pChar, BIOS_NO_WAIT);

    return status;
}

int Utils_remoteSendChar(char ch)
{
    int status = 0;

    status = RemoteDebug_putChar(ch);

    return status;
}

int Utils_setL3Pri_Pressure0(UInt32 initPressure0)
{
    volatile UInt32 *pAddr;

    pAddr = (volatile UInt32 *) 0x48140608;

    *pAddr = initPressure0;
    return 0;
}

int Utils_readL3Pri_Pressure0(void)
{
    volatile UInt32 *pAddr;

    pAddr = (volatile UInt32 *) 0x48140608;

    return *pAddr;
}


int Utils_setL3Pri_Pressure1(UInt32 initPressure1)
{
    volatile UInt32 *pAddr;

    pAddr = (volatile UInt32 *) 0x4814060C;

    *pAddr = initPressure1;
    return 0;
}

int Utils_readL3Pri_Pressure1(void)
{
    volatile UInt32 *pAddr;

    pAddr = (volatile UInt32 *) 0x4814060C;

    return *pAddr;
}

static char xtod(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    return c = 0;                                          // not Hex digit
}

static int HextoDec(char *hex, int l)
{
    if (*hex == 0)
        return (l);

    return HextoDec(hex + 1, l * 16 + xtod(*hex));         // hex+1?
}

int xstrtoi(char *hex)                                     // hex string to
                                                           // integer
{
    return HextoDec(hex, 0);
}

UInt32 Utils_setCpuFrequency (UInt32 freq)
{
    UInt cookie;
    Types_FreqHz cpuHz;
    Types_FreqHz OldCpuHz;

    BIOS_getCpuFreq(&OldCpuHz);

    cookie = Hwi_disable();
    cpuHz.lo = freq;
    cpuHz.hi = 0;
    ti_sysbios_BIOS_setCpuFreq(&cpuHz);
    Clock_tickStop();
    Clock_tickReconfig();
    Clock_tickStart();
    Hwi_restore(cookie);

    BIOS_getCpuFreq(&cpuHz);
    Vps_printf("***** SYSTEM  : Frequency <ORG> - %d, <NEW> - %d\n", OldCpuHz.lo, cpuHz.lo);
    return 0;
}

UInt32 Utils_getCurTimeInMsec()
{
    #if 1
    static UInt32 cpuKhz = 500*1000; // default
    static Bool isInit = FALSE;

    Types_Timestamp64 ts64;
    UInt64 curTs;

    if(!isInit)
    {
        /* do this only once */

        Types_FreqHz cpuHz;

        isInit = TRUE;

        Timestamp_getFreq(&cpuHz);

        cpuKhz = cpuHz.lo / 1000; /* convert to Khz */

        Vps_printf(" \n");
        Vps_printf(" *** UTILS: CPU KHz = %d Khz ***\n", cpuKhz);
        Vps_printf(" \n");

    }

    Timestamp_get64(&ts64);

    curTs = ((UInt64) ts64.hi << 32) | ts64.lo;

    return (UInt32)(curTs/(UInt64)cpuKhz);
    #else
    return Clock_getTicks();
    #endif
}

UInt64 Utils_getCurTimeInUsec()
{
    static UInt32 cpuMhz = 500; // default
    static Bool isInit = FALSE;

    Types_Timestamp64 ts64;
    UInt64 curTs;

    if(!isInit)
    {
        /* do this only once */

        Types_FreqHz cpuHz;

        isInit = TRUE;

        Timestamp_getFreq(&cpuHz);

        cpuMhz = cpuHz.lo / (1000*1000); /* convert to Mhz */

        Vps_printf(" \n");
        Vps_printf(" *** UTILS: CPU MHz = %d Mhz ***\n", cpuMhz);
        Vps_printf(" \n");


    }

    Timestamp_get64(&ts64);

    curTs = ((UInt64) ts64.hi << 32) | ts64.lo;

    return (curTs/cpuMhz);

}

UInt64 Utils_getTimeIn64bit(FVID2_Frame *pFrame)
{
    System_FrameInfo *pFrameInfo;

    UTILS_assert(pFrame->appData != NULL);
    pFrameInfo = (System_FrameInfo *) pFrame->appData;

    return pFrameInfo->ts64;
}

Bool Utils_doSkipFrame(Utils_frameSkipContext *frameSkipCtx )
{
    /*if the target framerate has changed, first time case needs to be visited?*/
    if(frameSkipCtx->firstTime)
    {
        frameSkipCtx->outCnt = 0;
        frameSkipCtx->inCnt = 0;

        frameSkipCtx->multipleCnt = frameSkipCtx->inputFrameRate * frameSkipCtx->outputFrameRate;
        frameSkipCtx->firstTime = FALSE;
    }

    if (frameSkipCtx->inCnt > frameSkipCtx->outCnt)
    {
        frameSkipCtx->outCnt += frameSkipCtx->outputFrameRate;
        /*skip this frame, return true*/
        return TRUE;
    }

    // out will also be multiple
    if (frameSkipCtx->inCnt == frameSkipCtx->multipleCnt)
    {
        // reset to avoid overflow
        frameSkipCtx->inCnt = frameSkipCtx->outCnt = 0;
    }

    frameSkipCtx->inCnt += frameSkipCtx->inputFrameRate;
    frameSkipCtx->outCnt += frameSkipCtx->outputFrameRate;

    /*display this frame, hence return false*/
    return FALSE;
}

 /* NOTE: if numBits passed is 2, then the bitPositions[0] and bitPositions[1] */
 /* should be consecutive and bitPositions[0] should be the smaller number*/
void Utils_resetAndSetPressureBits (UInt32 *regValue, UInt32 numBits,
                                    UInt32 bitPositions[],
                                    Utils_l3Pressure l3Pressure)
{
    UInt32 mask = 0x0, i;

    UTILS_assert ((numBits == 1) || (numBits == 2));

    for (i = 0; i < numBits; i++)
    {
        mask |= (1 << bitPositions[i]);
    }

    /*Reset the bits to be configured as 0 aka low*/
    *regValue &= ~mask;

    if (UTILS_L3_PRESSURE_LOW == l3Pressure)
    {
        /*Nothing to be done*/
    }
    else if (UTILS_L3_PRESSURE_MEDIUM == l3Pressure)
    {   /*medium pressure supported only for initiators having 2 bit settings*/
        if (numBits == 2)
        {
            /*Writing 0x01 for medium pressure*/
            *regValue |= ((1 << bitPositions[0]));
        }
        else
        {
            Vps_printf ("L3 PRESSURE: Incorrect Pressure bit settings");
            Vps_printf ("%s at Line number %d\n", __FILE__, __LINE__);
        }
    }
    else
    {
        /*Pressure is to be set as high*/
        /*Writing 0x11 for high pressure*/
        *regValue |= mask;
    }



}

/*IMPORTANT: The function Utils_setL3Pressure should be called only from  */
/*           the M3 cores                                                 */
void Utils_setL3Pressure (Utils_l3Initiator l3Initiator, Utils_l3Pressure l3Pressure)
{
    UInt32 regValue, numBits;
    UInt32 bitPositions[2];

    switch (l3Initiator)
    {
        case UTILS_INITIATOR_A8:
            regValue = Utils_readL3Pri_Pressure0 ();
            numBits = 2;
            bitPositions[0] = 0;
            bitPositions[1] = 1;
            Utils_resetAndSetPressureBits (&regValue, numBits, bitPositions,
                                           l3Pressure);
            Utils_setL3Pri_Pressure0 (regValue);

            break;

        case UTILS_INITIATOR_DUCATI:
            regValue = Utils_readL3Pri_Pressure1 ();
            numBits = 2;
            bitPositions[0] = 14;
            bitPositions[1] = 15;
            Utils_resetAndSetPressureBits (&regValue, numBits, bitPositions,
                                           l3Pressure);
            Utils_setL3Pri_Pressure1 (regValue);
            break;

        case UTILS_INITIATOR_HDVPSS0:
            regValue = Utils_readL3Pri_Pressure0 ();
            numBits = 1;
            bitPositions[0] = 8;
            Utils_resetAndSetPressureBits (&regValue, numBits, bitPositions,
                                           l3Pressure);
            Utils_setL3Pri_Pressure0 (regValue);
            break;

        case UTILS_INITIATOR_HDVPSS1:
            regValue = Utils_readL3Pri_Pressure0 ();
            numBits = 1;
            bitPositions[0] = 10;
            Utils_resetAndSetPressureBits (&regValue, numBits, bitPositions,
                                           l3Pressure);
            Utils_setL3Pri_Pressure0 (regValue);
            break;
#if !defined(TI_814X_BUILD) && !defined(TI_8107_BUILD)
        case UTILS_INITIATOR_DSP:
            regValue = Utils_readL3Pri_Pressure0 ();
            numBits = 2;
            bitPositions[0] = 4;
            bitPositions[1] = 5;
            Utils_resetAndSetPressureBits (&regValue, 2, bitPositions,
                                           l3Pressure);
            Utils_setL3Pri_Pressure0 (regValue);
            break;
#endif
        default:
            Vps_printf ("L3 PRESSURE: Incorrect Initiator passed");
            Vps_printf ("%s at Line number %d\n", __FILE__, __LINE__);
            break;

    }
}

