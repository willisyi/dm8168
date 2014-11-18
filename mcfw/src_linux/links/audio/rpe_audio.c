/*******************************************************************************
 *                                                                            
 * Copyright (c) 2012 Texas Instruments Incorporated - http://www.ti.com/      
 *                        ALL RIGHTS RESERVED                                  
 *                                                                            
 ******************************************************************************/

#include <stdint.h>  
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <ti/Std.h>
#include <ti/xdais/dm/iaudenc1.h>
#include <ti/xdais/dm/iauddec1.h>
#include "ti/rpe.h"
#include <link_api/audioLink.h>

#if     1
#define     WRITE_MODE      RPE_CPU_ACCESS_MODE_WRITE
#define     READ_MODE       RPE_CPU_ACCESS_MODE_READ
#else
#define     WRITE_MODE      RPE_CPU_ACCESS_MODE_NONE
#define     READ_MODE       RPE_CPU_ACCESS_MODE_NONE
#endif


/*
typedef enum
{
    AUDIO_CODEC_TYPE_G711,
    AUDIO_CODEC_TYPE_AAC_LC,
    AUDIO_CODEC_TYPE_MAX

} AUDIO_CODEC_TYPE;

*/
typedef     struct  
{
    Int32 codecType;
    Int32 numChannels;
    Int32 inputBytesPerSample;
} encContext;

typedef     struct  
{
    Int32 codecType;
    Int32 desiredChannelMode;
} decContext;

typedef     struct  
{
    UInt16   *encInputData;
    UInt8    *encOutputData;
    UInt32   encMinInputBufSize; 
    UInt32   encMinOutputBufSize;
    Int32    encFrameCnt;

    /** Client handle from create call */
    Rpe_ClientHandle            encClientHandle; 
    Rpe_CallDescHandle          encProcessCallDesc, /** Process call desc */
                                encControlCallDesc; /** Control call desc */
    /** Encoder Create Parameter */
    IAUDENC1_Params             encParams ;
    /** Pointers to decoder control call parameters located in call decriptor */
    IAUDENC1_DynamicParams      *encDynParams ;
    IAUDENC1_Status             *encStatus ;
    IAUDENC1_Cmd                *cmdId;

    /** Pointers to decoder process call parameters located in call decriptor */
    XDM1_BufDesc                *inBufDesc;
    XDM1_BufDesc                *outBufDesc;
    IAUDENC1_InArgs             *encInArgs;
    IAUDENC1_OutArgs            *encOutArgs;
} encContextDsp;


typedef     struct  
{
    UInt16   *decInputData;
    UInt8    *decOutputData;
    UInt32   decMinInputBufSize; 
    UInt32   decMinOutputBufSize;
    Int32    decFrameCnt;

    /** Client handle from create call */
    Rpe_ClientHandle            decClientHandle; 
    Rpe_CallDescHandle          decProcessCallDesc, /** Process call desc */
                                decControlCallDesc; /** Control call desc */
    /** Decoder Create Parameter */
    IAUDDEC1_Params             decParams;
    /** Pointers to decoder control call parameters located in call decriptor */
    IAUDDEC1_DynamicParams      *decDynParams;
    IAUDDEC1_Status             *decStatus;
    IAUDDEC1_Cmd                *cmdId;

    /** Pointers to decoder process call parameters located in call decriptor */
    XDM1_BufDesc                *inBufDesc;
    XDM1_BufDesc                *outBufDesc;
    IAUDDEC1_InArgs             *decInArgs;
    IAUDDEC1_OutArgs            *decOutArgs;
} decContextDsp;

#define     GET_ENC_INT_STRUCT(X)  ((encContextDsp *) ((UInt32)X + sizeof(encContext)))
#define     GET_DEC_INT_STRUCT(X)  ((decContextDsp *) ((UInt32)X + sizeof(decContext)))


static XDAS_Int8 channMap[7] = {-1, IAUDIO_1_0, IAUDIO_2_0, -1, -1, IAUDIO_3_2, IAUDIO_3_2};

static Int32 Audio_encodeAsULaw (Void* ctxMem, Audio_EncodeProcessParams *pPrm);
static Int32 Audio_decodeULaw(Void* ctxMem, Audio_DecodeProcessParams *pPrm);

