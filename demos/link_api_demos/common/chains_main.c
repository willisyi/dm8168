/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

//#include <demos/link_api_demos/common/chains.h>
#include "chains.h"
#include "ti_vdis_common_def.h"
#include <mcfw/interfaces/common_def/ti_vsys_common_def.h>
//20130422
/*
#include <pthread.h>
#include <semaphore.h>
#include <sys/ipc.h>
*/
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
#                ifdef TI816X_CZ
#                    define SYSTEM_PLATFORM_BOARD          SYSTEM_PLATFORM_BOARD_CZ
#                else
#                    ifdef TI816X_ETV
#                        define SYSTEM_PLATFORM_BOARD      SYSTEM_PLATFORM_BOARD_ETV
#                    else
#                        error "Unknown Board Type"
#                    endif
#                endif
#            endif
#        endif
#    endif
#endif

#define MAX_INPUT_STR_SIZE  (80)

//Chains_Ui_Config gChains_Ui_Config;
//input_event LocalStorage_enable.value=0;
//input_event LocalStorage_disable.value=0;



UInt32 gChains_enabledProcs[] = {
    SYSTEM_LINK_ID_M3VPSS,
    SYSTEM_LINK_ID_M3VIDEO,
    SYSTEM_LINK_ID_DSP,
};
/*
*  add by lyy
*  尚未完成，根据需求进行后续补充。
*  不同编码格式可根据需要添加，编码库支持常见格式的编码
*/

void showUsage()
{
    OSA_printf("\n");
    OSA_printf(" Video Capture server --- XTE 2012 \n");
    OSA_printf("\n");
    OSA_printf(" Usage: ./dvr_rdk_demo_link_api.out  <option 1> <option 2> ...\n");
    OSA_printf(" Following options are supported,\n");
	OSA_printf("\n");
	OSA_printf(" Resolution options - only one should be specified at a time,\n");
    OSA_printf("1080P60:    	 1920x1080@60fps capture mode\n");
	OSA_printf("1080P50:    	 1920x1080@50fps capture mode\n");
    OSA_printf("1080P30:    	 1920x1080@30fps capture mode\n");
	OSA_printf("1080P25:    	 1920x1080@25fps capture mode\n");
    OSA_printf("1080P24:    	 1920x1080@24fps capture mode\n");
	OSA_printf("1080I60:     	 1920x1080@50fps capture mode\n");	
	OSA_printf("1080I50:    	 1920x1080@50fps capture mode\n");	
	OSA_printf("720P60:          1280x720@50fps capture mode\n");	
	OSA_printf("720P30:          1280x720@30fps capture mode\n");
	OSA_printf("1080P60_720P60\n: 1920x1080@60fps +1280x720@60fps ");
	OSA_printf("\n");
	OSA_printf("Feature options\n");
	OSA_printf("RTSP:            RTSP network streaming is ENABLED\n");
	OSA_printf("\n");
	OSA_printf("Codec options:\n");
	OSA_printf("H264  <bitrate> <rcType> <Level>: H264 compression is ENABLED \n");
	OSA_printf("      with bitrate(kb)= <bitrate>, rcType = CBR or VBR, level = HIGH or MAIN or BASE\n");
    OSA_printf(" MJPEG <qvalue>               : MJPEG compression is ENABLED \n");
    OSA_printf("      with quality = <qvalue>  range[2,97]\n");
	OSA_printf("\n");
	OSA_printf(" Example,\n");
	OSA_printf("Example 1. #./dvr_rdk_demo_link_api.out 1080P60 RTSP H264  8000 VBR HIGH \n");
	OSA_printf("\n");
	
}

