/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2011 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
    \file chains_ipcBits.h
    \brief Chains function related to IPC Bits links
*/


#ifndef CHAINS_IPCBITS_H_
#define CHAINS_IPCBITS_H_

#include <osa.h>
#include <link_api/ipcLink.h>

#define CHAINS_IPC_BITS_MAX_FILE_SIZE_INFINITY                          (~(0u))

Int32 Chains_ipcBitsInit();
Int32 Chains_ipcBitsExit();
Void Chains_ipcBitsInitCreateParams_BitsOutRTOS(IpcBitsOutLinkRTOS_CreateParams *cp,
                                                Bool notifyPrevLink);
Void Chains_ipcBitsInitCreateParams_BitsOutHLOS(IpcBitsOutLinkHLOS_CreateParams *cp,
                                                System_LinkQueInfo *inQueInfo);
Void Chains_ipcBitsInitCreateParams_BitsInRTOS(IpcBitsInLinkRTOS_CreateParams *cp,
                                                Bool notifyNextLink);
Void Chains_ipcBitsInitCreateParams_BitsInHLOS(IpcBitsInLinkHLOS_CreateParams *cp);
Void Chains_ipcBitsInitSetBitsOutNoNotifyMode(Bool noNotifyMode);
Void Chains_ipcBitsInitSetBitsInNoNotifyMode(Bool noNotifyMode);
Void Chains_ipcBitsStop(void);

#endif /* CHAINS_IPCBITS_H_ */
