/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
    \ingroup LINK_API
    \defgroup DEC_LINK_API Video Decoder Link API

    Video Decode Link can be used to decode a bitstream of different codec
    types such as
    - H264
    - MPEG4 &
    - MJPEG

    @{
*/

/**
    \file decLink.h
    \brief Video Decoder Link API
*/

#ifndef _DEC_LINK_H_
#define _DEC_LINK_H_

/* include files */
#include <mcfw/interfaces/link_api/system.h>
#include <mcfw/interfaces/common_def/ti_vdec_common_def.h>

/* Define's */
/* @{ */

/** \brief Max number of DEC Link output queues */
#define DEC_LINK_MAX_OUT_QUE                  (1)

/** \brief Max number of DEC channels per link */
#define DEC_LINK_MAX_CH                       (64)

/** \brief Maximum number of output buffers supported per channel. */
#define DEC_LINK_MAX_NUM_OUT_BUF_PER_CH       (16)

/** \brief Default value for DPB size in frames */
#define DEC_LINK_DPB_SIZE_IN_FRAMES_DEFAULT   (-1)

/** \brief Default value for Number of buffers per channel request */
#define DEC_LINK_NUM_BUFFERS_PER_CHANNEL      (0)

/* Alg Create Status    */
/**
    DECODE Link: DEC Link Alg Create Status

    @{
*/

/** \brief Only the partial channel get created, neither codec instance
     nor the output buffers will get created. */
#define DEC_LINK_ALG_CREATE_STATUS_DONOT_CREATE (0)

/** \brief Fully functional channel will be created with Both codec
     instance and the output buffers created, reday to operate */
#define DEC_LINK_ALG_CREATE_STATUS_CREATE       (1)

/** \brief Not for Application use, used by Link internals */
#define DEC_LINK_ALG_CREATE_STATUS_CREATE_DONE  (2)

/** \brief Not for Application use, used by Link internals */
#define DEC_LINK_ALG_CREATE_STATUS_DELETE       (3)

/* @} */

/* Control Command's    */
/**
    \ingroup LINK_API_CMD
    \addtogroup DEC_LINK_API_CMD DECODE Link Control Commands

    @{
*/

/**
    \brief Print detaild IVA-HD statistics
     This is meant to be used by developer for internal debugging purposes
    \param NONE
*/
#define DEC_LINK_CMD_PRINT_IVAHD_STATISTICS  (0x2001)

/**
    \brief Dec command to enable channel
     Application can use this command to enable an alreday disabled channel
    \param chId
*/
#define DEC_LINK_CMD_ENABLE_CHANNEL          (0x2002)

/**
    \brief Dec command to disable channel
     Application can use this command to disable an alreday enabled channel
    \param chId
*/
#define DEC_LINK_CMD_DISABLE_CHANNEL         (0x2003)

/**
    \brief Dec command to print statistics
     Application can use this to print the detailed DEC link level statistics,
     Mainly used for debuging purpose
    \param NONE
*/
#define DEC_LINK_CMD_PRINT_STATISTICS        (0x2004)

/**
    \brief Dec command to set trick play configuration
     Application can use this Dec command to set trick play configuration
    \param chId
*/
#define DEC_LINK_CMD_SET_TRICKPLAYCONFIG     (0x2005)

/**
    \brief Dec command to print buffer statistics
     Application can use this command to print the buffer statistics
     Mainly used for debuging purpose
    \param NONE
*/
#define DEC_LINK_CMD_PRINT_BUFFER_STATISTICS (0x2006)

/**
    \brief Dec command to open channel
     Application can use this command to create/open an
     alreday excisting channel
    \param chId
*/
#define DEC_LINK_CMD_CREATE_CHANNEL          (0x2007)

/**
    \brief Dec command to close channel
     Application can use this command to delete/close
     an alreday created/opened channel
    \param chId
*/
#define DEC_LINK_CMD_DELETE_CHANNEL          (0x2008)

/**
    \brief Dec command to return buffer statistics
     Application can use this command to return the buffer statistics
     Mainly used for debuging purpose
    \param NONE
*/
#define DEC_LINK_CMD_GET_BUFFER_STATISTICS   (0x2009)

/**
    \brief Dec command to control error reporting
     Application can use this Dec command to Enable/Disable
     Decode Error reporting feature to A8
    \param NONE
*/
#define DEC_LINK_CMD_RESET_DEC_ERR_REPORTING (0x200A)
/* @} */

/**
 *  \brief DecLink buffer statistics structure
 *
 *  Defines the structure returned to application having the
 *  buffer statistics of one channel for the decoder link.
 */
typedef struct DecLink_ChBufferStats
{
    UInt32 numInBufQueCount;
    /**< Number of input buffers queued-in */
    UInt32 numOutBufQueCount;
    /**< Number of output buffers queued-in */
}DecLink_ChBufferStats;

/**
 *  \brief DecLink buffer statistics structure
 *
 *  Defines the structure returned to application having the
 *  buffer statistics for a specific list of channels of the decoder link.
 */
typedef struct DecLink_BufferStats
{
    UInt32  numCh;
    /**< Number of channels for buffer statistics requested */
    UInt32  chId[DEC_LINK_MAX_CH];
    /**< List of channels for which buffer statistics requested */
    DecLink_ChBufferStats stats[DEC_LINK_MAX_CH];
    /**< Buffer statistics of one channel for the decoder link */
} DecLink_BufferStats;

/** Mapping to DecLink Structure definition format*/
#define DecLink_ChErrorMsg VDEC_CH_ERROR_MSG

/**
 *  \brief DecLink decoder error report
 *
 *  Defines the structure returned to application having the
 *  decoder error report of a specific channel of the decoder link.
 */
typedef struct DecLink_ChErrorReport
{
    Bool enableErrReport;
    /**< Flag to enable/disable decoder error reporting to A8
      *  If set to FALSE : No signal to A8 at error occurance
      *            TRUE  : Notify A8 at appropriate decoder error
      *  Note: Decoder Fatal errors are always reported to A8 */

    Int32 chId;
    /**< Decoder channel number */
} DecLink_ChErrorReport;

/**
*   \brief Dec link channel dynamic params
*
*   Defines those parameters that can be changed dynamically on a per channel
*   basis for the decode link
*/
typedef struct DecLink_ChDynamicParams
{
    Int32 targetFrameRate;
    /**< Target frame rate of the decoder channel */
    Int32 targetBitRate;
    /**< Target bitrate of the decoder channel */
} DecLink_ChDynamicParams;

/**
*   \brief Dec link channel create params
*
*   Defines those parameters that can be configured/set during the create time
*   on a per channel basis for the decode link
*/
typedef struct DecLink_ChCreateParams
{
    UInt32 format;
    /**< Video Codec format/type */
    Int32 profile;
    /**< Video coding profile */
    Int32 targetMaxWidth;
    /**< Target frame width of the decoder */
    Int32 targetMaxHeight;
    /**< Target frame height of the decoder */
    Int32 displayDelay;
    /**< Max number of frames delayed by decoder */
    DecLink_ChDynamicParams defaultDynamicParams;
    /**< default dynamic params for decoder */
    UInt32 fieldMergeDecodeEnable;
    /**< Enable this option to decode 2 fields in same Decode link call
     * ie. both Top & Bottom field in same ouput buffer with Top filed
     * followed by bottom field data in field seperated mode */
    UInt32 processCallLevel;
    /**< Specifies if process call is done frame level or field level */
    UInt32 numBufPerCh;
    /**< Number of buffer to allocate per channel-wise */
    Int32 dpbBufSizeInFrames;
    /**< Size of the decoder picture buffer.If application
     *   knows the max number of reference frames in the
     *   stream to be fed to the decoder, it can set this
     *   value to enable reduced memory consumption.
     *   Application should set this value to default
     *   if it doesn't care about this parameter
     */
     UInt32 algCreateStatus;
    /**< App can only configure this variable with two below values
     *         DEC_LINK_ALG_CREATE_STATUS_DONOT_CREATE -> 0
     *         DEC_LINK_ALG_CREATE_STATUS_CREATE -> 1
     *   If 0: Only the partial channel get created, neither codec instance
     *         nor the output buffers will be get created. App will be
     *         able to make this channel fully functional dynamically by
     *         calling DEC_LINK_CMD_CREATE_CHANNEL API
     *   If 1: Fully functional channel will be created with Both codec
     *         instance and the output buffers created, reday to operate
     */
} DecLink_ChCreateParams;

/**
*   \brief Dec link create params
*
*   Defines those parameters that can be configured/set
*   during the DEC link create time
*/
typedef struct DecLink_CreateParams
{
    System_LinkInQueParams      inQueParams;
    /**< Input queue information */
    System_LinkOutQueParams     outQueParams;
    /**< Output queue information */
    DecLink_ChCreateParams      chCreateParams[DEC_LINK_MAX_CH];
    /**< Decoder link channel create params */
    UInt32                      tilerEnable;
    /**< Flag to Enable/Disable tiler for decode link ouput buffers */
} DecLink_CreateParams;

/**
*   \brief Dec link channel info
*
*   Defines the channel number of the DEC link for any channel
*   specific dec link configutaion/settings */
typedef struct DecLink_ChannelInfo
{
    UInt32 chId;
    /**< Decoder channel number */
} DecLink_ChannelInfo;

/**
*   \brief Dec link craete/open ChannelInfo params
*
*   Defines the complete set of parameters that can be required for
*   dynamically add/create/open a new channel to the excisting decode link
*/
typedef struct DecLink_addChannelInfo
{
    UInt32 chId;
    /**< chId of the new channel */
    System_LinkChInfo   chInfo;
    /**< channel specific link configuration parameters */
    DecLink_ChCreateParams createPrm;
    /**< channel specific create time paramters */
} DecLink_addChannelInfo;

/**
*   \brief Dec link trickplay configure params
*
*   Defines those parameters that can be configured/set
*   during dec link per channel vise Trickplay mode
*/
typedef struct DecLink_TPlayConfig
{
    UInt32 chId;
    /**< Decoder channel number */
    UInt32 inputFps;
    /**< FrameRate at which Decoder is getting the data */
    UInt32 targetFps;
    /**< Target FrameRate for TrickPlay. TrickPlay will generate
         target frame rate from the input framerate */
} DecLink_TPlayConfig;

/**
*   \brief Dec link function to register and init
*
*   - Creates link task
*   - Registers as a link with the system API
*
*   \return FVID2_SOK on success
*/
Int32 DecLink_init();

/**
*   \brief Dec link function to de-register and de-init
*
*   - Deletes link task
*   - De-registers as a link with the system API
*
*   \return FVID2_SOK on success
*/
Int32 DecLink_deInit();

/**
*   \brief Set defaults values for the dec link & channel parameters
*
*   \param pPrm [OUT] Default information
*/
static inline void DecLink_CreateParams_Init(DecLink_CreateParams *pPrm)
{
    UInt32 i;

    memset(pPrm, 0, sizeof(*pPrm));

    pPrm->inQueParams.prevLinkId = SYSTEM_LINK_ID_INVALID;
    pPrm->outQueParams.nextLink = SYSTEM_LINK_ID_INVALID;

    /* when set 0, decoder will take default value based on system
       defined default on BIOS side */
    for (i=0; i<DEC_LINK_MAX_CH;i++)
    {
        pPrm->chCreateParams[i].dpbBufSizeInFrames =
                                DEC_LINK_DPB_SIZE_IN_FRAMES_DEFAULT;
        pPrm->chCreateParams[i].numBufPerCh = 0;
        pPrm->chCreateParams[i].displayDelay = 0;
        pPrm->chCreateParams[i].algCreateStatus =
                                DEC_LINK_ALG_CREATE_STATUS_CREATE;
    }
}

#endif

/*@}*/

