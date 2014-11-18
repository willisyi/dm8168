/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2012 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
    \ingroup LINK_API
    \defgroup NULL_SOURCE_LINK_API Null Source Link API

    Video Src Link can be used to provide input to the next frames
    This is used to integrate other links when capture link is not available.

    This is useful when capture link is not used but some input is needed for other links


    @{
*/

/**
    \file NullSrclink.h
    \brief Video Source Link API
*/

#ifndef _NULL_SRC_LINK_H_
#define _NULL_SRC_LINK_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Include's    */

#include <mcfw/interfaces/link_api/system.h>

/* Define's */

/* @{ */

/** \brief Max Channels per output queue */
#define NULL_SRC_LINK_MAX_CH_PER_OUT_QUE   (16)

/* @} */

/* Data structure's */

/**
*    brief Null Video source link create parameters
*/
typedef struct
{
    System_LinkOutQueParams   outQueParams;
    /**< output queue information */

    System_LinkQueInfo        inputInfo;
    /**< Input queue information */

    UInt32                    timerPeriod;
    /**< Time period at which Null/Dummy Output command has to be generated */

    UInt32                    tilerEnable;
    /**< tilerEnable = TRUE, enable TILER format for the output
         tilerEnable = FALSE, disable TILER format for the output. */

} NullSrcLink_CreateParams;

/* function's */

/**
    \brief Video source link register and init

    - Creates link task
    - Registers as a link with the system API

    \return FVID2_SOK on success
*/
Int32 NullSrcLink_init();

/**
    \brief Null Video source link de-register and de-init

    - Deletes link task
    - De-registers as a link with the system API

    \return FVID2_SOK on success
*/
Int32 NullSrcLink_deInit();

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

/*@}*/

