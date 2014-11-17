/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2011 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
  \file demo_vdec_vdis_bits_rd.c
  \brief
  */



#include <demo_vdec_vdis.h>
#include <mcfw/interfaces/link_api/avsync.h>


VdecVdis_Config      gVdecVdis_config;
VdecVdis_IpcBitsCtrl gVdecVdis_obj;

void loop_test_open(UInt32 chId, UInt32 fileIndex);
static UInt32 VdecVdis_getChnlIdFromBufSize(UInt32 resId);


Void VdecVdis_setTplayConfig(VDIS_CHN vdispChnId, VDIS_AVSYNC speed)
{
    gVdecVdis_config.gDemo_TrickPlayMode.channel= vdispChnId;
    if (speed == VDIS_AVSYNC_2X || speed == VDIS_AVSYNC_4X) {
        gVdecVdis_config.gDemo_TrickPlayMode.speed = speed;
    }
    else {
        gVdecVdis_config.gDemo_TrickPlayMode.speed = 1;
    }
}

static Void VdecVdis_bitsRdGetEmptyBitBufs(VCODEC_BITSBUF_LIST_S *emptyBufList, UInt32 resId)
{
    VDEC_BUF_REQUEST_S reqInfo;
    UInt32 i;

    emptyBufList->numBufs = 0;

    // require 2 buffers for each channel
    reqInfo.numBufs = gVdecVdis_config.numChnlInRes[resId] ;
    reqInfo.reqType = VDEC_BUFREQTYPE_CHID;

    for (i = 0; i < reqInfo.numBufs ; i++)
    {
        reqInfo.u[i].chNum = VdecVdis_getChnlIdFromBufSize(resId);
    }

    Vdec_requestBitstreamBuffer(&reqInfo, emptyBufList, 0);
}

static UInt32 VdecVdis_getChnlIdFromBufSize(UInt32 resId)
{
    UInt32 channel;
    UInt32 lastId;
    static UInt16 tempSkipCount = 1;
    static UInt16 tempSkipFlag = 0;
    lastId = gVdecVdis_obj.lastId[resId];

    // get channel ID
    channel = gVdecVdis_config.resToChnl[resId][lastId];
    if (channel == gVdecVdis_config.gDemo_TrickPlayMode.channel){
        if (tempSkipCount++ < gVdecVdis_config.gDemo_TrickPlayMode.speed) {
            tempSkipFlag = 1;
        }
        else {
            tempSkipFlag = 0;
            tempSkipCount = 1;
        }
    }
    else {
        tempSkipFlag = 0;
    }

    if (tempSkipFlag == 0) {
        lastId ++;
    }

    if (lastId >= gVdecVdis_config.numChnlInRes[resId])
    {
        lastId = 0;
    }

    gVdecVdis_obj.lastId[resId] = lastId;

    OSA_assert(channel < gVdecVdis_config.fileNum);

    return channel;

}

static void VdecVdis_bitsRdFillEmptyBuf(VCODEC_BITSBUF_S *pEmptyBuf)
{
    int statHdr, statData;
    int curCh;

    curCh = pEmptyBuf->chnId;

    if(gVdecVdis_obj.fpRdHdr[curCh] == NULL)
        return;

    if(gVdecVdis_config.fileInfo[curCh].enable == 0)
        return;

    if(gVdecVdis_obj.switchInputFile)
    {
        if ((gVdecVdis_obj.switchInputFile) && (curCh == gVdecVdis_obj.chId))
        {
            loop_test_open(curCh, gVdecVdis_obj.FileIndex);
            gVdecVdis_obj.switchInputFile = FALSE;
        }
    }

    statHdr  = fscanf(gVdecVdis_obj.fpRdHdr[curCh],"%d",&(pEmptyBuf->filledBufSize));
    
    OSA_assert(pEmptyBuf->filledBufSize <= pEmptyBuf->bufSize);

    statData = read(gVdecVdis_obj.fdRdData[curCh], pEmptyBuf->bufVirtAddr, pEmptyBuf->filledBufSize);

    if( feof(gVdecVdis_obj.fpRdHdr[curCh]) || statData != pEmptyBuf->filledBufSize)
    {
        #ifdef IPCBITS_OUT_HOST_DEBUG
        OSA_printf(" CH%d: Reached the end of file, rewind !!!", curCh);
        #endif
        clearerr(gVdecVdis_obj.fpRdHdr[curCh]);
        
        rewind(gVdecVdis_obj.fpRdHdr[curCh]);
        lseek(gVdecVdis_obj.fdRdData[curCh], 0, SEEK_SET);
        statHdr = fscanf(gVdecVdis_obj.fpRdHdr[curCh],"%d",&(pEmptyBuf->filledBufSize));
        
        OSA_assert(pEmptyBuf->filledBufSize <= pEmptyBuf->bufSize);
        statData = read(gVdecVdis_obj.fdRdData[curCh], pEmptyBuf->bufVirtAddr, pEmptyBuf->filledBufSize);
    }
}