Int32   Audio_getEncoderContextSize (Int32 codecType)
{
    if (codecType == AUDIO_CODEC_TYPE_AAC_LC)
        return  (sizeof(encContext) + sizeof(encContextDsp));
    else
        return (sizeof(encContext));
}

Int32   Audio_getDecoderContextSize (Int32 codecType)
{
    if (codecType == AUDIO_CODEC_TYPE_AAC_LC)
        return  (sizeof(decContext) + sizeof(decContextDsp));
    else
        return (sizeof(decContext));
}

Void *Audio_createG711EncAlgorithm(Void *ctxMem)
{
    encContext           *handle;

    handle = ctxMem;
    if (handle == NULL)
    {
        AUDIO_ERROR_PRINT(("AUDIO: Enc creation <malloc> failed...\n"));
        return NULL;
    }
    handle->codecType = AUDIO_CODEC_TYPE_G711;
    return handle;
}

Void* Audio_createAacEncAlgorithm(Void *ctxMem, Audio_EncodeCreateParams *pPrm                            )
{
    Rpe_Attributes       instAttr = {0};
    Int32              status;                 
    encContextDsp        *ctx;
    encContext           *handle;
    IAUDENC1_Params      *encParams;

#if !defined(DSP_RPE_AUDIO_ENABLE)
    return NULL;
#endif

    /* Allocated for both external / internal data structure */
    handle = ctxMem;
    if (handle == NULL || pPrm == NULL)
    {
        AUDIO_ERROR_PRINT (("AUDIO: Enc creation <malloc> failed...\n"));
        return NULL;
    }
    handle->codecType = AUDIO_CODEC_TYPE_AAC_LC;

    ctx = GET_ENC_INT_STRUCT(handle);
    AUDIO_INFO_PRINT (("AUDIO: ENC Handle %X, Int Handle %X\n", (UInt32) handle, (UInt32)ctx));

    memset(ctx, 0, sizeof(encContextDsp));

    encParams = &ctx->encParams;

    /*--------------------------------------------------------------------*/
    /* Set AAC encoder create time parameters                             */
    /*--------------------------------------------------------------------*/
    encParams->size           = sizeof (IAUDENC1_Params);
    encParams->bitRate        = pPrm->bitRate;
    encParams->inputBitsPerSample = 16;
    encParams->sampleRate     = pPrm->sampleRate;
    encParams->ancFlag        = 0;
    encParams->channelMode    = channMap[pPrm->numberOfChannels];
    encParams->dataEndianness = (XDAS_Int32)(XDM_LE_16);
    encParams->crcFlag        = 0;
    encParams->dualMonoMode   = 0;
    encParams->encMode        = IAUDIO_CBR;
    encParams->inputFormat    = IAUDIO_INTERLEAVED;
    encParams->lfeFlag        = 0;
    encParams->maxBitRate     = 192000;

    handle->numChannels       = pPrm->numberOfChannels;
    handle->inputBytesPerSample = (16 >> 3);

    AUDIO_INFO_PRINT(("AUDIO: ENC -> bitrate %d inputBitsPerSample %d, sampleRate - %d, numberOfChannels - %d...\n", 
                    encParams->bitRate, encParams->inputBitsPerSample, encParams->sampleRate, pPrm->numberOfChannels));

    /*--------------------------------------------------------------------*/
    /* Set RPE attributes                                                 */
    /*--------------------------------------------------------------------*/
    instAttr.priority            = RPE_PROCESSING_PRIORITY_MEDIUM;
    instAttr.inBufCpuAccessMode  = WRITE_MODE;
    instAttr.outBufCpuAccessMode = READ_MODE;

    status = Rpe_create ("AAC_AENC_TI", 
                             &instAttr, 
                             &ctx->encParams, 
                             &ctx->encClientHandle);
    if (RPE_S_SUCCESS != status)
    {
        AUDIO_ERROR_PRINT (("AUDIO: ENC -> Rpe_create failed, status: %d\n", status));
        return NULL;
    }
    AUDIO_INFO_PRINT (("AUDIO: ENC -> Rpe_create(AAC_ENC) done....\n"));

    status = Rpe_acquireCallDescriptor (ctx->encClientHandle, 
                                        RPE_CALL_DESC_CONTROL,
                                        &ctx->encControlCallDesc,
                                        &ctx->cmdId, 
                                        &ctx->encDynParams,
                                        &ctx->encStatus);

    if (RPE_S_SUCCESS != status)
    {
        AUDIO_ERROR_PRINT (("AUDIO: ENC -> Rpe_acquireCallDescriptor failed, status: %d\n", status));
        Rpe_delete(ctx->encClientHandle);
        return NULL;
    }

    /*----------------------------------------------------------------------*/
    /* Call control api using XDM_GETBUFINFO to get I/O buffer requirements */
    /*----------------------------------------------------------------------*/

    *(ctx->cmdId) = XDM_GETBUFINFO;
    ctx->encDynParams->size = sizeof (IAUDENC1_DynamicParams);
    ctx->encStatus->size    = sizeof (IAUDENC1_Status);

    /*--------------------------------------------------------------------*/
    /* Set AAC encoder run time parameters                                */
    /*--------------------------------------------------------------------*/

    ctx->encDynParams->channelMode = encParams->channelMode;
    ctx->encDynParams->lfeFlag    = encParams->lfeFlag;
    ctx->encDynParams->sampleRate = encParams->sampleRate;
    ctx->encDynParams->bitRate = encParams->bitRate;
    ctx->encDynParams->dualMonoMode = encParams->dualMonoMode;
    ctx->encDynParams->inputBitsPerSample = encParams->inputBitsPerSample;
    

    status = Rpe_control (ctx->encControlCallDesc);
    if (RPE_S_SUCCESS != status)
    {
        AUDIO_ERROR_PRINT (("AUDIO: ENC -> Rpe control call XDM_GETBUFINFO failed, status: %d\n", status));
        Rpe_delete(ctx->encClientHandle);
        return NULL;
    }

    status = Rpe_acquireCallDescriptor (ctx->encClientHandle, 
                                        RPE_CALL_DESC_PROCESS,
                                        &ctx->encProcessCallDesc,
                                        &ctx->inBufDesc, 
                                        &ctx->outBufDesc,
                                        &ctx->encInArgs, 
                                        &ctx->encOutArgs);
    if (RPE_S_SUCCESS != status)
    {
        AUDIO_ERROR_PRINT (("AUDIO: ENC -> Rpe_acquireCallDescriptor failed, status: %d\n", status));
        Rpe_delete(ctx->encClientHandle);
        return NULL;
    }
/*
    AUDIO_INFO_PRINT(("AUDIO: ENC -> ********* Rpe_acquireCallDescriptor: %X %X %X %X %X\n", 
        (UInt32) ctx->encProcessCallDesc,
        (UInt32) ctx->inBufDesc,
        (UInt32) ctx->outBufDesc,
        (UInt32) ctx->encInArgs,
        (UInt32) ctx->encOutArgs
        ));
*/
    ctx->inBufDesc->numBufs = ctx->encStatus->bufInfo.minNumInBufs;
    ctx->encMinInputBufSize = 
        ctx->inBufDesc->descs[0].bufSize = ctx->encStatus->bufInfo.minInBufSize[0];
    ctx->outBufDesc->numBufs = ctx->encStatus->bufInfo.minNumOutBufs;
    ctx->encMinOutputBufSize = 
        ctx->outBufDesc->descs[0].bufSize = ctx->encStatus->bufInfo.minOutBufSize[0];

    *(ctx->cmdId) = XDM_SETDEFAULT;
    ctx->encDynParams->size = sizeof (IAUDENC1_DynamicParams);
    ctx->encStatus->size    = sizeof (IAUDENC1_Status);

    status = Rpe_control (ctx->encControlCallDesc);
    if (RPE_S_SUCCESS != status)
    {
        AUDIO_ERROR_PRINT (("AUDIO: ENC -> Rpe control call XDM_SETDEFAULT failed, status: %d\n", status));
        Rpe_delete(ctx->encClientHandle);
        return NULL;
    }

    /*--------------------------------------------------------------------*/
    /* Call control api using XDM_SETPARAMS to set run-time parameters    */
    /*--------------------------------------------------------------------*/

    *(ctx->cmdId) = XDM_SETPARAMS;
    ctx->encDynParams->size = sizeof (IAUDENC1_DynamicParams);
    ctx->encStatus->size    = sizeof (IAUDENC1_Status);
    status = Rpe_control (ctx->encControlCallDesc);
    if (RPE_S_SUCCESS != status)
    {
        AUDIO_ERROR_PRINT (("AUDIO: ENC -> Rpe control call XDM_SETPARAMS failed, status: %d\n", status));
        Rpe_delete(ctx->encClientHandle);
        return NULL;
    }

    ctx->encInArgs->size    = sizeof(IAUDENC1_InArgs);
    ctx->encInArgs->numInSamples = 0;
    ctx->encInArgs->ancData.buf = NULL;
    ctx->encInArgs->ancData.bufSize = 0;

    ctx->encOutArgs->size   = sizeof(IAUDENC1_OutArgs);
    ctx->encOutArgs->bytesGenerated = 0;
    ctx->encOutArgs->extendedError = XDM_EOK;

    pPrm->minInBufSize = ctx->encMinInputBufSize;
    pPrm->minOutBufSize = ctx->encMinOutputBufSize;
    return ctxMem;
}