/*
* parse args of main
* input:argc ,argv
*output:
*请根据参数需要进行后续完善
*多路部分，从定义到调用都要重新处理，此处仅为临时框架
 * 关于mpeg4的部分需要进行补充
*/
void ParseArgs(int argc,char **argv)
{
    int i=0,ch=0;

    for(i=1;i<argc;i++)
    {
        if(strcmp( argv[i],"1080P60") == 0)
        {
			gChains_ctrl.displayRes[0]= VSYS_STD_1080P_60;
			gChains_ctrl.channelConf[0].frameRate=60;
			gChains_ctrl.channelConf[0].encFrameRate=60;
			//gChains_ctrl.channelConf[0].intraFrameInterval=60;
        }
		else if( strcmp( argv[i] , "1080P50") == 0 )
		{
			gChains_ctrl.displayRes[0] = VSYS_STD_1080P_50;
			gChains_ctrl.channelConf[0].frameRate=50;
			gChains_ctrl.channelConf[0].encFrameRate=50;
		}
		else if( strcmp( argv[i], "1080P30" ) == 0 )
		{
			gChains_ctrl.displayRes[0] = VSYS_STD_1080P_30;
			gChains_ctrl.channelConf[0].encFrameRate=30;
			gChains_ctrl.channelConf[0].frameRate=30;
		}
		else if( strcmp( argv[i], "1080P25" ) == 0 )
		{
			gChains_ctrl.displayRes[0] = VSYS_STD_1080P_25;
                    gChains_ctrl.channelConf[0].encFrameRate=25;
			gChains_ctrl.channelConf[0].frameRate=25;
		}
		else if( strcmp( argv[i], "1080P24" ) == 0 )
		{
			gChains_ctrl.displayRes[0]= VSYS_STD_1080P_24;
			gChains_ctrl.channelConf[0].encFrameRate=24;
			gChains_ctrl.channelConf[0].frameRate=24;
		}
		else if( strcmp( argv[i], "1080I60" ) == 0 )
		{
		    gChains_ctrl.displayRes[0]= VSYS_STD_1080I_60;
                 gChains_ctrl.channelConf[0].encFrameRate=60;
		   gChains_ctrl.channelConf[0].frameRate=60;
		}
		else if( strcmp( argv[i], "1080I50" ) == 0 )
		{
			gChains_ctrl.displayRes[0]= VSYS_STD_1080I_50;
			gChains_ctrl.channelConf[0].encFrameRate=50;
			gChains_ctrl.channelConf[0].frameRate=50;
		}
		else if( strcmp( argv[i], "720P60" ) == 0 )
		{
			gChains_ctrl.displayRes[0]= VSYS_STD_720P_60;
			gChains_ctrl.channelConf[0].encFrameRate=60;
			gChains_ctrl.channelConf[0].frameRate=60;
		}
		else if( strcmp( argv[i], "720P30" ) == 0 )
		{
			gChains_ctrl.displayRes[0]= VSYS_STD_720P_30;
			gChains_ctrl.channelConf[0].encFrameRate=30;
			gChains_ctrl.channelConf[0].frameRate=30;
		}
		else if( strcmp( argv[i] , "480P60") == 0 )
		{
			gChains_ctrl.displayRes[0] = VSYS_STD_480P;
			gChains_ctrl.channelConf[0].encFrameRate=60;
			gChains_ctrl.channelConf[0].frameRate=60;
		}
		else if( strcmp( argv[i], "1080P60P_720P60" ) == 0 )
		{
			gChains_ctrl.channelNum= 2;
			gChains_ctrl.displayRes[0]= VSYS_STD_1080P_60;
			gChains_ctrl.channelConf[0].encFrameRate=60;
			gChains_ctrl.channelConf[0].frameRate=60;
			gChains_ctrl.displayRes[1] = VSYS_STD_720P_60;
			gChains_ctrl.channelConf[1].encFrameRate=60;
			gChains_ctrl.channelConf[1].frameRate=60;
		}
		else if( strcmp( argv[i],"RTSP") ==0 )
		{
			gChains_ctrl.channelConf[0].encFlag = TRUE;
            		gChains_ctrl.channelConf[0].enableTcp    = FALSE;
            		gChains_ctrl.channelConf[0].enableRtsp   = TRUE;
            		gChains_ctrl.channelConf[0].enableServer = TRUE;
            		gChains_ctrl.channelConf[0].enableClient = TRUE;
		}
		/*****************add by Sue **************/
		else if( strcmp( argv[i],"LocalStorage") ==0 )
		{
		//printf("\nLocalStorage...\n");
	     gChains_ctrl.channelConf[0].encFlag = TRUE;
            gChains_ctrl.channelConf[0].enableTcp    = FALSE;
            gChains_ctrl.channelConf[0].enableRtsp   = FALSE;
            gChains_ctrl.channelConf[0].enableServer = TRUE;
            gChains_ctrl.channelConf[0].enableClient = TRUE;
		}
		else if( strcmp( argv[i],"AUDIO") ==0 )
		{
		//printf("\nAUDIO...\n");
	         gChains_ctrl.channelConf[0].audioEnable= TRUE;
                gChains_ctrl.channelConf[0].audioSample_rate= atoi(argv[i+1]);
			if(strcmp(argv[i+2], "G711")==0)
			{
      	                gChains_ctrl.channelConf[0].audioCodeType =VSYS_AUD_CODEC_G711;
		             if(strcmp(argv[i+3], "aLaw")==0)
			     gChains_ctrl.channelConf[0].G711uLaw= FALSE;
				else if(strcmp(argv[i+3], "uLaw")==0)
			     gChains_ctrl.channelConf[0].G711uLaw= TRUE;

			}
                 else if(strcmp(argv[i+2], "AAC")==0)
                 {
      			gChains_ctrl.channelConf[0].audioCodeType =VSYS_AUD_CODEC_AAC;
      			gChains_ctrl.channelConf[0].audioBitRate= atoi(argv[i+3]);
                  }
                 else if(strcmp(argv[i+2], "G726")==0)
	            {
			 gChains_ctrl.channelConf[0].audioCodeType =VSYS_AUD_CODEC_G726;
			 gChains_ctrl.channelConf[0].audioBitRate= atoi(argv[i+3]);
		     }
	          else if(strcmp(argv[i+2], "G729")==0)
	            {
			 gChains_ctrl.channelConf[0].audioCodeType =VSYS_AUD_CODEC_G729;
		      }
	       }
		else if( strcmp( argv[i],"OSD_ON") ==0 )
		{
		    gChains_ctrl.enableOsdAlgLink =TRUE;
		    gChains_ctrl.channelConf[0].enableOsd=TRUE;
		    if(strcmp( argv[i+1],"font16")==0)
		    {
		        gChains_ctrl.channelConf[0].OSDfont=16;
		    }
		    else if(strcmp( argv[i+1],"font32")==0)
		    {
		        gChains_ctrl.channelConf[0].OSDfont=32;
		    }
		    gChains_ctrl.channelConf[0].OSDStartX=atoi(argv[i+2]);
		    gChains_ctrl.channelConf[0].OSDStartY=atoi(argv[i+3]);
		    i=i+3;
		}
		/******************************************/
		else if( strcmp( argv[i],"H264") == 0)
		{
		    gChains_ctrl.channelConf[0].bitRate= (atoi(argv[i+1]))*1024;
		    if( strcmp( argv[i+3],"HIGH") == 0)
		    {
		        gChains_ctrl.channelConf[0].encFormat=IVIDEO_H264HP;
                     gChains_ctrl.channelConf[0].encProfile=IH264_HIGH_PROFILE;
			}
			else if ( strcmp( argv[i+3],"MAIN") == 0)
			{
				gChains_ctrl.channelConf[0].encFormat=IVIDEO_H264MP;
				gChains_ctrl.channelConf[0].encProfile=IH264_MAIN_PROFILE;
			}
			else if ( strcmp( argv[i+3],"BASE") ==0 )
			{
				gChains_ctrl.channelConf[0].encFormat = IVIDEO_H264BP;
                          gChains_ctrl.channelConf[0].encProfile= IH264_BASELINE_PROFILE;
			}
			
			if( strcmp(argv[i+2],"CBR")==0 )
				gChains_ctrl.channelConf[0].rateCtrl=TRUE;
			else if (strcmp(argv[i+2],"VBR")==0 )
				gChains_ctrl.channelConf[0].rateCtrl=FALSE;

			i=i+3;
		}
		else if ( strcmp(argv[i],"MJPEG") == 0)
		{
		    gChains_ctrl.channelConf[ch].encFormat=IVIDEO_MJPEG;
		    gChains_ctrl.channelConf[ch].encProfile= atoi(argv[i+1]);
			i=i+1;
		}
			
    }
	
}
//added by guo
//for chains_doubleChCapScEncSend()
void Chains_doubleChParseArgs(int argc, char * * argv)
{
	int i;
	int channel,ch;
	gChains_ctrl.channelNum = 4;
	 for (i=0; i<gChains_ctrl.channelNum; i++) 
	 {
            gChains_ctrl.channelConf[i].encFlag      = TRUE;
            gChains_ctrl.channelConf[i].enableTcp    = FALSE;
            gChains_ctrl.channelConf[i].enableRtsp   = TRUE;
            gChains_ctrl.channelConf[i].enableServer = TRUE;
            gChains_ctrl.channelConf[i].enableClient = FALSE;
       }
	 //originally the belows are in chains_run()
	 channel=0;
	 gChains_ctrl.displayRes[channel] = SYSTEM_STD_AUTO_DETECT;//set autodetect division
	 Vsys_getResSize(gChains_ctrl.displayRes[channel], &gChains_ctrl.channelConf[channel].width, &gChains_ctrl.channelConf[channel].height);
   	 Vsys_getResRate(gChains_ctrl.displayRes[channel], &gChains_ctrl.channelConf[channel].frameRate);
        if(gChains_ctrl.channelConf[channel].encFlag == TRUE){
	  	gChains_ctrl.channelConf[channel].encFormat = IVIDEO_H264HP;
        	gChains_ctrl.channelConf[channel].encProfile = IH264_HIGH_PROFILE;
		gChains_ctrl.channelConf[channel].encFrameRate = 60;
		gChains_ctrl.channelConf[channel].intraFrameInterval = 60;//for h.264
		gChains_ctrl.channelConf[channel].rateCtrl = 0;//vbr
		gChains_ctrl.channelConf[channel].bitRate =8000 * 1000;
        }
	ParseArgs(argc,argv);
	 for (ch=1; ch<gChains_ctrl.channelNum; ch++) {
        	gChains_ctrl.displayRes[ch] = gChains_ctrl.displayRes[ch-1];
        	memcpy(&gChains_ctrl.channelConf[ch], &gChains_ctrl.channelConf[ch-1], sizeof(gChains_ctrl.channelConf[ch]));
        }
	//and then you can call chains_run(chains_doubleChCapScEncSend);
}

