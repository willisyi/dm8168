
/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include <demos/link_api_sd_demo/sd_demo/SD_demo.h>

Void SD_Demo_stop()
{
    UInt32 linkId, i;
    Int32 status = 0;

     for(i = 0; i < NUM_CAPTURE_DEVICES; i++)
     {
         if(gSD_Demo_ctrl.tvp5158Handle[i])
         {
                  status = Device_tvp5158Control(gSD_Demo_ctrl.tvp5158Handle[i],
	                                         DEVICE_CMD_STOP,
                                         NULL,
                                         NULL);
         }
     }

    /* stop needs to be in the reseverse order of create */

    if(gSD_Demo_ctrl.captureId!=SYSTEM_LINK_ID_INVALID)
    {
        System_linkStop(gSD_Demo_ctrl.captureId);
    }



    for(linkId=0; linkId<MAX_SCLR_LINK; linkId++)
    {
        if(gSD_Demo_ctrl.sclrId[linkId]!=SYSTEM_LINK_ID_INVALID)
        {
            System_linkStop(gSD_Demo_ctrl.sclrId[linkId]);
        }
    }

    for(linkId = 0;linkId < MAX_DEI_LINK;linkId++)
    {
        if(gSD_Demo_ctrl.deiId[linkId]!=SYSTEM_LINK_ID_INVALID)
        {
            System_linkStop(gSD_Demo_ctrl.deiId[linkId]);
        }
    }

   
    for(linkId=0; linkId<MAX_NSF_LINK; linkId++)
    {
        if(gSD_Demo_ctrl.nsfId[linkId]!=SYSTEM_LINK_ID_INVALID)
        {
            System_linkStop(gSD_Demo_ctrl.nsfId[linkId]);
        }
    }

    for(linkId = 0; linkId < MAX_IPC_FRAMES_LINK; linkId++)
    {
        if(gSD_Demo_ctrl.ipcFramesOutVpssId[linkId]!=SYSTEM_LINK_ID_INVALID)
        {
           System_linkStop(gSD_Demo_ctrl.ipcFramesOutVpssId[linkId]);
        }
        if(gSD_Demo_ctrl.ipcFramesInDspId[linkId]!=SYSTEM_LINK_ID_INVALID)
        {
            System_linkStop(gSD_Demo_ctrl.ipcFramesInDspId[linkId]);
        }
    }

    if(gSD_Demo_ctrl.ipcFramesOutVpssToHostId!=SYSTEM_LINK_ID_INVALID)
    {
        System_linkStop(gSD_Demo_ctrl.ipcFramesOutVpssToHostId);
    }

    if(gSD_Demo_ctrl.ipcFramesInHostId!=SYSTEM_LINK_ID_INVALID)
    {
        System_linkStop(gSD_Demo_ctrl.ipcFramesInHostId);
    }

    if(gSD_Demo_ctrl.ipcFramesOutHostId!=SYSTEM_LINK_ID_INVALID)
    {
        System_linkStop(gSD_Demo_ctrl.ipcFramesOutHostId);
    }

    if(gSD_Demo_ctrl.ipcFramesInVpssFromHostId!=SYSTEM_LINK_ID_INVALID)
    {
        System_linkStop(gSD_Demo_ctrl.ipcFramesInVpssFromHostId);
    }

    if(gSD_Demo_ctrl.ipcBitsOutDSPId!=SYSTEM_LINK_ID_INVALID)
    {
        System_linkStop(gSD_Demo_ctrl.ipcBitsOutDSPId);
    }

    if(gSD_Demo_ctrl.ipcOutVpssId!=SYSTEM_LINK_ID_INVALID)
        System_linkStop(gSD_Demo_ctrl.ipcOutVpssId);

    if(gSD_Demo_ctrl.ipcInVideoId!=SYSTEM_LINK_ID_INVALID)
        System_linkStop(gSD_Demo_ctrl.ipcInVideoId);

    if(gSD_Demo_ctrl.encId!=SYSTEM_LINK_ID_INVALID)
        System_linkStop(gSD_Demo_ctrl.encId);

    if(gSD_Demo_ctrl.ipcBitsOutRTOSId!=SYSTEM_LINK_ID_INVALID)
        System_linkStop(gSD_Demo_ctrl.ipcBitsOutRTOSId);



    for(linkId=0; linkId<MAX_IPC_BITS_LINK; linkId++)
    {
        if(gSD_Demo_ctrl.ipcBitsInHLOSId[linkId]!=SYSTEM_LINK_ID_INVALID)
        {
            System_linkStop(gSD_Demo_ctrl.ipcBitsInHLOSId[linkId]);
        }
    }


    for(linkId=0; linkId<MAX_SWMS_LINK; linkId++)
    {
      if(gSD_Demo_ctrl.swMsId[linkId]!=SYSTEM_LINK_ID_INVALID)
           System_linkStop(gSD_Demo_ctrl.swMsId[linkId]);
    }
    
    if(gSD_Demo_ctrl.prm.enableConfigExtVideoEncoder)
    {
        Device_sii9022aControl(gSD_Demo_ctrl.sii9022aHandle,
                                    DEVICE_CMD_STOP,
                                    NULL,
                                    NULL);
    }

    for(linkId=0; linkId<MAX_DISPLAY_LINK; linkId++)
    {
      if(gSD_Demo_ctrl.displayId[linkId]!=SYSTEM_LINK_ID_INVALID)
           System_linkStop(gSD_Demo_ctrl.displayId[linkId]);
    }

}