#define VDEC_VDIS_FRAME_DURATION_MS (33)

static Void VdecVdis_setFrameTimeStamp(VCODEC_BITSBUF_S *pEmptyBuf)
{
    UInt64 curTimeStamp =
      gVdecVdis_config.frameCnt[pEmptyBuf->chnId] * VDEC_VDIS_FRAME_DURATION_MS;
    pEmptyBuf->lowerTimeStamp = (UInt32)(curTimeStamp & 0xFFFFFFFF);
    pEmptyBuf->upperTimeStamp = (UInt32)((curTimeStamp >> 32)& 0xFFFFFFFF);
    if (0 == gVdecVdis_config.frameCnt[pEmptyBuf->chnId])
    {
        UInt32 displayChId;

        Vdec_mapDec2DisplayChId(VDIS_DEV_HDMI,pEmptyBuf->chnId,&displayChId);
        Vdis_setFirstVidPTS(VDIS_DEV_HDMI,displayChId,curTimeStamp);
        Vdec_mapDec2DisplayChId(VDIS_DEV_HDCOMP,pEmptyBuf->chnId,&displayChId);
        Vdis_setFirstVidPTS(VDIS_DEV_HDCOMP,displayChId,curTimeStamp);
        Vdec_mapDec2DisplayChId(VDIS_DEV_SD,pEmptyBuf->chnId,&displayChId);
        Vdis_setFirstVidPTS(VDIS_DEV_SD,displayChId,curTimeStamp);
    }
    gVdecVdis_config.frameCnt[pEmptyBuf->chnId] += 1;
}

static Void VdecVdis_bitsRdReadData(VCODEC_BITSBUF_LIST_S  *emptyBufList,UInt32 resId)
{
    VCODEC_BITSBUF_S *pEmptyBuf;
    Int i;

    for (i = 0; i < emptyBufList->numBufs; i++)
    {
        pEmptyBuf = &emptyBufList->bitsBuf[i];
        VdecVdis_bitsRdFillEmptyBuf(pEmptyBuf);
        VdecVdis_setFrameTimeStamp(pEmptyBuf);
        pEmptyBuf->seqId = Demo_displayGetCurSeqId();
    }
}

static Void VdecVdis_bitsRdSendFullBitBufs( VCODEC_BITSBUF_LIST_S *fullBufList)
{
    if (fullBufList->numBufs)
    {
        Vdec_putBitstreamBuffer(fullBufList);
    }
}

static Void *VdecVdis_bitsRdSendFxn(Void * prm)
{
    VCODEC_BITSBUF_LIST_S emptyBufList;
    UInt32 resId;

    static Int printStatsInterval = 0;

    OSA_semWait(&gVdecVdis_obj.thrStartSem,OSA_TIMEOUT_FOREVER);
    while (FALSE == gVdecVdis_obj.thrExit)
    {
        OSA_waitMsecs(MCFW_IPCBITS_SENDFXN_PERIOD_MS);

        for (resId = 0; resId < gVdecVdis_config.numRes; resId++)
        {
            VdecVdis_bitsRdGetEmptyBitBufs(&emptyBufList,resId);

            VdecVdis_bitsRdReadData(&emptyBufList,resId);

            VdecVdis_bitsRdSendFullBitBufs(&emptyBufList);
        }
        #ifdef IPCBITS_OUT_HOST_DEBUG
        if ((printStatsInterval % MCFW_IPCBITS_INFO_PRINT_INTERVAL) == 0)
        {
            OSA_printf("MCFW_IPCBITS:%s:INFO: periodic print..",__func__);
        }
        #endif
        printStatsInterval++;
    }

    return NULL;
}

