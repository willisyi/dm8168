/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#ifndef _DEC_LINK_MPEG4_PRIV_H_
#define _DEC_LINK_MPEG4_PRIV_H_

#include <mcfw/interfaces/link_api/decLink.h>
#include <impeg4vdec.h>
#include "decLink_algIf.h"

typedef struct DecLink_MPEG4Obj {
    IMPEG4VDEC_Handle algHandle;
    Int8 versionInfo[DEC_LINK_MPEG4_VERSION_STRING_MAX_LEN];
    Int linkID;
    Int channelID;
    Int scratchID;
    UInt32 ivaChID;
    IMPEG4VDEC_DynamicParams dynamicParams;
    IMPEG4VDEC_Status status;
    IMPEG4VDEC_Params staticParams;
    IMPEG4VDEC_InArgs inArgs;
    IMPEG4VDEC_OutArgs outArgs;
    XDM2_BufDesc inBufs;
    XDM2_BufDesc outBufs;
    UInt32 memUsed[UTILS_MEM_MAXHEAPS];
} DecLink_MPEG4Obj;

Int DecLinkMPEG4_algCreate(DecLink_MPEG4Obj * hObj,
                           DecLink_AlgCreateParams * algCreateParams,
                           DecLink_AlgDynamicParams * algDynamicParams,
                           Int linkID, Int channelID, Int scratchGroupID,
                           FVID2_Format *pFormat, UInt32 numFrames,
                           IRES_ResourceDescriptor resDesc[]);
Void DecLinkMPEG4_algDelete(DecLink_MPEG4Obj * hObj);

#endif                                                     /* _DEC_LINK_MPEG4_PRIV_H_  */




