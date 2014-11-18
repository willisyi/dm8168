/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#define ti_sysbios_family_c64p_Cache__nolocalnames
#define ti_sysbios_hal_Cache__nolocalnames
#include "scdLink_priv.h"
#include "scd_ti.h"
#include "ti/sdo/fc/dskt2/dskt2.h"
#include "ti/sdo/fc/rman/rman.h"
#include <mcfw/interfaces/common_def/ti_vsys_common_def.h>
#include <mcfw/src_bios6/utils/utils_mem.h>
#include <ti/sysbios/family/c64p/Cache.h>

static UInt8 gScratchId = 1;
static Int32 AlgLink_ScdalgSetChScdPrm(AlgLink_ScdchPrm    *pScdChPrm,
                            AlgLink_ScdChParams * params);

#define UTILS_SCD_OUTBUF_SIZE()   (sizeof(AlgLink_ScdResult))
#define ALG_LINK_SCD_MAX_DMACOPY_FRAMES_PER_CH       (2)

HPTimeStamp AlgLink_ScdGetFrameTS(AlgLink_ScdChObj * pAlgObj,UInt32 timeStampMsec)
{    
#if 1
    UInt32 interFrameInterval;
 
    interFrameInterval = (1000000/pAlgObj->frameSkipCtx.outputFrameRate);
 
    pAlgObj->frameTS.clock.tv_usec += interFrameInterval;
 
    if(pAlgObj->frameTS.clock.tv_usec >= (interFrameInterval * pAlgObj->frameSkipCtx.outputFrameRate))
    {
       pAlgObj->frameTS.clock.tv_sec ++;
       pAlgObj->frameTS.clock.tv_usec = 0;
    } 
#else  
    pAlgObj->frameTS.clock.tv_sec  = (timeStampMsec/1000);
    pAlgObj->frameTS.clock.tv_usec = (timeStampMsec % 1000) * 1000;
#endif 
   
    return (pAlgObj->frameTS);
} 


Int32 AlgLink_ScdVAInitParams(AlgLink_ScdchPrm *pChPrm) //(AlgLink_ScdObj * pObj)
{
    pChPrm->imgType       = DMVAL_IMG_LUMA;
    pChPrm->detectMode    = DMVAL_DETECTMODE_TAMPER;

    pChPrm->sensitiveness = DMVAL_SENSITIVITY_LOW;

#ifdef SYSTEM_DEBUG_SCD
    Vps_printf(" %d: SCD   : VA ALG Init Done of Channel - %d!!!\n", Utils_getCurTimeInMsec(),pChPrm->chId);
#endif 

      
    return FVID2_SOK;
}


static Int32 AlgLink_ScdCreateOutObj(AlgLink_ScdObj * pObj)
{
    AlgLink_ScdOutObj *pOutObj;

    Int32 status;
    UInt32 bufIdx;
    Int i,j,queueId;
    UInt32 totalBufCnt;

    for(queueId = 0; queueId < ALG_LINK_MAX_OUT_QUE; queueId++)
    {

        pOutObj = &pObj->outObj[queueId];

        pObj->outObj[queueId].numAllocPools = 1;

        pOutObj->buf_size[0] = UTILS_SCD_OUTBUF_SIZE();
        pOutObj->buf_size[0] = 
          VpsUtils_align(pOutObj->buf_size[0], 
                         SharedRegion_getCacheLineSize(SYSTEM_IPC_SR_CACHED));

        status = Utils_bitbufCreate(&pOutObj->bufOutQue, TRUE, FALSE,
                                    pObj->outObj[queueId].numAllocPools);
        UTILS_assert(status == FVID2_SOK);

        totalBufCnt = 0;
        for (i = 0; i < pOutObj->numAllocPools; i++)
        {
            pOutObj->outNumBufs[i] = (pObj->createArgs.numValidChForSCD * pObj->createArgs.numBufPerCh);

            for (j = 0; j < pObj->createArgs.numValidChForSCD; j++)
            {
                pOutObj->ch2poolMap[j] =  i;
            }

            status = Utils_memBitBufAlloc(&(pOutObj->outBufs[totalBufCnt]),
                                          pOutObj->buf_size[i],
                                          pOutObj->outNumBufs[i]);
            UTILS_assert(status == FVID2_SOK);

            for (bufIdx = 0; bufIdx < pOutObj->outNumBufs[i]; bufIdx++)
            {
                UTILS_assert((bufIdx + totalBufCnt) < ALG_LINK_SCD_MAX_OUT_FRAMES);
                pOutObj->outBufs[bufIdx + totalBufCnt].allocPoolID = i;
                pOutObj->outBufs[bufIdx + totalBufCnt].doNotDisplay =
                    FALSE;
                status =
                    Utils_bitbufPutEmptyBuf(&pOutObj->bufOutQue,
                                            &pOutObj->outBufs[bufIdx +
                                                              totalBufCnt]);
                UTILS_assert(status == FVID2_SOK);
            }
            /* align size to minimum required frame buffer alignment */
            totalBufCnt += pOutObj->outNumBufs[i];
        }
    }

    return (status);
}

Int32 AlgLink_ScdAlgChCreate(AlgLink_ScdObj * pObj)
{
    Int32               status, scdChId;
    AlgLink_ScdChParams *pChLinkPrm;
    AlgLink_ScdchPrm    *pChPrm;
    AlgLink_ScdChObj    *pChObj;
    FVID2_Frame         *pFrame;
    UInt32               frameId;


    for(scdChId = 0;  scdChId<pObj->createArgs.numValidChForSCD; scdChId++)
    {
        pChPrm     = &pObj->chParams[scdChId];
        pChLinkPrm = &pObj->createArgs.chDefaultParams[scdChId];
        pChObj     = &pObj->chObj[scdChId];

        pChObj->frameSkipCtx.firstTime       = TRUE;
        pChObj->frameSkipCtx.inputFrameRate  = pObj->createArgs.inputFrameRate;
        pChObj->frameSkipCtx.outputFrameRate = pObj->createArgs.outputFrameRate;
        pChObj->scdChStat                    = ALG_LINK_SCD_DETECTOR_UNAVAILABLE;
        pChObj->chId                         = pChLinkPrm->chId;

        status = AlgLink_ScdalgSetChScdPrm(pChPrm, pChLinkPrm);

        if(pObj->createArgs.thresold2WaitAfterFrmAlert > ALG_LINK_TAMPER_ALERT_MAX_THRESOLD)
            return FVID2_EFAIL;
        if(pObj->createArgs.thresold2WaitB4FrmAlert > ALG_LINK_TAMPER_ALERT_MAX_THRESOLD)
            return FVID2_EFAIL;
         
        pChPrm->thresold2WaitAfterFrmAlert   = pObj->createArgs.thresold2WaitAfterFrmAlert;
        pChPrm->thresold2WaitB4FrmAlert      = pObj->createArgs.thresold2WaitB4FrmAlert;

        pChPrm->numSecs2Wait2MarkStableFrame = pObj->createArgs.numSecs2Wait2MarkStableFrame;


        UTILS_assert(status==0);

        pChPrm->width  = pObj->inQueInfo->chInfo[pChPrm->chId].width +
                                pObj->inQueInfo->chInfo[pChPrm->chId].startX;
        pChPrm->height = pObj->inQueInfo->chInfo[pChPrm->chId].height +
                                pObj->inQueInfo->chInfo[pChPrm->chId].startY;
        pChPrm->stride = pObj->inQueInfo->chInfo[pChPrm->chId].pitch[0];

#ifdef SYSTEM_DEBUG_SCD
        Vps_printf(" %d: SCD    : Chan ID %d: Resolution %d x %d, In FPS = %d, Out FPS = %d!!!\n",
                   Utils_getCurTimeInMsec(),
                    pChPrm->chId,
                    pChPrm->width,
                    pChPrm->height,
                    pChObj->frameSkipCtx.inputFrameRate,
                    pChObj->frameSkipCtx.outputFrameRate
            );
#endif

         status = Utils_queCreate(&pChObj->scdProcessObj.freeQ,
                                  pObj->createArgs.numBufPerCh,
                                  pChObj->scdProcessObj.freeQMem,
                                  UTILS_QUE_FLAG_NO_BLOCK_QUE
                                     );

         UTILS_assert(status==FVID2_SOK);

         pChObj->scdProcessObj.processFrameSize = pChPrm->stride * pChPrm->height;

         pChObj->scdProcessObj.processFrameSize = \
                                        VpsUtils_align(pChObj->scdProcessObj.processFrameSize, 
                                        SharedRegion_getCacheLineSize(SYSTEM_IPC_SR_CACHED));

         /* alloc channel process buffer memory */

         for(frameId=0; frameId<ALG_LINK_SCD_MAX_DMACOPY_FRAMES_PER_CH; frameId++)
         {
             pFrame = &pChObj->scdProcessObj.processFrames[frameId];
             pFrame->appData = &pChObj->scdProcessObj.processFrameInfo[frameId];
             pFrame->addr[0][0] =
                     Utils_memAlloc(
                         pChObj->scdProcessObj.processFrameSize,
                         SharedRegion_getCacheLineSize(SYSTEM_IPC_SR_CACHED)
                     );

             UTILS_assert(pFrame->addr[0][0]!=NULL);

             status = Utils_quePut(&pChObj->scdProcessObj.freeQ, pFrame, BIOS_NO_WAIT);

             UTILS_assert(status==FVID2_SOK);
         }

    }
    return 0;
}

