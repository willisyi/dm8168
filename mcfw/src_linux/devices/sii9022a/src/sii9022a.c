/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
 *  \file vpsdrv_hdmi9022a.c
 *
 *  \brief HDMI9022a driver
 *  This file implements functionality for the HDMI.
 *
 */

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */
#include "ti_media_std.h"
#include "ti_vsys_common_def.h"
#include <device.h>
#include <device_videoEncoder.h>
#include <device_sii9022a.h>
#include <sii9022a_priv.h>
#include <osa_i2c.h>



/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */
#define TPI_INPUTBUS_PIXEL_REP_BIT4_MASK                (0x10u)


/* ========================================================================== */
/*                         Structure Declarations                             */
/* ========================================================================== */

typedef struct
{
    VSYS_VIDEO_STANDARD_E standard;
    UInt32                modeCode;
    UInt32                pixClk;
    UInt32                vFreq;
    UInt32                pixels;
    UInt32                lines;
    struct {
        UInt32            hBitToHSync;
        UInt32            field2Offset;
        UInt32            hWidth;
        UInt32            vBitToVSync;
        UInt32            vWidth;
    } embSyncPrms;
    struct {
        UInt32            deDelay;
        /**< Width of the area to the left of the active display */
        UInt32            deTop;
        /**< Height of the area to the top of the active display */
        UInt32            deCnt;
        /**< Width of the active display */
        UInt32            deLine;
        /**< Height of the active display */
    } extSyncPrms;
} Device_SiI9022AModeInfo;


/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */

static Int32 Device_sii9022aGetHdmiChipId(Device_Sii9022aObj* psiiObj,
                                   Device_HdmiChipId *hdmichipId);

static Int32 Device_sii9022aGetDetailedChipId(Device_Sii9022aHandle handle,
                                          Ptr cmdArgs,
                                          Ptr cmdStatusArgs);

static Int32 Device_sii9022aSetMode(Device_Sii9022aHandle handle,
                                Ptr cmdArgs,
                                Ptr cmdStatusArgs);

static Int32 Device_sii9022aStart(Device_Sii9022aHandle handle,
                              Ptr cmdArgs,
                              Ptr cmdStatusArgs);
static Int32 Device_sii9022aStop(Device_Sii9022aHandle handle,
                             Ptr cmdArgs,
                             Ptr cmdStatusArgs);

static Int32 Device_sii9022aGetHpd(Device_Sii9022aHandle handle,
                               Ptr cmdArgs,
                               Ptr cmdStatusArgs);

static Int32 Device_sii9022aSetPrms(Device_Sii9022aHandle handle,
                               Ptr cmdArgs,
                               Ptr cmdStatusArgs);
static Int32 Device_sii9022aGetPrms(Device_Sii9022aHandle handle,
                               Ptr cmdArgs,
                               Ptr cmdStatusArgs);
static Int32 Device_sii9022aDeviceInit(Device_Sii9022aObj* pSiiObj);

static Int32 Device_sii9022aReset(Device_Sii9022aObj* pSiiObj);
static Int32 Device_sii9022aEnable(Device_Sii9022aObj* pSiiObj);
static Int32 Device_sii9022aPowerUpTxm(Device_Sii9022aObj* pSiiObj);
static Int32 Device_sii9022aCfgInBus(Device_Sii9022aObj* pSiiObj);
static Int32 Device_sii9022aCfgYcMode(Device_Sii9022aObj* pSiiObj);
static Int32 Device_sii9022aCfgSyncMode(Device_Sii9022aObj* pSiiObj);
static Int32 Device_sii9022aPrgmEmbSyncTimingInfo(
                Device_Sii9022aObj *siiObj,
                Device_SiI9022AModeInfo *siModeInfo);
static Int32 Device_sii9022aPrgmExtSyncTimingInfo(
                Device_Sii9022aObj *siiObj,
                Device_SiI9022AModeInfo *siModeInfo);
static Int32 Device_sii9022aPrgmAvInfoFrame(
                Device_Sii9022aObj *siiObj,
                Device_SiI9022AModeInfo *modeInfo);
static Int32 Device_sii9022aPrgmMdResetRegs(
                Device_Sii9022aObj *siiObj,
                Device_SiI9022AModeInfo *siModeInfo);
static Int32 Device_sii9022aCalcCRC(UInt8 *regAddr, UInt8 *regValue, UInt32 *regCnt);

/* ========================================================================== */
/*                            Global Variables                                */
/* ========================================================================== */

