/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include "utils_dma_bios6.h"

Utils_DmaObj gUtils_dmaObj;

#define UTILS_DMA_M3_VIRT_2_PHYS_OFFSET     (0x80000000-0x20000000)

static inline UInt32 Utils_dmaVirtToPhys(UInt32 addr)
{
    UInt32 msbNibble;

    msbNibble = (addr >> 28);

    if(msbNibble >= 2 && msbNibble <= 3)
        addr+=UTILS_DMA_M3_VIRT_2_PHYS_OFFSET;

    return addr;
}

void Utils_dmaCallback(unsigned int tcc, EDMA3_RM_TccStatus status, void *appData)
{
    (void)tcc;
    Utils_DmaChObj *pObj = (Utils_DmaChObj *)appData;

    switch (status)
    {
        case EDMA3_RM_XFER_COMPLETE:
            Utils_dmaTrigger(pObj);
            break;
        case EDMA3_RM_E_CC_DMA_EVT_MISS:

            break;
        case EDMA3_RM_E_CC_QDMA_EVT_MISS:

            break;
        default:
            break;
    }
}


Int32 Utils_dmaInit()
{
    EDMA3_DRV_Result edma3Result = EDMA3_DRV_SOK;

    memset(&gUtils_dmaObj, 0, sizeof(gUtils_dmaObj));

    gUtils_dmaObj.hEdma = Utils_edma3init(gUtils_dmaObj.edma3InstanceId, &edma3Result);
    if (!gUtils_dmaObj.hEdma)
    {
        Vps_printf(
            " UTILS: DMA: Utils_dmaInit() ... FAILED (%d) \n",
                edma3Result
            );
    }

    return edma3Result;
}

Int32 Utils_dmaDeInit()
{
    EDMA3_DRV_Result edma3Result = EDMA3_DRV_SOK;

    edma3Result = Utils_edma3deinit(gUtils_dmaObj.edma3InstanceId, gUtils_dmaObj.hEdma);
    if (edma3Result != EDMA3_DRV_SOK)
    {
        Vps_printf(
            " UTILS: DMA: Utils_dmaDeInit() ... FAILED (%d) \n",
                edma3Result
            );
    }

    return edma3Result;
}

Int32 Utils_dmaCreateCh(Utils_DmaChObj *pObj, UInt32 eventQ, UInt32 maxTransfers, Bool enableIntCb)
{

    EDMA3_DRV_Result edma3Result = EDMA3_DRV_SOK;
    EDMA3_RM_TccCallback tccCb;
    Semaphore_Params semParams;

    memset(pObj, 0, sizeof(*pObj));

    maxTransfers *= 2; // to account for UV data transfer

    if(maxTransfers<UTILS_DMA_MIN_TX)
        maxTransfers = UTILS_DMA_MIN_TX;

    pObj->curTx = 0;
    pObj->numTx = 0;
    pObj->maxTransfers = maxTransfers;

    pObj->iTcc = EDMA3_DRV_TCC_ANY;
    pObj->iChannel = EDMA3_DRV_DMA_CHANNEL_ANY;

    Semaphore_Params_init(&semParams);

    semParams.mode = Semaphore_Mode_BINARY;

    pObj->semComplete = Semaphore_create(0u, &semParams, NULL);
    UTILS_assert(pObj->semComplete != NULL);


    Semaphore_Params_init(&semParams);

    semParams.mode = Semaphore_Mode_BINARY;

    pObj->semLock = Semaphore_create(1u, &semParams, NULL);
    UTILS_assert(pObj->semLock != NULL);

    pObj->enableIntCb = enableIntCb;

    if(pObj->enableIntCb)
        tccCb = &Utils_dmaCallback;
    else
        tccCb = NULL;

    if (edma3Result == EDMA3_DRV_SOK)
    {
        edma3Result = EDMA3_DRV_requestChannel (gUtils_dmaObj.hEdma, &pObj->iChannel, &pObj->iTcc,
                                       (EDMA3_RM_EventQueue)eventQ,
                                          tccCb, (void *)pObj);
    }

    if (edma3Result == EDMA3_DRV_SOK)
    {
        Vps_printf(" UTILS: DMA: Allocated CH (TCC) = %d (%d)\n", pObj->iChannel, pObj->iTcc);

        edma3Result = EDMA3_DRV_clearErrorBits (gUtils_dmaObj.hEdma,pObj->iChannel);
    }

    if (edma3Result == EDMA3_DRV_SOK)
    {
        UInt32 memSize = sizeof(EDMA3_DRV_PaRAMRegs)*pObj->maxTransfers;

        pObj->pParamSet = malloc(memSize);

        UTILS_assert(pObj->pParamSet != NULL);

        memset(pObj->pParamSet, 0, memSize);

        memSize = sizeof(Utils_DmaBlankPixel) * pObj->maxTransfers;

        pObj->pBlankPixel = malloc(memSize);

        UTILS_assert(pObj->pBlankPixel != NULL);

        pObj->pBlankPixelPhysAddr = (Utils_DmaBlankPixel*)Utils_dmaVirtToPhys((UInt32)pObj->pBlankPixel);

        memset(pObj->pBlankPixelPhysAddr, 0, memSize);
    }

    return edma3Result;
}