Int32 AlgLink_ScdVACreate(AlgLink_ScdObj * pObj)
{
    AlgLink_ScdchPrm    *pChPrm; 
    AlgLink_ScdChObj    *pChObj;
    UInt32 status;
    DMVALstatus libStatus;
    DMVALhandle *pDmvaHndl;
    Int32       scdChId;
    UInt32      sensitivity, numFrames2Wait2MarkStableFrame;
    UInt32      cacheMarbit;

    for(scdChId = 0;  scdChId<pObj->createArgs.numValidChForSCD; scdChId++)
    {
        pChObj    = &pObj->chObj[scdChId];
        pChPrm    = &pObj->chParams[scdChId];
        pDmvaHndl = &pObj->dmvalHndl[scdChId];

        pChObj->frameTS.clock.tv_sec  = 0;
        pChObj->frameTS.clock.tv_usec = 0; 

        status = AlgLink_ScdVAInitParams(pChPrm);
        UTILS_assert(status == FVID2_SOK);

        sensitivity = pObj->createArgs.chDefaultParams[scdChId].frmSensitivity;

        if(sensitivity > DMVAL_SENSITIVITY_HIGH)
            pChPrm->sensitiveness = DMVAL_SENSITIVITY_HIGH;
        else if(sensitivity < DMVAL_SENSITIVITY_LOW)
           pChPrm->sensitiveness = DMVAL_SENSITIVITY_LOW;
        else
           pChPrm->sensitiveness = (DMVALsensitivity) sensitivity;

        pChPrm->detectMode = (DMVALdetectMode) (pObj->createArgs.chDefaultParams[scdChId].mode & 0x1);

        pChPrm->resetCounter  = ALG_LINK_TAMPER_RESET_COUNT_SCALER_CONSTANT * pChObj->frameSkipCtx.outputFrameRate; 

        memset(pDmvaHndl,0x00,sizeof(DMVALhandle));
        pDmvaHndl->output = &pChObj->output;

        libStatus = DMVAL_create(pDmvaHndl,            // handle
                     pChPrm->detectMode,     // detectMode
                     pChPrm->imgType,        // imgType
                     pChPrm->sensitiveness,    // sensitivity
                     pChPrm->width,        // inputFrmWidth
                     pChPrm->height);        // inputFrmHeight

        if(libStatus != DMVAL_NO_ERROR)
        {
          Vps_printf(" %d: SCD   : VA Create Call Status 0x%X!!!\n", 
                  Utils_getCurTimeInMsec(),libStatus);
        }
        UTILS_assert(libStatus == DMVAL_NO_ERROR);   

           /* ALG mem alloc */
        pChObj->sizeA = VpsUtils_align(pDmvaHndl->bytesMemBufPermanent, 
                                        SharedRegion_getCacheLineSize(SYSTEM_IPC_SR_CACHED));
                                        
        pChObj->ptrA  = Utils_memAlloc(pChObj->sizeA, SharedRegion_getCacheLineSize(SYSTEM_IPC_SR_CACHED));
        UTILS_assert(pChObj->ptrA != NULL);

       cacheMarbit = ti_sysbios_family_c64p_Cache_getMar(pChObj->ptrA);
#ifdef SYSTEM_DEBUG_SCD
       Vps_printf(" %d: SCD   : VA MEMALLOC %d bytes for DMVAL internal memory at addr 0x%X!!!\n", 
                       Utils_getCurTimeInMsec(),pChObj->sizeA,pChObj->ptrA);

       if(cacheMarbit != ti_sysbios_family_c64p_Cache_Mar_ENABLE)
       {
          Vps_printf(" %d: SCD   : VA Internal memory allocated at Non-Cached!!!\n",  \
                       Utils_getCurTimeInMsec());
       }
#endif

       UTILS_assert(cacheMarbit == ti_sysbios_family_c64p_Cache_Mar_ENABLE);

       pChObj->sizeB = VpsUtils_align(pDmvaHndl->bytesMemBufOutput, 
                                        SharedRegion_getCacheLineSize(SYSTEM_IPC_SR_CACHED));

       pChObj->ptrB  = Utils_memAlloc(pChObj->sizeB, SharedRegion_getCacheLineSize(SYSTEM_IPC_SR_CACHED));

       UTILS_assert(pChObj->ptrB != NULL);
       cacheMarbit = ti_sysbios_family_c64p_Cache_getMar(pChObj->ptrB);
#ifdef SYSTEM_DEBUG_SCD
       Vps_printf(" %d: SCD   : VA MEMALLOC %d bytes for DMVAL output memory at addr 0x%X!!!\n", 
                       Utils_getCurTimeInMsec(),pChObj->sizeB,pChObj->ptrB);

       if(cacheMarbit != ti_sysbios_family_c64p_Cache_Mar_ENABLE)
       {
          Vps_printf(" %d: SCD   : VA Internal memory allocated at Non-Cached!!!\n",  \
                       Utils_getCurTimeInMsec());
       }
#endif

       UTILS_assert(cacheMarbit == ti_sysbios_family_c64p_Cache_Mar_ENABLE); 

       libStatus = DMVAL_configure(pDmvaHndl,pChObj->ptrA,pChObj->ptrB);
       UTILS_assert(libStatus == DMVAL_NO_ERROR);  

       libStatus = DMVAL_initModule(pDmvaHndl);
       UTILS_assert(libStatus == DMVAL_NO_ERROR);  

       DMVAL_setParameter(
                          pDmvaHndl,DMVAL_PARAM_SENSITIVITY,
                          (Int16*)&(pChPrm->sensitiveness),
                          1);

       DMVAL_setParameter(
                          pDmvaHndl,DMVAL_PARAM_TAMPERCOUNTER,
                          (Int16*)&(pChPrm->resetCounter),
                          1);

       DMVAL_setParameter(
                          pDmvaHndl,DMVAL_PARAM_TAMPER_BLOCKUP_TH, 
                          (Int16*)&(pChPrm->thresold2WaitB4FrmAlert),
                          1);

       DMVAL_setParameter(
                          pDmvaHndl,DMVAL_PARAM_TAMPER_BLOCKDOWN_TH, 
                          (Int16*)&(pChPrm->thresold2WaitAfterFrmAlert),
                          1);
       numFrames2Wait2MarkStableFrame = pChPrm->numSecs2Wait2MarkStableFrame * pChObj->frameSkipCtx.outputFrameRate;
       DMVAL_setParameter(
                          pDmvaHndl,DMVAL_PARAM_TAMPER_BACKGROUND_FRM_INTERVAL, 
                          (Int16*)&numFrames2Wait2MarkStableFrame, 
                          1);

#ifdef SYSTEM_DEBUG_SCD
       Vps_printf(" %d: SCD   : VA ALG Create Done of Channel - %d!!!\n", Utils_getCurTimeInMsec(),pChObj->chId);
#endif
    }
    return FVID2_SOK;
} 

