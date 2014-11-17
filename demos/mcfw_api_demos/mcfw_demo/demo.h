/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2011 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/



#ifndef _DEMO_H_
#define _DEMO_H_

#include <osa.h>
#include <osa_thr.h>
#include <osa_sem.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <errno.h>
#include <ctype.h>
#include <sys/wait.h>
#include <sys/types.h>  // For stat().
#include <sys/stat.h>   // For stat().
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>

#include "ti_vdis_common_def.h"
#include "ti_vdec_common_def.h"

#include "ti_vsys.h"
#include "ti_vcap.h"
#include "ti_venc.h"
#include "ti_vdec.h"
#include "ti_vdis.h"
#include "ti_vdis_timings.h"
#include "ti_audio.h"

#include "demos/graphic/graphic.h"
#include "demos/audio_sample/audio.h"
#include "demos/audio_sample/alg_uLawCodec.h"
#include "demos/display_process/display_process.h"

#include <demos/mcfw_api_demos/mcfw_demo/demo_swms.h>


/* To select if FBDEV interface is used for Graphics */
#if defined(TI_814X_BUILD)
#define USE_FBDEV   1
#endif

#if defined(TI_8107_BUILD)
#define USE_FBDEV   1
#endif

#if defined(TI_816X_BUILD)
#define USE_FBDEV   1
#endif

#define DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE            (0)
#define DEMO_VCAP_VENC_VDEC_VDIS_INTERLACED             (1)
#define DEMO_VCAP_VENC_VDIS                             (2)
#define DEMO_VCAP_VENC_VDIS_HD                          (3)
#define DEMO_VDEC_VDIS                                  (4)
#define DEMO_VCAP_VDIS                                  (5)
#define DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE_NON_D1     (6)
#define DEMO_CUSTOM_0                                   (7)
#define DEMO_CUSTOM_1                                   (8)
#define DEMO_CUSTOM_2                                   (9)
#define DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE_4CH        (10)
#define DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE_8CH        (11)
#define DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE_16CH_NRT   (12)
#define DEMO_VCAP_VENC_VDEC_VDIS_PROGRESSIVE_8CH_NRT    (13)
#define DEMO_HYBRIDDVR_16CH                             (14)
#define DEMO_CARDVR_4CH                                 (15)
#define DEMO_HYBRIDENC_36CH                             (16)
#define DEMO_LAST                                       (DEMO_HYBRIDENC_54CH)
#define DEMO_MAX                                        (DEMO_LAST+1)


#define DEMO_HD_DISPLAY_DEFAULT_STD             (VSYS_STD_1080P_60)

#define MAX_INPUT_STR_SIZE                      (128)
#define DEMO_SW_MS_INVALID_ID                   (0xFF)


#define SCD_MAX_TILES                           (9)
#define SCD_MAX_CHANNELS                        (16)


/* The below part is temporary for OSD specific items */
#define DEMO_OSD_NUM_WINDOWS        (4)
#define DEMO_OSD_NUM_BLIND_WINDOWS  (4)
#define DEMO_OSD_WIN_MAX_WIDTH   (320)
#define DEMO_OSD_WIN_MAX_HEIGHT  (64)
#define DEMO_OSD_WIN_WIDTH       (224)
#define DEMO_OSD_WIN_HEIGHT      (30)
#define DEMO_OSD_WIN0_STARTX     (16)
#define DEMO_OSD_WIN0_STARTY     (16)

#define DEMO_OSD_WIN_PITCH_H     (224)
#define DEMO_OSD_WIN_PITCH_V     (30)
#define DEMO_OSD_TRANSPARENCY    (1)
#define DEMO_OSD_GLOBAL_ALPHA    (0x80)
#define DEMO_OSD_ENABLE_WIN      (1)

#define DEMO_ENC_NUM_RESOLUTION_STREAMS     (3)
#define DEMO_BLIND_AREA_NUM_WINDOWS     (4)
#define DEMO_BLIND_AREA_WIN_WIDTH       (50)
#define DEMO_BLIND_AREA_WIN_HEIGHT      (20)
#define DEMO_BLIND_AREA_WIN0_STARTX     (512)
#define DEMO_BLIND_AREA_WIN0_STARTY     (16)
#define DEMO_SCD_ENABLE_MOTION_TRACKING (1)
#define DEMO_SCD_ENABLE_FILE_WRITE      (0)

typedef enum {
    DEMO_TYPE_PROGRESSIVE,
    DEMO_TYPE_INTERLACED
}demoType;

typedef enum {
    DEMO_AUDIO_TYPE_NONE,
    DEMO_AUDIO_TYPE_CAPTURE,
    DEMO_AUDIO_TYPE_ENCODE,
    DEMO_AUDIO_TYPE_DECODE
} demoAudioType;

typedef struct {

    UInt32 maxVencChannels;
    UInt32 maxVcapChannels;
    UInt32 maxVdisChannels;
    UInt32 maxVdecChannels;
    UInt32 VsysNumChs;

    Bool   audioCaptureActive;
    Bool   audioPlaybackActive;
    Int8   audioCaptureChNum;
    Int8   audioPlaybackChNum;
    Bool   isAudioPathSet;
    Bool   audioEnable;
    Bool   osdEnable;
    Bool   scdTileConfigInitFlag;
    demoType Type;
    demoAudioType audioType;
    Bool          audioInitialized;
    VSYS_USECASES_E usecase;
    UInt32 numDeis;
    UInt32 numSwMs;
    UInt32 numDisplays;
    UInt32 numEncChannels[DEMO_ENC_NUM_RESOLUTION_STREAMS];
    UInt32 bitRateKbps[DEMO_ENC_NUM_RESOLUTION_STREAMS];
    UInt32 defaultLayoutId[VDIS_DEV_MAX];
    Bool   scdEnable;
    UInt32 curDisplaySeqId;

} Demo_Info;