Device_SiI9022AModeInfo SiI9022ModeInfo[] =
{
    {VSYS_STD_480P, 3, 2700, 60, 858, 525,
        {16, 0, 62, 9, 6}, {122, 36, 720, 480}},
    {VSYS_STD_576P, 18, 2700, 50, 864, 625,
        {12, 0, 64, 5, 5}, {132, 44, 720, 576}},
    {VSYS_STD_720P_60, 4, 7425, 60, 1650, 750,
        {110, 0, 40, 5, 5}, {260, 25, 1280, 720}},
    {VSYS_STD_720P_50, 19, 7425, 50, 1980, 750,
        {440, 0, 40, 5, 5}, {260, 25, 1280, 720}},
    {VSYS_STD_1080P_30, 34, 7425, 30, 2200, 1125,
        {88, 0, 44, 4, 5}, {192, 41, 1920, 1080}},
    {VSYS_STD_1080P_50, 16, 7425, 50, 2640, 1125,
        {528, 0, 44, 4, 5}, {192, 41, 1920, 1080}},
    {VSYS_STD_1080P_60, 16, 14850, 60, 2200, 1125,
        {88, 0, 44, 4, 5}, {192, 41, 1920, 1080}},
    {VSYS_STD_1080I_60, 5, 7425, 60, 2200, 1125,
        {88, 564, 44, 2, 5}, {192, 20, 1920, 540}},
    {VSYS_STD_1080I_50, 20, 7425, 50, 2640, 1125,
        {528, 564, 44, 2, 5}, {192, 20, 1920, 540}},
};

#define DEVICE_SII9022A_MAX_MODES  (sizeof(SiI9022ModeInfo) /                     \
                                 sizeof(Device_SiI9022AModeInfo))


/* ========================================================================== */
/*                          Function Definitions                              */
/* ========================================================================== */


Int32 Device_sii9022aInit(void)
{

    /* Set to 0's for global object, descriptor memory  */
    memset(&gDevice_sii9022aCommonObj, 0, sizeof (Device_Sii9022aCommonObj));

    return 0;
}

Int32 Device_sii9022aDeInit( void )
{
    return 0;
}

Device_Sii9022aHandle Device_sii9022aCreate (UInt32 drvId,
                                 UInt32 instId,
                                 Ptr createArgs,
                                 Ptr createStatusArgs)
{

    Device_Sii9022aObj*  pSiiObj;
    Int32            retVal = 0;
    Device_VideoEncoderCreateParams* vidEncCreateArgs
        = (Device_VideoEncoderCreateParams*) createArgs;
    Device_VideoEncoderCreateStatus* vidEecCreateStatus
        = (Device_VideoEncoderCreateStatus*) createStatusArgs;

    if(NULL == vidEecCreateStatus)
    {
        retVal= -1;
    }

    if ((retVal >= 0) &&
        (vidEncCreateArgs->deviceI2cInstId > DEVICE_I2C_INST_ID_MAX))
    {
        retVal = -1;
    }


    if (retVal >= 0)
    {
        pSiiObj = (Device_Sii9022aObj *)malloc(sizeof(Device_Sii9022aObj));

        if ( pSiiObj == NULL )
        {
            return NULL;
        }
        else
        {
            memset(pSiiObj, 0, sizeof(Device_Sii9022aObj));

            gDevice_sii9022aCommonObj.sii9022aHandle[instId] = pSiiObj;

            pSiiObj->instId = instId;

            if (pSiiObj == NULL)
            {
                retVal = -1;
            }
            else
            {
                memcpy(&pSiiObj->createArgs,
                       vidEncCreateArgs,
                       sizeof(*vidEncCreateArgs));
            }
        }
    }

    if((retVal >= 0) && (NULL != pSiiObj))
    {
        gDevice_sii9022aCommonObj.prms.outputFormat = DEVICE_SII9022A_HDMI_RGB;
        if (DEVICE_VIDEO_ENCODER_EMBEDDED_SYNC == pSiiObj->createArgs.syncMode)
        {
            pSiiObj->syncCfgReg = 0x84;
            pSiiObj->inBusCfg = 0x60;
        }
        else /* Separate Sync Mode */
        {
            pSiiObj->syncCfgReg = 0x04;
            pSiiObj->inBusCfg = 0x70;
        }

        /* FALSE to latch data on falling clock edge. Rising edge otherwise */
        /* Bit 4 of of TPI Input bus and pixel repetation */
        if (pSiiObj->createArgs.clkEdge == FALSE)
        {
            pSiiObj->inBusCfg &= (~(TPI_INPUTBUS_PIXEL_REP_BIT4_MASK));
        }
        else
        {
            pSiiObj->inBusCfg |= TPI_INPUTBUS_PIXEL_REP_BIT4_MASK;
        }

        retVal = OSA_i2cOpen(&(gDevice_sii9022aCommonObj.i2cHandle), I2C_DEFAULT_INST_ID);


        /* Enable DE in syncpolarity register at 0x63 */
        pSiiObj->syncPolarityReg = 0x40;
        Device_sii9022aDeviceInit(pSiiObj);
        vidEecCreateStatus->retVal = 0;

        if ( retVal < 0)
        {
            return NULL;
        }

        return (pSiiObj);
    }
    else
    {
        if (NULL != vidEecCreateStatus)
        {
            vidEecCreateStatus->retVal = retVal;
        }
        return (NULL);
    }
}


Int32 Device_sii9022aDelete(Device_Sii9022aHandle handle, Ptr deleteArgs)
{
    Device_Sii9022aObj* pObj= (Device_Sii9022aObj*)handle;

    if ( pObj == NULL )
        return -1;

    OSA_i2cClose(&(gDevice_sii9022aCommonObj.i2cHandle));

    /*
     * free driver handle object
     */
    free(pObj);

    return 0;
}

