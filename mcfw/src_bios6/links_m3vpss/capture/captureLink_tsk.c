/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
    This file implements the state machine logic for this link.
    A message command will cause the state machine to take some action and then
    move to a different state.

    The state machine table is as shown below

    Cmds| CREATE | DETECT_VIDEO | START | NEW_DATA  | STOP   | DELETE |
 States |========|==============|=======|===========|========|========|
   IDLE | READY  | -            | -     | -         | -      | -      |
   READY| -      | READY        | RUN   | -         | READY  | IDLE   |
   RUN  | -      | -            | -     | RUN       | READY  | IDLE   |

 */

#include "captureLink_priv.h"

/* link stack */
#pragma DATA_ALIGN(gCaptureLink_tskStack, 32)
#pragma DATA_SECTION(gCaptureLink_tskStack, ".bss:taskStackSection")
UInt8 gCaptureLink_tskStack[CAPTURE_LINK_TSK_STACK_SIZE];

/* link object, stores all link related information */
CaptureLink_Obj gCaptureLink_obj;

/*
 * Run state implementation
 *
 * In this state link captures frames from VIP ports and sends it to the next
 * link. */
Int32 CaptureLink_tskRun(CaptureLink_Obj * pObj, Utils_TskHndl * pTsk,
                         Utils_MsgHndl ** pMsg, Bool * done, Bool * ackMsg)
{
    Int32 status = FVID2_SOK;
    Bool runDone, runAckMsg;
    Utils_MsgHndl *pRunMsg;
    UInt32 cmd, flushCmds[2];
    CaptureLink_ColorParams *params;
    Capture_AudioModeParams*audparams;
    CaptureLink_BlindInfo *blindInfo;
    
    /* READY loop done and ackMsg status */
    *done = FALSE;
    *ackMsg = FALSE;
    *pMsg = NULL;

    /* RUN loop done and ackMsg status */
    runDone = FALSE;
    runAckMsg = FALSE;

    /* RUN state loop */
    while (!runDone)
    {
        /* wait for message */
        status = Utils_tskRecvMsg(pTsk, &pRunMsg, BIOS_WAIT_FOREVER);
        if (status != FVID2_SOK)
            break;

        /* extract message command from message */
        cmd = Utils_msgGetCmd(pRunMsg);

        switch (cmd)
        {
            case SYSTEM_CMD_NEW_DATA:
                /* new data frames have been captured, process them */

                /* ACK or free message before proceding */
                Utils_tskAckOrFreeMsg(pRunMsg, status);

                flushCmds[0] = SYSTEM_CMD_NEW_DATA;
                Utils_tskFlushMsg(pTsk, flushCmds, 1);

                status = CaptureLink_drvProcessData(pObj);
                if (status != FVID2_SOK)
                {
                    /* in case of error exit RUN loop */
                    runDone = TRUE;

                    /* since message is already ACK'ed or free'ed do not ACK
                     * or free it again */
                    runAckMsg = FALSE;
                }
                break;

            case CAPTURE_LINK_CMD_FORCE_RESET:
                /* new data frames have been captured, process them */

                /* ACK or free message before proceding */
                Utils_tskAckOrFreeMsg(pRunMsg, status);

                CaptureLink_drvOverflowDetectAndReset(pObj, TRUE);
                break;

            case CAPTURE_LINK_CMD_PRINT_ADV_STATISTICS:
                /* new data frames have been captured, process them */

                /* ACK or free message before proceding */
                Utils_tskAckOrFreeMsg(pRunMsg, status);

                CaptureLink_drvPrintStatus(pObj);
                break;

            case CAPTURE_LINK_CMD_PRINT_BUFFER_STATISTICS:
                Utils_tskAckOrFreeMsg(pRunMsg, status);
                CaptureLink_printBufferStatus(pObj);
                break;

            case CAPTURE_LINK_CMD_CHANGE_BRIGHTNESS:

                params = (CaptureLink_ColorParams *)Utils_msgGetPrm(pRunMsg);

                /* ACK or free message before proceding */
                Utils_tskAckOrFreeMsg(pRunMsg, status);

                CaptureLink_drvSetColor(pObj, pObj->contrast, params->brightness, pObj->saturation, pObj->hue, params->chId);
                break;

            case CAPTURE_LINK_CMD_CHANGE_CONTRAST:

                params = (CaptureLink_ColorParams *)Utils_msgGetPrm(pRunMsg);

                /* ACK or free message before proceding */
                Utils_tskAckOrFreeMsg(pRunMsg, status);

                CaptureLink_drvSetColor(pObj, params->contrast, pObj->brightness, pObj->saturation, pObj->hue, params->chId);
                break;

            case CAPTURE_LINK_CMD_CHANGE_SATURATION:

                params = (CaptureLink_ColorParams *)Utils_msgGetPrm(pRunMsg);

                /* ACK or free message before proceding */
                Utils_tskAckOrFreeMsg(pRunMsg, status);

                CaptureLink_drvSetColor(pObj, pObj->contrast, pObj->brightness, params->satauration, pObj->hue, params->chId);
                break;

            case CAPTURE_LINK_CMD_CHANGE_HUE:

                params = (CaptureLink_ColorParams *)Utils_msgGetPrm(pRunMsg);

                /* ACK or free message before proceding */
                Utils_tskAckOrFreeMsg(pRunMsg, status);

                CaptureLink_drvSetColor(pObj, pObj->contrast, pObj->brightness, pObj->saturation, params->hue, params->chId);
                break;

            case CAPTURE_LINK_CMD_DETECT_VIDEO:

                Utils_tskAckOrFreeMsg(pRunMsg, status);

                /* detect video source, remain in READY state */
                CaptureLink_drvDetectVideo(pObj, FALSE, TRUE);
                break;

            case CAPTURE_LINK_CMD_GET_VIDEO_STATUS:

                status = CaptureLink_drvGetVideoStatus(pObj, Utils_msgGetPrm(pRunMsg));
                Utils_tskAckOrFreeMsg(pRunMsg, status);

                break;


            case CAPTURE_LINK_CMD_SET_AUDIO_CODEC_PARAMS:

                audparams = (Capture_AudioModeParams *)Utils_msgGetPrm(pRunMsg);

                status = CaptureLink_drvSetAudioParams(pObj, audparams);
                Utils_tskAckOrFreeMsg(pRunMsg, status);

                break;
            case CAPTURE_LINK_CMD_CONFIGURE_BLIND_AREA:
                blindInfo = (CaptureLink_BlindInfo *)Utils_msgGetPrm(pRunMsg);

                CapterLink_drvBlindAreaConfigure(pObj, blindInfo);

                /* ACK or free message before proceding */
                Utils_tskAckOrFreeMsg(pRunMsg, status);

                break;

            case CAPTURE_LINK_CMD_SKIP_ODD_FIELDS:

                status = CaptureLink_drvSkipOddFields(
                            pObj,
                            (CaptureLink_SkipOddFields *)Utils_msgGetPrm(pRunMsg)
                            );
                Utils_tskAckOrFreeMsg(pRunMsg, status);
                break;


            case SYSTEM_CMD_STOP:
                /* stop RUN loop and goto READY state */
                runDone = TRUE;

                /* ACK message after actually stopping the driver outside the
                 * RUN loop */
                runAckMsg = TRUE;
                break;

            case CAPTURE_LINK_CMD_SET_EXTRA_FRAMES_CH_ID:
                status = CaptureLink_drvSetExtraFramesChId(
                                pObj,
                                Utils_msgGetPrm(pRunMsg)
                            );
                Utils_tskAckOrFreeMsg(pRunMsg, status);
                break;

            case SYSTEM_CMD_DELETE:

                /* stop RUN loop and goto IDLE state */

                /* exit RUN loop */
                runDone = TRUE;

                /* exit READY loop */
                *done = TRUE;

                /* ACK message after exiting READY loop */
                *ackMsg = TRUE;

                /* Pass the received message to the READY loop */
                *pMsg = pRunMsg;

                break;

            default:

                /* invalid command for this state ACK it and continue RUN
                 * loop */
                Utils_tskAckOrFreeMsg(pRunMsg, status);
                break;
        }

    }

    /* RUN loop exited, stop driver */
    CaptureLink_drvStop(pObj);

    /* ACK message if not ACKed earlier */
    if (runAckMsg)
        Utils_tskAckOrFreeMsg(pRunMsg, status);

    return status;
}

