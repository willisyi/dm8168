/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2011 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/*----------------------------------------------------------------------------
 Defines referenced header files
-----------------------------------------------------------------------------*/

#include <ti_vgrpx.h>

#define RGB_KEY_24BIT_GRAY   0x00FFFFFF
#define RGB_KEY_16BIT_GRAY   0xFFFF

#include "img_ti_logo.h"
#include "graphic.h"
#include "demos/mcfw_api_demos/mcfw_demo/demo.h"
#include "demos/mcfw_api_demos/mcfw_demo/demo_swms.h"

/*----------------------------------------------------------------------------
 Definitions and macro
-----------------------------------------------------------------------------*/

#define HDMI_GRPX_ID    (0)
#define SDTV_GRPX_ID    (2)

#define NUM_GRPX_DISPLAYS   1

#define GRPX_PLANE_WIDTH    (1280)
#define GRPX_PLANE_HEIGHT   ( 720)

#define GRPX_STARTX_0        (50u)
#define GRPX_STARTY_0        (20u)
#define GRPX_STARTX_1        (GRPX_PLANE_WIDTH-250)
#define GRPX_STARTY_1        (GRPX_STARTY_0)
#define GRPX_STARTX_2        (GRPX_STARTX_0)
#define GRPX_STARTY_2        (GRPX_PLANE_HEIGHT-75)
#define GRPX_STARTX_3        (GRPX_STARTX_1)
#define GRPX_STARTY_3        (GRPX_STARTY_2)



typedef struct {
    unsigned char *buf[NUM_GRPX_DISPLAYS];
    unsigned int curWidth;
    unsigned int curHeight;
    unsigned int iconWidth;
    unsigned int iconHeight;
    unsigned int bytes_per_pixel;
    int buf_size;
    grpx_plane_type planeType;

} app_grpx_t;

/*----------------------------------------------------------------------------
 Declares variables
-----------------------------------------------------------------------------*/
static app_grpx_t grpx_obj;

/*----------------------------------------------------------------------------
 Declares a function prototype
-----------------------------------------------------------------------------*/
#define eprintf(x...) printf(" [GRPX] ERROR: " x);
#define dprintf(x...) printf(" [GRPX] " x);
//#define dprintf(x...)

/*----------------------------------------------------------------------------
 local function
-----------------------------------------------------------------------------*/

static int draw_fill_color(unsigned char *buf_addr)
{
    unsigned int i, j;
    unsigned char *p;
    app_grpx_t *grpx = &grpx_obj;

    if(buf_addr==NULL)
        return -1;

    p = (unsigned char *)buf_addr;
    for(i=0; i<grpx->curHeight; i++) {
        for(j=0; j<grpx->curWidth; j++) {
            switch(grpx->planeType)
            {
                case GRPX_FORMAT_RGB565:
                    *p++  = (RGB_KEY_16BIT_GRAY >> 0 ) & 0xFF;
                    *p++  = (RGB_KEY_16BIT_GRAY >> 8 ) & 0xFF;
                break;
                case GRPX_FORMAT_RGB888:
                    *p++  = (RGB_KEY_24BIT_GRAY >> 0  ) & 0xFF;
                    *p++  = (RGB_KEY_24BIT_GRAY >> 8  ) & 0xFF;
                    *p++  = (RGB_KEY_24BIT_GRAY >> 16 ) & 0xFF;
                    *p++  = 0x00;
                break;
                case GRPX_FORMAT_MAX:
                default:
                break;
            }
        }
    }

    return 0;
}


