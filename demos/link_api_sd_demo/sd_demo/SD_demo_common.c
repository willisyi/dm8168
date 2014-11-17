/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include <demos/link_api_sd_demo/sd_demo/SD_demo.h>
#include <demos/link_api_sd_demo/sd_demo/SD_demo_common.h>
#include <mcfw/interfaces/common_def/ti_vsys_common_def.h>
#define MAX_IN_STR_SIZE                      (128)

UInt32 gSD_Demo_enabledProcs[] = {
    SYSTEM_LINK_ID_M3VPSS,
    SYSTEM_LINK_ID_M3VIDEO,
    SYSTEM_LINK_ID_DSP,
};


char *gSD_Demo_cpuName[SYSTEM_PLATFORM_CPU_REV_MAX] = {
    "ES1.0",
    "ES1.1",
    "ES2.0",
    "ES2.1",
    "UNKNOWN",
};

char *gSD_Demo_boardName[SYSTEM_PLATFORM_BOARD_MAX] = {
    "UNKNOWN",
    "4x TVP5158 VS",
    "2x SII9135, 1x TVP7002 VC",
    "2x SIL1161A, 2x TVP7002 Catalog"
    "2x SIL1161A, 2x TVP7002 DVR"
};

char *gSD_Demo_boardRev[SYSTEM_PLATFORM_BOARD_REV_MAX] = {
    "UNKNOWN",
    "REV A",
    "REV B",
    "REV C",
    "DVR"
};

typedef struct
{
    Bool   scdTileConfigInitFlag;
    int  scdTileParam[SD_DEMO_SCD_MAX_CHANNELS][SD_DEMO_SCD_MAX_TILES];
}scdTileData;

scdTileData scdTileconfig;

char SD_Demo_getChar()
{
    char buffer[MAX_IN_STR_SIZE];

    fflush(stdin);
    fgets(buffer, MAX_IN_STR_SIZE, stdin);

    return(buffer[0]);
}


int SD_Demo_getIntValue(char *string, int minVal, int maxVal, int defaultVal)
{
    char inputStr[MAX_IN_STR_SIZE];
    int value;

    printf(" \n");
    printf(" Enter %s [Valid values, %d .. %d] : ", string, minVal, maxVal);

    fflush(stdin);
    fgets(inputStr, MAX_IN_STR_SIZE, stdin);

    value = atoi(inputStr);

    if(value < minVal || value > maxVal )
    {
        value = defaultVal;
        printf(" \n");
        printf(" WARNING: Invalid value specified, defaulting to value of = %d \n", value);
    }
    else
    {
        printf(" \n");
        printf(" Entered value = %d \n", value);
    }

    printf(" \n");

    return value;
}

int SD_Demo_getChId(char *string, int maxChId)
{
    char inputStr[MAX_IN_STR_SIZE];
    int chId;

    printf(" \n");
    printf(" Select %s CH ID [0 .. %d] : ", string, maxChId-1);

    fflush(stdin);
    fgets(inputStr, MAX_IN_STR_SIZE, stdin);

    chId = atoi(inputStr);

    if(chId < 0 || chId >= maxChId )
    {
        chId = 0;

        printf(" \n");
        printf(" WARNING: Invalid CH ID specified, defaulting to CH ID = %d \n", chId);
    }
    else
    {
        printf(" \n");
        printf(" Selected CH ID = %d \n", chId);
    }

    printf(" \n");

    return chId;
}

Bool SD_Demo_getFileWriteEnable()
{
    char inputStr[MAX_IN_STR_SIZE];
    Bool enable;

    printf(" \n");
    printf(" Enable file write (YES - y / NO - n) : ");

    inputStr[0] = 0;

    fflush(stdin);
    fgets(inputStr, MAX_IN_STR_SIZE, stdin);

    enable = FALSE;

    if(inputStr[0]=='y' || inputStr[0]=='Y' )
    {
        enable = TRUE;
    }

    printf(" \n");
    if(enable)
        printf(" File write ENABLED !!!\n");
    else
        printf(" File write DISABLED !!!\n");
    printf(" \n");
    return enable;
}