Int32 AlgLink_ScdLMDCreate(AlgLink_ScdObj * pObj)
{

    Int32               chId;
    SCD_createPrm       algCreatePrm;
    SCD_chPrm           chDefaultParams[ALG_LINK_SCD_MAX_CH];
    IALG_Fxns           *algFxns = (IALG_Fxns *)&SCD_TI;

    algCreatePrm.maxWidth    = pObj->createArgs.maxWidth;
    algCreatePrm.maxHeight   = pObj->createArgs.maxHeight;
    algCreatePrm.maxStride   = pObj->createArgs.maxStride;
    algCreatePrm.maxChannels = pObj->createArgs.numValidChForSCD;
    algCreatePrm.numSecs2WaitB4Init        = pObj->createArgs.numSecs2WaitB4Init;
    algCreatePrm.numSecs2WaitB4FrmAlert    = pObj->createArgs.numSecs2WaitB4FrmAlert;
    algCreatePrm.numSecs2WaitAfterFrmAlert = pObj->createArgs.numSecs2WaitAfterFrmAlert;
    algCreatePrm.fps                       = (SCD_Fps) pObj->createArgs.outputFrameRate;
    algCreatePrm.chDefaultParams           = (SCD_chPrm *)&chDefaultParams[0];

    for(chId=0; chId < algCreatePrm.maxChannels; chId++)
    {
        SCD_chPrm      * chl = &(algCreatePrm.chDefaultParams[chId]);
        AlgLink_ScdChParams  * chCreatePrm = &(pObj->createArgs.chDefaultParams[chId]);

        if ((chCreatePrm->mode == SCD_DETECTMODE_MONITOR_BLOCKS) ||
             (chCreatePrm->mode == SCD_DETECTMODE_MONITOR_BLOCKS_FRAME))
        {
          chl->blkConfig = (SCD_blkChngConfig *)chCreatePrm->blkConfig;
        }
        else
        {
          chl->blkConfig = NULL;
        }

        // The remaining parameter values filled in here do not really matter as
        // they will be over-written by calls to SCD_TI_setPrms. We'll fill in
        // just a few
        chl->chId    = chCreatePrm->chId;
        chl->mode     = (SCD_Mode)(chCreatePrm->mode & 0x2);
        chl->width    = pObj->createArgs.maxWidth;
        chl->height   = pObj->createArgs.maxHeight;
        chl->stride   = pObj->createArgs.maxStride;
        chl->curFrame = NULL;

        chl->frmSensitivity     = (SCD_Sensitivity)chCreatePrm->frmSensitivity;
        chl->frmIgnoreLightsON  = chCreatePrm->frmIgnoreLightsON;
        chl->frmIgnoreLightsOFF = chCreatePrm->frmIgnoreLightsOFF;
        chl->frmEdgeThreshold   = chCreatePrm->frmEdgeThreshold;
        
        if( pObj->inQueInfo->chInfo[chId].dataFormat != FVID2_DF_YUV420SP_UV)
        {
#ifdef SYSTEM_DEBUG_SCD
           Vps_printf(" %d: SCD    : Channel No. %d Create ERROR - Input Format Not Supported !!!\n",
                 Utils_getCurTimeInMsec(), chCreatePrm->chId);
#endif
             return FVID2_EFAIL;
        }

    }

    /* Create algorithm instance and get algo handle  */
    pObj->algHndl = DSKT2_createAlg((Int)gScratchId,
            (IALG_Fxns *)algFxns, NULL,(IALG_Params *)&algCreatePrm);

    if(pObj->algHndl == NULL)
    {
#ifdef SYSTEM_DEBUG_SCD
        Vps_printf(" %d: SCD    : Create ERROR !!!\n",
               Utils_getCurTimeInMsec());
#endif

        return FVID2_EFAIL;
    }

    return FVID2_SOK;
}


Int32 AlgLink_ScdalgCreate(AlgLink_ScdObj * pObj)
{
    Int32       status;

#ifdef SYSTEM_DEBUG_SCD
    Vps_printf(" %d: SCD    : Create in progress !!!\n", Utils_getCurTimeInMsec());
#endif

    pObj->totalFrameCount = 0;

    if(pObj->createArgs.numBufPerCh == 0)
        pObj->createArgs.numBufPerCh = ALG_LINK_SCD_MAX_OUT_FRAMES_PER_CH;

    if(pObj->createArgs.numBufPerCh > ALG_LINK_SCD_MAX_OUT_FRAMES_PER_CH)
    {
        Vps_printf("\n SCDLINK: WARNING: User is asking for %d buffers per CH. But max allowed is %d. \n"
            " Over riding user requested with max allowed \n\n",
            pObj->createArgs.numBufPerCh, ALG_LINK_SCD_MAX_OUT_FRAMES_PER_CH
            );

        pObj->createArgs.numBufPerCh = ALG_LINK_SCD_MAX_OUT_FRAMES_PER_CH;

    }

#ifdef SYSTEM_DEBUG_SCD
    Vps_printf(" %d: SCD    : Max WxH = %d x %d, Max CH = %d, FPS = %d !!!\n",
           Utils_getCurTimeInMsec(),
            pObj->createArgs.maxWidth,
            pObj->createArgs.maxHeight,
            pObj->createArgs.numValidChForSCD,
            pObj->createArgs.outputFrameRate
        );
#endif

    if( pObj->createArgs.numValidChForSCD > pObj->inQueInfo->numCh)
    {
#ifdef SYSTEM_DEBUG_SCD
        Vps_printf(" %d: SCD    : Create ERROR - SCD channels < InQueue Channels !!!\n",
               Utils_getCurTimeInMsec());
#endif

        return FVID2_EFAIL;
    }

    status = AlgLink_ScdLMDCreate(pObj);
    UTILS_assert(status == FVID2_SOK);


    AlgLink_ScdAlgChCreate(pObj);

    status = AlgLink_ScdVACreate(pObj);
    UTILS_assert(status == FVID2_SOK);
 
    AlgLink_ScdCreateOutObj(pObj);

    AlgLink_ScdresetStatistics(pObj);

    status = Utils_dmaCreateChDSP(&pObj->dmaCh, UTILS_DMA_DEFAULT_EVENT_Q, 1, FALSE);

    UTILS_assert(status==FVID2_SOK);

    status = Utils_queCreate(&pObj->processQ,
                             pObj->createArgs.numBufPerCh * pObj->createArgs.numValidChForSCD,
                             pObj->processQMem,
                             UTILS_QUE_FLAG_BLOCK_QUE_GET
                            );
    UTILS_assert(status==FVID2_SOK);

    AlgLink_scdAlgProcessTskSendCmd(pObj, SYSTEM_CMD_START);

#ifdef SYSTEM_DEBUG_SCD
    Vps_printf(" %d: SCD    : Create Done !!!\n",
               Utils_getCurTimeInMsec());
#endif

 return FVID2_SOK;
}

Int32 AlgLink_ScdAlgChDelete(AlgLink_ScdObj * pObj)
{
    AlgLink_ScdChObj *pChObj;
    FVID2_Frame *pFrame;
    UInt32 frameId;
    UInt32 chId;
    Int32 status;

    for(chId=0; chId<pObj->createArgs.numValidChForSCD; chId++)
    {
        pChObj = &pObj->chObj[chId];

        status = Utils_queDelete(&pChObj->scdProcessObj.freeQ);

        UTILS_assert(status==FVID2_SOK);

        /* free chann process buffer memory */

        for(frameId=0; frameId<ALG_LINK_SCD_MAX_DMACOPY_FRAMES_PER_CH; frameId++)
        {
            pFrame = &pChObj->scdProcessObj.processFrames[frameId];

            status = Utils_memFree(
                        pFrame->addr[0][0], pChObj->scdProcessObj.processFrameSize
                            );

            UTILS_assert(status==FVID2_SOK);
        }
    }

    return FVID2_SOK;
}

 
Int32 AlgLink_ScdVADelete(AlgLink_ScdObj * pObj)
{
    AlgLink_ScdChObj *pChObj;
    DMVALhandle *pDmvaHndl;
    UInt32 chId;

    /* Unless process task is stopped, don't free up memory 
     * It would avoid Alg call with junk data as this task and process task are 
     * independent.*/
    while(pObj->processTskState != ALG_LINK_PROCESS_TASK_STATE_STOPPED);

    for(chId=0; chId<pObj->createArgs.numValidChForSCD; chId++)
    {
        pChObj = &pObj->chObj[chId];

        pDmvaHndl = &pObj->dmvalHndl[chId];

        /* Free ALG mem */
        Utils_memFree(pChObj->ptrA,pChObj->sizeA);
        Utils_memFree(pChObj->ptrB,pChObj->sizeB);

        DMVAL_delete(pDmvaHndl);

    }
    return FVID2_SOK;
}

