/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/


#include "ti_media_std.h"
#include <device.h>
#include <device_videoDecoder.h>
#include <device_tvp5158.h>
#include <tvp5158_priv.h>

/* Global object storing all information related to all
  TVP5158 driver handles */

/* Control API that gets called when Device_control is called

  This API does handle level semaphore locking

  handle - TVP5158 driver handle
  cmd - command
  cmdArgs - command arguments
  cmdStatusArgs - command status

  returns error in case of
  - illegal parameters
  - I2C command RX/TX error
*/
Int32 Device_tvp5158Control ( Device_Tvp5158Handle handle,
                           UInt32 cmd, Ptr cmdArgs, Ptr cmdStatusArgs )
{
    Device_Tvp5158Obj *pObj = ( Device_Tvp5158Obj * ) handle;
    Int32 status;

    if ( pObj == NULL )
        return -1;

    switch ( cmd )
    {
        case DEVICE_CMD_START:
            status = Device_tvp5158Start ( pObj );
            break;

        case DEVICE_CMD_STOP:
            status = Device_tvp5158Stop ( pObj );
            break;

        case IOCTL_DEVICE_VIDEO_DECODER_GET_CHIP_ID:
            status = Device_tvp5158GetChipId ( pObj, cmdArgs, cmdStatusArgs );
            break;

        case IOCTL_DEVICE_VIDEO_DECODER_RESET:
            status = Device_tvp5158Reset ( pObj );
            break;

        case IOCTL_DEVICE_VIDEO_DECODER_SET_VIDEO_MODE:
            status = Device_tvp5158SetVideoMode ( pObj, cmdArgs );
            break;

        case IOCTL_DEVICE_VIDEO_DECODER_GET_VIDEO_STATUS:
            status = Device_tvp5158GetVideoStatus ( pObj, cmdArgs, cmdStatusArgs );
            break;

        case IOCTL_DEVICE_VIDEO_DECODER_SET_VIDEO_COLOR:
            status = Device_tvp5158SetVideoColor ( pObj, cmdArgs );
            break;

        case IOCTL_DEVICE_TVP5158_SET_VIDEO_NF:
            status = Device_tvp5158SetVideoNf ( pObj, cmdArgs );
            break;

        case IOCTL_DEVICE_VIDEO_DECODER_REG_WRITE:
            status = Device_tvp5158RegWrite ( pObj, cmdArgs );
            break;

        case IOCTL_DEVICE_VIDEO_DECODER_REG_READ:
            status = Device_tvp5158RegRead ( pObj, cmdArgs );
            break;

        case IOCTL_DEVICE_TVP5158_SET_AUDIO_MODE:
            status = Device_tvp5158SetAudioMode ( pObj, cmdArgs );
            break;

        default:
            status = -1;
            break;
    }

    return status;
}

/*
  Create API that gets called when Device_create is called

  This API does not configure the TVP5158 is any way.

  This API
  - validates parameters
  - allocates driver handle
  - stores create arguments in its internal data structure.

  Later the create arguments will be used when doing I2C communcation with
  TVP5158

  drvId - driver ID, must be DEVICE_VID_DEC_TVP5158_DRV
  instanceId - must be 0
  createArgs - create arguments
  createStatusArgs - create status
  fdmCbParams  - NOT USED, set to NULL

  returns NULL in case of any error
*/
Device_Tvp5158Handle Device_tvp5158Create ( UInt8 drvId,
                                            UInt8 instId,
                                            Ptr createArgs,
                                            Ptr createStatusArgs)
{
    Int32 status = 0;
    Device_Tvp5158Obj *pObj;
    Device_VideoDecoderCreateParams *vidDecCreateArgs
        = ( Device_VideoDecoderCreateParams * ) createArgs;

    Device_VideoDecoderCreateStatus *vidDecCreateStatus
        = ( Device_VideoDecoderCreateStatus * ) createStatusArgs;

    /*
     * check parameters
     */
    if ( vidDecCreateStatus == NULL )
        return NULL;

    vidDecCreateStatus->retVal = 0;

    if ( vidDecCreateArgs->deviceI2cInstId >= DEVICE_I2C_INST_ID_MAX
         || vidDecCreateArgs->numDevicesAtPort > DEVICE_TVP5158_DEV_MAX )
    {
        vidDecCreateStatus->retVal = -1;
        return NULL;
    }

    pObj = (Device_Tvp5158Obj *)malloc(sizeof(Device_Tvp5158Obj));

    if ( pObj == NULL )
    {
        vidDecCreateStatus->retVal = -1;
        return NULL;
    }
    else
    {
        memset(pObj, 0, sizeof(Device_Tvp5158Obj));

        gDevice_tvp5158CommonObj.tvp5158handle[instId] = pObj;

        /*
         * copy parameters to allocate driver handle
         */
        memcpy ( &pObj->createArgs, vidDecCreateArgs,
                 sizeof ( *vidDecCreateArgs ) );

        status = OSA_i2cOpen(&(gDevice_tvp5158CommonObj.i2cHandle), I2C_DEFAULT_INST_ID);

        if ( status < 0)
        {
            vidDecCreateStatus->retVal = -1;
            return NULL;
        }
    }
    /*
     * return driver handle object pointer
     */
    return pObj;
}

/*
  Delete function that is called when Device_delete is called

  This API
  - free's driver handle object

  handle - driver handle
  deleteArgs - NOT USED, set to NULL

*/
Int32 Device_tvp5158Delete ( Device_Tvp5158Handle handle, Ptr deleteArgs )
{
    Device_Tvp5158Obj *pObj = ( Device_Tvp5158Obj * ) handle;

    if ( pObj == NULL )
        return -1;


    OSA_i2cClose(&(gDevice_tvp5158CommonObj.i2cHandle));


    /*
     * free driver handle object
     */
    free(pObj);

    return 0;
}

/*
  System init for TVP5158 driver

  This API
  - create semaphore locks needed
  - registers driver to FVID2 sub-system
  - gets called as part of Device_deviceInit()

*/
Int32 Device_tvp5158Init (  )
{
    /*
     * Set to 0's for global object, descriptor memory
     */
    memset ( &gDevice_tvp5158CommonObj, 0, sizeof ( gDevice_tvp5158CommonObj ) );

    return 0;
}

Int32 Device_tvp5158DeInit (  )
{
    /*
     * Set to 0's for global object, descriptor memory
     */
    return 0;
}