Void Chains_setDefaultCfg()
{
    UInt32 displayResDefault[SYSTEM_DC_MAX_VENC] =
        {VSYS_STD_1080P_60,   //SYSTEM_DC_VENC_HDMI,
         VSYS_STD_1080P_60,    //SYSTEM_DC_VENC_HDCOMP,
         VSYS_STD_1080P_60,    //SYSTEM_DC_VENC_DVO2
         VSYS_STD_NTSC        //SYSTEM_DC_VENC_SD,
        };
	//gChains_Ui_Config.ch=1;
    gChains_ctrl.enableNsfLink = TRUE;
    gChains_ctrl.enableOsdAlgLink = FALSE;
    gChains_ctrl.enableVidFrameExport = FALSE;
    gChains_ctrl.bypassNsf = FALSE;
    gChains_ctrl.channelNum=1;
    memcpy(gChains_ctrl.displayRes,displayResDefault,sizeof(gChains_ctrl.displayRes));
  //audio defaultCfg
    gChains_ctrl.channelConf[0].audioEnable= FALSE;
    gChains_ctrl.channelConf[0].audioCodeType=VSYS_AUD_CODEC_G711;
    gChains_ctrl.channelConf[0].audioSample_rate=8000;
    gChains_ctrl.channelConf[0].G711uLaw=FALSE;
    gChains_ctrl.channelConf[0].enableOsd=FALSE;
}

char *gChains_cpuName[SYSTEM_PLATFORM_CPU_REV_MAX] = {
    "ES1.0",
    "ES1.1",
    "ES2.0",
    "ES2.1",
    "UNKNOWN",
};

char *gChains_boardName[SYSTEM_PLATFORM_BOARD_MAX] = {
    "UNKNOWN",
    "4x TVP5158 VS",
    "2x SII9135, 1x TVP7002 VC",
    "2x SIL1161A, 2x TVP7002 Catalog",
    "1x SII9233A, 1x SII9134 CZ",
    "2x SII9233A, 1x SII9022A ETV",
    "2x SIL1161A, 2x TVP7002 DVR",
};

char *gChains_boardRev[SYSTEM_PLATFORM_BOARD_REV_MAX] = {
    "UNKNOWN",
    "REV A",
    "REV B",
    "REV C",
    "DVR"
};
void audio_init()
{
    system("amixer cset name='PGA Capture Volume' 90,90");
    system("amixer sset 'Right PGA Mixer Mic3L' on");
    system("amixer sset 'Right PGA Mixer Mic3R' on");
    system("amixer sset 'Left PGA Mixer Mic3L' on");
    system("amixer sset 'Left PGA Mixer Mic3R' on");
    system("amixer sset 'Left PGA Mixer Line1L' off");
    system("amixer sset 'Right PGA Mixer Line1R' off");
    system("amixer sset name='Left PGA Mixer Line1R' off");
    system("amixer sset name='Right PGA Mixer Line1L' off");
}
Int32 Chains_detectBoard()
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
    printf(" %u: CHAINS  : CPU Revision [%s] !!! \r\n",
        OSA_getCurTimeInMsec(), gChains_cpuName[cpuRev]);

    /* Detect board */
    boardId = platformInfo.boardId;
    if (boardId >= SYSTEM_PLATFORM_BOARD_MAX)
    {
        boardId = SYSTEM_PLATFORM_BOARD_UNKNOWN;
    }
    printf(" %u: CHAINS  : Detected [%s] Board !!! \r\n",
        OSA_getCurTimeInMsec(), gChains_boardName[boardId]);

    /* Get base board revision */
    boardRev = platformInfo.baseBoardRev;
    if (boardRev >= SYSTEM_PLATFORM_BOARD_REV_MAX)
    {
        boardRev = SYSTEM_PLATFORM_BOARD_REV_UNKNOWN;
    }
    printf(" %u: CHAINS  : Base Board Revision [%s] !!! \r\n",
        OSA_getCurTimeInMsec(), gChains_boardRev[boardRev]);

    if (boardId != SYSTEM_PLATFORM_BOARD_UNKNOWN)
    {
        /* Get daughter card revision */
        boardRev = platformInfo.dcBoardRev;
        if (boardRev >= SYSTEM_PLATFORM_BOARD_REV_MAX)
        {
            boardRev = SYSTEM_PLATFORM_BOARD_REV_UNKNOWN;
        }
        printf(" %u: CHAINS  : Daughter Card Revision [%s] !!! \r\n",
            OSA_getCurTimeInMsec(), gChains_boardRev[boardRev]);
    }

    return 0;
}

