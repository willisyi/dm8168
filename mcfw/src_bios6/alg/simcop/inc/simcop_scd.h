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
/*        simcop_scd.h -- SCD interface header file                         */
/*                                                                          */
/*     DESCRIPTION                                                          */
/*                SCD interface header file                                 */
/*                  														*/
/*                                                                          */
/* ------------------------------------------------------------------------ */
/*            Copyright (c) 2012 Texas Instruments, Incorporated.           */
/*                           All Rights Reserved.                           */
/* ======================================================================== */
#ifndef SCD_H_
#define SCD_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <ti/psp/iss/alg/evf/inc/tistdtypes.h>


/**
 *******************************************************************************
 * SCD Macros Definition
 *******************************************************************************
**/
#define SCD_MAX_MEM_BLOCKS 12

#define SCD_PROFILE_TOTAL_TIME  (0)
#define SCD_PROFILE_SIMCOP_TIME (1)
#define SCD_PROFILE_CPU_TIME    (2)
#define SCD_MAX_PROFILE_LOG     (3)

/**
 *******************************************************************************
 *  @enum     SCD_Sensitivity
 *  @brief    Sensitivity enum for SCD
 *
 *  @remarks  None
 *
 *******************************************************************************
**/
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

/**
 *******************************************************************************
 *  @enum     SCD_Status
 *  @brief    Staus enum for SCD
 *
 *  @remarks  None
 *
 *******************************************************************************
**/
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

/**
 *******************************************************************************
 *  @enum     SCD_Fps
 *  @brief    FPS enum for SCD
 *
 *  @remarks  None
 *
 *******************************************************************************
**/
typedef enum
{
	SCD_FPS_10	= 10,
	SCD_FPS_06	= 6,
	SCD_FPS_05	= 5,
	SCD_FPS_03	= 3,
	SCD_FPS_02	= 2,
	SCD_FPS_01	= 1
} SCD_Fps;

/**
 *******************************************************************************
 *  @enum     SCD_Output
 *  @brief    Output enum for SCD_process
 *
 *  @remarks  None
 *
 *******************************************************************************
**/
typedef enum
{
	SCD_DETECTOR_UNAVAILABLE =-1,
	SCD_DETECTOR_NO_CHANGE	 = 0,
	SCD_DETECTER_QUALIYING   = 1,
	SCD_DETECTOR_CHANGE		 = 2
} SCD_Output;

/**
 *******************************************************************************
 *  @enum     SCD_Mode
 *  @brief    SCD modes
 *
 *  @remarks  None
 *
 *******************************************************************************
**/
typedef enum
{
	SCD_DETECTMODE_DISABLE				= 0,
	SCD_DETECTMODE_MONITOR_FULL_FRAME	= 1,
	SCD_DETECTMODE_MONITOR_BLOCKS		= 2,
	SCD_DETECTMODE_MONITOR_BLOCKS_FRAME	= 3
} SCD_Mode;

/**
 *******************************************************************************
 *  @struct   SCD_Obj
 *  @brief    SCD object.
 *
 *  @remarks  None
 *
 *******************************************************************************
**/
typedef struct {

  /* algorithm handle */
  
  void  * algHndl;     
  
  /*Stores the pointer to the Acquire function which is used to acqire simcop resources */
  
  int  (*acquire) (void *Handle);     
  
  /*Stores the pointer to the Release function which is used to release simcop resources*/
  
  void (*release) (void *Handle);

} SCD_Obj;

/**
 *******************************************************************************
 *  @struct   SCD_MemAllocPrm
 *  @brief    Structure containing algorithm memory requirements
 *
 *  @remarks  None
 *
 *******************************************************************************
**/
typedef struct
{
	Uint32 numMemBlocks;
    Ptr    memBlockAddr[SCD_MAX_MEM_BLOCKS];
    Uint32 memBlockSize[SCD_MAX_MEM_BLOCKS];
    Uint32 memBlockAlign[SCD_MAX_MEM_BLOCKS];

} SCD_MemAllocPrm;


/**
 *******************************************************************************
 *  @struct   SCD_AlgImagebufs
 *  @brief    Algorithm image buffers
 *
 *  @remarks  None
 *
 *******************************************************************************
**/
typedef struct
{
	    Ptr pBkgrdMeanSQ8_7;      /* Mean Image  - frame size * 2 bytes */
	    Ptr pBkgrdVarianceSQ12_3; /* Motion History - frame size * 2 bytes */
	    Ptr pMHIimageUQ8_0;       /* Motion History Image - frame size bytes */
} SCD_AlgImagebufs;

