/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2011 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#ifndef _GRAPHIC_PRIV_H_
#define _GRAPHIC_PRIV_H_

#include "mcfw/interfaces/ti_vdis.h"

#define FBDEV_NAME_0            "/dev/fb0"
#define FBDEV_NAME_2            "/dev/fb2"

#define eprintf(str,args...) printf(" [FBDEV] ERROR: " str, ## args);
#define dprintf(str,args...) printf(" [FBDEV] " str, ## args);

#define RGB_KEY_24BIT_GRAY   0x00FFFFFF
#define RGB_KEY_16BIT_GRAY   0xFFFF

#define NUM_FBDEV_BUFS  3

typedef struct {
    int fd;
    int fd2;
    unsigned char *buf[NUM_FBDEV_BUFS];
    unsigned char *phyBuf[NUM_FBDEV_BUFS];
    unsigned int bytes_per_pixel;
    int buf_size[NUM_FBDEV_BUFS];
    grpx_plane_type planeType;
    int curWidth;
    int curHeight;

} app_grpx_t;

app_grpx_t grpx_obj;

int draw_fill_color(unsigned char *buf_addr, int curWidth, int curHeight);
int draw_img(unsigned char *buf_addr, 
             unsigned char *img_addr, 
             int            sx, 
             int            sy, 
             int            wi, 
             int            ht,
			 int            planeWidth);
int disp_getregparams(int display_fd);
int disp_fbinfo(int fd);
int draw_grid(unsigned char *buf_addr, int flag,
                  int startX, int startY, int width, int height, int numBlkPerRow);
int draw_box(unsigned char *buf_addr, int flag,
                  int startX, int startY, int width, int height);
#endif /*   _GRAPHIC_H_ */

