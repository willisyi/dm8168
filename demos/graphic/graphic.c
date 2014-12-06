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
#include <math.h>
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

#define GRPX_SC_MARGIN_OFFSET   (3)

#include "ti_media_std.h"
#include "graphic.h"
#include "graphic_priv.h"
#include "pci_ep.h"

#include "demos/mcfw_api_demos/mcfw_demo/demo.h"
#include "demos/mcfw_api_demos/mcfw_demo/demo_swms.h"

#if defined(TI_816X_BUILD)
#include "graphic_single_buf_nontied.h"
#define GRPX_FB_SINGLE_BUFFER_NON_TIED_GRPX
#define grpx_fb_draw_logo grpx_fb_draw_singleBufNonTied
#endif

#if defined(TI_814X_BUILD)
#include "graphic_single_buf_tied.h"
#define GRPX_FB_SINGLE_BUFFER_TIED_GRPX
#endif

#if defined(TI_8107_BUILD)
#include "graphic_separate_buf_nontied.h"
#define GRPX_FB_SEPARATE_BUFFER_NON_TIED_GRPX
// #define NUM_GRPX_DISPLAYS   2
#endif

/*----------------------------------------------------------------------------
 local function
-----------------------------------------------------------------------------*/

int disp_getregparams(int display_fd)
{
    int ret;

    struct ti81xxfb_region_params regp;

    memset(&regp, 0, sizeof(regp));

    ret = ioctl(display_fd, TIFB_GET_PARAMS, &regp);
    if (ret < 0) {
        eprintf("TIFB_GET_PARAMS\n");
        return ret;
    }

    dprintf("\n");
    dprintf("Reg Params Info\n");
    dprintf("---------------\n");
    dprintf("region %d, postion %d x %d, prioirty %d\n",
        regp.ridx,
        regp.pos_x,
        regp.pos_y,
        regp.priority);
    dprintf("first %d, last %d\n",
        regp.firstregion,
        regp.lastregion);
    dprintf("sc en %d, sten en %d\n",
        regp.scalaren,
        regp.stencilingen);
    dprintf("tran en %d, type %d, key %d\n",
        regp.transen,
        regp.transtype,
        regp.transcolor);
    dprintf("blend %d, alpha %d\n"
        ,regp.blendtype,
        regp.blendalpha);
    dprintf("bb en %d, alpha %d\n",
        regp.bben,
        regp.bbalpha);
    dprintf("\n");
    return 0;
}

int disp_fbinfo(int fd)
{
    app_grpx_t *grpx = &grpx_obj;
    struct fb_fix_screeninfo fixinfo;
    struct fb_var_screeninfo varinfo, org_varinfo;
    int size;
    int ret;

    /* Get fix screen information. Fix screen information gives
     * fix information like panning step for horizontal and vertical
     * direction, line length, memory mapped start address and length etc.
     */
    ret = ioctl(fd, FBIOGET_FSCREENINFO, &fixinfo);
    if (ret < 0) {
        eprintf("FBIOGET_FSCREENINFO !!!\n");
        return -1;
    }

    {
        dprintf("\n");
        dprintf("Fix Screen Info\n");
        dprintf("---------------\n");
        dprintf("Line Length - %d\n", fixinfo.line_length);
        dprintf("Physical Address = %lx\n",fixinfo.smem_start);
        dprintf("Buffer Length = %d\n",fixinfo.smem_len);
        dprintf("\n");
    }
    grpx->phyBuf[0] = fixinfo.smem_start;

    /* Get variable screen information. Variable screen information
     * gives informtion like size of the image, bites per pixel,
     * virtual size of the image etc. */
    ret = ioctl(fd, FBIOGET_VSCREENINFO, &varinfo);
    if (ret < 0) {
        eprintf("FBIOGET_VSCREENINFO !!!\n");
        return -1;
    }

    {
        dprintf("\n");
        dprintf("Var Screen Info\n");
        dprintf("---------------\n");
        dprintf("Xres - %d\n", varinfo.xres);
        dprintf("Yres - %d\n", varinfo.yres);
        dprintf("Xres Virtual - %d\n", varinfo.xres_virtual);
        dprintf("Yres Virtual - %d\n", varinfo.yres_virtual);
        dprintf("Bits Per Pixel - %d\n", varinfo.bits_per_pixel);
        dprintf("Pixel Clk - %d\n", varinfo.pixclock);
        dprintf("Rotation - %d\n", varinfo.rotate);
        dprintf("\n");
    }

    disp_getregparams(fd);

    memcpy(&org_varinfo, &varinfo, sizeof(varinfo));

    /*
     * Set the resolution which read before again to prove the
     * FBIOPUT_VSCREENINFO ioctl.
     */

    ret = ioctl(fd, FBIOPUT_VSCREENINFO, &org_varinfo);
    if (ret < 0) {
        eprintf("FBIOPUT_VSCREENINFO !!!\n");
        return -1;
    }

    /* It is better to get fix screen information again. its because
     * changing variable screen info may also change fix screen info. */
    ret = ioctl(fd, FBIOGET_FSCREENINFO, &fixinfo);
    if (ret < 0) {
        eprintf("FBIOGET_FSCREENINFO !!!\n");
        return -1;
    }

    size = varinfo.xres*varinfo.yres*(varinfo.bits_per_pixel/8);
    dprintf("\n");
    dprintf("### BUF SIZE = %d Bytes !!! \n", size);
    dprintf("\n");

    return size;
}

