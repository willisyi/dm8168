/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2011 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/
#include <osa.h>
#include <mcfw/interfaces/ti_venc.h>
#include <mcfw/interfaces/ti_vcap.h>
#include <mcfw/interfaces/link_api/ipcLink.h>

#define MULTICH_IPCBITS_NO_NOTIFY_BITSINHLOS                           (TRUE)
#define MULTICH_IPCBITS_NO_NOTIFY_BITSOUTHLOS                          (TRUE)

Void MultiCh_ipcBitsInitCreateParams_BitsInHLOS(IpcBitsInLinkHLOS_CreateParams *cp)
{
    VENC_CALLBACK_S *callback;
    Ptr cbCtx;

    cp->baseCreateParams.noNotifyMode = MULTICH_IPCBITS_NO_NOTIFY_BITSINHLOS;
    Venc_getCallbackInfo(&callback, &cbCtx);
    cp->cbFxn = callback->newDataAvailableCb;
    cp->cbCtx = cbCtx;
    cp->baseCreateParams.notifyNextLink = FALSE;
    /* Previous link of bitsInHLOS is bitsOutRTOS. So, notifyPrevLink
     * should be set to false if bitsInHLOS is to operate in
     * NO_NOTIFY_MODE
     */
    cp->baseCreateParams.notifyPrevLink = !(MULTICH_IPCBITS_NO_NOTIFY_BITSINHLOS);
}


Void MultiCh_ipcBitsInitCreateParams_BitsInHLOSVcap(IpcBitsInLinkHLOS_CreateParams *cp)
{
    VCAP_CALLBACK_S *callback;
    Ptr cbCtx;

    cp->baseCreateParams.noNotifyMode = MULTICH_IPCBITS_NO_NOTIFY_BITSINHLOS;
    Vcap_getBitsCallbackInfo(&callback, &cbCtx);
    cp->cbFxn = callback->newDataAvailableCb;
    cp->cbCtx = cbCtx;
    cp->baseCreateParams.notifyNextLink = FALSE;
    /* Previous link of bitsInHLOS is bitsOutRTOS. So, notifyPrevLink
     * should be set to false if bitsInHLOS is to operate in
     * NO_NOTIFY_MODE
     */
    cp->baseCreateParams.notifyPrevLink = !(MULTICH_IPCBITS_NO_NOTIFY_BITSINHLOS);
}


Void MultiCh_ipcBitsInitCreateParams_BitsInRTOS(IpcBitsInLinkRTOS_CreateParams *cp,
                                                Bool notifyNextLink)
{
    /* Previous link of bitsInRTOS is bitsOutHLOSE. So, notifyPrevLink
     * should be set to false if bitsOutHLOS is to operate in
     * NO_NOTIFY_MODE
     */
    cp->baseCreateParams.noNotifyMode = MULTICH_IPCBITS_NO_NOTIFY_BITSOUTHLOS;
    cp->baseCreateParams.notifyNextLink = notifyNextLink;
    cp->baseCreateParams.notifyPrevLink = !(MULTICH_IPCBITS_NO_NOTIFY_BITSOUTHLOS);
}

Void MultiCh_ipcBitsInitCreateParams_BitsOutHLOS(IpcBitsOutLinkHLOS_CreateParams *cp,
                                                 System_LinkQueInfo *inQueInfo)
{
    /* Next link of bitsOutRTOS is bitsInHLOS. So, notifyPrevLink
     * should be set to false if bitsInHLOS is to operate in
     * NO_NOTIFY_MODE
     */
    cp->baseCreateParams.notifyNextLink = !(MULTICH_IPCBITS_NO_NOTIFY_BITSOUTHLOS);
    cp->baseCreateParams.notifyPrevLink = FALSE;
    if(inQueInfo!=NULL)
        cp->inQueInfo = *inQueInfo;
}

Void MultiCh_ipcBitsInitCreateParams_BitsOutRTOS(IpcBitsOutLinkRTOS_CreateParams *cp,
                                                Bool notifyPrevLink)
{
    /* Next link of bitsOutRTOS is bitsInHLOS. So, notifyPrevLink
     * should be set to false if bitsInHLOS is to operate in
     * NO_NOTIFY_MODE
     */
    cp->baseCreateParams.noNotifyMode = MULTICH_IPCBITS_NO_NOTIFY_BITSINHLOS;
    cp->baseCreateParams.notifyNextLink = !(MULTICH_IPCBITS_NO_NOTIFY_BITSOUTHLOS);
    cp->baseCreateParams.notifyPrevLink = notifyPrevLink;
}
