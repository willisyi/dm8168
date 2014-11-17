
#include <demos/link_api_sd_demo/sd_demo/SD_demo.h>

Int32 SD_Demo_printDetailedStatistics()
{
    UInt32 devId;

    if(gSD_Demo_ctrl.captureId!=SYSTEM_LINK_ID_INVALID)
    {
        System_linkControl(gSD_Demo_ctrl.captureId,
                            CAPTURE_LINK_CMD_PRINT_ADV_STATISTICS,
                            NULL, 0, TRUE);
    }

    for(devId=0; devId<MAX_SCLR_LINK; devId++)
    {
        if(gSD_Demo_ctrl.sclrId[devId]!=SYSTEM_LINK_ID_INVALID)
        {
             System_linkControl(gSD_Demo_ctrl.sclrId[devId],
                                SCLR_LINK_CMD_PRINT_STATISTICS,
                                NULL, 0, TRUE);
        }
    }
   for(devId=0; devId<MAX_NSF_LINK; devId++)
    {
        if(gSD_Demo_ctrl.nsfId[devId]!=SYSTEM_LINK_ID_INVALID)
        {
             System_linkControl(gSD_Demo_ctrl.nsfId[devId],
                                NSF_LINK_CMD_PRINT_STATISTICS,
                                NULL, 0, TRUE);
        }
    }

    OSA_waitMsecs(1000); // allow for print to complete

    for(devId=0; devId<MAX_DEI_LINK; devId++)
    {
        if(gSD_Demo_ctrl.deiId[devId]!=SYSTEM_LINK_ID_INVALID)
        {
            System_linkControl(gSD_Demo_ctrl.deiId[devId],
                            DEI_LINK_CMD_PRINT_STATISTICS,
                            NULL, 0, TRUE);
        }
    }

    for(devId=0; devId<MAX_SWMS_LINK; devId++)
    {
        if(gSD_Demo_ctrl.swMsId[devId]!=SYSTEM_LINK_ID_INVALID)
        {
            System_linkControl(gSD_Demo_ctrl.swMsId[devId],
                            SYSTEM_SW_MS_LINK_CMD_PRINT_STATISTICS,
                            NULL, 0, TRUE);
        }
    }

    for(devId=0; devId<MAX_DISPLAY_LINK; devId++)
    {
        if(gSD_Demo_ctrl.displayId[devId]!=SYSTEM_LINK_ID_INVALID)
        {
            System_linkControl(gSD_Demo_ctrl.displayId[devId],
                            DISPLAY_LINK_CMD_PRINT_STATISTICS,
                            NULL, 0, TRUE);
        }
    }
    for(devId=0; devId<MAX_IPC_FRAMES_LINK; devId++)
    {
        if(gSD_Demo_ctrl.ipcFramesOutVpssId[devId]!=SYSTEM_LINK_ID_INVALID)
        {
            System_linkControl(gSD_Demo_ctrl.ipcFramesOutVpssId[devId],
                            IPCFRAMESOUTRTOS_LINK_CMD_PRINT_STATISTICS,
                            NULL, 0, TRUE);
        }
    }


    OSA_waitMsecs(1000); // allow for print to complete

    for(devId=0; devId<MAX_ALG_LINK; devId++)
    {
        if(gSD_Demo_ctrl.dspAlgId[devId]!=SYSTEM_LINK_ID_INVALID)
        {
            System_linkControl(gSD_Demo_ctrl.dspAlgId[devId],
                            ALG_LINK_SCD_CMD_PRINT_STATISTICS,
                            NULL, 0, TRUE);
        }
    }

    OSA_waitMsecs(200); // allow for print to complete

    if(gSD_Demo_ctrl.encId != SYSTEM_LINK_ID_INVALID)
    {
        System_linkControl(gSD_Demo_ctrl.encId,
                            ENC_LINK_CMD_PRINT_IVAHD_STATISTICS,
                            NULL, 0, TRUE);
    }
    else if (gSD_Demo_ctrl.decId !=SYSTEM_LINK_ID_INVALID)
    {
        System_linkControl(gSD_Demo_ctrl.decId,
                            DEC_LINK_CMD_PRINT_IVAHD_STATISTICS,
                            NULL, 0, TRUE);
     }

    if(gSD_Demo_ctrl.encId != SYSTEM_LINK_ID_INVALID)
    {
        System_linkControl(gSD_Demo_ctrl.encId,
                            ENC_LINK_CMD_PRINT_STATISTICS,
                            NULL, 0, TRUE);
    }
    if(gSD_Demo_ctrl.decId != SYSTEM_LINK_ID_INVALID)
    {
        System_linkControl(gSD_Demo_ctrl.decId,
                            DEC_LINK_CMD_PRINT_STATISTICS,
                            NULL, 0, TRUE);
    }

    OSA_waitMsecs(500); // allow for print to complete

    SD_Demo_prfLoadPrint(TRUE,TRUE);

    OSA_waitMsecs(500); // allow for print to complete
    
    SD_Demo_printStatistics(TRUE);

    SD_Demo_ScdPrintStatistics(TRUE);

    return OSA_SOK;
}