Int32 AlgLink_ScdLMDDelete(AlgLink_ScdObj * pObj)
{
    Int32 scratchId = gScratchId;
    UInt32 outId, status;

    if(pObj->algHndl == NULL)
        return FVID2_EFAIL;

    /* Deactivate algorithm */
//    DSKT2_deactivateAlg(scratchId, (IALG_Handle)pObj->algHndl);

    DSKT2_freeAlg(scratchId, (IALG_Handle)pObj->algHndl);

    for (outId = 0; outId < ALG_LINK_MAX_OUT_QUE; outId++)
    {
        Int32 i,bitbuf_index;
        AlgLink_ScdOutObj *pOutObj;

        pOutObj = &pObj->outObj[outId];

        status = Utils_bitbufDelete(&pOutObj->bufOutQue);
        UTILS_assert(status == FVID2_SOK);

        bitbuf_index = 0;
        for (i = 0; i < pOutObj->numAllocPools; i++)
        {
            UTILS_assert((pOutObj->outBufs[bitbuf_index].bufSize ==
                          pOutObj->buf_size[i]));
            status = Utils_memBitBufFree(&pOutObj->outBufs[bitbuf_index],
                                pOutObj->outNumBufs[i]);
            UTILS_assert(status == FVID2_SOK);
            bitbuf_index += pOutObj->outNumBufs[i];
        }
    }
   UTILS_assert(status==FVID2_SOK);

   return FVID2_SOK;
}

Int32 AlgLink_ScdalgDelete(AlgLink_ScdObj * pObj)
{
    Int32 status = FVID2_SOK;

#ifdef SYSTEM_DEBUG_SCD
    Vps_printf(" %d: SCD    : Delete in progress !!!\n",
               Utils_getCurTimeInMsec());
#endif

    AlgLink_scdAlgProcessTskSendCmd(pObj, SYSTEM_CMD_STOP);

    status = Utils_dmaDeleteChDSP(&pObj->dmaCh);
    UTILS_assert(status==FVID2_SOK);

    /* Delete SCD VA Handle and Free VA memory*/
    status = AlgLink_ScdVADelete(pObj);
    UTILS_assert(status==FVID2_SOK);

    /* Deleting process queue after process task delete as it might be in use */
    status = Utils_queDelete(&pObj->processQ);
    UTILS_assert(status==FVID2_SOK);

    /* Delete SCD LMD Handle and Free Out Queue buffer used by LMD */
    status = AlgLink_ScdLMDDelete(pObj);
    UTILS_assert(status==FVID2_SOK);

    /* Delete SCD Channels Object and Release memory held by FreeQ etc.*/
    status = AlgLink_ScdAlgChDelete(pObj);
    UTILS_assert(status==FVID2_SOK);

#ifdef SYSTEM_DEBUG_SCD
    Vps_printf(" %d: SCD    : Delete Done !!!\n",
               Utils_getCurTimeInMsec());
#endif

    return status;
}

Int32 AlgLink_ScdalgProcess(AlgLink_ScdObj *pScdAlgLinkObj, UInt32 chId, AlgLink_ScdResult * scdResultBuff)
{
    SCD_Status      chanStatus;
    SCD_Result      scdResult;
    UInt32          chanID;
    SCD_chPrm       chanParam;
    AlgLink_ScdchPrm * chPrm;
    UInt32         blkIdx;

    /* Activate the Algorithm */
//    DSKT2_activateAlg(gScratchId, (IALG_Handle)pScdAlgLinkObj->algHndl);

    chanID = pScdAlgLinkObj->chParams[chId].chId;
    chPrm = &(pScdAlgLinkObj->chParams[chId]);

    chanParam.chId                 = chPrm->chId;
    chanParam.mode                 = (SCD_Mode)(chPrm->mode & 0x2);
    chanParam.width                = chPrm->width;
    chanParam.height               = chPrm->height;
    chanParam.stride               = chPrm->stride;
    chanParam.curFrame             = chPrm->curFrame;
    chanParam.frmSensitivity       = (SCD_Sensitivity)chPrm->frmSensitivity;
    chanParam.frmIgnoreLightsON    = chPrm->frmIgnoreLightsON;
    chanParam.frmIgnoreLightsOFF   = chPrm->frmIgnoreLightsOFF;
    chanParam.frmEdgeThreshold     = chPrm->frmEdgeThreshold;

    if(chPrm->chBlkConfigUpdate == TRUE)
    {
        chanParam.blkConfig = (SCD_blkChngConfig *) chPrm->blkConfig;
    }
    else
    {
        chanParam.blkConfig = NULL;
    }

    chanStatus = SCD_TI_setPrms(pScdAlgLinkObj->algHndl, &chanParam, chanID);

    if(chanStatus != SCD_NO_ERROR)
    {
#ifdef SYSTEM_DEBUG_SCD
        Vps_printf(" %d: SCD    : ERROR: Alg Set Params (chanID = %d) - 0x%08X !!!\n",
                  Utils_getCurTimeInMsec(), chanID, chanStatus);
#endif
        /* Deactivate algorithm */
//        DSKT2_deactivateAlg(gScratchId, (IALG_Handle)pScdAlgLinkObj->algHndl);
        return FVID2_EFAIL;
    }


    scdResult.frmResult = SCD_DETECTOR_NO_TAMPER;
    scdResult.blkResult = (SCD_blkChngMeta *)(scdResultBuff->blkResult);

    chanStatus = SCD_TI_process(pScdAlgLinkObj->algHndl, chanID, &scdResult);


    /* Deactivate algorithm */
//    DSKT2_deactivateAlg(gScratchId, (IALG_Handle)pScdAlgLinkObj->algHndl);

    if(chanStatus != SCD_NO_ERROR)
    {
#ifdef SYSTEM_DEBUG_SCD
        Vps_printf(" %d: SCD    : ERROR: Alg Process (chanID = %d) !!!\n",
                  Utils_getCurTimeInMsec(), chanID );
#endif

        return FVID2_EFAIL;
    }

    scdResultBuff->frmResult = (AlgLink_ScdOutput) scdResult.frmResult;
    scdResultBuff->chId = chanID;


    /* Motion detect event notification. */
     UInt32 numBlkChg;
     UInt32 monitoredBlk;
     UInt32 numHorzBlks, numVertBlks, numBlksInFrame, blkHeight;

     monitoredBlk = 0;
     numBlkChg = 0;
     numHorzBlks     = ((pScdAlgLinkObj->chParams[chId].width + 0x1F ) & (~0x1F)) / 32;  /* Rounding to make divisible by 32 */
     if((pScdAlgLinkObj->chParams[chId].height%ALG_LINK_SCD_BLK_HEIGHT_MIN) == 0)        /* For Block height is divisible by 10 */
        blkHeight = ALG_LINK_SCD_BLK_HEIGHT_MIN;
     else   /* For Block height is divisible by 12 */
        blkHeight = ALG_LINK_SCD_BLK_HEIGHT;
     numVertBlks    = pScdAlgLinkObj->chParams[chId].height / blkHeight;

     numBlksInFrame = numHorzBlks * numVertBlks;

     /* Logic  to see how many blocks of total enabled blocks experienced change.
      * For each block, algorithm returns no. of pixels changed in the current
      * frame. This is compared against thresold determined using SCD sensitivity .
      * if changed pixels are more than the calculated thresold, block is marked as changed
      * i.e. motion is detected in the block.
      * Minimum value of thresold is 20% and then it is incrmented by 10% for
      * each sensitivity level change. Thresold can vary from 20% - 100%
      * At max sensitivity, thresold would be 20%. That means if 20% pixels
      * are changed block is marked as changed.
      * At minimu sensitivity, thresold would be 100%. That means if 100% pixels
      * are changed block is marked as changed */

     for(blkIdx = 0; blkIdx < numBlksInFrame; blkIdx++)
     {
         SCD_blkChngConfig * blockConfig;

         blockConfig = &pScdAlgLinkObj->chParams[chId].blkConfig[blkIdx];
         scdResultBuff->blkConfig[blkIdx].monitored   = blockConfig->monitored;
         scdResultBuff->blkConfig[blkIdx].sensitivity = blockConfig->sensitivity;
         if(blockConfig->monitored == 1)
         {
             UInt32 threshold;

             monitoredBlk++;
             threshold = MOTION_DETECTION_SENSITIVITY(ALG_LINK_SCD_BLK_WIDTH, blkHeight) +
                               (MOTION_DETECTION_SENSITIVITY_STEP * (SCD_SENSITIVITY_MAX - blockConfig->sensitivity));

             if(scdResultBuff->blkResult[blkIdx].numPixelsChanged > threshold)
             {
                 numBlkChg++;
             }
         }
 
         /* If Motion detection mode ( **_MONITOR_BLOCK) is disabled of block is 
          * not monitor then set the values in the Result buff to zero to avoid 
          * false trigger/alarm. */

         if(((pScdAlgLinkObj->chParams[chId].mode & SCD_DETECTMODE_MONITOR_BLOCKS) == FALSE) || 
              (blockConfig->monitored == 0))
         {
              scdResultBuff->blkResult[blkIdx].numPixelsChanged  = 0;
              scdResultBuff->blkResult[blkIdx].numFrmsBlkChanged = 0;
         }
     }

     /* Logic  to notify A8-host about motion detection.
      * Currently, if atleast 1 block is detected as changed A8 is notified.
      * User can use commented logic to chnage this behavior. 
      * if((monitoredBlk > 0) && (numBlkChg > (NUM_BLOCK_MOTION_DETECTION_THRESOLD(monitoredBlk)))) 
      */

    if(pScdAlgLinkObj->createArgs.enableMotionNotify)
    {
        if((monitoredBlk > 0) && (numBlkChg > 0))
        {
#ifdef SYSTEM_DEBUG_SCD_RT
            Vps_printf(" %d: SCD    : Motion Detected (chanID = %d),  !!!\n",
                   Utils_getCurTimeInMsec(), chanID );
#endif
            System_linkControl(SYSTEM_LINK_ID_HOST, VSYS_EVENT_MOTION_DETECT, scdResultBuff, sizeof(AlgLink_ScdResult), FALSE);
        }
    }

#if 0
    if(pScdAlgLinkObj->createArgs.enableTamperNotify)
    {

         /* Tamper detect event notification. */
        if(scdResult.frmResult == SCD_DETECTOR_TAMPER )
        {
            pChStatus.frmResult = (UInt32) ALG_LINK_SCD_DETECTOR_CHANGE;
            pChStatus.chId      = chanID;//chObj->chId;

#ifdef SYSTEM_DEBUG_SCD_RT
            Vps_printf(" %d: SCD    : Tamper Detected (chanID = %d) !!!\n",
                      Utils_getCurTimeInMsec(), chanID );
#endif

            System_linkControl(SYSTEM_LINK_ID_HOST, VSYS_EVENT_TAMPER_DETECT, &pChStatus, sizeof(AlgLink_ScdChStatus), FALSE);
        }
    }
    else
    {

       if(scdResult.frmResult == SCD_DETECTOR_TAMPER )
       {
           Vps_printf(" %d [TAMPER DETECTED] : SCD CH ID %d !!!\n",
                  Utils_getCurTimeInMsec(), chanID );

          if((chObj->scdChStat == ALG_LINK_SCD_DETECTOR_UNAVAILABLE) ||
             (chObj->scdChStat == ALG_LINK_SCD_DETECTOR_NO_CHANGE))
          {
               Vps_printf(" %d [TAMPER DETECTED] : SCD CH ID %d !!!\n",
                      Utils_getCurTimeInMsec(), chanID );
          }
       }
       else
       {
          if(chObj->scdChStat == SCD_DETECTOR_TAMPER)
          {

               Vps_printf(" %d [TAMPER REMOVED ] : SCD CH ID %d !!!\n",
                        Utils_getCurTimeInMsec(), chanID );
          }
       }
    }

    chObj->scdChStat       = ALG_LINK_SCD_DETECTOR_UNAVAILABLE;
    if(scdResult.frmResult == SCD_DETECTOR_NO_TAMPER)
    {
        chObj->scdChStat = ALG_LINK_SCD_DETECTOR_NO_CHANGE;
    }
    else if(scdResult.frmResult == SCD_DETECTOR_TAMPER)
    {
        chObj->scdChStat = ALG_LINK_SCD_DETECTOR_CHANGE;
    }
#endif
    return FVID2_SOK;
}

