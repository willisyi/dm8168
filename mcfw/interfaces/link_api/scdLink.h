
/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2012 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
    \ingroup ALG_LINK_API
    \defgroup ALG_SCD_LINK_API ALG Link: SCD API

    SCD supports two kinds of scene change detect algorithm


    - Tamper Detect (TD) - this is Frame mode of SCD
        - SUPPORTED in ALL platforms
        - Detects camera tamper like hand blocking a camera
        - This is a global or whole frame level algorithm and gives
            YES or NO kind of decision for whole frame for tamper condition.

    - Live motion Detct (LMD) - this is Block mode of SCD
        - SUPPORTED in DM816x, DM814x ONLY
        - Calculates changed pixels in 32x12 block and returns info to user
            on frame to frame basis
        - This info can be used by user to detect motion in specific blocks
            which can be used for a higher motion detect algorithm
        - The data input to this block is live data from capture and
            hence this algorithm is called Live Motion Detect
        - NOTE, only information is returned to user about each block, decision
            of motion or no motion is left to the user

    @{
*/

/**
    \file scdLink.h
    \brief ALG Link: SCD API
*/

#ifndef _SCD_LINK_H_
#define _SCD_LINK_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Include's    */

/* Define's */

/**
    \brief Max supported SCD channels
*/
#define ALG_LINK_SCD_MAX_CH      (16)


/**
    \brief Max supported frame width for SCD
*/
#define ALG_LINK_SCD_MAX_FRAME_WIDTH               (352)

/**
    \brief Max supported frame height for SCD
*/
#define ALG_LINK_SCD_MAX_FRAME_HEIGHT              (288)

/**
    \brief Max supported block width for SCD in block mode
*/
#define ALG_LINK_SCD_BLK_WIDTH                     (32)

/**
    \brief Max supported block height for SCD in block mode
*/
#define ALG_LINK_SCD_BLK_HEIGHT                    (12)

/**
    \brief Min supported block height for SCD in block mode
*/
#define ALG_LINK_SCD_BLK_HEIGHT_MIN                (10)


/**
    \brief Max number of blocks in horizontal direction
*/
#define ALG_LINK_SCD_MAX_BLK_IN_H_DIR              (ALG_LINK_SCD_MAX_FRAME_WIDTH/ALG_LINK_SCD_BLK_WIDTH)

/**
    \brief Max number of blocks in vertical direction
*/
#define ALG_LINK_SCD_MAX_BLK_IN_V_DIR              (ALG_LINK_SCD_MAX_FRAME_HEIGHT/ALG_LINK_SCD_BLK_HEIGHT)

/**
    \brief Max total number of blocks in a frame
*/
#define ALG_LINK_SCD_MAX_BLOCKS_IN_FRAME          (ALG_LINK_SCD_MAX_BLK_IN_H_DIR * ALG_LINK_SCD_MAX_BLK_IN_V_DIR)

/**
    \ brief Recommended value of thresold that decides time taken before Tamper Alert 
      is issued in the event of Tamper.
*/
#define ALG_LINK_TIME_TO_WAIT_BEFORE_TAMPER_ALERT_THRESOLD  (16)

/**
    \ brief Recommended value of thresold that decides time till tamper state -s re-cket
     after Tamper Event
*/
#define ALG_LINK_TIME_TO_WAIT_AFTER_TAMPER_ALERT_THRESOLD   (8)



/* Constant's */


/**
    \brief SCD sensitivity level
    
    On DM816x, DM814x platform, for Tamper detection  valid range of sensitivity 
    is from 1 - 5; The senistivity value is clipped accordingly to this range.
    For motion detection (LMD), sensitivity range is from 0 - 8.
    
    On DM810x, for Tamper detection  valid range of sensitivity 
    is from 0 - 8;
*/
typedef enum
{
    ALG_LINK_SCD_SENSITIVITY_MIN          =  0,
    /**< Minimum sensitivity    */

    ALG_LINK_SCD_SENSITIVITY_VERYLOW      =  1,
    /**< Very low sensitivity    */

    ALG_LINK_SCD_SENSITIVITY_LOW          =  2,
    /**< Low sensitivity    */

    ALG_LINK_SCD_SENSITIVITY_MIDLO        =  3,
    /**< Mid-Lo sensitivity    */

    ALG_LINK_SCD_SENSITIVITY_MID          =  4,
    /**< Medium sensitivity    */

    ALG_LINK_SCD_SENSITIVITY_MIDHI        =  5,
    /**< Mid-Hi sensitivity    */

    ALG_LINK_SCD_SENSITIVITY_HIGH         =  6,
    /**< High sensitivity    */

    ALG_LINK_SCD_SENSITIVITY_VERYHIGH     =  7,
    /**< Very High sensitivity    */

    ALG_LINK_SCD_SENSITIVITY_MAX          =  8
    /**< Maximum sensitivity */

} AlgLink_ScdSensitivity;

/**
    \brief SCD mode
*/
typedef enum
{
    ALG_LINK_SCD_DETECTMODE_DISABLE				 = 0,
    /**< Disable SCD */

    ALG_LINK_SCD_DETECTMODE_MONITOR_FULL_FRAME	 = 1,
    /**< Frame based SCD = Tamper Detect (TD) mode

        SUPPORTED in ALL platforms
    */

    ALG_LINK_SCD_DETECTMODE_MONITOR_BLOCKS		 = 2,
    /**< Block based SCD - Live motion detect (LMD) mode

        ONLY SUPPORTED in DM816x, DM814x
    */

    ALG_LINK_SCD_DETECTMODE_MONITOR_BLOCKS_AND_FRAME = 3
    /**< Both frame and block based SCD - TD+LMD mode

        ONLY SUPPORTED in DM816x, DM814x
    */
} AlgLink_ScdMode;

/**
    \brief SCD Status for Tamper Detect
*/
typedef enum
{
    ALG_LINK_SCD_DETECTOR_UNAVAILABLE =-1,
    /**< Status NOT available   */

    ALG_LINK_SCD_DETECTOR_NO_CHANGE   = 0,
    /**< No scene change detected - Tamper detected */

    ALG_LINK_SCD_DETECTOR_QUALIFYING  = 1,
    /**< NOT USED right now */

    ALG_LINK_SCD_DETECTOR_CHANGE      = 2
    /**< Scene change detected - Tamper NOT detected */

} AlgLink_ScdOutput;


/* Control Command's    */

/**
    \ingroup ALG_LINK_API_CMD
    \addtogroup ALG_SCD_LINK_API_CMD  ALG Link: SCD API Control Commands

    @{
*/

/**
    \brief Link CMD: Set ignore lights ON flag for a channel

    SUPPORTED in ALL platforms

    AlgLink_ScdChParams.chId specifies the channel number
    AlgLink_ScdChParams.frmIgnoreLightsON specifies the if lights ON are ignored or NOT

    Other fields are not used

    \param AlgLink_ScdChParams *   [IN]  parameters
*/
#define ALG_LINK_SCD_CMD_SET_CHANNEL_IGNORELIGHTSON    (0x7001)

/**
    \brief Link CMD: Set ignore lights OFF flag for a channel

    SUPPORTED in ALL platforms

    AlgLink_ScdChParams.chId specifies the channel number
    AlgLink_ScdChParams.frmIgnoreLightsOFF specifies the if lights OFF are ignored or NOT

    Other fields are not used

    \param AlgLink_ScdChParams *   [IN]  parameters
*/
#define ALG_LINK_SCD_CMD_SET_CHANNEL_IGNORELIGHTSOFF   (0x7002)

/**
    \brief Link CMD: Set SCD detect mode for a channel

    ONLY SUPPORTED in DM816x, DM814x

    AlgLink_ScdChParams.chId specifies the channel number
    AlgLink_ScdChParams.mode specifies the new SCD mode

    Other fields are not used

    \param AlgLink_ScdChParams *   [IN]  parameters
*/
#define ALG_LINK_SCD_CMD_SET_CHANNEL_MODE              (0x7003)

/**
    \brief Link CMD: Set SCD sensitivity for a channel

    SUPPORTED in ALL platforms

    AlgLink_ScdChParams.chId specifies the channel number
    AlgLink_ScdChParams.frmSensitivity specifies the new SCD sensitivity

    On DM816x, DM814x platform, for Tamper detection  valid range of sensitivity 
    is from 1 - 5; The senistivity value is clipped accordingly to this range.
    For motion detection (LMD), sensitivity range is from 0 - 8.
    
    On DM810x, for Tamper detection  valid range of sensitivity 
    is from 0 - 8;

    Other fields are not used

    \param AlgLink_ScdChParams *   [IN]  parameters
*/
#define ALG_LINK_SCD_CMD_SET_CHANNEL_SENSITIVITY       (0x7004)

/**
    \brief Print SCD statistics

    In DM810x,
    - this also prints OSD statistics

    In DM816x, DM814x, OSD statistics print not implemented as of now

    \param NONE
*/
#define ALG_LINK_SCD_CMD_PRINT_STATISTICS              (0x7005)

/**
    \brief Set block configuration for a channel

    ONLY SUPPORTED in DM816x, DM814x

    \param AlgLink_ScdChblkUpdate   *   [IN]   Block update parameters
*/
#define ALG_LINK_SCD_CMD_SET_CHANNEL_BLOCKCONFIG       (0x7006)


/**
    \brief Get SCD status at frame level for all channels

    SUPPORTED in ALL platforms

    \param AlgLink_ScdAllChFrameStatus   *   [OUT]   status
*/
#define ALG_LINK_SCD_CMD_GET_ALL_CHANNEL_FRAME_STATUS   (0x7007)


/**
    \brief Reset SCD algorithm state for a given channel

    SUPPORTED in ALL platforms

    \param AlgLink_ScdChCtrl   *   [IN]   parameters
*/
#define ALG_LINK_SCD_CMD_CHANNEL_RESET                 (0x7008)

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
} AlgLink_ScdblkChngConfig;

