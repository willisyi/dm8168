/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2012 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
    \ingroup LINK_API
    \defgroup MERGE_LINK_API Frame Merge (MERGE) Link API

    MERGE Link can be used to merge several queues of channel data
    from other links into one output queue.

    @{
*/

/**
    \file mergeLink.h
    \brief Frame Merge (MERGE) Link API
*/

#ifndef _MERGE_LINK_H_
#define _MERGE_LINK_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Include's    */

#include <mcfw/interfaces/link_api/system.h>

/* Define's */

/* @{ */

/**
    \brief Max input queues to which a given MERGE link can connect to
*/
#define MERGE_LINK_MAX_IN_QUE	(6)

/* @} */

/**
    \ingroup LINK_API_CMD
    \addtogroup MERGE_LINK_API_CMD MERGE Link Control Commands

    @{
*/

/**
    \brief Link CMD: Get the channel info for the provided input link

    Merge link supports multiple input links which are then merged
    This link command is used to query the number of channels
    and the corresponding output channel number associated
    with a given input linkID.

    \param MergeLink_InLinkChInfo *   [IN/OUT]  parameters
*/
#define MERGE_LINK_CMD_GET_INPUT_LINK_CHINFO    (0x7001)

/* @} */

/* Data structure's */

/**
    \brief MERGE Link create parameters
*/
typedef struct
{
    UInt32 numInQue;
    /**< Number of inputs queue's */

    System_LinkInQueParams   inQueParams[MERGE_LINK_MAX_IN_QUE];
    /**< Input queue information */

    System_LinkOutQueParams   outQueParams;
    /**< Output queue information */

	UInt32 notifyNextLink;
	/**< TRUE: send command to next link notifying that new data is ready in que */

} MergeLink_CreateParams;

/**
    \brief MERGE_LINK_CMD_GET_INPUT_LINK_CHINFO command params
*/
typedef struct
{
    UInt32 inLinkID;
    /**< Input link ID whose chInfo is queried. This is an input parameter */
    UInt32 numCh;
    /**< Number of input channels */
    UInt32 startChNum;
    /**< The output merged channels associated with this link will start from
         this channel number
	 */
} MergeLink_InLinkChInfo;

/* function's */

/**
    \brief MERGE link register and init

    - Creates link task
    - Registers as a link with the system API

    \return FVID2_SOK on success
*/
Int32 MergeLink_init();

/**
    \brief MERGE link de-register and de-init

    - Deletes link task
    - De-registers as a link with the system API

    \return FVID2_SOK on success
*/
Int32 MergeLink_deInit();

/**
    \brief Set defaults for in link channel information

    \param inLinkChInfo [IN] Default information
*/
static inline void MergeLink_InLinkChInfo_Init(MergeLink_InLinkChInfo *inLinkChInfo)
{
    memset(inLinkChInfo,0,sizeof(*inLinkChInfo));
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

/*@}*/