static int VdecVdis_getCfgArray(dictionary *ini, VdecVdis_Config *cfg)
{
    UInt32 totalChnlNum,i;
    char temp[256];
    char *name;

    totalChnlNum = iniparser_getnsec(ini);

    cfg->fileNum = totalChnlNum;
    
    printf(" *** a new param codec is needed for ini, if you not sure about this\n");
    printf(" *** please reference demo_ini/704x576_02_32CH.ini \n");
    printf(" *** H264:  codec = h264\n");
    printf(" *** MPEG4: codec = mpeg4\n");
    printf(" *** MJPEG: codec = mjpeg\n");

    printf(" *** Two new params numbuf & displaydelay has been added for ini, if not defaults are set\n");

    for (i = 0; i < totalChnlNum; i++)
    {
        name = iniparser_getsecname(ini,i);
        sprintf(temp,"%s:%s",name,MCFW_IPC_BITS_INI_FILE_PATH);
        name = iniparser_getstring(ini,temp,NULL);
        strcpy(cfg->fileInfo[i].path,name);

        name = iniparser_getsecname(ini,i);
        sprintf(temp,"%s:%s",name,MCFW_IPC_BITS_INI_FILE_WIDTH);
        cfg->fileInfo[i].width = iniparser_getint(ini,temp,0);

        name = iniparser_getsecname(ini,i);
        sprintf(temp,"%s:%s",name,MCFW_IPC_BITS_INI_FILE_HEIGHT);
        cfg->fileInfo[i].height = iniparser_getint(ini,temp,0);

        name = iniparser_getsecname(ini,i);
        sprintf(temp,"%s:%s",name,MCFW_IPC_BITS_INI_FILE_ENABLE);
        cfg->fileInfo[i].enable = iniparser_getboolean(ini,temp,1);

        name = iniparser_getsecname(ini,i);
        sprintf(temp,"%s:%s",name,MCFW_IPC_BITS_INI_FILE_CODEC);
        name = iniparser_getstring(ini,temp,NULL);
        if(name != NULL )
            strcpy(cfg->fileInfo[i].codec,name);

        name = iniparser_getsecname(ini,i);
        sprintf(temp,"%s:%s",name,MCFW_IPC_BITS_INI_FILE_NUMBUF);
        cfg->fileInfo[i].numbuf = iniparser_getint(ini,temp,0);

        name = iniparser_getsecname(ini,i);
        sprintf(temp,"%s:%s",name,MCFW_IPC_BITS_INI_FILE_DISPLAYDELAY);
        cfg->fileInfo[i].displaydelay = iniparser_getint(ini,temp,0);

    }

    return 0;
}

static int VdecVdis_bitsRdParseIniFile(void)
{
    dictionary * ini = NULL ;
    char        iniPath[MCFW_IPC_BITS_MAX_FULL_PATH_FILENAME_LENGTH];
#if MCFW_IPC_BITS_GENERATE_INI_AUTOMATIC
    char        iniGenerate[MCFW_IPC_BITS_MAX_FULL_PATH_FILENAME_LENGTH];
#endif

#if MCFW_IPC_BITS_GENERATE_INI_AUTOMATIC
    printf(" \n");
    printf(" *** Do you want Generate ini files ? (y/n) : ");
    fscanf(stdin, "%255s", iniGenerate);
    printf(" \n");

    if ((iniGenerate[0] == 'Y') ||(iniGenerate[0] == 'y'))
    {
        int fileNum, inputWidth, inputHeight,totalChnlNum = 0;
        int i;
        FILE *file = NULL;
        char temp[MCFW_IPC_BITS_MAX_FULL_PATH_FILENAME_LENGTH];


        while(NULL == file)
        {
            printf(" \n");
            printf(" Enter the .ini filename to create with full path : ");

            fflush(stdin);
            fscanf(stdin, "%255s", iniPath);

            if((file = fopen(iniPath, "w+")) == NULL)
            {
                printf(" \n");
                printf(" ERROR: Cannot open Output file [%s] for writing !!! \n",iniPath);
            }
        }

        strcpy(temp,iniPath);
        dirname(temp);

        printf(" \n");
        do{
            printf(" *** Enter input width  : ");
            fscanf(stdin, "%d", &inputWidth);
            printf(" *** Enter input height : ");
            fscanf(stdin, "%d", &inputHeight);
            printf(" *** How many channels are there ? (1..32) : ");
            fscanf(stdin, "%d", &fileNum);

            for (i = 0; i < fileNum; i++)
            {
                fprintf(file,"[%s%02d]\n",MCFW_IPC_BITS_INI_FILE_NO,totalChnlNum);
                fprintf(file,"path      = %s/chnl%02d.%s\n",temp,totalChnlNum,"h264");
                fprintf(file,"width     = %d\n",inputWidth);
                fprintf(file,"height    = %d\n",inputHeight);
                fprintf(file,"enable    = %d\n\n",1);
                totalChnlNum ++;
            }

            printf(" *** Do you have more channels ? (y/n) : ");
            fscanf(stdin, "%255s", iniGenerate);

            printf(" \n");

        } while((iniGenerate[0] == 'Y') || (iniGenerate[0] == 'y'));

        fclose(file);

        printf(" \n");
        printf(" *** IMPORTANT *** \n");
        printf(" \n");
        printf(" Ini file [%s] generated. \n"
               " Please open the file and modify the location and \n"
               " filename of the actual input bitstream file before \n"
               " proceeding. \n",
                iniPath
            );
        printf(" \n");
    }
#endif

    do
    {
        printf(" \n");
        printf(" Sample ini files available in ./demo_ini folder. \n");
        printf(" Enter the .ini filename with full path : ");

        fflush(stdin);
        fscanf(stdin, "%255s", iniPath);
        ini = iniparser_load(iniPath);

        if(ini==NULL)
        {
            printf(" \n");
            printf(" ERROR: [%s] ini file could not be accessed !!!\n", iniPath);
        }
        else
        {
            VdecVdis_getCfgArray(ini, &gVdecVdis_config);
            iniparser_freedict(ini);

            if(gVdecVdis_config.fileNum==0)
            {
                printf(" \n");
                printf(" ERROR: [%s] ini file does not have any input bitstream file information !!! \n"
                       "        Please try again !!! \n", iniPath);
            }
            else
            {
                /* sucessfully pasred the file, exit loop */
                break;
            }
        }
    } while (1);


    return 0 ;
}

