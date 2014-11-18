/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2012 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
    \ingroup SYSTEM_LINK_API
    \defgroup SYSTEM_DEBUG_API System Debug API

    Modify this file to enable/disable prints from different links.

    When SYSTEM_DEBUG_xxx_RT  is enabled a print is done for every frame.
    In real-time system such prints may slow down the system and hence
    are intended to used only for debug purposes.

    @{
*/

/**
    \file system_debug.h
    \brief System debug API
*/

#ifndef _SYSTEM_DEBUG_H_
#define _SYSTEM_DEBUG_H_

#ifdef  __cplusplus
extern "C" {
#endif

/* Define's */

/* @{ */

/**
   \brief  Macro to enable usage of Tiler 
*/
#if defined(TI_814X_BUILD) || defined(TI_8107_BUILD)
//#undef SYSTEM_USE_TILER
#define SYSTEM_USE_TILER    
#else
#define SYSTEM_USE_TILER
#endif
/**< Uncomment to Disable tiler at build time */
//#undef SYSTEM_USE_TILER

#if defined(TI8107_DVR)
/**
   \brief define this to use I2C from M3 side  
*/
#define SYSTEM_USE_VIDEO_DECODER
#endif

#if !defined(TI_8107_BUILD)
/**
   \brief RPE Integration with DSP 
*/
 #define DSP_RPE_AUDIO_ENABLE
#endif

#if defined(TI816X_CZ)
/**
   \brief define this to use I2C from M3 side  
*/
#define SYSTEM_USE_VIDEO_DECODER
#endif

#if defined(TI816X_ETV)
/**
   \brief define this to use I2C from M3 side
*/
#define SYSTEM_USE_VIDEO_DECODER
#endif

/** 
   \brief Define SYSTEM_DISABLE_AUDIO set by build options 
*/
#ifndef SYSTEM_DISABLE_AUDIO
#define SYSTEM_ENABLE_AUDIO
#endif

//#define SYSTEM_DEBUG_VIP_RES_ALLOC
//#define SYSTEM_DEBUG_TILER
//#define SYSTEM_DEBUG_CMD_ERROR

/** 
   \brief Define to enable debug of various links 
*/
#define SYSTEM_DEBUG
#define SYSTEM_DEBUG_CAPTURE
#define SYSTEM_DEBUG_NSF
#define SYSTEM_DEBUG_DEI
#define SYSTEM_DEBUG_DISPLAY
#define SYSTEM_DEBUG_DISPLAY_HWMS
#define SYSTEM_DEBUG_SCALAR
#define SYSTEM_DEBUG_NULL
#define SYSTEM_DEBUG_DUP
#define SYSTEM_DEBUG_SWMS
#define SYSTEM_DEBUG_SCLR
#define SYSTEM_DEBUG_IPC
#define SYSTEM_DEBUG_MERGE
#define SYSTEM_DEBUG_ENC
#define SYSTEM_DEBUG_DEC
#define SYSTEM_DEBUG_SCD
#define SYSTEM_DEBUG_SELECT
#define SYSTEM_DEBUG_GRPX

/** 
   \brief Define to get detailed avsync logs 
*/
#ifndef DDR_MEM_256M 
#define SYSTEM_DEBUG_AVSYNC_DETAILED_LOGS
#endif

//#define SYSTEM_DEBUG_MEMALLOC
//#define SYSTEM_DEBUG_DISABLE_PROCESS_MULTI
//#define SYSTEM_VERBOSE_PRINTS
/** \brief Enable real time debug of links */
//#define SYSTEM_DEBUG_RT

/* enabling avg log print for DM810x ONLY by default */
#if defined(TI_8107_BUILD)
#define SYSTEM_PRINT_RT_AVG_STATS_LOG
#endif

/** 
   \brief Minimum time to print log
*/
#define SYSTEM_RT_STATS_LOG_INTERVAL    (10) /* in secs */

/** 
   \brief Enable real time logs of links at various processing time instances - like input dequeing, procesing, output queueing etc
*/
#ifdef SYSTEM_DEBUG_RT
    #define SYSTEM_DEBUG_CAPTURE_RT
    #define SYSTEM_DEBUG_NSF_RT
    #define SYSTEM_DEBUG_DEI_RT
    #define SYSTEM_DEBUG_DISPLAY_RT
    #define SYSTEM_DEBUG_SCALAR_RT
    #define SYSTEM_DEBUG_NULL_RT
    #define SYSTEM_DEBUG_SWMS_RT
    #define SYSTEM_DEBUG_SCLR_RT
    #define SYSTEM_DEBUG_IPC_RT
    #define SYSTEM_DEBUG_ENC_RT
    #define SYSTEM_DEBUG_DEC_RT
#endif

/*@}*/

#ifdef  __cplusplus
}
#endif

#endif /* _SYSTEM_DEBUG_H_ */

/* @} */

/**
   \ingroup SYSTEM_LINK_API
   \defgroup SYSTEM_DEBUG_API System Debug API
*/