/* IDLE and READY state implementation */
Void CaptureLink_tskMain(struct Utils_TskHndl * pTsk, Utils_MsgHndl * pMsg)
{
    UInt32 cmd = Utils_msgGetCmd(pMsg);
    Bool ackMsg, done;
    Int32 status;
    CaptureLink_Obj *pObj;
    CaptureLink_BlindInfo *blindInfo;
    
    /* IDLE state */

    pObj = (CaptureLink_Obj *) pTsk->appData;

    if (cmd != SYSTEM_CMD_CREATE)
    {
        /* invalid command recived in IDLE status, be in IDLE state and ACK
         * with error status */
        Utils_tskAckOrFreeMsg(pMsg, FVID2_EFAIL);
        return;
    }

    /* Create command received, create the driver */

    status = CaptureLink_drvCreate(pObj, Utils_msgGetPrm(pMsg));

    if(status==FVID2_SOK)
    {
        /* detect video source, remain in READY state */
        status =
            CaptureLink_drvConfigureVideoDecoder(pObj,
                                   (UInt32) BIOS_WAIT_FOREVER);
    }

    /* ACK based on create status */
    Utils_tskAckOrFreeMsg(pMsg, status);

    /* if create status is error then remain in IDLE state */
    if (status != FVID2_SOK)
        return;

    /* create success, entering READY state */

    done = FALSE;
    ackMsg = FALSE;

    /* READY state loop */
    while (!done)
    {
        /* wait for message */
        status = Utils_tskRecvMsg(pTsk, &pMsg, BIOS_WAIT_FOREVER);
        if (status != FVID2_SOK)
            break;

        /* extract message command from message */
        cmd = Utils_msgGetCmd(pMsg);

        switch (cmd)
        {
            case CAPTURE_LINK_CMD_CONFIGURE_VIP_DECODERS:
                /* This is done after create itself and not here */
                Utils_tskAckOrFreeMsg(pMsg, status);
                break;

            case CAPTURE_LINK_CMD_GET_VIDEO_STATUS:

                status = CaptureLink_drvGetVideoStatus(pObj, Utils_msgGetPrm(pMsg));
                Utils_tskAckOrFreeMsg(pMsg, status);
                break;

            case CAPTURE_LINK_CMD_SKIP_ODD_FIELDS:

                status = CaptureLink_drvSkipOddFields(
                            pObj,
                            (CaptureLink_SkipOddFields *)Utils_msgGetPrm(pMsg)
                            );
                Utils_tskAckOrFreeMsg(pMsg, status);
                break;

            case SYSTEM_CMD_START:
                /* Start capture driver */
                status = CaptureLink_drvStart(pObj);

                /* ACK based on create status */
                Utils_tskAckOrFreeMsg(pMsg, status);

                /* if start status is error then remain in READY state */
                if (status == FVID2_SOK)
                {
                    /* start success, entering RUN state */
                    status =
                        CaptureLink_tskRun(pObj, pTsk, &pMsg, &done, &ackMsg);

                    /** done = FALSE, exit RUN state
                       done = TRUE, exit RUN and READY state
                    */
                }

                break;

            case CAPTURE_LINK_CMD_CONFIGURE_BLIND_AREA:
                blindInfo = (CaptureLink_BlindInfo *)Utils_msgGetPrm(pMsg);

                CapterLink_drvBlindAreaConfigure(pObj, blindInfo);

                /* ACK or free message before proceding */
                Utils_tskAckOrFreeMsg(pMsg, status);

                break;

            case SYSTEM_CMD_DELETE:

                /* exit READY state */
                done = TRUE;
                ackMsg = TRUE;
                break;
            default:
                /* invalid command for this state ACK it and continue READY
                 * loop */
                Utils_tskAckOrFreeMsg(pMsg, status);
                break;
        }
    }

    /* exiting READY state, delete driver */
    CaptureLink_drvDelete(pObj);

    /* ACK message if not previously ACK'ed */
    if (ackMsg && pMsg != NULL)
        Utils_tskAckOrFreeMsg(pMsg, status);

    /* entering IDLE state */
    return;
}

