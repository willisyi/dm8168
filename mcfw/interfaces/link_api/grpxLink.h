/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2012 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
    \ingroup LINK_API
    \defgroup GRPX_LINK_API GRPX Link API

    - This API can be used to setup and control the GRPX plane on displays.
    - This API can be used in place of FBDev in situations where simple GRPX control is needed.
        - This API is not intended to replace FBDev
        - This API only offers a single buffer mode of operation
    - This API should NOT be used with FBDev in parallel.

    @{
*/

/**
    \file grpxLink.h
    \brief GRPX Link API
*/

#ifndef _GRPX_LINK_H
#define _GRPX_LINK_H

#ifdef __cplusplus
extern "C" {
#endif

/* Include's    */

#include <mcfw/interfaces/link_api/system.h>
#include <mcfw/interfaces/common_def/ti_vgrpx_common_def.h>


/* Control Commands */

/**
    \ingroup LINK_API_CMD
    \addtogroup GRPX_LINK_API_CMD  GRPX Link API Control Commands

    @{
*/


/**
    \brief Link CMD: Set dynamic control parameter to the GRPX plane

    \param VGRPX_DYNAMIC_PARAM_S * [IN] Dynamic parameters
*/
#define GRPX_LINK_CMD_SET_DYNAMIC_PARAMS     (0xB001)

/**
    \brief Link CMD: Get buffer allocation info

    This command is useful when the same buffer needs to shared across
    two GRPX planes.

    In this case this command can be used to get the buffer info
    and this buffer info can be passed to the second GRPX plane
    via VGRPX_CREATE_PARAM_S

    \param VGRPX_BUFFER_INFO_S * [OUT] Buffer information
*/
#define GRPX_LINK_CMD_GET_BUFFER_INFO        (0xB002)


/* @}  */

/* Function's */

/**
    \brief GRPX link register and init

    - Creates link task
    - Registers as a link with the system API

    \return FVID2_SOK on success
*/
Int32 GrpxLink_init();

/**
    \brief GRPX link de-register and de-init

    - Deletes link task
    - De-registers as a link with the system API

    \return FVID2_SOK on success
*/
Int32 GrpxLink_deInit();


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* _GRPX_LINK_H */

/*@}*/