static Bool VdecVdis_bitsRdFileExists( const char* filename )
{

    if(access( filename, F_OK ) == 0 ){

        struct stat status;
        stat( filename, &status );

        return (status.st_mode & S_IFREG) != 0;
    }

    return FALSE;
}
void loop_test_open(UInt32 fileId, UInt32 fileIndex)
{
    static Uint32 loopNum[64] = {0};
    char   fileNameHdr[MCFW_IPC_BITS_MAX_FULL_PATH_FILENAME_LENGTH];
    VdecVdis_FileInfo *pFileInfo;

    OSA_assert (fileIndex < gVdecVdis_config.fileNum);
    loopNum[fileId] = fileIndex;

    close(gVdecVdis_obj.fdRdData[fileId]);
    
    pFileInfo = &gVdecVdis_config.fileInfo[fileIndex];
    if(strcmp(pFileInfo->codec,"h264") == 0)
        Demo_generateH264HdrFile(pFileInfo->path);
    else if(strcmp(pFileInfo->codec,"mjpeg") == 0)
        Demo_generateMjpgHdrFile(pFileInfo->path);
    else if(strcmp(pFileInfo->codec,"mpeg4") == 0)
        Demo_generateMpeg4HdrFile(pFileInfo->path);

    bzero(fileNameHdr,MCFW_IPC_BITS_MAX_FULL_PATH_FILENAME_LENGTH);
    strcat(fileNameHdr, pFileInfo->path);
    strcat(fileNameHdr,".hdr\0");
    
    if (FALSE == VdecVdis_bitsRdFileExists(fileNameHdr))
    {
        printf(" %d: ERROR: Header File [%s] for [%s] not found !!! \n",
            fileId,
            fileNameHdr,
            pFileInfo->path
        );
    }

    fclose(gVdecVdis_obj.fpRdHdr[fileId]);
    gVdecVdis_obj.fpRdHdr[fileId] = fopen(fileNameHdr,"r");
    OSA_assert(gVdecVdis_obj.fpRdHdr[fileId] != NULL);
    gVdecVdis_obj.fdRdData[fileId] = open(pFileInfo->path, O_RDONLY);
    if(gVdecVdis_obj.fdRdData[fileId]<= 0)
        perror("---------!!!!!!!!!1-----");
}


