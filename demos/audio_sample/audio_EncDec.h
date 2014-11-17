/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/
#ifndef	_AUDIO_ENCDEC_H
#define	_AUDIO_ENCDEC_H
#include <alg_uLawCodec.h>

#define AUDIO_G711CODEC_ENABLE

#include <osa.h>

enum AudioCodecId { ALG_AUD_CODEC_G711=1 };
Int32 AUDIO_audioEncode(Int32 AudioCodecId, short *dst, short *src, Int32 bufsize);
Int32 AUDIO_audioDecode(Int32 AudioCodecId, char* pIndata, short* pOutdata, Int32 inBufSize);

#endif	/* _AUDIO_ENCDEC_H */