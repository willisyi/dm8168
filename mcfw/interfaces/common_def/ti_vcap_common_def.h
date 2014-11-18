/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2012 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
    \ingroup MCFW_VCAP_API

    @{
*/

/**
    \file ti_vcap_common_def.h
    \brief McFW Video Capture (VCAP) API- Common definitions and data structures
*/

#ifndef __TI_VCAP_COMMON_DEF_H__
#define __TI_VCAP_COMMON_DEF_H__

/* Include's    */

/** Maximum Video Capture channels */
#define VCAP_CHN_MAX        (16)

/* Data structure's */

/**
    \brief video capture status information for channel
*/

typedef struct
{
  UInt32 chId;
  /**< channel ID */

  UInt32 vipInstId;
  /**< VIP instance ID */

  UInt32 isVideoDetect;
  /**<  TRUE: Video signal is present, FALSE: no video signal */

  UInt32 frameWidth;
  /**< Frame or field width in pixels
   *
   * This is detected video signal frame or field width.
   *
   * Further change in width or height due to additional
   * cropping, scaling like CIF, HALF-D1 is not accoutned for in this field
  */

  UInt32 frameHeight;
  /**< Frame or field height in lines
   *
   * This is detected video signal frame or field height.
   *
   * Further change in width or height due to additional
   * cropping, scaling like CIF, HALF-D1 is not accoutned for in this field
  */

  UInt32 frameInterval;
  /**< Interval between two fields or frames in micro-seconds */

  UInt32 isInterlaced;
  /**< TRUE: Source is Interlaced, FALSE: Source is Progressive */

} VCAP_VIDEO_SOURCE_CH_STATUS_S;

/**
    \brief video source status parameters
*/

typedef struct {

    UInt32 channelNum;
    /**< channel number */
} VCAP_VIDEO_SOURCE_STATUS_PARAMS_S;

/**
    \brief video source status information
*/

typedef struct {

    UInt32 numChannels;
    /**< numbers of channel need to get video source status */
    VCAP_VIDEO_SOURCE_CH_STATUS_S chStatus[VCAP_CHN_MAX];
    /**< video source status for each channel */

} VCAP_VIDEO_SOURCE_STATUS_S;



#endif


/* @} */
