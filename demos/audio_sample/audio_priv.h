/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#ifndef _AUDIO_PRIV_H_
#define _AUDIO_PRIV_H_

#include <alsa/asoundlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <pthread.h>
#include <sched.h>

#include <audio_EncDec.h>
#include <mcfw/interfaces/link_api/avsync_hlos.h>
// WARNING - this is the only supported rate - dont change
#define AUDIO_CHANNELS_SUPPORTED                            1       //< Only mono
#define AUDIO_CHANNEL_SAMPLE_INTERLEAVE_LEN                 1
#define AUDIO_SAMPLES_TO_READ_PER_CHANNEL                   (128)       //< Ensure to align with hw param BUFFER_SIZE; only < 256 works right now
#define AUDIO_SAMPLE_LEN                                    2
#define AUDIO_PLAYBACK_SAMPLE_MULTIPLIER                    8

/* We store huge audio samples one shot with a corresponding timestamp
        == SAMPLES_TO_READ_PER_CHANNEL * PLAYBACK_SAMPLE_MULTIPLIER
    Driver will consume lot of time in sending out this data, hence huge backend delay
    which we need to adjust when a comparison is done rendered video PTS
*/
#define AUDIO_BACKEND_DELAY_IN_MS           (((AUDIO_SAMPLES_TO_READ_PER_CHANNEL * AUDIO_PLAYBACK_SAMPLE_MULTIPLIER) / AUDIO_SAMPLE_RATE_DEFAULT) * 1000)

#define AUDIO_RECORD_DIR        "audio"
#define AUDIO_RECORD_FILE       "record"

//#ifdef TI_814X_BUILD
//#define ALSA_CAPTURE_DEVICE     "plughw:1,0"
//#else
#define ALSA_CAPTURE_DEVICE     "plughw:0,0"
//#endif

// #define	AUDIO_STORE_HEADER			/* Store header in audio recording */

#define AUDIO_ERROR(...) \
  fprintf(stderr, " ERROR  (%s|%s|%d): ", __FILE__, __func__, __LINE__); \
  fprintf(stderr, __VA_ARGS__);

#define     AUD_DEVICE_PRINT_ERROR_AND_RETURN(str, err, hdl)        \
        fprintf (stderr, "AUDIO >> " str, snd_strerror (err));  \
        snd_pcm_close (hdl);    \
        return  -1;

#define AUDIO_CAPTURE_TSK_PRI                           18
#define AUDIO_CAPTURE_TSK_STACK_SIZE                    (10*1024)

#define AUDIO_PLAY_TSK_PRI                              18
#define AUDIO_PLAY_TSK_STACK_SIZE                       (10*1024)

#define AUDIO_RECORD_FILE_MAX_SIZE                      (20*1024*1024)
#define AUDIO_MAX_FILES                                 10
#define AUDIO_BUFFER_SIZE                               ((8192*2)>>2)

#define AUDIO_MAX_PLAYBACK_BUF_SIZE                     (120*1024)  /* Max hd read for ~4 secs worth of data assuming 16KHz */

#define AUDIO_MAX_QUE_LENGTH                                (32)

typedef enum    _BKT_STATUS
{
    BKT_VALID,
    BKT_END,
    BKT_START
} BKT_STATUS;


typedef struct  _AudioBkt
{
    Int8            id[5];
    Uint8           numChannels;
    Uint16      samplingRate;
    Uint64      timestamp;
    Uint32      data_size;
    Uint32      bkt_status;
} AudioBkt;

typedef struct  _AudioStats
{
    Int32   errorCnt;
    Int32   lastError;
} AudioStats;

typedef     Void * (*ThrEntryFunc)(Void *);

typedef pthread_t ThrId;

typedef struct  AVSyncInfo
{
    AvsyncLink_AudQueObj queObj;
    AudFrm_Buf audFrames[AUDIO_MAX_QUE_LENGTH];
    OSA_QueHndl freeAudFrmQue;
    Uint64 firstVideoTS;
    Uint64 firstAudioTS;
    Uint64 tsIncPerCh[AUDIO_MAX_CHANNELS];
}AVSyncInfo;

typedef struct  _TaskCtx
{
    ThrId   handle;
    Int8    exitFlag;
} TaskCtx;

Int32 Audio_thrCreate(TaskCtx *ctx, ThrEntryFunc entryFunc, Uint32 pri, Uint32 stackSize);

Int32 Audio_thrDelete (TaskCtx *ctx);

#endif  /* _AUDIO_PRIV_H_ */
