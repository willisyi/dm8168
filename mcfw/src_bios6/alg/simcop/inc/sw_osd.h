/* ======================================================================== */
/*  TEXAS INSTRUMENTS, INC.                                                 */
/*                                                                          */
/*  SWOSD Library                                                           */
/*                                                                          */
/*  This library contains proprietary intellectual property of Texas        */
/*  Instruments, Inc.  The library and its source code are protected by     */
/*  various copyrights, and portions may also be protected by patents or    */
/*  other legal protections.                                                */
/*                                                                          */
/*  This software is licensed for use with Texas Instruments TMS320         */
/*  family DSPs.  This license was provided to you prior to installing      */
/*  the software.  You may review this license by consulting the file       */
/*  TI_license.PDF which accompanies the files in this library.             */
/*                                                                          */
/* ------------------------------------------------------------------------ */
/*                                                                          */
/*     NAME                                                                 */
/*        swosd.h -- SWOSD interface header file                            */
/*                                                                          */
/*     DESCRIPTION                                                          */
/*                SWOSD interface header file                               */
/*                  														*/
/*                                                                          */
/* ------------------------------------------------------------------------ */
/*            Copyright (c) 2012 Texas Instruments, Incorporated.           */
/*                           All Rights Reserved.                           */
/* ======================================================================== */

#ifndef _SWOSD_
#define _SWOSD_