/**
    \brief SCD channel parameters that can be changed dynamically
*/
typedef struct
{
    UInt32 chId;
    /**< Channel number, 0.. \ref ALG_LINK_SCD_MAX_CH-1 */

    UInt32 mode;
    /**< SCD mode, for valid values see \ref AlgLink_ScdMode */

    UInt32 frmIgnoreLightsON;
    /**< TRUE: ignore sudden, significantly brighter avg. scene luminance, FALSE: Do no ignore */

    UInt32 frmIgnoreLightsOFF;
    /**< TRUE: ignore sudden, significantly darker avg. scene luminance, FALSE: Do not ignore */

    UInt32 frmSensitivity;
    /**< Internal threshold for scd change tolerance; globally applied to entire frame
            For valid values see \ref AlgLink_ScdSensitivity
    */

    UInt32 frmEdgeThreshold;
    /**< Set minimum number of edge pixels required to indicate non-tamper event

        Set to 100 for now ALWAYS
    */


    UInt32 blkNumBlksInFrame;
    /**< Number of blocks in a frame for block based SCD

        NOT SUPPORTED in DM810x

        ONLY valid when
            AlgLink_ScdChParams.mode =
                ALG_LINK_SCD_DETECTMODE_MONITOR_BLOCKS
                    OR
                ALG_LINK_SCD_DETECTMODE_MONITOR_BLOCKS_AND_FRAMES
    */

    AlgLink_ScdblkChngConfig blkConfig[ALG_LINK_SCD_MAX_BLOCKS_IN_FRAME];
    /**< Block level configuration */

} AlgLink_ScdChParams;