Int32 Audio_encodeAsAac (Void* ctxMem, Audio_EncodeProcessParams *pPrm)
{
    Int32          status = -1;                 
    encContextDsp  *ctx;
    encContext     *handle;
    
#if !defined(DSP_RPE_AUDIO_ENABLE)
    pPrm->outBuf.dataBufSize = 0;
    return 0;
#endif
    
    /* Allocated for both external / internal data structure */
    handle = ctxMem;

    if (handle)
    {
        ctx = GET_ENC_INT_STRUCT(handle);

        ctx->inBufDesc->descs[0].buf = (XDAS_Int8*)pPrm->inBuf.dataBuf;
        ctx->outBufDesc->descs[0].buf = (XDAS_Int8*)pPrm->outBuf.dataBuf;
        if (pPrm->inBuf.dataBufSize > ctx->encMinInputBufSize)
        {
             pPrm->inBuf.dataBufSize = ctx->encMinInputBufSize;
        }
        ctx->inBufDesc->descs[0].bufSize = pPrm->inBuf.dataBufSize;
        ctx->encInArgs->numInSamples = (ctx->inBufDesc->descs[0].bufSize / handle->inputBytesPerSample);
        ctx->encInArgs->numInSamples /= handle->numChannels;

        ctx->outBufDesc->descs[0].bufSize = pPrm->outBuf.dataBufSize;
    
        status = Rpe_process (ctx->encProcessCallDesc);

/*
        printf ("ENC -> in<%X, %d> out<%X, %d>, bytesGen %d, err %d, Bufs in <%X>, out <%X>\n", 
            (UInt32) pPrm->inBuf.dataBuf, pPrm->inBuf.dataBufSize, 
            (UInt32) pPrm->outBuf.dataBuf, pPrm->outBuf.dataBufSize, 
            ctx->encOutArgs->bytesGenerated, ctx->encOutArgs->extendedError,
            (UInt32) ctx->inBufDesc->descs[0].buf, (UInt32) ctx->outBufDesc->descs[0].buf);
*/
        if ((RPE_S_SUCCESS != status) && (ctx->encOutArgs->extendedError))
        {
            AUDIO_ERROR_PRINT(("AUDIO: ENC -> AAC encoder failed error %0x to encode frame %d \n", 
                            (UInt32) ctx->encOutArgs->extendedError, ctx->encFrameCnt));
            if (XDM_ISFATALERROR(ctx->encOutArgs->extendedError)) 
            {
                *(ctx->cmdId) = XDM_RESET;
                status = Rpe_control (ctx->encControlCallDesc);    
            }
            ctx->encOutArgs->extendedError = 0;
        }
        *(ctx->cmdId) = XDM_GETSTATUS;
        status = Rpe_control (ctx->encControlCallDesc);   
        pPrm->outBuf.dataBufSize = ctx->encOutArgs->bytesGenerated;
        if (ctx->encOutArgs->extendedError)
        {
            AUDIO_ERROR_PRINT(("AUDIO: ENC -> Error %X, frames => %d, InSamples %d <%d/%d bytes>, " 
                    "Bytes generated - %d </ %d>\n", 
                    ctx->encOutArgs->extendedError, ctx->encFrameCnt, ctx->encInArgs->numInSamples, pPrm->inBuf.dataBufSize, 
                    ctx->inBufDesc->descs[0].bufSize, ctx->encOutArgs->bytesGenerated, 
                    ctx->outBufDesc->descs[0].bufSize));              

        }
        ctx->encFrameCnt++;
    } 
    return status;
}

