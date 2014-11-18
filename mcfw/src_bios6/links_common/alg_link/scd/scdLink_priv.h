/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#ifndef _ALG_LINK_SCD_PRIV_H_
#define _ALG_LINK_SCD_PRIV_H_

#include <mcfw/src_bios6/utils/utils.h>
#include <mcfw/src_bios6/utils/utils_dma.h>
#include <mcfw/src_bios6/links_common/system/system_priv_common.h>
#include <mcfw/interfaces/link_api/algLink.h>

#include <mcfw/src_bios6/alg/simcop/inc/simcop_scd.h>



#define ALG_LINK_SIMCOP_SCD_MAX_CH      (48)

#define ALG_LINK_MAX_PROCESS_FRAMES     (2)

#define ALG_LINK_SIMCOP_SCD_WIDTH_ALIGN     (16)

typedef struct {

    UInt32 chId;
    /**< Channel number, 0..ALG_LINK_SIMCOP_SCD_MAX_CH-1 */

    UInt32 inFrameRecvCount;
    UInt32 inFrameProcessCount;
    UInt32 inFrameSkipCount;

    SCD_ProcessPrm          algProcessPrm;
    SCD_ProcessStatus       algProcessStatus;
    SCD_InitMeanVarMHIPrm   algInitMeanVarMHIPrm;
    SCD_AlgImagebufs        algTmpImageBufs;

    AlgLink_ScdOutput       scdStatus;
    AlgLink_ScdOutput       prevScdStatus;

    Utils_frameSkipContext  frameSkipContext;

    Bool                    enableScd;

    Ptr                     memBlockAddr[SCD_MAX_MEM_BLOCKS];

    UInt32                  startTime;
    Bool                    skipInitialFrames;

    FVID2_Frame             processFrames[ALG_LINK_MAX_PROCESS_FRAMES];

    UInt32                  processFrameSize;

    Utils_QueHandle         freeQ;
    FVID2_Frame            *freeQMem[ALG_LINK_MAX_PROCESS_FRAMES];

    UInt32                  width;
    UInt32                  height;
    Bool                    rtPrmUpdate;

    FVID2_Frame             *pPrevProcessFrame;

    AlgLink_ScdMode         scdMode;

    Bool                    algReset;

} AlgLink_ScdChObj;

typedef struct {

    Utils_TskHndl   *pTsk;

    System_LinkQueInfo * inQueInfo;

    AlgLink_ScdChObj chObj[ALG_LINK_SIMCOP_SCD_MAX_CH];

    AlgLink_ScdCreateParams  scdCreateParams;

    SCD_Obj             algObj;
    SCD_MemAllocPrm     algMemAllocPrm;
    SCD_MemAllocPrm     algPerChMemAllocPrm;
    SCD_CreatePrm       algCreatePrm;

    UInt32 statsStartTime;

    UInt32 processFrameCount;
    UInt32 totalTime;

    Utils_QueHandle         processQ;
    FVID2_Frame            *processQMem[ALG_LINK_MAX_PROCESS_FRAMES*ALG_LINK_SIMCOP_SCD_MAX_CH];

    Utils_DmaChObj          dmaCh;

    Utils_TskHndl           processTsk;
    char                    processTskName[32];

    UInt64                  profileLog[SCD_MAX_PROFILE_LOG];

} AlgLink_ScdObj;

Int32 AlgLink_scdAlgCreate(AlgLink_ScdObj * pObj);

Int32 AlgLink_scdAlgDelete(AlgLink_ScdObj * pObj);

Int32 AlgLink_scdAlgProcessFrames(Utils_TskHndl *pTsk, AlgLink_ScdObj * pObj,
                                FVID2_FrameList *pFrameList
                                );

Int32 AlgLink_scdAlgPrintStatistics(AlgLink_ScdObj * pObj, Bool resetAfterPrint);

Int32 AlgLink_scdAlgGetAllChFrameStatus(AlgLink_ScdObj * pObj, AlgLink_ScdAllChFrameStatus *pPrm);

Int32 AlgLink_scdAlgSetChMode(AlgLink_ScdObj * pObj, AlgLink_ScdChParams *pPrm);
Int32 AlgLink_scdAlgSetChIgnoreLightsOn(AlgLink_ScdObj * pObj, AlgLink_ScdChParams *pPrm);
Int32 AlgLink_scdAlgSetChIgnoreLightsOff(AlgLink_ScdObj * pObj, AlgLink_ScdChParams *pPrm);
Int32 AlgLink_scdAlgSetChSensitivity(AlgLink_ScdObj * pObj, AlgLink_ScdChParams *pPrm);
Int32 AlgLink_scdAlgResetCh(AlgLink_ScdObj * pObj, AlgLink_ScdChCtrl *pPrm);

Int32 AlgLink_scdAlgProcessTskInit(AlgLink_ScdObj *pObj, UInt32 objId);
Int32 AlgLink_scdAlgProcessTskDeInit(AlgLink_ScdObj *pObj);
Int32 AlgLink_scdAlgProcessTskSendCmd(AlgLink_ScdObj *pObj, UInt32 cmd);



#endif
