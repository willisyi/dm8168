/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#ifndef _DEVICEDRV_TVP5158_H_
#define _DEVICEDRV_TVP5158_H_

Int32 Device_tvp5158Init (  );
Int32 Device_tvp5158DeInit (  );

Device_Tvp5158Handle Device_tvp5158Create ( UInt32 drvId,
                                UInt32 instanceId,
                                Ptr createArgs,
                                Ptr createStatusArgs);

Int32 Device_tvp5158Delete ( Device_Tvp5158Handle handle, Ptr deleteArgs );

Int32 Device_tvp5158Control ( Device_Tvp5158Handle handle,
                           UInt32 cmd, Ptr cmdArgs, Ptr cmdStatusArgs );

#endif
