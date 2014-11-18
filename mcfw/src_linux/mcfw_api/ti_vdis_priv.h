#ifndef __TI_VDIS_PRIV_H__
#define __TI_VDIS_PRIV_H__

#include "ti_vdis.h"

#include <device.h>
#include <device_videoEncoder.h>
#include <device_sii9022a.h>
#include <mcfw/src_linux/devices/sii9022a/src/sii9022a_priv.h>
#include <sii9022a.h>
#include <thsfilters.h>
#include <osa_mutex.h>
#include <mcfw/interfaces/link_api/avsync.h>


#define VDIS_MOSAIC UInt32

#define TI_VDIS_DEBUG    0
/* =============================================================================
 * Structure
 * =============================================================================
 */
typedef struct
{
    Bool                        initDone;
	UInt32                      displayConfigInitDone;
    UInt32                      swMsId[VDIS_DEV_MAX];
    UInt32                      displayId[VDIS_DEV_MAX];
    UInt32                      grpxId[VDIS_DEV_MAX];
    UInt32                      ipcFramesOutHostId;
    UInt32                      ipcFramesInVpssFromHostId;
    VDIS_PARAMS_S               vdisConfig;
    Device_Sii9022aHandle       sii9022aHandle;
	OSA_MutexHndl               vdisMutexHandle;
    AvsyncLink_LinkSynchConfigParams    avsyncCfg[VDIS_DEV_MAX];
    UInt32                      mpSclrId;

}VDIS_MODULE_CONTEXT_S;

typedef struct
{
    UInt32      ch2WinMap[VDIS_CHN_MAX];
    Bool        isEnableChn[VDIS_CHN_MAX];
}VDIS_CHN_MAP_INFO_S;

extern VDIS_MODULE_CONTEXT_S gVdisModuleContext;


Int32 Vdis_delete();
Int32 Vdis_getDisplayId(VDIS_DEV vdDevId);
Int32 Vdis_getDisplayContextIndex(VDIS_DEV vdDevId);
Int32 Vdis_getSwMsContextIndex(VDIS_DEV vdDevId);

#endif

