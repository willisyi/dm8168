/*
 *  Copyright 2008
 *  Texas Instruments Incorporated
 *
 *  All rights reserved.  Property of Texas Instruments Incorporated
 *  Restricted rights to use, duplicate or disclose this code are
 *  granted through contract.
 *
 */

#ifndef _SWOSD_
#define _SWOSD_

#ifdef __cplusplus
extern "C" {
#endif




#define SWOSD_MAX_WIDTH  720  ///< Max supported window width

#define SWOSD_SOK    0  ///< Status : OK
#define SWOSD_EFAIL  -1 ///< Status : Generic error

#define SWOSD_FORMAT_YUV422i     0   ///< Data format : YUV422 interleaved
#define SWOSD_FORMAT_RGB565      1   ///< Data format : RGB565
#define SWOSD_FORMAT_YUV420sp    2   ///< Data format : YUV420 semi planar

/**
  \brief Open parameters
*/
typedef struct {

  UInt16 maxWidth;      ///< Max width
  UInt16 maxHeight;     ///< Max height

} SWOSD_OpenPrm;

/**
  \brief Global Parameters

  Transperency feature,
  - When transperency is enabled,
    - if Graphics window pixel value == 0x0000 then blending is not done for that pixel and video pixel is copied to output window
    - if Graphics window pixel value != 0x0000 then graphics window pixel is blended with video window pixel as usual
  - When transperency is disabled,
    - graphics window pixel is blended with video window pixel as usual

*/
typedef struct {

  UInt8  globalAlpha;         ///< 8-bit global Alpha Value, used only if Alpha window is not enabled, set to 0 to disable alpha blend \n 0: Min Alpha, show only video, 128: Max Alpha, show only Graphics
  Bool   transperencyEnable;  ///< TRUE: enable transperency, FALSE: disable transperency
  UInt32 transperencyColor32; ///< 32-bit transperency color
} SWOSD_GlobalPrm;

/**
  \brief Graphics window parameter's
*/
typedef struct {
  Int32 format;      ///< SWOSD_FORMAT_YUV422i / SWOSD_FORMAT_RGB565 / SWOSD_FORMAT_YUV420sp
  Int16 startX;      ///< in pixels, relative to start of video window, must be multiple of 2
  Int16 startY;      ///< in lines, relative to start of video window
  Int16 width;       ///< in pixels, must be multiple of 4
  Int16 height;      ///< in lines
  Int32 lineOffset;  ///< in pixels, must be >= width, must be multiple of 4, recommended to be multiple of 32 for efficiency

} SWOSD_WindowPrm;

typedef struct {

  void *algHndl;

  SWOSD_OpenPrm   openPrm;
  SWOSD_GlobalPrm globalPrm;
  SWOSD_WindowPrm videoWindowPrm;
  SWOSD_WindowPrm graphicsWindowPrm;

  void *videoWindowAddr;
  void *graphicsWindowAddr;
  void *alphaWindowAddr;

} SWOSD_Obj;

Int32 SWOSD_open(SWOSD_Obj *pObj, SWOSD_OpenPrm *openPrm);
Int32 SWOSD_close(SWOSD_Obj *pObj);

Int32 SWOSD_blendWindow(SWOSD_Obj *pObj);

#ifdef __cplusplus
}
#endif

#endif //_SWOSD_

