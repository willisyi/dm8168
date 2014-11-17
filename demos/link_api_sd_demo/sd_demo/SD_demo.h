/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#ifndef _SD_DEMO_H_
#define _SD_DEMO_H_



#include <osa.h>
#include <link_api/system.h>
#include <link_api/captureLink.h>
#include <link_api/deiLink.h>
#include <link_api/nsfLink.h>
#include <link_api/algLink.h>
#include <link_api/displayLink.h>
#include <link_api/nullLink.h>
#include <link_api/sclrLink.h>
#include <link_api/grpxLink.h>
#include <link_api/dupLink.h>
#include <link_api/swMsLink.h>
#include <link_api/mergeLink.h>
#include <link_api/nullSrcLink.h>
#include <link_api/ipcLink.h>
#include <link_api/systemLink_m3vpss.h>
#include <link_api/systemLink_m3video.h>
#include <link_api/encLink.h>
#include <link_api/decLink.h>

#include <mcfw/interfaces/link_api/system_tiler.h>
#include <mcfw/interfaces/common_def/ti_vdis_common_def.h>

#include <device.h>
#include <device_videoDecoder.h>
#include <device_tvp5158.h>
#include <mcfw/src_linux/devices/tvp5158/src/tvp5158_priv.h>
#include <tvp5158.h>
#include <device_sii9022a.h>
#include <mcfw/src_linux/devices/sii9022a/src/sii9022a_priv.h>
#include <sii9022a.h>
#include <thsfilters.h>

#include <ti/xdais/xdas.h>
#include <ti/xdais/dm/xdm.h>
#include <ti/xdais/dm/ivideo.h>
#include <ih264enc.h>
#include <ih264vdec.h>

#include <demos/link_api_sd_demo/sd_demo/SD_demo_swMs.h>
#include <demos/link_api_sd_demo/sd_demo/SD_demo_ipc.h>
#include <demos/link_api_sd_demo/sd_demo/SD_demo_scd_bits_wr.h>
#include <demos/link_api_sd_demo/sd_demo/SD_demo_common.h>

#define SD_DEMO_OSD_NUM_WINDOWS      (4)
#define SD_DEMO_OSD_WIN_MAX_WIDTH    (320)
#define SD_DEMO_OSD_WIN_MAX_HEIGHT   (64)
#define SD_DEMO_OSD_WIN0_STARTX      (16)
#define SD_DEMO_OSD_WIN0_STARTY      (16)
#define SD_DEMO_OSD_WIN1_STARTX      (500)
#define SD_DEMO_OSD_WIN1_STARTY      (150)
#define SD_DEMO_OSD_WIN_WIDTH       (224)
#define SD_DEMO_OSD_WIN_HEIGHT      (30)
#define SD_DEMO_OSD_WIN_PITCH       (SD_DEMO_OSD_WIN_WIDTH)
#define SD_DEMO_OSD_TRANSPARENCY    (1)
#define SD_DEMO_OSD_GLOBAL_ALPHA    (0x80)
#define SD_DEMO_OSD_ENABLE_WIN      (1)
#define SD_DEMO_OSD_MAX_FILE_NAME_SIZE (128)

#define EXAMPLE_OSD_NUM_WINDOWS     (3)
#define EXAMPLE_OSD_WIN_MAX_WIDTH   (320)
#define EXAMPLE_OSD_WIN_MAX_HEIGHT  (64)
#define EXAMPLE_OSD_WIN_WIDTH       (160)
#define EXAMPLE_OSD_WIN_HEIGHT      (32)
#define EXAMPLE_OSD_WIN0_STARTX     (16)
#define EXAMPLE_OSD_WIN0_STARTY     (16)

#define EXAMPLE_OSD_WIN_PITCH       (EXAMPLE_OSD_WIN_WIDTH)
#define EXAMPLE_OSD_TRANSPARENCY    (0)
#define EXAMPLE_OSD_GLOBAL_ALPHA    (0x80)
#define EXAMPLE_OSD_ENABLE_WIN      (1)
#define EXAMPLE_OSD_DISABLE_WIN     (0)

#define OSD_BUF_HEAP_SR_ID          (0)


#define SD_DEMO_BLIND_AREA_NUM_WINDOWS     (4)
#define SD_DEMO_BLIND_AREA_WIN_WIDTH       (50)
#define SD_DEMO_BLIND_AREA_WIN_HEIGHT      (20)
#define SD_DEMO_BLIND_AREA_WIN0_STARTX     (512)
#define SD_DEMO_BLIND_AREA_WIN0_STARTY     (16)

#define     NUM_CAPTURE_DEVICES     (4)
#define     NUM_DISPLAY_DEVICES     (4)

