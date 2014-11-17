/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2011 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
    \file chains_ipcFrames.h
    \brief Chains function related to IPC Frames links
*/


#ifndef CHAINS_IPCFRAMES_H_
#define CHAINS_IPCFRAMES_H_

#include <osa.h>
#include <link_api/ipcLink.h>


Int32 Chains_ipcFramesInit();
Int32 Chains_ipcFramesExit();
Void Chains_ipcFramesStop(void);
Void Chains_ipcFramesInSetCbInfo (IpcFramesInLinkHLOS_CreateParams *createParams);

#endif /* CHAINS_IPCBITS_H_ */
