/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#ifndef _UTILS_ENCDEC_H_
#define _UTILS_ENCDEC_H_

#include <ti/xdais/xdas.h>
#include <ti/xdais/dm/xdm.h>
#include <ti/xdais/dm/ivideo.h>
#include <mcfw/interfaces/link_api/encLink.h>
#include <mcfw/interfaces/link_api/decLink.h>
#include <mcfw/interfaces/link_api/systemLink_m3video.h>
#include <mcfw/src_bios6/utils/utils_trace.h>

/* =============================================================================
 * All success and failure codes for the module
 * ========================================================================== */

/** @brief Operation successful. */
#define UTILS_ENCDEC_S_SUCCESS               (0)

/** @brief General Failure */
#define UTILS_ENCDEC_E_FAIL                  (-1)

/** @brief Unknow coding type */
#define UTILS_ENCDEC_E_UNKNOWNCODINGTFORMAT  (-2)

/** @brief Internal error: unknown resolution class */
#define UTILS_ENCDEC_E_INT_UNKNOWNRESOLUTIONCLASS    (-64)

#define UTILS_ENCDEC_ACTIVITY_LOG_LENGTH (64)

#ifdef TI_816X_BUILD
#    define  NUM_HDVICP_RESOURCES                    (3)
#else
#    if defined(TI_814X_BUILD) || defined(TI_8107_BUILD)
#        define  NUM_HDVICP_RESOURCES                (1)
#    else
#        error "Unknow Device.."
#    endif
#endif

/** @enum EncDec_ResolutionClass
 *  @brief Enumeration of different resolution class.
 */
typedef enum EncDec_ResolutionClass {
    UTILS_ENCDEC_RESOLUTION_CLASS_FIRST = 0,
    UTILS_ENCDEC_RESOLUTION_CLASS_16MP = UTILS_ENCDEC_RESOLUTION_CLASS_FIRST,
    UTILS_ENCDEC_RESOLUTION_CLASS_WUXGA,
    UTILS_ENCDEC_RESOLUTION_CLASS_1080P,
    UTILS_ENCDEC_RESOLUTION_CLASS_720P,
    UTILS_ENCDEC_RESOLUTION_CLASS_D1,
    UTILS_ENCDEC_RESOLUTION_CLASS_CIF,
    UTILS_ENCDEC_RESOLUTION_CLASS_LAST = UTILS_ENCDEC_RESOLUTION_CLASS_CIF,
    UTILS_ENCDEC_RESOLUTION_CLASS_COUNT =
        (UTILS_ENCDEC_RESOLUTION_CLASS_LAST + 1)
} EncDec_ResolutionClass;


typedef enum EncDec_AlgorithmType {
    UTILS_ALGTYPE_NONE = 0,
    UTILS_ALGTYPE_H264_ENC,
    UTILS_ALGTYPE_H264_DEC,
    UTILS_ALGTYPE_MJPEG_ENC,
    UTILS_ALGTYPE_MJPEG_DEC,
    UTILS_ALGTYPE_MPEG4_DEC,
    UTILS_ALGTYPE_LAST = UTILS_ALGTYPE_MPEG4_DEC,
    UTILS_ALGTYPE_COUNT =
        (UTILS_ALGTYPE_LAST + 1)
} EncDec_AlgorithmType;


typedef struct EncDec_AlgorithmActivityLog {
    EncDec_AlgorithmType algType[UTILS_ENCDEC_ACTIVITY_LOG_LENGTH];
    UInt32 writeIdx;
} EncDec_AlgorithmActivityLog;

#define UTILS_ENCDEC_RESOLUTION_CLASS_16MP_WIDTH                        (4*1024)
#define UTILS_ENCDEC_RESOLUTION_CLASS_16MP_HEIGHT                       (4*1024)

#define UTILS_ENCDEC_RESOLUTION_CLASS_WUXGA_WIDTH                         (1920)
#define UTILS_ENCDEC_RESOLUTION_CLASS_WUXGA_HEIGHT                        (1200)