int draw_fill_color(unsigned char *buf_addr, int curWidth, int curHeight)
{
    unsigned int i, j;
    unsigned char *p;
    app_grpx_t *grpx = &grpx_obj;

    if(buf_addr==NULL)
        return -1;

    p = (unsigned char *)buf_addr;
    for(i = 0; i < curHeight; i++) {
        for(j = 0; j < curWidth; j++) {
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
                break;
                case GRPX_FORMAT_ARGB8888:
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
/* function to draw box on grapics plane */
int draw_grid(unsigned char *buf_addr, int flag, int width, int height, int startX, int startY, int numBlkPerRow)
{
    unsigned int i, j, k;
    unsigned char *p;
    int            lineWidth;
    int            numHoriGridLines;
    int            numVerGridLines;
    int            horiGridSpace;
    int            verGridSpace;
    int            planeWidth;
    int            color;
    int            x; 

    app_grpx_t *grpx = &grpx_obj;

    if(buf_addr==NULL)
        return -1;

    planeWidth  = GRPX_PLANE_GRID_WIDTH;
    numHoriGridLines = 4; /* for 3x3 Grid, Need to make it configurable*/
    numVerGridLines  = 4; /* for 3x3 Grid, Need to make it configurable*/
    horiGridSpace    = width/(numVerGridLines- 1);
    verGridSpace     = height/(numHoriGridLines - 1);
    lineWidth        = 2;  /* In pixels */

    x = planeWidth/width;                 /*Required factor for vertical spacing(last tile is narrower) */
 
    p = (unsigned char *)(buf_addr)+ ((startX * grpx->bytes_per_pixel)+( startY * grpx->bytes_per_pixel * planeWidth));    /* New buffer address according to offsets*/

    if(flag == TRUE)
       color = GRPX_PLANE_GRID_COLOR;
    else
       color = GRPX_PLANE_GRID_COLOR_BLANK;

    for(k=0; k<numHoriGridLines; k++)
    {
       p = (unsigned char *)(buf_addr) 
                           +  ((startX * grpx->bytes_per_pixel)+( startY * grpx->bytes_per_pixel * planeWidth)) /* To go to point where windows starts */
                           +  (k * grpx->bytes_per_pixel * (planeWidth * ((height/(numHoriGridLines - 1)))));   /* To go to next Horizontal Line       */

       /* Except first horizontal line, come back 2 lines (lineWidth) up 
        * in top direction to start drawing line */ 
       if(k !=0)
          p = p - ( grpx->bytes_per_pixel * lineWidth * planeWidth);


        for(i=0; i<lineWidth; i++)
        {
            for(j=0; j<width; j++)
            {
                if(grpx->planeType == GRPX_FORMAT_RGB565)
                {
                    {
                        *p = color;
                        *(p + 1) = color;
                    }
                }
                if(grpx->planeType == GRPX_FORMAT_RGB888)
                {
                    {
                        *p = color;
                        *(p + 1) = color;
                        *(p + 2) = color;
                    }
                }
                if(grpx->planeType == GRPX_FORMAT_ARGB8888)
                {
                    {

                        *p = color;
                        *(p + 1) = color;
                        *(p + 2) = color;
                        *(p + 3) = color;
                    }
                }
                p += grpx->bytes_per_pixel;
             }
             p += grpx->bytes_per_pixel * (planeWidth - width);
         }

    }
    for(k=0; k<numVerGridLines; k++)
    {
#ifdef GRPX_PLANE_GRID_SCD_TILE
       int verSpacing;
       if(k == 3)
       {
         verSpacing = width - lineWidth;
       }
       else
       {
#if 1
         float numBlk;
         int numHorBlkPerGridBox;

         /* Below double operation for ceiling is done to make left 2 colum of  
          * same width and last colum will be adjusted as per resoultion
          * CIF has 11 block per row in SCD and QCIF has 6 block of SCD 
          * In case of CIF colum will be (4 + 4 + 3) and QCIF (2 + 2 + 2) 
          * Example 11%3 = 3.7;  ceil(3.7) = 4               */

         numBlk = ((float)numBlkPerRow)/((float)(numVerGridLines - 1)); /* fractional No. */
         numHorBlkPerGridBox = ceil(numBlk);          /* Round to next integer*/
#else
         int numBlk;
         int numHorBlkPerGridBox;

         /* Below operation for ceiling is done to make left 2 colum of  
          * same width and last colum will be adjusted as per resoultion
          * CIF has 11 block per row in SCD and QCIF has 6 block of SCD 
          * In case of CIF colum will be (4 + 4 + 3) and QCIF (2 + 2 + 2) */

         /* Calcualte remainder and subtract from denominator to get no 
          * which would be added to Numerator to round to next divisible no. 
          * Example 11%3 = 2;  3-2 = 1;    11+1 = 12;    12/3 = 4
          * Example 6%3 = 0;                6+0 = 6;      6/3 = 2 */

         if(numBlkPerRow%(numVerGridLines - 1))
            numBlk =(numVerGridLines - 1) - (numBlkPerRow%(numVerGridLines - 1)); 
         else
            numBlk = 0;

         numHorBlkPerGridBox = (numBlkPerRow + numBlk)/(numVerGridLines - 1);          /* Round to next integer*/

#endif
         /*Offset_2_Jump_2_Nxt_Verlin -> k *   Block Size calc  *  Num Blk per tile  */
         verSpacing = k * ((planeWidth/numBlkPerRow)/x) * numHorBlkPerGridBox;
       }
       p = (unsigned char *)(buf_addr) + (grpx->bytes_per_pixel * verSpacing)            /* New buffer address according to the offsets*/
       + ((startX * grpx->bytes_per_pixel)+( startY * grpx->bytes_per_pixel * planeWidth));
#else
        p = (unsigned char *)(buf_addr) + (grpx->bytes_per_pixel * (k * (width/(numVerGridLines - 1)) - lineWidth))
       + ((startX * grpx->bytes_per_pixel)+( startY * grpx->bytes_per_pixel * planeWidth));
#endif
        for(i=0; i<height; i++)
        {
            for(j=0; j<lineWidth; j++)
            {
                if(grpx->planeType == GRPX_FORMAT_RGB565)
                {
                    {
                        *p = color;
                        *(p + 1) = color;
                    }
                }
                if(grpx->planeType == GRPX_FORMAT_RGB888)
                {
                    {
                        *p = color;
                        *(p + 1) = color;
                        *(p + 2) = color;
                    }
                }
                if(grpx->planeType == GRPX_FORMAT_ARGB8888)
                {
                    {

                        *p = color;
                        *(p + 1) = color;
                        *(p + 2) = color;
                        *(p + 3) = color;
                    }
                }
                p += grpx->bytes_per_pixel;
             }
             p += (grpx->bytes_per_pixel * (planeWidth - lineWidth));
         }
    }
    return 0;

}

/* function to draw box on grapics plane */
int draw_box(unsigned char *buf_addr, int flag,
             int            startX,
             int            startY,
             int            width,
             int            height)
{
    unsigned int i, j, k;
    unsigned char *p;
    int            lineWidth;
    int            numHoriGridLines;
    int            numVerGridLines;
    int            planeWidth;
    int            color;
    app_grpx_t *grpx = &grpx_obj;

    if(buf_addr==NULL)
        return -1;

    planeWidth  = GRPX_PLANE_GRID_WIDTH;

    numHoriGridLines = 2;
    numVerGridLines  = 2;
    lineWidth        = 2;  /* In pixels */

    p = (unsigned char *)(buf_addr) + ((startX * grpx->bytes_per_pixel)+( startY * grpx->bytes_per_pixel * planeWidth));

    if(flag == TRUE)
       color = GRPX_PLANE_GRID_COLOR;
    else
       color = GRPX_PLANE_GRID_COLOR_BLANK;

    for(k=0; k<numHoriGridLines; k++)
    {
        for(i=0; i<lineWidth; i++)
        {
            for(j=0; j<width; j++)
            {
                if(grpx->planeType == GRPX_FORMAT_RGB565)
                {
                    {
                        *p = color;
                        *(p + 1) = color;
                    }
                }
                if(grpx->planeType == GRPX_FORMAT_RGB888)
                {
                    {
                        *p = color;
                        *(p + 1) = color;
                        *(p + 2) = color;
                    }
                }
                if(grpx->planeType == GRPX_FORMAT_ARGB8888)
                {
                    {
                        *p = color;
                        *(p + 1) = color;
                        *(p + 2) = color;
                        *(p + 3) = color;
                    }
                }
                p += grpx->bytes_per_pixel;
             }
             p += (grpx->bytes_per_pixel * (planeWidth - width));
         }
         p += grpx->bytes_per_pixel * (planeWidth * (height - lineWidth));
    }

    for(k=0; k<numVerGridLines; k++)
    {
        p = (unsigned char *)(buf_addr) + \
                  ((((k * width)  + startX) * grpx->bytes_per_pixel) +  \
                       ( startY * grpx->bytes_per_pixel * planeWidth));
        for(i=0; i<height; i++)
        {
            for(j=0; j<lineWidth; j++)
            {
                if(grpx->planeType == GRPX_FORMAT_RGB565)
                {
                    {
                        *p = color;
                        *(p + 1) = color;
                    }
                }
                if(grpx->planeType == GRPX_FORMAT_RGB888)
                {
                    {
                        *p = color;
                        *(p + 1) = color;
                        *(p + 2) = color;
                    }
                }
                if(grpx->planeType == GRPX_FORMAT_ARGB8888)
                {
                    {
                        *p = color;
                        *(p + 1) = color;
                        *(p + 2) = color;
                        *(p + 3) = color;
                    }
                }
                p += grpx->bytes_per_pixel;
             }
             p += (grpx->bytes_per_pixel * (planeWidth - lineWidth));
         }
    }
    return 0;
}

int draw_img(unsigned char *buf_addr,
             unsigned char *img_addr,
             int            sx,
             int            sy,
             int            wi,
             int            ht,
             int            planeWidth)
{
    unsigned int i, j;
    unsigned char *p;
    app_grpx_t *grpx = &grpx_obj;

    if(buf_addr==NULL || img_addr==NULL)
        return -1;

    p = (unsigned char *)(buf_addr + ((sx * grpx->bytes_per_pixel)+( sy * grpx->bytes_per_pixel * planeWidth)));

    for(j=0; j<ht; j++)
    {
        for(i=0; i<wi; i++)
        {
            if(grpx->planeType == GRPX_FORMAT_RGB565)
            {
                {
                    *p = *img_addr;
                    *(p + 1) = *(img_addr + 1);
                }
            }
            if(grpx->planeType == GRPX_FORMAT_RGB888)
            {
                {
                    *p = *img_addr;
                    *(p + 1) = *(img_addr + 1);
                    *(p + 2) = *(img_addr + 2);
                }
            }
            if(grpx->planeType == GRPX_FORMAT_ARGB8888)
            {
                {
                    *p = *img_addr;
                    *(p + 1) = *(img_addr + 1);
                    *(p + 2) = *(img_addr + 2);
                    *(p + 3) = *(img_addr + 3);
                }
            }
            p        += grpx->bytes_per_pixel;
            img_addr += grpx->bytes_per_pixel;
        }
        p += ((planeWidth-wi) * grpx->bytes_per_pixel);
    }

    return 0;
}

int clear_img(unsigned char *buf_addr, int sx, int sy, int wi, int ht, int planeWidth)
{
    unsigned int i, j;
    unsigned char *p;
    app_grpx_t *grpx = &grpx_obj;
    unsigned char *key;


    if(buf_addr==NULL)
        return -1;

    p = (unsigned char *)(buf_addr + ((sx * grpx->bytes_per_pixel)+( sy * grpx->bytes_per_pixel * planeWidth)));
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
            }
            if(grpx->planeType == GRPX_FORMAT_ARGB8888)
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
        p += ((planeWidth-wi) * grpx->bytes_per_pixel);
    }

    return 0;
}

Int32 grpx_fb_scale(VDIS_DEV devId,
                    UInt32   startX,
                    UInt32   startY,
                    UInt32   outWidth,
                    UInt32   outHeight)
{

    struct ti81xxfb_scparams scparams;
    Int32 fd = 0, status = 0;
    app_grpx_t *grpx = &grpx_obj;
    int dummy;
    struct ti81xxfb_region_params  regp;
    char buffer[10];
    int r = -1;

    if (devId == VDIS_DEV_HDMI || devId == VDIS_DEV_DVO2){
        fd = grpx->fd;
        Vdis_isGrpxOn(0, buffer, &r);
    }
    if (devId == VDIS_DEV_SD){
        fd = grpx->fd2;
        Vdis_isGrpxOn(2, buffer, &r);
    }

    /* Set Scalar Params for resolution conversion
     * inHeight and inWidth should remain same based on grpx buffer type
     */

#if defined(GRPX_FB_SEPARATE_BUFFER_NON_TIED_GRPX)
    if (devId == VDIS_DEV_HDMI){
        scparams.inwidth  = GRPX_PLANE_HD_WIDTH;
        scparams.inheight = GRPX_PLANE_HD_HEIGHT;
    }
    if (devId == VDIS_DEV_SD){
        scparams.inwidth  = GRPX_PLANE_SD_WIDTH;
        scparams.inheight = GRPX_PLANE_SD_HEIGHT;
    }
#endif

#if defined(GRPX_FB_SINGLE_BUFFER_TIED_GRPX) || defined (GRPX_FB_SINGLE_BUFFER_NON_TIED_GRPX)
     scparams.inwidth  = GRPX_PLANE_WIDTH;
     scparams.inheight = GRPX_PLANE_HEIGHT;
#endif



    // this "-GRPX_SC_MARGIN_OFFSET" is needed since scaling can result in +2 extra pixels, so we compensate by doing -2 here
    scparams.outwidth = outWidth - GRPX_SC_MARGIN_OFFSET;
    scparams.outheight = outHeight - GRPX_SC_MARGIN_OFFSET;
    scparams.coeff = NULL;

    if (ioctl(fd, TIFB_GET_PARAMS, &regp) < 0) {
        eprintf("TIFB_GET_PARAMS !!!\n");
    }

    regp.pos_x = startX;
    regp.pos_y = startY;
    regp.transen = TI81XXFB_FEATURE_ENABLE;
    regp.transcolor = RGB_KEY_24BIT_GRAY;
    regp.scalaren = TI81XXFB_FEATURE_DISABLE;

    /*not call the IOCTL, ONLY if 100% sure that GRPX is off*/
    if (!((r == 0) && (atoi(buffer) == 0))) {
        if (ioctl(fd, FBIO_WAITFORVSYNC, &dummy)) {
            eprintf("FBIO_WAITFORVSYNC !!!\n");
            return -1;
        }
    }
    if ((status = ioctl(fd, TIFB_SET_SCINFO, &scparams)) < 0) {
        eprintf("TIFB_SET_SCINFO !!!\n");
    }


    if (ioctl(fd, TIFB_SET_PARAMS, &regp) < 0) {
        eprintf("TIFB_SET_PARAMS !!!\n");
    }

    return (status);

}

int grpx_fb_init(grpx_plane_type planeType)
{
    app_grpx_t *grpx = &grpx_obj;

    memset(grpx, 0, sizeof(app_grpx_t));

    // need to start and stop FBDev once for the RGB565 and SC to take effect

    if(planeType >= GRPX_FORMAT_MAX)
    {
        return -1;
    }
    else
    {
        grpx->planeType = planeType;
    }

    /* For TI816x */
#if defined(GRPX_FB_SINGLE_BUFFER_NON_TIED_GRPX)
    grpx_fb_start_singleBufNonTied();
#endif

    /* For TI814x */
#if defined(GRPX_FB_SINGLE_BUFFER_TIED_GRPX)
    grpx_fb_start_singleBufTied();
#endif

    /* For TI8107 */
#if defined (GRPX_FB_SEPARATE_BUFFER_NON_TIED_GRPX)
    grpx_fb_start_separateBufNonTied();
#endif

    system("echo 0 > /sys/devices/platform/vpss/graphics0/enabled");
    //system("echo 1:dvo2 > /sys/devices/platform/vpss/graphics0/nodes");
    system("echo 0:hdmi > /sys/devices/platform/vpss/graphics0/nodes");//guo
    system("echo 1 > /sys/devices/platform/vpss/graphics0/enabled");

    return 0;
}

void grpx_fb_exit(void)
{
    dprintf("\n");
    dprintf("grpx_fb_exit ... \n");

    system("echo 0 > /sys/devices/platform/vpss/graphics0/enabled");

#if defined(GRPX_FB_SINGLE_BUFFER_NON_TIED_GRPX)
    grpx_fb_stop_singleBufNonTied();
#endif

#if defined(GRPX_FB_SINGLE_BUFFER_TIED_GRPX)
    grpx_fb_stop_singleBufTied();
#endif

#if defined (GRPX_FB_SEPARATE_BUFFER_NON_TIED_GRPX)
    grpx_fb_stop_separateBufNonTied();
#endif

    return;
}

int grpx_fb_draw_grid(int width,int height, int startX, int startY, int numBlkPerRow)
{
    VDIS_DEV devId;
    app_grpx_t *grpx = &grpx_obj;

    dprintf("grpx_fb_draw_grid ... \n");

    devId = VDIS_DEV_HDMI;

    if(grpx->planeType >= GRPX_FORMAT_MAX)
    {
        return -1;
    }

    if (devId == VDIS_DEV_HDMI)
    {
        draw_grid(grpx->buf[0], TRUE, width, height, startX, startY, numBlkPerRow);

//       grpx_grid_fb_start_singleBufNonTied();
    }
    dprintf("grpx_fb_draw_grid ... Done !!! \n");

    return 0;
}


int grpx_fb_draw_grid_exit(int width,int height, int startX, int startY, int numBlkPerRow)
{
    dprintf("\n");
    dprintf("grpx_fb_grid_exit ... \n");
    {
        app_grpx_t *grpx = &grpx_obj;

        draw_grid(grpx->buf[0], FALSE, width, height, startX, startY, numBlkPerRow);

        dprintf("grpx_fb_grid_exit ... Done!!!\n");
        dprintf("\n");

     }
//    grpx_grid_fb_stop_singleBufNonTied();

    return 0;
}

//int grpx_fb_draw_box()
int grpx_fb_draw_box(int width,
                     int height,
                     int startX,
                     int startY)

{
    VDIS_DEV devId;
    app_grpx_t *grpx = &grpx_obj;

//    dprintf("grpx_fb_draw_box ... \n");

    devId = VDIS_DEV_HDMI;

    if(grpx->planeType >= GRPX_FORMAT_MAX)
    {
        return -1;
    }

    if (devId == VDIS_DEV_HDMI)
    {
#if 0
       printf(" Width %d height %d startX %d startY %d \n",
                       width, height, startX, startY );
#endif
       draw_box(grpx->buf[0], TRUE,
                 startX,
                 startY,
                 width,
                 height
                 );

//       grpx_grid_fb_start_singleBufNonTied();
    }
//    dprintf("grpx_fb_draw_box ... Done !!! \n");

    return 0;
}

int grpx_fb_draw_box_exit(int width,
                          int height,
                          int startX,
                          int startY)
{
//    dprintf("grpx_fb_box_exit ... \n");
    {
        app_grpx_t *grpx = &grpx_obj;
#if 0
       printf(" Width %d height %d startX %d startY %d \n",
                       width, height, startX, startY );
#endif

        draw_box(grpx->buf[0], FALSE,
                 startX,
                 startY,
                 width,
                 height
                 );
//        dprintf("grpx_fb_box_exit ... Done!!!\n");

     }
//    grpx_grid_fb_stop_singleBufNonTied();

    return 0;
}


#if 0
Int32 grpx_fb_demo()
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

            grpx_fb_scale(devId, startX, startY, outWidth, outHeight);
        }
        for(i=loopCount; i>=1; i--)
        {
            Demo_swMsGetOutSize(Vdis_getResolution(devId), &outWidth, &outHeight);

            startX = offsetX*i;
            startY = offsetY*i;

            outWidth  -= startX*2;
            outHeight -= startY*2;

            grpx_fb_scale(devId, startX, startY, outWidth, outHeight);
        }

        /* restore to original */
        Demo_swMsGetOutSize(Vdis_getResolution(devId), &outWidth, &outHeight);

        dprintf("[reset] %d x %d\n", outWidth, outHeight);
        grpx_fb_scale(devId, 0, 0, outWidth, outHeight);
    }

    return 0;
}

