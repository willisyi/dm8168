/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2012 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
    \ingroup LINK_API
    \defgroup AUDIO_LINK_API Audio Link API

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
    \file audioLink.h
    \brief Audio Link API
*/

#ifndef __AUDIO_LINK_H__
#define __AUDIO_LINK_H__

#ifdef  __cplusplus
extern "C" {
#endif

/* Include's */
#include <mcfw/interfaces/link_api/system_debug.h>

/* Define's */

/* @{ */

/**
    \brief Enable high level prints related to audio 
*/
#define     AUDIO_ENABLE_INFO_PRINT

/**
    \brief Enable error prints related to audio 
*/
#define     AUDIO_ENABLE_ERROR_PRINT

#ifdef      AUDIO_ENABLE_INFO_PRINT
#define     AUDIO_INFO_PRINT(X)    printf X
#else
#define     AUDIO_INFO_PRINT(X)    
#endif

#ifdef      AUDIO_ENABLE_ERROR_PRINT
#define     AUDIO_ERROR_PRINT(X)    printf X
#else
#define     AUDIO_ERROR_PRINT(X)    
#endif

/**
    \brief Maximum capture channels
*/
#ifdef TI_816X_BUILD
#define     ACAP_CHANNELS_MAX      16 /**< Max capture channels */
#else
#define     ACAP_CHANNELS_MAX       4 /**< Max capture channels */
#endif

/**
    \brief Maximum string size
*/
#define     AUDIO_MAX_STRING_SIZE  256

/* @} */



/* Enum's */

/* @{ */

/**
    \brief Audio Encoder / Decoder types supported
*/
typedef enum
{
    AUDIO_CODEC_TYPE_G711, /**< G711 encode or decode */
    AUDIO_CODEC_TYPE_AAC_LC, /**< AAC-LC encode or decode - DSP Based */
    AUDIO_CODEC_TYPE_MAX /**< Max supported encode, decode options */

} AUDIO_CODEC_TYPE;

/* @} */


/* Data structure's */
/* @{ */

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
} Audio_BufferParams;


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
} Audio_EncodeCreateParams;


/**
    \brief Audio Link - Encode Process Parameters
*/
typedef struct {

    Audio_BufferParams   inBuf;        /**< Input Buffer details. dataBufSize tells the input number of bytes */
    Audio_BufferParams   outBuf;       /**< Output Buffer details. dataBufSize tells the max output buffer size */

} Audio_EncodeProcessParams;


/**
    \brief Audio Link - Audio capture channel parameters 
*/
typedef struct {

    Audio_BufferParams          captureBuf;  /**< Buffer to store captured audio. If encode is enabled, this will serve as encode input */
    Bool                        enableEncode;/**< Flag to enable encoding of captured audio */
    Audio_EncodeCreateParams    encodeParam; /**< Encoder create params */
    Audio_BufferParams          encodeBuf;   /**< Buffer to store encoded data */ 

} Audio_CaptureChParams;


/**
    \brief Audio Capture Params
*/
typedef struct
{
    Int8                  captureDevice[AUDIO_MAX_STRING_SIZE];     /**< Alsa capture device name */
    UInt32                numChannels;              /**< Num of audio channels to capture */
    UInt32                audioVolume;              /**< Audio volume */
    UInt32                sampleRate;               /**< Capture Sampling Frequency in Hz */
    Audio_CaptureChParams chPrm[ACAP_CHANNELS_MAX]; /**< Channel parameters */
    Bool                  enableTVP5158;            /**< Set TVP 5158 capture */
    Int32                 sampleLen;                /**< Bytes / sample */

} Audio_CaptureParams;


/**
    \brief Audio Link - Audio capture data parameters for a channel
*/
typedef struct {

        Void   *captureDataBuf;   /**< Pointer to captured audio is returned */
        UInt32  captureDataSize;  /**< captured data size  */
        Void   *encodeDataBuf;    /**< Pointer to encoded audio is returned */
        UInt32  encodeDataSize;   /**< encoded data size */

} Audio_getDataParams;