static int clear_img(VDIS_DEV devId, unsigned char *buf_addr, int sx, int sy, int wi, int ht)
{
    unsigned int i, j;
    unsigned char *p;
    app_grpx_t *grpx = &grpx_obj;
    unsigned char *key;


    if(buf_addr==NULL)
        return -1;

    p = (unsigned char *)(buf_addr + ((sx * grpx->bytes_per_pixel)+( sy * grpx->bytes_per_pixel * grpx->curWidth)));
    for(j=0; j<ht; j++)
    {
        for(i=0; i<wi; i++)
        {
            if(grpx->planeType == GRPX_FORMAT_RGB565)
            {
                unsigned short key_rgb565 = RGB_KEY_16BIT_GRAY;
                key = (unsigned char *)&key_rgb565;

                *p       = key[0];
                *(p + 1) = key[1];
            }
            if(grpx->planeType == GRPX_FORMAT_RGB888)
            {
                unsigned int key_rgb888 = RGB_KEY_24BIT_GRAY;
                key = (unsigned char *)&key_rgb888;

                *p       = key[0];
                *(p + 1) = key[1];
                *(p + 2) = key[2];
                *(p + 3) = key[3];
            }
            p        += grpx->bytes_per_pixel;
        }
        p += ((grpx->curWidth-wi) * grpx->bytes_per_pixel);
    }

    return 0;
}

static int draw_img(VDIS_DEV devId, unsigned char *buf_addr, unsigned char *img_addr, int sx, int sy, int wi, int ht)
{
    unsigned int i, j;
    unsigned char *p;
    app_grpx_t *grpx = &grpx_obj;
    unsigned char *key;


    if(buf_addr==NULL || img_addr==NULL)
        return -1;

    p = (unsigned char *)(buf_addr + ((sx * grpx->bytes_per_pixel)+( sy * grpx->bytes_per_pixel * grpx->curWidth)));
    for(j=0; j<ht; j++)
    {
        for(i=0; i<wi; i++)
        {
            if(grpx->planeType == GRPX_FORMAT_RGB565)
            {
                unsigned short key_rgb565 = RGB_KEY_16BIT_GRAY;
                key = (unsigned char *)&key_rgb565;

                *p = *img_addr;
                *(p + 1) = *(img_addr + 1);
            }
            if(grpx->planeType == GRPX_FORMAT_RGB888)
            {
                unsigned int key_rgb888 = RGB_KEY_24BIT_GRAY;
                key = (unsigned char *)&key_rgb888;

                *p = *img_addr;
                *(p + 1) = *(img_addr + 1);
                *(p + 2) = *(img_addr + 2);
                *(p + 3) = *(img_addr + 3);

            }
            p        += grpx->bytes_per_pixel;
            img_addr += grpx->bytes_per_pixel;
        }
        p += ((grpx->curWidth-wi) * grpx->bytes_per_pixel);
    }

    return 0;
}

int grpx_link_draw_logo(VDIS_DEV devId, UInt32 startX, UInt32 startY, Bool clearImg)
{
    int ret=0;
    unsigned char * img_ti_logo = NULL;
    app_grpx_t *grpx = &grpx_obj;

    if(grpx->planeType >= GRPX_FORMAT_MAX)
    {
        return -1;
    }
    if(grpx->planeType == GRPX_FORMAT_RGB565)
    {
        img_ti_logo = (unsigned char *)img_ti_logo_RGB565;
        grpx->iconWidth  = 226;
        grpx->iconHeight = 31;

    }
    if(grpx->planeType == GRPX_FORMAT_RGB888)
    {
        img_ti_logo = (unsigned char *)img_ti_logo_RGB888;
        grpx->iconWidth  = 240;
        grpx->iconHeight = 60;
    }

    if (devId == VDIS_DEV_HDMI) {
        if(clearImg)
        {
            clear_img(   devId,
                    grpx->buf[0],
                    startX,
                    startY,
                    grpx->iconWidth,
                    grpx->iconHeight
                );
        }
        else
        {
            draw_img(   devId,
                    grpx->buf[0],
                    img_ti_logo,
                    startX,
                    startY,
                    grpx->iconWidth,
                    grpx->iconHeight
                );
        }
    }

    return ret;

}