Bool SD_Demo_isPathValid( const char* absolutePath )
{

    if(access( absolutePath, F_OK ) == 0 ){

        struct stat status;
        stat( absolutePath, &status );

        return (status.st_mode & S_IFDIR) != 0;
    }
    return FALSE;
}

int SD_Demo_getFileWritePath(char *path, char *defaultPath)
{
    int status=0;

    printf(" \n");
    printf(" Enter file write path : ");

    fflush(stdin);
    fgets(path, MAX_IN_STR_SIZE, stdin);

    printf(" \n");

    /* remove \n from the path name */
    path[ strlen(path)-1 ] = 0;

    if(!SD_Demo_isPathValid(path))
    {
        printf(" WARNING: Invalid path [%s], trying default path [%s] ...\n", path, defaultPath);

        strcpy(path, defaultPath);

        if(!SD_Demo_isPathValid(path))
        {
            printf(" WARNING: Invalid default path [%s], file write will FAIL !!! \n", path);

            status = -1;
        }
    }

    if(status==0)
    {
        printf(" Selected file write path [%s] \n", path);
    }

    printf(" \n");

    return 0;


}

Int32 SD_Demo_detectBoard()
{
    Int32 status;
    UInt32 boardRev, boardId, cpuRev;

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
    printf(" %u: SD_Demo  : CPU Revision [%s] !!! \r\n",
        OSA_getCurTimeInMsec(), gSD_Demo_cpuName[cpuRev]);

    /* Detect board */
    boardId = platformInfo.boardId;
    if (boardId >= SYSTEM_PLATFORM_BOARD_MAX)
    {
        boardId = SYSTEM_PLATFORM_BOARD_UNKNOWN;
    }
    printf(" %u: SD_Demo  : Detected [%s] Board !!! \r\n",
        OSA_getCurTimeInMsec(), gSD_Demo_boardName[boardId]);

    /* Get base board revision */
    boardRev = platformInfo.baseBoardRev;
    if (boardRev >= SYSTEM_PLATFORM_BOARD_REV_MAX)
    {
        boardRev = SYSTEM_PLATFORM_BOARD_REV_UNKNOWN;
    }
    printf(" %u: SD_Demo  : Base Board Revision [%s] !!! \r\n",
        OSA_getCurTimeInMsec(), gSD_Demo_boardRev[boardRev]);

    if (boardId != SYSTEM_PLATFORM_BOARD_UNKNOWN)
    {
        /* Get daughter card revision */
        boardRev = platformInfo.dcBoardRev;
        if (boardRev >= SYSTEM_PLATFORM_BOARD_REV_MAX)
        {
            boardRev = SYSTEM_PLATFORM_BOARD_REV_UNKNOWN;
        }
        printf(" %u: SD_Demo  : Daughter Card Revision [%s] !!! \r\n",
            OSA_getCurTimeInMsec(), gSD_Demo_boardRev[boardRev]);
    }

    return 0;
}


Int32 SD_Demo_memPrintHeapStatus()
{
    UInt32 numProcs, procId;
    SystemCommon_PrintStatus printStatus;

    memset(&printStatus, 0, sizeof(printStatus));
    numProcs = sizeof(gSD_Demo_enabledProcs)/sizeof(gSD_Demo_enabledProcs[0]);

    printStatus.printHeapStatus = TRUE;

    for(procId=0; procId<numProcs; procId++)
    {
        System_linkControl(
                gSD_Demo_enabledProcs[procId],
                SYSTEM_COMMON_CMD_CPU_LOAD_CALC_START,
                &printStatus,
                sizeof(printStatus),
                TRUE
            );
    }

    return 0;
}