Int32 Audio_encode(Void* ctxMem, Audio_EncodeProcessParams *pPrm)
{
    encContext *handle = ctxMem;
    if (handle)
    {
        switch (handle->codecType)
        {
            case AUDIO_CODEC_TYPE_AAC_LC:
                return Audio_encodeAsAac(ctxMem, pPrm);
            
            case AUDIO_CODEC_TYPE_G711:
                return Audio_encodeAsULaw(ctxMem, pPrm);
        }
    }
    return 0;
}


Void *Audio_createAacDecAlgorithm(Void *ctxMem, Audio_DecodeCreateParams *pPrm)
{
    Rpe_Attributes       instAttr = {0};
    Int32                status;                 
    decContextDsp        *ctx;
    decContext           *handle;
    IAUDDEC1_Params      *decParams;

#if !defined(DSP_RPE_AUDIO_ENABLE)
    return NULL;
#endif

    /* Allocated for both external / internal data structure */
    handle = ctxMem;
    if (handle == NULL || pPrm == NULL)
    {
        AUDIO_ERROR_PRINT(("AUDIO: DEC -> creation <malloc> failed...\n"));
        return NULL;
    }
    handle->codecType = AUDIO_CODEC_TYPE_AAC_LC;
    handle->desiredChannelMode = pPrm->desiredChannelMode;

    ctx = GET_DEC_INT_STRUCT(handle);
    AUDIO_INFO_PRINT (("AUDIO: DEC -> Handle %X, Int Handle %X\n", (UInt32) handle, (UInt32)ctx));


    memset(ctx, 0, sizeof(decContextDsp));

    decParams = &ctx->decParams;
    
    /*-------------------------------------------------------------------*/
    /* Set AAC decoder create time parameters                            */
    /*-------------------------------------------------------------------*/
    decParams->size           = sizeof (IAUDDEC1_Params);
    decParams->outputPCMWidth = 16;
    decParams->pcmFormat      = IAUDIO_INTERLEAVED;
    decParams->dataEndianness = XDM_LE_16;

    /*--------------------------------------------------------------------*/
    /* Set inst attributes                                                */
    /*--------------------------------------------------------------------*/
    instAttr.priority            = RPE_PROCESSING_PRIORITY_MEDIUM;
    instAttr.inBufCpuAccessMode  = WRITE_MODE;
    instAttr.outBufCpuAccessMode = READ_MODE;

    status = Rpe_create ("AAC_ADEC_TI", 
                            &instAttr, 
                            &ctx->decParams, 
                            &ctx->decClientHandle);
    if (RPE_S_SUCCESS != status)
    {
        AUDIO_ERROR_PRINT (("AUDIO: DEC -> Rpe_create failed, status: %d\n", status));
        return NULL;
    }
    AUDIO_INFO_PRINT (("AUDIO: DEC -> Rpe_create(AAC_DEC) done....\n"));

    status = Rpe_acquireCallDescriptor (ctx->decClientHandle, 
                                        RPE_CALL_DESC_CONTROL,
                                        &ctx->decControlCallDesc,
                                        &ctx->cmdId, 
                                        &ctx->decDynParams,
                                        &ctx->decStatus);
    if (RPE_S_SUCCESS != status)
    {
        AUDIO_ERROR_PRINT (("AUDIO: DEC -> Rpe_acquireCallDescriptor failed, status: %d\n", status));
        Rpe_delete(ctx->decClientHandle);
        return NULL;
    }
    /*----------------------------------------------------------------------*/
    /* Call control api using XDM_GETBUFINFO to get I/O buffer requirements */
    /*----------------------------------------------------------------------*/

    *(ctx->cmdId) = XDM_GETBUFINFO;
    ctx->decDynParams->size = sizeof(IAUDDEC1_DynamicParams);
    ctx->decStatus->size    = sizeof(IAUDDEC1_Status);

    status = Rpe_control (ctx->decControlCallDesc);
    if (RPE_S_SUCCESS != status)
    {
        AUDIO_ERROR_PRINT (("AUDIO: DEC -> Rpe control call XDM_GETBUFINFO failed, status: %d\n", status));
        Rpe_delete(ctx->decClientHandle);
        return NULL;
    }
    
    status = Rpe_acquireCallDescriptor (ctx->decClientHandle, 
                                        RPE_CALL_DESC_PROCESS,
                                        &ctx->decProcessCallDesc,
                                        &ctx->inBufDesc, 
                                        &ctx->outBufDesc,
                                        &ctx->decInArgs, 
                                        &ctx->decOutArgs);
    if (RPE_S_SUCCESS != status)
    {
        AUDIO_ERROR_PRINT (("AUDIO: DEC -> Rpe_acquireCallDescriptor failed, status: %d\n", status));
        Rpe_delete(ctx->decClientHandle);
        return NULL;
    }
                                         
    ctx->inBufDesc->numBufs = ctx->decStatus->bufInfo.minNumInBufs;
    ctx->decMinInputBufSize = 
        ctx->inBufDesc->descs[0].bufSize = ctx->decStatus->bufInfo.minInBufSize[0];
    ctx->outBufDesc->numBufs = ctx->decStatus->bufInfo.minNumOutBufs;
    ctx->decMinOutputBufSize = 
        ctx->outBufDesc->descs[0].bufSize = ctx->decStatus->bufInfo.minOutBufSize[0];

    *(ctx->cmdId) = XDM_SETDEFAULT;
    ctx->decDynParams->size = sizeof(IAUDDEC1_DynamicParams);
    ctx->decStatus->size    = sizeof(IAUDDEC1_Status);

    status = Rpe_control (ctx->decControlCallDesc);
    if (RPE_S_SUCCESS != status)
    {
        AUDIO_ERROR_PRINT (("AUDIO: DEC -> Rpe control call XDM_SETDEFAULT failed, status: %d\n", status));
        Rpe_delete(ctx->decClientHandle);
        return NULL;
    }

    /*-------------------------------------------------------------------*/
    /* Call control api using XDM_SETPARAMS to set run-time parameters   */
    /*-------------------------------------------------------------------*/

    *(ctx->cmdId) = XDM_SETPARAMS;
    ctx->decDynParams->size = sizeof(IAUDDEC1_DynamicParams);
    ctx->decStatus->size    = sizeof(IAUDDEC1_Status);
    ctx->decDynParams->downSampleSbrFlag = 0;
    
    status = Rpe_control (ctx->decControlCallDesc);
    if (RPE_S_SUCCESS != status)
    {
        AUDIO_ERROR_PRINT (("AUDIO: DEC -> Rpe control call XDM_SETPARAMS failed, status: %d\n", status));
        Rpe_delete(ctx->decClientHandle);
        return NULL;
    }

    ctx->decInArgs->size    = sizeof(IAUDDEC1_InArgs);
    ctx->decInArgs->numBytes = 0;
    ctx->decInArgs->lfeFlag = 0;
    ctx->decInArgs->desiredChannelMode = pPrm->desiredChannelMode;

    ctx->decOutArgs->size   = sizeof(IAUDDEC1_OutArgs);
    ctx->decOutArgs->bytesConsumed = 0;
    ctx->decOutArgs->extendedError = XDM_EOK;

    pPrm->minInBufSize = ctx->decMinInputBufSize;
    pPrm->minOutBufSize = ctx->decMinOutputBufSize;

    return ctxMem;
}


