/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/


#ifndef __TI_VGRPX_H__
#define __TI_VGRPX_H__


#ifdef __cplusplus
extern "C" {
#endif

#include "ti_vsys.h"
#include "ti_vgrpx_common_def.h"


Int32 Vgrpx_create(UInt32 grpxId, VGRPX_CREATE_PARAM_S *pPrm);
Int32 Vgrpx_getInfo(UInt32 grpxId, VGRPX_BUFFER_INFO_S *pInfo);
Int32 Vgrpx_setDynamicParam(UInt32 grpxId, VGRPX_DYNAMIC_PARAM_S *pPrm);
Int32 Vgrpx_enable(UInt32 grpxId, Bool enable);
Int32 Vgrpx_delete(UInt32 grpxId);

UInt32 Vgrpx_mmap(UInt32 addr, UInt32 size, Bool isCached);
Void   Vgrpx_unmap(UInt32 addr, UInt32 size);
Void   Vgrpx_cacheWb(UInt32 addr, UInt32 size);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __TI_VGRPX_H__ */