#else

Int32 grpx_fb_draw_demo()
{
    UInt32 devId;
    UInt32 offsetX, offsetY;
    int curOffset = 0;
    int loopCount = 100;
    app_grpx_t *grpx = &grpx_obj;

    devId = VDIS_DEV_DVO2;

    while(loopCount--)
    {

        offsetX = curOffset*10;
        offsetY = curOffset*10;

        /* clear logo's at old position */
        grpx_fb_draw_logo(grpx->buf[0], GRPX_STARTX_0+offsetX, GRPX_STARTY_0+offsetY, grpx->curWidth, TRUE);
        grpx_fb_draw_logo(grpx->buf[0], GRPX_STARTX_1-offsetX, GRPX_STARTY_1+offsetY, grpx->curWidth, TRUE);
        grpx_fb_draw_logo(grpx->buf[0], GRPX_STARTX_2+offsetX, GRPX_STARTY_2-offsetY, grpx->curWidth, TRUE);
        grpx_fb_draw_logo(grpx->buf[0], GRPX_STARTX_3-offsetX, GRPX_STARTY_3-offsetY, grpx->curWidth, TRUE);

        curOffset = (curOffset+1)%20;

        offsetX = curOffset*10;
        offsetY = curOffset*10;

        /* draw logo at new position */
        grpx_fb_draw_logo(grpx->buf[0], GRPX_STARTX_0+offsetX, GRPX_STARTY_0+offsetY, grpx->curWidth, FALSE);
        grpx_fb_draw_logo(grpx->buf[0], GRPX_STARTX_1-offsetX, GRPX_STARTY_1+offsetY, grpx->curWidth, FALSE);
        grpx_fb_draw_logo(grpx->buf[0], GRPX_STARTX_2+offsetX, GRPX_STARTY_2-offsetY, grpx->curWidth, FALSE);
        grpx_fb_draw_logo(grpx->buf[0], GRPX_STARTX_3-offsetX, GRPX_STARTY_3-offsetY, grpx->curWidth, FALSE);

        OSA_waitMsecs(100);
    }

    /* clear logo's at old position */
    grpx_fb_draw_logo(grpx->buf[0], GRPX_STARTX_0+offsetX, GRPX_STARTY_0+offsetY, grpx->curWidth, TRUE);
    grpx_fb_draw_logo(grpx->buf[0], GRPX_STARTX_1-offsetX, GRPX_STARTY_1+offsetY, grpx->curWidth, TRUE);
    grpx_fb_draw_logo(grpx->buf[0], GRPX_STARTX_2+offsetX, GRPX_STARTY_2-offsetY, grpx->curWidth, TRUE);
    grpx_fb_draw_logo(grpx->buf[0], GRPX_STARTX_3-offsetX, GRPX_STARTY_3-offsetY, grpx->curWidth, TRUE);

    offsetX = 0;
    offsetY = 0;

    /* draw logo at original position */
    grpx_fb_draw_logo(grpx->buf[0], GRPX_STARTX_0+offsetX, GRPX_STARTY_0+offsetY, grpx->curWidth, FALSE);
    grpx_fb_draw_logo(grpx->buf[0], GRPX_STARTX_1-offsetX, GRPX_STARTY_1+offsetY, grpx->curWidth, FALSE);
    grpx_fb_draw_logo(grpx->buf[0], GRPX_STARTX_2+offsetX, GRPX_STARTY_2-offsetY, grpx->curWidth, FALSE);
    grpx_fb_draw_logo(grpx->buf[0], GRPX_STARTX_3-offsetX, GRPX_STARTY_3-offsetY, grpx->curWidth, FALSE);

    return 0;
}

