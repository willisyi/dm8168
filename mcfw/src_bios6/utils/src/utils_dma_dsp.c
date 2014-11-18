/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include "utils_dma_bios6.h"

Utils_DmaObj gUtils_dmaObj;

Int32 Utils_dmaOpen()
{
    EDMA3_RM_Result edma3Result = EDMA3_RM_SOK;

    memset(&gUtils_dmaObj, 0, sizeof(gUtils_dmaObj));

    gUtils_dmaObj.rmEdma = Utils_edma3Open(gUtils_dmaObj.edma3InstanceId, &edma3Result);
    if (!gUtils_dmaObj.rmEdma)
    {
        Vps_printf(
            " UTILS: DMA: Utils_dmaOpen() ... FAILED (%d) \n",
                edma3Result
            );
    }
    UTILS_assert(edma3Result == EDMA3_RM_SOK);

    if(edma3Result == EDMA3_RM_SOK)
        return  FVID2_SOK;
    else
        return  FVID2_EFAIL;
}

Int32 Utils_dmaClose()
{
    EDMA3_RM_Result edma3Result = EDMA3_RM_SOK;

    edma3Result = Utils_edma3Close(gUtils_dmaObj.edma3InstanceId, gUtils_dmaObj.rmEdma);
    if (edma3Result != EDMA3_RM_SOK)
    {
        Vps_printf(
            " UTILS: DMA: Utils_dmaClose() ... FAILED (%d) \n",
                edma3Result
            );
    }
    UTILS_assert(edma3Result == EDMA3_RM_SOK);
    if(edma3Result == EDMA3_RM_SOK)
        return  FVID2_SOK;
    else
        return  FVID2_EFAIL;
}


Int32 Utils_dmaCreateChDSP(Utils_DmaChObj *pObj, UInt32 eventQ, UInt32 maxTransfers, Bool enableIntCb)
{

    EDMA3_RM_Result edma3Result = EDMA3_RM_SOK;

    Semaphore_Params semParams;

    memset(pObj, 0, sizeof(*pObj));

    maxTransfers *= 2; // to account for UV data transfer

    if(maxTransfers<UTILS_DMA_MIN_TX)
        maxTransfers = UTILS_DMA_MIN_TX;

    pObj->curTx = 0;
    pObj->numTx = 0;
    pObj->maxTransfers = maxTransfers;

    pObj->iTcc     = EDMA3_RM_TCC_ANY;
    pObj->iChannel = EDMA3_RM_DMA_CHANNEL_ANY;
    pObj->iParam   = EDMA3_RM_PARAM_ANY;

    /* SemComplete is defined to use for interupt based DMA trabsfer complete Wait */
    /* Not used currently in DSP. See M3 side impletation for reference*/
    Semaphore_Params_init(&semParams);

    semParams.mode = Semaphore_Mode_BINARY;

    pObj->semComplete = Semaphore_create(0u, &semParams, NULL);
    UTILS_assert(pObj->semComplete != NULL);


    Semaphore_Params_init(&semParams);

    semParams.mode = Semaphore_Mode_BINARY;

    pObj->semLock = Semaphore_create(1u, &semParams, NULL);
    UTILS_assert(pObj->semLock != NULL);

    pObj->enableIntCb = enableIntCb;

    pObj->resObj.resId = pObj->iChannel;
    pObj->resObj.type  = EDMA3_RM_RES_DMA_CHANNEL;

    edma3Result = EDMA3_RM_allocLogicalChannel(gUtils_dmaObj.rmEdma,
                        &pObj->resObj,
                        &pObj->iParam,
                        &pObj->iTcc);

    pObj->iChannel = pObj->resObj.resId;                                      

    if (edma3Result != EDMA3_RM_SOK)
    {
        Vps_printf(" UTILS: DSP DMA Channle Alloc: EDMA3_RM_allocLogicalChannel call FAIL \n");
        Vps_printf(" UTILS: Return Code is: %d \n", (Int32) edma3Result);
        UTILS_assert(edma3Result == EDMA3_RM_SOK);
    }
    else
    {
        Vps_printf(" UTILS: DSP DMA Channle Alloc: EDMA3_RM_allocLogicalChannel call Successful \n");
        Vps_printf("Allocated Chanel %d, Param %d and TC is %d\n",pObj->iChannel, pObj->iParam, pObj->iTcc);
    }

    if (edma3Result == EDMA3_RM_SOK)
    {
        UInt32 memSize = sizeof(EDMA3_RM_PaRAMRegs)*pObj->maxTransfers;

        pObj->pRMParamSet = malloc(memSize);

        UTILS_assert(pObj->pRMParamSet != NULL);

        memset(pObj->pRMParamSet, 0, memSize);

    }

    if(edma3Result == EDMA3_RM_SOK)
        return  FVID2_SOK;
    else
        return  FVID2_EFAIL;

}