Int32 SD_Demo_prfLoadCalcEnable(Bool enable, Bool printStatus, Bool printTskLoad)
{
    UInt32 numProcs, procId;

    numProcs = sizeof(gSD_Demo_enabledProcs)/sizeof(gSD_Demo_enabledProcs[0]);

    for(procId=0; procId<numProcs; procId++)
    {
        if(enable)
        {

            /* Start Load calculation at each CPU*/ 
            System_linkControl(
                gSD_Demo_enabledProcs[procId],
                SYSTEM_COMMON_CMD_CPU_LOAD_CALC_START,
                NULL,
                0,
                TRUE
            );
        }
        else
        {
            /* Stop Load calculation at each CPU*/ 
            System_linkControl(
                gSD_Demo_enabledProcs[procId],
                SYSTEM_COMMON_CMD_CPU_LOAD_CALC_STOP,
                NULL,
                0,
                TRUE
            );
            if(printStatus)
            {
                SystemCommon_PrintStatus printStatus;

                 /* Print Loading of each CPU*/ 
                memset(&printStatus, 0, sizeof(printStatus));

                printStatus.printCpuLoad = TRUE;
                printStatus.printTskLoad = printTskLoad;
                System_linkControl(
                    gSD_Demo_enabledProcs[procId],
                    SYSTEM_COMMON_CMD_PRINT_STATUS,
                    &printStatus,
                    sizeof(printStatus),
                    TRUE
                );
            }
            /* Reset Load calculation at each CPU*/ 
            System_linkControl(
                gSD_Demo_enabledProcs[procId],
                SYSTEM_COMMON_CMD_CPU_LOAD_CALC_RESET,
                NULL,
                0,
                TRUE
            );
        }
    }
    return 0;
}
Int32 SD_Demo_prfLoadPrint(Bool printTskLoad,Bool resetTskLoad)
{
    UInt32 numProcs, procId;

    numProcs = sizeof(gSD_Demo_enabledProcs)/sizeof(gSD_Demo_enabledProcs[0]);

    for(procId=0; procId<numProcs; procId++)
    {
        SystemCommon_PrintStatus printStatus;

        /* Print Loading of each CPU*/ 
        memset(&printStatus, 0, sizeof(printStatus));

        printStatus.printCpuLoad = TRUE;
        printStatus.printTskLoad = printTskLoad;
        System_linkControl(
                   gSD_Demo_enabledProcs[procId],
                   SYSTEM_COMMON_CMD_PRINT_STATUS,
                   &printStatus,
                   sizeof(printStatus),
                   TRUE);

        /* Reset Load calculation at each CPU*/ 
        if (resetTskLoad)
        {
            System_linkControl(
               gSD_Demo_enabledProcs[procId],
               SYSTEM_COMMON_CMD_CPU_LOAD_CALC_RESET,
               NULL,
               0,
               TRUE);
        }
        OSA_waitMsecs(500); // allow for print to complete
    }
    return 0;
}



Int32 SD_Demo_captureGetTamperStatus(Ptr pPrm)
{
    AlgLink_ScdChStatus *pScdStat = (AlgLink_ScdChStatus *)pPrm;

    if(pScdStat->size != sizeof(AlgLink_ScdChStatus))
    {
       printf(" [TAMPER DETECT EVENT] Invalid param size, ignoring results\n");
    }

    if(pScdStat->frmResult == ALG_LINK_SCD_DETECTOR_CHANGE)
    {
        printf(" [TAMPER DETECTED] %d: SCD CH <%d> CAP CH = %d \n", OSA_getCurTimeInMsec(), pScdStat->chId, pScdStat->chId);
    }

    return 0;
}

Int32 SD_Demo_captureGetMotionStatus(Ptr pPrm)
{
    AlgLink_ScdResult *pScdResult = (AlgLink_ScdResult *)pPrm;

    printf(" [MOTION DETECTED] %d: SCD CH <%d> CAP CH = %d \n", OSA_getCurTimeInMsec(), pScdResult->chId, pScdResult->chId);


    return 0;
}

