/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#ifndef _DEC_LINK_MJPEG_PRIV_H_
#define _DEC_LINK_MJPEG_PRIV_H_

#include <mcfw/interfaces/link_api/decLink.h>
#include <ijpegvdec.h>
#include "decLink_algIf.h"

typedef struct DecLink_JPEGObj {
    IJPEGVDEC_Handle algHandle;
    Int8 versionInfo[DEC_LINK_JPEG_VERSION_STRING_MAX_LEN];
    Int linkID;
    Int channelID;
    Int scratchID;
    UInt32 ivaChID;
    IJPEGVDEC_DynamicParams dynamicParams;
    IJPEGVDEC_Status status;
    IJPEGVDEC_Params staticParams;
    IJPEGVDEC_InArgs inArgs;
    IJPEGVDEC_OutArgs outArgs;
    XDM2_BufDesc inBufs;
    XDM2_BufDesc outBufs;
    UInt32 memUsed[UTILS_MEM_MAXHEAPS];
} DecLink_JPEGObj;

Int DecLinkJPEG_algCreate(DecLink_JPEGObj * hObj,
                          DecLink_AlgCreateParams * algCreateParams,
                          DecLink_AlgDynamicParams * algDynamicParams,
                          Int linkID, Int channelID, Int scratchGroupID,
                          FVID2_Format *pFormat, UInt32 numFrames,
                          IRES_ResourceDescriptor resDesc[]);
Void DecLinkJPEG_algDelete(DecLink_JPEGObj * hObj);

#endif                                                     /* _DEC_LINK_PRIV_H_
                                                            */
