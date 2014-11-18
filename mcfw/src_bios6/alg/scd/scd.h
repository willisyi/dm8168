// ======================================================================== 
//																			
//  AUTHORS         					  	            
//    Darnell J. Moore	(DJM)			       		            
//									    
//  CONTACT                						    
//	  djmoore@ti.com, +1 214 480 7422
//    DSP Solutions R&D Center, Dallas, TX
//    Video & Image Processing Laboratory
//    Embedded Vision Branch
//									    
//  FILE NAME								    
//	  scd.h							    
//									    
//  REVISION HISTORY                                                        
//    Oct 14, 2011 * DJM created
//    Dec  5, 2011 * DJM added support for motion detection
//    Apr 11, 2012 * DJM added support for frame-based process flow and made
//					 various bug fixes and improvements
//
//  DESCRIPTION                                                             
//	  Public header file for Scene Change Detection algorithm, which 
//    enables the following:
//    a) detection of change/motion in individual image tiles 
//    b) detection of global changes to scene consistent with basic camera
//       tampering events, e.g. fully blocking lens, blinding with flashlight,
//       etc.
//
// ------------------------------------------------------------------------ 
//         Copyright (c) 2011 Texas Instruments, Incorporated.           
//                           All Rights Reserved.                           
// ======================================================================== 
#ifndef SCD_H_
#define SCD_H_

#ifdef __cplusplus
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////
//
// Types Declarations
//
/////////////////////////////////////////////////////////////////////////////
#ifdef WIN32
#include "common_types.h"
#include "framework_components_PCbridge.h"
#else
typedef char					bool;
typedef char   					S08;
typedef unsigned char   		U08;
typedef signed short    		S16;
typedef unsigned short  		U16;
typedef signed int      		S32;
typedef unsigned int    		U32;
typedef float           		F32;
typedef long long				S64;
typedef unsigned long long		U64;
typedef void *					PTR;
#endif

/////////////////////////////////////////////////////////////////////////////
//
// PUBLIC SCD Enumerated Types
//
/////////////////////////////////////////////////////////////////////////////
typedef enum {
	SCD_SENSITIVITY_MIN		 =  0,
	SCD_SENSITIVITY_VERYLOW	 =  1,
	SCD_SENSITIVITY_LOW		 =  2,
	SCD_SENSITIVITY_MIDLO	 =  3,
	SCD_SENSITIVITY_MID		 =  4,
	SCD_SENSITIVITY_MIDHI	 =  5,
	SCD_SENSITIVITY_HIGH	 =  6,
	SCD_SENSITIVITY_VERYHIGH =  7,
	SCD_SENSITIVITY_MAX		 =  8
} SCD_Sensitivity;

typedef enum
{
	SCD_NO_ERROR = 0,
	SCD_ERR_INSTANCE_CREATE_FAILED = 1,
	SCD_ERR_INPUT_INVALID = 2,
	SCD_ERR_INPUT_INVALID_FRAME = 4,
	SCD_ERR_INPUT_NEGATIVE = 8,
	SCD_ERR_INPUT_EXCEEDED_RANGE = 16,
	SCD_ERR_MEMORY_EXCEEDED_BOUNDARY = 32,
	SCD_ERR_MEMORY_INSUFFICIENT = 64,
	SCD_ERR_MEMORY_POINTER_NULL = 128,
	SCD_ERR_INTERNAL_FAILURE = 256,
	SCD_WARNING_LOW_MEMORY = 512,
	SCD_WARNING_INITIALIZING = 1024,
	SCD_WARNING_PARAMETER_UNDERSPECIFIED = 2048,
	SCD_WARNING_DISABLED = 4096
} SCD_Status;

typedef enum
{
	SCD_FPS_10	= 10,
	SCD_FPS_06	= 6,
	SCD_FPS_05	= 5,
	SCD_FPS_03	= 3,
	SCD_FPS_02	= 2,
	SCD_FPS_01	= 1
} SCD_Fps;

typedef enum
{
	SCD_DETECTOR_UNAVAILABLE =-1,
	SCD_DETECTOR_NO_TAMPER	 = 0,
	SCD_DETECTOR_QUALIFYING	 = 1,
	SCD_DETECTOR_TAMPER		 = 2
} SCD_Output;

typedef enum
{
	SCD_DETECTMODE_DISABLE				= 0,
	SCD_DETECTMODE_MONITOR_FULL_FRAME	= 1,
	SCD_DETECTMODE_MONITOR_BLOCKS		= 2,
	SCD_DETECTMODE_MONITOR_BLOCKS_FRAME	= 3
} SCD_Mode;

