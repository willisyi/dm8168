/*
 *******************************************************************************
 * HDVICP2.0 Based H.264 HP Decoder
 *
 * "HDVICP2.0 Based H.264 HP Decoder" is a software module developed on TI's
 *  HDVICP2 based SOCs. This module is capable of decoding a compressed
 *  high/main/baseline profile H.264 bit-stream into a YUV 4:2:0 Raw video.
 *  Based on "ISO/IEC 14496-10".
 * Copyright (C) 2009 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *******************************************************************************
 */

/**
 *******************************************************************************
 * @file <framesize.c>
 *
 * @brief  This file has the implementataion for the generation of frame size
 *         information
 *
 *
 * @author: Pavan Shastry (pavanvs@ti.com)
 *
 * @version 0.0 (Jan 2008) : Base version  [Mody Mihir]
 * @version 0.1 (Jan 2010) : Modified for IVAHD, Bug fixes.
 *                           [Keshav]
 * @version 0.2 (Feb 2010) : Code commenting and cleanup
 *                           [Ramakrishna Adireddy]
 ******************************************************************************
 */
/******************************************************************************
 *  INCLUDE FILES
 ******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define   NAL_ACCESS_UNIT_DELIMITER_CODE 0x00000109
#define   NALU_TYPE_SEI   6
#define   NALU_TYPE_SPS   7
#define   NALU_TYPE_PPS   8
#define   NALU_TYPE_AUD   9
#define   NALU_TYPE_EOS    10
#define   NALU_TYPE_FILL 12

int H264VID_getFrameSize ( int * ,unsigned int, int ) ;


int  prev_nal_type     = 9;
int  prev_vcl_nal_flag = 0;

/**
 ******************************************************************************
 *  @fn   main()
 *  @brief The entry function
 *
 *  @param[in]  argc  : Arguments count
 *  @param[in]  argv  : pointer to the array of arguments
 *
 *  @return[out]  : None
 ******************************************************************************
 */
void Demo_generateH264HdrFile(char *filename)
{
    FILE *file ,*file2;
    unsigned int x, j, k;

    int framesize = 0;
    int tempframesize = 0;
    int retValue  = 0;

    char Framesizefilename[1024];

    int stream[100048];
    int *Strptr;
    int new_nal = 1;

    unsigned int i         = 0;
    unsigned int Total     = 0;
    unsigned int bufferlen = 100000 ;

    file = fopen(filename, "rb" );

    bzero(Framesizefilename,1024);
    strcat(Framesizefilename, filename);
    strcat(Framesizefilename,".hdr\0");

    if(file == NULL)
    {
        printf("Error: can't open Input file.\n");
        exit(1);
    }
    else
    {
        printf(" Input file [%s] opened successfully !!!\n", filename);
    }

    if((file2 = fopen(Framesizefilename, "w+")) == NULL)
    {
        printf("Error: Cannot open Output file.\n");
        exit(1);
    }
    else
    {
        printf(" Output file [%s] opened successfully !!!\n", Framesizefilename);
    }

    x = getc( file );

    while( x != EOF )
    {
        stream[i++] = x;
        if (i >= bufferlen)
        {
            if ((retValue = H264VID_getFrameSize( stream ,bufferlen, new_nal)) == 0)
            {
                framesize += bufferlen;
                i = 0;
                new_nal = 0;
            }
            else
            {
                new_nal    = 1;
                framesize += retValue;
                Total     += framesize;

                //                printf("frame size( %d ) = %d \tTotal= 0x%x\n",
                //                      index++, framesize, Total);
                fprintf(file2,"%d\n",framesize);

                for (j=retValue, k=0; j < bufferlen; j++, k++)
                {
                    stream[k] = stream[j];
                }

                i         = k;
                framesize = 0;
            }
        }

        x = getc( file );
    }

    bufferlen = i;
    Strptr    = stream;
    tempframesize = framesize;
    while ((framesize = H264VID_getFrameSize( Strptr ,bufferlen, new_nal)) != 0)
    {
	Total += (framesize + tempframesize);
        //       printf ("frame size( %d ) = %d \tTotal= 0x%x\n", index++, framesize, Total);
        fprintf(file2,"%d\n",(framesize + tempframesize));

        Strptr    += framesize;
        bufferlen -= framesize;
        tempframesize = 0;
    }
    bufferlen += tempframesize;
    Total += bufferlen;
    //    printf ("frame size( %d ) = %d \tTotal= 0x%x\n", index++, bufferlen, Total);
    fprintf(file2,"%d\n",bufferlen);

    fclose(file);
    fclose(file2);

    //    printf("File Closed\n");
}

/**
 ******************************************************************************
 *  @fn    H264VID_getFrameSize
 *  @brief This function finds out each frame size of the given input bit
 *         stream.
 *
 *  @param[in]  stream    : Pointer to the bit stream
 *  @param[in]  bufferLen :
 *  @param[in]  new_nal   :
 *
 *  @return[out]  : None
 ******************************************************************************
 */
int H264VID_getFrameSize (int *stream, unsigned int bufferLen, int new_nal)
{
    unsigned int wordCount,nalLength,frameSize = 0;
    unsigned int loopCount;
    unsigned short int  firstMbInSliceFlag;
    unsigned short int NewNalType;
    int vcl_nal_flag;

    if (bufferLen < 8) return (0);

    if(new_nal)
    {
        prev_nal_type     = stream[4] & 0x1f;
        prev_vcl_nal_flag = ((prev_nal_type <=5) && (prev_nal_type > 0));
    }

    nalLength   = 0;

    for (wordCount = 4; wordCount < bufferLen; wordCount++)
    {
        loopCount = wordCount;

        if ((stream[loopCount++] == 0 && stream[loopCount++] == 0 &&
                    stream[loopCount++] == 0 && stream[loopCount++] == 0x01)  ||
                (stream[loopCount++] == 0 && stream[loopCount++] == 0 &&
                 stream[loopCount++] == 1))
        {
            NewNalType         = stream[loopCount++] & 0x1f;
            firstMbInSliceFlag = stream[loopCount++] & 0x80;
            vcl_nal_flag       = ((NewNalType <=5) && (NewNalType > 0));

            if(vcl_nal_flag && prev_vcl_nal_flag)
            {
                prev_nal_type     = NewNalType;
                prev_vcl_nal_flag = vcl_nal_flag;

                if( (firstMbInSliceFlag))
                    break;
            }

            if ((prev_vcl_nal_flag && !vcl_nal_flag))
            {
                prev_nal_type     = NewNalType;
                prev_vcl_nal_flag = vcl_nal_flag;

                break;
            }

            prev_nal_type     = NewNalType;
            prev_vcl_nal_flag = vcl_nal_flag;
        }

        nalLength++;
    }

    frameSize = nalLength + 4;

    if (wordCount >= bufferLen)
    {
        return (0);
    }
    else
    {
        return (frameSize);
    }

}



