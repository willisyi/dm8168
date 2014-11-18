/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/


#include "ti_media_std.h"
#include <device.h>

UInt32 Device_getVidDecI2cAddr(UInt32 vidDecId, UInt32 vipInstId)
{

   UInt32 devAddr = 0;

#ifdef TI814X_DVR
    #if 1
    UInt32 devAddrTvp5158[DEVICE_CAPT_INST_MAX] = { 0x5b, 0x5a, 0x59, 0x58 };
    #else
    UInt32 devAddrTvp5158[DEVICE_CAPT_INST_MAX] = { 0x58, 0x59, 0x5a, 0x5b };
    #endif

    switch (vidDecId)
    {
        case DEVICE_VID_DEC_TVP5158_DRV:

            devAddr = devAddrTvp5158[vipInstId];
            break;

        default:
           break;

    }
#endif

#ifdef TI816X_DVR
   UInt32 devAddrTvp5158[DEVICE_CAPT_INST_MAX] = { 0x5c, 0x5d, 0x5e, 0x5f };
   UInt32 devAddrSii9135[DEVICE_CAPT_INST_MAX] = { 0x31, 0x00, 0x30, 0x00 };
   UInt32 devAddrTvp7002[DEVICE_CAPT_INST_MAX] = { 0x5d, 0x00, 0x5c, 0x00 };
   UInt8 devAddrSii9022[2u]                    = { 0x39, 0x39 };

   switch (vidDecId)
   {
       case DEVICE_VID_DEC_TVP5158_DRV:
           devAddr = devAddrTvp5158[vipInstId];
           break;

       case DEVICE_VID_DEC_SII9135_DRV:
           devAddr = devAddrSii9135[vipInstId];
           break;

       case DEVICE_VID_DEC_TVP7002_DRV:
           devAddr = devAddrTvp7002[vipInstId];
           break;

        case DEVICE_VID_ENC_SII9022A_DRV:
            devAddr = devAddrSii9022[0];
            break;

       default:
           break;
   }
#endif

#ifdef TI816X_EVM
    UInt32 devAddrTvp5158[DEVICE_CAPT_INST_MAX] = { 0x58, 0x5c, 0x5a, 0x5e };
    UInt32 devAddrSii9135[DEVICE_CAPT_INST_MAX] = { 0x31, 0x00, 0x30, 0x00 };
    UInt32 devAddrTvp7002[DEVICE_CAPT_INST_MAX] = { 0x5d, 0x00, 0x5c, 0x00 };
    UInt8 devAddrSii9022[2u]                    = { 0x39, 0x39 };

    switch (vidDecId)
    {
        case DEVICE_VID_DEC_TVP5158_DRV:

            devAddr = devAddrTvp5158[vipInstId];
            break;

        case DEVICE_VID_DEC_SII9135_DRV:
            devAddr = devAddrSii9135[vipInstId];
            break;

        case DEVICE_VID_DEC_TVP7002_DRV:
            devAddr = devAddrTvp7002[vipInstId];
            break;

        case DEVICE_VID_ENC_SII9022A_DRV:
            devAddr = devAddrSii9022[0];
            break;

        default:
           break;

    }
#endif

#ifdef TI816X_CZ
    UInt8 devAddrSii9233a[2u]                    = { 0x30, 0x34 };
    UInt8 devAddrSii9134[2u]                     = { 0x39, 0x3c };
    switch (vidDecId)
    {
        case DEVICE_VID_DEC_SII9233A_DRV:
            devAddr = devAddrSii9233a[0];
            break;

        case DEVICE_VID_ENC_SII9134_DRV:
            devAddr = devAddrSii9134[0];
            break;

        default:
            break;
    }
#endif

#if defined (TI814X_EVM) || defined (TI8107_EVM) || defined (TI8107_DVR)

    UInt32 devAddrTvp5158[DEVICE_CAPT_INST_MAX] = { 0x58, 0x5c, 0x5a, 0x5e };

    //TODO
    UInt32 devAddrSii9135[DEVICE_CAPT_INST_MAX] = { 0x00, 0x00, 0x30, 0x00 };
    UInt32 devAddrTvp7002[DEVICE_CAPT_INST_MAX] = { 0x5d, 0x00, 0x00, 0x00 };
    UInt8  devAddrSii9022[2u]                   = { 0x39, 0x39 };

    switch (vidDecId)
    {
        case DEVICE_VID_DEC_TVP5158_DRV:

            devAddr = devAddrTvp5158[vipInstId];
            break;

        case DEVICE_VID_DEC_SII9135_DRV:
            devAddr = devAddrSii9135[vipInstId];
            break;

        case DEVICE_VID_DEC_TVP7002_DRV:
            devAddr = devAddrTvp7002[vipInstId];
            break;

        case DEVICE_VID_ENC_SII9022A_DRV:
            devAddr = devAddrSii9022[0];
            break;
        default:
            break;
    }


#endif
   return (devAddr);

}