static Int   VdecVdis_bitsRdOpenFileHandles(Bool headGenerate)
{
    UInt32 status = OSA_SOK;
    UInt32 fileId, resId;
    Bool   found = FALSE;
    char   fileNameHdr[MCFW_IPC_BITS_MAX_FULL_PATH_FILENAME_LENGTH];

    VdecVdis_FileInfo *pFileInfo;
    
    gVdecVdis_config.numRes = 0;
    OSA_assert(gVdecVdis_config.fileNum <= MCFW_IPC_BITS_MAX_FILE_NUM_IN_INI);
    
    /* assuming valid files = fileNum */
    for (fileId = 0; fileId < gVdecVdis_config.fileNum; fileId++)
    {
        pFileInfo = &gVdecVdis_config.fileInfo[fileId];

        printf(" %d: Opening file [%s] of %d x %d  Codec: %s... \n",
            fileId,
            pFileInfo->path,
            pFileInfo->width,
            pFileInfo->height,
            pFileInfo->codec
            );

        gVdecVdis_obj.fdRdData[fileId]   = -1;
        gVdecVdis_obj.fpRdHdr[fileId]    = NULL;

        if(!pFileInfo->enable)
        {
            printf(" %d: WARNING: File not ENABLED in .ini file ... ignoring this flag !!! \n", fileId);
        }

        if(pFileInfo->width==0
            ||
            pFileInfo->height==0
        )
        {
            printf(" %d: ERROR: Invalid width x height specified !!! \n", fileId);

            status = OSA_EFAIL;

            continue;
        }

        if(strlen(pFileInfo->codec) == 0)
        {
            printf(" %d: WARNING: Codec type not set. Will be considered as H264 ... ignoring this flag !!! \n", fileId);
            strcpy(pFileInfo->codec,"h264");
        }

        if(pFileInfo->displaydelay == 0)
        {
            printf(" %d: WARNING: Either the displaydelay was not set or Default value was set as 0\n", fileId);
        }

        if(pFileInfo->numbuf == 0)
        {
            printf(" %d: WARNING: Either the Num of output buffers not set or Default value was set as zero\n", fileId);
        }

        if (FALSE == VdecVdis_bitsRdFileExists(pFileInfo->path))
        {
            printf(" %d: ERROR: File [%s] not found !!! \n",
                fileId,
                pFileInfo->path
            );

            status = OSA_EFAIL;

            continue;
        }
        else
        {
            status = OSA_SOK;
        }


        if (headGenerate)
        {
            if(strcmp(pFileInfo->codec,"h264") == 0)
                Demo_generateH264HdrFile(pFileInfo->path);
            else if(strcmp(pFileInfo->codec,"mjpeg") == 0)
                Demo_generateMjpgHdrFile(pFileInfo->path);
            else if(strcmp(pFileInfo->codec,"mpeg4") == 0)
                Demo_generateMpeg4HdrFile(pFileInfo->path);
        }

        gVdecVdis_obj.fdRdData[fileId] = open(pFileInfo->path, O_RDONLY);
        OSA_assert(gVdecVdis_obj.fdRdData[fileId] >  0 );

        
        bzero(fileNameHdr,MCFW_IPC_BITS_MAX_FULL_PATH_FILENAME_LENGTH);
        strcat(fileNameHdr, pFileInfo->path);
        strcat(fileNameHdr,".hdr\0");

        if (FALSE == VdecVdis_bitsRdFileExists(fileNameHdr))
        {
            printf(" %d: ERROR: Header File [%s] for [%s] not found !!! \n",
                fileId,
                fileNameHdr,
                pFileInfo->path
            );

            status = OSA_EFAIL;

            continue;
        }
        else
        {
            status = OSA_SOK;
        }


        gVdecVdis_obj.fpRdHdr[fileId] = fopen(fileNameHdr,"r");
        OSA_assert(gVdecVdis_obj.fpRdHdr[fileId] != NULL);
        
        // find same resolution in the array
        found = FALSE;

        for(resId=0;resId<gVdecVdis_config.numRes;resId++)
        {
            if ((pFileInfo->width == gVdecVdis_config.res[resId].width) &&
                (pFileInfo->height == gVdecVdis_config.res[resId].height))
            {
                found = TRUE;
                break;
            }
        }

        if (!found)
        {
            gVdecVdis_config.res[resId].width  = pFileInfo->width;
            gVdecVdis_config.res[resId].height = pFileInfo->height;

            gVdecVdis_config.numRes++;
        }

        gVdecVdis_config.numChnlInRes[resId] ++;

        gVdecVdis_config.resToChnl
                [
                resId
                ]
                [
            gVdecVdis_config.numChnlInRes[resId] - 1
                ]
                    = fileId;
    }

    if( status != OSA_SOK )
    {
        printf(" \n");
        printf(" !!! There were some errors in accessing the one of the input files or width x height of the bitstreams. !!!\n");
        printf(" !!! Please check, Continuing with the rest!!! \n");
        printf(" \n");
    }
    for ( resId = 0; resId < gVdecVdis_config.numRes; resId++)
    {
        printf ("gVdecVdis_config.numRes : %d gVdecVdis_config.numChnlInRes[%d] : %d\n",
              gVdecVdis_config.numRes, resId, gVdecVdis_config.numChnlInRes[resId]);
    }

    printf(" File open ... DONE !!!\n");
    printf(" \n");

    return status;
}