typedef struct {

    Int8  inFile[MAX_INPUT_STR_SIZE];
    Int8  outFile[MAX_INPUT_STR_SIZE];
    Int32 bitRate;
    Int32 numChannels;
    Int32 sampleRate;
    Int32 encodeType;
} Audio_EncInfo;

typedef struct {

    Int8  inFile[MAX_INPUT_STR_SIZE];
    Int8  outFile[MAX_INPUT_STR_SIZE];
    Int32 numChannels;
    Int32 decodeType;

} Audio_DecInfo;

typedef struct {

    Int8  outFile[MAX_INPUT_STR_SIZE];
    Int32 numEncodeChannels;

} Audio_CapInfo;

extern Demo_Info gDemo_info;
extern AlgLink_OsdChWinParams g_osdChParam[];
extern AlgLink_OsdChBlindWinParams g_osdChBlindParam[];
extern CaptureLink_BlindInfo g_blindAreaChParam[];

Void  VcapVdis_start();
Void  VcapVdis_stop();

Void  VcapVenc_start(Bool hdDemo);
Void  VcapVenc_stop();
Int32 VcapVenc_printStatistics(Bool resetStats);
Int32 Scd_printStatistics(Bool resetStats);


Void  VdecVdis_start();
Void  VdecVdis_stop();

Void  VcapVencVdecVdis_start( Bool doProgressiveVenc, Bool enableSecondaryOut, int demoId );
Void  VcapVencVdecVdis_stop();
Int32 VcapVencVdecVdis_printStatistics(Bool resetStats, Bool allChs);

Void VcapVencVdecVdisCustom_start(Bool enableDecode, Bool enableCapture, UInt32 numVipInst);
Void VcapVencVdecVdisCustom_stop();

void  Demo_generateH264HdrFile(char *filename);
void  Demo_generateMjpgHdrFile(char *filename);
void  Demo_generateMpeg4HdrFile(char *filename);

Int32 Demo_swMsGetOutSize(UInt32 outRes, UInt32 * width, UInt32 * height);

int Demo_audioCaptureStop();
int Demo_audioCaptureSetParams(Bool set_params);
int Demo_audioSetParams();
int Demo_audioPlaybackStop();
int Demo_audioCaptureStart(UInt32 chId);
int Demo_audioPlaybackStart(UInt32 chId, UInt32 playbackDevId);
int Demo_audioSettings(int demoId);
int Demo_audioEnable(Bool enable);

int Demo_captureSettings(int demoId);
int Demo_captureGetVideoSourceStatus();
int Demo_captureGetTamperStatus(Ptr pPrm);
int Demo_captureGetMotionStatus(Ptr pPrm);
int Demo_decodeErrorStatus(Ptr pPrm);
VSYS_VIDEO_STANDARD_E Demo_captureGetSignalStandard();

int Demo_encodeSettings(int demoId);

int Demo_decodeSettings(int demoId);

int Demo_displaySwitchChn(int devId, int startChId);
int Demo_displaySwitchSDChan(int devId, int startChId);
int Demo_displayChnEnable(int chId, Bool enable);
int Demo_displayGetLayoutId(int demoId);
int Demo_displaySetResolution(UInt32 displayId, UInt32 resolution);
int Demo_displaySettings(int demoId);
UInt32 Demo_displayGetCurSeqId();

Int32 Demo_osdInit(UInt32 numCh, UInt8 *osdFormat);
Void  Demo_osdDeinit();

Int32 Demo_blindAreaInit(UInt32 numCh, UInt32 demoId);

int  Demo_printInfo(int demoId);
Int32 Demo_startStopAudioEncodeDecode (Int32 demoId, Int32 option, Bool userOpt);
int Demo_printBuffersInfo();
int Demo_printAvsyncInfo();
int Demo_switchIVAMap();
int  Demo_startStop(int demoId, Bool startDemo);
int  Demo_run(int demoId);
char Demo_getChar();
int  Demo_getChId(char *string, int maxChId);
int  Demo_getIntValue(char *string, int minVal, int maxVal, int defaultVal);
Bool Demo_getFileWriteEnable();
Bool Demo_getMotionTrackEnable();
Bool Demo_isPathValid( const char* absolutePath );
int  Demo_getFileWritePath(char *path, char *defaultPath);
Void VdecVdis_setTplayConfig(VDIS_CHN vdispChnId, VDIS_AVSYNC speed);

#if defined(TI_814X_BUILD) || defined(TI_8107_BUILD)
int Demo_change8ChMode(int demoId);
int Demo_change16ChMode(int demoId);
#endif

Void Demo_initAudioSystem(Void);
Void Demo_deInitAudioSystem(Void);

Bool Demo_startAudioEncodeSystem (Void);
Bool Demo_startAudioDecodeSystem (Void);
Bool Demo_startAudioCaptureSystem (Void);
Bool Demo_stopAudioDecodeSystem(Bool userOpt);
Bool Demo_stopAudioEncodeSystem (Bool userOpt);
Bool Demo_stopAudioCaptureSystem (Bool userOpt);
Bool Demo_IsCaptureActive(Void);
Bool Demo_IsEncodeActive(Void);
Bool Demo_IsDecodeActive(Void);


#endif /* TI_MULTICH_DEMO_H */
