#ifndef __TI_MULTICHCOMMON_H__
#define __TI_MULTICHCOMMON_H__

#include <osa.h>

#include "mcfw/src_linux/mcfw_api/ti_vsys_priv.h"

#include "multich_ipcbits.h"
#include "mcfw/interfaces/common_def/ti_vsys_common_def.h"





/* The below part is temporary for OSD specific items */
#define EXAMPLE_OSD_NUM_WINDOWS     (3)
#define EXAMPLE_OSD_WIN_MAX_WIDTH   (320)
#define EXAMPLE_OSD_WIN_MAX_HEIGHT  (64)
#define EXAMPLE_OSD_WIN_WIDTH       (160)
#define EXAMPLE_OSD_WIN_HEIGHT      (32)
#define EXAMPLE_OSD_WIN0_STARTX     (16)
#define EXAMPLE_OSD_WIN0_STARTY     (16)

#define EXAMPLE_OSD_WIN_PITCH       (EXAMPLE_OSD_WIN_WIDTH)
#define EXAMPLE_OSD_TRANSPARENCY    (0)
#define EXAMPLE_OSD_GLOBAL_ALPHA    (0x80)
#define EXAMPLE_OSD_ENABLE_WIN      (1)
#define EXAMPLE_OSD_DISABLE_WIN     (0)

#define OSD_BUF_HEAP_SR_ID          (0)

#define EXAMPLE_SCD_MAX_WIDTH   (360)
#define EXAMPLE_SCD_MAX_HEIGHT  (240)


#define MULTI_CH_MAX_QUE        (4)


Int32 MultiCh_detectBoard();

Int32 MultiCh_memPrintHeapStatus();

Void MultiCh_swMsGetDefaultLayoutPrm(UInt32 devId, SwMsLink_CreateParams *swMsCreateArgs, Bool forceLowCostScaling);

Void MultiCh_setDec2DispMap(VDIS_DEV displayID,
                            UInt32 numChn,
                            UInt32 startDecChn,
                            UInt32 startDisplayChn);
Int32 MultiCh_swMsGetOutSize(UInt32 outRes, UInt32 * width, UInt32 * height);

static inline Int32 MultiCh_DisplayRes_Init(UInt32 displayRes[])
{
    Int32 i;

    /* set display res to a invalid display res */
    for (i = 0; i < SYSTEM_DC_MAX_VENC; i++)
    {
        displayRes[i] = VSYS_STD_MAX;
    }

    displayRes[SYSTEM_DC_VENC_HDMI]   =
            VSYS_STD_1080P_60;
    displayRes[SYSTEM_DC_VENC_HDCOMP] =
            VSYS_STD_1080P_60;
    displayRes[SYSTEM_DC_VENC_DVO2]   =
            VSYS_STD_1080P_60;
    displayRes[SYSTEM_DC_VENC_SD]     =
            VSYS_STD_NTSC;

    return 0;
}

#endif