Void *Audio_createG711DecAlgorithm(Void *ctxMem)
{
    decContext           *handle;

    handle = ctxMem;
    if (handle == NULL)
    {
        AUDIO_ERROR_PRINT (("AUDIO: DEC -> Dec creation <malloc> failed...\n"));
        return NULL;
    }
    handle->codecType = AUDIO_CODEC_TYPE_G711;
    return handle;
}

Int32 Audio_decodeAac (Void* ctxMem, Audio_DecodeProcessParams *pPrm)
{
    Int32          status = -1;                 
    decContextDsp  *ctx;
    decContext     *handle;
    
#if !defined(DSP_RPE_AUDIO_ENABLE)
    return 0;
#endif
    
    /* Allocated for both external / internal data structure */
    handle = ctxMem;

    if (handle)
    {
        ctx = GET_DEC_INT_STRUCT(handle);

        ctx->inBufDesc->descs[0].buf = (XDAS_Int8*)pPrm->inBuf.dataBuf;
        ctx->outBufDesc->descs[0].buf = (XDAS_Int8*)pPrm->outBuf.dataBuf;
        if (pPrm->inBuf.dataBufSize >= ctx->decMinInputBufSize)
        {
             pPrm->inBuf.dataBufSize = ctx->decMinInputBufSize;
        }
        else
        {
            pPrm->numSamples = 0;
            pPrm->outBuf.dataBufSize = 0;
            pPrm->inBuf.dataBufSize = 0;
            return 0;
        }

        ctx->inBufDesc->descs[0].bufSize = pPrm->inBuf.dataBufSize;
        ctx->decInArgs->numBytes = ctx->inBufDesc->descs[0].bufSize;
        ctx->outBufDesc->descs[0].bufSize = pPrm->outBuf.dataBufSize;

        pPrm->outBuf.dataBufSize = 0;

        status = Rpe_process (ctx->decProcessCallDesc);
        if (RPE_S_SUCCESS != status)
        {
            AUDIO_ERROR_PRINT (("AUDIO: DEC -> AUDIO: Rpe process call failed, status: %d\n", status));
            return -1;
        }
        
        if (ctx->decOutArgs->extendedError)
        {
           AUDIO_ERROR_PRINT (("AUDIO: DEC -> AAC decoder failed error %0x to decode frame %d \n", 
                    (UInt32) ctx->decOutArgs->extendedError, ctx->decFrameCnt));
                    
            if (XDM_ISFATALERROR (ctx->decOutArgs->extendedError))
            {
                *(ctx->cmdId) = XDM_RESET;
                status = Rpe_control (ctx->decControlCallDesc);
                AUDIO_ERROR_PRINT (("AUDIO: DEC -> AAC decoder Rpe_control for control status: %d\n", status));
                return -1;
            }
        }

        *(ctx->cmdId) = XDM_GETSTATUS;
        status = Rpe_control (ctx->decControlCallDesc);    
        if (RPE_S_SUCCESS != status)
        {
            AUDIO_ERROR_PRINT (("AUDIO: DEC -> AAC decoder Rpe control call XDM_GETSTATUS failed, status: %d\n", status));
            return -1;
        }            
        if (ctx->decStatus->pcmFormat == IAUDIO_1_0)
            pPrm->outBuf.dataBufSize = ctx->decStatus->numSamples;
        else
            pPrm->outBuf.dataBufSize = ctx->decStatus->numSamples * 2; /* others not handled */

        pPrm->inBuf.dataBufSize = ctx->decOutArgs->bytesConsumed;
        ctx->decFrameCnt++;
        
        pPrm->pcmFormat = ctx->decStatus->pcmFormat;
        pPrm->channelMode = ctx->decStatus->channelMode;
        pPrm->numSamples = ctx->decStatus->numSamples;
        pPrm->bytesPerSample = (ctx->decStatus->outputBitsPerSample >> 3);
    }
    return status;
}