/**
    \brief Block level config - part of AlgLink_ScdChblkUpdate
*/
typedef struct
{
   UInt32               blockId;
   /**< Block ID for which parameters need to be changed
        Valid values, 0 .. ALG_LINK_SCD_MAX_BLOCKS_IN_FRAME-1 */

   UInt32               sensitivity;
   /**< Blocks's sensitivity setting for change detection
            For valid values see \ref AlgLink_ScdSensitivity
     */


   UInt32               monitorBlock;
   /**< TRUE: monitor block for change detection
        FALSE: ignore this block
   */

} AlgLink_ScdChBlkConfig;

/**
    \brief Block's whose config needs to be updated
*/
typedef struct
{
    UInt32                 chId;
    /**< Channel number, 0..ALG_LINK_SCD_MAX_CH-1 */

    UInt32                 numValidBlock;
    /**< Number of blocks to be updated,
        i.e number of valid entries in AlgLink_ScdChblkUpdate.blkConfig[] arrary
    */

    AlgLink_ScdChBlkConfig blkConfig[ALG_LINK_SCD_MAX_BLOCKS_IN_FRAME];
    /**< Config of each block that is to be updated */

}AlgLink_ScdChblkUpdate;

/**
    \brief SCD status of a block - part of AlgLink_ScdChStatus
*/
typedef struct
{
    UInt32 numFrmsBlkChanged;
    /**< Number of consecutive frames with motion in block */

    UInt32 numPixelsChanged;
    /**< Raw number of block pixels that were changed */

} AlgLink_ScdblkChngMeta;

/**
    \brief SCD status that is returned with event VSYS_EVENT_TAMPER_DETECT

    When tamper is detect a event, VSYS_EVENT_TAMPER_DETECT, is to Linux.
    Along with the event a pointer to status is sent across

    ONLY valid when ALG_LINK_SCD_ENABLE_EVENT_NOTIFY is 1

    NOT SUPPORTED for DM810x
*/
typedef struct
{
    UInt32 chId;
    /**< SCD channel number on which tamper event was detected

        Valid values, 0..ALG_LINK_SCD_MAX_CH-1
    */

    UInt32 frmResult;
    /**< SCD change detection result from entire frame
            For valid values, see \ref AlgLink_ScdOutput
    */

} AlgLink_ScdChStatus;

