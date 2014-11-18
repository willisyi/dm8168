/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2012 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
    \ingroup ALG_LINK_API
    \defgroup ALG_OSD_LINK_API ALG Link: OSD API

    OSD Link allows to blend a user specified bitmaps on incoming video data.
    Bitmaps are blended pixel by pixel with optional alpha blending and
    transperencty.

    Depending on the platforms OSD is implemented either on DSP or SIMCOP.

    Features of OSD are listed below, including comparision of DSP based
    and SIMCOP based algorithms,


    <table>
        <tr>
            <td>
                Feature
            </td>
            <td>
                DSP based OSD
            </td>
            <td>
                SIMCOP based OSD
            </td>
        </tr>
        <tr>
            <td>
                Platform supported
            </td>
            <td>
                DM816x, DM814x
            </td>
            <td>
                DM810x
            </td>
        </tr>
        <tr>
            <td>
                Bitmap Data format
                - This MUST match the incoming video data format.
                - OSD does not do nay format conversion.
            </td>
            <td>
                YUV420SP, YUV422I - YUYV
            </td>
            <td>
                YUV420SP
            </td>
        </tr>
        <tr>
            <td>
                Global alpha
            </td>
            <td>
                YES - per CH, per Window basis
            </td>
            <td>
                YES - all CHs, all windows will have same global alpha. CH0, WIN0 value is applied for all CHs.
            </td>
        </tr>
        <tr>
            <td>
                Transperency
            </td>
            <td>
                YES - per CH, per Window basis
            </td>
            <td>
                YES - all CHs, all windows will have same transperency. CH0, WIN0 value is applied for all CHs.
            </td>
        </tr>
        <tr>
            <td>
                Transperency Color
            </td>
            <td>
                YES - per CH basis
            </td>
            <td>
                YES - all CHs, will have same color key. CH0 value is applied for all CHs.
            </td>
        </tr>
        <tr>
            <td>
                Window Width alignement
            </td>
            <td>
                Needs to be 8 pixel aligned
            </td>
            <td>
                Needs to be 4 pixel aligned
            </td>
        </tr>
        <tr>
            <td>
                Separate Blind window command
            </td>
            <td>
                NO - OSD window itself can be used as blind window
            </td>
            <td>
                YES - spearate better optimized mechanism for blind window
            </td>
        </tr>

    </table>



    @{
*/

/**
    \file osdLink.h
    \brief ALG Link: OSD API
*/

#ifndef _OSD_LINK_H_
#define _OSD_LINK_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Include's    */

/* Defines */

/* @{ */

/**
    \brief Max supported OSD windows
*/
#define ALG_LINK_OSD_MAX_WINDOWS (10)

/**
    \brief Max supported OSD channels
*/
#define ALG_LINK_OSD_MAX_CH      (48)

/**
    \brief Max width alignment required for OSD window

    - SIMCOP based OSD needs 4 pixels
    - DSP based OSD needs 8 pixels

    This define defines the max required one
*/
#define ALG_LINK_OSD_WIN_WIDTH_ALIGN_MAX    8


/* @} */

/* Control Commands */

/**
    \ingroup ALG_LINK_API_CMD
    \addtogroup ALG_OSD_LINK_API_CMD  ALG Link: OSD API Control Commands

    @{
*/

/**
    \brief Link CMD: Configure OSD params for a given channel

    SUPPORTED in ALL platforms

    NOTE,
    - Individual commands to set individual OSD parameters like Width/Height is not supported.
    - Recommendation is that user maintain the OSD parameters for each channel in the app
        and then when any update is needed in a specific parameter,
        they can update the local parameters and send this command with the local parameters as input
        to update OSD parameters for that channel.
        All parameter specified in this command get applied on the specified channel.
    - When OSD bitmap content needs to be updated to need to send a new command,
        The content can be updated in the OSD bitmap buffer that is set in the earlier command for that channel.
    - The OSD buffer can contain any data in any language, the OSD buffer just blends the
        data passed by the user. Fonts, multi-lingual support etc is handled by the application.

    \param AlgLink_OsdChWinParams * [IN] OSD Window parameters
*/
#define ALG_LINK_OSD_CMD_SET_CHANNEL_WIN_PRM            (0x6001)


/**
    \brief Link CMD: Set blind window params

    ONLY SUPPORTED in DM810x

    NOTE,
    - This is similar in stype to OSD bitmap window ONLY here a solid widnow of
        a specified color is drawn on top of the video
    - Since drawing can be done more efficiently via EDMA than SIMCOP,
        a separate command is added to support this in SIMCOP based OSD
    - For DSP based blind window can be implemented using normal OSD window
        and keeping the buffer data to be a solid color

    \param AlgLink_OsdChBlindWinParams * [IN] Blind window parameters
*/
#define ALG_LINK_OSD_CMD_SET_CHANNEL_BLIND_WIN_PRM      (0x6002)

/* @}  */

/* Data structure's */

/**
  \brief OSD window parameter's
*/
typedef struct
{
  UInt8  *addr[2][2];
  /**< OSD buffer pointer

        - MUST be physical address.
            - Buffer MUST be contigous in memory, i.e malloc() buffer in Linux will NOT work.
            - User MUST take care of virtual to physical address translation when using Linux
            - Recommended to use Vsys_allocBuf() API to alloc the OSD buffer

        - When data format is YUV422
            - addr[0][0] - YUV422 plane pointer

        - When data format is YUV420
            - addr[0][0] - Y-plane pointer
            - addr[0][1] - C-plane pointer

        - Other array element's are not used.
    */

  UInt32 format;
  /**< SYSTEM_DF_YUV420SP_UV or SYSTEM_DF_YUV422I_YUYV

    SYSTEM_DF_YUV422I_YUYV
    - ONLY SUPPORTED in DM816x, DM814x

    SYSTEM_DF_YUV420SP_UV
    - SUPPORTED in ALL platforms
  */

  UInt32 startX;
  /**< Start position of window in X direction,
        specified in pixels,
        relative to start of video window,
        must be multiple of 2
    */

  UInt32 startY;
  /**< Start position of window in Y direction,
        specified in lines,
        relative to start of video window,
        must be multiple of 2
    */

  UInt32 width;
  /**< Width of window,
        specified in pixels,
        must be multiple of 4 in SIMCOP based OSD
        must be multiple of 8 in DSP based OSD
    */

  UInt32 height;
  /**< Height of window,
        specified in pixels,
        must be multiple of 4
    */


  UInt32 lineOffset;
  /**<
        Line offset in pixels,
        must be >= width, must be multiple of 4,
        recommended to be multiple of 32 for DDR efficiency
    */

  UInt32 globalAlpha;
  /**<
        8-bit global Alpha Value, used only if Alpha window is not enabled,
        Set to 0 to disable alpha blend.

        0   : Min Alpha, show only video
        0x80: Max Alpha, show only OSD

        Supported in DM816x, DM814x
        - on per CH, per Window basis.
        - can be changed dynamically

        Supported in DM810x with the following restrictions,
        - Value specified for CH0, WIN0 is applied for all CHs and all windows
        - Value specified during create time is used and cannot be updated at run-time
    */

  UInt32 transperencyEnable;
  /**<
        TRUE: enable transperency,

        when OSD pixel = AlgLink_OsdChWinParams.colorKey[]
        then
            video is shown
        else
            OSD pixel is blended with Video taking into account AlgLink_OsdWindowPrm.globalAlpha

        FALSE: disable transperency

        OSD pixel is always blended with Video taking into account AlgLink_OsdWindowPrm.globalAlpha

        Supported in DM816x, DM814x
        - on per CH, per Window basis.
        - can be changed dynamically

        Supported in DM810x with the following restrictions,
        - Value specified for CH0, WIN0 is applied for all CHs and all windows
        - Value specified during create time is used and cannot be updated at run-time
    */

  UInt32 enableWin;
   /**< TRUE: Enable display of OSD window for the channel
        FALSE: Disable display of OSD window for the channel
    */

} AlgLink_OsdWindowPrm;


/**
    \brief OSD channel - window parameters

    Specifies OSD parameters that can be changed dynamically
    on a per channel basis

    See structure/field details for restrictions applicable for DM810x platform.
*/
typedef struct AlgLink_OsdChWinParams
{
    UInt32 chId;
    /**< OSD channel number.
         Valid values: 0 .. \ref ALG_LINK_OSD_MAX_CH-1
    */

    UInt32 numWindows;
    /**< Number of OSD windows for this channel.
         Valid values: 0.. \ref ALG_LINK_OSD_MAX_WINDOWS
    */

    AlgLink_OsdWindowPrm winPrm[ALG_LINK_OSD_MAX_WINDOWS];
    /**< OSD window parameters */

    UInt32  colorKey[3];
    /**< Color key for Y, U, V.
        This is used when AlgLink_OsdWindowPrm.transperencyEnable = TRUE

        colorKey[0] = Y,
        colorKey[1] = U,
        colorKey[2] = V,
    */

} AlgLink_OsdChWinParams;

/**
  \brief OSD blind window parameter's

    Blind window is rectangle which is drawn on top of the video to cover
    the specified video region.

    This is useful in application like privacy masking.
*/
typedef struct
{
  UInt32 startX;
  /**< Start position of window in X direction,
        specified in pixels,
        relative to start of video window,
        must be multiple of 2
    */

  UInt32 startY;
  /**< Start position of window in Y direction,
        specified in lines,
        relative to start of video window,
        must be multiple of 2
    */

  UInt32 width;
  /**< Width of window,
        specified in pixels,
        must be multiple of 4
    */

  UInt32 height;
  /**< Height of window,
        specified in pixels,
        must be multiple of 4
    */

  UInt32 fillColorYUYV;
  /**< Windows color in YUYV format */

  UInt32 enableWin;
  /**< TRUE: Draw this blind area,
        FALSE: Do not draw this area
    */
} AlgLink_OsdBlindWindowPrm;

/**
    \brief OSD channel - blind window parameters

    Specifies blind window parameters that can be changed dynamically
    on a per channel basis

    SUPPORTED only in DM810x
*/
typedef struct
{
    UInt32                  chId;
    /**< OSD channel number.
         Valid values: 0 .. \ref ALG_LINK_OSD_MAX_CH-1
    */

    UInt32                  numWindows;
    /**< Number of blind windows for this channel.
         Valid values: 0.. \ref ALG_LINK_OSD_MAX_WINDOWS
    */

    AlgLink_OsdBlindWindowPrm    winPrm[ALG_LINK_OSD_MAX_WINDOWS];
    /**< Blind window parameters */

} AlgLink_OsdChBlindWinParams;

/**
    \brief OSD channel create time configuration parameters
*/
typedef struct
{
    UInt32 maxWidth;
    /**< Set the max width of OSD window */

    UInt32 maxHeight;
    /**< Set the max height of OSD window */

    AlgLink_OsdChWinParams chDefaultParams;
    /**< Initial window params for all channels */

//	AlgLink_OsdChBlindWinParams ChBlindWinParams;

} AlgLink_OsdChCreateParams;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

/*@}*/

