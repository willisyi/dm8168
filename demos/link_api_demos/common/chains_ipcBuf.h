/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2011 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
    \file chains_ipcBufAlloc.h
    \brief
*/
#ifndef _CHAINS_IPCBUFALLOC_H_
#define _CHAINS_IPCBUFALLOC_H_

Void Chains_createBuf(Ptr srBufPtr, Ptr bufPtr, Ptr phyAddr, 
							UInt32 bufSize, UInt32 srIndex);
Void Chains_deleteBuf(Ptr bufPtr, UInt32 bufSize, UInt32 srIndex);

Void Chains_fillBuf(UInt8 *bufPtr, Int8 *fileName, UInt32 fSize);

#endif //_CHAINS_IPCBUFALLOC_H_
