#include "multich_common.h"


#if defined (TI816X_DVR) || defined (TI8107_DVR)
#    define     SYSTEM_PLATFORM_BOARD                 SYSTEM_PLATFORM_BOARD_DVR
#else
#    ifdef TI816X_EVM
#        define SYSTEM_PLATFORM_BOARD                 SYSTEM_PLATFORM_BOARD_VS
#    else
#        ifdef TI814X_EVM
#            define SYSTEM_PLATFORM_BOARD             SYSTEM_PLATFORM_BOARD_VS
#        else
#           ifdef TI814X_DVR
#                define SYSTEM_PLATFORM_BOARD             SYSTEM_PLATFORM_BOARD_VS
#            else
#               ifdef TI8107_EVM
#                   define SYSTEM_PLATFORM_BOARD          SYSTEM_PLATFORM_BOARD_VS
#               else
#                   ifdef TI816X_CZ
#                       define SYSTEM_PLATFORM_BOARD      SYSTEM_PLATFORM_BOARD_CZ
#                   else
#                       ifdef TI816X_ETV
#                           define SYSTEM_PLATFORM_BOARD  SYSTEM_PLATFORM_BOARD_ETV
#                       else
#                           error "Unknown Board Type"
#                       endif
#                   endif
#               endif
#            endif
#        endif
#    endif
#endif

UInt32 gMultiCh_enabledProcs[] = {
    SYSTEM_LINK_ID_M3VPSS,
    SYSTEM_LINK_ID_M3VIDEO,
#if !defined (TI_8107_BUILD)
    SYSTEM_LINK_ID_DSP
#endif


};

char *gMultiCh_cpuName[SYSTEM_PLATFORM_CPU_REV_MAX] = {
    "ES1.0",
    "ES1.1",
    "ES2.0",
    "ES2.1",
    "UNKNOWN",
};

char *gMultiCh_boardName[SYSTEM_PLATFORM_BOARD_MAX] = {
    "UNKNOWN",
    "4x TVP5158 VS",
    "2x SII9135, 1x TVP7002 VC",
    "2x SIL1161A, 2x TVP7002 Catalog"
    "2x SIL1161A, 2x TVP7002 DVR"
    "1x SII9233A, 1x SII9134 CZ"
};

char *gMultiCh_boardRev[SYSTEM_PLATFORM_BOARD_REV_MAX] = {
    "UNKNOWN",
    "REV A",
    "REV B",
    "REV C",
    "DVR"
};

Int32 MultiCh_detectBoard()
{
    Int32 status;
    UInt32 boardRev;
    UInt32 cpuRev;
    UInt32 boardId;


    SystemVpss_PlatformInfo  platformInfo;

    status = System_linkControl(
                SYSTEM_LINK_ID_M3VPSS,
                SYSTEM_M3VPSS_CMD_GET_PLATFORM_INFO,
                &platformInfo,
                sizeof(platformInfo),
                TRUE
                );

    UTILS_assert(status==OSA_SOK);

    /* Get CPU version */
    cpuRev = platformInfo.cpuRev;
    if (cpuRev >= SYSTEM_PLATFORM_CPU_REV_MAX)
    {
        cpuRev = SYSTEM_PLATFORM_CPU_REV_UNKNOWN;
    }
    printf(" %u: MCFW  : CPU Revision [%s] !!! \r\n",
        OSA_getCurTimeInMsec(), gMultiCh_cpuName[cpuRev]);

    /* Detect board */
    boardId = platformInfo.boardId;
    if (boardId >= SYSTEM_PLATFORM_BOARD_MAX)
    {
        boardId = SYSTEM_PLATFORM_BOARD_UNKNOWN;
    }
    printf(" %u: MCFW  : Detected [%s] Board !!! \r\n",
        OSA_getCurTimeInMsec(), gMultiCh_boardName[boardId]);

    /* Get base board revision */
    boardRev = platformInfo.baseBoardRev;
    if (boardRev >= SYSTEM_PLATFORM_BOARD_REV_MAX)
    {
        boardRev = SYSTEM_PLATFORM_BOARD_REV_UNKNOWN;
    }
    printf(" %u: MCFW  : Base Board Revision [%s] !!! \r\n",
        OSA_getCurTimeInMsec(), gMultiCh_boardRev[boardRev]);

    if (boardId != SYSTEM_PLATFORM_BOARD_UNKNOWN)
    {
        /* Get daughter card revision */
        boardRev = platformInfo.dcBoardRev;
        if (boardRev >= SYSTEM_PLATFORM_BOARD_REV_MAX)
        {
            boardRev = SYSTEM_PLATFORM_BOARD_REV_UNKNOWN;
        }
        printf(" %u: MCFW  : Daughter Card Revision [%s] !!! \r\n",
            OSA_getCurTimeInMsec(), gMultiCh_boardRev[boardRev]);
    }

    return 0;
}

