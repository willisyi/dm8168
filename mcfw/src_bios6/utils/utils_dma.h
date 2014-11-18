/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
    \ingroup UTILS_API
    \defgroup UTILS_DMA_API DMA API
    @{
*/

#ifndef _UTILS_DMA_H_
#define _UTILS_DMA_H_

#include <mcfw/src_bios6/utils/utils.h>
#include <ti/sdo/edma3/drv/edma3_drv.h>
#include <ti/sdo/edma3/rm/edma3_rm.h>
#include <ti/sdo/edma3/rm/src/edma3resmgr.h>

#define UTILS_DMA_DEFAULT_EVENT_Q   (3)
#define UTILS_DMA_MAX_PITCH         (32767)
#define UTILS_DMA_MAX_PLANES 	(2)

typedef struct
{
    Ptr    destAddr[UTILS_DMA_MAX_PLANES];  /* MUST be 4-byte aligned */
    UInt32 destPitch[UTILS_DMA_MAX_PLANES]; /* MUST be 4-byte aligned */

    UInt32 dataFormat; /* Only  FVID2_DF_YUV422I_YUYV or FVID2_DF_YUV420SP_UV supported as of now */

    UInt32 startX;  /* in pixels, MUST be multiple of 2 of YUV422I and multiple of 4 for YUV420SP */
    UInt32 startY;  /* in lines, MUST be multiple of 2 for YUV420SP  */
    UInt32 width;   /* in pixels, MUST be multiple of 2 of YUV422I and multiple of 4 for YUV420SP */
    UInt32 height;  /* in lines, MUST be multiple of 2 for YUV420SP  */

    UInt32 fillColorYUYV; /* Color in YUYV format .*/

} Utils_DmaFill2D;

typedef struct
{
    Ptr    destAddr[UTILS_DMA_MAX_PLANES];  /* MUST be 4-byte aligned */
    UInt32 destPitch[UTILS_DMA_MAX_PLANES]; /* MUST be 4-byte aligned */

    Ptr    srcAddr[UTILS_DMA_MAX_PLANES];   /* MUST be 4-byte aligned */
    UInt32 srcPitch[UTILS_DMA_MAX_PLANES];  /* MUST be 4-byte aligned */

    UInt32 dataFormat; /* Only  FVID2_DF_YUV422I_YUYV or FVID2_DF_YUV420SP_UV supported as of now */

    UInt32 srcStartX;  /* in pixels, MUST be multiple of 2 of YUV422I and multiple of 4 for YUV420SP */
    UInt32 srcStartY;  /* in lines, MUST be multiple of 2 for YUV420SP  */
    UInt32 destStartX;  /* in pixels, MUST be multiple of 2 of YUV422I and multiple of 4 for YUV420SP */
    UInt32 destStartY;  /* in lines, MUST be multiple of 2 for YUV420SP  */

    UInt32 width;   /* in pixels, MUST be multiple of 2 of YUV422I and multiple of 4 for YUV420SP */
    UInt32 height;  /* in lines, MUST be multiple of 2 for YUV420SP  */

} Utils_DmaCopy2D;

typedef struct UtilsDmaBlankPixel_s
{
    UInt32 pixel;
} Utils_DmaBlankPixel;

/*
    User should not set or modify any of the fields in this structure.

    This fields are initialized and used by the API internally
*/
typedef struct {

    unsigned int iChannel;
    unsigned int iTcc;
    unsigned int iParam;

    Semaphore_Handle semComplete;
    Semaphore_Handle semLock;

    UInt32 maxTransfers;
    UInt32 curTx;
    UInt32 numTx;

    Bool enableIntCb;

    EDMA3_DRV_PaRAMRegs *pParamSet;
    EDMA3_RM_ResDesc resObj;
    EDMA3_RM_PaRAMRegs *pRMParamSet;

    Utils_DmaBlankPixel * pBlankPixel;
    Utils_DmaBlankPixel * pBlankPixelPhysAddr;

} Utils_DmaChObj;

Int32 Utils_dmaInit();
Int32 Utils_dmaDeInit();

Int32 Utils_dmaOpen();
Int32 Utils_dmaClose();

Int32 Utils_dmaCreateCh(Utils_DmaChObj *pObj, UInt32 eventQ, UInt32 maxTransfers, Bool enableIntCb);

Int32 Utils_dmaFill2D(Utils_DmaChObj *pObj, Utils_DmaFill2D *pInfo, UInt32 numTransfers);
Int32 Utils_dmaCopy2D(Utils_DmaChObj *pObj, Utils_DmaCopy2D *pInfo, UInt32 numTransfers);

Int32 Utils_dmaDeleteCh(Utils_DmaChObj *pObj);

Int32 Utils_dmaCreateChDSP(Utils_DmaChObj *pObj, UInt32 eventQ, UInt32 maxTransfers, Bool enableIntCb);
Int32 Utils_dmaDeleteChDSP(Utils_DmaChObj *pObj);

#define UTILS_DMA_GENERATE_FILL_PATTERN(y,u,v)             ((((y) & 0xFF) << 0)  | \
                                                            (((u) & 0xFF) << 8)  | \
                                                            (((y) & 0xFF) << 16) | \
                                                            (((v) & 0xFF) << 24))
#endif

/* @} */
