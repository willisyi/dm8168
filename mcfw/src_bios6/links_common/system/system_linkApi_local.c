/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include "system_priv_common.h"

Int32 System_linkControl_local(UInt32 linkId, UInt32 cmd, Void * pPrm,
                               UInt32 prmSize, Bool waitAck)
{
    Int32 status;
    Utils_MbxHndl *pToMbx;
    UInt32 flags = 0;

    linkId = SYSTEM_GET_LINK_ID(linkId);

    UTILS_assert(linkId < SYSTEM_LINK_ID_MAX);

    pToMbx = &gSystem_objCommon.linkObj[linkId].pTsk->mbx;

    if (waitAck)
        flags = UTILS_MBX_FLAG_WAIT_ACK;

    status = Utils_mbxSendMsg(&gSystem_objCommon.mbx, pToMbx, cmd, pPrm, flags);

    return status;
}

Int32 System_sendLinkCmd_local(UInt32 linkId, UInt32 cmd)
{
    Utils_MbxHndl *pToMbx;

    linkId = SYSTEM_GET_LINK_ID(linkId);

    UTILS_assert(linkId < SYSTEM_LINK_ID_MAX);

    pToMbx = &gSystem_objCommon.linkObj[linkId].pTsk->mbx;

    return Utils_mbxSendCmd(pToMbx, cmd);
}

Int32 System_linkGetInfo_local(UInt32 linkId, System_LinkInfo * info)
{
    System_LinkObj *pTsk;

    linkId = SYSTEM_GET_LINK_ID(linkId);

    UTILS_assert(linkId < SYSTEM_LINK_ID_MAX);

    pTsk = &gSystem_objCommon.linkObj[linkId];

    if (pTsk->getLinkInfo != NULL)
        return pTsk->getLinkInfo(pTsk->pTsk, info);

    return FVID2_EFAIL;
}
