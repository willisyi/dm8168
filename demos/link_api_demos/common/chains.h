/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#ifndef _CHAINS_H_
#define _CHAINS_H_


#include <osa.h>

#include <link_api/system.h>
#include <link_api/captureLink.h>
#include <link_api/deiLink.h>
#include <link_api/nsfLink.h>
#include <link_api/algLink.h>
#include <link_api/displayLink.h>
#include <link_api/nullLink.h>
#include <link_api/grpxLink.h>
#include <link_api/dupLink.h>
#include <link_api/swMsLink.h>
#include <link_api/selectLink.h>
#include <link_api/mergeLink.h>
#include <link_api/nullSrcLink.h>
#include <link_api/ipcLink.h>
#include <link_api/systemLink_m3vpss.h>
#include <link_api/systemLink_m3video.h>
#include <link_api/encLink.h>
#include <link_api/decLink.h>
#include <assert.h>
#include <mcfw/interfaces/link_api/algLink.h>
#include <demos/link_api_demos/common/chains_swMs.h>
#include <demos/link_api_demos/common/chains_ipcBits.h>
#include <mcfw/interfaces/link_api/system_common.h>
#include <demos/graphic/graphic.h>

#include <ti/xdais/xdas.h>
#include <ti/xdais/dm/xdm.h>
#include <ti/xdais/dm/ivideo.h>
#include <ih264enc.h>
#include <ih264vdec.h>
#include <demos/audio_sample/audio.h>
#include <time.h>
#include "share_mem.h"
#define CHAINS_OSD_NUM_WINDOWS      (4)
#define CHAINS_OSD_WIN_MAX_WIDTH    (400)
#define CHAINS_OSD_WIN_MAX_HEIGHT   (400)
#define CHAINS_OSD_WIN0_STARTX      (80)
#define CHAINS_OSD_WIN0_STARTY      (16)
#define CHAINS_OSD_WIN1_STARTX      (256)
#define CHAINS_OSD_WIN1_STARTY      (128)
#define CHAINS_OSD_WIN_WIDTH        (220)
#define CHAINS_OSD_WIN_HEIGHT       (112)
#define CHAINS_OSD_WIN_PITCH        (CHAINS_OSD_WIN_WIDTH)
#define CHAINS_OSD_TRANSPARENCY     (TRUE)
#define CHAINS_OSD_GLOBAL_ALPHA     (0x40)
#define CHAINS_OSD_ENABLE_WIN       (TRUE)
#define CHAINS_OSD_MAX_FILE_NAME_SIZE (128)
#define CHAINS_OSD_WIN0_FILE_NAME   "xtLogoYUV422i.yuyv"

#define CHAINS_INIT_STRUCT(structName,structObj)  structName##_Init(&structObj)

#define AUDIO_ON 0
#define AUDIO_OFF 1
#define AUDIO_XX 2
typedef struct Chains_channelConf {
    Bool   autoDetect;
    UInt32 width;
    UInt32 height;
    UInt32 frameRate;

    Bool   encFlag;
    UInt32 encFormat;
    UInt32 encProfile;
    UInt32 encFrameRate;
    UInt32 intraFrameInterval;

    Bool   enableTcp;
    Bool   enableRtsp;
    Bool   enableClient;
    Bool   enableServer;
    Bool   rateCtrl;
    UInt32 bitRate;

    /*Audio Set 20130413*/
    Bool   audioEnable;
    Uint16  audioSample_rate;
    UInt32   audioCodeType;
    Bool   G711uLaw;
    UInt32  audioBitRate ;
    Bool enableOsd;
    UInt32 OSDfont;
    UInt32 OSDStartX;
    UInt32 OSDStartY;
	
    char   serverIp[16];
} Chains_channelConf;

typedef struct {

    /* Enable NSF during chain */
    Bool   enableNsfLink;

    Bool   enableOsdAlgLink;

    Bool   enableVidFrameExport;
    /* NSF mode when NSF Link is in chain

        TRUE: NSF is bypass and is in CHR DS mode
        FALSE: NSF is bypass and is in CHR DS mode
    */
    Bool bypassNsf;

    UInt32 channelNum;
    UInt32 displayRes[16];
    Chains_channelConf channelConf[16];
} Chains_Ctrl;