/////////////////////////////////////////////////////////////////////////////
//
// PUBLIC SCD DEFINED TYPES
//
/////////////////////////////////////////////////////////////////////////////

/**
    \brief SCD Block Change parameters
*/
typedef struct
{
	SCD_Sensitivity sensitivity;
	/**< Blocks's sensitivity setting for change detection */

	U32 monitored;
	/**< Flag indicates whether to monitor block for change detection*/
} SCD_blkChngConfig;

typedef struct
{
	U32 numFrmsBlkChanged;
	/**< Number of consecutive frames with motion in block*/

	U32 numPixelsChanged;
	/**< Raw number of block pixels to change*/
} SCD_blkChngMeta;

typedef struct
{
	S32 sumModelPixelVal;
	S32 sumCurFrmPixelVal;
	S32 numBSUBpixels;
	S32 numIFDpixels;
	S32 frmNumEdges;

	SCD_Output	       frmResult;
    /**< SCD change detection result from entire frame */

	SCD_blkChngMeta	  *blkResult;   
	/**< SCD change detection result from individual frame tiles/blocks 
	     Allocated at app level (outside algorithm) in AlgLink_ScdChStatus; 
	     array length = ALG_LINK_SCD_MAX_BLOCKS_IN_FRAME */
} SCD_Result;

/**
    \brief SCD Channel parameters used to SET BEFORE CALLING PROCESS
*/
typedef struct
{
    U32 chId;
    /**< Unique video channel identifier, e.g. channel no. */

 	SCD_Mode mode;
    /**< Enable/Disable scd running on algo*/

	U32 width;
    /**< Set the width (in pixels) of video frame that scd will process */

	U32 height;
    /**< Set the height (in pixels) of video frame that scd will process */

    U32 stride;
    /**< Set the video frame pitch/stride of the images in the video buffer*/

    PTR curFrame;
    /** Luma pointer to current frame */

	SCD_Sensitivity frmSensitivity;
    /**< internal threshold for scd change tolerance; globally applied to entire frame */

	U32 frmIgnoreLightsON;
	/**< Set to 1 to ignore sudden, significantly brighter avg. scene luminance */

	U32 frmIgnoreLightsOFF;
	/**< Set to 1 to ignore sudden, significantly darker avg. scene luminance */

	U32 frmEdgeThreshold;
	/**< Set minimum number of edge pixels required to indicate non-tamper event */
	
	SCD_blkChngConfig	*blkConfig;
	/**< Linear array of pointers referencing 2D matrix of frame blocks that
		 scd will monitor for motion detection (configuration) */
} SCD_chPrm;

/**
    \brief SCD Algorithm parameters (CREATE)
*/
typedef struct
{
    U32  maxWidth;
    /**< Set the maximum width (in pixels) of video frame that scd will process */
    
    U32  maxHeight;
    /**< Set the maximum height (in pixels) of video frame that scd will process */
	
	U32  maxStride;
    /**< Set the maximum video frame pitch/stride of the images in the video buffer*/
    
    U32  maxChannels;
	/**< Set the maximum number of video channels that SCD will monitor (Max is 16) */

	U32 numSecs2WaitB4Init;
	/**< Set the number of seconds to wait before initializing SCD monitoring. 
		 This wait time occurs when the algorithm instance first starts and 
		 allows video input to stabilize.*/

	U32 numSecs2WaitB4FrmAlert;
	/**< Set the number of seconds to wait before signaling a frame-level scene change event.*/

	U32 numSecs2WaitAfterFrmAlert;
	/**< Set to 1 to n for the maximum number of seconds to wait for pre-tamper 
	     conditions to return following a tamper event */

    SCD_Fps fps;
    /**< Set Pre-defined processing rate (in frames per second) */

	SCD_chPrm	*chDefaultParams;
	/**< Pointer to array of channel params used to configure SCD Algorithm. */
} SCD_createPrm;

/////////////////////////////////////////////////////////////////////////////
//
// PUBLIC SCD APPLICATION PROGRAMMING INTERFACE (API)
//
/////////////////////////////////////////////////////////////////////////////

SCD_Status 
SCD_TI_setPrms(PTR        handle,
			   SCD_chPrm *pScdChPrm,
               U32        chanID);
SCD_Status
SCD_TI_process(PTR		   handle, 
			   U32		   chanID,
			   SCD_Result  *pScdResult);

SCD_Status
SCD_TI_getChannelTamperState(PTR         algHndl, 
							 U32         chanID,
							 SCD_Output *state);

SCD_Status
SCD_TI_resetChannel(PTR         algHndl, 
					U32         chanID);
#ifdef __cplusplus
}
#endif
#endif /* SCD_H_ */
