/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
    \ingroup LINK_API
    \defgroup DUP_LINK_API Frame Duplicator (DUP) Link API

    @{
*/

/**
    \file dupLink.h
    \brief Frame Duplicator (DUP) Link API
*/

#ifndef _DUP_LINK_H_
#define _DUP_LINK_H_

/* Include's    */

#include <mcfw/interfaces/link_api/system.h>

/* Define's */

/* @{ */

/** \brief Max output queues to which a given DUP link can connect to */
#define DUP_LINK_MAX_OUT_QUE	(5)

/* @} */

/* Data structure's */
/* @{ */

/**
    \brief DUP link create parameters
*/
typedef struct
{
    System_LinkInQueParams   inQueParams;
    /**< Input queue information */

    UInt32  numOutQue;
    /**< Number of output queues */

    System_LinkOutQueParams   outQueParams[DUP_LINK_MAX_OUT_QUE];
    /**< Input queue information */

	UInt32  notifyNextLink;
	/**< TRUE: send command to next link notifying that new data is ready in que */

} DupLink_CreateParams;

/* @} */

/* function's */
/* @{ */

/**
    \brief DUP link register and init

    - Creates link task
    - Registers as a link with the system API

    \return FVID2_SOK on success
*/
Int32 DupLink_init();

/**
    \brief DUP link de-register and de-init

    - Deletes link task
    - De-registers as a link with the system API

    \return FVID2_SOK on success
*/
Int32 DupLink_deInit();
/* @} */

#endif

/*@}*/