/**
 *******************************************************************************
 *  @struct   SCD_CreatePrm
 *  @brief    SCD Create params
 *
 *  @remarks  None
 *
 *******************************************************************************
**/
typedef struct
{
	 /* Set the maximum width (in pixels) of video frame that scd will process */
    Uint32  maxWidth;

    /* Set the maximum height (in pixels) of video frame that scd will process */
    Uint32  maxHeight;

    /* Set the maximum video frame pitch/stride of the images in the video buffer */
	Uint32  maxPitch;
	
	/* Pointer to the Acquire function which acqires simcop resources */
	
	int (*acquire) (void *Handle);
	
	/* Pointer to the Release function which frees simcop resources */
	
    void (*release) (void *Handle);
	
} SCD_CreatePrm;

/**
 *******************************************************************************
 *  @struct   SCD_ProcessPrm
 *  @brief    SCD Process Params
 *
 *  @remarks  None
 *
 *******************************************************************************
**/
typedef struct {

    /* Channel Id */
	Uint32  chanId;

	Uint32  width;
    Uint32  height;
    Ptr     inAddr; // only Y plane is needed
    Ptr     prevInAddr; // only Y plane is needed
    Uint32  pitch;     /* in bytes - only Y plane is needed */

    SCD_AlgImagebufs  * pAlgImageBuf;

    /* Enable/Disable scd running on algo */
 	Uint32 mode;

 	/* internal threshold for scd change tolerance; globally applied to entire frame */
	Uint32 frmSensitivity;

	/* Set to 1 to ignore sudden, significantly brighter avg. scene luminance */
	Uint32 frmIgnoreLightsON;

	/* Set to 1 to ignore sudden, significantly darker avg. scene luminance */
	Uint32 frmIgnoreLightsOFF;

	/* Set Pre-defined processing rate (in frames per second) */
    Uint32 fps;

    /*Number of seconds to wait after a non tamper event */
    Uint32 numSecs2WaitAfterFrmAlert;

    UInt64 profileLog[SCD_MAX_PROFILE_LOG];

} SCD_ProcessPrm;


typedef struct {

    Int32      output;   // Tamper detect YES or NO
    Int32      status;   // error or success

} SCD_ProcessStatus;

typedef struct {

    /* Channel Id */
	Uint32  chanId;

	Uint32  width;
    Uint32  height;
    Ptr     inAddr; // only Y plane is needed
    Uint32  pitch;     /* in bytes - only Y plane is needed */

    SCD_AlgImagebufs  * pAlgImageBuf;

} SCD_InitMeanVarMHIPrm;

/**
 *******************************************************************************
  Open API to initialize SCD object parameters

  Operations
    - Initializes algorithm handle
 *******************************************************************************
 **/
Int32 SCD_open(SCD_Obj *pObj, SCD_CreatePrm *pPrm);


/**
 *******************************************************************************
 * SCD API which gives memory allocation required info from algorithm
 *
 * Arguments to the function
 *
 * - Handle of the algorithm
 * - pointer to the structure SCD_MemAllocPrm will return internal memory requirement
 *    for the algorithm
 * - pointer to the structure SCD_MemAllocPrm will return memory required by each channel for following buffers
 *     a)Mean Image
 *     b)Variance Image
 *     c)Motion History Image
 *     d)Update Mask from MHI
 *    In the same order
 *******************************************************************************
 **/
Int32 SCD_getAllocInfo(SCD_Obj *pObj,SCD_MemAllocPrm * pPrm, SCD_MemAllocPrm * pChnPrm);

/**
 *********************************************************************************
    API for user to give allocated pointers to the algorithm( for internal memory)
 *********************************************************************************
 **/
Int32 SCD_setAllocInfo(SCD_Obj *pObj,SCD_MemAllocPrm * pPrm);


/**
 *************************************************************************************
 SCD API which detection the scene change . This API performs actual SCD functinality.
 *************************************************************************************
 **/
Int32 SCD_process(SCD_Obj *pObj, SCD_ProcessPrm *pPrm, SCD_ProcessStatus *pStatus,Uint32 dirtybit);

/**
 *********************************************************************************
 * SCD API which release the resource and deletes the handle
 *********************************************************************************
**/
Int32 SCD_close(SCD_Obj *pObj);

/**
 ***********************************************************************************************
 *SCD API which Initialises MeanImage,VarianceImage,MHImage & UpdateMHmask from the current frame
 *first time for each channel this API must be called with a stable input frame
 *Before Calling this API following functions should be called
 * 1) CPIS_init
 * 2) SCD_open
 * 3) SCD_setAllocInfo
 ***********************************************************************************************
**/

Int32 SCD_initMeanVarMHI(SCD_Obj * pObj, SCD_InitMeanVarMHIPrm *pPrm,Uint32 dirtybit);

#ifdef __cplusplus
}
#endif
#endif /* SCD_H_ */