UInt32 Chains_standardChoose()
{
    char opMode[128];
    OSA_printf("Select video standard:"
            "\r\n 1: 1920x1080P60"
            "\r\n 2: 1920x1080P50"
            "\r\n 3: 1920x1080P30"
            "\r\n 4: 1920x1080P25"
            "\r\n 5: 1920x1080P24"
            "\r\n 6: 1920x1080I60"
            "\r\n 7: 1920x1080I50"
            "\r\n 8: 1280x720P60"
            "\r\n 9: 1280x720P50"
            "\r\n 10: 1280x720P30"
            "\r\n 11: 1280x720P25"
            "\r\n 12: 1280x720P24"
            "\r\n 13: 720x576P50"
            "\r\n 14: 720x576I50"
            "\r\n 15: 720x480P60"
            "\r\n 16: 720x480I60"
            "\r\n 17: WUXGA(1920x1200)60"
            "\r\n 18: UXGA+(1600*1200)60"
            "\r\n 19: SXGA+(1400*1050)60"
            "\r\n 20: WXGA(1360*768)60"
            "\r\n 21: SXGA(1280*1024)60"
            "\r\n 22: WXGA(1280*768)60"
            "\r\n 23: XGA(1024*768)60"
            "\r\n 24: SVGA(800*600)60"
            "\r\n 25: VGA(640*480)60"
            "\r\n");
    fflush(stdin);
    fscanf(stdin,"%s",&(opMode[0]));
    UInt32 idx = atoi(opMode);
    VSYS_VIDEO_STANDARD_E standard;
    switch (idx) {
        default:
        case 1:
            standard = VSYS_STD_1080P_60;
            break;
        case 2:
            standard = VSYS_STD_1080P_50;
            break;
        case 3:
            standard = VSYS_STD_1080P_30;
            break;
        case 4:
            standard = VSYS_STD_1080P_25;
            break;
        case 5:
            standard = VSYS_STD_1080P_24;
            break;
        case 6:
            standard = VSYS_STD_1080I_60;
            break;
        case 7:
            standard = VSYS_STD_1080I_50;
            break;
        case 8:
            standard = VSYS_STD_720P_60;
            break;
        case 9:
            standard = VSYS_STD_720P_50;
            break;
        case 10:
            standard = VSYS_STD_720P_30;
            break;
        case 11:
            standard = VSYS_STD_720P_25;
            break;
        case 12:
            standard = VSYS_STD_720P_24;
            break;
        case 13:
            standard = VSYS_STD_576P;
            break;
        case 14:
            standard = VSYS_STD_576I;
            break;
        case 15:
            standard = VSYS_STD_480P;
            break;
        case 16:
            standard = VSYS_STD_480I;
            break;
        case 17:
            standard = VSYS_STD_WUXGA_60;
            break;
        case 18:
            standard = VSYS_STD_UXGA_60;
            break;
        case 19:
            standard = VSYS_STD_SXGAP_60;
            break;
        case 20:
            standard = VSYS_STD_1360_768_60;
            break;
        case 21:
            standard = VSYS_STD_SXGA_60;
            break;
        case 22:
            standard = VSYS_STD_WXGA_60;
            break;
        case 23:
            standard = VSYS_STD_XGA_60;
            break;
        case 24:
            standard = VSYS_STD_SVGA_60;
            break;
        case 25:
            standard = VSYS_STD_VGA_60;
            break;
        case 26:
            standard = VSYS_STD_CIF;
            break;
    }
    return standard;
}

Void Chains_menuChoose(UInt32 channel)
{
    char opMode[128];
    OSA_printf("Set auto detecting video standard mode (y/n): ");
    fflush(stdin);
    fscanf(stdin,"%s",&(opMode[0]));
    if (opMode[0] == 'y') {
        gChains_ctrl.displayRes[channel] = SYSTEM_STD_AUTO_DETECT;
    }
    else {
        gChains_ctrl.displayRes[channel] = Chains_standardChoose();
    }
    Vsys_getResSize(gChains_ctrl.displayRes[channel], &gChains_ctrl.channelConf[channel].width, &gChains_ctrl.channelConf[channel].height);
    Vsys_getResRate(gChains_ctrl.displayRes[channel], &gChains_ctrl.channelConf[channel].frameRate);

    if (gChains_ctrl.channelConf[channel].encFlag == TRUE) {
        if (gChains_ctrl.channelConf[channel].enableTcp == TRUE &&
            gChains_ctrl.channelConf[channel].enableClient == TRUE &&
            gChains_ctrl.channelConf[channel].enableServer == FALSE) {
            OSA_printf("Please input server ip:\r\n");
            fflush(stdin);
            memset(gChains_ctrl.channelConf[channel].serverIp, 0, 16);
            fscanf(stdin,"%s",gChains_ctrl.channelConf[channel].serverIp);
        }

        OSA_printf("Select encode format:"
                   "\r\n 1: h264 high profile"
                   "\r\n 2: h264 main profile"
                   "\r\n 3: h264 base profile"
                   "\r\n 4: MJPEG"
                   "\r\n");
        fflush(stdin);
        fscanf(stdin,"%s",&(opMode[0]));
        UInt32 encIdx = atoi(opMode);
        UInt32 encFmt;
        UInt32 encPrf;
        switch (encIdx) {
            default:
            case 1:
                encFmt = IVIDEO_H264HP;
                encPrf = IH264_HIGH_PROFILE;
                break;
            case 2:
                encFmt = IVIDEO_H264MP;
                encPrf = IH264_MAIN_PROFILE;
                break;
            case 3:
                encFmt = IVIDEO_H264BP;
                encPrf = IH264_BASELINE_PROFILE;
                break;
            case 4:
                encFmt = IVIDEO_MJPEG;
                encPrf = 0;
                break;
        }
        if (encFmt == IVIDEO_MJPEG) {
            OSA_printf("Set MJPEG quality factor (range:[2, 97]): ");
            fflush(stdin);
            fscanf(stdin,"%s",&(opMode[0]));
            encPrf = atoi(opMode);
        }
        gChains_ctrl.channelConf[channel].encFormat = encFmt;
        gChains_ctrl.channelConf[channel].encProfile = encPrf;

        OSA_printf("Set target frame rate (range:[1, 60]): ");
        fflush(stdin);
        fscanf(stdin,"%s",&(opMode[0]));
        gChains_ctrl.channelConf[channel].encFrameRate = atoi(opMode);

        if (encFmt == IVIDEO_MJPEG) {
            return;
        }

        OSA_printf("Set intra frame interval (range:[1, ]): ");
        fflush(stdin);
        fscanf(stdin,"%s",&(opMode[0]));
        gChains_ctrl.channelConf[channel].intraFrameInterval = atoi(opMode);

        OSA_printf("Set bit rate control algorithm (0=VBR, 1=CBR): ");
        fflush(stdin);
        fscanf(stdin,"%s",&(opMode[0]));
        gChains_ctrl.channelConf[channel].rateCtrl = atoi(opMode);

        UInt32 maxBitRate = 40 * 1000;
        OSA_printf("Set bit rate (unit:kbps, range:[1, %d]): ", maxBitRate);
        fflush(stdin);
        fscanf(stdin,"%s",&(opMode[0]));
        UInt32 bitRate = atoi(opMode);
        if (bitRate < 1) bitRate = 1;
        //if (bitRate > maxBitRate) bitRate = maxBitRate;
        gChains_ctrl.channelConf[channel].bitRate = bitRate * 1000;
    }
}
//此函数内多通道的代码尚未添加
Void Chains_run(Chains_RunFunc chainsRunFunc)
{
    Chains_Ctrl chainsCtrl;
   

    Chains_detectBoard();

    System_linkControl(
        SYSTEM_LINK_ID_M3VPSS,
        SYSTEM_M3VPSS_CMD_RESET_VIDEO_DEVICES,
        NULL,
        0,
        TRUE
        );
/*
    int ch=0;
    Chains_menuChoose(ch);
    for (ch=1; ch<gChains_ctrl.channelNum; ch++) {
        char opMode[128];
        OSA_printf("Do you want the configure of channel %d is the same as previous (y/n): ", ch);
        fflush(stdin);
        fscanf(stdin,"%s",&(opMode[0]));
        if (opMode[0] == 'y') {
        	gChains_ctrl.displayRes[ch] = gChains_ctrl.displayRes[ch-1];
        	memcpy(&gChains_ctrl.channelConf[ch], &gChains_ctrl.channelConf[ch-1], sizeof(gChains_ctrl.channelConf[ch]));
        }
        else {
            Chains_menuChoose(ch);
        }
    }
*/
    memcpy(&chainsCtrl, &gChains_ctrl, sizeof(gChains_ctrl));

    Chains_memPrintHeapStatus();

    UTILS_assert(chainsRunFunc!=NULL);
    chainsRunFunc(&chainsCtrl);

    Chains_memPrintHeapStatus();
}

