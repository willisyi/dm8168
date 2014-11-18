/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include "encLink_priv.h"

#pragma DATA_ALIGN(gEncLink_tskStack, 32)
#pragma DATA_SECTION(gEncLink_tskStack, ".bss:taskStackSection")
UInt8 gEncLink_tskStack[ENC_LINK_OBJ_MAX][ENC_LINK_TSK_STACK_SIZE];

#pragma DATA_ALIGN(gEncLink_obj, 32)
#pragma DATA_SECTION(gEncLink_obj, ".bss:gEncLink_objSection")
EncLink_Obj gEncLink_obj[ENC_LINK_OBJ_MAX];

Void EncLink_tskMain(struct Utils_TskHndl *pTsk, Utils_MsgHndl * pMsg)
{
    UInt32 cmd = Utils_msgGetCmd(pMsg);
    Bool ackMsg, done;
    Int32 status;
    EncLink_Obj *pObj;

    pObj = (EncLink_Obj *) pTsk->appData;

    if (cmd != SYSTEM_CMD_CREATE)
    {
        Utils_tskAckOrFreeMsg(pMsg, FVID2_EFAIL);
        return;
    }

    status = EncLink_codecCreate(pObj, Utils_msgGetPrm(pMsg));

    Utils_tskAckOrFreeMsg(pMsg, status);

    if (status != FVID2_SOK)
        return;

    Utils_encdecHdvicpPrfInit();
    done = FALSE;
    ackMsg = FALSE;

    while (!done)
    {
        status = Utils_tskRecvMsg(pTsk, &pMsg, BIOS_WAIT_FOREVER);
        if (status != FVID2_SOK)
            break;

        cmd = Utils_msgGetCmd(pMsg);

        switch (cmd)
        {
            case SYSTEM_CMD_NEW_DATA:
                Utils_tskAckOrFreeMsg(pMsg, status);

                EncLink_codecProcessData(pObj);
                break;

            case ENC_LINK_CMD_GET_PROCESSED_DATA:
                Utils_tskAckOrFreeMsg(pMsg, status);

                EncLink_codecGetProcessedDataMsgHandler(pObj);
                break;
            case ENC_LINK_CMD_GET_CODEC_PARAMS:
                {
                EncLink_GetDynParams *params = {0};

                params = (EncLink_GetDynParams *) Utils_msgGetPrm(pMsg);

                params->inputHeight
                    = pObj->chObj[params->chId].algObj.algDynamicParams.inputHeight;
                params->inputWidth
                    = pObj->chObj[params->chId].algObj.algDynamicParams.inputWidth;
                params->targetBitRate
                    = pObj->chObj[params->chId].algObj.algDynamicParams.targetBitRate;
                params->targetFps
                    = pObj->chObj[params->chId].algObj.algDynamicParams.targetFrameRate;
                params->intraFrameInterval
                    = pObj->chObj[params->chId].algObj.algDynamicParams.intraFrameInterval;

                Utils_tskAckOrFreeMsg(pMsg, status);

                EncLink_codecGetDynParams(pObj, params);
                }
                break;

            case ENC_LINK_CMD_SET_CODEC_BITRATE:
                {
                EncLink_ChBitRateParams *params;

                params = (EncLink_ChBitRateParams *) Utils_msgGetPrm(pMsg);
                EncLink_codecSetBitrate(pObj, params);
                Utils_tskAckOrFreeMsg(pMsg, status);
                }
                break;

           case ENC_LINK_CMD_SET_CODEC_INPUT_FPS:
                {
                EncLink_ChInputFpsParam *params;

                params = (EncLink_ChInputFpsParam *) Utils_msgGetPrm(pMsg);
                EncLink_codecInputSetFps(pObj, params);
                Utils_tskAckOrFreeMsg(pMsg, status);
                }
                break;

            case ENC_LINK_CMD_SET_CODEC_FPS:
                {
                EncLink_ChFpsParams *params;

                params = (EncLink_ChFpsParams *) Utils_msgGetPrm(pMsg);
                EncLink_codecSetFps(pObj, params);
                Utils_tskAckOrFreeMsg(pMsg, status);
                }
                break;
            case ENC_LINK_CMD_SET_CODEC_INTRAI:
                {
                EncLink_ChIntraFrIntParams *params;

                params = (EncLink_ChIntraFrIntParams *) Utils_msgGetPrm(pMsg);
                EncLink_codecSetIntraIRate(pObj, params);
                Utils_tskAckOrFreeMsg(pMsg, status);
                }
                break;
            case ENC_LINK_CMD_SET_CODEC_FORCEI:
                {
                EncLink_ChForceIFrParams *params;

                params = (EncLink_ChForceIFrParams *) Utils_msgGetPrm(pMsg);
                EncLink_codecSetForceIDR(pObj, params);
                Utils_tskAckOrFreeMsg(pMsg, status);
                }
                break;
            case ENC_LINK_CMD_SET_CODEC_RCALGO:
                {
                    EncLink_ChRcAlgParams *params;

                    params = (EncLink_ChRcAlgParams *) Utils_msgGetPrm(pMsg);
                    EncLink_codecSetrcAlg(pObj, params);
                    Utils_tskAckOrFreeMsg(pMsg, status);
                }
                break;
            case ENC_LINK_CMD_SET_CODEC_QP_I:
               {
               EncLink_ChQPParams *params;

               params = (EncLink_ChQPParams *) Utils_msgGetPrm(pMsg);
               EncLink_codecSetqpParamI(pObj, params);
               Utils_tskAckOrFreeMsg(pMsg, status);
               }
               break;
            case ENC_LINK_CMD_SET_CODEC_QP_P:
               {
               EncLink_ChQPParams *params;

               params = (EncLink_ChQPParams *) Utils_msgGetPrm(pMsg);
               EncLink_codecSetqpParamP(pObj, params);
               Utils_tskAckOrFreeMsg(pMsg, status);
               }
               break;
            case ENC_LINK_CMD_SET_CODEC_SNAPSHOT:
               {
                  EncLink_ChannelInfo *params;
                  params = (EncLink_ChannelInfo *) Utils_msgGetPrm(pMsg);
                  EncLink_codecForceDumpFrame(pObj, params);
                  Utils_tskAckOrFreeMsg(pMsg, status);
               }
               break;
            case ENC_LINK_CMD_SET_CODEC_VBRD:
               {
               EncLink_ChCVBRDurationParams *params;

               params = (EncLink_ChCVBRDurationParams *) Utils_msgGetPrm(pMsg);
               EncLink_codecSetVBRDuration(pObj, params);
               Utils_tskAckOrFreeMsg(pMsg, status);
               }
               break;
            case ENC_LINK_CMD_SET_CODEC_VBRS:
               {
               EncLink_ChCVBRSensitivityParams *params;

               params = (EncLink_ChCVBRSensitivityParams *) Utils_msgGetPrm(pMsg);
               EncLink_codecSetVBRSensitivity(pObj, params);
               Utils_tskAckOrFreeMsg(pMsg, status);
               }
               break;
           case ENC_LINK_CMD_SET_CODEC_ROI:
               {
               EncLink_ChROIParams *params;

               params = (EncLink_ChROIParams *) Utils_msgGetPrm(pMsg);
               EncLink_codecSetROIPrms(pObj, params);
               Utils_tskAckOrFreeMsg(pMsg, status);
               }
               break;

            case ENC_LINK_CMD_DISABLE_CHANNEL:
                {
                EncLink_ChannelInfo *params;

                params = (EncLink_ChannelInfo *) Utils_msgGetPrm(pMsg);
                EncLink_codecDisableChannel(pObj, params);
                Utils_tskAckOrFreeMsg(pMsg, status);
                }
                break;
            case ENC_LINK_CMD_ENABLE_CHANNEL:
                {
                EncLink_ChannelInfo *params;

                params = (EncLink_ChannelInfo *) Utils_msgGetPrm(pMsg);
                EncLink_codecEnableChannel(pObj, params);
                Utils_tskAckOrFreeMsg(pMsg, status);
                }
                break;
            case ENC_LINK_CMD_PRINT_IVAHD_STATISTICS:
                Utils_tskAckOrFreeMsg(pMsg, status);

                Utils_encdecHdvicpPrfPrint();
                break;
            case ENC_LINK_CMD_PRINT_STATISTICS:
                EncLink_printStatistics(pObj, TRUE);
                Utils_tskAckOrFreeMsg(pMsg, status);
                break;

            case ENC_LINK_CMD_PRINT_BUFFER_STATISTICS:
                Utils_tskAckOrFreeMsg(pMsg, status);
                EncLink_printBufferStatus(pObj);
                break;

            case ENC_LINK_CMD_INTERNAL_IVAMAPCHANGE:
            {
                EncLink_codecIVAMapChangeHandler(pObj);
                Utils_tskAckOrFreeMsg(pMsg, ENC_LINK_S_SUCCESS);
                break;
            }

            case SYSTEM_CMD_STOP:
                EncLink_codecStop(pObj);
                Utils_tskAckOrFreeMsg(pMsg, status);
                break;

            case SYSTEM_CMD_DELETE:
                EncLink_codecStop(pObj);
                done = TRUE;
                ackMsg = TRUE;
                break;

            default:
                Utils_tskAckOrFreeMsg(pMsg, status);
                break;
        }
    }

    EncLink_codecDelete(pObj);

    if (ackMsg && pMsg != NULL)
        Utils_tskAckOrFreeMsg(pMsg, status);

    return;
}

