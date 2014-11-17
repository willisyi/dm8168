/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include <demos/link_api_sd_demo/sd_demo/SD_demo.h>

Void SD_Demo_start()
{
    UInt32 linkId;
    UInt32 i;

    if(gSD_Demo_ctrl.prm.enableConfigExtVideoEncoder)
    {
       int status;
       status = Device_sii9022aControl(gSD_Demo_ctrl.sii9022aHandle,
                                     DEVICE_CMD_START,
                                     NULL,
                                     NULL);
    }

    for(linkId = 0; linkId < MAX_DISPLAY_LINK; linkId++)
    {
        if(gSD_Demo_ctrl.displayId[linkId]!=SYSTEM_LINK_ID_INVALID)
        {
            System_linkStart(gSD_Demo_ctrl.displayId[linkId]);
        }
    }

    for(linkId = 0; linkId < MAX_SWMS_LINK; linkId++)
    {
        if(gSD_Demo_ctrl.swMsId[linkId]!=SYSTEM_LINK_ID_INVALID)
        {
            System_linkStart(gSD_Demo_ctrl.swMsId[linkId]);
        }
    }
    for(linkId=0; linkId<MAX_IPC_BITS_LINK; linkId++)
    {
        if(gSD_Demo_ctrl.ipcBitsInHLOSId[linkId]!=SYSTEM_LINK_ID_INVALID)
        {
            System_linkStart(gSD_Demo_ctrl.ipcBitsInHLOSId[linkId]);
        }
    }

    if (gSD_Demo_ctrl.ipcFramesOutHostId != SYSTEM_LINK_ID_INVALID)
    {
        System_linkStart(gSD_Demo_ctrl.ipcFramesOutHostId);
        OSA_assert(gSD_Demo_ctrl.ipcFramesInVpssFromHostId != SYSTEM_LINK_ID_INVALID);
        System_linkStart(gSD_Demo_ctrl.ipcFramesInVpssFromHostId);
    }
    if(gSD_Demo_ctrl.ipcBitsOutRTOSId!=SYSTEM_LINK_ID_INVALID)
        System_linkStart(gSD_Demo_ctrl.ipcBitsOutRTOSId);

    if(gSD_Demo_ctrl.encId!=SYSTEM_LINK_ID_INVALID)
        System_linkStart(gSD_Demo_ctrl.encId);


    if(gSD_Demo_ctrl.ipcInVideoId!=SYSTEM_LINK_ID_INVALID)
        System_linkStart(gSD_Demo_ctrl.ipcInVideoId);

    if(gSD_Demo_ctrl.ipcOutVpssId!=SYSTEM_LINK_ID_INVALID)
        System_linkStart(gSD_Demo_ctrl.ipcOutVpssId);


    for(i = 0; i < NUM_CAPTURE_DEVICES; i++)
    {
        int status;
        if(gSD_Demo_ctrl.tvp5158Handle[i])
        {
             status = Device_tvp5158Control(gSD_Demo_ctrl.tvp5158Handle[i],
                                    DEVICE_CMD_START,
                                    NULL,
                                    NULL);
        }
    }
    if(gSD_Demo_ctrl.ipcFramesInHostId!=SYSTEM_LINK_ID_INVALID)
    {
        System_linkStart(gSD_Demo_ctrl.ipcFramesInHostId);
    }

    /* start can happen in any order, except its recommended to start capture Link the last */
    if(gSD_Demo_ctrl.ipcFramesOutVpssToHostId!=SYSTEM_LINK_ID_INVALID)
    {
        System_linkStart(gSD_Demo_ctrl.ipcFramesOutVpssToHostId);
    }

    /* start can happen in any order, except its recommended to start capture Link the last */
    for(linkId = 0; linkId < MAX_IPC_FRAMES_LINK; linkId++)
    {
        if(gSD_Demo_ctrl.ipcFramesOutVpssId[linkId]!=SYSTEM_LINK_ID_INVALID)
        {
            System_linkStart(gSD_Demo_ctrl.ipcFramesOutVpssId[linkId]);
        }
        if(gSD_Demo_ctrl.ipcFramesInDspId[linkId]!=SYSTEM_LINK_ID_INVALID)
        {
            System_linkStart(gSD_Demo_ctrl.ipcFramesInDspId[linkId]);
        }
    }
    
    /* start can happen in any order, except its recommended to start capture Link the last */
    for(linkId=0; linkId<MAX_NSF_LINK; linkId++)
    {
        if(gSD_Demo_ctrl.nsfId[linkId]!=SYSTEM_LINK_ID_INVALID)
        {
            System_linkStart(gSD_Demo_ctrl.nsfId[linkId]);
        }
    }

    for(linkId = 0; linkId < MAX_DEI_LINK; linkId++)
    {
        if(gSD_Demo_ctrl.deiId[linkId]!=SYSTEM_LINK_ID_INVALID)
        {
            System_linkStart(gSD_Demo_ctrl.deiId[linkId]);
        }
    }

    for(linkId=0; linkId<MAX_SCLR_LINK; linkId++)
    {
        if(gSD_Demo_ctrl.sclrId[linkId]!=SYSTEM_LINK_ID_INVALID)
        {
            System_linkStart(gSD_Demo_ctrl.sclrId[linkId]);
        }
    }

    if(gSD_Demo_ctrl.captureId!=SYSTEM_LINK_ID_INVALID)
    {
        System_linkStart(gSD_Demo_ctrl.captureId);
    }
}
