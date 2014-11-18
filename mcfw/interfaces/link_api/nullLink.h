/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2012 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
    \ingroup LINK_API
    \defgroup NULL_LINK_API Null Sink Link API

    Null Link can be used to take input from a link and then without doing
    anything return it back to the same link.

    This useful when a link output cannot be given to any other link for testing
    purpose we just want to run a given link but not really use the output.

    In such cases the output queue of link can be connected to a Null link.

    The null link will operate like any other link from interface point of view.
    But it wont do anything with the frames it gets.
    It will simply return it back to the sending link.

    @{
*/

/**
    \file nullLink.h
    \brief Null Link API
*/

#ifndef _NULL_LINK_H_
#define _NULL_LINK_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Include's    */

#include <mcfw/interfaces/link_api/system.h>

/* Define's */

/* @{ */

/** \brief Max input queues to which a given null link can connect to */
#define NULL_LINK_MAX_IN_QUE        (4)

/* @} */


/* Data structure's */

/**
    \brief Null link create parameters
*/
typedef struct
{
    UInt32  numInQue;
    /**< Number of input queues */

    System_LinkInQueParams   inQueParams[NULL_LINK_MAX_IN_QUE];
    /**< Input queue information */

} NullLink_CreateParams;

/* function's */

/**
    \brief Null link register and init

    - Creates link task
    - Registers as a link with the system API

    \return FVID2_SOK on success
*/
Int32 NullLink_init();

/**
    \brief Null link de-register and de-init

    - Deletes link task
    - De-registers as a link with the system API

    \return FVID2_SOK on success
*/
Int32 NullLink_deInit();


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

/*@}*/
