/*******************************************************************************
 *                                                                            
 * Copyright (c) 2012 Texas Instruments Incorporated - http://www.ti.com/      
 *                        ALL RIGHTS RESERVED                                  
 *                                                                            
 ******************************************************************************/

#include "ti_audio_priv.h"
#include "ti_vcap.h"

/* =============================================================================
 * Globals
 * =============================================================================
 */

ACAP_MODULE_CONTEXT_S gAcapModuleContext;


/**
    \brief     Initialize audio sub system
    
    \param  None

    \return   ERROR_NONE on success
*/
Int32 Audio_systemInit (Void)
{
    if (gAcapModuleContext.rpeInitialized == FALSE)
    {
//        AUDIO_INFO_PRINT(("\nAUDIO: Audio_systemInit()....\n"));
        Audio_systemProcInit();
        gAcapModuleContext.rpeInitialized = TRUE;
    }
//    Audio_printMemStats();
    return ERROR_NONE;
}

/**
    \brief     De-init audio sub system
    
    \param  None

    \return   ERROR_NONE on success
*/
Int32 Audio_systemDeInit (Void)
{
    if (gAcapModuleContext.rpeInitialized == TRUE)
    {
        AUDIO_INFO_PRINT(("\nAUDIO: Audio_systemProcDeInit()....\n"));
        Audio_systemProcDeInit();
        gAcapModuleContext.rpeInitialized = FALSE;
    }
//    Audio_printMemStats();
    return ERROR_NONE;
}

/**
    \brief     Initialize audio capture params
    
    \param  pContext    [OUT] Default settings

    \return   ERROR_NONE on success
*/
Int32 Acap_params_init(ACAP_PARAMS_S * pContext)
{
    if (pContext)
    {
        Int32 i;

        strcpy(pContext->captureDevice, ALSA_CAPTURE_DEVICE);
        pContext->enableTVP5158 = TRUE;
        pContext->numChannels = ACAP_CHANNELS_MAX;
        pContext->audioVolume = ACAP_VOLUME_MAX;
        pContext->sampleRate = 16000;
        pContext->sampleLen = Audio_getSampleLenInBytes();
        for (i=0; i<ACAP_CHANNELS_MAX; i++)
        {
            pContext->chPrm[i].enableEncode = FALSE;

            pContext->chPrm[i].captureBuf.dataBuf = NULL;
            pContext->chPrm[i].captureBuf.dataBufSize = Audio_getMinCaptureChBufSize();

            pContext->chPrm[i].encodeBuf.dataBuf = NULL;
            pContext->chPrm[i].encodeBuf.dataBufSize = Audio_getMinCaptureChBufSize() / 2;

            pContext->chPrm[i].encodeParam.bitRate = 32000;
            pContext->chPrm[i].encodeParam.encoderType = AUDIO_CODEC_TYPE_AAC_LC;
            pContext->chPrm[i].encodeParam.numberOfChannels = 1;
            pContext->chPrm[i].encodeParam.sampleRate = 16000;
        }    
        return ERROR_NONE;
    }
    return ERROR_FAIL;
}


/**
    \brief    Init audio capture

    MUST be called after video capture is started.
    A capture thread starts capturing audio data for all channels & store in buffers provided by app.
    Once audio capture starts, audio data for individual channel can be retrieved separately

    \param  pContext    [IN] Capture configuration

    \return   ERROR_NONE on success
*/
Int32 Acap_init(ACAP_PARAMS_S * pContext)
{
    if (pContext)
    {
        if (pContext->numChannels > ACAP_CHANNELS_MAX)
        {
            AUDIO_INFO_PRINT(("AUDIO: Audio channels not supported !!!!\n"));
        }
        memcpy(&gAcapModuleContext.acapParams, pContext, sizeof(ACAP_PARAMS_S));
    
        if (pContext->enableTVP5158)
            Vcap_setAudioModeParam(pContext->numChannels, pContext->sampleRate, pContext->audioVolume);
        return Audio_startCapture((Audio_CaptureParams *)&gAcapModuleContext.acapParams);
    }
    return ERROR_NONE;
}