Int32 Device_sii9022aControl (Device_Sii9022aHandle handle,
                              UInt32 cmd,
                              Ptr cmdArgs,
                              Ptr cmdStatusArgs)
{
    Device_Sii9022aObj *pSiiObj = (Device_Sii9022aObj *)handle;
    Int32 status;

    if ( handle == NULL )
        return -1;

    /*lock handle    */

    switch (cmd)
    {
        case DEVICE_CMD_START:
            status = Device_sii9022aStart(pSiiObj, cmdArgs, cmdStatusArgs);
            break;

        case DEVICE_CMD_STOP:
            status = Device_sii9022aStop(pSiiObj, cmdArgs, cmdStatusArgs);
            break;

        case IOCTL_DEVICE_VIDEO_ENCODER_GET_CHIP_ID:
            status = -1;
            break;

        case IOCTL_DEVICE_VIDEO_ENCODER_SET_MODE:
            status = Device_sii9022aSetMode(pSiiObj, cmdArgs, cmdStatusArgs);
            break;

        case IOCTL_DEVICE_SII9022A_GET_DETAILED_CHIP_ID:
            status = Device_sii9022aGetDetailedChipId(handle,
                                                  cmdArgs,
                                                  cmdStatusArgs);
            break;

        case IOCTL_DEVICE_SII9022A_QUERY_HPD:
            status = Device_sii9022aGetHpd(pSiiObj, cmdArgs, cmdStatusArgs);
            break;

        case IOCTL_DEVICE_SII9022A_SET_PARAMS:
            status = Device_sii9022aSetPrms(pSiiObj, cmdArgs, cmdStatusArgs);
            break;

        case IOCTL_DEVICE_SII9022A_GET_PARAMS:
            status = Device_sii9022aGetPrms(pSiiObj, cmdArgs, cmdStatusArgs);
            break;

        default:
            status = -1;
            break;
    }

    /* unlock handle   */
    return status;
}

/*
  Get TP5158 chip ID, revision ID and firmware patch ID
*/
static Int32 Device_sii9022aGetDetailedChipId(Device_Sii9022aHandle handle, Ptr cmdArgs,
                                  Ptr cmdStatusArgs)
{
    Int32 status = 0;
    Device_HdmiChipId *hdmichipId = (Device_HdmiChipId*)cmdArgs;
    Device_Sii9022aObj *pSiiObj = (Device_Sii9022aObj*)handle;

    hdmichipId->deviceId = pSiiObj->hdmiChipid.deviceId;
    hdmichipId->deviceProdRevId = pSiiObj->hdmiChipid.deviceProdRevId;
    hdmichipId->tpiRevId = pSiiObj->hdmiChipid.tpiRevId;
    hdmichipId->hdcpRevTpi = pSiiObj->hdmiChipid.hdcpRevTpi;
    status = 0;

    return (status);
}



/* Enable TMDS output */
static Int32 Device_sii9022aStart(Device_Sii9022aHandle handle,
                              Ptr cmdArgs,
                              Ptr cmdStatusArgs)
{
    Int32                    retVal = 0;
    UInt8                    regValue = 0u, regAddr;
    Device_Sii9022aObj         *pSiiObj = (Device_Sii9022aObj*)handle;

    regAddr = 0x1A;
    retVal = OSA_i2cRead8(
                &gDevice_sii9022aCommonObj.i2cHandle,
                pSiiObj->createArgs.deviceI2cAddr,
                &regAddr,
                &regValue,
                1u);
    if (retVal >= 0)
    {
        /* Enable HDMI output */
        regValue |= 0x1;
        /* Enable Output TMDS */
        regValue &= 0xEF;

        retVal = OSA_i2cWrite8(
                    &gDevice_sii9022aCommonObj.i2cHandle,
                    pSiiObj->createArgs.deviceI2cAddr,
                    &regAddr,
                    &regValue,
                    1u);

        /* Configure Input Bus after starting */
        retVal |= Device_sii9022aCfgInBus(pSiiObj);
    }

    return (retVal);
}



/* Disable TMDS output */
static Int32 Device_sii9022aStop(Device_Sii9022aHandle handle,
                             Ptr cmdArgs,
                             Ptr cmdStatusArgs)
{
    Int32                    retVal = 0;
    UInt8                    regValue = 0u, regAddr;
    Device_Sii9022aObj         *pSiiObj = (Device_Sii9022aObj*)handle;

    regAddr = 0x1A;
    retVal = OSA_i2cRead8(
                &gDevice_sii9022aCommonObj.i2cHandle,
                pSiiObj->createArgs.deviceI2cAddr,
                &regAddr,
                &regValue,
                1u);
    if (retVal >= 0)
    {
        /* Enable HDMI output */
        regValue |= 0x1;
        /* Power Down Output TMDS Clock */
        regValue |= 0x10;

        retVal = OSA_i2cWrite8(
                    &gDevice_sii9022aCommonObj.i2cHandle,
                    pSiiObj->createArgs.deviceI2cAddr,
                    &regAddr,
                    &regValue,
                    1u);
    }

    return (retVal);
}



