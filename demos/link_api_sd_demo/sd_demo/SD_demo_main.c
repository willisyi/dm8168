/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include <demos/link_api_sd_demo/sd_demo/SD_demo.h>
#include "ti_vdis_common_def.h"
#include <mcfw/interfaces/common_def/ti_vsys_common_def.h>
#include <demos/link_api_sd_demo/sd_demo/SD_demo_scd_bits_wr.h>


#ifdef TI816X_DVR
#    define     SYSTEM_PLATFORM_BOARD                 SYSTEM_PLATFORM_BOARD_DVR
#else
#    ifdef TI816X_EVM
#        define SYSTEM_PLATFORM_BOARD                 SYSTEM_PLATFORM_BOARD_VS
#    else
#        ifdef TI814X_EVM
#            define SYSTEM_PLATFORM_BOARD             SYSTEM_PLATFORM_BOARD_VS
#        else
#            ifdef TI814X_DVR
#                 define SYSTEM_PLATFORM_BOARD             SYSTEM_PLATFORM_BOARD_VS
#            else
#                 error "Unknown Board Type"
#            endif
#        endif
#    endif
#endif


SD_Demo_Ctrl gSD_Demo_ctrl;

char gSD_Demo_menuMain0[] = {
    "\r\n ============"
    "\r\n Demo Select"
    "\r\n ============"
    "\r\n"
};

char gSD_Demo_menuMainVs[] = {
    "\r\n"
    "\r\n 1: 16 CH Cap + DEI/H + Sclr + Enc + SW (SC5) + Disp  (16CH Cap+Enc+Disp, Progressive Enc)"
    "\r\n "
    "\r\n e: Exit "
    "\r\n "
    "\r\n Enter Choice: "
};

Void SD_Demo_menuMainShow()
{
    printf(gSD_Demo_menuMain0);

    printf(gSD_Demo_menuMainVs);
}

char gSD_Demo_runMenu[] = {

    "\r\n ============="
    "\r\n Run-Time Menu"
    "\r\n ============="
    "\r\n"
    "\r\n"
    "\r\n 1: Disable Capture Channel"
    "\r\n 2: Enable Capture Channel"
    "\r\n 3: Change Encoder Bit-rate"
    "\r\n 4: Change Encoder Frame-rate"
    "\r\n 5: BlindArea/PrivacyMask Configuration"
    "\r\n 6: Live Motion Detection (LMD) Configuration"
    "\r\n i: Print detailed system information"
    "\r\n"
    "\r\n e: Stop Demo"
    "\r\n"
    "\r\n Enter Choice: "
};

Void SD_Demo_runMenuSettings()
{
    printf(gSD_Demo_runMenu);
}


int main (int argc, char **argv )
{
    /*System Initialization. A8 (Linux) side link init calls. e..g IPC Init calls*/
    System_init();

    SD_Demo_main();

    /*System De-Initialization. A8 (Linux) side link de-init calls. e..g IPC De-Init calls*/
    System_deInit();

    return (0);
}

int SD_Demo_main()
{
    char ch;
    Bool done;

    done = FALSE;


    while(!done)
    {

        /* Demo Menu Print */
        SD_Demo_menuMainShow();

        /* Select Demo Option. Currently only one is added */
        ch = SD_Demo_getChar();

        switch(ch)
        {
            case '1':
                SD_Demo_run(); /* 16 Channel D1 Cap+Enc+Dis Video Chain */
                break;
            case 'e':
                done = TRUE;
                break;
            default:
                printf("Invalid Demo ID\n"); 
         }
    }

   return 0;
}