char gChains_menuMain0[] = {
    "\r\n ============"
    "\r\n Chain Select"
    "\r\n ============"
    "\r\n"
};

char gChains_menuMainVs[] = {
    "\r\n"
    "\r\n 1: Multi  CH Capture + NSF     + DEI     + Display           (16CH 4x TVP5158, NTSC, YUV422I )"
    "\r\n 2: Multi  CH Src + Enc + Dec + Snk (VideoM3 chain)           (VideoM3 local loopBack)"
    "\r\n 3: Multi  CH Cap + NF + Enc + Dec + SW (DEI/H,SC5) + Disp    (16CH Cap+Enc+Dec+Disp, Interlace Enc/Dec) "
    "\r\n 4: Multi  CH Cap + NF + DEI/H + Enc + Dec + SW (SC5) + Disp  (16CH Cap+Enc+Dec+Disp, Progressive Enc/Dec)"
    "\r\n "
    "\r\n s: System Settings "
    "\r\n "
    "\r\n x: Exit "
    "\r\n "
    "\r\n Enter Choice: "
};

char gChains_menuMainSingleCh[] = {
    "\r\n"
    "\r\n Connect a right DVI source to VIP port, this will be input for Sii9233a"
    "\r\n"
    "\r\n 1: Single CH Cap + Dis                                    (VIP0: 1x Sii9233a)"
    "\r\n 2: Single CH Cap + Nsf + SwMs + Dis                       (VIP0: 1x Sii9233a)"
    "\r\n 3: Single CH Cap + Enc + Dec + Dis                        (VIP0: 1x Sii9233a)"
    "\r\n 4: Single CH Cap + Enc + Rtsp                             (VIP0: 1x Sii9233a)"
    "\r\n 5: Single CH Cap + Enc + Tcp                              (VIP0: 1x Sii9233a)"
    "\r\n 6: Single CH Tcp + Dec + Dis                              (VIP0: 1x Sii9233a)"
    "\r\n 7: Single CH Cap + Enc + Tcp + Dec + Dis                  (VIP0: 1x Sii9233a)"
    "\r\n s: System Settings "
    "\r\n "
    "\r\n x: Exit "
    "\r\n "
    "\r\n Enter Choice: "
};

char gChains_menuMainDoubleCh[] = {
    "\r\n"
    "\r\n Connect right video sources to VIP ports, this will be input for Sii9233a"
    "\r\n"
    "\r\n 1: Double CH Cap + Dis                                    (VIP:  2x Sii9233a)"
    "\r\n 2: Double CH Cap + SwMs + Dis                             (VIP:  2x Sii9233a)"
    "\r\n 3: Double CH Cap + Enc + Dec + Dis                        (VIP:  2x Sii9233a)"
    "\r\n 4: Double CH Cap + Enc + RTSP                             (VIP:  2x Sii9233a)"
    "\r\n "
    "\r\n x: Exit "
    "\r\n "
    "\r\n Enter Choice: "
};

Void Chains_menuMainShow()
{
    printf(gChains_menuMain0);

    Chains_menuCurrentSettingsShow();

    if (SYSTEM_PLATFORM_BOARD == SYSTEM_PLATFORM_BOARD_VS) {
        printf(gChains_menuMainVs);
    }
    else if (SYSTEM_PLATFORM_BOARD == SYSTEM_PLATFORM_BOARD_CZ) {
        printf(gChains_menuMainSingleCh);
    }
    else if (SYSTEM_PLATFORM_BOARD == SYSTEM_PLATFORM_BOARD_ETV) {
        printf(gChains_menuMainDoubleCh);
    }
}

Void Chains_menuMainRunVs(char ch)
{
    gChains_ctrl.enableNsfLink = FALSE;

    switch(ch)
    {
        case '1':
            gChains_ctrl.enableNsfLink = TRUE;
            Chains_run(Chains_multiChCaptureNsfDei);
            break;
        case '2':
            Chains_run(Chains_multiChEncDecLoopBack);
            break;
        case '3':
            Chains_run(chains_multiChDucatiSystemUseCaseSwMsTriDisplay2);
            break;
        case '4':
            Chains_run(chains_multiChDucatiSystemUseCaseSwMsTriDisplay1);
            break;
        default:
            break;
    }
}

Void Chains_menuMainRunSingleCh(char ch)
{
    gChains_ctrl.channelNum = 1;
    switch(ch)
    {
        case '1':
            gChains_ctrl.channelConf[0].encFlag = FALSE;
            Chains_run(Chains_singleChCaptureSii9233a);
            break;
        case '2':
            gChains_ctrl.channelConf[0].encFlag = FALSE;
            Chains_run(Chains_singleChCapNsfSwMsDis);
            break;
        case '3':
            gChains_ctrl.channelConf[0].encFlag = TRUE;
            Chains_run(Chains_singleChCapNsfEncDecSwMsDis);
            break;
        case '4':
            gChains_ctrl.channelConf[0].encFlag = TRUE;
            gChains_ctrl.channelConf[0].enableTcp    = FALSE;
            gChains_ctrl.channelConf[0].enableRtsp   = TRUE;
            gChains_ctrl.channelConf[0].enableServer = TRUE;
            gChains_ctrl.channelConf[0].enableClient = FALSE;
            Chains_run(Chains_singleChCapEncSend);
            break;
        case '5':
            gChains_ctrl.channelConf[0].encFlag = TRUE;
            gChains_ctrl.channelConf[0].enableTcp    = TRUE;
            gChains_ctrl.channelConf[0].enableRtsp   = FALSE;
            gChains_ctrl.channelConf[0].enableServer = TRUE;
            gChains_ctrl.channelConf[0].enableClient = FALSE;
            Chains_run(Chains_singleChCapEncSend);
            break;
        case '6':
            gChains_ctrl.channelConf[0].encFlag = TRUE;
            gChains_ctrl.channelConf[0].enableTcp    = TRUE;
            gChains_ctrl.channelConf[0].enableRtsp   = FALSE;
            gChains_ctrl.channelConf[0].enableServer = FALSE;
            gChains_ctrl.channelConf[0].enableClient = TRUE;
            Chains_run(Chains_singleChRecvDecDis);
            break;
        case '7':
            gChains_ctrl.channelConf[0].encFlag = TRUE;
            gChains_ctrl.channelConf[0].enableTcp    = TRUE;
            gChains_ctrl.channelConf[0].enableRtsp   = FALSE;
            gChains_ctrl.channelConf[0].enableServer = TRUE;
            gChains_ctrl.channelConf[0].enableClient = TRUE;
            Chains_run(Chains_singleChDucatiSystem);
            break;
        case '8':
            gChains_ctrl.channelConf[0].encFlag = TRUE;
            gChains_ctrl.channelConf[0].enableTcp    = FALSE;
            gChains_ctrl.channelConf[0].enableRtsp   = FALSE;
            gChains_ctrl.channelConf[0].enableServer = TRUE;
            gChains_ctrl.channelConf[0].enableClient = TRUE;
            Chains_run(Chains_singleChDucatiSystem);
            break;
    }
}