/**
    \brief Audio Link - Audio Decode create parameters
*/
typedef struct {

    Int32        decoderType;         /**< CodecType - AUDIO_ENC_TYPE_AAC_LC / G711 */
    Int32        desiredChannelMode;  /**< Channel mode - mono, stereo */
    Int32        minInBufSize;        /**< Min input buffer size requirement - returned by encoder */ 
    Int32        minOutBufSize;       /**< Min output buffer size requirement - returned by encoder */ 

} Audio_DecodeCreateParams;


/**
    \brief Audio Link - Audio Decode process parameters
*/
typedef struct {

    Audio_BufferParams      inBuf;        /**< Input Buffer details. dataBufSize tells the input number of bytes */
    Audio_BufferParams      outBuf;       /**< Output Buffer details. dataBufSize tells the max output buffer size */
    Int32                   numSamples;    /**< Samples generated */
    Int32                   channelMode;  /**< Output channel mode - same as IAUDIO_ChannelMode */
    Int32                   pcmFormat;     /**< Output PCM Format - Block/Interleaved, same as IAUDIO_PcmFormat*/
    Int32                   bytesPerSample; /**< Bytes per sample */
} Audio_DecodeProcessParams;

/* @} */


/* function's */

/**
 *  \brief    System initialization function.
 *              Does IPC/SysLink initialization between cores.
 *
 *  \return    None
 */
Void Audio_systemProcInit (Void);

/**
 *  \brief     System de-initialization function.
 *              Does IPC/SysLink finalization.
 *
 *  \return     None
 */
void Audio_systemProcDeInit (Void);


/**
    \brief     Return size of encode context structure. 

    \param  codecType                         [IN]   Type of encoder
    
    \return   Size of encode context struct
*/
Int32   Audio_getEncoderContextSize (Int32 codecType);


/**
    \brief     Create AAC Encoder Algorithm
    
    \param  ctxMem                       [IN]   Memory to store internal data structure
    \param  pPrm                          [IN,OUT]   bitrate, samplerate, no. of channels of stream to be encoded.
                                                                    returns min input / output buffer requirement

    \return   Encoder handle on success
*/
Void* Audio_createAacEncAlgorithm(Void *ctxMem, Audio_EncodeCreateParams *pPrm);


/**
    \brief     Create G711 Encoder Algorithm
    
    \param  ctxMem                       [IN]   Memory to store internal data structure

    \return   Encoder handle on success
*/
Void *Audio_createG711EncAlgorithm(Void *ctxMem);


/**
    \brief     Return size of decode context structure. 

    \param  codecType                         [IN]   Type of decoder
    
    \return   Size of decode context struct
*/
Int32  Audio_getDecoderContextSize (Int32 codecType);

/**
    \brief     Create AAC Decoder Algorithm
    
    \param  ctxMem                         [IN]   Memory to store internal data structure
    \param  pPrm                             [IN,OUT]   Channel Mode, returns min input / output buffer requirement

    \return   Decoder handle on success
*/
Void *Audio_createAacDecAlgorithm(Void *ctxMem, Audio_DecodeCreateParams *pPrm);


/**
    \brief     Create G711 Decoder Algorithm
    
    \param  ctxMem                       [IN]   Memory to store internal data structure

    \return   Decoder handle on success
*/
Void *Audio_createG711DecAlgorithm(Void *ctxMem);

/**
    \brief     Delete Encoder Algorithm
    
    \param  ctxMem                       [IN]   Memory with internal data structure

    \return   0 on success
*/
Int32 Audio_deleteEncAlgorithm(Void *ctxMem);



/**
    \brief     Delete  Decoder Algorithm
    
    \param  ctxMem                       [IN]   Memory with internal data structure

    \return   0 on success
*/
Int32 Audio_deleteDecAlgorithm(Void *ctxMem);