Int32 Audio_decode(Void* ctxMem, Audio_DecodeProcessParams *pPrm)
{
    decContext *handle = ctxMem;
    if (handle)
    {
        switch (handle->codecType)
        {
            case AUDIO_CODEC_TYPE_AAC_LC:
                return Audio_decodeAac(ctxMem, pPrm);
            
            case AUDIO_CODEC_TYPE_G711:
                return Audio_decodeULaw(ctxMem, pPrm);
                return 0;
        }
    }
    return 0;
}


Int32 Audio_deleteEncAlgorithm(Void *ctxMem)
{
    Int32 status = -1;
    decContext *handle;

    handle = ctxMem;
    if (handle != NULL)
    {
        if (handle->codecType == AUDIO_CODEC_TYPE_AAC_LC)
        {
            encContextDsp *ctx = GET_ENC_INT_STRUCT(handle);
            status = Rpe_delete(ctx->encClientHandle);
        }
        else
        {
            status = 0;
        }
    }    
    return status;
}

Int32 Audio_deleteDecAlgorithm(Void *ctxMem)
{
    Int32 status = -1;
    decContext *handle;

    handle = ctxMem;
    if (handle != NULL)
    {
        if (handle->codecType == AUDIO_CODEC_TYPE_AAC_LC)
        {
            decContextDsp *ctx = GET_DEC_INT_STRUCT(handle);
            status = Rpe_delete(ctx->decClientHandle);
        }
        else
        {
            status = 0;
        }
    }    
    return status;
}