Int32 MultiCh_prfLoadCalcEnable(Bool enable, Bool printStatus, Bool printTskLoad)
{
    UInt32 numProcs, procId;

    numProcs = sizeof(gMultiCh_enabledProcs)/sizeof(gMultiCh_enabledProcs[0]);

    for(procId=0; procId<numProcs; procId++)
    {
        if(enable)
        {
            System_linkControl(
                gMultiCh_enabledProcs[procId],
                SYSTEM_COMMON_CMD_CPU_LOAD_CALC_START,
                NULL,
                0,
                TRUE
            );
        }
        else
        {
            System_linkControl(
                gMultiCh_enabledProcs[procId],
                SYSTEM_COMMON_CMD_CPU_LOAD_CALC_STOP,
                NULL,
                0,
                TRUE
            );
            if(printStatus)
            {
                SystemCommon_PrintStatus printStatus;

                memset(&printStatus, 0, sizeof(printStatus));

                printStatus.printCpuLoad = TRUE;
                printStatus.printTskLoad = printTskLoad;
                System_linkControl(
                    gMultiCh_enabledProcs[procId],
                    SYSTEM_COMMON_CMD_PRINT_STATUS,
                    &printStatus,
                    sizeof(printStatus),
                    TRUE
                );
            }
            System_linkControl(
                gMultiCh_enabledProcs[procId],
                SYSTEM_COMMON_CMD_CPU_LOAD_CALC_RESET,
                NULL,
                0,
                TRUE
            );
        }
    }

    return 0;
}

Int32 MultiCh_prfLoadPrint(Bool printTskLoad,Bool resetTskLoad)
{
    UInt32 numProcs, procId;

    numProcs = sizeof(gMultiCh_enabledProcs)/sizeof(gMultiCh_enabledProcs[0]);

    for(procId=0; procId<numProcs; procId++)
    {
        SystemCommon_PrintStatus printStatus;

        memset(&printStatus, 0, sizeof(printStatus));

        printStatus.printCpuLoad = TRUE;
        printStatus.printTskLoad = printTskLoad;
        System_linkControl(
                   gMultiCh_enabledProcs[procId],
                   SYSTEM_COMMON_CMD_PRINT_STATUS,
                   &printStatus,
                   sizeof(printStatus),
                   TRUE);
        if (resetTskLoad)
        {
            System_linkControl(
               gMultiCh_enabledProcs[procId],
               SYSTEM_COMMON_CMD_CPU_LOAD_CALC_RESET,
               NULL,
               0,
               TRUE);
        }
        OSA_waitMsecs(500); // allow for print to complete
    }
    return 0;
}

Int32 MultiCh_memPrintHeapStatus()
{
    UInt32 numProcs, procId;
    SystemCommon_PrintStatus printStatus;

    memset(&printStatus, 0, sizeof(printStatus));
    numProcs = sizeof(gMultiCh_enabledProcs)/sizeof(gMultiCh_enabledProcs[0]);

    printStatus.printHeapStatus = TRUE;

    for(procId=0; procId<numProcs; procId++)
    {
        System_linkControl(
                gMultiCh_enabledProcs[procId],
                SYSTEM_COMMON_CMD_CPU_LOAD_CALC_START,
                &printStatus,
                sizeof(printStatus),
                TRUE
            );
    }

    return 0;
}

Int32 MultiCh_swMsGetOutSize(UInt32 outRes, UInt32 * width, UInt32 * height)
{
    switch (outRes)
    {
        case VSYS_STD_MAX:
            *width = 1920;
            *height = 1200;
            break;

        case VSYS_STD_720P_60:
            *width = 1280;
            *height = 720;
            break;

        default:
        case VSYS_STD_1080I_60:
        case VSYS_STD_1080P_60:
        case VSYS_STD_1080P_50:
        case VSYS_STD_1080P_30:
            *width = 1920;
            *height = 1080;
            break;

        case VSYS_STD_NTSC:
            *width = 720;
            *height = 480;
            break;

        case VSYS_STD_PAL:
            *width = 720;
            *height = 576;
            break;

        case VSYS_STD_XGA_60:
            *width = 1024;
            *height = 768;
            break;

        case VSYS_STD_SXGA_60:
            *width = 1280;
            *height = 1024;
            break;

        case VSYS_STD_WUXGA_60:
            *width = 1920;
            *height = 1200;
            break;

        case VSYS_STD_SVGA_60:
            *width = 800;
            *height = 600;
            break;
    }
    return 0;
}