/*----------------------------------------------------------------------------
 grpx_link draw function
-----------------------------------------------------------------------------*/
int grpx_link_draw(VDIS_DEV devId)
{
    int ret=0;
    unsigned char * img_ti_logo = NULL;
    app_grpx_t *grpx = &grpx_obj;

    dprintf("grpx_link_draw ... \n");

    if(grpx->planeType >= GRPX_FORMAT_MAX)
    {
        return -1;
    }
    if(grpx->planeType == GRPX_FORMAT_RGB565)
    {
        img_ti_logo = (unsigned char *)img_ti_logo_RGB565;
        grpx->iconWidth  = 160;
        grpx->iconHeight = 64;

    }
    if(grpx->planeType == GRPX_FORMAT_RGB888)
    {
        img_ti_logo = (unsigned char *)img_ti_logo_RGB888;
        grpx->iconWidth  = 240;
        grpx->iconHeight = 60;
    }

    if (devId == VDIS_DEV_HDMI) {

        grpx_link_draw_logo(devId, GRPX_STARTX_0, GRPX_STARTY_0, FALSE);
        grpx_link_draw_logo(devId, GRPX_STARTX_1, GRPX_STARTY_1, FALSE);
        grpx_link_draw_logo(devId, GRPX_STARTX_2, GRPX_STARTY_2, FALSE);
        grpx_link_draw_logo(devId, GRPX_STARTX_3, GRPX_STARTY_3, FALSE);
    }
    dprintf("grpx_link_draw ... Done !!! \n");

    return ret;
}


Int32 grpx_link_scale(UInt32 devId, UInt32 startX, UInt32 startY, UInt32 outWidth, UInt32 outHeight)
{
    UInt32 grpxId;
    VGRPX_DYNAMIC_PARAM_S grpxParams;

    app_grpx_t *grpx = &grpx_obj;

    grpxId = HDMI_GRPX_ID;
    if(devId==VDIS_DEV_SD)
        grpxId = SDTV_GRPX_ID;

    grpxParams.scaleEnable = TRUE;
    grpxParams.transperencyEnable = TRUE;
    grpxParams.transperencyColor = RGB_KEY_24BIT_GRAY;
    grpxParams.inWidth = grpx->curWidth;
    grpxParams.inHeight = grpx->curHeight;
    grpxParams.displayWidth  = outWidth;
    grpxParams.displayHeight = outHeight;
    grpxParams.displayStartX = startX;
    grpxParams.displayStartY = startY;

    Vgrpx_setDynamicParam(grpxId, &grpxParams);

    return (0);
}

