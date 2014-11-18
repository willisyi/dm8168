/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#ifndef _ENC_LINK_H264_PRIV_H_
#define _ENC_LINK_H264_PRIV_H_

#include <mcfw/interfaces/link_api/encLink.h>
#include <ih264enc.h>
#include "encLink_algIf.h"


typedef struct EncLink_H264Obj {
    IH264ENC_Handle algHandle;
    Int8 versionInfo[ENC_LINK_ALG_VERSION_STRING_MAX_LEN];
    Int linkID;
    Int channelID;
    Int scratchID;
    UInt32 ivaChID;
    IH264ENC_DynamicParams dynamicParams;
    IH264ENC_Status status;
    IH264ENC_Params staticParams;
    IH264ENC_InArgs inArgs;
    IH264ENC_OutArgs outArgs;
    IVIDEO2_BufDesc inBufs;
    XDM2_BufDesc outBufs;
    IVIDEO_Format format;
    UInt32 memUsed[UTILS_MEM_MAXHEAPS];
} EncLink_H264Obj;

Int EncLinkH264_algCreate(EncLink_H264Obj * hObj,
                          EncLink_AlgCreateParams * algCreateParams,
                          EncLink_AlgDynamicParams * algDynamicParams,
                          Int linkID, Int channelID, Int scratchGroupID);
Void EncLinkH264_algDelete(EncLink_H264Obj * hObj);
#endif                                                     /* _ENC_LINK_PRIV_H_
                                                            */