/* A8 Event handler. Processes event received from other cores. */
Int32 SD_Demo_eventHandler(UInt32 eventId, Ptr pPrm, Ptr appData)
{

    /* Tamper Detect Event signal by DSP-SCD link */
    if(eventId==VSYS_EVENT_TAMPER_DETECT)
    {
        SD_Demo_captureGetTamperStatus(pPrm);
    }

    /* Motion Detect Event signal by DSP-SCD link 
     * Below logic is used in DSP to signal motion detect
     *   a. 1st check blocks with more than 50% pixel change. 
     *   b. If the no. of blocks (with the true condition of above check) 
     *      are more than 50% of total enabled block then we send command 
     *      to A8 of motion detected. 
     */

    if(eventId==VSYS_EVENT_MOTION_DETECT)
    {
        SD_Demo_captureGetMotionStatus(pPrm);
    }

    return 0;
}

Int32 SD_Demo_allocBuf(UInt32 srRegId, UInt32 bufSize, UInt32 bufAlign, SD_Demo_AllocBufInfo *bufInfo)
{
    IHeap_Handle heapHndl;

    heapHndl = SharedRegion_getHeap(srRegId);
    OSA_assert(heapHndl != NULL);

    bufInfo->virtAddr = NULL;
    bufInfo->physAddr = NULL;
    bufInfo->srPtr    = 0;

    bufInfo->virtAddr = Memory_alloc(heapHndl, bufSize, bufAlign, NULL);

    if(bufInfo->virtAddr==NULL)
        return -1;

    bufInfo->physAddr = Memory_translate (bufInfo->virtAddr, Memory_XltFlags_Virt2Phys);

    if(bufInfo->physAddr==NULL)
        return -1;

    bufInfo->srPtr = SharedRegion_getSRPtr(bufInfo->virtAddr,srRegId);

    return 0;
}

Int32 SD_Demo_freeBuf(UInt32 srRegId, UInt8 *virtAddr, UInt32 bufSize)
{
    IHeap_Handle heapHndl;

    heapHndl = SharedRegion_getHeap(srRegId);
    OSA_assert(heapHndl != NULL);

    OSA_assert(virtAddr != NULL);

    Memory_free(heapHndl, virtAddr, bufSize);

    return 0;
}


int SD_Demo_enableDisbaleChannel(Bool enableChn)
{
    int chId;

    char *onOffName[] = { "OFF ", "ON" };
    Int32 status=OSA_SOK;
    DeiLink_ChannelInfo channelInfo;
    SclrLink_ChannelInfo SclrchannelInfo;
    UInt32 cmdId, deiId;
    UInt32 noOfDEIChan;
    
    
    chId = SD_Demo_getChId("CAPTURE", gSD_Demo_ctrl.numCapChannels);


    /* Secondary channel, control SC5 for enable/disable */
    SclrchannelInfo.channelId = chId;
    SclrchannelInfo.enable    = enableChn;

    if(enableChn)
        cmdId = SCLR_LINK_CMD_ENABLE_CHANNEL;
    else
        cmdId = SCLR_LINK_CMD_DISABLE_CHANNEL;

    if(gSD_Demo_ctrl.sclrId[0]!=SYSTEM_LINK_ID_INVALID)
    {
        status = System_linkControl(
                                    gSD_Demo_ctrl.sclrId[0],
                                    cmdId,
                                    &(SclrchannelInfo),
                                    sizeof(SclrLink_ChannelInfo),
                                    TRUE
                                    );
       printf(" SD Demo: Sclr CH%d = [%s]\n", chId, onOffName[enableChn]);
    }
    UTILS_assert(status==OSA_SOK);

    noOfDEIChan = DEI_LINK_MAX_CH;

    /* Channel has to disable/enable on all the paths of DEI */
    /* Commands are send separatly for all the queues */
    channelInfo.channelId = chId%noOfDEIChan;
    /* DEI-SC Stream/Queue */
    channelInfo.streamId  = 0;
    channelInfo.enable    = enableChn;
    deiId = chId/noOfDEIChan;

    if(enableChn)
        cmdId = DEI_LINK_CMD_ENABLE_CHANNEL;
    else
        cmdId = DEI_LINK_CMD_DISABLE_CHANNEL;


    if(deiId < MAX_DEI_LINK)
    {
        if(gSD_Demo_ctrl.deiId[deiId]!=SYSTEM_LINK_ID_INVALID)
        {
            status = System_linkControl(
                                        gSD_Demo_ctrl.deiId[deiId],
                                        cmdId,
                                        &(channelInfo),
                                        sizeof(DeiLink_ChannelInfo),
                                        TRUE
                                        );
           printf(" SD Demo: DEI-ID %d Stream/QueueId %d ChId %d = [%s]\n",
                     deiId,channelInfo.streamId, channelInfo.channelId, onOffName[enableChn]);
        }
    }

    UTILS_assert(status==OSA_SOK);
    /* DEI-VIP Stream/Queue */
    channelInfo.streamId  = 1;
    if(deiId < MAX_DEI_LINK)
    {
        if(gSD_Demo_ctrl.deiId[deiId]!=SYSTEM_LINK_ID_INVALID)
        {
            status = System_linkControl(
                                        gSD_Demo_ctrl.deiId[deiId],
                                        cmdId,
                                        &(channelInfo),
                                        sizeof(DeiLink_ChannelInfo),
                                        TRUE
                                        );
           printf(" SD Demo: DEI-ID %d Stream/QueueId %d ChId %d = [%s]\n",
                     deiId,channelInfo.streamId, channelInfo.channelId, onOffName[enableChn]);
        }
    }
    UTILS_assert(status==OSA_SOK);
    /* DEI-VIP Secondary Stream/Queue */
    channelInfo.streamId  = 2;
    if(deiId < MAX_DEI_LINK)
    {
        if(gSD_Demo_ctrl.deiId[deiId]!=SYSTEM_LINK_ID_INVALID)
        {
            status = System_linkControl(
                                        gSD_Demo_ctrl.deiId[deiId],
                                        cmdId,
                                        &(channelInfo),
                                        sizeof(DeiLink_ChannelInfo),
                                        TRUE
                                        );
           printf(" SD Demo: DEI-ID %d Stream/QueueId %d ChId %d = [%s]\n",
                     deiId,channelInfo.streamId, channelInfo.channelId, onOffName[enableChn]);
        }
    }
    UTILS_assert(status==OSA_SOK);
    return 0;
}

