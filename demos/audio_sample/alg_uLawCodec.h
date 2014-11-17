/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/
#ifndef	_AUDIO_G711CODEC_H
#define	_AUDIO_G711CODEC_H

#include <osa.h>

Int32 ALG_ulawEncode(short *dst, short *src, Int32 bufsize);
//Int32 ALG_alawEncode(short *dst, short *src, Int32 bufsize);
short ALG_ulawDecode(unsigned short input) ;

Void Audio_G711codecEnable();
Void Audio_G711codecDisable();
Int32 Audio_G711codecPrintStats();
#endif	/* _AUDIO_G711CODEC_H */