/*----------------------------------------------------------------------------
 grpx_link init/exit function
-----------------------------------------------------------------------------*/
int grpx_link_init(grpx_plane_type planeType)
{
    UInt32 outWidth, outHeight;

    VGRPX_CREATE_PARAM_S createPrm;
    VGRPX_BUFFER_INFO_S  bufInfo;

    app_grpx_t *grpx = &grpx_obj;

    dprintf("\n");
    dprintf("Starting !!!\n")

    memset(grpx, 0, sizeof(app_grpx_t));

    if(planeType >= GRPX_FORMAT_MAX)
    {
        return -1;
    }
    else
    {
        grpx->planeType = planeType;
    }

    grpx->bytes_per_pixel = 4;
    if(planeType==GRPX_FORMAT_RGB565)
        grpx->bytes_per_pixel = 2;

    grpx->curWidth = GRPX_PLANE_WIDTH;
    grpx->curHeight = GRPX_PLANE_HEIGHT;

    Demo_swMsGetOutSize(Vdis_getResolution(VDIS_DEV_HDMI), &outWidth,  &outHeight);

    createPrm.grpxId                    = HDMI_GRPX_ID;
    createPrm.bufferInfo.bufferPhysAddr = 0;
    createPrm.bufferInfo.bufferPitch    = 0;
    createPrm.bufferInfo.bufferWidth    = grpx->curWidth;
    createPrm.bufferInfo.bufferHeight   = grpx->curHeight;

    if(planeType==GRPX_FORMAT_RGB565)
        createPrm.bufferInfo.dataFormat = VGRPX_DATA_FORMAT_RGB565;
    else
        createPrm.bufferInfo.dataFormat = VGRPX_DATA_FORMAT_ARGB888;

    createPrm.bufferInfo.scanFormat     = VGRPX_SF_PROGRESSIVE;

    createPrm.dynPrm.scaleEnable        = TRUE;
    createPrm.dynPrm.transperencyEnable = TRUE;
    createPrm.dynPrm.transperencyColor  = RGB_KEY_24BIT_GRAY;
    createPrm.dynPrm.inWidth            = grpx->curWidth;
    createPrm.dynPrm.inHeight           = grpx->curHeight;
    createPrm.dynPrm.displayWidth       = outWidth;
    createPrm.dynPrm.displayHeight      = outHeight;
    createPrm.dynPrm.displayStartX      = 0;
    createPrm.dynPrm.displayStartY      = 0;

    Vgrpx_create(HDMI_GRPX_ID, &createPrm);

    Vgrpx_getInfo(HDMI_GRPX_ID, &bufInfo);

    grpx->buf_size = bufInfo.bufferPitch*bufInfo.bufferHeight;

    grpx->buf[0] = (unsigned char *)Vgrpx_mmap(bufInfo.bufferPhysAddr, grpx->buf_size, FALSE);

    if (grpx->buf[0] == NULL) {
        eprintf("GRPX0: mmap() failed !!!\n");
        return -1;
    }

    Demo_swMsGetOutSize(Vdis_getResolution(VDIS_DEV_SD), &outWidth, &outHeight);

    createPrm.grpxId                    = SDTV_GRPX_ID;
    createPrm.bufferInfo.bufferPhysAddr = bufInfo.bufferPhysAddr;
    createPrm.bufferInfo.bufferPitch    = bufInfo.bufferPitch;
    createPrm.bufferInfo.bufferWidth    = bufInfo.bufferWidth;
    createPrm.bufferInfo.bufferHeight   = bufInfo.bufferHeight;
    createPrm.bufferInfo.dataFormat     = bufInfo.dataFormat;
    createPrm.bufferInfo.scanFormat     = bufInfo.scanFormat;

    createPrm.dynPrm.scaleEnable        = TRUE;
    createPrm.dynPrm.transperencyEnable = TRUE;
    createPrm.dynPrm.transperencyColor  = RGB_KEY_24BIT_GRAY;
    createPrm.dynPrm.inWidth            = grpx->curWidth;
    createPrm.dynPrm.inHeight           = grpx->curHeight;
    createPrm.dynPrm.displayWidth       = outWidth;
    createPrm.dynPrm.displayHeight      = outHeight;
    createPrm.dynPrm.displayStartX      = 0;
    createPrm.dynPrm.displayStartY      = 0;

    Vgrpx_create(SDTV_GRPX_ID, &createPrm);

    Vgrpx_enable(HDMI_GRPX_ID, TRUE);
    Vgrpx_enable(SDTV_GRPX_ID, TRUE);

    {
        UInt32 curTime = OSA_getCurTimeInMsec();
        //# init fb - fill RGB_KEY
        draw_fill_color(grpx->buf[0]);

        curTime = OSA_getCurTimeInMsec() - curTime;

        dprintf("### Memfill time = %d msecs\n", curTime);
    }

    dprintf("Start DONE !!!\n");
    dprintf("\n");

    return 0;
}

void grpx_link_exit(void)
{
    int ret=0;
    app_grpx_t *grpx = &grpx_obj;

    dprintf("\n");
    dprintf("grpx_link_exit ... \n");

    if(grpx->buf[0]) {
        Vgrpx_unmap((UInt32)grpx->buf[0], grpx->buf_size);
    }

    Vgrpx_enable(SDTV_GRPX_ID, FALSE);
    Vgrpx_enable(HDMI_GRPX_ID, FALSE);

    Vgrpx_delete(SDTV_GRPX_ID);
    Vgrpx_delete(HDMI_GRPX_ID);

    dprintf("grpx_link_exit ... Done (%d) !!!\n", ret);
    dprintf("\n");

    return;
}