int SD_Demo_scdTileInit()
{
    UInt32 chId, idx;

    /* Initializing Tiles at A8 
     * Read Section 2.1.2 in DM81xx_DVR_RDK_DemoGuide.pdf for more details on 
     * Tile based block config update at UI.
     * Configuring params of all the tiles.
     */

    if(scdTileconfig.scdTileConfigInitFlag == FALSE)
    {
        for(chId = 0; chId < SD_DEMO_SCD_MAX_CHANNELS; chId++)
           for(idx = 0; idx < SD_DEMO_SCD_MAX_TILES; idx++)
              scdTileconfig.scdTileParam[chId][idx] = 0;  /*disabling all the tiles */

        scdTileconfig.scdTileConfigInitFlag = TRUE;
    }
    return 0;
}
int SD_Demo_scdBlockConfigUpdate()
{
      UInt32 chId;
      AlgLink_ScdChFrameParams scdChPrm;
      /**< SCD channel configurable params */

      AlgLink_ScdChblkUpdate scdChBlkPrm;
      /**< SCD channel block configurable params */


      printf("\n In the current demo, frame is divided in to a set of 3x3 tiles (Max 9-tiles).\n");
      printf("\n User can select any tile starting from no 0 - 8 (topLeft - bottomRight)\n");
//    printf(" Once the tile is selected, user can update block config of all the blocks within that tile\n");
      chId = SD_Demo_getChId("CAPTURE", gSD_Demo_ctrl.numCapChannels);

      memset(&scdChBlkPrm, 0, sizeof(scdChBlkPrm));
      scdChPrm.chId = chId;

      {
           UInt32 tileId,startX, startY, endX, endY;
           UInt32 maxHorBlks, maxVerBlks, maxHorBlkPerTile, maxVerBlkPerTile;
           Int32 i, j, sensitivity, flag = 0;

           maxHorBlks = 11;  /* CIF Resolution rounded to 32 pixels */
           maxVerBlks = 24;  /* CIF Resolution*/
           maxHorBlkPerTile   = 4;
           maxVerBlkPerTile   = 8;


           printf("\n Enabled Tile Nos:\t");
           for(i = 0; i < SD_DEMO_SCD_MAX_TILES; i++)
           {
              if(scdTileconfig.scdTileParam[chId][i] == 1)
              {
                 flag = 1;
                 printf("%d, ",i);
              }
           }
           if(flag == 0)
              printf("Currently zero tiles are enabled for LMD for Chan-%d\n",chId);

           printf("\n");
           tileId = SD_Demo_getIntValue(" Tile Id/No.", 0, 8, 0);

           startX = (tileId%3)*maxHorBlkPerTile;
           startY = (tileId/3)*maxVerBlkPerTile;

           endX = startX + 4;
           endY = startY + 8;

           if(endX > maxHorBlks)
              endX = maxHorBlks;
           if(endY > maxVerBlks)
              endY = maxVerBlks;

           scdChBlkPrm.chId = chId;
           scdChBlkPrm.numValidBlock = 0;

          flag        = SD_Demo_getIntValue("LMD block enable/disable flag", 0, 1, 1);

          if(flag == 1)
          {
             sensitivity = SD_Demo_getIntValue("LMD block sensitivity", 0, 6, 6);
             scdTileconfig.scdTileParam[chId][tileId] = 1;
          }
          else
          {
             scdTileconfig.scdTileParam[chId][tileId] = 0;
             sensitivity = 0;
          }


           for(i = startY; i < endY; i++)
           {
               for(j = startX; j < endX; j++)
               {
                   AlgLink_ScdChBlkConfig * blkConfig;
                   blkConfig = &scdChBlkPrm.blkConfig[scdChBlkPrm.numValidBlock];

                   blkConfig->blockId      =  j + (i * 11)   ;
                   blkConfig->sensitivity  = sensitivity;
                   blkConfig->monitorBlock = flag;
                   scdChBlkPrm.numValidBlock++;
               }
           }
           printf("\nNumber of blocks in the tile to be updated %d\n\n",scdChBlkPrm.numValidBlock);
      }
      System_linkControl(
                         gSD_Demo_ctrl.dspAlgId[1],
                         ALG_LINK_SCD_CMD_SET_CHANNEL_BLOCKCONFIG,
                         &(scdChBlkPrm),
                         sizeof(scdChBlkPrm),
                         TRUE
                         );

      return 0;
}