Int32 AlgLink_ScdalgProcessData(AlgLink_ScdObj * pObj)
{
    UInt32 curTime;
    FVID2_Frame *pFrame;
    Int32 chIdx, status;
    Bitstream_Buf *pOutBuf;
    Bitstream_BufList outBitBufList;
    System_FrameInfo *pInFrameInfo;
    Bool doFrameDrop;
    AlgLink_ScdChObj *pChObj;
    AlgLink_ScdchPrm    *pChPrm; 
    Utils_BitBufHndl *bufOutQue;
    DMVALstatus libStatus;
    DMVALimage inImage;
    Bool frameProcessedTAMPER,frameProcessedLMD; 

    DMVALout *pOutput;

    bufOutQue = &pObj->outObj[0].bufOutQue;
    outBitBufList.numBufs = 0;
    outBitBufList.appData = NULL;
    frameProcessedTAMPER = FALSE;
    frameProcessedLMD    = FALSE;

    status = FVID2_EFAIL;

    pFrame = NULL;
    
    Utils_queGet(&pObj->processQ, (Ptr*)&pFrame, 1, BIOS_WAIT_FOREVER);

    if(pFrame==NULL)
        return FVID2_EFAIL;
    do
    {
        for(chIdx = 0; chIdx < pObj->createArgs.numValidChForSCD; chIdx++)
        {
          if(pFrame->channelNum == pObj->chParams[chIdx].chId)
          {
            break;
          }
        }

        pChObj = &pObj->chObj[chIdx]; 
        pChPrm    = &pObj->chParams[chIdx];

        ti_sysbios_family_c64p_Cache_inv(pFrame->addr[0][0], pChObj->scdProcessObj.processFrameSize, 
                                        ti_sysbios_family_c64p_Cache_Type_ALL,TRUE);
      
        pInFrameInfo = (System_FrameInfo *) pFrame->appData;

        doFrameDrop = FALSE;
        pOutBuf = NULL;
        pObj->totalFrameCount++; 

        if(pChPrm->detectMode == DMVAL_DETECTMODE_TAMPER)
        {
             /***************************
                VA TAMPER ALG APPLY HERE
             ***************************/      
             DMVALhandle *pDmvaHndl = &pObj->dmvalHndl[chIdx];


             inImage.pixelDepth  = DMVAL_PIXEL_U08;
             inImage.width       = pChPrm->width;    
             inImage.height      = pChPrm->height;
             inImage.imageStride = pChPrm->stride;
             inImage.imageData   = pFrame->addr[0][0];

             inImage.imageSize   = (inImage.imageStride * inImage.height);
             inImage.type        = DMVAL_IMG_LUMA;
             inImage.horzOffset  = 0;
             inImage.vertOffset  = 0;
             inImage.timeStamp   = AlgLink_ScdGetFrameTS(pChObj,pFrame->timeStamp);

             curTime = Utils_getCurTimeInMsec(); 
     

             libStatus = DMVAL_NO_ERROR;
             libStatus = DMVAL_process(pDmvaHndl,&inImage);    
             if(libStatus != DMVAL_NO_ERROR)
             {
                Vps_printf(" %d: SCD   : VA Create Call Status 0x%X!!!\n", 
                       Utils_getCurTimeInMsec(),libStatus);
             }

             UTILS_assert(libStatus == DMVAL_NO_ERROR); 

             pChObj->inFrameProcessTime += (Utils_getCurTimeInMsec() - curTime);
             frameProcessedTAMPER = TRUE;

             pOutput = pDmvaHndl->output;  

              if(pObj->createArgs.enableTamperNotify)
              {

                   /* Tamper detect event notification. */
                  if(pOutput->modeResult & DMVAL_TAMPER_SCENECHANGE )
                  {
                     AlgLink_ScdChStatus pChStatus;
                     pChStatus.frmResult = (UInt32) ALG_LINK_SCD_DETECTOR_CHANGE;
                     pChStatus.chId      = pChPrm->chId;

                     System_linkControl(SYSTEM_LINK_ID_HOST, 
                                        VSYS_EVENT_TAMPER_DETECT, 
                                        &pChStatus, 
                                        sizeof(AlgLink_ScdChStatus), 
                                        FALSE);
                  }
              }
              else
              {
#ifdef SYSTEM_DEBUG_SCD_RT
                 Vps_printf(" %d [TAMPER Result %d] : SCD CH ID %d !!!\n",
                        Utils_getCurTimeInMsec(), pOutput->modeResult, pChPrm->chId );
#endif                        
                 if((pOutput->modeResult & DMVAL_TAMPER_SCENECHANGE ) || (pOutput->modeResult & DMVAL_DETECTOR_TAMPER))
                 {

                    if((pChObj->scdChStat == ALG_LINK_SCD_DETECTOR_UNAVAILABLE) ||
                       (pChObj->scdChStat == ALG_LINK_SCD_DETECTOR_NO_CHANGE))
                    {
                         Vps_printf(" %d [TAMPER DETECTED] : SCD CH ID %d !!!\n",
                                Utils_getCurTimeInMsec(), pChPrm->chId );
                    }
                 }
                 else
                 {
                    if(pChObj->scdChStat == ALG_LINK_SCD_DETECTOR_CHANGE)
                    {
                         Vps_printf(" %d [TAMPER REMOVED ] : SCD CH ID %d !!!\n",
                                  Utils_getCurTimeInMsec(), pChPrm->chId );
                    }
                 }
              }

              pChObj->scdChStat       = ALG_LINK_SCD_DETECTOR_NO_CHANGE;
              if(pOutput->modeResult == DMVAL_TAMPER_NONE)
              {
                  pChObj->scdChStat = ALG_LINK_SCD_DETECTOR_NO_CHANGE;
              }
              else if((pOutput->modeResult & DMVAL_TAMPER_SCENECHANGE) || (pOutput->modeResult & DMVAL_DETECTOR_TAMPER))
              {
                  pChObj->scdChStat = ALG_LINK_SCD_DETECTOR_CHANGE;
              }

        }
        else
        {
            pChObj->scdChStat       = ALG_LINK_SCD_DETECTOR_UNAVAILABLE;           
        }

        status = Utils_bitbufGetEmptyBuf(bufOutQue,
                                         &pOutBuf,
                                         0, //pObj->outObj.ch2poolMap[chIdx], /*Need to change later.*/
                                         BIOS_NO_WAIT);

        if(!((status == FVID2_SOK) && (pOutBuf)))
        {
           doFrameDrop = TRUE;
        }

        if(doFrameDrop == FALSE)
        {
            pObj->chParams[chIdx].curFrame = pFrame->addr[0][0];


            pOutBuf->lowerTimeStamp = (UInt32)(pInFrameInfo->ts64 & 0xFFFFFFFF);
            pOutBuf->upperTimeStamp = (UInt32)(pInFrameInfo->ts64 >> 32);
            pOutBuf->channelNum = pFrame->channelNum;
            pOutBuf->fillLength = sizeof(AlgLink_ScdResult);
            pOutBuf->frameWidth = pObj->chParams[chIdx].width;
            pOutBuf->frameHeight = pObj->chParams[chIdx].height;

            curTime = Utils_getCurTimeInMsec();

             /***************************
               SCD LMD ALG PROCESS CALL
             ***************************/      

            AlgLink_ScdalgProcess(pObj, chIdx, (AlgLink_ScdResult *) pOutBuf->addr);
            pChObj->inFrameProcessTime += (Utils_getCurTimeInMsec() - curTime);
            ti_sysbios_family_c64p_Cache_wb(pOutBuf->addr, sizeof(AlgLink_ScdResult), 
                                            ti_sysbios_family_c64p_Cache_Type_ALL,TRUE);

            frameProcessedLMD = TRUE;
            pObj->chParams[chIdx].chBlkConfigUpdate = FALSE;



            outBitBufList.bufs[outBitBufList.numBufs] = pOutBuf;
            outBitBufList.numBufs++;
        }



        if(frameProcessedTAMPER || frameProcessedLMD)
            pChObj->inFrameProcessCount++;
        else
            pChObj->inFrameProcessSkipCount++;

        status = Utils_quePut(&pChObj->scdProcessObj.freeQ, pFrame, BIOS_NO_WAIT);
        UTILS_assert(status == FVID2_SOK);

        if((outBitBufList.numBufs == pObj->createArgs.numValidChForSCD) ||
            (pObj->processTskState == ALG_LINK_PROCESS_TASK_STATE_STOPPING))
        {
            break;
        }

        pFrame = NULL;
        status = Utils_queGet(&pObj->processQ, (Ptr*)&pFrame, 1, BIOS_NO_WAIT);

        if (UTILS_ISERROR(status)) {
            break;
        }
    }while(pFrame != NULL);

    if (outBitBufList.numBufs)
    {
        status = Utils_bitbufPutFull(bufOutQue,
                                     &outBitBufList);
        UTILS_assert(status == FVID2_SOK);
        status = FVID2_SOK;
    }
    return status;
}