/*
  Set Mode
*/
static Int32 Device_sii9022aSetMode(Device_Sii9022aHandle handle,
                                Ptr cmdArgs,
                                Ptr cmdStatusArgs)
{
    Int32                    retVal = 0;
    UInt32                   modeCnt;
    Device_Sii9022aObj         *pSiiObj = (Device_Sii9022aObj*)handle;
    Device_SiI9022aModeParams  *modeInfo = (Device_SiI9022aModeParams *)cmdArgs;
    Device_SiI9022AModeInfo *siModeInfo = NULL;

    for (modeCnt = 0u; modeCnt < DEVICE_SII9022A_MAX_MODES; modeCnt ++)
    {
        siModeInfo = &SiI9022ModeInfo[modeCnt];
        if (modeInfo->standard == siModeInfo->standard)
        {
            break;
        }
    }

    if (DEVICE_SII9022A_MAX_MODES == modeCnt)
    {
        printf("Unsupported mode\n");
        retVal = -1;
    }

    if (retVal >= 0)
    {
        if (DEVICE_VIDEO_ENCODER_EMBEDDED_SYNC == pSiiObj->createArgs.syncMode)
        {
            retVal |= Device_sii9022aPrgmEmbSyncTimingInfo(pSiiObj, siModeInfo);
        }
        else if (DEVICE_VIDEO_ENCODER_EXTERNAL_SYNC == pSiiObj->createArgs.syncMode)
        {
            retVal |= Device_sii9022aPrgmExtSyncTimingInfo(pSiiObj, siModeInfo);
        }
        else
        {
            retVal = -1;
        }
        retVal |= Device_sii9022aPrgmAvInfoFrame(pSiiObj, siModeInfo);
        retVal |= Device_sii9022aPrgmMdResetRegs(pSiiObj, siModeInfo);
    }

    return (retVal);
}



static Int32 Device_sii9022aGetHpd(Device_Sii9022aHandle handle,
                               Ptr cmdArgs,
                               Ptr cmdStatusArgs)
{
    Int32                    retVal = 0;
    UInt8                    regValue = 0u, regAddr;
    Device_Sii9022aObj         *pSiiObj = (Device_Sii9022aObj *)handle;
    Device_SiI9022aHpdPrms     *hpd = (Device_SiI9022aHpdPrms *)cmdArgs;

    regAddr = 0x3D;
    retVal = OSA_i2cRead8(
                &gDevice_sii9022aCommonObj.i2cHandle,
                pSiiObj->createArgs.deviceI2cAddr,
                &regAddr,
                &regValue,
                1u);

    if (retVal >= 0)
    {
        hpd->hpdEvtPending = regValue & 0x01;
        hpd->busError = regValue & 0x02;
        hpd->hpdStatus = regValue & 0x04;
    }

    return (retVal);
}



/*
  Device Init
*/
static Int32 Device_sii9022aDeviceInit(Device_Sii9022aObj* psiiObj)
{
    Int32 status = 0;

    status = Device_sii9022aReset(psiiObj);
    if(status >= 0)
    {
        status = Device_sii9022aGetHdmiChipId(psiiObj,&(psiiObj->hdmiChipid));
    }

    if (status >= 0)
    {
        status = Device_sii9022aPowerUpTxm(psiiObj);
    }

    if (status >= 0)
    {
        status = Device_sii9022aEnable(psiiObj);
    }

    if (status >= 0)
    {
        status = Device_sii9022aCfgInBus(psiiObj);
    }


    if (status >= 0)
    {
        status = Device_sii9022aCfgYcMode(psiiObj);
    }

    if (status >= 0)
    {
        status = Device_sii9022aCfgSyncMode(psiiObj);
    }

    return status;
}

/*
  Device Res-set
*/
static Int32 Device_sii9022aReset(Device_Sii9022aObj* pSiiObj)
{
    Int32 status = 0;
    UInt8 regAddr[4];
    UInt8 regValue[4];
    UInt8 numRegs;
    UInt32 rptCnt = 0;

    numRegs = 0;
    regAddr[numRegs] = 0xc7;
    regValue[numRegs] = 0x00;
    numRegs++;

    /* TODO Reset is failing for the first time with chains on TI814x. To look
     * into
     */
    for (rptCnt = 0; rptCnt < 5; rptCnt++)
    {
        status = OSA_i2cWrite8(
                &gDevice_sii9022aCommonObj.i2cHandle,
                pSiiObj->createArgs.deviceI2cAddr,
                regAddr,
                regValue,
                numRegs);
        if (status >= 0)
        {
            break;
        }
   }
    if (status < 0)
    {
        status = -1;
    }
    return status;
}