Chains_Ctrl gChains_ctrl;
int AudioOn;
int AudioOff;
int Audioflag;//xte_Sue Audio buffer available
//extern Chains_IpcBitsCtrl gChains_ipcBitsCtrl;
//******dyx20131108******
int enablelocst;
int disablelocst;
time_t rawtime;
struct tm *LocalTime;
extern UInt32 osdId;
extern AlgLink_CreateParams    DSPLinkPrm;
extern AlgLink_OsdChWinParams g_osdChParam[];//guo for demo osd
extern AlgLink_OsdChBlindWinParams g_osdChBlindParam[];
extern char NameExample[48][32];//max 256/16=16  or 256/32 = 8
//added for RTP
/*
extern int shmid;
extern int semFull,semEmpty;
extern int semFull_audio,semEmpty_audio;
extern shared_use *shared_stuff;
extern void *Videoptr ;
extern void *audioptr ;
*/
/*存储音频包的结构体*/
typedef struct  AudioBag
{
    Int8            id[5];
    Uint8           numChannels;
    Uint16      samplingRate;
    Uint64      timestamp;
    Uint32      data_size;
    Uint32      bkt_status;
    Uint8        buf_encode[3000];
} AudioBag;
AudioBag audiobag;


Int32 Demo_osdInit(UInt32 numCh, UInt8 *osdFormat);//guo
Void Demo_osdDeinit();//guo

typedef Void (*Chains_RunFunc)();


Void Chains_main();

Void Chains_singleChCaptureSii9233a(Chains_Ctrl *chainsCfg);
Void Chains_singleChCapNsfSwMsDis(Chains_Ctrl *chainsCfg);
Void Chains_singleChEncDecLoopBack(Chains_Ctrl *chainsCfg);
Void Chains_singleChDucatiSystem(Chains_Ctrl *chainsCfg);
Void Chains_singleChCapNsfEncDecSwMsDis(Chains_Ctrl *chainsCfg);
Void Chains_singleChCapEncSend(Chains_Ctrl *chainsCfg);
Void Chains_singleChRecvDecDis(Chains_Ctrl *chainsCfg);

Void Chains_doubleChCapDis(Chains_Ctrl *chainsCfg);
Void Chains_doubleChCapDisTest(Chains_Ctrl *chainsCfg);
Void Chains_doubleChCapSwMsDis(Chains_Ctrl *chainsCfg);
Void Chains_doubleChCapNsfEncDecSwMsDis(Chains_Ctrl *chainsCfg);
Void Chains_doubleChCapEncSend(Chains_Ctrl *chainsCfg);
Void Chains_doubleChCapScEncSend(Chains_Ctrl *chainsCfg);

Void Chains_multiChCaptureNsfDei(Chains_Ctrl *chainsCfg);
Void Chains_multiChSystemUseCaseSwMsTriDisplay(Chains_Ctrl *chainsCfg);
Void Chains_multiChSystemUseCaseSwMsTriDisplay2(Chains_Ctrl *chainsCfg);
Void Chains_tvp5158NonMuxCapture(Chains_Ctrl *chainsCfg);

Void Chains_multiChCaptureDeiIpcOutIn(Chains_Ctrl *chainsCfg);
Void Chains_multiChEncDecLoopBack(Chains_Ctrl *chainsCfg);
Void chains_multiChDucatiSystemUseCaseSwMsTriDisplay1(Chains_Ctrl *chainsCfg);
Void chains_multiChDucatiSystemUseCaseSwMsTriDisplay2(Chains_Ctrl *chainsCfg);

Void Chains_menuSettings();
Void Chains_menuCurrentSettingsShow();
char Chains_menuRunTime();

unsigned int Chains_ChanInputRunTime();
unsigned int Chains_BitRateInputRunTime();
unsigned int Chains_FrameRateInputRunTime();
unsigned int Chains_IntraFrRateInputRunTime();
#ifdef	SYSTEM_ENABLE_AUDIO
char Chains_audioMenuRunTime(void);
unsigned int Chains_AudioStorageInputRunTime(char *path);
unsigned int Chains_AudioCaptureInputRunTime(Bool captureActiveFlag, Int8 maxAudChannels);
unsigned int Chains_AudioPlaybackInputRunTime(Bool playbackActiveFlag, Int8 maxAudChannels);
#endif


Int32 Chains_detectBoard();

Int32 Chains_displayCtrlInit(UInt32 displayRes[]);
Int32 Chains_displayCtrlDeInit();
Int32 Chains_grpxEnable(UInt32 grpxId, Bool enable);
Int32 Chains_prfLoadCalcEnable(Bool enable, Bool printStatus, Bool printTskLoad);
Int32 Chains_memPrintHeapStatus();
Int32 Chains_IsInterlaced(SYSTEM_Standard std);

#endif