static Int32 Audio_encodeAsULaw (Void* ctxMem, Audio_EncodeProcessParams *pPrm)
{
    encContext     *handle;
    UInt16   i;
    Int16    data;
    UInt16   isNegative;
    Int16    nOut;
    Int16    lowByte = 1;
    Int32    outputSize = 0;
    Int16    *src, *dst;
   
    handle = ctxMem;

    if (handle && pPrm)
    {
        if (pPrm->inBuf.dataBufSize > 0)
        {    
            outputSize = pPrm->inBuf.dataBufSize / 2;
            src = pPrm->inBuf.dataBuf;
            dst = pPrm->outBuf.dataBuf;
            if (pPrm->outBuf.dataBufSize < outputSize)
                outputSize = pPrm->outBuf.dataBufSize;

            for (i = 0; i < outputSize; i++)
            {
                data = *(src + i);
                data >>= 2;
                isNegative = (data < 0 ? 1 : 0);

                if (isNegative)
                    data = -data;

                if (data <= 1) {
                    nOut = (char) data;
                } else if (data <= 31) {
                    nOut = ((data - 1) >> 1) + 1;
                } else if (data <= 95) {
                    nOut = ((data - 31) >> 2) + 16;
                } else if (data <= 223) {
                    nOut = ((data - 95) >> 3) + 32;
                } else if (data <= 479) {
                    nOut = ((data - 223) >> 4) + 48;
                } else if (data <= 991) {
                    nOut = ((data - 479) >> 5) + 64;
                } else if (data <= 2015) {
                    nOut = ((data - 991) >> 6) + 80;
                } else if (data <= 4063) {
                    nOut = ((data - 2015) >> 7) + 96;
                } else if (data <= 7903) {
                    nOut = ((data - 4063) >> 8) + 112;
                } else {
                    nOut = 127;
                }
         
                if (isNegative) {
                    nOut = 127 - nOut;
                } else {
                    nOut = 255 - nOut;
                }

                // Pack the bytes in a word
                if (lowByte)
                    *(dst + (i >> 1)) = (nOut & 0x00FF);
                else
                    *(dst + (i >> 1)) |= ((nOut << 8) & 0xFF00);
                lowByte ^= 0x1;
            }
        }
    }
    pPrm->outBuf.dataBufSize = outputSize;
    /* set bytes consumed */
    pPrm->inBuf.dataBufSize = (outputSize * 2);
    return 0;
}

