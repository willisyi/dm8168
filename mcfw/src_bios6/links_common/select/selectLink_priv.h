/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#ifndef _SELECT_LINK_PRIV_H_
#define _SELECT_LINK_PRIV_H_

#include <mcfw/src_bios6/links_common/system/system_priv_common.h>
#include <mcfw/interfaces/link_api/selectLink.h>

#define SELECT_LINK_OBJ_MAX   (4)

#define SELECT_LINK_CH_NOT_MAPPED   (0xFFFF)

typedef struct {

    UInt32 queId;
    UInt32 outChNum;

    Bool   rtChInfoUpdate;

    System_LinkChInfo rtChInfo;

} SelectLink_ChInfo;

typedef struct {
    UInt32 tskId;

    Utils_TskHndl tsk;

    SelectLink_CreateParams createArgs;

    System_LinkInfo inTskInfo;

    System_LinkInfo info;

    Utils_BufHndl outFrameQue[SELECT_LINK_MAX_OUT_QUE];

    SelectLink_ChInfo   inChInfo[SYSTEM_MAX_CH_PER_OUT_QUE];

    SelectLink_OutQueChInfo   prevOutQueChInfo[SELECT_LINK_MAX_OUT_QUE];

} SelectLink_Obj;


Int32 SelectLink_drvSetOutQueChInfo(SelectLink_Obj * pObj, SelectLink_OutQueChInfo *pPrm);
Int32 SelectLink_drvGetOutQueChInfo(SelectLink_Obj * pObj, SelectLink_OutQueChInfo *pPrm);

#endif