Int32 CaptureLink_init()
{
    Int32 status;
    System_LinkObj linkObj;
    CaptureLink_Obj *pObj;

    /* register link with system API */

    pObj = &gCaptureLink_obj;

    memset(pObj, 0, sizeof(*pObj));

    linkObj.pTsk = &pObj->tsk;
    linkObj.linkGetFullFrames = CaptureLink_getFullFrames;
    linkObj.linkPutEmptyFrames = CaptureLink_putEmptyFrames;
    linkObj.getLinkInfo = CaptureLink_getInfo;

    System_registerLink(SYSTEM_LINK_ID_CAPTURE, &linkObj);

    /** Create link task, task remains in IDLE state
        CaptureLink_tskMain is called when a message command is received
    */

    status = Utils_tskCreate(&pObj->tsk,
                             CaptureLink_tskMain,
                             CAPTURE_LINK_TSK_PRI,
                             gCaptureLink_tskStack,
                             CAPTURE_LINK_TSK_STACK_SIZE, pObj, "CAPTURE ");
    UTILS_assert(status == FVID2_SOK);

    return status;
}

Int32 CaptureLink_deInit()
{
    CaptureLink_Obj *pObj;

    pObj = &gCaptureLink_obj;

    /*
     * Delete link task */
    Utils_tskDelete(&pObj->tsk);

    return FVID2_SOK;
}