/* forceLowCostScaling set to TRUE will take effect only when lineSkipMode is set to TRUE */
Void MultiCh_swMsGetDefaultLayoutPrm(UInt32 devId, SwMsLink_CreateParams *swMsCreateArgs, Bool forceLowCostScaling)
{
    SwMsLink_LayoutPrm *layoutInfo;
    SwMsLink_LayoutWinInfo *winInfo;
    UInt32 outWidth, outHeight, row, col, winId, widthAlign, heightAlign;
    VDIS_MOSAIC_S *mosaicParam;
    WINDOW_S *windowInfo;

    MultiCh_swMsGetOutSize(swMsCreateArgs->maxOutRes, &outWidth, &outHeight);

    widthAlign = 8;
    heightAlign = 1;

    if(devId>=VDIS_DEV_MAX)
        devId = VDIS_DEV_HDMI;

    layoutInfo = &swMsCreateArgs->layoutPrm;

    /* init to known default */
    memset(layoutInfo, 0, sizeof(*layoutInfo));

    mosaicParam = &(gVdisModuleContext.vdisConfig.mosaicParams[devId]);
    if (mosaicParam->userSetDefaultSWMLayout == TRUE)
    {
      /* get default layout info from user define */
      layoutInfo->onlyCh2WinMapChanged = mosaicParam->onlyCh2WinMapChanged;
      layoutInfo->outputFPS = mosaicParam->outputFPS;
      layoutInfo->numWin = mosaicParam->numberOfWindows;
      for (winId = 0; winId < layoutInfo->numWin; winId++)
      {
        winInfo = &layoutInfo->winInfo[winId];
        windowInfo = &mosaicParam->winList[winId];
        winInfo->width  = windowInfo->width;
        winInfo->height = windowInfo->height;
        winInfo->startX = windowInfo->start_X;
        winInfo->startY = windowInfo->start_Y;
        winInfo->channelNum = mosaicParam->chnMap[winId];
        winInfo->bypass = mosaicParam->useLowCostScaling[winId];
        #ifdef TI_816X_BUILD
        /* support CIF preview when lowCostSC is enabled */
        if ((gVsysModuleContext.vsysConfig.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC) &&
             gVsysModuleContext.vsysConfig.enableSecondaryOut == TRUE)
        {
            if ((mosaicParam->useLowCostScaling[winId] == TRUE) &&
                (winInfo->channelNum < VDIS_CHN_MAX/2))
            {
                winInfo->channelNum = winInfo->channelNum + VDIS_CHN_MAX;
                winInfo->bypass  = FALSE;
            }
        }
        #endif

      }
    }
    else /* using MCFW default layout info */
    {
      layoutInfo->onlyCh2WinMapChanged = FALSE;
      layoutInfo->outputFPS = mosaicParam->outputFPS;
      layoutInfo->numWin = 8;
      for(row=0; row<2; row++)
      {
          for(col=0; col<2; col++)
          {
              winId = row*2+col;

              winInfo = &layoutInfo->winInfo[winId];

              winInfo->width  = SystemUtils_floor((outWidth*2)/5, widthAlign);
              winInfo->height = SystemUtils_floor(outHeight/2, heightAlign);
              winInfo->startX = winInfo->width*col;
              winInfo->startY = winInfo->height*row;
              if (forceLowCostScaling == TRUE)
                winInfo->bypass = TRUE;
              else
                winInfo->bypass = FALSE;
              winInfo->channelNum = devId*(gVdisModuleContext.vdisConfig.numChannels/2) + winId;
              #ifdef TI_816X_BUILD
              /* support CIF preview when lowCostSC is enabled */
              if ((gVsysModuleContext.vsysConfig.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC) &&
                  (gVsysModuleContext.vsysConfig.enableSecondaryOut == TRUE))
              {
                if ((mosaicParam->useLowCostScaling[winId] == TRUE) &&
                    (winInfo->channelNum < VDIS_CHN_MAX/2))
                {
                  winInfo->channelNum = winInfo->channelNum + VDIS_CHN_MAX;
                }
              }
              #endif
          }
      }

      for(row=0; row<4; row++)
      {
          winId = 4 + row;

          winInfo = &layoutInfo->winInfo[winId];

          winInfo->width  = layoutInfo->winInfo[0].width/2;
          winInfo->height = layoutInfo->winInfo[0].height/2;
          winInfo->startX = layoutInfo->winInfo[0].width*2;
          winInfo->startY = winInfo->height*row;
          if (forceLowCostScaling == TRUE)
            winInfo->bypass = TRUE;
          else
            winInfo->bypass = FALSE;

          winInfo->channelNum = devId*(gVdisModuleContext.vdisConfig.numChannels/2) + winId;
          #ifdef TI_816X_BUILD
          /* support CIF preview when lowCostSC is enabled */
          if ((gVsysModuleContext.vsysConfig.systemUseCase == VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC)&&
              (gVsysModuleContext.vsysConfig.enableSecondaryOut == TRUE))
          {
            if ((mosaicParam->useLowCostScaling[winId] == TRUE) &&
                (winInfo->channelNum < VDIS_CHN_MAX/2))
            {
              winInfo->channelNum = winInfo->channelNum + VDIS_CHN_MAX;
            }
          }
          #endif
      }
    }
}

Void MultiCh_setDec2DispMap(VDIS_DEV displayID,
                            UInt32 numChn,
                            UInt32 startDecChn,
                            UInt32 startDisplayChn)
{
    Int i;
    VDEC_DISPLAY_CHN_MAP_S decChMap[VDEC_CHN_MAX];

    OSA_assert(numChn < VDEC_CHN_MAX);
    for (i = 0; i < numChn; i++)
    {
        decChMap[i].decChnId = startDecChn + i;
        decChMap[i].displayChnId = startDisplayChn + i;
    }
    Vdec_setDecCh2DisplayChMap(displayID,numChn,decChMap);
}


