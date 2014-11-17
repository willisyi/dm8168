/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include <demos/link_api_sd_demo/sd_demo/SD_demo.h>


Void SD_Demo_delete()
{
    UInt32 linkId;

    /* Deleting Capture Links and associated VPSS-M3 links */
    SD_Demo_capDelete();

    /* Deleting Encoder Links and associated VIDEO-M3 links */
    SD_Demo_encDelete();

    /* Deleting DSP Links and associated VIDEO-M3 links */
    SD_Demo_algDelete();

    /* Deleting Display Links and associated VPSS-M3 links */
    SD_Demo_disDelete();

    /* Deleting Host-Processor A-8 Links*/
    for(linkId=0; linkId<MAX_IPC_BITS_LINK; linkId++)
    {
        if(gSD_Demo_ctrl.ipcBitsInHLOSId[linkId]!=SYSTEM_LINK_ID_INVALID)
            System_linkDelete(gSD_Demo_ctrl.ipcBitsInHLOSId[linkId]);
    }
}

Void SD_Demo_capDelete()
{
    UInt32 linkId;
    if(gSD_Demo_ctrl.captureId!=SYSTEM_LINK_ID_INVALID)
        System_linkDelete(gSD_Demo_ctrl.captureId);

    for(linkId= 0; linkId< MAX_DUP_LINK; linkId++)
    {
        if(gSD_Demo_ctrl.dupId[linkId]!=SYSTEM_LINK_ID_INVALID)
            System_linkDelete(gSD_Demo_ctrl.dupId[linkId]);
    }

    for(linkId= 0; linkId< MAX_MERGE_LINK; linkId++)
    {
        if(gSD_Demo_ctrl.mergeId[linkId]!=SYSTEM_LINK_ID_INVALID)
            System_linkDelete(gSD_Demo_ctrl.mergeId[linkId]);
    }

    for(linkId=0; linkId<MAX_DEI_LINK; linkId++)
    {
        if(gSD_Demo_ctrl.deiId[linkId]!=SYSTEM_LINK_ID_INVALID)
            System_linkDelete(gSD_Demo_ctrl.deiId[linkId]);
    }

    for(linkId=0; linkId<MAX_SCLR_LINK; linkId++)
    {
        if(gSD_Demo_ctrl.sclrId[linkId]!=SYSTEM_LINK_ID_INVALID)
            System_linkDelete(gSD_Demo_ctrl.sclrId[linkId]);
    }

    for(linkId=0; linkId<MAX_NSF_LINK; linkId++)
    {
        if(gSD_Demo_ctrl.nsfId[linkId]!=SYSTEM_LINK_ID_INVALID)
            System_linkDelete(gSD_Demo_ctrl.nsfId[linkId]);
    }

    for(linkId=0; linkId<MAX_IPC_FRAMES_LINK; linkId++)
    {
        if(gSD_Demo_ctrl.ipcFramesOutVpssId[linkId]!=SYSTEM_LINK_ID_INVALID)
        {
           System_linkDelete(gSD_Demo_ctrl.ipcFramesOutVpssId[linkId]);
        }
    }

    if(gSD_Demo_ctrl.ipcFramesOutVpssToHostId!=SYSTEM_LINK_ID_INVALID)
    {
        System_linkDelete(gSD_Demo_ctrl.ipcFramesOutVpssToHostId);
    }

    if(gSD_Demo_ctrl.ipcFramesInHostId!=SYSTEM_LINK_ID_INVALID)
    {
        System_linkDelete(gSD_Demo_ctrl.ipcFramesInHostId);
    }

    if(gSD_Demo_ctrl.ipcFramesOutHostId!=SYSTEM_LINK_ID_INVALID)
    {
        System_linkDelete(gSD_Demo_ctrl.ipcFramesOutHostId);
    }

    if(gSD_Demo_ctrl.ipcFramesInVpssFromHostId!=SYSTEM_LINK_ID_INVALID)
    {
        System_linkDelete(gSD_Demo_ctrl.ipcFramesInVpssFromHostId);
    }

    if(gSD_Demo_ctrl.ipcOutVpssId!=SYSTEM_LINK_ID_INVALID)
    {
        System_linkDelete(gSD_Demo_ctrl.ipcOutVpssId);
    }


    for(linkId = 0; linkId < NUM_CAPTURE_DEVICES; linkId++)
    {
        Device_tvp5158Delete(gSD_Demo_ctrl. tvp5158Handle[linkId], NULL);
    }

}

Void SD_Demo_encDelete()
{
    if(gSD_Demo_ctrl.ipcInVideoId!=SYSTEM_LINK_ID_INVALID)
    {
        System_linkDelete(gSD_Demo_ctrl.ipcInVideoId);
    }

    if(gSD_Demo_ctrl.encId!=SYSTEM_LINK_ID_INVALID)
    {
        System_linkDelete(gSD_Demo_ctrl.encId);
    }

    if(gSD_Demo_ctrl.ipcBitsOutRTOSId!=SYSTEM_LINK_ID_INVALID)
    {
        System_linkDelete(gSD_Demo_ctrl.ipcBitsOutRTOSId);
    }

}
Void SD_Demo_algDelete()
{
    UInt32 linkId;
    for(linkId=0; linkId<MAX_IPC_FRAMES_LINK; linkId++)
    {
        if(gSD_Demo_ctrl.ipcFramesInDspId[linkId]!=SYSTEM_LINK_ID_INVALID)
        {
            System_linkDelete(gSD_Demo_ctrl.ipcFramesInDspId[linkId]);
        }
    }
    for(linkId=0; linkId<MAX_ALG_LINK; linkId++)
    {
        if(gSD_Demo_ctrl.dspAlgId[linkId]!=SYSTEM_LINK_ID_INVALID)
        {
            System_linkDelete(gSD_Demo_ctrl.dspAlgId[linkId]);
        }
    }

}

Void SD_Demo_disDelete()
{
    UInt32 linkId;

    for(linkId=0; linkId<MAX_SWMS_LINK; linkId++)
    {
        if(gSD_Demo_ctrl.swMsId[linkId]!=SYSTEM_LINK_ID_INVALID)
            System_linkDelete(gSD_Demo_ctrl.swMsId[linkId]);
    }

    for(linkId=0; linkId<MAX_DISPLAY_LINK; linkId++)
    {
        if(gSD_Demo_ctrl.displayId[linkId]!=SYSTEM_LINK_ID_INVALID)
            System_linkDelete(gSD_Demo_ctrl.displayId[linkId]);
    }

}