/* API to update Encoder Bitrate */
int SD_Demo_encodeBitrateChange()
{
    int chId, value;
    EncLink_ChBitRateParams params;

    chId = SD_Demo_getChId("ENCODE", gSD_Demo_ctrl.numEncChannels);

    value = SD_Demo_getIntValue("Encode Bit-rate (in Kbps)", 64, 4000, 2000);

    memset(&params, 0, sizeof(params));

    params.chId = chId;
    printf("\r\n Channel Selected: %d", params.chId);

    /* New bitrate value */
    params.targetBitRate = value * 1000;

    System_linkControl(
                       gSD_Demo_ctrl.encId,
                       ENC_LINK_CMD_SET_CODEC_BITRATE,
                       &params,
                       sizeof(params),
                       TRUE);
    return 0;
}

/* API to update Encoder Framerate */
int SD_Demo_encodeFramerateChange()
{
    int chId, value;
    EncLink_ChFpsParams params;

    chId = SD_Demo_getChId("ENCODE", gSD_Demo_ctrl.numEncChannels);

    value = SD_Demo_getIntValue("Encode frame-rate", 1, 30, 30);

    memset(&params, 0, sizeof(params));

    /* New fps vaule in fps x 1000 formate */
    params.targetFps = (1000 * value);

    params.targetBitRate = 0;

    params.chId = chId;
    printf("\r\n Channel Selected: %d", params.chId);

    System_linkControl(
                       gSD_Demo_ctrl.encId,
                       ENC_LINK_CMD_SET_CODEC_FPS,
                       &params, 
                       sizeof(params),
                       TRUE);


    return 0;
}