/*
  Device Enable
*/
static Int32 Device_sii9022aEnable(Device_Sii9022aObj* pSiiObj)
{
    Int32 status = 0;
    UInt8 regAddr[4];
    UInt8 regValue[4];
    UInt8 numRegs;

    numRegs = 0;
    regAddr[numRegs] = 0xBC;
    regValue[numRegs] = 0x01;
    numRegs++;

    regAddr[numRegs] = 0xBD;
    regValue[numRegs] = 0x82;
    numRegs++;

    status = OSA_i2cWrite8(
                &gDevice_sii9022aCommonObj.i2cHandle,
                pSiiObj->createArgs.deviceI2cAddr,
                regAddr,
                regValue,
                numRegs);

    if (status >= 0)
    {
        numRegs = 0;
        regAddr[numRegs] = 0xBE;
        regValue[numRegs] = 0x01;
        numRegs++;

        status = OSA_i2cRead8(
                    &gDevice_sii9022aCommonObj.i2cHandle,
                    pSiiObj->createArgs.deviceI2cAddr,
                    regAddr,
                    regValue,
                    numRegs);
        if (status >= 0)
        {
            regValue[0u] |= 0x01;
            status = OSA_i2cWrite8(
                        &gDevice_sii9022aCommonObj.i2cHandle,
                        pSiiObj->createArgs.deviceI2cAddr,
                        regAddr,
                        regValue,
                        numRegs);
        }
    }

    return status;
}


/*
  Device Power up Transmitter
*/
static Int32 Device_sii9022aPowerUpTxm(Device_Sii9022aObj* pSiiObj)
{
    Int32 status = 0;
    UInt8 regAddr;
    UInt8 regValue = 0u;

    regAddr = 0x1E;
    status = OSA_i2cRead8(
                &gDevice_sii9022aCommonObj.i2cHandle,
                pSiiObj->createArgs.deviceI2cAddr,
                &regAddr,
                &regValue,
                1);

    if (status >= 0)
    {
        regValue &= 0xFC;
        status = OSA_i2cWrite8(
                    &gDevice_sii9022aCommonObj.i2cHandle,
                    pSiiObj->createArgs.deviceI2cAddr,
                    &regAddr,
                    &regValue,
                    1);
    }

    return status;
}


/*
  Configure Input Bus and pixel repetation
*/
static Int32 Device_sii9022aCfgInBus(Device_Sii9022aObj* pSiiObj)
{
    Int32 status = 0;
    UInt8 regAddr;
    UInt8 regValue;

    regAddr = 0x08;
    regValue = pSiiObj->inBusCfg;

    status = OSA_i2cWrite8(
                    &gDevice_sii9022aCommonObj.i2cHandle,
                    pSiiObj->createArgs.deviceI2cAddr,
                    &regAddr,
                    &regValue,
                    1);

    return status;
}


/*
  Configure YC Mode
*/
static Int32 Device_sii9022aCfgYcMode(Device_Sii9022aObj* pSiiObj)
{
    Int32 status = 0;
    UInt8 regAddr;
    UInt8 regValue;

    regAddr = 0x0B;
    regValue = 0x00;

    status = OSA_i2cWrite8(
                    &gDevice_sii9022aCommonObj.i2cHandle,
                    pSiiObj->createArgs.deviceI2cAddr,
                    &regAddr,
                    &regValue,
                    1);

    return status;
}


/*
  Configure Sync Mode
*/
static Int32 Device_sii9022aCfgSyncMode(Device_Sii9022aObj* pSiiObj)
{
    Int32 status = 0;
    UInt8 regAddr;
    UInt8 regValue;

    regAddr = 0x60;
    regValue = pSiiObj->syncCfgReg;
    status = OSA_i2cWrite8(
                    &gDevice_sii9022aCommonObj.i2cHandle,
                    pSiiObj->createArgs.deviceI2cAddr,
                    &regAddr,
                    &regValue,
                    1);
    return status;
}


/*
  Get TP5158 chip ID, revision ID and firmware patch ID
*/
static Int32 Device_sii9022aGetHdmiChipId(Device_Sii9022aObj* pSiiObj,
                                   Device_HdmiChipId *hdmichipId)
{
    Int32 status = 0;
    UInt8 regAddr[8];
    UInt8 regValue[8];
    UInt8 numRegs;

    numRegs = 0;
    regAddr[numRegs] = 0x1b;
    regValue[numRegs] = 0;
    numRegs++;

    regAddr[numRegs] = 0x1c;
    regValue[numRegs] = 0;
    numRegs++;

    regAddr[numRegs] = 0x1d;
    regValue[numRegs] = 0;
    numRegs++;

    regAddr[numRegs] = 0x30;
    regValue[numRegs] = 0;
    numRegs++;

    status = OSA_i2cRead8(
                &gDevice_sii9022aCommonObj.i2cHandle,
                pSiiObj->createArgs.deviceI2cAddr,
                regAddr,
                regValue,
                numRegs);

    if (status < 0)
    {
        status = -1;
    }

    if(status >= 0)
    {
        hdmichipId->deviceId = regValue[0];
        hdmichipId->deviceProdRevId = regValue[1];
        hdmichipId->tpiRevId = regValue[2];
        hdmichipId->hdcpRevTpi = regValue[3];
    }

    return (status);
}


/*
 * Program Timing information for the separate sync input
 */
