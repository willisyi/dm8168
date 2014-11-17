/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2011 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#ifndef _GRAPHIC_SINGLEBUFTIED_H_
#define _GRAPHIC_SINGLEBUFTIED_H_



#define NUM_GRPX_DISPLAYS      (1)

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


Int32 grpx_fb_start_singleBufTied();
Int32 grpx_fb_stop_singleBufTied();
Int32 grpx_fb_draw_singleBufTied();

#endif /*   _GRAPHIC_H_ */

