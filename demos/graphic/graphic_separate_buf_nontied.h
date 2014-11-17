/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2011 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#ifndef _GRAPHIC_SEPARATEBUFNONTIED_H_
#define _GRAPHIC_SEPARATEBUFNONTIED_H_

#define NUM_GRPX_DISPLAYS      (2)

#define GRPX_PLANE_HD_WIDTH    (1280)
#define GRPX_PLANE_HD_HEIGHT   ( 720)

#define GRPX_HD_STARTX_0        (50u)
#define GRPX_HD_STARTY_0        (20u)
#define GRPX_HD_STARTX_1        (GRPX_PLANE_HD_WIDTH-250)
#define GRPX_HD_STARTY_1        (GRPX_HD_STARTY_0)
#define GRPX_HD_STARTX_2        (GRPX_HD_STARTX_0)
#define GRPX_HD_STARTY_2        (GRPX_PLANE_HD_HEIGHT-75)
#define GRPX_HD_STARTX_3        (GRPX_HD_STARTX_1)
#define GRPX_HD_STARTY_3        (GRPX_HD_STARTY_2)


#define GRPX_PLANE_SD_WIDTH    (720)
#define GRPX_PLANE_SD_HEIGHT   (480)


#define GRPX_SD_STARTX_0        (50u)
#define GRPX_SD_STARTY_0        (20u)
#define GRPX_SD_STARTX_1        (GRPX_PLANE_SD_WIDTH-275)
#define GRPX_SD_STARTY_1        (GRPX_SD_STARTY_0)
#define GRPX_SD_STARTX_2        (GRPX_SD_STARTX_0)
#define GRPX_SD_STARTY_2        (GRPX_PLANE_SD_HEIGHT- 50)
#define GRPX_SD_STARTX_3        (GRPX_SD_STARTX_1)
#define GRPX_SD_STARTY_3        (GRPX_SD_STARTY_2)


Int32 grpx_fb_start_separateBufNonTied();
Int32 grpx_fb_stop_separateBufNonTied();
Int32 grpx_fb_draw_separateNonTied();

#endif /*   _GRAPHIC_H_ */