Int32 Utils_dmaDeleteCh(Utils_DmaChObj *pObj)
{
    EDMA3_DRV_Result edma3Result = EDMA3_DRV_SOK;

    edma3Result = EDMA3_DRV_freeChannel(gUtils_dmaObj.hEdma, pObj->iChannel);

    free(pObj->pParamSet);
    free(pObj->pBlankPixel);
    pObj->pParamSet = NULL;

    Semaphore_delete(&pObj->semComplete);
    Semaphore_delete(&pObj->semLock);

    Vps_printf(" UTILS: DMA: Free'ed CH (TCC) = %d (%d)\n", pObj->iChannel, pObj->iTcc);

    return edma3Result;
}

Int32 Utils_dmaPollWaitComplete(Utils_DmaChObj *pObj)
{
    uint16_t tccStatus;

    EDMA3_DRV_waitAndClearTcc(gUtils_dmaObj.hEdma,pObj->iTcc);
    EDMA3_DRV_checkAndClearTcc(gUtils_dmaObj.hEdma,pObj->iTcc, &tccStatus);
    EDMA3_DRV_clearErrorBits (gUtils_dmaObj.hEdma,pObj->iChannel);

    return 0;
}

Int32 Utils_dmaTrigger(Utils_DmaChObj *pObj)
{
    EDMA3_DRV_Result edma3Result = EDMA3_DRV_SOK;
    uint16_t tccStatus;

    edma3Result |= EDMA3_DRV_checkAndClearTcc(gUtils_dmaObj.hEdma,pObj->iTcc, &tccStatus);
    edma3Result |= EDMA3_DRV_clearErrorBits (gUtils_dmaObj.hEdma,pObj->iChannel);

    if(pObj->curTx>=pObj->numTx)
    {

    /* all transfers complete */
        Semaphore_post(pObj->semComplete);

        return edma3Result;
    }

    edma3Result = EDMA3_DRV_setPaRAM(
                    gUtils_dmaObj.hEdma,
                    pObj->iChannel,
                    &pObj->pParamSet[pObj->curTx]
                    );

    if (edma3Result == EDMA3_DRV_SOK)
    {
        edma3Result |= EDMA3_DRV_enableTransfer (gUtils_dmaObj.hEdma,pObj->iChannel,
                                           EDMA3_DRV_TRIG_MODE_MANUAL);
    }

    if (edma3Result != EDMA3_DRV_SOK)
    {
        Vps_printf(
            " UTILS: DMA: %d: Utils_dmaTrigger() ... FAILED (%d) \n",
                pObj->curTx, edma3Result
            );
    }

    pObj->curTx++;

    return edma3Result;
}

Int32 Utils_dmaParamSetCheck(EDMA3_DRV_PaRAMRegs *pParamSet)
{

    if(pParamSet->aCnt == 0
        ||
       pParamSet->bCnt == 0
        ||
       pParamSet->cCnt == 0
        ||
       pParamSet->destAddr == NULL
        ||
       pParamSet->srcAddr == NULL
        ||
       pParamSet->bCntReload == 0
    )
    {
        return FVID2_EFAIL;
    }

    return FVID2_SOK;
}

