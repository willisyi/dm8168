/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/
#include <alg_uLawCodec.h>

extern Uint8 audioG711codec;

int ALG_ulawEncode(short *dst, short *src, Int32 bufsize)
{
    unsigned short i;
    short data;
    unsigned short isNegative;
    short nOut;
    short lowByte = 1;
    int outputSize = bufsize / 2;

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
        return (outputSize);
}

short ALG_ulawDecode(unsigned short input) 
{
    unsigned short isNegative;
    short nOut;
    isNegative= ((input & 0x80) == 0);
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




Void Audio_G711codecEnable()
{
	audioG711codec = TRUE;
	printf("\r\n G711 Codec is enabled\r\n");
}

Void Audio_G711codecDisable()
{
	audioG711codec = FALSE;
	printf("\r\n G711 Codec is disabled\r\n");
}

Int32 Audio_G711codecPrintStats()
{
	printf ("\n============================\n");
	if (audioG711codec == TRUE)
		printf ("G711 ENABLED,  \n");
	else
		printf ("G711 DISABLED,  \n");
	return 0;
}
