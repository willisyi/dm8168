/*******************************************************************************
 *                                                                            
 * Copyright (c) 2012 Texas Instruments Incorporated - http://www.ti.com/      
 *                        ALL RIGHTS RESERVED                                  
 *                                                                            
 ******************************************************************************/

/**
    \ingroup MCFW_API
    \defgroup MCFW_ACAP_API McFW Audio API

    Audio Capture, Encode, Decode APIs using RPE - Remote Processor Execute.
    RPE is used to utilize DSP codecs. 
    Audio capture with tvp5158 can be started for configurable number of audio channels, 
    captured data can be received in raw / encoded format.
    Supports AAC / G711 encode / decode now. 
    ACAP APIs does capture & optional encode
    AENC APIs do stand alone encoding of supplied PCM from application
    AENC APIs do decoding of compressed audio input provided from application

    @{
*/

/**
    \file ti_audio.h
    \brief McFW Audio Capture, Encode, Decode API
*/

#ifndef __TI_AUDIO_H__
#define __TI_AUDIO_H__


#ifdef __cplusplus
extern "C" {
#endif

/* Include's */
#include <mcfw/interfaces/ti_media_std.h>
#include <link_api/audioLink.h>

/* Define's */

/* @{ */

#define ALSA_CAPTURE_DEVICE     "plughw:0,0"

#define     ACAP_VOLUME_MAX        8  /**< Max tvp5158 volume */

/* @} */


/* Data structure's */
/* @{ */

/**
    \brief Encoder Handle
*/
typedef void *AENC_HANDLE;
/**
    \brief Decoder Handle
*/
typedef void *ADEC_HANDLE;

/**
    \brief Audio Link - Buffer Parameters 
*/
typedef struct {

        Void  *dataBuf;  
        /**< Data buffer. Should be >= alg's expected min input buffer size, else codec API will fail.
                     Can be set just once; data would be copied to this same data pointer every time in this case.
                     If different buffers are used, AXXX_setBufInfo() should be invoked before every AXXX_process() API.
                     Should be in shared region if DSP encode / decode is used.
                 */
        UInt32 dataBufSize;  
        /**< Size of dataBuf */
} AUDIO_BUFFER_PARAMS_S;


/**
    \brief Audio Link - Encode Create Parameters
*/
typedef struct {

    Int32        encoderType;        /**< CodecType - AUDIO_ENC_TYPE_AAC_LC / G711 */
    Int32        bitRate;            /**< Bitrate of encoded stream */
    Int32        sampleRate;         /**< Sample rate of encoded stream */
    Int32        numberOfChannels;   /**< Number of audio channels.  mono=1, stereo=2 */

    Int32        minInBufSize;      /**< Min input buffer size requirement - returned by encoder */ 
    Int32        minOutBufSize;     /**< Min output buffer size requirement - returned by encoder */ 
} AENC_CREATE_PARAMS_S;


/**
    \brief Audio Link - Encode Process Parameters
*/
typedef struct {

    AUDIO_BUFFER_PARAMS_S   inBuf;        /**< Input Buffer details. dataBufSize tells the input number of bytes */
    AUDIO_BUFFER_PARAMS_S   outBuf;       /**< Output Buffer details. dataBufSize tells the max output buffer size */

} AENC_PROCESS_PARAMS_S;


/**
    \brief Audio Link - Audio capture channel parameters 
*/
typedef struct {

    AUDIO_BUFFER_PARAMS_S    captureBuf;  /**< Buffer to store captured audio. If encode is enabled, this will serve as encode input */
    Bool                     enableEncode;/**< Flag to enable encoding of captured audio */
    AENC_CREATE_PARAMS_S     encodeParam; /**< Encoder create params */
    AUDIO_BUFFER_PARAMS_S    encodeBuf;   /**< Buffer to store encoded data */ 

} ACAP_CHANNEL_PARAMS_S;


/**
    \brief Audio Capture Params
*/
typedef struct
{
    Int8                  captureDevice[AUDIO_MAX_STRING_SIZE];     /**< Alsa capture device name */
    UInt32                numChannels;              /**< Num of audio channels to capture */
    UInt32                audioVolume;              /**< Audio volume */
    UInt32                sampleRate;               /**< Capture Sampling Frequency in Hz */
    ACAP_CHANNEL_PARAMS_S chPrm[ACAP_CHANNELS_MAX]; /**< Channel parameters */
    Bool                  enableTVP5158;            /**< Set TVP 5158 capture */
    Int32                 sampleLen;                /**< Bytes / sample */

} ACAP_PARAMS_S;


/**
    \brief Audio Link - Audio capture data parameters for a channel
*/
typedef struct {

        Void   *captureDataBuf;   /**< Pointer to captured audio is returned */
        UInt32  captureDataSize;  /**< captured data size  */
        Void   *encodeDataBuf;    /**< Pointer to encoded audio is returned */
        UInt32  encodeDataSize;   /**< encoded data size */

} ACAP_GET_DATA_PARAMS_S;


/**
    \brief Audio Link - Audio Decode create parameters
*/
typedef struct {

    Int32        decoderType;         /**< CodecType - AUDIO_ENC_TYPE_AAC_LC / G711 */
    Int32        desiredChannelMode;  /**< Channel mode - mono, stereo */
    Int32        minInBufSize;        /**< Min input buffer size requirement - returned by encoder */ 
    Int32        minOutBufSize;       /**< Min output buffer size requirement - returned by encoder */ 

} ADEC_CREATE_PARAMS_S;


/**
    \brief Audio Link - Audio Decode process parameters
*/
typedef struct {

    AUDIO_BUFFER_PARAMS_S   inBuf;        /**< Input Buffer details. dataBufSize tells the input number of bytes */
    AUDIO_BUFFER_PARAMS_S   outBuf;       /**< Output Buffer details. dataBufSize tells the max output buffer size */
    Int32                   numSamples;    /**< Samples generated */
    Int32                   channelMode;  /**< Output channel mode - same as IAUDIO_ChannelMode */
    Int32                   pcmFormat;     /**< Output PCM Format - Block/Interleaved, same as IAUDIO_PcmFormat*/
    Int32                   bytesPerSample; /**< Bytes per sample */
} ADEC_PROCESS_PARAMS_S;

/* @} */


/* function's */

/**
    \brief     Initialize audio sub system
    
    \return   ERROR_NONE on success
*/
Int32 Audio_systemInit (Void);

/**
    \brief     De-init audio sub system
    
    \return   ERROR_NONE on success
*/
Int32 Audio_systemDeInit (Void);


/**
    \brief     Initialize audio capture params
    
    \param  pContext    [OUT] Default settings

    \return   ERROR_NONE on success
*/
Int32 Acap_params_init(ACAP_PARAMS_S * pContext);


/**
    \brief    Init audio capture

    MUST be called after video capture is started.
    A capture thread starts capturing audio data for all channels & store in buffers provided by app.
    Once audio capture starts, audio data for individual channel can be retrieved separately

    \param  pContext    [IN] Capture configuration

    \return   ERROR_NONE on success
*/
Int32 Acap_init(ACAP_PARAMS_S * pContext);


/**
    \brief  De-init Audio system 

    \return ERROR_NONE on success
*/
Int32 Acap_deInit();


/**
    \brief Dynamic change of audio capture parameters

    \param  chId      [IN] Channel number 
    \param  volume [IN] Audio volume 

    \return ERROR_NONE on success
*/
Int32 Acap_setChannelVolume(UInt32 chId, UInt32 volume);


/**
    \brief Dynamic change of audio capture parameters

    \param  chId      [IN] Channel number 
    \param  sampleRate   [IN] Sample Frequence in Hz

    \return ERROR_NONE on success
*/
Int32 Acap_setChannelSampleRate(UInt32 chId, UInt32 sampleRate);


/**
    \brief Enable specific audio channel capture

    \param  chNum  [IN] Channel number

    \return ERROR_NONE on success
*/
Int32 Acap_startChannel(UInt8 chNum);


/**
    \brief Disable a particular audio channel capture

    \param  chNum  [IN] Channel number

    \return ERROR_NONE on success
*/
Int32 Acap_stopChannel(UInt8 chNum);


/**
    \brief Retrieve an audio channel data

     App should invoke this API at proper rate to avoid capture data loss.
     App should consume captureData buffer at real time [if app consumes raw data] &
     return back the captureData chunk buffer to system, otherwise data loss will occur

    \param  chNum                     [IN]             Channel number
    \param  pPrm                        [OUT]         Pointer to Raw Audio data and/or encoded data

    \return ERROR_NONE on success
*/
Int32 Acap_getData(UInt8 chNum, ACAP_GET_DATA_PARAMS_S *pPrm);


/**
    \brief Return back the raw capture data buffer / encoded buffer got via an earlier call to Acap_getData()

     App should invoke this API at proper rate to avoid capture data loss. 

    NOT IMPLEMENTED.

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
                                 );

/**
    \brief Start capture of all channels

    \return ERROR_NONE on success
*/
Int32 Acap_start();


/**
    \brief Stop capture of all channels

    \return ERROR_NONE on success
*/
Int32 Acap_stop();



/**
    \brief Create Encoder Algorithm
             Encoder creation will be done internally if encode is enabled during capture init.
             Encoder APIs can be invoked independently by application.

    \param  pPrm             [IN]   Encode parameters like encodeType, bitrate, sample rate, num of channels etc

    \return Encoder Handle on success, NULL otherwise
*/
AENC_HANDLE Aenc_create(AENC_CREATE_PARAMS_S *pPrm);


/**
    \brief Encode process call

    \param  handle                      [IN]          Encoder handle
    \param  pPrm                        [IN,OUT]   Input buffer / samples, Output buffer / max out buffer size details. 
                                                                  Returns data in outbuffer .

    \return number Of bytes generated [encoded] on success, 0 on failure
*/
Int32 Aenc_process(AENC_HANDLE handle, AENC_PROCESS_PARAMS_S *pPrm);


/**
    \brief Delete encoder algorithm

    \param  handle                      [IN]   Encoder Handle

    \return 0 on success, negative value otherwise
*/
Int32 Aenc_delete(AENC_HANDLE handle);



/************************* ti_adec.h ************************/

/**
    \brief Create Decode Algorithm

    \param  pPrm               [IN]   Decoder create param 

    \return Decoder Handle on success, NULL otherwise
*/
ADEC_HANDLE Adec_create(ADEC_CREATE_PARAMS_S *pPrm);


/**
    \brief Audio Decode process call
    \param  handle                      [IN]   Decoder handle
    \param  pPrm                        [IN,OUT]   Input buffer / samples, Output buffer / max out buffer size details. 
                                                                  Returns data in outbuffer.

    \return number Of samples generated [decoded] on success, 0 on failure
*/
Int32 Adec_process(ADEC_HANDLE handle, ADEC_PROCESS_PARAMS_S *pPrm);


/**
    \brief Delete decoder algorithm

    \param  handle                      [IN]   Decoder handle

    \return 0 on success, negative value otherwise
*/
Int32 Adec_delete(ADEC_HANDLE handle);


#ifdef  __cplusplus
}
#endif
#endif /* __TI_AUDIO_H__ */

/*@}*/

