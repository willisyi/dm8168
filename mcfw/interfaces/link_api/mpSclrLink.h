/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2012 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
    \ingroup LINK_API
    \defgroup MP_SCLR_LINK_API Mega Pixel Scalar Link API

    Mega pixel Scalar link can be used to scale video frames of any size. This
    link will accept video frame of any size, from previous link and if the 
    frame size is less than or equals to 1920X1080, the scaling is bypassed 
    and the input frame passed onto the next link.
    
    If the frame size is greater than 1920X1080, the frames is scaled down to
    pre-configured size (1920X1080) and passed onto next link.

    MP_SCLR has one output
    - Output from SC5 Scalar
    - For scaled frames, output data format YUV422I in non-tiled memory.
    - For the frame that are not scaled, input frames are passed without 
        processing to next link.
    @{
*/

/**
    \file mpSclrLink.h
    \brief Mega Pixel Scalar (MP_SCLR) Link API
*/

#ifndef _MP_SCLR_LINK_H_
#define _MP_SCLR_LINK_H_

#include <mcfw/interfaces/link_api/system.h>
#include <mcfw/interfaces/link_api/vidframe.h>

#define MP_SCLR_LINK_OBJ_MAX        (SYSTEM_LINK_ID_MP_SCLR_COUNT)
/**< Number of instances of Mega Pixel Scalar to be initialized */

#define MP_SCLR_LINK_MAX_OUT_QUE    (1)
/**<  MP_SCLR Scalar output ID */

/**
    \ingroup LINK_API_CMD
    \addtogroup MP_SCLR_LINK_API_CMD MP SCLR Link Control Commands

    @{
*/

#define MP_SCLR_LINK_CMD_PRINT_STATISTICS       (0x7000)
/**< MP_SCLR command to print stats */
#define MP_SCLR_LINK_CMD_SET_OUTPUTRESOLUTION   (0x7001)
/**< MP_SCLR command to change the output resolution of a given channel. 
        This feature is not yet supported */
#define MP_SCLR_LINK_CMD_GET_OUTPUTRESOLUTION   (0x7002)
/**< MP_SCLR command to get current output resolution of given channel  
        This feature is not yet supported */
#define MP_SCLR_LINK_CMD_MAX                    (0x7003)
/**< MP_SCLR Command upper limit - Guard, does nothing */

/* @} */

#define MP_SCLR_LINK_SC5                        (0)
/**< MP_SCLR SC 5 instace to be used */
#define MP_SCLR_LINK_SEC0_SC3                   (1)
/**< MP_SCLR SC 3 instace to be used */
#define MP_SCLR_LINK_MAX_CH                     (4)
/**< Maximum channels per instance of MP SCLR */
#define MP_SCLR_LINK_DEFAULT_OUTPUT_WIDTH       (1920)
/**< Unless reconfigured with command MP_SCLR_LINK_CMD_SET_OUTPUTRESOLUTION,
    this link will scale Mega Pixel frame to this width.*/
#define MP_SCLR_LINK_DEFAULT_OUTPUT_HEIGHT      (1080)
/**< Unless reconfigured with command MP_SCLR_LINK_CMD_SET_OUTPUTRESOLUTION,
    this link will scale Mega Pixel frame to this height */
    
/**
    \brief MP_SCLR link set ouput resolution
*/
typedef struct
{
    UInt32 chId;
    /**< output channel Id */
    UInt32 width;
    /**< ouput width */
    UInt32 height;
    /**< ouput height */
    UInt32 pitch[VIDFRAME_MAX_PLANES];
    /**< ouput pitch */
} MpSclrLink_chDynamicSetOutRes;


/**
    \brief Mega Pixel Scalar link create parameters
*/
typedef struct
{
    System_LinkInQueParams      inQueParams;
    /**< Input queue information. */
    System_LinkOutQueParams     outQueParams;
    /**< Output queue information. */
    UInt32                      pathId;
    /**< Instace of scalar to be used */
    UInt32                      numCh;
    /**< Number of channels required */
    UInt32                      enableLineSkip;
    /**< Line skip config */
} MpSclrLink_CreateParams;


/**
    \brief MP_SCLR link register and initialize

    - Creates link task
    - Registers as a link with the system API

    \return FVID2_SOK on success
*/
Int32 MpSclrLink_init();

/**
    \brief MP_SCLR link de-register and de-init

    - Deletes link task
    - De-registers as a link with the system API

    \return FVID2_SOK on success
*/
Int32 MpSclrLink_deInit();

/**
    \brief MP_SCLR Initialize create parameters

    - Sets the create parameters to default valid values

    \return None
*/
static inline void MpSclrLink_CreateParams_Init(MpSclrLink_CreateParams *pPrm)
{

    memset(pPrm, 0, sizeof(*pPrm));

    pPrm->inQueParams.prevLinkId = SYSTEM_LINK_ID_INVALID;
    pPrm->inQueParams.prevLinkQueId = 0;

    pPrm->outQueParams.nextLink = SYSTEM_LINK_ID_INVALID;
    pPrm->enableLineSkip = FALSE;

    pPrm->pathId = MP_SCLR_LINK_SC5;
    pPrm->numCh = 1;
}

#endif /* _MP_SCLR_LINK_H_ */

/*@}*/