Int32 grpx_fb_pcie_demo()
{
    UInt32 devId;
    int loopCount = 100;
    app_grpx_t *grpx = &grpx_obj;
    int dev_fd;
    devId = VDIS_DEV_DVO2;

    int i, ret;
    struct pcie_buffer buffer;
    struct pcie_buffer_info rxbuffer[2];

    dev_fd = open("/dev/pci_ep",O_RDWR);
    if(dev_fd == -1 )
        eprintf("open");

    for (i = 0 ; i < 2 ; i ++) {
        buffer.index = i;
        ret = ioctl(dev_fd,PCI_EP_GET_RXBUFF,&buffer);
        if( ret == -1 )
            eprintf("ioctl");

        rxbuffer[i].index           = i;
        rxbuffer[i].length          = buffer.length;
        rxbuffer[i].physical_addr   = buffer.physical_addr;
        rxbuffer[i].user_addr = (char *)mmap(0,buffer.length,
                    PROT_READ | PROT_WRITE,
                    MAP_SHARED,dev_fd,(off_t)buffer.physical_addr);
    }

    while(loopCount--)
    {
        /** dqueue **/
        ret = ioctl(dev_fd,PCI_EP_DQ_RXBUFF,&buffer);
        if( ret )
            eprintf("ioctl DQ TBUFF");

        draw_img(grpx->buf[0], rxbuffer[buffer.index].user_addr, 0, 0, grpx->curWidth, grpx->curHeight, grpx->curWidth);

        /**enter queue**/
        ret = ioctl(dev_fd,PCI_EP_EQ_RXBUFF,&buffer);
        if( ret == -1 )
            eprintf("ioctl DQ TBUFF");
    }

    for( i = 0 ; i < 2 ; i ++)
        munmap(rxbuffer[i].user_addr,rxbuffer[i].length);

    close(dev_fd);
    return 0;
}

#endif

