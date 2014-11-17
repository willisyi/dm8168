/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/
#include <audio_EncDec.h>

int AUDIO_audioEncode(int AudioCodecId, short *dst, short *src, Int32 bufsize)
{
	//printf("#####if gets here xyx_0327");
	int ret = OSA_EFAIL;
	switch( AudioCodecId == ALG_AUD_CODEC_G711 )
	{
		default:
		case ALG_AUD_CODEC_G711:
					ret = ALG_ulawEncode(dst, src, bufsize);
		break;
	}
       /*
	if (AudioCodecId)
	{
		ret = ALG_ulawEncode(dst, src, bufsize);
	}
	else
	{
		ret = ALG_alawEncode(dst, src, bufsize);
	}
	*/
	return ret;
}

int AUDIO_audioDecode(int AudioCodecId, char* pIndata, short* pOutdata, int inBufSize)
{
    int sizeConsumed = 0, i = 0 ;
    sizeConsumed = inBufSize;    
	switch( AudioCodecId == ALG_AUD_CODEC_G711 )
	{
		default:
		case ALG_AUD_CODEC_G711:	
		for( i = 0; i < sizeConsumed ; i++)
	    {
	        *((unsigned short*)(pOutdata) + i) = ALG_ulawDecode(*((unsigned char *)(pIndata) + i ));
	    }
		break;
	}
    return TRUE ;
}