Int32 AlgLink_ScdAlgCopyFrame(AlgLink_ScdObj * pObj, FVID2_Frame *pFrame)
{
    FVID2_Frame *pEmptyFrame;
    AlgLink_ScdchPrm * pChPrm;
    AlgLink_ScdChObj * pChObj;
    Utils_DmaCopy2D dmaPrm;
    Int32 status, chIdx;

    pEmptyFrame = NULL;

    /* Search through ID list to get the proper channel no. */
    for(chIdx = 0; chIdx < pObj->createArgs.numValidChForSCD; chIdx++)
    {
      if(pFrame->channelNum == pObj->createArgs.chDefaultParams[chIdx].chId)
      {
        break;
      }
    }

    pChPrm = &pObj->chParams[chIdx];
    pChObj = &pObj->chObj[chIdx];;

    Utils_queGet(&pChObj->scdProcessObj.freeQ, (Ptr*)&pEmptyFrame, 1, BIOS_NO_WAIT);

    if(pEmptyFrame==NULL)
    {
        /* no free available for SCD, so skipping this frame */
        pChObj->inFrameUserSkipCount++;
        return FVID2_SOK;
    }
    ti_sysbios_family_c64p_Cache_inv(pEmptyFrame->addr[0][0], pChObj->scdProcessObj.processFrameSize, 
                                    ti_sysbios_family_c64p_Cache_Type_ALL,TRUE);

    memcpy(pEmptyFrame->appData, pFrame->appData, sizeof(System_FrameInfo));    
    /* found a free frame, do DMA copy of Y data to this frame */

    pEmptyFrame->timeStamp = pFrame->timeStamp;

    dmaPrm.destAddr[0]  = pEmptyFrame->addr[0][0];
    dmaPrm.destPitch[0] = pChPrm->stride;

    dmaPrm.srcAddr[0]  = pFrame->addr[0][0];
    dmaPrm.srcPitch[0] = pChPrm->stride;

    /* need to transfer only Y data, so set data format as 422I and set width = actual width / 2 */
    dmaPrm.dataFormat = FVID2_DF_YUV422I_YUYV;
    dmaPrm.srcStartX  = 0;
    dmaPrm.srcStartY  = 0;
    dmaPrm.destStartX = 0;
    dmaPrm.destStartY = 0;
    dmaPrm.width      = pChPrm->width/2;
    dmaPrm.height     = pChPrm->height;

    status = Utils_dmaCopy2D(&pObj->dmaCh, &dmaPrm, 1);
    UTILS_assert(status==FVID2_SOK);

    pEmptyFrame->channelNum = pFrame->channelNum;

    status = Utils_quePut(&pObj->processQ, pEmptyFrame, BIOS_NO_WAIT);

    if(status!=FVID2_SOK)
    {
        /* cannot submit frame now process queue is full, release frame to free Q */
        pChObj->inFrameUserSkipCount++;

        status = Utils_quePut(&pChObj->scdProcessObj.freeQ, pEmptyFrame, BIOS_NO_WAIT);

        /* this assert should never occur */
        UTILS_assert(status==FVID2_SOK);

        return FVID2_SOK;
    }

    return FVID2_SOK;
}

