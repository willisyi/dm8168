/*******************************************************************************
 *                                                                             *
 *  Copyright (c) 2011 Texas Instruments Incorporated - http://www.ti.com/     *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
    \ingroup LINK_API
    \defgroup VIDFRAME Video  data structure definition

    This file defines the data structure representing an encoded video frame's
     object

    @{
*/

/**
    \file vidframe.h
    \brief Definition of encoded video frame data structures
*/

#ifndef _VIDFRAME_H_
#define _VIDFRAME_H_

/**
 * @brief  Buffer alignment needed for IVA-HA codecs
 */

/**
 * @def    VIDFRAME_MAX_FRAME_BUFS
 * @brief  Maximum number of frame buf in a Frame_BufList @sa Frame_BufList
 */
#define VIDFRAME_MAX_FRAME_BUFS  (16)

/**
 * @def    VIDFRAME_MAX_FIELDS
 * @brief  Maximum number of fields in a frame
 */
#define VIDFRAME_MAX_FIELDS      (2u)

/**
 * @def    VIDFRAME_MAX_PLANES
 * @brief  Maximum number of video place (Y/U/V or R/G/B)
 */
#define VIDFRAME_MAX_PLANES      (3u)
/**
    \brief Video frame buffer
*/
typedef struct VIDFrame_Buf {
    UInt32 reserved[2];
    /**< First two 32 bit entries are reserved to allow use as Que element */
    Ptr    addr[VIDFRAME_MAX_FIELDS][VIDFRAME_MAX_PLANES];
    /**< virtual address of vid frame buffer pointers */
    Ptr    phyAddr[VIDFRAME_MAX_FIELDS][VIDFRAME_MAX_PLANES];
    /**< virtual address of vid frame buffer pointers */
    UInt32 channelNum;
    /**< Coding type */
    UInt32 timeStamp;
    /**< Time stamp */
    UInt32 fid;
    /**< Field indentifier (TOP/BOTTOM/FRAME) */
    UInt32 frameWidth;
    /**< Width of the frame */
    UInt32 frameHeight;
    /**< Height of the frame */
    UInt32 framePitch[VIDFRAME_MAX_PLANES];
    /**< Pitch of the frame */
    Ptr    linkPrivate;
    /**< Link private info. Application should preserve this value and not overwrite it */
} VIDFrame_Buf;

/**
    \brief Video frame buffer
*/
typedef struct VIDFrame_BufList {
    UInt32 numFrames;
    /**< Number of valid VIDFrame_Buf entries */
    VIDFrame_Buf  frames[VIDFRAME_MAX_FRAME_BUFS];
    /**< Array of VIDFrame_Buf structures */

} VIDFrame_BufList;

/** \brief Enum for Video frame field identifier */
typedef enum
{
    VIDFRAME_FID_TOP = 0,
    /**< Top field. */
    VIDFRAME_FID_BOTTOM,
    /**< Bottom field. */
    VIDFRAME_FID_FRAME,
    /**< Frame mode - Contains both the fields or a progressive frame. */
    VIDFRAME_FID_MAX
    /**< Should be the last value of this enumeration.
         Will be used by driver for validating the input parameters. */
} VIDFrame_Fid;

#endif /* _VIDFRAME_H_*/

/** @}*/

