/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#ifndef _DEC_LINK_H264_PRIV_H_
#define _DEC_LINK_H264_PRIV_H_

#include <mcfw/interfaces/link_api/decLink.h>
#include <ih264vdec.h>
#include "decLink_algIf.h"

typedef struct DecLink_H264Obj {
    IH264VDEC_Handle algHandle;
    Int8 versionInfo[DEC_LINK_H264_VERSION_STRING_MAX_LEN];
    Int linkID;
    Int channelID;
    Int scratchID;
    UInt32 ivaChID;
    IH264VDEC_DynamicParams dynamicParams;
    IH264VDEC_Status status;
    IH264VDEC_Params staticParams;
    IH264VDEC_InArgs inArgs;
    IH264VDEC_OutArgs outArgs;
    XDM2_BufDesc inBufs;
    XDM2_BufDesc outBufs;
    UInt32 numProcessCalls;
    UInt32 memUsed[UTILS_MEM_MAXHEAPS];
} DecLink_H264Obj;

Int DecLinkH264_algCreate(DecLink_H264Obj * hObj,
                          DecLink_AlgCreateParams * algCreateParams,
                          DecLink_AlgDynamicParams * algDynamicParams,
                          Int linkID, Int channelID, Int scratchGroupID,
                          FVID2_Format *pFormat, UInt32 numFrames,
                          IRES_ResourceDescriptor resDesc[]);
Void DecLinkH264_algDelete(DecLink_H264Obj * hObj);

#endif                                                     /* _DEC_LINK_PRIV_H_
                                                            */