Int32 AlgLink_ScdAlgSubmitFrames(AlgLink_ScdObj * pObj, FVID2_FrameList *pFrameList)
{
    Int32 status = FVID2_SOK;
    UInt32 frameId, chIdx;
    FVID2_Frame *pFrame;
    AlgLink_ScdChObj *pChObj;
    Bool skipFrame;
    Int32 frameFound;
    
    for(frameId=0; frameId<pFrameList->numFrames; frameId++)
    {
        pFrame = pFrameList->frames[frameId];

        if(pFrame==NULL)
            continue;

        frameFound = 0;

        /* Search through ID list to get the proper channel no. */
        for(chIdx = 0; chIdx < pObj->createArgs.numValidChForSCD; chIdx++)
        {
            if(pFrame->channelNum == pObj->createArgs.chDefaultParams[chIdx].chId)
            {
              frameFound = 1;
              break;
            }
        }

        /* If no corresponding channel is enabled, discard/return the frame */
        if(frameFound == 0)
            continue;

        UTILS_assert(chIdx < UTILS_ARRAYSIZE(pObj->chObj));

        pChObj = &pObj->chObj[chIdx];
        pChObj->inFrameRecvCount++;
      
        /* Frame rate control at input of SCD link */
        skipFrame = Utils_doSkipFrame(&pChObj->frameSkipCtx);
        if( (skipFrame) || (pObj->createArgs.chDefaultParams[chIdx].mode == 0))
        {
            pChObj->inFrameUserSkipCount++;
            continue;
        }

        AlgLink_ScdAlgCopyFrame(pObj, pFrame);
    }

    return status;
}


Int32 AlgLink_ScdalgSetChblkUpdate(AlgLink_ScdObj * pObj,
                            AlgLink_ScdChblkUpdate * params)
{
    AlgLink_ScdchPrm *pScdChPrm;
    Int32 channelUpdated, idx;

    channelUpdated = FVID2_EFAIL;
    // Search through ID list to get the proper channel no.
    for(idx = 0; idx < pObj->createArgs.numValidChForSCD; idx++)
    {
        pScdChPrm = &pObj->chParams[idx];
        if(params->chId == pScdChPrm->chId)
        {
            pScdChPrm->chBlkConfigUpdate = TRUE;
            for(idx = 0; idx < params->numValidBlock; idx++)
            {
                SCD_blkChngConfig * blkConfig;
                blkConfig = (SCD_blkChngConfig *) ((UInt32)(pScdChPrm->blkConfig) + \
                               (sizeof(SCD_blkChngConfig) * params->blkConfig[idx].blockId));

                blkConfig->monitored = params->blkConfig[idx].monitorBlock;
                blkConfig->sensitivity = (SCD_Sensitivity) params->blkConfig[idx].sensitivity;

                channelUpdated = FVID2_SOK;
            }
            break;
        }
    }

    return (channelUpdated);
}

Int32 AlgLink_ScdalgSetChScdSensitivity(AlgLink_ScdObj * pObj,
                            AlgLink_ScdChParams * params)
{
    AlgLink_ScdchPrm *pScdChPrm;
    Int32 channelUpdated, idx;
    UInt32 sensitivity;
    DMVALhandle *pDmvaHndl;

    channelUpdated = FVID2_EFAIL;
    // Search through ID list to get the proper channel no.
    for(idx = 0; idx < pObj->createArgs.numValidChForSCD; idx++)
    {
      pScdChPrm = &pObj->chParams[idx];
      if(params->chId == pScdChPrm->chId)
      {
         /* VA Tamper Sensitivity update */
        pDmvaHndl   = &pObj->dmvalHndl[idx];
        sensitivity = params->frmSensitivity;

        if(sensitivity > DMVAL_SENSITIVITY_HIGH)
            pScdChPrm->sensitiveness = DMVAL_SENSITIVITY_HIGH;
        else if(sensitivity < DMVAL_SENSITIVITY_LOW)
           pScdChPrm->sensitiveness = DMVAL_SENSITIVITY_LOW;
        else
           pScdChPrm->sensitiveness = (DMVALsensitivity) sensitivity;

        DMVAL_setParameter(pDmvaHndl,DMVAL_PARAM_SENSITIVITY,(Int16*)&(pScdChPrm->sensitiveness),1);

         /* SCD LMD Sensitivity update */
        pScdChPrm->frmSensitivity  = (SCD_Sensitivity) params->frmSensitivity;

        channelUpdated = FVID2_SOK;
        break;
      }
    }

    return (channelUpdated);
}

Int32 AlgLink_ScdalgSetChScdMode(AlgLink_ScdObj * pObj,
                            AlgLink_ScdChParams * params)
{
    AlgLink_ScdchPrm *pScdChPrm;
    Int32 channelUpdated, idx;

    channelUpdated = FVID2_EFAIL;
    // Search through ID list to get the proper channel no.
    for(idx = 0; idx < pObj->createArgs.numValidChForSCD; idx++)
    {
      pScdChPrm = &pObj->chParams[idx];
      if(params->chId == pScdChPrm->chId)
      {
         /* SCD LMD mode update */
        pScdChPrm->mode       = (SCD_Mode) (params->mode & 0x2);

         /* VA Tamper mode update */
        pScdChPrm->detectMode = (DMVALdetectMode) (params->mode & 0x1);

        channelUpdated = FVID2_SOK;
        break;
      }
    }

    return (channelUpdated);
}

Int32 AlgLink_ScdalgSetChScdIgnoreLightsOn(AlgLink_ScdObj * pObj,
                            AlgLink_ScdChParams * params)
{
    AlgLink_ScdchPrm *pScdChPrm;
    Int32 channelUpdated, idx;

    channelUpdated = FVID2_EFAIL;
    // Search through ID list to get the proper channel no.
    for(idx = 0; idx < pObj->createArgs.numValidChForSCD; idx++)
    {
      pScdChPrm = &pObj->chParams[idx];
      if(params->chId == pScdChPrm->chId)
      {
        pScdChPrm->frmIgnoreLightsON = params->frmIgnoreLightsON;
        channelUpdated = FVID2_SOK;
        break;
      }
    }

    return (channelUpdated);
}

Int32 AlgLink_ScdalgSetChScdIgnoreLightsOff(AlgLink_ScdObj * pObj,
                            AlgLink_ScdChParams * params)
{
    AlgLink_ScdchPrm *pScdChPrm;
    Int32 channelUpdated, idx;

    channelUpdated = FVID2_EFAIL;
    // Search through ID list to get the proper channel no.
    for(idx = 0; idx < pObj->createArgs.numValidChForSCD; idx++)
    {
      pScdChPrm = &pObj->chParams[idx];
      if(params->chId == pScdChPrm->chId)
      {
        pScdChPrm->frmIgnoreLightsOFF = params->frmIgnoreLightsOFF;
        channelUpdated = FVID2_SOK;
        break;
      }
    }

    return (channelUpdated);
}

static Int32 AlgLink_ScdalgSetChScdPrm(AlgLink_ScdchPrm    *pScdChPrm,
                            AlgLink_ScdChParams * params)
{
    Int32 blkIdx;

    pScdChPrm->chId               = params->chId;
    pScdChPrm->chBlkConfigUpdate  = FALSE;

    pScdChPrm->mode               = (SCD_Mode) params->mode;
    pScdChPrm->frmSensitivity     = (SCD_Sensitivity) params->frmSensitivity;
    pScdChPrm->frmIgnoreLightsOFF = params->frmIgnoreLightsOFF;
    pScdChPrm->frmIgnoreLightsON  = params->frmIgnoreLightsON;


    for(blkIdx = 0; blkIdx < ALG_LINK_SCD_MAX_BLOCKS_IN_FRAME; blkIdx++)
    {
        pScdChPrm->blkConfig[blkIdx].monitored   = params->blkConfig[blkIdx].monitored;
        pScdChPrm->blkConfig[blkIdx].sensitivity = (SCD_Sensitivity) params->blkConfig[blkIdx].sensitivity;
    }

    return 0;
}