#define UTILS_ENCDEC_RESOLUTION_CLASS_1080P_WIDTH                         (1920)
#define UTILS_ENCDEC_RESOLUTION_CLASS_1080P_HEIGHT                        (1080)

#define UTILS_ENCDEC_RESOLUTION_CLASS_720P_WIDTH                          (1280)
#define UTILS_ENCDEC_RESOLUTION_CLASS_720P_HEIGHT                          (720)

#define UTILS_ENCDEC_RESOLUTION_CLASS_D1_WIDTH                             (720)
#define UTILS_ENCDEC_RESOLUTION_CLASS_D1_HEIGHT                            (576)

#define UTILS_ENCDEC_RESOLUTION_CLASS_CIF_WIDTH                            (368)
#define UTILS_ENCDEC_RESOLUTION_CLASS_CIF_HEIGHT                           (288)


typedef Void (*Utils_encdecIVAMapChangeNotifyCb)(Ptr ctx,SystemVideo_Ivahd2ChMap_Tbl* tbl);

typedef struct Utils_encdecIVAMapChangeNotifyCbInfo {
    Utils_encdecIVAMapChangeNotifyCb       fxns;
    Ptr                                   ctx;
} Utils_encdecIVAMapChangeNotifyCbInfo;


/**
    \brief Check if coding type is H264

    This checks the IVIDEO format to see if it is H264 codec
    \param format        [IN] IVIDEO_Format enum

    \return TRUE if codec type is H264
*/
static inline Bool Utils_encdecIsH264(IVIDEO_Format format)
{
    Bool isH264;

    switch (format)
    {
        case IVIDEO_H264BP:
        case IVIDEO_H264MP:
        case IVIDEO_H264HP:
            isH264 = TRUE;
            break;
        default:
            isH264 = FALSE;
            break;
    }
    return isH264;
}
/**
    \brief Check if coding type is MPEG4

    This checks the IVIDEO format to see if it is MPEG4 codec
    \param format        [IN] IVIDEO_Format enum

    \return TRUE if codec type is MPEG4
*/
static inline Bool Utils_encdecIsMPEG4(IVIDEO_Format format)
{
    Bool isMPEG4;

    switch (format)
    {
        case IVIDEO_MPEG4SP:
        case IVIDEO_MPEG4ASP:
            isMPEG4 = TRUE;
            break;
        default:
            isMPEG4 = FALSE;
            break;
    }
    return isMPEG4;
}

/**
    \brief Check if coding type is MJPEG

    This checks the IVIDEO format to see if it is MJPEG codec
    \param format        [IN] IVIDEO_Format enum

    \return TRUE if codec type is MJPEG
*/
static inline Bool Utils_encdecIsJPEG(IVIDEO_Format format)
{
    Bool isjpeg;

    switch (format)
    {
        case IVIDEO_MJPEG:
            isjpeg = TRUE;
            break;
        default:
            isjpeg = FALSE;
            break;
    }
    return isjpeg;
}


static inline UInt32 Utils_encdecMapFVID2XDMContentType(UInt32 scanFormat)
{
    return ((scanFormat == FVID2_SF_INTERLACED) ? IVIDEO_INTERLACED :
            IVIDEO_PROGRESSIVE);
}

static inline UInt32 Utils_encdecMapFVID2XDMChromaFormat(UInt32 chromaFormat)
{
    UTILS_assert((chromaFormat == FVID2_DF_YUV420SP_UV) ||
                 (chromaFormat == FVID2_DF_YUV420SP_VU));
    return (XDM_YUV_420SP);
}

static inline UInt32 Utils_encdecMapXDMContentType2FVID2FID(UInt32 contentType)
{
    FVID2_Fid fid = FVID2_FID_MAX;

    switch (contentType)
    {
        case IVIDEO_INTERLACED_TOPFIELD:
            fid = FVID2_FID_TOP;
            break;
        case IVIDEO_INTERLACED_BOTTOMFIELD:
            fid = FVID2_FID_BOTTOM;
            break;
        default:
            // fid = FVID2_FID_FRAME;
            /* For progressive frame driver expects fid to be set to
             * FVID2_FID_TOP */
            fid = FVID2_FID_TOP;
            break;
    }
    return fid;
}