Int32 grpx_link_scale_demo()
{
    UInt32 devId;
    UInt32 outWidth, outHeight;
    UInt32 startX, startY;
    UInt32 offsetX, offsetY;
    UInt32 loopCount, i;
    UInt32 runCount;

    devId = VDIS_DEV_SD;

    runCount = 10000;

    loopCount = 100;
    offsetX = offsetY = 1;

    /* putting in a loop for test */
    while(runCount--)
    {
        /* putting in another loop to change size and position every few msecs */
        for(i=1; i<=loopCount; i++)
        {
            Demo_swMsGetOutSize(Vdis_getResolution(devId), &outWidth, &outHeight);

            startX = offsetX*i;
            startY = offsetY*i;

            outWidth  -= startX*2;
            outHeight -= startY*2;

            grpx_link_scale(devId, startX, startY, outWidth, outHeight);

            OSA_waitMsecs(17);
        }
        for(i=loopCount; i>=1; i--)
        {
            Demo_swMsGetOutSize(Vdis_getResolution(devId), &outWidth, &outHeight);

            startX = offsetX*i;
            startY = offsetY*i;

            outWidth  -= startX*2;
            outHeight -= startY*2;

            grpx_link_scale(devId, startX, startY, outWidth, outHeight);

            OSA_waitMsecs(17);
        }

        /* restore to original */
        Demo_swMsGetOutSize(Vdis_getResolution(devId), &outWidth, &outHeight);

        dprintf("[reset] %d x %d\n", outWidth, outHeight);
        grpx_link_scale(devId, 0, 0, outWidth, outHeight);

        OSA_waitMsecs(17);
    }

    return 0;
}

Int32 grpx_link_draw_demo()
{
    UInt32 devId;
    UInt32 offsetX, offsetY;
    int curOffset = 0;
    int loopCount = 100;

    devId = VDIS_DEV_HDMI;

    while(loopCount--)
    {

        offsetX = curOffset*10;
        offsetY = curOffset*10;

        /* clear logo's at old position */
        grpx_link_draw_logo(devId, GRPX_STARTX_0+offsetX, GRPX_STARTY_0+offsetY, TRUE);
        grpx_link_draw_logo(devId, GRPX_STARTX_1-offsetX, GRPX_STARTY_1+offsetY, TRUE);
        grpx_link_draw_logo(devId, GRPX_STARTX_2+offsetX, GRPX_STARTY_2-offsetY, TRUE);
        grpx_link_draw_logo(devId, GRPX_STARTX_3-offsetX, GRPX_STARTY_3-offsetY, TRUE);

        curOffset = (curOffset+1)%20;

        offsetX = curOffset*10;
        offsetY = curOffset*10;

        /* draw logo at new position */
        grpx_link_draw_logo(devId, GRPX_STARTX_0+offsetX, GRPX_STARTY_0+offsetY, FALSE);
        grpx_link_draw_logo(devId, GRPX_STARTX_1-offsetX, GRPX_STARTY_1+offsetY, FALSE);
        grpx_link_draw_logo(devId, GRPX_STARTX_2+offsetX, GRPX_STARTY_2-offsetY, FALSE);
        grpx_link_draw_logo(devId, GRPX_STARTX_3-offsetX, GRPX_STARTY_3-offsetY, FALSE);

        OSA_waitMsecs(100);
    }

    /* clear logo's at old position */
    grpx_link_draw_logo(devId, GRPX_STARTX_0+offsetX, GRPX_STARTY_0+offsetY, TRUE);
    grpx_link_draw_logo(devId, GRPX_STARTX_1-offsetX, GRPX_STARTY_1+offsetY, TRUE);
    grpx_link_draw_logo(devId, GRPX_STARTX_2+offsetX, GRPX_STARTY_2-offsetY, TRUE);
    grpx_link_draw_logo(devId, GRPX_STARTX_3-offsetX, GRPX_STARTY_3-offsetY, TRUE);

    offsetX = 0;
    offsetY = 0;

    /* draw logo at original position */
    grpx_link_draw_logo(devId, GRPX_STARTX_0+offsetX, GRPX_STARTY_0+offsetY, FALSE);
    grpx_link_draw_logo(devId, GRPX_STARTX_1-offsetX, GRPX_STARTY_1+offsetY, FALSE);
    grpx_link_draw_logo(devId, GRPX_STARTX_2+offsetX, GRPX_STARTY_2-offsetY, FALSE);
    grpx_link_draw_logo(devId, GRPX_STARTX_3-offsetX, GRPX_STARTY_3-offsetY, FALSE);

    return 0;
}

Int32 grpx_link_demo()
{
//    grpx_link_scale_demo();
    grpx_link_draw_demo();

    return 0;
}