#define SD_DEMO_INIT_STRUCT(structName,structObj)  structName##_Init(&structObj)


#define MAX_DEI_LINK        (3)
#define MAX_IPC_FRAMES_LINK (2)
#define MAX_ALG_LINK        (2)
#define MAX_SCLR_LINK       (2)
#define MAX_NSF_LINK        (4)
#define MAX_IPC_BITS_LINK   (2)
#define MAX_SWMS_LINK       (2)
#define MAX_DISPLAY_LINK    (2)
#define MAX_MERGE_LINK      (3)
#define MAX_DUP_LINK        (2)

/* =============================================================================
 * Structure
 * =============================================================================
 */
typedef struct
{
    UInt32                         captureId;
    /* Capture Link ID */
    
    UInt32                         dspAlgId[MAX_ALG_LINK];
    /* DSP-ALG Link ID. Either OSD or SCD Feature */
    UInt32                         nsfId[MAX_NSF_LINK];
    /* NSF Link ID*/
    UInt32                         sclrId[MAX_SCLR_LINK];

    UInt32                         deiId[MAX_DEI_LINK];

    UInt32                         ipcFramesInDspId[MAX_IPC_FRAMES_LINK];
    UInt32                         ipcFramesOutVpssId[MAX_IPC_FRAMES_LINK];
    UInt32                         ipcFramesOutVpssToHostId;
    UInt32                         ipcFramesInHostId;
    UInt32                         ipcFramesOutHostId;
    UInt32                         ipcFramesInVpssFromHostId;

    UInt32                         ipcBitsInHLOSId[MAX_IPC_BITS_LINK];
    UInt32                         ipcBitsOutDSPId;
    UInt32                         ipcBitsOutRTOSId;
    UInt32                         ipcInVideoId;
    UInt32                         ipcOutVpssId;

    UInt32                         encId;
    UInt32                         decId;
    UInt32                         swMsId[MAX_SWMS_LINK];
    UInt32                         displayId[MAX_DISPLAY_LINK];

    UInt32                         mergeId[MAX_MERGE_LINK];
    UInt32                         dupId[MAX_DUP_LINK];
    
    UInt32                         numCapChannels;
    UInt32                         numEncChannels;
    UInt32                         numSubChains;

    Bool                           isPalMode;
    Bool                           enableOsdAlgLink;
    Bool                           enableScdAlgLink;
    Bool                           enableVideoFramesExport;

    Ptr                            callbackArg;
    Ptr                            bitscallbackArg;
    UInt32                         displayRes[SYSTEM_DC_MAX_VENC];

    Device_Tvp5158Handle           tvp5158Handle[NUM_CAPTURE_DEVICES];
    Device_Sii9022aHandle          sii9022aHandle;
    VDIS_PARAMS_S                  prm;

}SD_Demo_Ctrl;

extern SD_Demo_Ctrl gSD_Demo_ctrl;



typedef Void (*SD_DEMO_RunFunc)();

int SD_Demo_main();
int SD_Demo_run();
int SD_Demo_init();
int SD_Demo_deinit();


Void SD_Demo_multiChDucatiSystemUseCaseSwMsTriDisplay3(SD_Demo_Ctrl *demoCfg);
Void SD_Demo_menuSettings();
Void SD_Demo_menuCurrentSettingsShow();
Void SD_Demo_runMenuSettings();

unsigned int SD_Demo_ChanInputRunTime();
unsigned int SD_Demo_BitRateInputRunTime();
unsigned int SD_Demo_FrameRateInputRunTime();
unsigned int SD_Demo_IntraFrRateInputRunTime();

Int32 SD_Demo_osdInit(UInt32 numCh, UInt8 *osdFormat);
Void  SD_Demo_osdDeinit();

Int32 SD_Demo_blindAreaInit(UInt32 numCh);
Int32 SD_Demo_blindAreaUpdate(UInt32 numCh);

Void  SD_Demo_capParamSet();
Void  SD_Demo_encParamSet();
Void  SD_Demo_algParamSet();
Void  SD_Demo_disParamSet();

Void  SD_Demo_capDelete();
Void  SD_Demo_encDelete();
Void  SD_Demo_algDelete();
Void  SD_Demo_disDelete();

Void SD_Demo_create();
Void SD_Demo_delete();
Void SD_Demo_start();
Void SD_Demo_stop();

Void SD_Demo_ipcBitsInitCreateParams_BitsOutRTOS(IpcBitsOutLinkRTOS_CreateParams *cp,
                                                Bool notifyPrevLink);
Void SD_Demo_ipcBitsInitCreateParams_BitsInHLOS(IpcBitsInLinkHLOS_CreateParams *cp);

Int32 SD_Demo_printDetailedStatistics();
#endif