/**
    \brief  De-init Audio system 

    \param None

    \return ERROR_NONE on success
*/
Int32 Acap_deInit()
{
    Audio_stopCapture();
    return ERROR_NONE;
}


/**
    \brief Dynamic change of audio capture parameters

    \param  samplingHz   [IN] Sample Frequence in Hz

    \return ERROR_NONE on success
*/
Int32 Acap_setChannelVolume(UInt32 chId, UInt32 volume)
{
    return ERROR_NONE;
}


/**
    \brief Dynamic change of audio capture parameters

    \param  audioVolume [IN] Audio volume -

    \return ERROR_NONE on success
*/
Int32 Acap_setChannelSampleRate(UInt32 chId, UInt32 sampleRate)
{
    return ERROR_NONE;
}


/**
    \brief Enable specific audio channel capture

    \param  chNum  [IN] Channel number

    \return ERROR_NONE on success
*/
Int32 Acap_startChannel(UInt8 chNum)
{
    Audio_enableCapChannel(chNum);
    return ERROR_NONE;
}



/**
    \brief Disable a particular audio channel capture

    \param  chNum  [IN] Channel number

    \return ERROR_NONE on success
*/
Int32 Acap_stopChannel(UInt8 chNum)
{
    Audio_disableCapChannel(chNum);
    return ERROR_NONE;
}



/**
    \brief Retrieve an audio channel data

     App should invoke this API at proper rate to avoid capture data loss.
     App should consume captureData buffer at real time <if app consumes raw data> &
     return back the captureData chunk buffer to system, otherwise data loss will occur

    \param  chNum                     [IN]             Channel number
    \param  pPrm                        [OUT]         Pointer to Raw Audio data and/or encoded data

    \return ERROR_NONE on success
*/
Int32 Acap_getData(UInt8 chNum, ACAP_GET_DATA_PARAMS_S *pPrm)
{
    if (pPrm)
        return Audio_getCapChData(chNum, (Audio_getDataParams*)pPrm);
    else
        return ERROR_FAIL;
}


/**
    \brief Return back the raw capture data buffer / encoded buffer got via an earlier call to Acap_getData()

     App should invoke this API at proper rate to avoid capture data loss 
    
    \param  chNum                                     [IN]           Channel number
    \param  captureDataSizeConsumed       [IN]           Data size in bytes consumed by App.
                                                                                  This should be <= captureDataSize returned from Acap_getData()
    \param  encodeDataSizeConsumed       [IN]           Data size in bytes consumed by App.
                                                                                  This should be <= encodeDataSize returned from Acap_getData()

    \return ERROR_NONE on success
*/
Int32 Acap_setConsumedData(UInt8 chNum,
                           UInt32 captureDataSizeConsumed,
                           UInt32 encodeDataSizeConsumed
                                 )
{
    return Audio_setCapConsumedChData(chNum, captureDataSizeConsumed, encodeDataSizeConsumed);
}


/**
    \brief Start capture of all channels

    \param  None

    \return ERROR_NONE on success
*/
Int32 Acap_start()
{
    Int32 i;
    for (i=0; i<gAcapModuleContext.acapParams.numChannels; i++)
        Audio_enableCapChannel(i);
//    Audio_printMemStats();
    return ERROR_NONE;
}


/**
    \brief Stop capture of all channels

    \param  None

    \return ERROR_NONE on success
*/
Int32 Acap_stop()
{
    Int32 i;
    for (i=0; i<ACAP_CHANNELS_MAX; i++)
        Audio_disableCapChannel(i);

//    Audio_printMemStats();
    return ERROR_NONE;
}