Int32 Utils_dmaFill2D(Utils_DmaChObj *pObj, Utils_DmaFill2D *pInfo, UInt32 numTransfers)
{
    EDMA3_DRV_Result edma3Result = EDMA3_DRV_SOK;
    EDMA3_DRV_PaRAMRegs *pParamSet;
    UInt32 bpp; /* bytes per pixel */
    Utils_DmaBlankPixel *pBlankPixel;
    UInt32 i;

    if(pInfo->dataFormat==FVID2_DF_YUV420SP_UV)
    {
        /* UV and Y will be transferred separately */
        if(numTransfers>pObj->maxTransfers/2)
            return FVID2_EFAIL;
    }

    if(numTransfers>pObj->maxTransfers)
        return FVID2_EFAIL;

    Semaphore_pend(pObj->semLock, BIOS_WAIT_FOREVER);

    for(i=0; i<numTransfers; i++)
    {
        pObj->curTx = 0;
        pObj->numTx = 0;

        pParamSet = &pObj->pParamSet[pObj->numTx];
        pBlankPixel = &pObj->pBlankPixelPhysAddr[pObj->numTx];

        pObj->numTx++;

        bpp = 2;
        if(pInfo->dataFormat==FVID2_DF_YUV420SP_UV)
            bpp = 1;

        pParamSet->destAddr   = (UInt32)pInfo->destAddr[0]
                              + pInfo->destPitch[0]*pInfo->startY
                              + pInfo->startX * bpp;
        pParamSet->srcAddr    = (uint32_t)&(pBlankPixel->pixel);

        if(pInfo->dataFormat==FVID2_DF_YUV420SP_UV)
        {
            /* set fill color for Y-plane */
            UInt8 y0 = (pInfo->fillColorYUYV & 0x000000FF);
            UInt8 y1 = (((pInfo->fillColorYUYV & 0x00FF0000) >> 16) & 0xFF);

            pBlankPixel->pixel = (y0<<0) | (y1<<8) | (y0<<16) | (y1<<24);
        }
        else
        {
            /* set the fill color */
            pBlankPixel->pixel = pInfo->fillColorYUYV;
        }

        pParamSet->srcBIdx    = 0;
        pParamSet->destBIdx   = UTILS_DMA_BYTES_TO_WRITE;
        pParamSet->srcCIdx    = 0;
        pParamSet->destCIdx   = pInfo->destPitch[0];

        pParamSet->aCnt       = UTILS_DMA_BYTES_TO_WRITE;
        pParamSet->bCnt       = (pInfo->width*bpp)/UTILS_DMA_BYTES_TO_WRITE;
        pParamSet->cCnt       = pInfo->height;
        /* If cCnt is zero DMA xfer doesnt happen.So set cCnt to 1 if cCnt happens to be zero */
        if (0 == pParamSet->cCnt)
            pParamSet->cCnt = 1;

        pParamSet->bCntReload = pParamSet->bCnt;

        pParamSet->linkAddr   = 0xFFFFu;

        pParamSet->opt = 0;
        pParamSet->opt |= (EDMA3_CCRL_OPT_ITCCHEN_ENABLE << EDMA3_CCRL_OPT_ITCCHEN_SHIFT);
        pParamSet->opt |= (EDMA3_CCRL_OPT_TCINTEN_ENABLE << EDMA3_CCRL_OPT_TCINTEN_SHIFT);
        pParamSet->opt |= ((pObj->iTcc << EDMA3_CCRL_OPT_TCC_SHIFT) & EDMA3_CCRL_OPT_TCC_MASK);
        pParamSet->opt |= (EDMA3_CCRL_OPT_SYNCDIM_ABSYNC << EDMA3_CCRL_OPT_SYNCDIM_SHIFT);
        Cache_wb((Ptr)pParamSet->srcAddr,UTILS_DMA_BYTES_TO_WRITE,TRUE,NULL);

        UTILS_assert(
            Utils_dmaParamSetCheck(pParamSet) == FVID2_SOK
        );

        edma3Result = Utils_dmaTrigger(pObj);

        if (edma3Result == EDMA3_DRV_SOK)
        {
            if(pObj->enableIntCb)
            {
                Semaphore_pend(pObj->semComplete, BIOS_WAIT_FOREVER);
            }
            else
            {
                Utils_dmaPollWaitComplete(pObj);
            }
        }

        if(pInfo->dataFormat==FVID2_DF_YUV420SP_UV)
        {
            pObj->curTx = 0;
            pObj->numTx = 0;

            /* setup PaRAM for UV plane */
            pParamSet = &pObj->pParamSet[pObj->numTx];
            pBlankPixel = &pObj->pBlankPixelPhysAddr[pObj->numTx];
            pObj->numTx++;

            bpp = 1;

            /* for chroma plane when input or output is tiled we need to set pitch as -32KB
                since other wise pitch value will overflow the 16-bit register in EDMA.

                Hence all for all Chroma TX's we set pitch as -pitch and DMA from last line
                to first line
            */

            pParamSet->destAddr   = (UInt32)pInfo->destAddr[1]
                                  + pInfo->destPitch[1]*((pInfo->startY+pInfo->height)/2 - 1)
                                  + pInfo->startX * bpp;
            pParamSet->srcAddr    = (uint32_t)&(pBlankPixel->pixel);

            {
                /* set fill color for C-plane */
                UInt8 c0 = (((pInfo->fillColorYUYV & 0x0000FF00) >> 8)  & 0xFF);
                UInt8 c1 = (((pInfo->fillColorYUYV & 0xFF000000) >> 24) & 0xFF);

                pBlankPixel->pixel = (c0<<0) | (c1<<8) | (c0<<16) | (c1<<24);
            }

            pParamSet->srcBIdx    = 0;
            pParamSet->destBIdx   = UTILS_DMA_BYTES_TO_WRITE;
            pParamSet->srcCIdx    = 0;
            pParamSet->destCIdx   = -pInfo->destPitch[1];

            pParamSet->aCnt       = UTILS_DMA_BYTES_TO_WRITE;
            pParamSet->bCnt       = (pInfo->width*bpp)/UTILS_DMA_BYTES_TO_WRITE;
            pParamSet->cCnt       = pInfo->height/2;
            /* If cCnt is zero DMA xfer doesnt happen.So set cCnt to 1 if cCnt happens to be zero */
            if (0 == pParamSet->cCnt)
                pParamSet->cCnt = 1;

            pParamSet->bCntReload = pParamSet->bCnt;

            pParamSet->linkAddr   = 0xFFFFu;

            pParamSet->opt = 0;
            pParamSet->opt |= (EDMA3_CCRL_OPT_ITCCHEN_ENABLE << EDMA3_CCRL_OPT_ITCCHEN_SHIFT);
            pParamSet->opt |= (EDMA3_CCRL_OPT_TCINTEN_ENABLE << EDMA3_CCRL_OPT_TCINTEN_SHIFT);
            pParamSet->opt |= ((pObj->iTcc << EDMA3_CCRL_OPT_TCC_SHIFT) & EDMA3_CCRL_OPT_TCC_MASK);
            pParamSet->opt |= (EDMA3_CCRL_OPT_SYNCDIM_ABSYNC << EDMA3_CCRL_OPT_SYNCDIM_SHIFT);
            Cache_wb((Ptr)pParamSet->srcAddr,UTILS_DMA_BYTES_TO_WRITE,TRUE,NULL);

            UTILS_assert(
                Utils_dmaParamSetCheck(pParamSet) == FVID2_SOK
            );

            edma3Result = Utils_dmaTrigger(pObj);

            if (edma3Result == EDMA3_DRV_SOK)
            {
                if(pObj->enableIntCb)
                {
                    Semaphore_pend(pObj->semComplete, BIOS_WAIT_FOREVER);
                }
                else
                {
                    Utils_dmaPollWaitComplete(pObj);
                }
            }
        }

        /* goto next tranfer information */
        pInfo++;
    }

    Semaphore_post(pObj->semLock);

    return edma3Result;
}