static Int32 Device_sii9022aPrgmExtSyncTimingInfo(
            Device_Sii9022aObj *pSiiObj,
            Device_SiI9022AModeInfo *siModeInfo)
{
    Int32 retVal;
    UInt8 regValue[20], regAddr[20];
    UInt32 numRegs;

    numRegs = 0;
    regAddr[numRegs] = 0x62;
    regValue[numRegs] = siModeInfo->extSyncPrms.deDelay & 0xFF;
    numRegs ++;

    regAddr[numRegs] = 0x63;
    pSiiObj->syncPolarityReg &= ~(0x03);
    pSiiObj->syncPolarityReg |=
        ((siModeInfo->extSyncPrms.deDelay & 0x300) >> 8);
    regValue[numRegs] = pSiiObj->syncPolarityReg & 0xFF;
    numRegs ++;

    regAddr[numRegs] = 0x64;
    regValue[numRegs] = siModeInfo->extSyncPrms.deTop & 0xFF;
    numRegs ++;

    regAddr[numRegs] = 0x66;
    regValue[numRegs] = siModeInfo->extSyncPrms.deCnt & 0xFF;
    numRegs ++;

    regAddr[numRegs] = 0x67;
    regValue[numRegs] = (siModeInfo->extSyncPrms.deCnt & 0xF00) >> 8;
    numRegs ++;

    regAddr[numRegs] = 0x68;
    regValue[numRegs] = siModeInfo->extSyncPrms.deLine & 0xFF;
    numRegs ++;

    regAddr[numRegs] = 0x69;
    regValue[numRegs] = (siModeInfo->extSyncPrms.deLine & 0x700) >> 8;
    numRegs ++;

    regAddr[numRegs] = 0x00;
    regValue[numRegs] = siModeInfo->pixClk & 0xFF;
    numRegs ++;

    regAddr[numRegs] = 0x01;
    regValue[numRegs] = (siModeInfo->pixClk & 0xFF00) >> 8;
    numRegs ++;

    regAddr[numRegs] = 0x02;
    regValue[numRegs] = siModeInfo->vFreq & 0xFF;
    numRegs ++;

    regAddr[numRegs] = 0x03;
    regValue[numRegs] = (siModeInfo->vFreq & 0xFF00) >> 8;
    numRegs ++;

    regAddr[numRegs] = 0x04;
    regValue[numRegs] = siModeInfo->pixels & 0xFF;
    numRegs ++;

    regAddr[numRegs] = 0x05;
    regValue[numRegs] = (siModeInfo->pixels & 0xFF00) >> 8;
    numRegs ++;

    regAddr[numRegs] = 0x06;
    regValue[numRegs] = siModeInfo->lines & 0xFF;
    numRegs ++;

    regAddr[numRegs] = 0x07;
    regValue[numRegs] = (siModeInfo->lines & 0xFF00) >> 8;
    numRegs ++;

    regAddr[numRegs] = 0x08;
    regValue[numRegs] = pSiiObj->inBusCfg;
    numRegs ++;

    regAddr[numRegs] = 0x09;
    regValue[numRegs] = 0x00;
    numRegs ++;

    regAddr[numRegs] = 0x0A;
    switch (gDevice_sii9022aCommonObj.prms.outputFormat)
    {
        case DEVICE_SII9022A_HDMI_RGB:
            regValue[numRegs] = 0x10;
            pSiiObj->isRgbOutput = 1;
            break;

        case DEVICE_SII9022A_HDMI_YUV444:
            regValue[numRegs] = 0x11;
            pSiiObj->isRgbOutput = 0;
            break;

        case DEVICE_SII9022A_HDMI_YUV422:
            regValue[numRegs] = 0x12;
            pSiiObj->isRgbOutput = 0;
            break;

        case DEVICE_SII9022A_DVI_RGB:
            regValue[numRegs] = 0x13;
            pSiiObj->isRgbOutput = 1;
            break;
        default:
            break;
    }
    numRegs ++;

    retVal = OSA_i2cWrite8(
                &gDevice_sii9022aCommonObj.i2cHandle,
                pSiiObj->createArgs.deviceI2cAddr,
                regAddr,
                regValue,
                numRegs);

    return (retVal);
}


