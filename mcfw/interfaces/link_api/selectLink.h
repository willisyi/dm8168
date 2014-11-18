/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2012 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
    \ingroup LINK_API
    \defgroup SELECT_LINK_API Select Link API

    SELECT Link can be used to select expected channel(s) data
    from input channels.

    @{
*/

/**
    \file selectLink.h
    \brief Select Link API
*/

#ifndef _SELECT_LINK_H_
#define _SELECT_LINK_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Include's    */

#include <mcfw/interfaces/link_api/system.h>

/* Define's */

/* @{ */

/**
    \brief Max output queues to which a given SELECT link can connect to
*/
#define SELECT_LINK_MAX_OUT_QUE	(6)

/* @} */

/**
    \ingroup LINK_API_CMD
    \addtogroup SELECT_LINK_API_CMD SELECT Link Control Commands

    @{
*/

/**
    \brief Link CMD: Dynamically change channels mapped to a output que

    NOTE: Same input channel cannot be mapped to two different output queues

    \param SelectLink_OutQueChInfo  * [IN] channel mapping info
*/
#define SELECT_LINK_CMD_SET_OUT_QUE_CH_INFO         (0xB000)


/**
    \brief Link CMD: Get channel ID mapped to a output que

    SelectLink_OutQueChInfo.outQueId MUST be set by user.
    After API returns SelectLink_OutQueChInfo.numOutCh
    and SelectLink_OutQueChInfo.inChNum[] is filled with
    info of SelectLink_OutQueChInfo.outQueId

    \param SelectLink_OutQueChInfo * [IN/OUT] channel mapping info
*/
#define SELECT_LINK_CMD_GET_OUT_QUE_CH_INFO         (0xB001)

/* @} */

/* Data structure's */

/**
    \brief SELECT_LINK_CMD_GET_OUT_QUE_CH_INFO command params
    Information about which channels from input are selected at the output
*/
typedef struct
{
    UInt32 outQueId;
    /**< Que ID for which this is applicable
            - IGNORED when used with SelectLink_CreateParams
    */

    UInt32 numOutCh;
    /**< number of output channels in this output queue */

    UInt32 inChNum[SYSTEM_MAX_CH_PER_OUT_QUE];
    /**< input channel number which maps to this output queue */

} SelectLink_OutQueChInfo;

/**
    \brief SELECT Link create parameters
*/
typedef struct
{
    UInt32 numOutQue;
    /**< Number of inputs queue's */

    System_LinkInQueParams     inQueParams;
    /**< Input queue information. */

    System_LinkOutQueParams   outQueParams[SELECT_LINK_MAX_OUT_QUE];
    /**< Output queue information */

    SelectLink_OutQueChInfo   outQueChInfo[SELECT_LINK_MAX_OUT_QUE];
    /**< Information about which channels from input are selected at the output */

} SelectLink_CreateParams;

/* function's */

/**
    \brief SELECT link register and init

    - Creates link task
    - Registers as a link with the system API

    \return FVID2_SOK on success
*/
Int32 SelectLink_init();

/**
    \brief SELECT link de-register and de-init

    - Deletes link task
    - De-registers as a link with the system API

    \return FVID2_SOK on success
*/
Int32 SelectLink_deInit();

/**
    \brief Set defaults for create parameter

    \param pPrm [IN] Default parameters
*/
static void inline SelectLink_CreateParams_Init(SelectLink_CreateParams *pPrm)
{
    memset(pPrm, 0, sizeof(*pPrm));
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

/*@}*/