/**
    \brief     Encode process
    
    \param  ctxMem                       [IN]   Memory with internal data structure
    \param  pPrm                           [IN,OUT]   Encode parameters

    \return   0 on success
*/
Int32 Audio_encode(Void* ctxMem, Audio_EncodeProcessParams *pPrm);

/**
    \brief     Decode process
    
    \param  ctxMem                       [IN]   Memory with internal data structure
    \param  pPrm                           [IN,OUT]   Decode parameters

    \return   0 on success
*/
Int32 Audio_decode(Void* ctxMem, Audio_DecodeProcessParams *pPrm);

/**
    \brief     Helper function to allocate memory 
    \return   0 on success
 */
Void* Audio_allocMem (Int32 _size);


/**
    \brief     Helper function to free memory 
    \return   0 on success
 */
Void Audio_freeMem (Void *buf);


/**
    \brief     Start capture / demux of audio samples

    \param   pPrm     [IN]     Audio Capture Parameters 
 
    \return   0 on success
 */
Int32 Audio_startCapture (Audio_CaptureParams *pPrm);


/**
    \brief     Get channel statistics

    \param   chNum [IN]                audio channel number
    \param   totalEncCalls [OUT]     Total encode calls done for specified audio channel
    \param   totalTimeInMS [OUT]   Total time spent in encode 
 
    \return	 0 on success
 */
Int32 Audio_getChStats (Int32 chNum, Int32 *totalEncCalls, UInt32 *totalTimeInMS);

/**
    \brief     Retrieve Min Buffer Size - Samples for each channel. App should provide this buffer size for each ch

    \return   size in bytes for the capture channel buffer. 
 */
Int32   Audio_getMinCaptureChBufSize (Void);

/**
    \brief    Retrieve Length of sample in bytes

   \return   returns audio sample length in bytes 
 */
Int32   Audio_getSampleLenInBytes(Void);

/**
    \brief     Enable capture 

    \param  chNum [IN]  channel number to be enabled 
    \return   None
 */
Void Audio_enableCapChannel(Int32 chNum);


/**
    \brief     Disable capture

    \param  chNum [IN]  channel number to be disabled 
    \return	 None
 */
Void Audio_disableCapChannel(Int32 chNum);

/**
    \brief     Stop capture task

   \return   None
 */
Int32 Audio_stopCapture (Void);


/**
    \brief     Get audio channel data

    \param  chNum [IN]  channel number for which data is requested
    \param   pPrm [IN,OUT]  Channel data information

    \return 0 on success
 */
Int32 Audio_getCapChData (UInt8 chNum, Audio_getDataParams *pPrm);


/**
    \brief     Set data consumed info 

    NOT IMPLEMENTED

    \param  chNum [IN]  channel number for which data is already consumed 
    \param   captureDataSizeConsumed [IN]  Tells how much capture data already got via Audio_getCapChData is consumed. For now, all of this is assumed to be consumed
    \param   encodeDataSizeConsumed [IN]  Tells how much encoded data already got via Audio_getCapChData is consumed. For now, all of this is assumed to be consumed
 
    \return 0 on success
 */
Int32 Audio_setCapConsumedChData(UInt8 chNum,
                           UInt32 captureDataSizeConsumed,
                           UInt32 encodeDataSizeConsumed
                                 );

/**
    \brief 
Allocate buffer from shared region - can be used for in / out buffers to encoder /decoder

    \param  bufSize                      [IN]   Size in bytes

    \return allocated buf pointer on success, NULL otherwise
*/
Void *Audio_allocateSharedRegionBuf (Int32 bufSize);

/**
    \brief 
Delete buffer allocated from shared region 

    \param  buf                           [IN]    Buffer pointer
    \param  bufSize                      [IN]    Size in bytes

    \return allocated buf pointer on success, NULL otherwise
*/

Void Audio_freeSharedRegionBuf (Void *buf, Int32 bufSize);


/**
    \brief 
Print memory allocation statistics of audio 

    \return None
*/
Void Audio_printMemStats (Void);

#ifdef  __cplusplus
}
#endif
#endif /* __AUDIO_LINK_H__ */

/*@}*/