Int32 Utils_dmaDeleteChDSP(Utils_DmaChObj *pObj)
{
    EDMA3_RM_Result edma3Result = EDMA3_RM_SOK;

    edma3Result = EDMA3_RM_freeLogicalChannel(gUtils_dmaObj.rmEdma,&pObj->resObj);
    UTILS_assert(edma3Result == EDMA3_RM_SOK);

    free(pObj->pRMParamSet);
    pObj->pRMParamSet = NULL;

    Semaphore_delete(&pObj->semComplete);
    Semaphore_delete(&pObj->semLock);
    Vps_printf(" UTILS: DMA: Free'ed CH (TCC) = %d (%d)\n", pObj->iChannel, pObj->iTcc);

    if(edma3Result == EDMA3_RM_SOK)
        return  FVID2_SOK;
    else
        return  FVID2_EFAIL;
}

Int32 Utils_dmaPollWaitComplete(Utils_DmaChObj *pObj)
{
    uint16_t tccStatus;

    EDMA3_RM_waitAndClearTcc(gUtils_dmaObj.rmEdma,pObj->iTcc);
    EDMA3_RM_checkAndClearTcc(gUtils_dmaObj.rmEdma,pObj->iTcc, &tccStatus);

    return 0;
}

Int32 Utils_dmaTrigger(Utils_DmaChObj *pObj)
{
    EDMA3_RM_Result edma3Result = EDMA3_RM_SOK;
    uint16_t tccStatus;
    EDMA3_RM_Instance * rmInst;
    UInt32 lCh;
        
    rmInst = (EDMA3_RM_Instance *) gUtils_dmaObj.rmEdma;
    lCh = pObj->iChannel;

    edma3Result |= EDMA3_RM_checkAndClearTcc(gUtils_dmaObj.rmEdma,pObj->iTcc, &tccStatus);

    edma3Result = EDMA3_RM_setPaRAM(
                    gUtils_dmaObj.rmEdma,
                    &pObj->resObj,
                    &pObj->pRMParamSet[pObj->curTx]
                    );


    if (edma3Result == EDMA3_RM_SOK)
    {
        if (lCh < 32u)
        {
            rmInst->shadowRegs->ESR  = (1UL << lCh);
        }
        else
        {
            rmInst->shadowRegs->ESRH  = (1UL << (lCh-32u));
        }
    }

    if (edma3Result != EDMA3_RM_SOK)
    {
        Vps_printf(
            " UTILS: DMA: %d: Utils_dmaTrigger() ... FAILED (%d) \n",
                pObj->curTx, edma3Result
            );
    }

    return edma3Result;
}

Int32 Utils_dmaCopy2D(Utils_DmaChObj *pObj, Utils_DmaCopy2D *pInfo, UInt32 numTransfers)
{
    EDMA3_RM_Result edma3Result = EDMA3_RM_SOK;
    EDMA3_RM_PaRAMRegs *pParamSet;
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

        pParamSet = &pObj->pRMParamSet[pObj->numTx];

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

        edma3Result = Utils_dmaTrigger(pObj);
 
        /* Polling based EDMA transfer complete wait */
        Utils_dmaPollWaitComplete(pObj);

        if(pInfo->dataFormat==FVID2_DF_YUV420SP_UV)
        {
            pObj->curTx = 0;
            pObj->numTx = 0;

            /* setup PaRAM for UV plane */
            pParamSet = &pObj->pRMParamSet[pObj->numTx];

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
        

            edma3Result = Utils_dmaTrigger(pObj);

            Utils_dmaPollWaitComplete(pObj);

            /* goto next tranfer information */
            pInfo++;

        }
    }    

    Semaphore_post(pObj->semLock);

    return edma3Result;
}



