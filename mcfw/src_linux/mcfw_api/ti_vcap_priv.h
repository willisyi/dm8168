#ifndef __TI_VCAP_PRIV_H__
#define __TI_VCAP_PRIV_H__


#include "ti_vcap.h"

#include <mcfw/interfaces/link_api/system.h>
#include <mcfw/interfaces/link_api/captureLink.h>
#include <mcfw/interfaces/link_api/deiLink.h>
#include <mcfw/interfaces/link_api/nsfLink.h>
#include <mcfw/interfaces/link_api/algLink.h>

#include <device.h>
#include <device_videoDecoder.h>
#include <device_tvp5158.h>
#include <mcfw/src_linux/devices/tvp5158/src/tvp5158_priv.h>
#include <tvp5158.h>

#define MAX_DEI_LINK    (3)
#define MAX_IPC_FRAMES_LINK    (2)
#define MAX_ALG_LINK    (2)
#define MAX_SCLR_LINK   (2)
#define MAX_NSF_LINK   (4)

/* =============================================================================
 * Structure
 * =============================================================================
 */
typedef struct
{
    UInt32                         captureId;
    UInt32                         dspAlgId[MAX_ALG_LINK];
    UInt32                         nsfId[MAX_NSF_LINK];
    UInt32                         sclrId[MAX_SCLR_LINK];
    UInt32                         deiId[MAX_DEI_LINK];
    UInt32                         ipcFramesInDspId[MAX_IPC_FRAMES_LINK];
    UInt32                         ipcFramesOutVpssId[MAX_IPC_FRAMES_LINK];
    UInt32                         nullSrcId;
    UInt32                         ipcFramesOutVpssToHostId;
    UInt32                         ipcFramesInHostId;
    UInt32                         ipcBitsInHLOSId;
    UInt32                         capSwMsId;
    UInt32                         numChannels;
    VCAP_PARAMS_S                  vcapConfig;
    VCAP_CALLBACK_S                callbackFxn;
    VCAP_CALLBACK_S                bitscallbackFxn;
    Device_Tvp5158Handle           tvp5158Handle[VCAP_DEV_MAX];
    VCAP_VIDEO_SOURCE_STATUS_S     videoStatus;
    Device_VideoDecoderColorParams colorPrm;
    Bool                           isPalMode;
    Ptr callbackArg;
    Ptr bitscallbackArg;
}VCAP_MODULE_CONTEXT_S;

extern VCAP_MODULE_CONTEXT_S gVcapModuleContext;

/**
    \brief Video decoder mode params

    \return NONE
*/

typedef Device_VideoDecoderVideoModeParams VCAP_VIDDEC_PARAMS_S;


Int32 Vcap_delete();
Void Vcap_ipcFramesInCbFxn(Ptr cbCtx);
Int32 Vcap_setExtraFramesChId(UInt32 chId);
Int32 Vcap_configVideoDecoder(VCAP_VIDDEC_PARAMS_S *modeParams, UInt32 numDevices);
Int32 Vcap_deleteVideoDecoder();




#endif