Void Chains_menuMainRunDoubleCh(char ch)
{
    UInt32 i;
    switch(ch)
    {
        case '1':
            gChains_ctrl.channelNum = 2;
            for (i=0; i<gChains_ctrl.channelNum; i++) {
                gChains_ctrl.channelConf[i].encFlag = FALSE;
            }
            Chains_run(Chains_doubleChCapDis);
            break;
        case '2':
            gChains_ctrl.channelNum = 4;
            for (i=0; i<gChains_ctrl.channelNum; i++) {
                gChains_ctrl.channelConf[i].encFlag = FALSE;
            }
            Chains_run(Chains_doubleChCapSwMsDis);
            break;
        case '3':
            gChains_ctrl.channelNum = 2;
            for (i=0; i<gChains_ctrl.channelNum; i++) {
                gChains_ctrl.channelConf[i].encFlag      = TRUE;
                gChains_ctrl.channelConf[i].enableTcp    = FALSE;
                gChains_ctrl.channelConf[i].enableRtsp   = FALSE;
                gChains_ctrl.channelConf[i].enableServer = FALSE;
                gChains_ctrl.channelConf[i].enableClient = FALSE;
            }
            Chains_run(Chains_doubleChCapNsfEncDecSwMsDis);
            break;
        case '4':
            gChains_ctrl.channelNum = 4;
            for (i=0; i<gChains_ctrl.channelNum; i++) {
                gChains_ctrl.channelConf[i].encFlag      = TRUE;
                gChains_ctrl.channelConf[i].enableTcp    = FALSE;
                gChains_ctrl.channelConf[i].enableRtsp   = TRUE;
                gChains_ctrl.channelConf[i].enableServer = TRUE;
                gChains_ctrl.channelConf[i].enableClient = FALSE;
            }
            Chains_run(Chains_doubleChCapScEncSend);
            break;
        case '5':
            gChains_ctrl.channelNum = 2;
            for (i=0; i<gChains_ctrl.channelNum; i++) {
                gChains_ctrl.channelConf[i].encFlag      = TRUE;
                gChains_ctrl.channelConf[i].enableTcp    = FALSE;
                gChains_ctrl.channelConf[i].enableRtsp   = TRUE;
                gChains_ctrl.channelConf[i].enableServer = TRUE;
                gChains_ctrl.channelConf[i].enableClient = FALSE;
            }
            Chains_run(Chains_doubleChCapEncSend);
            break;
    }
}

//此函数中添加函数分析功能，通过参数控制功能
int main ( int argc, char **argv )
{
   //******dyx20131108******
    if(argc == 1)//argc 记录参数个数--Sue
    {
         showUsage();
         return 0;
    }
	enablelocst=1;
   	disablelocst=0;
    System_init();
	memset(&gChains_ctrl,0,sizeof(gChains_ctrl));
    Chains_setDefaultCfg();
// #define ONE
#ifdef ONE
   ParseArgs( argc, argv);
#else
    Chains_doubleChParseArgs(argc,argv);
#endif	
	gChains_ctrl.channelConf[0].intraFrameInterval=2*(gChains_ctrl.channelConf[0].encFrameRate);
	
     if(gChains_ctrl.channelConf[0].audioEnable)
    {
		   audio_init();
		   Audioflag=0;//xte_Sue Audio buffer available
		   AudioOn=0;//xte_Sue Audio on
     }
		if(gChains_ctrl.channelConf[0].enableServer== TRUE)//updata by Sue 
#ifdef ONE
	    	Chains_run(Chains_singleChCapEncSend);


#else
	Chains_run(Chains_doubleChCapScEncSend);
#endif
    System_deInit();

    return (0);
}

Void Chains_main()
{
    char ch[MAX_INPUT_STR_SIZE];
    Bool done;

    done = FALSE;

   // Chains_setDefaultCfg();

    while(!done)
    {

        Chains_menuMainShow();

        fgets(ch, MAX_INPUT_STR_SIZE, stdin);
        if(ch[1] != '\n' || ch[0] == '\n')
            continue;

        printf(" \r\n");

        switch (SYSTEM_PLATFORM_BOARD)
        {
            case SYSTEM_PLATFORM_BOARD_ETV:
                Chains_menuMainRunDoubleCh(ch[0]);
                break;
            case SYSTEM_PLATFORM_BOARD_CZ:
                Chains_menuMainRunSingleCh(ch[0]);
                break;
            case SYSTEM_PLATFORM_BOARD_VS:
                Chains_menuMainRunVs(ch[0]);
                break;
        }


        switch(ch[0])
        {
            case 's':
                Chains_menuSettings();
                break;
            case 'x':
                done = TRUE;
                break;
            case 'd':
                Chains_detectBoard();
                break;
        }

    }
}

Void Chains_menuCurrentSettingsShow()
{
    static char *nsfModeName[] =
        { "SNF + TNF", "CHR DS ONLY" };

    printf("\r\n Current System Settings,");
    printf("\r\n NSF Mode              : %s", nsfModeName[gChains_ctrl.bypassNsf]);
}

char gChains_menuSettings0[] = {
    "\r\n ==============="
    "\r\n System Settings"
    "\r\n ==============="
    "\r\n"
};

char gChains_menuSettings1[] = {
    "\r\n"
    "\r\n 1: NSF Bypass Mode"
    "\r\n "
    "\r\n x: Exit "
    "\r\n "
};

Void Chains_menuSettingsShow()
{
    printf(gChains_menuSettings0);

    Chains_menuCurrentSettingsShow();

    printf(gChains_menuSettings1);
}

