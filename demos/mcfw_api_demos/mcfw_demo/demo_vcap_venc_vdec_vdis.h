/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2011 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
    \file demo_vcap_venc_vdec_vdis.h
    \brief Chains function related to IPC Bits links
*/


#ifndef _DEMO_VCAP_VENC_VDEC_VDIS_H_
#define _DEMO_VCAP_VENC_VDEC_VDIS_H_

#include <demo.h>
#include <demo_vcap_venc_vdec_vdis_bits_rdwr.h>


#if defined(TI_814X_BUILD) || defined(TI_8107_BUILD)
int Demo_change8ChMode(int demoId);
int Demo_change16ChMode(int demoId);
#endif


#endif
