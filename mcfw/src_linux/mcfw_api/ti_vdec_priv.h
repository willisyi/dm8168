#ifndef __TI_VDEC_PRIV_H__
#define __TI_VDEC_PRIV_H__


#include "ti_vdec.h"
#include "ti_vsys_priv.h"

/* =============================================================================
 * Structure
 * =============================================================================
 */
typedef struct
{
    UInt32 decId;
    UInt32 ipcBitsInRTOSId;
    UInt32 ipcBitsOutHLOSId;
    UInt32 ipcM3OutId;
    UInt32 ipcM3InId;

    VDEC_PARAMS_S vdecConfig;
    UInt32 vdisChIdMap[VDIS_DEV_MAX][VDEC_CHN_MAX];
}VDEC_MODULE_CONTEXT_S;


typedef struct
{
    VDEC_CHN decChnId;
    UInt32   displayChnId;
} VDEC_DISPLAY_CHN_MAP_S;

extern VDEC_MODULE_CONTEXT_S gVdecModuleContext;

Int32 Vdec_create(System_LinkInQueParams *vdecOutQue, UInt32 vdecNextLinkId, Bool tilerEnable, UInt32 numFramesPerCh);
Int32 Vdec_delete();
Int32 Vdec_setDecCh2DisplayChMap(UInt32 displayID, UInt32 numCh, VDEC_DISPLAY_CHN_MAP_S chMap[]);

#endif