int SD_Demo_run()
{
    char ch;
    Bool done;
    UInt8  osdFormat[ALG_LINK_OSD_MAX_CH];

    done = FALSE;

    /* Demo Object initilization, Capture and Display device initialization */
    SD_Demo_init();

    /* Number of capture channels */
    gSD_Demo_ctrl.numCapChannels = 16;

    /* Number of encoder channels, H264 and MJPEG */
    gSD_Demo_ctrl.numEncChannels = 2 * gSD_Demo_ctrl.numCapChannels;

    gSD_Demo_ctrl.enableOsdAlgLink = 1;    /* Alg Link OSD enable/Disable */
    gSD_Demo_ctrl.enableScdAlgLink = 1;    /* Alg Link SCD enable/Disable */
    gSD_Demo_ctrl.enableVideoFramesExport = TRUE;   /* Frame Export to A8 */

    if(gSD_Demo_ctrl.enableScdAlgLink == TRUE)
    {
        /* Thread creation for SCD block metaData File write*/
        SD_Demo_ScdBitsWriteCreate();
        SD_Demo_scdTileInit();
    }

    /* Init the application specific module which will handle bitstream exchange */
    SD_Demo_bitsWriteCreate();

      /* Init the application specific module which will handle video frame exchange */
      /* Frame Write thread, Frames exported to A8 from VPSS-M3*/
    if(gSD_Demo_ctrl.enableVideoFramesExport == TRUE)
      SD_Demo_ipcFramesCreate();

    /* Setting Link create time params and creating Links */
    SD_Demo_create();

    if(gSD_Demo_ctrl.enableOsdAlgLink)
    {
        /* OSD Related configuraion and DSP Link parameter update*/
        memset(osdFormat, SYSTEM_DF_YUV420SP_UV, gSD_Demo_ctrl.numEncChannels);
        SD_Demo_osdInit(gSD_Demo_ctrl.numEncChannels, osdFormat);	
    }

    /* Blind area Related configuraion and Capture Link parameter update*/
    SD_Demo_blindAreaInit(gSD_Demo_ctrl.numCapChannels);

    /* Initiating/Starting Link operation*/
    SD_Demo_start();

    /* Register A-8 Event handler, Acknowledges SCD Events*/
    Vsys_registerEventHandler(SD_Demo_eventHandler, NULL);

    while(!done)
    {
        SD_Demo_runMenuSettings();
        ch = SD_Demo_getChar();
        printf(" \r\n");

        switch(ch)
        {
           case '1':
                /* Capture Channel Disable link call */
                SD_Demo_enableDisbaleChannel(FALSE);
                break;
           case '2':
                /* Capture Channel Enable link call */
                SD_Demo_enableDisbaleChannel(TRUE);
                break;
           case '3':
                /* Encoder bitrate update link call */
                SD_Demo_encodeBitrateChange();
                break;
           case '4':
                /* Encoder framerate update link call */
                SD_Demo_encodeFramerateChange();
                break;
           case '5':
                /* Capture Channel blind area update link call */
                SD_Demo_blindAreaUpdate(gSD_Demo_ctrl.numCapChannels);
                break;
           case '6':
                /* SCD block config update DSP link call */
                SD_Demo_scdBlockConfigUpdate();
                break;
           case 'i':
                /* Display detailed system information */
                SD_Demo_printDetailedStatistics(&gSD_Demo_ctrl);
                break;
            case 'e':
                done = TRUE;
                break;
        }

    }

    if(gSD_Demo_ctrl.enableScdAlgLink == TRUE)
    {
        SD_Demo_ScdBitsWriteStop();
    }
    
    /* Frame Write Stop */
    if(gSD_Demo_ctrl.enableVideoFramesExport == TRUE)
    {
       SD_Demo_ipcFramesStop();
    }
    
    /* Stoping Link operation*/
    SD_Demo_stop();

    /* OSD De-init call, Release memory*/
    if(gSD_Demo_ctrl.enableOsdAlgLink)
        SD_Demo_osdDeinit();

    /* Delete Links, free memory resources*/
    SD_Demo_delete();

    /* Delete Encoder Bitstream File Write Thread*/
    SD_Demo_bitsWriteDelete();

    /* Delete Frame Write thread, Frames exported to A8 from VPSS-M3*/
    if(gSD_Demo_ctrl.enableVideoFramesExport == TRUE)
       SD_Demo_ipcFramesDelete();

    /* Delete SCD block metaData file Write Thread*/
    if(gSD_Demo_ctrl.enableScdAlgLink == TRUE)
    {
        SD_Demo_ScdBitsWriteDelete();
    }

    /* Display device De-initialization */
    SD_Demo_deinit();
    return 0;
}

