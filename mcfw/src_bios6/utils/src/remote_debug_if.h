/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2010 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#ifndef _REMOTE_DEBUG_IF_
#define _REMOTE_DEBUG_IF_

#define REMOTE_DEBUG_PARAM_BUF_SIZE   (1024)
#define REMOTE_DEBUG_LOG_BUF_SIZE     (18*1024)

#define REMOTE_DEBUG_HEADER_TAG       (0)

#define REMOTE_DEBUG_CORE_ID_C6XDSP    (0)
#define REMOTE_DEBUG_CORE_ID_M3VIDEO   (1)
#define REMOTE_DEBUG_CORE_ID_M3VPSS    (2)
#define REMOTE_DEBUG_CORE_ID_MAX       (3)

#define REMOTE_DEBUG_FLAG_TYPE_CHAR    (0x01000000)
#define REMOTE_DEBUG_FLAG_TYPE_STRING (0x10000000)

typedef struct {

    volatile unsigned int headerTag;
    volatile unsigned int serverIdx;
    volatile unsigned int clientIdx;
    volatile unsigned int serverFlags[2]; /** RESERVED, DO NOT USE */
    volatile unsigned int clientFlags[2]; /** RESERVED, DO NOT USE */
    volatile unsigned int reserved;       /** RESERVED, DO NOT USE */

    volatile unsigned char serverLogBuf[REMOTE_DEBUG_LOG_BUF_SIZE];
    volatile unsigned char serverParamBuf[REMOTE_DEBUG_PARAM_BUF_SIZE]; /** RESERVED, DO NOT USE */
    volatile unsigned char clientParamBuf[REMOTE_DEBUG_PARAM_BUF_SIZE]; /** RESERVED, DO NOT USE */

} RemoteDebug_CoreObj;

#endif                                                     /* _REMOTE_DEBUG_IF_ 
                                                            */
