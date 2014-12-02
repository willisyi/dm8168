/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2011 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/



#ifndef _SCD_BIT_WR_H_
#define _SCD_BIT_WR_H_

#include <demos/mcfw_api_demos/mcfw_demo/demo.h>
#include<math.h>

//#define IPC_BITS_DEBUG

#define DEMO_SCD_MOTION_DETECTION_SENSITIVITY(x,y)           ((x * y)/5) // 20%
#define DEMO_SCD_MOTION_DETECTION_SENSITIVITY_STEP           (10)        // 10%

#define DEMO_SCD_MOTION_TRACK_DEBUG 0

#define DEMO_SCD_MOTION_TRACK_GRPX_WIDTH  (1280) 
#define DEMO_SCD_MOTION_TRACK_GRPX_HEIGHT  (720) 

#define DEMO_SCD_MOTION_TRACK_BOX_WIDTH_CIF  (116) //GRPX_PLANE_GRID_WIDTH/ALG_LINK_SCD_MAX_FRAME_WIDTH  
#define DEMO_SCD_MOTION_TRACK_BOX_HEIGHT_CIF  (30) //GRPX_PLANE_GRID_HEIGHT/ALG_LINK_SCD_MAX_FRAME_HEIGHT 

#define DEMO_SCD_MOTION_TRACK_BOX_WIDTH_QCIF  (218) //GRPX_PLANE_GRID_WIDTH/ALG_LINK_SCD_MAX_FRAME_WIDTH  
#define DEMO_SCD_MOTION_TRACK_BOX_HEIGHT_QCIF  (60) //GRPX_PLANE_GRID_HEIGHT/ALG_LINK_SCD_MAX_FRAME_HEIGHT 

typedef struct {

    UInt32 totalDataSize;
    UInt32 numFrames;
    UInt32 maxWidth;
    UInt32 minWidth;
    UInt32 maxHeight;
    UInt32 minHeight;
} Scd_ChInfo;

typedef struct {

    OSA_ThrHndl wrThrHndl;
    OSA_SemHndl wrSem;
    Bool        exitWrThr;
    Bool        isWrThrStopDone;

    Scd_ChInfo chInfo[48]; //chInfo[VENC_CHN_MAX];

    UInt32 statsStartTime;
#if 1//DEMO_SCD_ENABLE_FILE_WRITE
    Bool fileWriteEnable;
    char fileWriteName[512];
    UInt32  chId;
#endif
    UInt32  winId[16];
    UInt32  fileWriteChn;
    Bool    enableMotionTracking;
    UInt32  chIdTrack;
    UInt32  prevWinIdTrack;
    UInt32  numberOfWindows;
    UInt32  prevNumberOfWindows;
    UInt32  startChId;
    Bool    drawGrid;
    Bool    gridPresent;
    Bool    layoutUpdate;
} Scd_Ctrl;


extern Scd_Ctrl gScd_ctrl;

VCAP_CALLBACK_S       gbitscallbackFxn;
Int32 Scd_resetStatistics();
Int32 Scd_printStatistics(Bool resetStats);

Int32 Scd_bitsWriteCreate(UInt32 useCase);
Int32 Scd_bitsWriteDelete();
Void Scd_bitsWriteStop();
Void Scd_enableMotionTracking();
Void Scd_disableMotionTracking();
Bool Scd_isMotionTrackingEnabled();
Void Scd_trackLayout(UInt32 numberOfWindows, UInt32 startChId);
//Bool Scd_isMotionTrackingEnabled();                       /* not required */

Int32 Scd_trackCh(UInt32 tCh);                              /* To receive the entered(tracking) channel id for switch through TrackChId */
void Scd_windowGrid(UInt32 numberOfWindows, UInt32 chId, Bool flag, UInt32 numBlkPerRow);    /* To draw or undrwa the grid in caso of channel/layout switch */


#endif
