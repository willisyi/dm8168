/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2011 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#ifndef _GRAPHIC_H_
#define _GRAPHIC_H_

#include "mcfw/interfaces/ti_vdis.h"

#define GRPX_PLANE_GRID_WIDTH    (1920)
#define GRPX_PLANE_GRID_HEIGHT   (1080)

#define GRPX_PLANE_GRID_COLOR           (0) //(0xFF)
#define GRPX_PLANE_GRID_COLOR_BLANK     (0xFF)

/* In this mode 3x3 tile is made where in 1st two coloum are of same width and 
 * last colum is of lesser width. This limitation is from SCD block size i.e. 32 and no. of blocks which are there in CIF stream*/
#define GRPX_PLANE_GRID_SCD_TILE 1

typedef enum {
    GRPX_FORMAT_RGB565 = 0,
    GRPX_FORMAT_RGB888 = 1,
    GRPX_FORMAT_ARGB8888 = 2,
    GRPX_FORMAT_MAX

}grpx_plane_type;

int grpx_fb_init(grpx_plane_type planeType);
void grpx_fb_exit(void);

int grpx_fb_draw_grid(int width, int height, int startX, int startY, int numBlkPerRow);
int grpx_fb_draw_grid_exit(int width, int height, int startX, int startY, int numBlkPerRow);

int grpx_fb_draw_box(
                    int width,
                    int height,
                    int startX,
                    int startY);
                    
int grpx_fb_draw_box_exit(
                            int width,
                            int height,
                            int startX,
                            int startY);

Int32 grpx_fb_scale(VDIS_DEV devId, 
				   UInt32 startX, 
				   UInt32 startY, 
				   UInt32 outWidth, 
				   UInt32 outHeight);
//Int32 grpx_fb_demo();

int grpx_link_init(grpx_plane_type planeType);
void grpx_link_exit(void);
int grpx_link_draw(VDIS_DEV devId);
Int32 grpx_link_scale(VDIS_DEV devId, 
 					  UInt32 startX, 
 					  UInt32 startY,
					  UInt32 outWidth, 
					  UInt32 outHeight);
// Int32 grpx_link_demo();

#if 1

#define grpx_init            grpx_fb_init
#define grpx_exit            grpx_fb_exit
#define grpx_scale           grpx_fb_scale
#define grpx_draw_grid       grpx_fb_draw_grid
#define grpx_draw_grid_exit  grpx_fb_draw_grid_exit

#define grpx_draw_box        grpx_fb_draw_box
#define grpx_draw_box_exit   grpx_fb_draw_box_exit

#else

#define grpx_init   grpx_link_init
#define grpx_exit   grpx_link_exit
#define grpx_draw   grpx_link_draw
#define grpx_scale  grpx_link_scale
#define grpx_demo   grpx_link_demo


#endif

#endif /*   _GRAPHIC_H_ */

