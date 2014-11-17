
#ifndef _DEMO_TEXT_H_
#define _DEMO_TEXT_H_

#include "demo.h"

#define SWOSD_USER_TRANSPARANCY_ON 1

typedef struct{
    System_VideoDataFormat	format;
    /* bitmap font format. such as 420i 420sp and so on*/
    char*	charList;
    /* character list of the bitmap*/

    Bool	unicode;
    /*  if unicode, it means 2 bytes for one character,
        otherwise 1 byte for one character.
        */

    UInt16	ListLen;
    /* how many character in this list */

    UInt16*  widthList;
    /* width of each character in unit of pixel.*/

    Uint16	height;
    /*  character height in unit of pixel */
    char *fontY;            // address of Y data
    char *fontUV;           // address of UV data
    UInt32 fontWidthPerLine; //width per line for the font data, in unit of pixel
}Bitmap_Info;

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
void Demo_text_draw(Bitmap_Info font, AlgLink_OsdWindowPrm* winPrm, char* text, UInt16 textLen, char* osdMem,UInt32* padingColor);


#endif