/**
    \brief SCD status of block level SCD

    This results is generated for every frame and is available in
    the output queue (\ref ALG_LINK_SCD_OUT_QUE) of the SCD Link.

    This status is also available with the event VSYS_EVENT_MOTION_DETECT
*/
typedef struct
{
    UInt32 chId;
    /**< Channel number, 0 .. \ref ALG_LINK_SCD_MAX_CH-1 */

    UInt32 frmResult;
    /**< SCD change detection result from entire frame */

    AlgLink_ScdblkChngConfig blkConfig[ALG_LINK_SCD_MAX_BLOCKS_IN_FRAME];
    /**< Linear array of configuration of frame blocks that
         scd will monitor for motion detection (configuration)
    */

    AlgLink_ScdblkChngMeta  blkResult[ALG_LINK_SCD_MAX_BLOCKS_IN_FRAME];
    /**< SCD change detection result from individual frame tiles/blocks
         array length = \ref ALG_LINK_SCD_MAX_BLOCKS_IN_FRAME
    */

} AlgLink_ScdResult;


/**
    \brief SCD frame level status information for all CHs
*/
typedef struct
{
    UInt32 numCh;
    /**< Channel number, 0 .. \ref ALG_LINK_SCD_MAX_CH-1 */

    AlgLink_ScdChStatus chanFrameResult[ALG_LINK_SCD_MAX_CH];
    /**< SCD frame level state. For valid values see, AlgLink_ScdOutput */
} AlgLink_ScdAllChFrameStatus;

/**
    \brief Data structure to select CH on which reset operation is done
*/
typedef struct
{
    UInt32 chId;
    /**< Channel number, 0.. \ref ALG_LINK_SCD_MAX_CH-1 */

} AlgLink_ScdChCtrl;

/**
    \brief SCD link create parameters
*/
typedef struct
{
    UInt32 maxWidth;
    /**< Set the maximum width (in pixels) of video frame that scd will process */

    UInt32 maxHeight;
    /**< Set the maximum height (in pixels) of video frame that scd will process */

    UInt32 maxStride;
    /**< Set the maximum video frame pitch/stride of the images in the video buffer*/

    UInt32 numSecs2WaitB4Init;
    /**< Set the number of seconds to wait before initializing SCD monitoring.
         This wait time occurs when the algorithm instance first starts and
         allows video input to stabilize.*/

    UInt32 numSecs2WaitB4FrmAlert;
    /**< Set the number of seconds to wait before signaling a frame-level scene change event.*/

    UInt32 numSecs2WaitAfterFrmAlert;
	/**< Set to 1 to n for the maximum number of seconds to wait for pre-tamper
	     conditions to return following a tamper event */

    UInt32 numSecs2Wait2MarkStableFrame;
	/**< Set to 1 to n for the maximum number of seconds to mark new scene
	     as non-tamper condition. If Tamper persist for these many frames, 
         current frame will be mark as stable scene. */

    UInt32 thresold2WaitB4FrmAlert;
    /**< Set the thresold to wait before signaling a frame-level scene change event.
         It can take values from 0-32. Recommended value for this 
         ALG_LINK_TIME_TO_WAIT_BEFORE_TAMPER_ALERT_THRESOLD.  User can tune this
         Value to change response time. */

    UInt32 thresold2WaitAfterFrmAlert;
	/**< Set thresold to wait for pre-tamper conditions to return following a 
         tamper event. It can take values from 0-32.  Recommended value for this 
         ALG_LINK_TIME_TO_WAIT_AFTER_TAMPER_ALERT_THRESOLD.  User can tune this
         Value as per requirement. */

    UInt32 inputFrameRate;
    /**< Frames per second fed to the SCD */

    UInt32 outputFrameRate;
    /**< Frames per second alogorithm is operated at */

    AlgLink_ScdChParams chDefaultParams[ALG_LINK_SCD_MAX_CH];
    /**< Pointer to array of channel params used to configure ScdAlgLink.

        - SCD need not be enabled for all channels.
        - The number of channels are set via AlgLink_ScdCreateParams.numValidChForSCD
        - The valid channels or channels selected for SCD as specified via
            AlgLink_ScdCreateParams.chDefaultParams[x].chId
    */

    UInt32 numBufPerCh;
    /**< Number of buffer to allocate per channel - ONLY applicable for LMD mode of operation */

    Bool enableMotionNotify;
    /**< Enable CPU side Live Motion detect Notify to A8 by VSYS_EVENT_MOTION_DETECT command.
      * If disabled Application has to take care motion detection part from
      * block metaData that is exported by DSP to A8 */

    Bool enableTamperNotify;
    /**< Enable CPU side Tamper detect Notify to A8 by VSYS_EVENT_TAMPER_DETECT command
      * If disabled Application has to take care Tamper detection notify part
      * at A8 by calling ALG_LINK_SCD_CMD_GET_ALL_CHANNEL_FRAME_STATUS command
      * with appropriate structure */

    UInt32 numValidChForSCD;
    /**< Set the maximum channels on which scd will run.

            This determines the number of channels info that is valid in

            AlgLink_ScdCreateParams.chDefaultParams[]

         */
} AlgLink_ScdCreateParams;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

/*@}*/