static inline IVIDEO_ContentType Utils_encdecMapFVID2FID2XDMContentType(FVID2_Fid fid)
{
    IVIDEO_ContentType contentType = IVIDEO_PROGRESSIVE;

    switch (fid)
    {
        case FVID2_FID_TOP:
            contentType = IVIDEO_INTERLACED_TOPFIELD;
            break;
        case FVID2_FID_BOTTOM:
            contentType = IVIDEO_INTERLACED_BOTTOMFIELD;
            break;
        case FVID2_FID_FRAME:
            contentType = IVIDEO_PROGRESSIVE_FRAME;
            break;
        default:
            contentType = IVIDEO_PROGRESSIVE_FRAME;
            break;
    }
    return contentType;
}

static inline Bool Utils_encdecIsGopStart(UInt32 frameType, UInt32 contentType)
{
    Bool isGopStart = FALSE;

    switch (frameType)
    {
        case IVIDEO_I_FRAME:
        case IVIDEO_IDR_FRAME:
        case IVIDEO_II_FRAME:
        case IVIDEO_MBAFF_I_FRAME:
        case IVIDEO_MBAFF_IDR_FRAME:
            isGopStart = TRUE;
            break;
        case IVIDEO_IP_FRAME:
        case IVIDEO_IB_FRAME:
            if (contentType == IVIDEO_INTERLACED_TOPFIELD)
            {
                isGopStart = TRUE;
            }
            break;
        case IVIDEO_PI_FRAME:
        case IVIDEO_BI_FRAME:
            if (contentType == IVIDEO_INTERLACED_BOTTOMFIELD)
            {
                isGopStart = TRUE;
            }
            break;
        default:
            isGopStart = FALSE;
            break;
    }
    return isGopStart;
}

/** @def   UTILS_ENCDEC_BITBUF_SCALING_FACTOR
 *  @brief Define that controls the size of bitbuf in realtion to its resoltuion
 */
#define UTILS_ENCDEC_BITBUF_SCALING_FACTOR                         (2)

/** @enum UTILS_ENCDEC_GET_BITBUF_SIZE
 *  @brief Macro that returns max size of encoded bitbuffer for a given resolution
 */

#define UTILS_ENCDEC_GET_BITBUF_SIZE(width,height,bitrate,framerate)          \
                    (((width) * (height))/2)

#define UTILS_ENCDEC_H264PADX                                              (32)
#define UTILS_ENCDEC_H264PADY                                              (24)

/** @enum UTILS_ENCDEC_GET_PADDED_WIDTH
 *  @brief Macro that padded width for given width */
#define UTILS_ENCDEC_GET_PADDED_WIDTH(width)                                   \
                  (((width) + (2 * UTILS_ENCDEC_H264PADX) + 127) & 0xFFFFFF80)

/** @enum UTILS_ENCDEC_GET_PADDED_HEIGHT
 *  @brief Macro that padded height for given height */
#define UTILS_ENCDEC_GET_PADDED_HEIGHT(height)                                 \
                                        ((height) + (4 * UTILS_ENCDEC_H264PADY))

Int Utils_encdecGetCodecLevel(UInt32 codingFormat,
                              UInt32 maxWidth,
                              UInt32 maxHeight,
                              UInt32 maxFrameRate,
                              UInt32 maxBitRate, Int32 * pLevel,
                              Bool isEnc);

Int Utils_encdecInit();

Int Utils_encdecDeInit();

Int Utils_encdecGetEncoderIVAID(UInt32 chId);

Int Utils_encdecGetDecoderIVAID(UInt32 chId);

Int Utils_encdecRegisterIVAMapChangeNotifyCb(Utils_encdecIVAMapChangeNotifyCbInfo *cbInfo);
Int Utils_encdecUnRegisterIVAMapChangeNotifyCb(Utils_encdecIVAMapChangeNotifyCbInfo *cbInfo);

#endif