/**
    \brief Create Encoder Algorithm
             Encoder creation will be done internally if encode is enabled during capture init.
             Encoder APIs can be invoked independently by application.

    \param  pPrm             [IN]   Encode parameters like encodeType, bitrate, sample rate, num of channels etc

    \return Encoder Handle on success, NULL otherwise
*/
AENC_HANDLE Aenc_create(AENC_CREATE_PARAMS_S *pPrm)
{
    Void *handle = NULL;

    if (pPrm)
    {
        switch (pPrm->encoderType)
        {
            case AUDIO_CODEC_TYPE_AAC_LC:
            {
                handle = Audio_allocMem(Audio_getEncoderContextSize(pPrm->encoderType));
                if (Audio_createAacEncAlgorithm(handle, (Audio_EncodeCreateParams *)pPrm) == NULL)
                {
                    Audio_freeMem(handle);
                    return NULL;
                }
            }
            break;
        
            case AUDIO_CODEC_TYPE_G711:
            {
                handle = Audio_allocMem(Audio_getEncoderContextSize(pPrm->encoderType));
                if (Audio_createG711EncAlgorithm(handle) == NULL)
                {
                    Audio_freeMem(handle);
                    return NULL;
                }
            }
        }
    }
    return handle;
}


/**
    \brief Encode process call

    \param  handle                      [IN]          Encoder handle
    \param  pPrm                        [IN,OUT]   Input buffer / samples, Output buffer / max out buffer size details. 
                                                                  Returns data in outbuffer .

    \return number Of bytes generated <encoded> on success, 0 on failure
*/
Int32 Aenc_process(AENC_HANDLE handle, AENC_PROCESS_PARAMS_S *pPrm)
{
    return Audio_encode(handle, (Audio_EncodeProcessParams*)pPrm);
}



/**
    \brief Delete encoder algorithm

    \param  handle                      [IN]   Encoder Handle

    \return 0 on success, negative value otherwise
*/
Int32 Aenc_delete(AENC_HANDLE handle)
{
    Int32 status = ERROR_FAIL;

    if (handle)
    {
        status = Audio_deleteEncAlgorithm(handle);
        Audio_freeMem(handle);
    }
    return status;
}




/************************* ti_adec.h ************************/

/**
    \brief Create Decode Algorithm

    \param  pPrm               [IN]   Decoder create param 

    \return Decoder Handle on success, NULL otherwise
*/
ADEC_HANDLE Adec_create(ADEC_CREATE_PARAMS_S *pPrm)
{
    Void *handle = NULL;

    if (pPrm)
    {
        switch (pPrm->decoderType)
        {
            case AUDIO_CODEC_TYPE_AAC_LC:
            {
                handle = Audio_allocMem(Audio_getDecoderContextSize(pPrm->decoderType));
                if (Audio_createAacDecAlgorithm(handle, (Audio_DecodeCreateParams*)pPrm) == NULL)
                {
                    Audio_freeMem(handle);
                    return NULL;
                }
            }
            break;
        
            case AUDIO_CODEC_TYPE_G711:
            {
                handle = Audio_allocMem(Audio_getDecoderContextSize(pPrm->decoderType));
                if (Audio_createG711DecAlgorithm(handle) == NULL)
                {
                    Audio_freeMem(handle);
                    return NULL;
                }
            }
        }
    }
    return handle;
}





/**
    \brief Audio Decode process call
    \param  handle                      [IN]   Decoder handle
    \param  pPrm                        [IN,OUT]   Input buffer / samples, Output buffer / max out buffer size details. 
                                                                  Returns data in outbuffer.

    \return number Of samples generated <decoded> on success, 0 on failure
*/
Int32 Adec_process(ADEC_HANDLE handle, ADEC_PROCESS_PARAMS_S *pPrm)
{
    return Audio_decode(handle, (Audio_DecodeProcessParams*)pPrm);
}



/**
    \brief Delete decoder algorithm

    \param  handle                      [IN]   Decoder handle

    \return 0 on success, negative value otherwise
*/
Int32 Adec_delete(ADEC_HANDLE handle)
{
    Int32 status = ERROR_FAIL;

    if (handle)
    {
        status = Audio_deleteDecAlgorithm(handle);
        Audio_freeMem(handle);
    }
    return status;
}



