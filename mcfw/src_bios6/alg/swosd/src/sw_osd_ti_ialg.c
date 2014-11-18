/*
 *  Copyright 2008
 *  Texas Instruments Incorporated
 *
 *  All rights reserved.  Property of Texas Instruments Incorporated
 *  Restricted rights to use, duplicate or disclose this code are
 *  granted through contract.
 *
 */

#include <sw_osd_ti_priv.h>


#define SWOSD_TI_VERSIONSTRING "0.91.00.00"

/*------------------------------- MACROS ------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Set MTAB_NRECS to the number of Memtabs required for the algorithm.       */
/*---------------------------------------------------------------------------*/
#define MTAB_NRECS              2

#define IALGFXNS  \
    &SWOSD_TI_IALG,  /* module ID */                         \
    NULL,            /* activate */                          \
    SWOSD_TI_alloc,  /* alloc */                             \
    NULL,            /* control (NULL => no control ops) */  \
    NULL,            /* deactivate */                        \
    SWOSD_TI_free,   /* free */                              \
    SWOSD_TI_initObj,/* init */                              \
    NULL,            /* moved */                             \
    SWOSD_TI_numAlloc             /* numAlloc (NULL => IALG_MAXMEMRECS) */

IALG_Fxns SWOSD_TI_IALG = {      /* module_vendor_interface */
    IALGFXNS
};

SWOSD_Params SWOSD_TI_PARAMS = {
    sizeof(SWOSD_Params),       /* size */
    SWOSD_MAX_WIDTH,                    /* maxWidth */
    1080,                               /* maxHeight */
};

Int SWOSD_TI_numAlloc(void)
{
    return (MTAB_NRECS);
}

Int SWOSD_TI_alloc(const IALG_Params *algParams, IALG_Fxns **pf, IALG_MemRec memTab[])
{
    const SWOSD_Params *params = (SWOSD_Params *)algParams;

    memTab[0].size = sizeof(SWOSD_TI_Obj);
    memTab[0].alignment = 0;
    memTab[0].space = IALG_DARAM0;
    memTab[0].attrs = IALG_PERSIST;

    memTab[1].size = (params->maxWidth*(2+2+2+2))*2;
    memTab[1].alignment = 128;
    memTab[1].space = IALG_DARAM0;
    memTab[1].attrs = IALG_PERSIST;

    return (2);
}

Int SWOSD_TI_free(IALG_Handle handle, IALG_MemRec memTab[])
{
    return (SWOSD_TI_alloc(NULL, NULL, memTab));
}

Int SWOSD_TI_initObj(IALG_Handle handle, const IALG_MemRec memTab[],
    IALG_Handle p, const IALG_Params *algParams)
{
    const SWOSD_Params *params = (SWOSD_Params *)algParams;
    SWOSD_TI_Obj *obj = (SWOSD_TI_Obj *)handle;

    if (params == NULL) {
        params = &SWOSD_TI_PARAMS;
    }

    obj->swOsdCtrl.openPrm.maxWidth  = params->maxWidth;
    obj->swOsdCtrl.openPrm.maxHeight = params->maxHeight;

    obj->memLineBuf = memTab[1].base;

    return (SWOSD_SOK);
}