static Int16 Audio_decodeULawOneSample(UInt16 input) 
{
    UInt16 isNegative;
    Int16 nOut;

    isNegative = ((input & 0x80) == 0);
    if (isNegative) 
      nOut= 127 - input;
    else 
      nOut= 255 - input;

    if (nOut < 2)
      nOut *= 2;
    else if (nOut < 16) 
      nOut= ((nOut - 1) << 1) +1 + 1;
    else if (nOut < 32) 
      nOut= ((nOut - 16) << 2) +2 + 31;
    else if (nOut < 48) 
      nOut= ((nOut - 32) << 3) +4 + 95;
    else if (nOut < 64) 
      nOut= ((nOut - 48) << 4) +8 + 223;
    else if (nOut < 80) 
      nOut= ((nOut - 64) << 5) +16 + 479;
    else if (nOut < 96) 
      nOut= ((nOut - 80) << 6) +32 + 991;
    else if (nOut < 112) 
      nOut= ((nOut - 96) << 7) +64 + 2015;
    else 
      nOut= ((nOut - 112) << 8) +128 + 4063;
    if (isNegative)
      nOut= - nOut;
    nOut <<= 2;
    return nOut;
}

Int32 Audio_decodeULaw(Void* ctxMem, Audio_DecodeProcessParams *pPrm)
{
    if (ctxMem && pPrm)
    {
        Int8    *pOutdata, *pIndata;
        Int32  sizeConsumed = 0, i = 0 ;

        sizeConsumed = pPrm->inBuf.dataBufSize;
        if (pPrm->outBuf.dataBufSize < (sizeConsumed * 2))
        {
            sizeConsumed = pPrm->outBuf.dataBufSize / 2;
        }

        pOutdata = pPrm->outBuf.dataBuf;
        pIndata = pPrm->inBuf.dataBuf;

    	for( i = 0; i < sizeConsumed ; i++)
        {
            *((UInt16*)(pOutdata) + i) = Audio_decodeULawOneSample(*((UInt8 *)(pIndata) + i ));
        }
        pPrm->outBuf.dataBufSize = sizeConsumed * 2;
        /* set bytes consumed */
        pPrm->inBuf.dataBufSize = sizeConsumed;
        pPrm->numSamples = pPrm->outBuf.dataBufSize;
        pPrm->bytesPerSample = 1;
        pPrm->channelMode = 0;
    }
    return 0;
}