static Int32 Device_sii9022aPrgmEmbSyncTimingInfo(
            Device_Sii9022aObj *pSiiObj,
            Device_SiI9022AModeInfo *siModeInfo)
{
    Int32 retVal;
    UInt8 regValue[20], regAddr[20];
    UInt32 numRegs;

    numRegs = 0;
    regAddr[numRegs] = 0x62;
    regValue[numRegs] = siModeInfo->embSyncPrms.hBitToHSync & 0xFF;
    numRegs ++;

    regAddr[numRegs] = 0x63;
    pSiiObj->syncPolarityReg &= ~(0x03);
    pSiiObj->syncPolarityReg |=
        ((siModeInfo->embSyncPrms.hBitToHSync & 0x300) >> 8);
    regValue[numRegs] = pSiiObj->syncPolarityReg & 0xFF;
    numRegs ++;

    regAddr[numRegs] = 0x64;
    regValue[numRegs] = siModeInfo->embSyncPrms.field2Offset & 0xFF;
    numRegs ++;

    regAddr[numRegs] = 0x65;
    regValue[numRegs] = (siModeInfo->embSyncPrms.field2Offset & 0xF00) >> 8;
    numRegs ++;

    regAddr[numRegs] = 0x66;
    regValue[numRegs] = siModeInfo->embSyncPrms.hWidth & 0xFF;
    numRegs ++;

    regAddr[numRegs] = 0x67;
    regValue[numRegs] = (siModeInfo->embSyncPrms.hWidth & 0x300) >> 8;
    numRegs ++;

    regAddr[numRegs] = 0x68;
    regValue[numRegs] = siModeInfo->embSyncPrms.vBitToVSync & 0x3F;
    numRegs ++;

    regAddr[numRegs] = 0x69;
    regValue[numRegs] = siModeInfo->embSyncPrms.vWidth & 0x3F;
    numRegs ++;

    regAddr[numRegs] = 0x00;
    regValue[numRegs] = siModeInfo->pixClk & 0xFF;
    numRegs ++;

    regAddr[numRegs] = 0x01;
    regValue[numRegs] = (siModeInfo->pixClk & 0xFF00) >> 8;
    numRegs ++;

    regAddr[numRegs] = 0x02;
    regValue[numRegs] = siModeInfo->vFreq & 0xFF;
    numRegs ++;

    regAddr[numRegs] = 0x03;
    regValue[numRegs] = (siModeInfo->vFreq & 0xFF00) >> 8;
    numRegs ++;

    regAddr[numRegs] = 0x04;
    regValue[numRegs] = siModeInfo->pixels & 0xFF;
    numRegs ++;

    regAddr[numRegs] = 0x05;
    regValue[numRegs] = (siModeInfo->pixels & 0xFF00) >> 8;
    numRegs ++;

    regAddr[numRegs] = 0x06;
    regValue[numRegs] = siModeInfo->lines & 0xFF;
    numRegs ++;

    regAddr[numRegs] = 0x07;
    regValue[numRegs] = (siModeInfo->lines & 0xFF00) >> 8;
    numRegs ++;

    regAddr[numRegs] = 0x08;
    regValue[numRegs] = pSiiObj->inBusCfg;
    numRegs ++;

    regAddr[numRegs] = 0x09;
    regValue[numRegs] = 0x02;
    numRegs ++;

    regAddr[numRegs] = 0x0A;
    switch (gDevice_sii9022aCommonObj.prms.outputFormat)
    {
        case DEVICE_SII9022A_HDMI_RGB:
            regValue[numRegs] = 0x10;
            pSiiObj->isRgbOutput = 1;
            break;

        case DEVICE_SII9022A_HDMI_YUV444:
            regValue[numRegs] = 0x11;
            pSiiObj->isRgbOutput = 0;
            break;

        case DEVICE_SII9022A_HDMI_YUV422:
            regValue[numRegs] = 0x12;
            pSiiObj->isRgbOutput = 0;
            break;

        case DEVICE_SII9022A_DVI_RGB:
            regValue[numRegs] = 0x13;
            pSiiObj->isRgbOutput = 1;
            break;
        default:
            break;
    }
    numRegs ++;

    retVal = OSA_i2cWrite8(
                &gDevice_sii9022aCommonObj.i2cHandle,
                pSiiObj->createArgs.deviceI2cAddr,
                regAddr,
                regValue,
                numRegs);

    return (retVal);
}

/* Program AVInfo Frame */
static Int32 Device_sii9022aPrgmAvInfoFrame(Device_Sii9022aObj *pSiiObj,
                                        Device_SiI9022AModeInfo *modeInfo)
{
    Int32 retVal = 0;
    UInt8 regValue[15], regAddr[15];
    UInt32 regCnt;

    regCnt = 0;

    regAddr[regCnt] = 0x0D;
    if (pSiiObj->isRgbOutput)
    {
        regValue[regCnt] = 0x01;
    }
    else
    {
        regValue[regCnt] = 0x21;
    }
    regCnt ++;

    regAddr[regCnt] = 0x0E;
    regValue[regCnt] = 0xA0;
    regCnt ++;

    regAddr[regCnt] = 0x0F;
    regValue[regCnt] = 0x00;
    regCnt ++;

    regAddr[regCnt] = 0x10;
    regValue[regCnt] = modeInfo->modeCode & 0x7F;
    regCnt ++;

    regAddr[regCnt] = 0x11;
    regValue[regCnt] = 0x00;
    regCnt ++;

    regAddr[regCnt] = 0x12;
    regValue[regCnt] = 0x00;
    regCnt ++;

    regAddr[regCnt] = 0x13;
    regValue[regCnt] = 0x00;
    regCnt ++;

    regAddr[regCnt] = 0x14;
    regValue[regCnt] = 0x00;
    regCnt ++;

    regAddr[regCnt] = 0x15;
    regValue[regCnt] = 0x00;
    regCnt ++;

    regAddr[regCnt] = 0x16;
    regValue[regCnt] = 0x00;
    regCnt ++;

    regAddr[regCnt] = 0x17;
    regValue[regCnt] = 0x00;
    regCnt ++;

    regAddr[regCnt] = 0x18;
    regValue[regCnt] = 0x00;
    regCnt ++;

    regAddr[regCnt] = 0x19;
    regValue[regCnt] = 0x00;
    regCnt ++;

    if (retVal >= 0)
    {
        retVal = Device_sii9022aCalcCRC(regAddr, regValue, &regCnt);

        if (retVal >= 0)
        {
            if (DEVICE_SII9022A_DVI_RGB == gDevice_sii9022aCommonObj.prms.outputFormat)
            {
                memset(regValue, 0x0, sizeof(regValue));
            }

            retVal = OSA_i2cWrite8(
                        &gDevice_sii9022aCommonObj.i2cHandle,
                        pSiiObj->createArgs.deviceI2cAddr,
                        regAddr,
                        regValue,
                        regCnt);
        }
    }

    regAddr[0] = 0x19;
    regValue[0] = 0x00;
    retVal |= OSA_i2cWrite8(
                &gDevice_sii9022aCommonObj.i2cHandle,
                pSiiObj->createArgs.deviceI2cAddr,
                regAddr,
                regValue,
                1);

    return (retVal);
}