static Int32 VdecVdis_bitsRdGetFileInfoFromIniFile()
{
    char headerGen[20];
    Bool hdrGenerate = TRUE;

    VdecVdis_bitsRdParseIniFile();

    printf(" \n");
    printf(" *** Generate header files *** \n");
    printf(" \n");
    printf(" This will take very long time if the file is large.\n");
    printf(" - For first time you MUST select YES. \n");
    printf(" - If you already have the header file on disk, you can select NO. \n");
    printf(" \n");
    printf(" Do you want to generate header ? (YES - y, NO - n) : ");

    fscanf(stdin, "%20s", headerGen);

    if ((headerGen[0] == 'N') || (headerGen[0] == 'n') )
        hdrGenerate = FALSE;

    printf(" \n");

    VdecVdis_bitsRdOpenFileHandles(hdrGenerate);
    return OSA_SOK;
}


static Int32 VdecVdis_bitsRdResetFileHandles()
{
    Int i;

    for (i = 0; i < MCFW_IPC_BITS_MAX_FILE_NUM_IN_INI; i++)
    {
        if(gVdecVdis_obj.fpRdHdr[i]!=NULL)
        {
            fclose(gVdecVdis_obj.fpRdHdr[i]);
            gVdecVdis_obj.fpRdHdr[i] = NULL;
        }
        if(gVdecVdis_obj.fdRdData[i] > 0)
        {
            close(gVdecVdis_obj.fdRdData[i]);
            gVdecVdis_obj.fdRdData[i] = -1;
        }
    }
    return OSA_SOK;
}

static Void VdecVdis_bitsRdInitThrObj()
{
    int status;

    gVdecVdis_obj.thrExit = FALSE;
    status = OSA_semCreate(&gVdecVdis_obj.thrStartSem,1,0);
    OSA_assert(status==OSA_SOK);

    status = OSA_thrCreate(&gVdecVdis_obj.thrHandle,
            VdecVdis_bitsRdSendFxn,
            MCFW_IPCBITS_SENDFXN_TSK_PRI,
            MCFW_IPCBITS_SENDFXN_TSK_STACK_SIZE,
            &gVdecVdis_obj);

    OSA_assert(status==OSA_SOK);
}

static Void VdecVdis_bitsRdDeInitThrObj()
{
    gVdecVdis_obj.thrExit = TRUE;
    OSA_thrDelete(&gVdecVdis_obj.thrHandle);
    OSA_semDelete(&gVdecVdis_obj.thrStartSem);
}

Int32 VdecVdis_bitsRdInit()
{
    memset(&gVdecVdis_obj   , 0, sizeof(gVdecVdis_obj));
    memset(&gVdecVdis_config, 0, sizeof(gVdecVdis_config));
    gVdecVdis_config.gDemo_TrickPlayMode.speed = 1;
    gVdecVdis_obj.enable64BitTimeStamp = TRUE;
    gVdecVdis_obj.audioSyncCtrl.gotAudioTS = FALSE;

    VdecVdis_bitsRdResetFileHandles();

    VdecVdis_bitsRdGetFileInfoFromIniFile();

    VdecVdis_bitsRdInitThrObj();

    return OSA_SOK;
}

Void VdecVdis_bitsRdStart()
{
    OSA_semSignal(&gVdecVdis_obj.thrStartSem);
}

Void VdecVdis_bitsRdStop()
{
    gVdecVdis_obj.thrExit = TRUE;

    /* wait for the thread to exit */
    OSA_waitMsecs(1000);
}

Int32 VdecVdis_bitsRdExit()
{
    VdecVdis_bitsRdDeInitThrObj();
    VdecVdis_bitsRdResetFileHandles();

    return OSA_SOK;
}


Void  VdecVdis_switchInputFile(UInt32 chId, UInt32 FileIndex)
{
    gVdecVdis_obj.FileIndex = FileIndex;
    gVdecVdis_obj.chId = chId;
    gVdecVdis_obj.switchInputFile = TRUE;
}


