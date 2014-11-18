/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#ifndef _ENC_LINK_JPEG_PRIV_H_
#define _ENC_LINK_JPEG_PRIV_H_

#include <mcfw/interfaces/link_api/encLink.h>
#include <ijpegenc.h>

#include "encLink_algIf.h"

/**
 *  Analytic info output buffer size, this buffer is used to place MV & SAD of
 *  encoded frame, should be big enough to hold the size of  typical HD sequence
*/
#define ANALYTICINFO_OUTPUT_BUFF_SIZE      0x00028000
typedef struct IJPEGVENC_Obj *JPEGVENC_Handle;

typedef struct EncLink_JPEGObj {
    JPEGVENC_Handle algHandle;
    Int8 versionInfo[ENC_LINK_ALG_VERSION_STRING_MAX_LEN];
    Int linkID;
    Int channelID;
    Int scratchID;
    UInt32 ivaChID;
    IJPEGVENC_DynamicParams dynamicParams;
    IJPEGVENC_Status status;
    IJPEGVENC_Params staticParams;
    IJPEGVENC_InArgs inArgs;
    IJPEGVENC_OutArgs outArgs;
    IVIDEO2_BufDesc inBufs;
    XDM2_BufDesc outBufs;
    IVIDEO_Format format;
    UInt32 memUsed[UTILS_MEM_MAXHEAPS];
} EncLink_JPEGObj;

Int EncLinkJPEG_algCreate(EncLink_JPEGObj * hObj,
                          EncLink_AlgCreateParams * algCreateParams,
                          EncLink_AlgDynamicParams * algDynamicParams,
                          Int linkID, Int channelID, Int scratchGroupID);
Void EncLinkJPEG_algDelete(EncLink_JPEGObj * hObj);

#endif                                                     /* _ENC_LINK_PRIV_H_ */
