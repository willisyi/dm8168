// ======================================================================== 
//																		
//									    
//  FILE NAME								    
//	  HELLOWORLDALG.h							    
//									   
//									    
//  DESCRIPTION                                                             
//	  Public header file for Hello World algorithm
//
// ------------------------------------------------------------------------ 
//         Copyright (c) 2011 Texas Instruments, Incorporated.           
//                           All Rights Reserved.                           
// ======================================================================== 
#ifndef HELLOWORLDALG_H_
#define HELLOWORLDALG_H_

#include <xdc/std.h>
#include <ti/xdais/ialg.h>

/////////////////////////////////////////////////////////////////////////////
//
// Types Declarations
//
/////////////////////////////////////////////////////////////////////////////

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


/////////////////////////////////////////////////////////////////////////////
//
// PUBLIC HELLOWORLDALG Enumerated Types
//
/////////////////////////////////////////////////////////////////////////////

typedef enum
{
	HELLOWORLDALG_NO_ERROR = 0,
	HELLOWORLDALG_ERR_INSTANCE_CREATE_FAILED = 1,
	HELLOWORLDALG_ERR_INPUT_INVALID = 2,
	HELLOWORLDALG_ERR_MEMORY_INSUFFICIENT = 3,
	HELLOWORLDALG_ERR_INPUT_INVALID_FRAME = 4,
} HELLOWORLDALG_Status;

/////////////////////////////////////////////////////////////////////////////
//
// PUBLIC HELLOWORLDALG DEFINED TYPES
//
/////////////////////////////////////////////////////////////////////////////

extern IALG_Fxns	HELLOWORLDALG_TI_IALG;

typedef struct
{
	S08   helloWorldResult;
    /**< HELLOWORLDALG result 1  */

    /**< HELLOWORLDALG result 2  */

} HELLOWORLDALG_Result;


/**
    \brief HELLOWORLDALG Channel parameters used to SET BEFORE CALLING PROCESS
*/
typedef struct
{
    U32 chId;
    /**< Unique video channel identifier, e.g. channel no. */

	U32 width;
    /**< Set the width (in pixels) of video frame that HELLOWORLDALG will process */

	U32 height;
    /**< Set the height (in pixels) of video frame that HELLOWORLDALG will process */

    U32 stride;
    /**< Set the video frame pitch/stride of the images in the video buffer*/

    PTR curFrame;
    /** Luma pointer to current frame */

    /** Any Other parameters for your algorithm per channel here */

} HELLOWORLDALG_chPrm;

/**
    \brief HELLOWORLDALG Algorithm parameters (CREATE)
*/
typedef struct
{
    U32  maxWidth;
    /**< Set the maximum width (in pixels) of video frame that HELLOWORLDALG will process */
    
    U32  maxHeight;
    /**< Set the maximum height (in pixels) of video frame that HELLOWORLDALG will process */
	
	U32  maxStride;
    /**< Set the maximum video frame pitch/stride of the images in the video buffer*/
    
    U32  maxChannels;
	/**< Set the maximum number of video channels that HELLOWORLDALG will monitor (Max is 16) */

	/**< Any other create time parameters for your algorithm specific need here */

	HELLOWORLDALG_chPrm	*chDefaultParams;
	/**< Pointer to array of channel params used to configure HELLOWORLDALG Algorithm. */
} HELLOWORLDALG_createPrm;

/////////////////////////////////////////////////////////////////////////////
//
// PUBLIC HELLOWORLDALG APPLICATION PROGRAMMING INTERFACE (API)
//
/////////////////////////////////////////////////////////////////////////////

HELLOWORLDALG_Status 
HELLOWORLDALG_TI_setPrms(PTR        handle,
			   HELLOWORLDALG_chPrm *pHELLOWORLDALGChPrm,
               U32        chanID);

HELLOWORLDALG_Status
HELLOWORLDALG_TI_process(PTR		   handle, 
			   U32		   chanID,
			   HELLOWORLDALG_Result  *pHELLOWORLDALGResult);


#endif /* HELLOWORLDALG_H_ */
