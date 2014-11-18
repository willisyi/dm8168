/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#ifndef _ALG_LINK_OSD_PRIV_H_
#define _ALG_LINK_OSD_PRIV_H_

#include <mcfw/src_bios6/utils/utils.h>
#include <mcfw/src_bios6/utils/utils_dma.h>
#include <mcfw/src_bios6/links_m3vpss/system/system_priv_m3vpss.h>
#include <mcfw/interfaces/link_api/algLink.h>

#include <mcfw/src_bios6/alg/simcop/inc/sw_osd.h>

#define ALG_LINK_OSD_MAX_FRAMES_PER_CH           (10)

#define ALG_LINK_OSD_MAX_VALID_CH                (SWOSD_MAX_CHANNELS)

#define ALG_LINK_OSD_MIN_WIN_WIDTH               (4)
#define ALG_LINK_OSD_MIN_WIN_HEIGHT              (2)

typedef struct {

    SWOSD_DynamicPrm    algDynamicPrm;

    Utils_QueHandle inQue;

    FVID2_Frame *inQueMem[ALG_LINK_OSD_MAX_FRAMES_PER_CH];

    UInt32 inFrameRecvCount;
    UInt32 inFrameQueCount;
    UInt32 inFrameProcessCount;
    UInt32 inFrameRejectCount;

    UInt32          numBlindDmaWin;
    Utils_DmaFill2D blindWinDmaPrm[ALG_LINK_OSD_MAX_WINDOWS];

} AlgLink_OsdChObj;

typedef struct {

    System_LinkQueInfo * inQueInfo;

    UInt32               numValidOsdCh;

    AlgLink_OsdChObj chObj[ALG_LINK_OSD_MAX_CH];

    AlgLink_OsdChCreateParams  osdChCreateParams[ALG_LINK_OSD_MAX_CH];

    SWOSD_Obj           algObj;
    SWOSD_MemAllocPrm   algMemAllocPrm;
    SWOSD_CreatePrm     algCreatePrm;

    SWOSD_BlendFramePrm algBlendFramePrm;

    UInt32 statsStartTime;

    UInt32 processFrameCount;
    UInt32 totalTime;

    Utils_DmaChObj dmaObj;

    UInt64  profileLog[SWOSD_MAX_PROFILE_LOG];

} AlgLink_OsdObj;

Int32 AlgLink_osdAlgCreate(AlgLink_OsdObj * pObj);

Int32 AlgLink_osdAlgDelete(AlgLink_OsdObj * pObj);

Int32 AlgLink_osdAlgProcessFrames(Utils_TskHndl *pTsk, AlgLink_OsdObj * pObj,
                                FVID2_FrameList *pFrameList
                                );

Int32 AlgLink_osdAlgSetChOsdWinPrm(AlgLink_OsdObj * pObj,
                            AlgLink_OsdChWinParams * params,
                            Bool callAlgApi
                            );

Int32 AlgLink_osdAlgSetChOsdBlindWinPrm(AlgLink_OsdObj * pObj,
                            AlgLink_OsdChBlindWinParams * params);

Int32 AlgLink_osdAlgPrintStatistics(AlgLink_OsdObj * pObj, Bool resetAfterPrint);

/* User provides global alpha from 0..0x80
    but simcop expects global alpha from 0..0xff
    so scale user supplied value to simcop expected value
*/
static inline UInt32 AlgLink_osdAlgScaleGlobalAlpha(UInt32 userGlobalAlpha)
{
    if(userGlobalAlpha>=0x80)
        return 0xFF;

    return userGlobalAlpha*2;
}

#endif