Int32 EncLink_init()
{
    Int32 status;
    System_LinkObj linkObj;
    EncLink_Obj *pObj;
    char name[32];
    UInt32 objId;

    for (objId = 0; objId < ENC_LINK_OBJ_MAX; objId++)
    {
        pObj = &gEncLink_obj[objId];

        memset(pObj, 0, sizeof(*pObj));
        pObj->linkId = SYSTEM_LINK_ID_VENC_0 + objId;

        linkObj.pTsk = &pObj->tsk;
        linkObj.linkGetFullFrames = NULL;
        linkObj.linkPutEmptyFrames = NULL;
        linkObj.linkGetFullBitBufs = EncLink_getFullBufs;
        linkObj.linkPutEmptyBitBufs = EncLink_putEmptyBufs;
        linkObj.getLinkInfo = EncLink_getInfo;

        sprintf(name, "ENC%d   ", objId);

        System_registerLink(pObj->linkId, &linkObj);

        status = Utils_tskCreate(&pObj->tsk,
                                 EncLink_tskMain,
                                 ENC_LINK_TSK_PRI,
                                 gEncLink_tskStack[objId],
                                 ENC_LINK_TSK_STACK_SIZE, pObj, name);
        UTILS_assert(status == FVID2_SOK);
    }

    return status;
}

