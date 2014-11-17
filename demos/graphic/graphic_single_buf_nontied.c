/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2011 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/*----------------------------------------------------------------------------
 Defines referenced header files
-----------------------------------------------------------------------------*/


#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <linux/fb.h>
#include <linux/ti81xxfb.h>

#include "ti_media_std.h"
#include "graphic.h"
#include "graphic_priv.h"

#include "img_ti_logo.h"

#include "graphic_single_buf_nontied.h"
#include "demos/mcfw_api_demos/mcfw_demo/demo.h"
#include "demos/mcfw_api_demos/mcfw_demo/demo_swms.h"

#define GRPX_SC_MARGIN_OFFSET   (3)

int grpx_fb_draw_singleBufNonTied(unsigned char *buf_addr, UInt32 startX, UInt32 startY, UInt32 planeWidth, Bool clearImg)
{
    int ret=0;
    int iconWidth, iconHeight;
    unsigned char * img_ti_logo = NULL;
    app_grpx_t *grpx = &grpx_obj;

    if(grpx->planeType >= GRPX_FORMAT_MAX)
    {
        return -1;
    }
    if(grpx->planeType == GRPX_FORMAT_RGB565)
    {
        img_ti_logo = (unsigned char *)img_ti_logo_RGB565;
        iconWidth  = 226;
        iconHeight = 31;
    }
    if(grpx->planeType == GRPX_FORMAT_RGB888 || grpx->planeType == GRPX_FORMAT_ARGB8888)
    {
        img_ti_logo = (unsigned char *)img_ti_logo_RGB888;
        iconWidth  = 240;
        iconHeight = 60;
    }

    {
        if(clearImg)
        {
            clear_img(buf_addr,
                    startX,
                    startY,
                    iconWidth,
                    iconHeight,
                    planeWidth
                );
        }
        else
        {
            draw_img(buf_addr,
                    img_ti_logo,
                    startX,
                    startY,
                    iconWidth,
                    iconHeight,
                    planeWidth
                );
        }
    }

    return ret;

}


Int32 grpx_fb_start_singleBufNonTied()
{

    struct fb_var_screeninfo varinfo;
    int hdbuf_size, i;
	UInt32 outWidth, outHeight;

    app_grpx_t *grpx = &grpx_obj;

    dprintf("\n");
    dprintf("FB: Starting !!!\n")

    //# Open the display device
    grpx->fd = open(FBDEV_NAME_0, O_RDWR);
    if (grpx->fd <= 0) {
        eprintf("FB: Could not open device [%s] !!! \n",FBDEV_NAME_0);
        return -1;
    }

    dprintf("FB: Opened device [%s] (fd=%d) !!!\n",FBDEV_NAME_0, grpx->fd);

    if (ioctl(grpx->fd, FBIOGET_VSCREENINFO, &varinfo) < 0) {
        eprintf("FB0: FBIOGET_VSCREENINFO !!!\n");
    }
    varinfo.xres           =  GRPX_PLANE_WIDTH;
    varinfo.yres           =  GRPX_PLANE_HEIGHT;
    varinfo.xres_virtual   =  GRPX_PLANE_WIDTH;
    varinfo.yres_virtual   =  GRPX_PLANE_HEIGHT;

    if (grpx->planeType == GRPX_FORMAT_RGB565)
    {
        varinfo.bits_per_pixel   =  16;
        varinfo.red.length       =  5;
        varinfo.green.length     =  6;
        varinfo.blue.length      =  5;

        varinfo.red.offset       =  11;
        varinfo.green.offset     =  5;
        varinfo.blue.offset      =  0;
    }
    else if (grpx->planeType == GRPX_FORMAT_RGB888)
    {
        varinfo.bits_per_pixel   =  24;
        varinfo.red.length       =  8;
        varinfo.green.length     =  8;
        varinfo.blue.length      =  8;

        varinfo.red.offset       =  16;
        varinfo.green.offset     =  8;
        varinfo.blue.offset      =  0;
    }

    if (ioctl(grpx->fd, FBIOPUT_VSCREENINFO, &varinfo) < 0) {
        eprintf("FB: FBIOPUT_VSCREENINFO !!!\n");
    }

    grpx->bytes_per_pixel = (varinfo.bits_per_pixel / 8);

	/* Map FB dev Buffer */

    hdbuf_size = disp_fbinfo(grpx->fd);
    if(hdbuf_size < 0)
    {
        eprintf("FB0: disp_fbinfo() !!!\n");
        return -1;
    }

    grpx->buf[0] = (unsigned char *)mmap (0, GRPX_PLANE_WIDTH*GRPX_PLANE_HEIGHT*grpx->bytes_per_pixel*NUM_GRPX_DISPLAYS,
            (PROT_READ|PROT_WRITE), MAP_SHARED, grpx->fd, 0);
    if (grpx->buf[0] == MAP_FAILED) {
        eprintf("FB0: mmap() failed !!!\n");
        return -1;
    }
    grpx->buf_size[0] = hdbuf_size;
    for(i = 1; i < NUM_GRPX_DISPLAYS; i++) {
        grpx->buf[i] = grpx->buf[i-1] + (GRPX_PLANE_WIDTH*GRPX_PLANE_HEIGHT*grpx->bytes_per_pixel);
		grpx->buf_size[i] = (GRPX_PLANE_WIDTH*GRPX_PLANE_HEIGHT*grpx->bytes_per_pixel);
    }

	/* Scale FB dev Buffer to screen resolution */

    //Demo_swMsGetOutSize(Vdis_getResolution(VDIS_DEV_HDMI), &outWidth,  &outHeight);
    grpx->curWidth = GRPX_PLANE_WIDTH;
    grpx->curHeight = GRPX_PLANE_HEIGHT;
    grpx_fb_scale(VDIS_DEV_DVO2, 0, 0, GRPX_PLANE_WIDTH, GRPX_PLANE_HEIGHT);


	/* Fill FB dev Buffer */

	if(grpx->buf[0])
	{
    	draw_fill_color(grpx->buf[0], GRPX_PLANE_WIDTH, GRPX_PLANE_HEIGHT);
	}
	if(grpx->buf[1])
	{
	    draw_fill_color(grpx->buf[1], GRPX_PLANE_WIDTH, GRPX_PLANE_HEIGHT);
	}

    dprintf("FB: Start DONE !!!\n");
    dprintf("\n");

	return 0;
}

Int32 grpx_fb_stop_singleBufNonTied()
{
    int i, ret=0;
    app_grpx_t *grpx = &grpx_obj;

    dprintf("\n");
    dprintf("grpx_fb_exit ... \n");

    for(i = 0; i < NUM_GRPX_DISPLAYS; i++) {
        ret = munmap(grpx->buf[i], grpx->buf_size[i]);
	    if(ret)
	        dprintf("FB0: munmap failed (%d) !!!\n", ret);
    }

    if(grpx->fd > 0) {
        ret = close(grpx->fd);
    }

    dprintf("grpx_fb_exit ... Done (%d) !!!\n", ret);
    dprintf("\n");

	return 0;
}


