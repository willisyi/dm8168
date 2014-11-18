/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2012 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
    \addtogroup GRPX_LINK_API

    @{
*/

/**
    \file ti_vgrpx_common_def.h
    \brief GRPX Link API - Common Data structure's and constant's

    This file defines Data structure's and constant's that are common
    between Link API and McFW API
*/


#ifndef __TI_VGRPX_COMMON_DEF_H__
#define __TI_VGRPX_COMMON_DEF_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Include's    */

#include "ti_vsys_common_def.h"


/**
    \brief GRPX Plane 0
*/
#define VGRPX_ID_0   (0)

/**
    \brief GRPX Plane 1
*/
#define VGRPX_ID_1   (1)

/**
    \brief GRPX Plane 2
*/
#define VGRPX_ID_2   (2)


/**
    \brief GRPX Plane Max ID
*/
#define VGRPX_ID_MAX (3)

/**
    \brief GRPX Data format - 16-bit RGB565
*/
#define VGRPX_DATA_FORMAT_RGB565        (0)

/**
    \brief GRPX Data format - 32-bit ARGB888
*/
#define VGRPX_DATA_FORMAT_ARGB888       (1)


/**
    \brief GRPX Scan format - interlaced
*/
#define VGRPX_SF_INTERLACED             (0)

/**
    \brief GRPX Scan format - progressive
*/
#define VGRPX_SF_PROGRESSIVE            (1)

/* Data structure's */

/**
    \brief GRPX plane dynamic parameters.

    These parameters can optionally be changed after GRPX link is started.
*/
typedef struct {

    UInt32 scaleEnable;
    /**<
        TRUE: enable scaling, here GRPX of
            VGRPX_DYNAMIC_PARAM_S.inWidth x VGRPX_DYNAMIC_PARAM_S.inHeight
            is scaled to
            VGRPX_DYNAMIC_PARAM_S.displayWidth x VGRPX_DYNAMIC_PARAM_S.displayHeight

        FALSE:
            disable GRPX scaling
            Here,
            VGRPX_DYNAMIC_PARAM_S.inWidth MUST be equal to VGRPX_DYNAMIC_PARAM_S.displayWidth
                And
            VGRPX_DYNAMIC_PARAM_S.inHeight MUST be equal to VGRPX_DYNAMIC_PARAM_S.displayHeight
    */

    UInt32 transperencyEnable;
    /**<
        TRUE: enable transperency based color keying

        FALSE: transperency is disabled

        When enabled,
        when GRPX pixel = VGRPX_DYNAMIC_PARAM_S.transperencyColor
        then
            video is shown
        else
            GRPX pixel is blended with video
    */

    UInt32 transperencyColor;
    /**<
        Transperency color, valid when VGRPX_DYNAMIC_PARAM_S.transperencyEnable = TRUE

        Specified in RGB888 format always as below

        bit  0 ..  8 = B
        bit  8 .. 15 = G
        bit 16 .. 23 = R
        bit 24 .. 31 = NOT USED
    */

    UInt32 inWidth;
    /**<
        Width of GRPX data in memory. Specified in pixels.

        MUST be <= VGRPX_BUFFER_INFO_S.bufferWidth
    */

    UInt32 inHeight;
    /**<
        Height of GRPX data in memory. Specified in lines

        MUST be <= VGRPX_BUFFER_INFO_S.bufferHeight
    */

    UInt32 displayWidth;
    /**<
        Width of GRPX on display.

        VGRPX_DYNAMIC_PARAM_S.displayStartX + VGRPX_DYNAMIC_PARAM_S.displayWidth
        MUST be <= selected display resolution width
    */

    UInt32 displayHeight;
    /**<
        Height of GRPX on display.

        VGRPX_DYNAMIC_PARAM_S.displayStartY + VGRPX_DYNAMIC_PARAM_S.displayHeight
        MUST be <= selected display resolution width
    */

    UInt32 displayStartX;
    /**<
        Start X position of GRPX on display

        VGRPX_DYNAMIC_PARAM_S.displayStartX + VGRPX_DYNAMIC_PARAM_S.displayWidth
        MUST be <= selected display resolution width
    */

    UInt32 displayStartY;
    /**<
        Start Y position of GRPX on display

        VGRPX_DYNAMIC_PARAM_S.displayStartY + VGRPX_DYNAMIC_PARAM_S.displayHeight
        MUST be <= selected display resolution width
    */

} VGRPX_DYNAMIC_PARAM_S;

/**
    \brief GRPX Plane buffer info
*/
typedef struct {

    UInt32 bufferPhysAddr;
    /**<
        Address of GRPX plane in memory

        - MUST be physical address.
            - Buffer MUST be contigous in memory, i.e malloc() buffer in Linux will NOT work.
            - User MUST take care of virtual to physical address translation when using Linux
            - Recommended to use Vsys_allocBuf() API to alloc the OSD buffer

        - if 0 or NULL is specified then buffer is allocated by the GRPX link
    */

    UInt32 bufferPitch;
    /**<
       Pitch of data in buffer. Specified in units of bytes.

       if 0 is specified, then pitch is auto-calculated by the link taking into account
       VGRPX_BUFFER_INFO_S.bufferWidth and VGRPX_BUFFER_INFO_S.dataFormat
    */


    UInt32 bufferWidth;
    /**<
        Width of buffer in memory. Specified in pixels.

        This will be used for memory allocation. Actual input size can less than this.
    */

    UInt32 bufferHeight;
    /**<
        Height of buffer in memory. Specified in lines.

        This will be used for memory allocation. Actual input size can less than this.
    */

    UInt32 dataFormat;
    /**<
        \ref VGRPX_DATA_FORMAT_RGB565 or \ref VGRPX_DATA_FORMAT_ARGB888
    */

    UInt32 scanFormat;
    /**<
       \ref VGRPX_SF_INTERLACED or \ref VGRPX_SF_PROGRESSIVE

       This MUST match actual display standard
    */

} VGRPX_BUFFER_INFO_S;

/**
    \brief GRPX Link create parameters
*/
typedef struct {

    UInt32 grpxId;
    /**<
        GRPX plane to be created

        \ref VGRPX_ID_0 or \ref VGRPX_ID_1 or \ref VGRPX_ID_2
    */


    VGRPX_BUFFER_INFO_S   bufferInfo;
    /**<
        Buffer information
    */

    VGRPX_DYNAMIC_PARAM_S dynPrm;
    /**<
        Initial parameters, can be changed later.
    */

} VGRPX_CREATE_PARAM_S;


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __TI_VGRPX_COMMON_DEF_H__ */

/* @} */
