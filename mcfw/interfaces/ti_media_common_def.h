/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
    \ingroup MCFW_API
    \defgroup MCFW_COMMON_API McFW Common Const's and Data structure's used by all McFW Sub-system's

    @{
*/

/**
    \file ti_media_common_def.h
    \brief McFW Common Const's and Data structure's used by all McFW Sub-system's
*/

#ifndef __TI_MEDIA_COMMON_DEF_H__
#define __TI_MEDIA_COMMON_DEF_H__

#include "ti_media_std.h"
#include "ti_media_error_def.h"
#include "common_def/ti_venc_common_def.h"

/* =============================================================================
 * Defines
 * =============================================================================
 */

/**
    Maximum Number of Bistream buffers in VCODEC_BITSBUF_LIST_S
*/
#define     VCODEC_BITSBUF_MAX             (64)

/**
    Maximum Number of video frame buffers in VIDEO_FRAMEBUF_LIST_S
*/
#define     VIDEO_FRAMEBUF_MAX             (16)

/**
    Maximum Number of alg frame result buffers in ALG_FRAMERESULTBUF_LIST_S
*/
#define     ALG_FRAMERESULTBUF_MAX         (16)

/**
    Maximum Number of video fields in VIDEO frame (TOP/BOTTOM)
*/
#define     VIDEO_MAX_FIELDS               (2)

/**
    Maximum Number of video planes in video frame (Y/U/V)
*/
#define     VIDEO_MAX_PLANES               (3)

/**
 * Maximum  Number of input channels to a MCFW Module
 */
#define     VIDEO_MAX_NUM_CHANNELS         (64)

/**
    Const to use when infinite timeout is needed
*/
#define     TIMEOUT_WAIT_FOREVER           ((UInt32)-1)

/**
    Const to use when NO timeout is needed
*/
#define     TIMEOUT_NO_WAIT                ((UInt32)0)

/** Maximum Blocks per Frame */
#define VALG_MAX_BLOCKS_IN_FRAME            (264)

/**
    Capture Channel ID Data Type, can take values from 0..(VCAP_CHN_MAX-1)
*/
#define VCAP_CHN    UInt32

/**
    Capture Stream ID Data Type, can take values from 0..(VCAP_STRM_MAX-1)
*/
#define VCAP_STRM   UInt32

/**
    Alg Channel ID Data Type, can take values from 0..(VALG_CHN_MAX-1)
*/
#define VALG_CHN    UInt32

/**
    Encode Channel ID Data Type, can take values from 0..(VENC_CHN_MAX-1)
*/
#define VENC_CHN    UInt32

/**
    Encode Stream ID Data Type, can take values from 0..(VENC_STRM_MAX-1)
*/
#define VENC_STRM   UInt32

/**
    Decode Channel ID Data Type, can take values from 0..(VDEC_CHN_MAX-1)
*/
#define VDEC_CHN    UInt32

/**
    Display Channel ID Data Type, can take values from 0..(VDIS_CHN_MAX-1)
*/
#define VDIS_CHN    UInt32

/**
    Display Mosaic Window ID Data Type, can take values from 0..(VDIS_MOSAIC_WIN_MAX-1)
*/
#define VDIS_WIN    UInt32

/**
    Spl channel number indicating API applies to all display channels
*/
#define VDIS_CHN_ALL      (VIDEO_MAX_NUM_CHANNELS + 1)


/* =============================================================================
 * Enums
 * =============================================================================
 */

/**
    \brief Video Data Format's
*/
typedef enum VIDEO_FORMATE_E
{
    VF_YUV422I_UYVY = 0,
    /**< YUV 422 Interleaved format - UYVY. */

    VF_YUV422I_YUYV,
    /**< YUV 422 Interleaved format - YUYV. */

    VF_YUV422I_YVYU,
    /**< YUV 422 Interleaved format - YVYU. NOT USED */

    VF_YUV422I_VYUY,
    /**< YUV 422 Interleaved format - VYUY. NOT USED */

    VF_YUV422SP_UV,
    /**< YUV 422 Semi-Planar - Y separate, UV interleaved. */

    VF_YUV422SP_VU,
    /**< YUV 422 Semi-Planar - Y separate, VU interleaved. NOT USED */

    VF_YUV422P,
    /**< YUV 422 Planar - Y, U and V separate. NOT USED */

    VF_YUV420SP_UV,
    /**< YUV 420 Semi-Planar - Y separate, UV interleaved. */

    VF_YUV420SP_VU,
    /**< YUV 420 Semi-Planar - Y separate, VU interleaved. NOT USED */

    VF_YUV420P,
    /**< YUV 420 Planar - Y, U and V separate. NOT USED */

} VIDEO_FORMATE_E;

/**
    \brief Video Scan Type - Progressive or Interlaced or Auto-detect
*/
typedef enum  VIDEO_TYPE_E //
{
    PROGRESSIVE =0,
    /**< Progressive mode. */

    INTERLACED,
    /**< Interlaced mode. */

    VT_AUTO_DETECT
    /**< Auto Detect mode */

} VIDEO_TYPE_E;

/**
    \brief Video System Standard - NTSC or PAL or Auto-detect
*/
typedef enum  VIDEO_STANDARD_E
{
    NTSC = 0,
    /**< NTSC mode. */

    PAL,
    /**< PAL mode. */

    VS_AUTO_DETECT
    /**< Auto Detect mode */

} VIDEO_STANDARD_E;

/**
    \brief Noise Filter Setting - ON or OFF or AUTO
*/
typedef enum NF_FLAG_E
{
    NF_FALSE =0,
    /**< Noise Filter is OFF */

    NF_TRUE,
    /**< Noise Filter is ON */

    NF_AUTO,
    /**< Noise Filter is in auto-adjustment mode */

} NF_FLAG_E;

/**
    \brief Encoder is frame based (progressive) encode or field based (interlaced) encode or auto decided by framework
*/
typedef enum ENCODER_TYPE_E
{
    FIELD_BASED = 0,
    /**< Field based encoder */

    FRAME_BASED,
    /**< Frame based encoder */

    ET_AUTO,
    /**< Decide by framework based on system use-case */

} ENCODER_TYPE_E;

/**
    \brief Encoded frame type
*/
typedef enum {

    VCODEC_FRAME_TYPE_I_FRAME,
    /**< I-frame or Key Frame */

    VCODEC_FRAME_TYPE_P_FRAME,
    /**< P-frame */

    VCODEC_FRAME_TYPE_B_FRAME,
    /**< B-frame */

} VCODEC_FRAME_TYPE_E;

/**
    \brief Encoded frame type
*/
typedef enum {

    VIDEO_FID_TYPE_TOP_FIELD,
    /**< Top field of video frame */

    VIDEO_FID_TYPE_BOT_FIELD,
    /**< Bot field of video frame */

    VIDEO_FID_TYPE_FRAME = VIDEO_FID_TYPE_TOP_FIELD,
    /**< Progressive frame.
     *   Progressive frame is assigned the same FID
     *   as top field (0) which is waht is expected
     *   by low level drivers
     **/
} VIDEO_FID_TYPE_E;

/* =============================================================================
 * Structures
 * =============================================================================
 */


/**
    \brief Window Position Data structure

    Used to specific only position of a window without width and height
*/
typedef struct POSITION_S
{
    UInt32 start_X;
    /**< Horizontal offset in pixels, from which picture needs to be positioned. */

    UInt32 start_Y;
    /**< Vertical offset from which picture needs to be positioned. */

} POSITION_S;

/**
    \brief Video Bitstream Buffer Information

    This structure is used to get information of encoded frames from VENC
    OR to pass information of encoded frames to VDEC
*/
typedef struct {

    UInt32              reserved;
    /**< Used internally, USER MUST NOT MODIFY THIS VALUE */

    VENC_CHN            chnId;
    /**< Encoder/Decoder channel ID 0..(VENC_CHN_MAX-1)  */

    VENC_STRM           strmId;
    /**< Encoder stream ID, not valid for decoder, 0..(VENC_STRM_MAX-1)  */

    VCODEC_TYPE_E       codecType;
    /**< Video compression format */

    VCODEC_FRAME_TYPE_E frameType;
    /**< Compressed frame type */

    UInt32              bufSize;
    /**< Size of buffer, in bytes */

    UInt32              filledBufSize;
    /**< Actual size of bistream in buffer, in bytes */

    UInt32              mvDataOffset;
    /**< Actual offset to mv data bistream in buffer, in bytes */

    UInt32              mvDataFilledSize;
    /**< Actual size of mv data bistream in buffer, in bytes */

    Void               *bufVirtAddr;
    /**< User Virtual Space Buffer Address, can be used with write() or fwrite() APIs */

    Void               *bufPhysAddr;
    /**< User System Physical Buffer Address, can be used with EDMA APIs */

    UInt32              timestamp;
    /**< Capture or Display Frame timestamp, in msec */

    UInt32              upperTimeStamp;
    /**< Original Capture time stamp:Upper 32 bit value*/

    UInt32              lowerTimeStamp;
    /**< Original Capture time stamp: Lower 32 bit value*/

    UInt32              encodeTimestamp;
    /**< Time stamp after encode is complete */

    UInt32              numTemporalLayerSetInCodec;
    /**< numTemporalLayer which has been configured in 
        * the create time of h264 encoder */

    UInt32              temporalId;
    /**< SVC TemporalId */

    UInt32              fieldId;
    /**< 0: Even field or Frame based, 1: Odd Field */

    UInt32              frameWidth;
    /**< Width of frame, in pixels */

    UInt32              frameHeight;
    /**< Height of frame, in lines */

    UInt32              doNotDisplay;
    /**< Flag indicating frame should not be displayed
     *   This is useful when display should start from a
     *   particular frame.
     *   This is temporary until Avsync suuports seek functionality*/
     
     UInt32 bottomFieldBitBufSize;
     /**< Size of the bottom field Bitstream. Filled by field Merged
      *   interlaced encoders     */
     UInt32 seqId;
    /**< Sequence Id associated with this frame. The video 
      *  backend will display a frame only if current sequenceid
      *  configured in Vdis matches the seqId associated with the frame
      *  The current sequence id (called display sequence id) can be 
      *  set when invoking a Vdis API such as PLAY/TPLAY/SCAN etc.
      *  This feature enables flushing of frames queued withing mcfw 
      *  when application switches from one play state to another 
      *  such scan -> play and does not want to display the older
      *  scan mode frames in play state
      */
     UInt32 inputFileChanged;
    /**< This need to be set to 1 only for the first frame, if the input 
         file is changed. This info is required for the decoder to start 
         display from an IDR frame. IF this is set decoder skip all frames 
         till it find the first IDR frame and avoid any garbage display */
          
} VCODEC_BITSBUF_S;

/**
    \brief Video Bitstream Buffer List

    List of Video Bitstream Buffer's allows user to efficient exchange
    multiple frames with VENC, VDEC sub-system with less overhead
*/
typedef struct {

    UInt32              numBufs;
    /**< Number of valid frame's in bitsBuf[]. MUST be <= VCODEC_BITSBUF_MAX */

    VCODEC_BITSBUF_S    bitsBuf[VCODEC_BITSBUF_MAX];
    /**< Bistream information list */

} VCODEC_BITSBUF_LIST_S;

/**
    \brief Video Frame Buffer Information

    This structure is used to get information of video frames from VCAP
    OR to pass information of video frames to VDIS
*/
typedef struct {

    Ptr    addr[VIDEO_MAX_FIELDS][VIDEO_MAX_PLANES];
    /**< virtual address of vid frame buffer pointers */

    Ptr    phyAddr[VIDEO_MAX_FIELDS][VIDEO_MAX_PLANES];
    /**< virtual address of vid frame buffer pointers */

    VCAP_CHN channelNum;
    /**< Coding type */

    UInt32 timeStamp;
    /**< Capture or Display time stamp */

    UInt32 fid;
    /**< Field indentifier (TOP/BOTTOM/FRAME) */

    UInt32 frameWidth;
    /**< Width of the frame */

    UInt32 frameHeight;
    /**< Height of the frame */
    
    UInt32 framePitch[VIDEO_MAX_PLANES];
    /**< Pitch of the frame */

    Ptr    linkPrivate;
    /**< Link private info. Application should preserve this value and not overwrite it */

} VIDEO_FRAMEBUF_S;

/**
    \brief Video Bitstream Buffer List

    List of Video Bitstream Buffer's allows user to efficient exchange
    multiple frames with VENC, VDEC sub-system with less overhead
*/
typedef struct {

    UInt32              numFrames;
    /**< Number of valid frame's in frames[]. MUST be <= VIDEO_FRAMEBUF_MAX */

    VIDEO_FRAMEBUF_S    frames[VIDEO_FRAMEBUF_MAX];
    /**< Video frame information list */

} VIDEO_FRAMEBUF_LIST_S;



/* @} */

/* Data structure's */

/**
    \brief Block configuration - part of AlgLink_ScdChParams
*/
typedef struct
{
    UInt32 sensitivity;
    /**< Blocks's sensitivity setting for change detection
            For valid values see \ref AlgLink_ScdSensitivity
         */

    UInt32 monitored;
    /**< TRUE: monitor block for change detection
        FALSE: ignore this block
    */
} VALG_FRAMERESULTBUF_SCDBLKCONFIG_S;

/**
    \brief SCD status of a block - part of AlgLink_ScdChStatus
*/
typedef struct
{
    UInt32 numFrmsBlkChanged;
    /**< Number of consecutive frames with motion in block */

    UInt32 numPixelsChanged;
    /**< Raw number of block pixels that were changed */

} VALG_FRAMERESULTBUF_SCDBLKCHNGMETA_S;


/**
    \brief Video Bitstream Buffer Information

    This structure is used to get information of encoded frames from VENC
    OR to pass information of encoded frames to VDEC
*/
typedef struct {

    UInt32              reserved;
    /**< Used internally, USER MUST NOT MODIFY THIS VALUE */

    VALG_CHN            chnId;
    /**< Encoder/Decoder channel ID 0..(VENC_CHN_MAX-1)  */

    UInt32              bufSize;
    /**< Size of buffer, in bytes */

    UInt32              filledBufSize;
    /**< Actual size of bistream in buffer, in bytes */

    Void               *bufVirtAddr;
    /**< User Virtual Space Buffer Address, can be used with write() or fwrite() APIs */

    Void               *bufPhysAddr;
    /**< User System Physical Buffer Address, can be used with EDMA APIs */

    UInt32              timestamp;
    /**< Capture or Display Frame timestamp, in msec */

    UInt32              upperTimeStamp;
    /**< Original Capture time stamp:Upper 32 bit value*/

    UInt32              lowerTimeStamp;
    /**< Original Capture time stamp: Lower 32 bit value*/

    UInt32              frameWidth;
    /**< Width of frame, in pixels */

    UInt32              frameHeight;
    /**< Height of frame, in lines */

    UInt32              frmResult;
    /**< SCD change detection result from entire frame */

    VALG_FRAMERESULTBUF_SCDBLKCONFIG_S blkConfig[VALG_MAX_BLOCKS_IN_FRAME];
    /**< Linear array of configuration of frame blocks that
         scd will monitor for motion detection (configuration)
    */

    VALG_FRAMERESULTBUF_SCDBLKCHNGMETA_S  blkResult[VALG_MAX_BLOCKS_IN_FRAME];
    /**< SCD change detection result from individual frame tiles/blocks
         array length = ALG_LINK_SCD_MAX_BLOCKS_IN_FRAME
    */

} VALG_FRAMERESULTBUF_S;

/**
    \brief Alg Frame result Buffer List

    List of Alg Frame Result Buffer's allows user to efficient exchange
    multiple frames with alg sub-system with less overhead
*/
typedef struct {

    UInt32                  numBufs;
    /**< Number of valid frame's in frames[]. MUST be <= VIDEO_FRAMEBUF_MAX */

    VALG_FRAMERESULTBUF_S    bitsBuf[ALG_FRAMERESULTBUF_MAX];
    /**< Alg frame result information list */

} VALG_FRAMERESULTBUF_LIST_S;

/**
    \brief CHannel specific video info

    Provides channel specific info for each channel. This is used
    by the application to query a module input channel information
*/
typedef struct  VIDEO_CHANNEL_INFO_S {

    UInt32  width;
    UInt32  height;
} VIDEO_CHANNEL_INFO_S;

/**
    \brief Channel info structure

    This structure provides info on the number of channels and
    each channels input frame info such as width/height etc.
*/
typedef struct VIDEO_CHANNEL_LIST_INFO_S {

    UInt32              numCh;
    /**< Number of input channels processed by a MCFW module */

    VIDEO_CHANNEL_INFO_S  chInfo[VIDEO_MAX_NUM_CHANNELS];
    /**< Channel specific information list */

} VIDEO_CHANNEL_LIST_INFO_S;

#endif

/* @} */