Int32 EncLink_deInit()
{
    UInt32 objId;
    EncLink_Obj *pObj;

    for (objId = 0; objId < ENC_LINK_OBJ_MAX; objId++)
    {
        pObj = &gEncLink_obj[objId];

        Utils_tskDelete(&pObj->tsk);
    }

    return FVID2_SOK;
}

Int32 EncLink_getInfo(Utils_TskHndl * pTsk, System_LinkInfo * info)
{
    EncLink_Obj *pObj = (EncLink_Obj *) pTsk->appData;

    memcpy(info, &pObj->info, sizeof(*info));

    return FVID2_SOK;
}

Int32 EncLink_getFullBufs(Utils_TskHndl * pTsk, UInt16 queId,
                          Bitstream_BufList * pBufList)
{
    EncLink_Obj *pObj = (EncLink_Obj *) pTsk->appData;

    UTILS_assert(queId < ENC_LINK_MAX_OUT_QUE);

    return Utils_bitbufGetFull(&pObj->outObj.bufOutQue, pBufList, BIOS_NO_WAIT);
}

Int32 EncLink_putEmptyBufs(Utils_TskHndl * pTsk, UInt16 queId,
                           Bitstream_BufList * pBufList)
{
    EncLink_Obj *pObj = (EncLink_Obj *) pTsk->appData;

    UTILS_assert(queId < ENC_LINK_MAX_OUT_QUE);

    return Utils_bitbufPutEmpty(&pObj->outObj.bufOutQue, pBufList);
}
