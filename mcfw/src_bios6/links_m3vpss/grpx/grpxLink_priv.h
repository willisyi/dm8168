/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#ifndef _GRPX_LINK_PRIV_H_
#define _GRPX_LINK_PRIV_H_

#include <mcfw/src_bios6/links_m3vpss/system/system_priv_m3vpss.h>
#include <mcfw/interfaces/link_api/grpxLink.h>


#define GRPX_LINK_OBJ_MAX (3)

#define GRPX_SC_MARGIN_OFFSET   (2)

#define GRPX_LINK_CMD_DO_DEQUE                (0x0500)


/*
 * The maximum number of frame info is equal to max grpx queue length.
 * This is set to max number of frames queued X 2 since we need to
 * queue frames as two separate fields
 */
#define GRPX_LINK_MAX_FRAMES                  (4)


#define GRPX_LINK_BUFFER_ALIGNMENT            (16*KB)

#define GRPX_LINK_INVALID_INQUEID             (~(0u))

typedef struct {
    UInt32 tskId;

    Utils_TskHndl tsk;

    VGRPX_CREATE_PARAM_S createArgs;

    FVID2_Handle grpxHndl;

    Vps_GrpxCreateParams grpxCreateArgs;
    Vps_GrpxCreateStatus grpxCreateStatus;

    FVID2_Format         grpxFormat;
    Vps_GrpxParamsList   grpxParams;
    Vps_GrpxRtParams     grpxRtParams;
    Vps_GrpxScParams     grpxScParams;

    FVID2_Frame          grpxFrame[GRPX_LINK_MAX_FRAMES];

    Vps_GrpxScCoeff      grpxScCoeff;

    FVID2_FrameList      frameList;

    Bool   freeBufferMem;
    UInt32 bufferMemAddr;
    UInt32 bufferMemSize;

    Bool   rtParamUpdate;

    UInt32 dequeCount;
    UInt32 cbCount;

    UInt32 totalTime;

    UInt32 startTime;
    UInt32 prevTime;
    UInt32 minCbTime;
    UInt32 maxCbTime;
    UInt32 lastCbTime;

    UInt32 maxLatency;
    UInt32 minLatency;

} GrpxLink_Obj;


Int32 GrpxLink_drvCreate(GrpxLink_Obj * pObj,
                            VGRPX_CREATE_PARAM_S * pPrm);
Int32 GrpxLink_drvStart(GrpxLink_Obj * pObj);
Int32 GrpxLink_drvStop(GrpxLink_Obj * pObj);
Int32 GrpxLink_drvDelete(GrpxLink_Obj * pObj);
Int32 GrpxLink_drvPrintStatistics(GrpxLink_Obj * pObj);
Int32 GrpxLink_drvSetDynamicParams(GrpxLink_Obj * pObj, VGRPX_DYNAMIC_PARAM_S *pPrm);
Int32 GrpxLink_drvGetBufferInfo(GrpxLink_Obj * pObj, VGRPX_BUFFER_INFO_S *pPrm);

Int32 GrpxLink_drvFvidCreate(GrpxLink_Obj * pObj);
Int32 GrpxLink_drvProcessFrames(GrpxLink_Obj * pObj);
Int32 GrpxLink_drvAllocAndQueFrame(GrpxLink_Obj * pObj);

#endif