Int32 Utils_dmaCopy2D(Utils_DmaChObj *pObj, Utils_DmaCopy2D *pInfo, UInt32 numTransfers)
{
    EDMA3_DRV_Result edma3Result = EDMA3_DRV_SOK;
    EDMA3_DRV_PaRAMRegs *pParamSet;
    UInt32 bpp; /* bytes per pixel */
    UInt32 i;

    if(pInfo->dataFormat==FVID2_DF_YUV420SP_UV)
    {
        /* UV and Y will be transferred separately */
        if(numTransfers>pObj->maxTransfers/2)
            return FVID2_EFAIL;
    }

    if(numTransfers>pObj->maxTransfers)
        return FVID2_EFAIL;

    Semaphore_pend(pObj->semLock, BIOS_WAIT_FOREVER);

    for(i=0; i<numTransfers; i++)
    {
        pObj->curTx = 0;
        pObj->numTx = 0;

        pParamSet = &pObj->pParamSet[pObj->numTx];

        pObj->numTx++;

        bpp = 2;
        if(pInfo->dataFormat==FVID2_DF_YUV420SP_UV)
            bpp = 1;

        pParamSet->destAddr   = (UInt32)pInfo->destAddr[0]
                              + pInfo->destPitch[0]*pInfo->destStartY
                              + pInfo->destStartX * bpp;

        pParamSet->srcAddr   = (UInt32)pInfo->srcAddr[0]
                              + pInfo->srcPitch[0]*pInfo->srcStartY
                              + pInfo->srcStartX * bpp;

        pParamSet->srcBIdx    = pInfo->srcPitch[0];
        pParamSet->destBIdx   = pInfo->destPitch[0];
        pParamSet->srcCIdx    = 0;
        pParamSet->destCIdx   = 0;

        pParamSet->aCnt       = (pInfo->width*bpp);
        pParamSet->bCnt       = pInfo->height;
        pParamSet->cCnt       = 1;

        pParamSet->bCntReload = pParamSet->bCnt;

        pParamSet->linkAddr   = 0xFFFFu;

        pParamSet->opt = 0;
        pParamSet->opt |= (EDMA3_CCRL_OPT_ITCCHEN_ENABLE << EDMA3_CCRL_OPT_ITCCHEN_SHIFT);
        pParamSet->opt |= (EDMA3_CCRL_OPT_TCINTEN_ENABLE << EDMA3_CCRL_OPT_TCINTEN_SHIFT);
        pParamSet->opt |= ((pObj->iTcc << EDMA3_CCRL_OPT_TCC_SHIFT) & EDMA3_CCRL_OPT_TCC_MASK);
        pParamSet->opt |= (EDMA3_CCRL_OPT_SYNCDIM_ABSYNC << EDMA3_CCRL_OPT_SYNCDIM_SHIFT);

        UTILS_assert(
            Utils_dmaParamSetCheck(pParamSet) == FVID2_SOK
        );

        edma3Result = Utils_dmaTrigger(pObj);

        if (edma3Result == EDMA3_DRV_SOK)
        {
            if(pObj->enableIntCb)
            {
                Semaphore_pend(pObj->semComplete, BIOS_WAIT_FOREVER);
            }
            else
            {
                Utils_dmaPollWaitComplete(pObj);
            }
        }

        if(pInfo->dataFormat==FVID2_DF_YUV420SP_UV)
        {
            pObj->curTx = 0;
            pObj->numTx = 0;

            /* setup PaRAM for UV plane */
            pParamSet = &pObj->pParamSet[pObj->numTx];

            pObj->numTx++;

            bpp = 1;

            /* for chroma plane when input or output is tiled we need to set pitch as -32KB
                since other wise pitch value will overflow the 16-bit register in EDMA.

                Hence all for all Chroma TX's we set pitch as -pitch and DMA from last line
                to first line
            */

            pParamSet->destAddr   = (UInt32)pInfo->destAddr[1]
                                  + pInfo->destPitch[1]*((pInfo->destStartY+pInfo->height)/2-1)
                                  + pInfo->destStartX * bpp;

            pParamSet->srcAddr   = (UInt32)pInfo->srcAddr[1]
                                  + pInfo->srcPitch[1]*((pInfo->srcStartY+pInfo->height)/2-1)
                                  + pInfo->srcStartX * bpp;

            pParamSet->srcBIdx    = -pInfo->srcPitch[1];
            pParamSet->destBIdx   = -pInfo->destPitch[1];
            pParamSet->srcCIdx    = 0;
            pParamSet->destCIdx   = 0;

            pParamSet->aCnt       = (pInfo->width*bpp);
            pParamSet->bCnt       = pInfo->height/2;
            pParamSet->cCnt       = 1;

            pParamSet->bCntReload = pParamSet->bCnt;

            pParamSet->linkAddr   = 0xFFFFu;

            pParamSet->opt = 0;
            pParamSet->opt |= (EDMA3_CCRL_OPT_ITCCHEN_ENABLE << EDMA3_CCRL_OPT_ITCCHEN_SHIFT);
            pParamSet->opt |= (EDMA3_CCRL_OPT_TCINTEN_ENABLE << EDMA3_CCRL_OPT_TCINTEN_SHIFT);
            pParamSet->opt |= ((pObj->iTcc << EDMA3_CCRL_OPT_TCC_SHIFT) & EDMA3_CCRL_OPT_TCC_MASK);
            pParamSet->opt |= (EDMA3_CCRL_OPT_SYNCDIM_ABSYNC << EDMA3_CCRL_OPT_SYNCDIM_SHIFT);

            UTILS_assert(
                Utils_dmaParamSetCheck(pParamSet) == FVID2_SOK
            );

            edma3Result = Utils_dmaTrigger(pObj);

            if (edma3Result == EDMA3_DRV_SOK)
            {
                if(pObj->enableIntCb)
                {
                    Semaphore_pend(pObj->semComplete, BIOS_WAIT_FOREVER);
                }
                else
                {
                    Utils_dmaPollWaitComplete(pObj);
                }
            }
        }

        /* goto next tranfer information */
        pInfo++;

    }

    Semaphore_post(pObj->semLock);

    return edma3Result;
}