#ifdef __cplusplus
extern "C" {
#endif

#include <ti/psp/iss/alg/evf/inc/tistdtypes.h>


/* Status Macros */
#define SWOSD_ERROR                          (-1)
#define SWOSD_OK                             (0)


/* SWOSD Macros */
#define SWOSD_MAX_FRAMES_PER_BLEND_FRAME      (16)
#define SWOSD_MAX_CHANNELS                    (48)
#define SWOSD_MAX_PLANES                      (2) // Y and C for YUV422
#define SWOSD_MAX_MEM_BLOCKS                  (16)
#define SWOSD_MAX_WINDOWS                     (8)
#define ALIGN_16                              (16)
#define ALIGN_128 							  (128)

#define SWOSD_PROFILE_TOTAL_TIME            (0)
#define SWOSD_PROFILE_GATHER_TOTAL_TIME     (1)
#define SWOSD_PROFILE_SCATTER_TOTAL_TIME    (2)
#define SWOSD_PROFILE_BLEND_TOTAL_TIME      (3)
#define SWOSD_PROFILE_EDMA_TOTAL_TIME       (4)
#define SWOSD_PROFILE_SIMCOP_TOTAL_TIME     (5)
#define SWOSD_PROFILE_SCATGATH_CPU_TIME     (6)
#define SWOSD_PROFILE_BLEND_CPU_TIME        (7)
#define SWOSD_MAX_PROFILE_LOG               (8)


/**
 *******************************************************************************
 *  @enum     SWOSD_DataFormat
 *  @brief    Supported Video color format.
 *
 *  @remarks  None
 *
 *******************************************************************************
**/
typedef enum
{
    SWOSD_FORMAT_YUV422I_YUYV = 0,
    SWOSD_FORMAT_YUV422I_UYVY = 1,
	SWOSD_FORMAT_YUV420SP_UV  = 2,
	SWOSD_FORMAT_DEFAULT = SWOSD_FORMAT_YUV420SP_UV

} SWOSD_DataFormat;

/**
 *******************************************************************************
 *  @struct   SWOSD_WindowPrm
 *  @brief    Window Parameters
 *
 *  transparency feature,
 *  - When transparency is enabled,
 *    - if Graphics window pixel value == 0x0000 then blending is not done for that pixel and video pixel is copied to output window
 *    - if Graphics window pixel value != 0x0000 then graphics window pixel is blended with video window pixel as usual
 *  - When transparency is disabled,
 *    - graphics window pixel is blended with video window pixel as usual
 *  @remarks  None
 *
 * Grpx window will always be in non-tiled memory
 * adjust height and starty for UV plane appropiately
 *******************************************************************************
**/
typedef struct
{
  Uint16 startX;      //  in pixels, relative to start of video window, must be multiple of 2
  Uint16 startY;      //  in lines, relative to start of video window
  Uint16 width;       //  in pixels, must be multiple of 4
  Uint16 height;      //  in lines, must be multiple of 2
  Uint8  alpha;               //  8-bit global Alpha Value, used only if Alpha window is not enabled
                              //                0: Min Alpha, show only video
                              //         255: Max Alpha, show only Graphics

  Uint32 lineOffset[SWOSD_MAX_PLANES];  //  in pixels, must be >= width, must be multiple of 4, recommended to be multiple of 32 for efficiency
  Ptr    graphicsWindowAddr[SWOSD_MAX_PLANES]; //  points to Graphic window address

} SWOSD_WindowPrm;

/**
 *******************************************************************************
 *  @struct   SWOSD_DynamicPrm
 *  @brief    Dynamic parameters.
 *
 *  @remarks  None
 *
 *******************************************************************************
**/
typedef struct {

    Uint32 numWindows;

    SWOSD_WindowPrm winPrm[SWOSD_MAX_WINDOWS];

} SWOSD_DynamicPrm;


/**
 *******************************************************************************
 *  @struct   SWOSD_StaticPrm
 *  @brief    Static parameters.
 *
 *  @remarks  None
 *
 *******************************************************************************
**/
typedef struct {

    Uint16 maxWidth;      		     //  Max width of Grpx window
    Uint16 maxHeight;     		     //  Max height of Grpx window
    SWOSD_DataFormat  dataFormat;    //  SWOSD_FORMAT_YUV422i / SWOSD_FORMAT_RGB565 / SWOSD_FORMAT_YUV420sp
    Bool             isTiledMem;
    Bool             isInterlaced;
    Uint32           videoLineOffset[SWOSD_MAX_PLANES];

}SWOSD_StaticPrm;

/**
 *******************************************************************************
 *  @struct   SWOSD_Obj
 *  @brief    SWOSD Object
 *
 *  @remarks  None
 *
 *******************************************************************************
**/
typedef struct {

  void *algHndl;                        /* algorithm handle */

  int (*acquire) (void *Handle);        /*Returns the dirty bit as status */
  void (*release) (void *Handle);

} SWOSD_Obj;

typedef struct
{
    Uint32 numChannels;
    Uint8 colorKey[SWOSD_MAX_PLANES]; /* 8-bit color key for Y and C */

    Bool transparencyEnable;          /* transparencyEnable for all channels
									    When transparency is enabled,
									      - if Graphics window pixel value == ColorKey
									           then blending is not done for that pixel and video pixel is copied to output window
									      - if Graphics window pixel value != ColorKey
									           then graphics window pixel is blended with video window pixel as usual
									    When transparency is disabled,
										  - graphics window pixel is blended with video window pixel as usual.
										    Colorkey has no existence */

    Uint8 useGlobalAlpha;           /* GlobalAlpha flag
                                       - 0 : disable global alpha
                                       - 1 : global alpha used for all channels */

    Uint8 globalAlphaValue;         /* global alpha value, 0-255
                                       - 255 : let see foreground
                                       - 0   : let see background */

    SWOSD_StaticPrm    chStaticPrm[SWOSD_MAX_CHANNELS];
    SWOSD_DynamicPrm   chDynamicPrm[SWOSD_MAX_CHANNELS];

    Uint32             maxFramePerBlendFrame;
    Uint32             maxWindowsPerCh;

    int (*acquire) (void *Handle);
    void (*release) (void *Handle);
} SWOSD_CreatePrm;

/**
 *******************************************************************************
 *  @struct   SWOSD_Frame
 *  @brief    Frame Object.
 *
 *  @remarks  None
 *
 *******************************************************************************
**/
typedef struct {

    Uint32 channelNum;
    Uint32 fid;
    Ptr    addr[SWOSD_MAX_PLANES];

} SWOSD_Frame;

/**
 *******************************************************************************
 *  @struct   SWOSD_BlendFramePrm
 *  @brief    Blend Frmae Object.
 *
 *  @remarks  None
 *
 *******************************************************************************
**/
typedef struct
{
    Uint32 numFrames;
    SWOSD_Frame frames[SWOSD_MAX_FRAMES_PER_BLEND_FRAME];

    UInt64 profileLog[SWOSD_MAX_PROFILE_LOG];

} SWOSD_BlendFramePrm;

/**
 *******************************************************************************
 *  @struct   SWOSD_MemAllocPrm
 *  @brief    Memory allocation parameters.
 *
 *  @remarks  None
 *
 *******************************************************************************
**/
typedef struct
{
    Uint32 numMemBlocks;
    Ptr    memBlockAddr[SWOSD_MAX_MEM_BLOCKS];
    Uint32 memBlockSize[SWOSD_MAX_MEM_BLOCKS];
    Uint32 memBlockAlign[SWOSD_MAX_MEM_BLOCKS];

} SWOSD_MemAllocPrm;

/**************************************************************/
/*           API declarations							      */
/**************************************************************/


/*
    Open API to initialize SWOSD object parameters
  Operations
    - Initializes algorithm handle
*/
Int32 SWOSD_open(SWOSD_Obj *pObj, SWOSD_CreatePrm *pPrm);

/*
    Get memory allocation required info from algorithm
*/
Int32 SWOSD_getMemAllocInfo(SWOSD_Obj *pObj, SWOSD_MemAllocPrm *pPrm);

/*
    API for user to give allocated pointers to the algorithm
*/
Int32 SWOSD_setMemAllocInfo(SWOSD_Obj *pObj, SWOSD_MemAllocPrm *pPrm);

/* Apply dynamic params for chNum */
Int32 SWOSD_setDynamicPrm(SWOSD_Obj *pObj, Uint32 chNum, SWOSD_DynamicPrm *pPrm);

/* Update Transparency enable flag and Color keys for all channels */
Int32 SWOSD_updateTransparencyEnableFlag(SWOSD_Obj *pObj, Bool transparencyEnable, Uint8 colorKey[SWOSD_MAX_PLANES]);

/* Update global alpha flag and  global alpha value */
Int32 SWOSD_updateGlobalAlpha(SWOSD_Obj *pObj, Bool useGlobalAlpha, Uint8 globalAlphaValue);

/*
 API to all frames blending. This API performs actual OSD functinality.
 Here the Video buffer pointer along with DDR pointers should
 be populated in pPrm by the user/application before calling this API
*/
Int32 SWOSD_blendFrames(SWOSD_Obj *pObj, SWOSD_BlendFramePrm *pPrm, int dirtyBit );

/* SWOSD close API which release the resource and deletes the handle */
Int32 SWOSD_close(SWOSD_Obj *pObj);



#ifdef __cplusplus
}
#endif

#endif //_SWOSD_