Void Chains_menuSettings()
{
    char ch[MAX_INPUT_STR_SIZE];
    Bool done = FALSE;
    char inputStr[MAX_INPUT_STR_SIZE];
    Int32 value;

    Chains_menuSettingsShow();

    while(!done)
    {
        printf("\r\n Enter Choice: ");

    fgets(ch, MAX_INPUT_STR_SIZE, stdin);
    if(ch[1] != '\n' || ch[0] == '\n')
        continue;
        printf(" \r\n");

        switch(ch[0])
        {
            case '1':
                printf(" \r\n Enter NSF Mode [1: CHR DS ONLY, 2: SNF + TNF] : ");
                fgets(inputStr, MAX_INPUT_STR_SIZE, stdin);
                value = atoi(inputStr);

                if(value==1)
                    gChains_ctrl.bypassNsf = TRUE;
                if(value==2)
                    gChains_ctrl.bypassNsf = FALSE;

                break;
            case 'x':
                done = TRUE;
                break;
        }
    }
}
// 初始化显示的基本配置，并通知相关线程初始化已完成
Int32 Chains_displayCtrlInit(UInt32 displayRes[])
{
    Int32   status, i;

    VDIS_PARAMS_S prm;

    Vdis_params_init(&prm);

    // HDCOMP and DVO2 Tied
    displayRes[2] = displayRes[1];

    for (i = 0; i < SYSTEM_DC_MAX_VENC; i++)
    {
        prm.deviceParams[i].resolution = displayRes[i];
    }
    /* Setting SD resolution for SD VENC */
    prm.deviceParams[SYSTEM_DC_VENC_SD].resolution = VSYS_STD_NTSC;

    prm.deviceParams[SYSTEM_DC_VENC_HDMI].enable = TRUE;
    prm.deviceParams[SYSTEM_DC_VENC_DVO2].enable = TRUE;
    prm.deviceParams[SYSTEM_DC_VENC_HDCOMP].enable = FALSE;
    prm.deviceParams[SYSTEM_DC_VENC_SD].enable = TRUE;

    //Vdis_tiedVencInit(VDIS_DEV_HDCOMP, VDIS_DEV_HDMI, &prm);

    prm.enableConfigExtVideoEncoder = TRUE;
    prm.enableConfigExtThsFilter = FALSE;
    prm.enableEdgeEnhancement = FALSE;
    prm.numChannels = 1;
    prm.numUserChannels = 1;

    status = System_linkControl(
        SYSTEM_LINK_ID_M3VPSS,
        SYSTEM_M3VPSS_CMD_GET_DISPLAYCTRL_INIT,
        &prm,
        sizeof(prm),
        TRUE
        );
    UTILS_assert(status==OSA_SOK);

    return status;
}

Int32 Chains_displayCtrlDeInit()
{
    Int32 status;

    status = System_linkControl(
        SYSTEM_LINK_ID_M3VPSS,
        SYSTEM_M3VPSS_CMD_GET_DISPLAYCTRL_DEINIT,
        NULL,
        0,
        TRUE
        );
    UTILS_assert(status==OSA_SOK);

    return status;
}

Int32 Chains_grpxEnable(UInt32 grpxId, Bool enable)
{
    return 0;
}

Int32 Chains_prfLoadCalcEnable(Bool enable, Bool printStatus, Bool printTskLoad)
{
    UInt32 numProcs, procId;

    numProcs = sizeof(gChains_enabledProcs)/sizeof(gChains_enabledProcs[0]);

    for(procId=0; procId<numProcs; procId++)
    {
        if(enable)
        {
            System_linkControl(
                gChains_enabledProcs[procId],
                SYSTEM_COMMON_CMD_CPU_LOAD_CALC_START,
                NULL,
                0,
                TRUE
            );
        }
        else
        {
            System_linkControl(
                gChains_enabledProcs[procId],
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
                    gChains_enabledProcs[procId],
                    SYSTEM_COMMON_CMD_PRINT_STATUS,
                    &printStatus,
                    sizeof(printStatus),
                    TRUE
                );
            }
            System_linkControl(
                gChains_enabledProcs[procId],
                SYSTEM_COMMON_CMD_CPU_LOAD_CALC_RESET,
                NULL,
                0,
                TRUE
            );
        }
    }

    return 0;
}

Int32 Chains_memPrintHeapStatus()
{
    UInt32 numProcs, procId;
    SystemCommon_PrintStatus printStatus;

    memset(&printStatus, 0, sizeof(printStatus));
    numProcs = sizeof(gChains_enabledProcs)/sizeof(gChains_enabledProcs[0]);

    printStatus.printHeapStatus = TRUE;

    for(procId=0; procId<numProcs; procId++)
    {
        System_linkControl(
                gChains_enabledProcs[procId],
                SYSTEM_COMMON_CMD_CPU_LOAD_CALC_START,
                &printStatus,
                sizeof(printStatus),
                TRUE
            );
    }

    return 0;
}

char gChains_runTimeMenu[] = {
    "\r\n ===================="
    "\r\n Chains Run-time Menu"
    "\r\n ===================="
    "\r\n"
    "\r\n 0: Stop Chain"
    "\r\n"
    "\r\n s: Switch Display Layout  (Sequential change of layout)"
    "\r\n c: Switch Display Channel (Sequential: Increments by 1)"
    "\r\n p: Print Capture Statistics "
    "\r\n b: Modify Encoder Bit Rate "
    "\r\n f: Modify Encoder Frame Rate "
    "\r\n r: Modify Intra Frame Interval(GOP) "
    "\r\n t: Toggle force IDR frame "
    "\r\n g: Print Encoder Dynamic Parameters"
    "\r\n i: Print IVA-HD Statistics "
    "\r\n m: Print SwMs Statistics "
#ifdef  SYSTEM_ENABLE_AUDIO
     "\r\n a: Audio Capture / Playback"
#endif
    "\r\n "
    "\r\n Enter Choice: "
};
//------------------------------Sue-------------------------------
char gChains_runTimeMenuXTE[] = {
    "\r\n ===================="
    "\r\n Chains Run-time Menu"
    "\r\n ===================="
    "\r\n"
    "\r\n 0: Stop Chain"
    "\r\n"
    "\r\n p: Print Capture Statistics "
    "\r\n b: Modify Encoder Bit Rate "
    "\r\n f:  Modify Encoder Frame Rate "
    "\r\n a: Audio Capture Stop"
    "\r\n o: Modify OSD param"
    "\r\n s: single/double:Local Storage/change layout  "
    "\r\n L/E: double:Local Storage/stop  "
    "\r\n 1/2: double:change ch  "
    "\r\n e: Stop Local Storage "
    "\r\n Enter Choice: "
};
//---------------------------------------------------------------
char Chains_menuRunTime()
{
    char ch[MAX_INPUT_STR_SIZE];
    printf(gChains_runTimeMenuXTE);
    fgets(ch, MAX_INPUT_STR_SIZE, stdin);
    if(ch[1] != '\n' || ch[0] == '\n')
    ch[0] = '\n';
    return ch[0];
}

