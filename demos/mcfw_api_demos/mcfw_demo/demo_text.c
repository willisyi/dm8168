#include <demo.h>
#include <demo_text.h>

//this API get the index of the text
void Demo_text_get_index(Bitmap_Info font, char* text, UInt16* index,UInt16* indexPix, UInt16 textLen)
{
    int i,j;
    UInt16* unicode;

    // get index table, here the index is in unit of character
    if(font.unicode)
    {
        UInt16 * cList = (UInt16*)font.charList;
        unicode = (UInt16*)text;

        for(i = 0; i < textLen; i++)
        {
            //if no character is found, set the first character
            index[i] = 0;
            for(j = 0;j < font.ListLen; j++)
            {
                if(unicode[i] == cList[j])
                {

                    index[i] = j;
                    break;
                }
            }
        }
    }
    else
    {
        for(i = 0; i < textLen; i++)
        {
            //if no character is found, set the first character
            index[i] = 0;

            for(j = 0;j < font.ListLen; j++)
            {
                if(text[i] == font.charList[j])
                {
                    index[i] = j;
                    break;
                }
            }
        }
    }


    //convert index from unit of character to pixel
    for(i=0;i<textLen;i++)
    {
        indexPix[i] = 0;
        for(j=0;j<index[i];j++)
        {
            indexPix[i] += font.widthList[j];
        }
    }
}
//this API draw one line of memory into osdMem
void Demo_text_draw_osd_by_index(Bitmap_Info font, UInt16* index,UInt16* indexPix, UInt16 textLen,
        char* osdMem, UInt32 lineOffset,UInt32* padingColor,UInt32 pixPerLine)
{
    int i,j,scaleX;
    int padPix;
    char* temp;
    char* src;
    int uvOffset;

    switch(font.format){
        case SYSTEM_DF_YUV422I_YUYV:
            // 2 bytes per pixel
            scaleX = 2;

            padPix = lineOffset - pixPerLine;
            UTILS_assert((padPix%2) == 0);


            // process each line
            for(i = 0; i < font.height;i++)
            {
                temp = osdMem + i*lineOffset*scaleX;


                //process each character
                for(j = 0; j < textLen;j++)
                {
                    src = font.fontY + indexPix[j]*scaleX + i*font.fontWidthPerLine*scaleX;
                    memcpy(temp,src,font.widthList[index[j]]*scaleX);
                    temp += font.widthList[index[j]]*scaleX;
                }

                //pending padingColor
                for(j = 0; j < padPix;j+=scaleX)
                {
                    *temp++ = padingColor[0];
                    *temp++ = padingColor[1];
                    *temp++ = padingColor[0];
                    *temp++ = padingColor[2];
                }
            }
            break;
        case SYSTEM_DF_YUV420SP_UV:
            // 1 bytes per pixel
            scaleX = 1;

            padPix = lineOffset - pixPerLine;
            UTILS_assert((padPix%2) == 0);

            // process Y data for each line
            for(i = 0; i < font.height;i++)
            {
                temp = osdMem + i*lineOffset*scaleX;
                //process each character
                for(j = 0; j < textLen;j++)
                {
                    src = font.fontY + indexPix[j]*scaleX + i*font.fontWidthPerLine;
                    memcpy(temp,src,font.widthList[index[j]]*scaleX);
                    temp += font.widthList[index[j]]*scaleX;
                }

                //pending padingColor
                for(j = 0; j < padPix;j+=scaleX)
                {
                    *temp++ = padingColor[0];
                }

            }
            UTILS_assert((font.height%2 == 0));
            // process UV data for each line
            uvOffset = lineOffset * font.height;

            for(i = 0; i < font.height/2;i++)
            {
                temp = osdMem + i*lineOffset*scaleX + uvOffset;
                //process each character
                for(j = 0; j < textLen;j++)
                {
                    src = font.fontUV + indexPix[j]*scaleX + i*font.fontWidthPerLine;
                    memcpy(temp,src,font.widthList[index[j]]*scaleX);
                    temp += font.widthList[index[j]]*scaleX;
                }

                //pending padingColor
                for(j = 0; j < padPix;j+=2)
                {
                    *temp++ = padingColor[1];
                    *temp++ = padingColor[2];
                }
            }
            break;
        default:
            printf("not support this format yet, font.format %d\n",font.format);
            break;
    }

}

/****
  \brief this API will convert the ascii text into bitmap, and return the bitmap memory
  Here we don't do format convert, so please call this API with the correct font format.
font:       which font is selected to product the bitmap
winPrm:     this API will set some information such as font width, height for low level to draw it.
text:       you need using the character included in the charList, if it is not in the charList, it will
be replaced to the first character in the charList.
for non-unicode, you can use char* example="example88";
for unicode, you should following the following example
in unicode: UInt16 example8[]= "Ê¾Àý8\0008\000"; because it is unicode, so '8' is just 1 byte, we need
convert it to 2 byte, so we add '\000' to the end of this string, because of this, we can't use the libc
string api, because it consider '\000' as the end of string. so you'll lost the second '8'.
if it is not fixed width.
textLen:    length of the drawing osd in unit of character. in unicode mode, it should be the length of unicode, not in byte.
osdMem:     this is the memory contain the bitmap, the caller should make sure the memory is enouth
for the font.
padingColor:   color being filled for line gap, because the OSD need window width to be multiple of 4
it should be padingColor[3], Y U V or R G B
*/
void Demo_text_draw(Bitmap_Info font, AlgLink_OsdWindowPrm* winPrm, char* text, UInt16 textLen, char* osdMem,UInt32* padingColor)
{
    //position of each character in the charList by unit of character
    UInt16 index[256];
    //position of each character in the bitmap data by unit of pixel
    UInt16 indexPix[256];
    UInt16 i;
    UInt32 pixPerLine; //osd pixel perline, need pading some pixel if it is not multiple of 4
    if(textLen > 256)
        textLen = 256;

    memset(index,0,sizeof(index));
    memset(indexPix,0,sizeof(indexPix));

    //set osd window prm accroding to the font information
    winPrm->height = font.height;

    Demo_text_get_index(font,text,index,indexPix,textLen);

    //set osd window prm accroding to the text width
    winPrm->width = 0;
    for(i = 0;i < textLen; i++)
    {
        winPrm->width += font.widthList[index[i]];
    }
    pixPerLine = winPrm->width;

    //Width of window, specified in pixels, must be multiple of 8, not 4
    winPrm->width = ((winPrm->width + ALG_LINK_OSD_WIN_WIDTH_ALIGN_MAX - 1)/ALG_LINK_OSD_WIN_WIDTH_ALIGN_MAX)
        * ALG_LINK_OSD_WIN_WIDTH_ALIGN_MAX;
    winPrm->lineOffset = winPrm->width;
    if(font.format == SYSTEM_DF_YUV420SP_UV)
    {
        winPrm->addr[0][1] = winPrm->addr[0][0] + winPrm->lineOffset * font.height;
    }

    //draw the osd into memory
    {
        Demo_text_draw_osd_by_index(font,index,indexPix,textLen,
                osdMem,winPrm->lineOffset,padingColor,pixPerLine);
    }
}

