/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#ifndef _ALG_LINK_OSD_PRIV_H_
#define _ALG_LINK_OSD_PRIV_H_

#include <mcfw/src_bios6/utils/utils.h>
#include <mcfw/src_bios6/links_c6xdsp/system/system_priv_c6xdsp.h>
#include <mcfw/interfaces/link_api/algLink.h>

#include "sw_osd.h"

typedef struct {

	UInt8 globalAlpha;
	/**< global alpha value for transperency */

	Bool transperencyEnable;
	/**< flag to enable/disable tranperency */

	Bool enableWin;
	/**< flag to enable/disable OSD window */

	UInt8 *addr[2][2];
	/**< Y/C buf Ptr/yuv422 interleaved ptr */

	SWOSD_WindowPrm osdWinPrm;
	/**< osd window parameters. */

} AlgLink_OsdWinObj;

typedef struct {

	UInt8 numWindows;
	/**< number of OSD windows per frame */

	AlgLink_OsdWinObj osdWinObj[ALG_LINK_OSD_MAX_WINDOWS];
	/**< osd window Object. */

    SWOSD_Obj   osdObj;

    UInt32 colorKey[3]; /* for Y, U, V */

} AlgLink_OsdChObj;

typedef struct AlgLink_OsdObj {
    System_LinkQueInfo * inQueInfo;

    AlgLink_OsdChObj chObj[ALG_LINK_OSD_MAX_CH];

    AlgLink_OsdChCreateParams  osdChCreateParams[ALG_LINK_OSD_MAX_CH];

	SWOSD_Obj osdObj;
	/**< handle to OSD algorithm */

} AlgLink_OsdObj;

Int32 AlgLink_OsdalgCreate(AlgLink_OsdObj * pObj);
Int32 AlgLink_OsdalgDelete(AlgLink_OsdObj * pObj);
Int32 AlgLink_OsdalgProcessFrame(AlgLink_OsdObj * pObj, FVID2_Frame *pFrame);
Int32 AlgLink_OsdalgSetChOsdWinPrm(AlgLink_OsdObj * pObj,
                            AlgLink_OsdChWinParams * params);
Int32 AlgLink_OsdalgSetChOsdWinSize(AlgLink_OsdObj * pObj,
                            AlgLink_OsdChWinParams * params);
Int32 AlgLink_OsdalgSetChOsdWinPos(AlgLink_OsdObj * pObj,
                            AlgLink_OsdChWinParams * params);
Int32 AlgLink_OsdalgSetChOsdWinTransparency(AlgLink_OsdObj * pObj,
                            AlgLink_OsdChWinParams * params);
Int32 AlgLink_OsdalgSetChOsdWinAlpha(AlgLink_OsdObj * pObj,
                            AlgLink_OsdChWinParams * params);
Int32 AlgLink_OsdalgSetChOsdWinEnable(AlgLink_OsdObj * pObj,
                            AlgLink_OsdChWinParams * params);
#endif