#ifdef  SYSTEM_ENABLE_AUDIO
char gChains_runTimeAudioMenu[] = {
    "\r\n ===================="
    "\r\n Audio Run-time Menu"
    "\r\n ===================="
    "\r\n"
    "\r\n 0: Exit Audio Menu"
    "\r\n"
    "\r\n f: Setup Audio Storage Path <set this first>"
    "\r\n s: Start Audio Capture"
    "\r\n t: Stop Audio Capture"
    "\r\n p: Start Audio Playback"
    "\r\n b: Stop Audio Playback"
    "\r\n d: Audio Statistics"
    "\r\n "
    "\r\n Enter Choice: "
} ;

char Chains_audioMenuRunTime()
{
    char buffer[MAX_INPUT_STR_SIZE];

    printf(gChains_runTimeAudioMenu);
    fgets(buffer, MAX_INPUT_STR_SIZE, stdin);
    if (buffer[0] == '\n')
    {
        fgets(buffer, sizeof(buffer), stdin);
    }
    buffer[1] = '\0';
    return buffer[0];
}

unsigned int Chains_AudioStorageInputRunTime(char *path)
{
    printf ("\r\n Provide Audio storage path <e.g. %s or %s> : ", "/audio", "/media/sda2/audio");
    fgets(path, MAX_INPUT_STR_SIZE, stdin);
    if (strcmp(path, "\n") == 0) {
        /** printf("\r\n New Line read"); **/
        fgets(path, MAX_INPUT_STR_SIZE, stdin);
    }
    return 0;
}

unsigned int Chains_AudioCaptureInputRunTime(Bool captureActiveFlag, Int8 maxAudChannels)
{
    UInt32 value;
    char inputStr[MAX_INPUT_STR_SIZE];

    if (captureActiveFlag == TRUE)
    {
        printf ("Audio capture already active... stopping current capture...\n");
    }

    printf("\r\n For which channel would you like to capture audio? Enter (1-%d): ", maxAudChannels);
        fgets(inputStr, MAX_INPUT_STR_SIZE, stdin);
    if (strcmp(inputStr, "\n") == 0) {
        /** printf("\r\n New Line read"); **/
        fgets(inputStr, MAX_INPUT_STR_SIZE, stdin);
    }

    value = atoi(inputStr);

    if(!(value>=1 && value<=maxAudChannels))
    {
        printf("\r\n Wrong Channel number entered ? Defaulting to 1");
        value = 1;
    }
   return value;
}

unsigned int Chains_AudioPlaybackInputRunTime(Bool playbackActiveFlag, Int8 maxAudChannels)
{
    UInt32 value;
    char inputStr[MAX_INPUT_STR_SIZE];

    if (playbackActiveFlag == TRUE)
    {
        printf ("Audio playback already active... stopping current playback...\n");
    }

    printf("\r\n Which channel audio would you like to play? Enter (1-%d): ", maxAudChannels);
        fgets(inputStr, MAX_INPUT_STR_SIZE, stdin);
    if (strcmp(inputStr, "\n") == 0) {
        /** printf("\r\n New Line read"); **/
        fgets(inputStr, MAX_INPUT_STR_SIZE, stdin);
    }

    value = atoi(inputStr);

    if(!(value>=1 && value<=maxAudChannels))
    {
        printf("\r\n Wrong Channel number entered ? Defaulting to 1");
        value = 1;
    }
   return value;
}

#endif

unsigned int Chains_ChanInputRunTime()
{
    UInt32 value = 128;
    char inputStr[MAX_INPUT_STR_SIZE];

        printf("\r\n For which channel would you like to apply this change? Enter (0-15): ");
        fgets(inputStr, MAX_INPUT_STR_SIZE, stdin);
        if (strcmp(inputStr, "\n") == 0) {
                /** printf("\r\n New Line read"); **/
                fgets(inputStr, MAX_INPUT_STR_SIZE, stdin);
        }

    if (strcmp(inputStr, "\n") != 0)
        value = atoi(inputStr);

        if(!(value>=0 && value<=15))
        {
                printf("\r\n Wrong Channel number entered. ");
                printf("Setting/Query will not be successfull");
                value = 128;
        }

    return value;
}

unsigned int Chains_BitRateInputRunTime()
{
    UInt32 value;
    char inputStr[MAX_INPUT_STR_SIZE];

    printf("\r\n Required bitrate? Enter(Range 64-4000 kbps): ");

    fgets(inputStr, MAX_INPUT_STR_SIZE, stdin);
    if (strcmp(inputStr, "\n") == 0) {
        /** printf("\r\n New Line read"); **/
        fgets(inputStr, MAX_INPUT_STR_SIZE, stdin);
    }

    value = atoi(inputStr);

    if(!(value>=64 && value<=4000))
    {
        printf("\r\n Erroreous Bitrate Value Entered, Not doing anything ");
        value = 0;
    }

    return (value  * 1000);
}

unsigned int Chains_FrameRateInputRunTime()
{
    UInt32 value;
    char inputStr[MAX_INPUT_STR_SIZE];

    printf("\r\n Required framerate? Enter(Choose 8,15,26,30): ");

    fgets(inputStr, MAX_INPUT_STR_SIZE, stdin);
    if (strcmp(inputStr, "\n") == 0) {
        /** printf("\r\n New Line read"); **/
        fgets(inputStr, MAX_INPUT_STR_SIZE, stdin);
    }

    value = atoi(inputStr);

    if(!(value==8 || value==15 || value==26 || value==30 ))
    {
        printf("\r\n Erroreous FPS Value Chosen, Not doing anything ");
        value = 0;
    }

    return value;
}

unsigned int Chains_IntraFrRateInputRunTime()
{
    UInt32 value;
    char inputStr[MAX_INPUT_STR_SIZE];

    printf("\r\n Required Intra-framerate(GOP) interval? Enter[Range 1 - 30]: ");

    fgets(inputStr, MAX_INPUT_STR_SIZE, stdin);
    if (strcmp(inputStr, "\n") == 0) {
        /** printf("\r\n New Line read"); **/
        fgets(inputStr, MAX_INPUT_STR_SIZE, stdin);
    }

    value = atoi(inputStr);

    if(!(value>=1 && value<=30))
    {
        printf("\r\n Erroreous GOP Value Entered, Not doing anything ");
        value = 0;
    }

    return value;
}

Int32 Chains_IsInterlaced(SYSTEM_Standard std)
{
    Int32 res = FALSE;
    switch (std) {
    case SYSTEM_STD_480I:
    case SYSTEM_STD_576I:
    case SYSTEM_STD_1080I_60:
    case SYSTEM_STD_1080I_50:
        res = TRUE;
        break;
    default:
        res = FALSE;
        break;
    }
    return res;
}