Int32 AlgLink_ScdVAChReset(AlgLink_ScdObj * pObj, UInt32 chId)
{
    AlgLink_ScdchPrm    *pChPrm; 
    AlgLink_ScdChObj    *pChObj;

    DMVALstatus libStatus;
    DMVALhandle *pDmvaHndl;
    Int32       scdChId;
    UInt32      sensitivity, numFrames2Wait2MarkStableFrame;

    for(scdChId = 0;  scdChId<pObj->createArgs.numValidChForSCD; scdChId++)
    {
        pChObj    = &pObj->chObj[scdChId];
        if(chId == pChObj->chId)
           break;
    }

    pChPrm    = &pObj->chParams[scdChId];
    pDmvaHndl = &pObj->dmvalHndl[scdChId];

    pChObj->frameTS.clock.tv_sec  = 0;
    pChObj->frameTS.clock.tv_usec = 0; 

    sensitivity = pObj->createArgs.chDefaultParams[scdChId].frmSensitivity;

    if(sensitivity > DMVAL_SENSITIVITY_HIGH)
        pChPrm->sensitiveness = DMVAL_SENSITIVITY_HIGH;
    else if(sensitivity < DMVAL_SENSITIVITY_LOW)
       pChPrm->sensitiveness = DMVAL_SENSITIVITY_LOW;
    else
       pChPrm->sensitiveness = (DMVALsensitivity) sensitivity;

    pChPrm->detectMode = (DMVALdetectMode) (pObj->createArgs.chDefaultParams[scdChId].mode & 0x1);

    pChPrm->resetCounter  = ALG_LINK_TAMPER_RESET_COUNT_SCALER_CONSTANT * pChObj->frameSkipCtx.outputFrameRate; 

    memset(pDmvaHndl,0x00,sizeof(DMVALhandle));
    pDmvaHndl->output = &pChObj->output;

    libStatus = DMVAL_initModule(pDmvaHndl);
    UTILS_assert(libStatus == DMVAL_NO_ERROR);   

    DMVAL_setParameter(
                       pDmvaHndl,DMVAL_PARAM_SENSITIVITY,
                       (Int16*)&(pChPrm->sensitiveness),
                       1);

    DMVAL_setParameter(
                       pDmvaHndl,DMVAL_PARAM_TAMPERCOUNTER,
                       (Int16*)&(pChPrm->resetCounter),
                       1);

    DMVAL_setParameter(
                    pDmvaHndl,DMVAL_PARAM_TAMPER_BLOCKUP_TH, 
                       (Int16*)pChPrm->thresold2WaitB4FrmAlert,
                       1);

    DMVAL_setParameter(
                       pDmvaHndl,DMVAL_PARAM_TAMPER_BLOCKDOWN_TH, 
                       (Int16*)&pChPrm->thresold2WaitAfterFrmAlert,
                       1);

    numFrames2Wait2MarkStableFrame = pChPrm->numSecs2Wait2MarkStableFrame * pChObj->frameSkipCtx.outputFrameRate;
    DMVAL_setParameter(
                       pDmvaHndl,DMVAL_PARAM_TAMPER_BACKGROUND_FRM_INTERVAL, 
                       (Int16*)&numFrames2Wait2MarkStableFrame, 
                       1);

#ifdef SYSTEM_DEBUG_SCD
    Vps_printf(" %d: SCD   : VA ALG Reset Done of Channel - %d!!!\n", Utils_getCurTimeInMsec(),pChObj->chId);
#endif

    return FVID2_SOK;
} 

Int32 AlgLink_ScdresetStatistics(AlgLink_ScdObj *pObj)
{
    UInt32 chId;
    AlgLink_ScdChObj *pChObj;
    UInt32 oldIntState;

    oldIntState = Hwi_disable();

    for (chId = 0; chId < pObj->createArgs.numValidChForSCD; chId++)
    {
        pChObj = &pObj->chObj[chId];

        pChObj->inFrameRecvCount = 0;
        pChObj->inFrameUserSkipCount    = 0;


        pChObj->inFrameProcessCount     = 0;
        pChObj->inFrameProcessTime      = 0;
        pChObj->inFrameProcessSkipCount = 0;

    }
    Hwi_restore(oldIntState);
    pObj->totalFrameCount = 0;

    pObj->statsStartTime = Utils_getCurTimeInMsec();



    return 0;
}


Int32 AlgLink_scdAlgGetAllChFrameStatus(AlgLink_ScdObj * pObj, AlgLink_ScdAllChFrameStatus *pPrm)
{
    AlgLink_ScdChObj *pChObj;
    AlgLink_ScdCreateParams *pCreateArgs;
    UInt32 chId;

    pCreateArgs = &pObj->createArgs;

    pPrm->numCh = pCreateArgs->numValidChForSCD;

    for(chId=0; chId<pCreateArgs->numValidChForSCD; chId++)
    {
       pPrm->chanFrameResult[chId].frmResult = (UInt32)ALG_LINK_SCD_DETECTOR_UNAVAILABLE;
    }

    for(chId=0; chId<pCreateArgs->numValidChForSCD; chId++)
    {
        pChObj = &pObj->chObj[chId];

        pPrm->chanFrameResult[chId].chId      = pChObj->chId;
        pPrm->chanFrameResult[chId].frmResult = pChObj->scdChStat;
    }

    return FVID2_SOK;

}

Int32 AlgLink_ScdalgGetChResetChannel(AlgLink_ScdObj * pObj,
                            AlgLink_ScdChCtrl * params)
{
    SCD_Status      chanStatus;

    chanStatus = SCD_TI_resetChannel(
                                    pObj->algHndl,
                                    params->chId);

    if(chanStatus != SCD_NO_ERROR)
    {
#ifdef SYSTEM_DEBUG_SCD
        Vps_printf(" %d: SCD    : ERROR: Alg Process (chanID = %d) !!!\n",
                  Utils_getCurTimeInMsec(), params->chId );
#endif

        return FVID2_EFAIL;
    }
    else
    {
        Vps_printf(" %d: SCD    : chanID = %d Reset Done !!!\n",
                  Utils_getCurTimeInMsec(), params->chId);
    }

    AlgLink_ScdVAChReset(pObj, params->chId);

    return 0;
}

Int32 AlgLink_ScdprintStatistics (AlgLink_ScdObj *pObj, Bool resetAfterPrint)
{
    UInt32 chId;
    AlgLink_ScdChObj *pChObj;
    UInt32 elaspedTime;

    elaspedTime = Utils_getCurTimeInMsec() - pObj->statsStartTime; // in msecs
    elaspedTime /= 1000; // convert to secs

    Vps_printf( " \n"
            " *** SCD Statistics *** \n"
            " \n"
            " Elasped Time           : %d secs\n"
            " Total Fields Processed : %d \n"
            " Total Fields FPS       : %d FPS\n"
            " \n"
            " \n"
            " CH  | In Recv In Process User Skip Process Skip In Process Time \n"
            " Num | FPS     FPS        FPS       FPS          per frame (msec)\n"
            " ----------------------------------------------------------------\n",
            elaspedTime,
            pObj->totalFrameCount,
            pObj->totalFrameCount / (elaspedTime)
                    );

    for (chId = 0; chId < pObj->createArgs.numValidChForSCD; chId++)
    {
        pChObj = &pObj->chObj[chId];

        Vps_printf( " %3d | %7d %10d %8d %11d %14d\n",
            pChObj->chId,
            pChObj->inFrameRecvCount/elaspedTime,
            pChObj->inFrameProcessCount/elaspedTime,
            pChObj->inFrameUserSkipCount/elaspedTime,
            pChObj->inFrameProcessSkipCount/elaspedTime,
            (pChObj->inFrameProcessTime  )/(pChObj->inFrameProcessCount)
            );
    }

    Vps_printf( " \n");

    if(resetAfterPrint)
    {
        AlgLink_ScdresetStatistics(pObj);
    }
    return FVID2_SOK;
}