static Int32 Device_sii9022aPrgmMdResetRegs(Device_Sii9022aObj *pSiiObj,
                                        Device_SiI9022AModeInfo *siModeInfo)
{
    Int32 retVal = 0;
    UInt8 regAddr, regValue;

    regAddr = 0x63;
    regValue = pSiiObj->syncPolarityReg;
    retVal = OSA_i2cWrite8(
                &gDevice_sii9022aCommonObj.i2cHandle,
                pSiiObj->createArgs.deviceI2cAddr,
                &regAddr,
                &regValue,
                1);

    regAddr = 0x60;
    regValue = pSiiObj->syncCfgReg;
    retVal |= OSA_i2cWrite8(
                &gDevice_sii9022aCommonObj.i2cHandle,
                pSiiObj->createArgs.deviceI2cAddr,
                &regAddr,
                &regValue,
                1);

    if (retVal >= 0)
    {
        usleep(5000);
        regAddr = 0x61;
        retVal = OSA_i2cRead8(
                        &gDevice_sii9022aCommonObj.i2cHandle,
                        pSiiObj->createArgs.deviceI2cAddr,
                        &regAddr,
                        &regValue,
                        1);
        if (retVal >= 0)
        {
            /* Set the same sync polarity in 0x63 register */
            pSiiObj->syncPolarityReg &= ~(0x30);
            pSiiObj->syncPolarityReg |= ((regValue & 0x03) << 4);

            regAddr = 0x63;
            regValue = pSiiObj->syncPolarityReg;
            retVal = OSA_i2cWrite8(
                        &gDevice_sii9022aCommonObj.i2cHandle,
                        pSiiObj->createArgs.deviceI2cAddr,
                        &regAddr,
                        &regValue,
                        1);
        }
    }

    return (retVal);
}

static Int32 Device_sii9022aCalcCRC(UInt8 *regAddr, UInt8 *regValue, UInt32 *regCnt)
{
    Int32 retVal = 0;
    UInt32 cnt, sum = 0;

    for (cnt = 0u; cnt < *regCnt; cnt ++)
    {
        sum += regValue[cnt];
    }

    if (retVal >= 0)
    {
        sum += 0x82 + 0x02 + 13;
        sum &= 0xFF;
        regValue[*regCnt] = 0x100 - sum;
        regAddr[*regCnt] = 0x0C;
        (*regCnt) ++;
    }

    return (retVal);
}

static Int32 Device_sii9022aSetPrms(Device_Sii9022aHandle handle,
                               Ptr cmdArgs,
                               Ptr cmdStatusArgs)
{
    Int32 retVal = 0;
    Device_SiI9022aPrms *prms = (Device_SiI9022aPrms *)cmdArgs;

    if (NULL == prms)
    {
        printf("Sii9022A: Null Pointer\n");
        retVal = -1;
    }
    else
    {
        if (DEVICE_SII9022A_MAX_FORMAT <= prms->outputFormat)
        {
            printf("Sii9022A: Wrong output Type\n");
            retVal = -1;
        }

        if (retVal >= 0)
        {
            gDevice_sii9022aCommonObj.prms.outputFormat = prms->outputFormat;
        }
    }

    return (retVal);
}

static Int32 Device_sii9022aGetPrms(Device_Sii9022aHandle handle,
                               Ptr cmdArgs,
                               Ptr cmdStatusArgs)
{
    Int32 retVal = 0;
    Device_SiI9022aPrms *prms = (Device_SiI9022aPrms *)cmdArgs;

    if (NULL == prms)
    {
        printf("Sii9022A: Null Pointer\n");
        retVal = -1;
    }
    else
    {
        prms->outputFormat = gDevice_sii9022aCommonObj.prms.outputFormat;
    }

    return (retVal);
}