/* return capture link info to the next link calling this API via system API */
Int32 CaptureLink_getInfo(Utils_TskHndl * pTsk, System_LinkInfo * info)
{
    CaptureLink_Obj *pObj = (CaptureLink_Obj *) pTsk->appData;

    memcpy(info, &pObj->info, sizeof(*info));

    return FVID2_SOK;
}

/* return captured frames to the next link calling this API via system API */
Int32 CaptureLink_getFullFrames(Utils_TskHndl * pTsk, UInt16 queId,
                                FVID2_FrameList * pFrameList)
{
    CaptureLink_Obj *pObj = (CaptureLink_Obj *) pTsk->appData;
    Int32 status;

    UTILS_assert(queId < CAPTURE_LINK_MAX_OUT_QUE);

    status =  Utils_bufGetFull(&pObj->bufQue[queId], pFrameList, BIOS_NO_WAIT);
    if (status == 0)
    {
        CaptureLink_drvSetFrameCropBufPtr(pObj, pFrameList);
    }
    return status;
}

/* release captured frames to driver when the next link calls this API via
 * system API */
Int32 CaptureLink_putEmptyFrames(Utils_TskHndl * pTsk, UInt16 queId,
                                 FVID2_FrameList * pFrameList)
{
    CaptureLink_Obj *pObj = (CaptureLink_Obj *) pTsk->appData;

    UTILS_assert(queId < CAPTURE_LINK_MAX_OUT_QUE);

    CaptureLink_drvReSetFrameCropBufPtr(pObj, pFrameList);
    return CaptureLink_drvPutEmptyFrames(pObj, pFrameList);
}
