/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2012 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
    \ingroup LINK_API
    \defgroup CAPTURE_LINK_API Capture Link API

    Capture Link can be used to instantiate capture upto
    SYSTEM_CAPTURE_INST_MAX Video input port instances.

    Each instance can have upto two outputs.

    The frames from these capture outputs can be put in upto
    four output queues.

    Each output queue can inturn to be connected to a link like
    Display or DEI or NSF.

    @{
*/

/**
    \file captureLink.h
    \brief Capture Link API
*/

#ifndef _CAPTURE_LINK_H_
#define _CAPTURE_LINK_H_

#ifdef __cplusplus
extern "C" {
#endif


/* Include's    */
#include <mcfw/interfaces/link_api/system.h>

/* Define's */

/* @{ */

/** \brief Max outputs per VIP instance */
#define CAPTURE_LINK_MAX_OUTPUT_PER_INST  (2)

/** \brief Max output queues in the capture link */
#define CAPTURE_LINK_MAX_OUT_QUE          (4)

/** \brief Max Channels per output queue */
#define CAPTURE_LINK_MAX_CH_PER_OUT_QUE   (16)

/** \brief Indicates number of output buffers to be set to default
 *         value by the capture link
 */
#define CAPTURE_LINK_NUM_BUFS_PER_CH_DEFAULT (0)

/** \brief Max blind area supported for each channel */
#define CAPTURE_LINK_MAX_BLIND_AREA_PER_CHNL (16)

/** \brief NO odd fields will be skipped */
#define CAPTURE_LINK_ODD_FIELD_SKIP_NONE   (0)

/** \brief All odd fields will be skipped */
#define CAPTURE_LINK_ODD_FIELD_SKIP_ALL    (1)

/** \brief 1/2 odd fields will be skipped */
#define CAPTURE_LINK_ODD_FIELD_SKIP_1_2    (2)

/** \brief 1/4 odd fields will be skipped */
#define CAPTURE_LINK_ODD_FIELD_SKIP_1_4    (3)


/* @} */

/* Control Command's    */

/**
    \ingroup LINK_API_CMD
    \addtogroup CAPTURE_LINK_API_CMD  CAPTURE Link Control Commands

    @{
*/

/**
    \brief Link CMD: Detect capture video source format

    This command can make the capture link wait until video
    source is detect on all the expected input video sources

    \param UInt32 timeout  [IN] BIOS_WAIT_FOREVER or BIOS_NO_WAIT
*/
#define CAPTURE_LINK_CMD_DETECT_VIDEO            (0x1000)

/**
    \brief Link CMD: force reset VIP port

    This command will force reset the VIP port, this will cause
    the both portA and portB on the VIP.

    VIP instance is determined based on overflow check using
    IOCTL_VPS_CAPT_CHECK_OVERFLOW

    \param None
*/
#define CAPTURE_LINK_CMD_FORCE_RESET             (0x1001)

/**
    \brief Print detaild capture link information

    This is meant to be used by driver developer for internal debugging purposes.

    \param NONE
*/
#define CAPTURE_LINK_CMD_PRINT_ADV_STATISTICS    (0x1002)

/**
    \brief Set video brightness, configure the external ADC device

    This API set video brightness of the video, it configure the external
    ADC device to achieve this function. need support by external ADC device.
    on most of device this can be set different for each channel.

    This command will used when the external decoder is controlled by
    M3 side. It is should not be used when the external decoder is controlled by A8.

    \param Int32 brightness [IN] value of brightness to set, should be in
                                 range of 0 to 255
*/
#define CAPTURE_LINK_CMD_CHANGE_BRIGHTNESS       (0x1003)

/**
    \brief Set video contrast, configure the external ADC device

    This API set video contrast of the video, it configure the external
    ADC device to achieve this function. need support by external ADC device.
    on most of device this can be set different for each channel.

    This command will used when the external decoder is controlled by
    M3 side. It is should not be used when the external decoder is controlled by A8.

    \param Int32 contrast [IN] value of contrast to set, should be in
                                 range of 0 to 255
*/
#define CAPTURE_LINK_CMD_CHANGE_CONTRAST         (0x1004)

/**
    \brief Set video saturation, configure the external ADC device

    This API set video saturation of the video, it configure the external
    ADC device to achieve this function. need support by external ADC device.
    on most of device this can be set different for each channel.

    This command will used when the external decoder is controlled by
    M3 side. It is should not be used when the external decoder is controlled by A8.

    \param Int32 saturation [IN] value of saturation to set, should be in
                                 range of 0 to 255
*/
#define CAPTURE_LINK_CMD_CHANGE_SATURATION       (0x1005)

/**
    \brief Set video hue, configure the external ADC device

    This API set video hue of the video, it configure the external
    ADC device to achieve this function. need support by external ADC device.
    on most of device this can be set different for each channel.

    This command will used when the external decoder is controlled by
    M3 side. It is should not be used when the external decoder is controlled by A8.

    \param Int32 hue [IN] value of hue to set, should be in
                                 range of 0 to 255
*/
#define CAPTURE_LINK_CMD_CHANGE_HUE              (0x1006)

/**
    \brief configure the external decoder.

    This command will used when the external decoder is controlled by
    M3 side. It is should not be used when the external decoder is controlled by A8.

    \param NONE
*/
#define CAPTURE_LINK_CMD_CONFIGURE_VIP_DECODERS  (0x1008)

/**
    \brief get the video source status.

    Get the video source status such as width, height, interlace or prograssive.

    This command will used when the external decoder is controlled by
    M3 side. It is should not be used when the external decoder is controlled by A8.

    \param VCAP_VIDEO_SOURCE_STATUS_S *pStatus [OUT] video source status.
*/
#define CAPTURE_LINK_CMD_GET_VIDEO_STATUS        (0x1009)

/**
    \brief set the audio codec parameters of the external decoder

    Set the codec parameters of the external decoder, such as sample frequency,
    cascade mode, volume and so on.

    This command will used when the external decoder is controlled by
    M3 side. It is should not be used when the external decoder is controlled by A8.

    \param Capture_AudioModeParams * audArgs [IN] audio parameter to set
*/
#define CAPTURE_LINK_CMD_SET_AUDIO_CODEC_PARAMS  (0x100A)

/**
    \brief Print buffer information of the capture link

    Print buffer count be queued and dequeued.

    \param NONE
*/
#define CAPTURE_LINK_CMD_PRINT_BUFFER_STATISTICS (0x100B)

/**
    \brief add one extra buffer pool for given channel

    Add one extra frames pool that used for capture for a given channel when frames from the normal
    buffer pool are exhausted. Now this is only used when SD spot display to avoid the SD display
    dequeue effect the normal operation of the other link.

    \param CaptureLink_ExtraFramesChId *pPrm [IN] Given queue id and channel id
*/
#define CAPTURE_LINK_CMD_SET_EXTRA_FRAMES_CH_ID  (0x100C)

/**
    \brief set blind area

    Set blind are information, this will create a black area on the captured frames,
    this will effect both preview path and encoder path.

    \param CaptureLink_BlindInfo *blindAreaInfo [IN] blind area info , channel ID, number of
                                                    blind are, blind rectangle information.
*/
#define CAPTURE_LINK_CMD_CONFIGURE_BLIND_AREA    (0x100D)

/**
    \brief skip the odd fields

    This API will skip the odd fields from the capture link.
    Only applicable for interlaced input source.

    \param CaptureLink_SkipOddFields *pPrm [IN] channel ID in which queID and the skip mode.
    the skip mode could be
    CAPTURE_LINK_ODD_FIELD_SKIP_NONE to CAPTURE_LINK_ODD_FIELD_SKIP_1_4
*/
#define CAPTURE_LINK_CMD_SKIP_ODD_FIELDS   (0x100E)
/* @} */

/**
    \brief Parameters used to specify or change at run-time
        the channel for which extra buffers are needed

    This is applicable only when CaptureLink_CreateParams.numExtraBuf > 0
*/
typedef struct
{
    UInt32 queId;
    /**< Queue for which the extra channel buffers are needed */

    UInt32 chId;
    /**< Channel in the queue for whom extra channel buffers are needed */

} CaptureLink_ExtraFramesChId;

/**
    \brief Capture Color Specific Parameters
*/

typedef struct
{
    UInt32 brightness;
    /**< Brightness: 0-255 */

    UInt32 contrast;
    /**< contrast: 0-255 */

    UInt32 satauration;
    /**< satauration: 0-255 */

    UInt32 hue;
    /**< hue: 0-255 */

    UInt32 chId;
    /**< chId: 0-15 */

}CaptureLink_ColorParams;


/**
    \brief Capture output parameters
*/
typedef struct
{
    UInt32              dataFormat;
    /**< output data format, YUV422, YUV420, RGB, see System_VideoDataFormat */

    UInt32              scEnable;
    /**< TRUE: enable scaling, FALSE: disable scaling */

    UInt32              scOutWidth;
    /**< Scaler output width */

    UInt32              scOutHeight;
    /**< Scaler output height */

    UInt32              outQueId;
    /**< Link output que ID to which this VIP instance output frames are put */

} CaptureLink_OutParams;

/**
    \brief VIP instance information
*/
typedef struct
{
    UInt32                        vipInstId;
    /**< VIP capture driver instance ID, see SYSTEM_CAPTURE_INST_VIPx_PORTy */

    UInt32                        videoDecoderId;
    /**< Video decoder instance ID, see SYSTEM_DEVICE_VID_DEC_xxx_DRV */

    UInt32                        inDataFormat;
    /**< Input source data format, RGB or YUV422, see System_VideoDataFormat */

    UInt32                        standard;
    /**< Required video standard, see System_VideoStandard */

    UInt32                        numOutput;
    /**< Number of outputs per VIP */

    CaptureLink_OutParams         outParams[CAPTURE_LINK_MAX_OUTPUT_PER_INST];
    /**< Information about each output */

    UInt32                        numChPerOutput;
    /**< Number of individual channels per outputs */

    Bool                          frameCaptureMode;
    /**< To determine if field based or frame based capture mode is requried.
         This is true for only interlaced captures. */

    Bool                          fieldsMerged;
    /**< This is to determine whether fields will be merged or separate in case
         frame capture is enabled above. */

} CaptureLink_VipInstParams;

/**
    \brief windows of the blind area
*/
typedef struct
{
    UInt32 startX;
    /**< in pixels, MUST be multiple of 2 of YUV422I and multiple of 4 for YUV420SP */
    UInt32 startY;
    /**< in lines, MUST be multiple of 2 for YUV420SP  */
    UInt32 width;
    /**< in pixels, MUST be multiple of 2 of YUV422I and multiple of 4 for YUV420SP */
    UInt32 height;
    /**< in lines, MUST be multiple of 2 for YUV420SP  */
    UInt32 fillColorYUYV;
    /**< Color in YUYV format .*/
    UInt32 enableWin;
    /**< TRUE: Draw this blind area, FALSE: Do not draw this area */
} CaptureLink_BlindWin;

/**
    \brief information of the blind area
*/
typedef struct
{
    UInt32                  channelId;
    /**< channel id for the blind area */
    UInt32                  queId;
    /**< queue id of the blind area */
    UInt32                  numBlindArea;
    /**< number of valid blind area, 0 means disable bland area of this channel */

    CaptureLink_BlindWin    win[CAPTURE_LINK_MAX_BLIND_AREA_PER_CHNL];
    /**< window settings of blind are */
} CaptureLink_BlindInfo;

/**
    \brief Params for CAPTURE_LINK_CMD_SKIP_ODD_FIELDS
*/
typedef struct
{
    UInt32 queId;
    /**< Output Que ID */
    UInt32 skipOddFieldsChBitMask;
    /**< Channels for which odd fields should be skipped

        bit0 = CH0
        bit1 = CH1
        ...
        bitN = CHN
    */
    UInt32 oddFieldSkipRatio;
    /**< skip mode, it should be CAPTURE_LINK_ODD_FIELD_SKIP_XXXX */
} CaptureLink_SkipOddFields;

/**
    \brief Capture Link create parameters
*/
typedef struct
{
    Bool                      isPalMode;
    /**< pal mode based on usecase */

    UInt16                    numVipInst;
    /**< Number of VIP instances in this link */

    CaptureLink_VipInstParams vipInst[SYSTEM_CAPTURE_INST_MAX];
    /**< VIP instance information */

    System_LinkOutQueParams   outQueParams[CAPTURE_LINK_MAX_OUT_QUE];
    /**< Output queue information */

    UInt32                    tilerEnable;
    /**< TRUE/FALSE: Enable/disable tiler */


    UInt32                    fakeHdMode;
    /**< Capture in D1 but tells link size is 1080p, useful to test HD data flow on DVR HW or VS EVM */

    UInt32                    enableSdCrop;
    /**< Applicable only for D1/CIF capture input, crops 720 to 704 and 360 to 352 at video decoder.
        */
    UInt32                    doCropInCapture;
    /**< Applicable only for D1/CIF capture input, crops 720 to 704 and 360 to 352 at video decoder.
         Input is still 720/360. Crop is done in capture link by adjusting capture frame buffer pointer
         */
    UInt32                    numBufsPerCh;
    /**< Number of output buffers per channel in capture */

    UInt32                    numExtraBufs;
    /**<
        Number of extra buffers allocated and used by capture across channels.
        Buffers are allocated using WxH of CH0

        This is useful in some cases where the next link may hold the buffers
        of a given channel for a longer duration.

        In such cases the extra buffers will used for capture. This will ensure
        capture does not drop frames due to next link holding onto the
        buffer for a longer time

        Example, when capture is connect to SD display link directly

        This should be used typically when all CHs have the same properties like WxH
    */

    UInt32                    maxBlindAreasPerCh;
    /**< max blind area for one channel */

} CaptureLink_CreateParams;

/**
    \brief Audio Codec parameters
*/
typedef struct
{
    UInt32 deviceNum;
  /**< Device number for which to apply the audio parameters */

    UInt32 numAudioChannels;
  /**< Number of audio channels */
    UInt32 samplingHz;
  /**< Audio sampling rate in Hz, Valid values: 8000, 16000 */

    UInt32 masterModeEnable;
  /**< TRUE: Master mode of operation, FALSE: Slave mode of operation */

    UInt32 dspModeEnable;
  /**< TRUE: DSP data format mode, FALSE: I2S data format mode */

    UInt32 ulawEnable;
  /**< TRUE: 8-bit ulaw data format mode, FALSE: 16-bit PCM data format mode */

    UInt32 cascadeStage;
  /**< Cascade stage number, Valid values: 0..3 */

    UInt32 audioVolume;
  /**< Audio volume, Valid values: 0..8. Refer to TVP5158 datasheet for details
   */

    UInt32 tdmChannelNum;
    /**< Number of TDM channels: 0: 2CH, 1: 4CH, 2: 8CH, 3: 12CH 4: 16CH */

} Capture_AudioModeParams;

/**
    \brief Capture link register and init

    - Creates link task
    - Registers as a link with the system API

    \return FVID2_SOK on success
*/
Int32 CaptureLink_init();

/**
    \brief Capture link de-register and de-init

    - Deletes link task
    - De-registers as a link with the system API

    \return FVID2_SOK on success
*/
Int32 CaptureLink_deInit();

/**
    \brief Set defaults for in link channel information

    \param pPrm [OUT] Default information
*/
static inline void CaptureLink_CreateParams_Init(CaptureLink_CreateParams *pPrm)
{
    UInt32 i;
    memset(pPrm, 0, sizeof(*pPrm));

    pPrm->numVipInst = 0;
    pPrm->tilerEnable = FALSE;
    pPrm->fakeHdMode = FALSE;
    pPrm->enableSdCrop = TRUE;
    pPrm->doCropInCapture = TRUE;
    pPrm->numBufsPerCh = CAPTURE_LINK_NUM_BUFS_PER_CH_DEFAULT;
    pPrm->numExtraBufs = 0;

    for (i=0; i<SYSTEM_CAPTURE_INST_MAX; i++)
    {
        pPrm->vipInst[i].numChPerOutput = 0;
    }
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif


/*@}*/
