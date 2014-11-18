/*******************************************************************************
 *                                                                            
 * Copyright (c) 2012 Texas Instruments Incorporated - http://www.ti.com/      
 *                        ALL RIGHTS RESERVED                                  
 *                                                                            
 ******************************************************************************/

/**
    \ingroup MCFW_API
    \defgroup MCFW_VCAP_API McFW Audio API

    @{
*/

/**
    \file ti_audio.h
    \brief McFW Audio Capture, Encode, Decode API
*/

#ifndef __TI_AUDIO_PRIV_H__
#define __TI_AUDIO_PRIV_H__


#ifdef __cplusplus
extern "C" {
#endif


#include "ti_media_common_def.h"
#include "ti_audio.h"
#include <link_api/audioLink.h>

typedef struct {

    ACAP_PARAMS_S   acapParams;
    Bool            rpeInitialized;
} ACAP_MODULE_CONTEXT_S;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif      /* __TI_AUDIO_PRIV_H__ */


