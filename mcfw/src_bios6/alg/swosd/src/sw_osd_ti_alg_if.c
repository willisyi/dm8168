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


Int32 g_scratchIndex = 1;

Int32 SWOSD_open(SWOSD_Obj * pObj, SWOSD_OpenPrm *openPrm)
{
    Int32 status = SWOSD_SOK;
    Int32 scratchId = g_scratchIndex;

    SWOSD_Params prm;
	
    IALG_Fxns * algFxns = (IALG_Fxns *)&SWOSD_TI_IALG;
    IRES_Fxns * resFxns = &SWOSD_TI_IRES;

    if(openPrm==NULL || pObj==NULL)
        return SWOSD_EFAIL;

    if(   openPrm->maxWidth==0
         || openPrm->maxHeight==0
        || openPrm->maxWidth  > SWOSD_MAX_WIDTH
        || openPrm->maxHeight > 1080
        ) {
        return NULL;
    }

    memset(pObj, 0, sizeof(SWOSD_Obj));

    memcpy(&pObj->openPrm, openPrm, sizeof(SWOSD_OpenPrm));

    memset(&prm, 0, sizeof(prm));
    prm.size = sizeof(prm);
    prm.maxHeight = pObj->openPrm.maxHeight;
    prm.maxWidth  = pObj->openPrm.maxWidth;

    pObj->algHndl = DSKT2_createAlg((Int)scratchId,
            (IALG_Fxns *)algFxns, NULL,(IALG_Params *)&prm);

    if(pObj->algHndl==NULL)
    {
        status = SWOSD_EFAIL;
        return status;
    }

    /* Assign resources to the algorithm */
    status = RMAN_assignResources((IALG_Handle)pObj->algHndl, resFxns, scratchId);
    if (status != IRES_OK) {
        status = SWOSD_EFAIL;
        return status;
    }

	
    return status;
}

Int32 SWOSD_close(SWOSD_Obj *pObj)
{
    Int32 scratchId = g_scratchIndex;
    IRES_Status status;
    IRES_Fxns * resFxns = &SWOSD_TI_IRES;
	
    if(pObj->algHndl == NULL)
        return SWOSD_EFAIL;

	/*
	* Deactivate All Resources
	*/
	RMAN_deactivateAllResources((IALG_Handle)pObj->algHndl, resFxns, scratchId);

	/* Deactivate algorithm */
	DSKT2_deactivateAlg(scratchId, (IALG_Handle)pObj->algHndl);

    /*
    * Free resources assigned to this algorihtm
    */
    status = RMAN_freeResources((IALG_Handle)pObj->algHndl, resFxns, scratchId);

    if (IRES_OK != status) {
	    return SWOSD_EFAIL;
    }

    DSKT2_freeAlg(scratchId, (IALG_Handle)pObj->algHndl);

    return SWOSD_SOK;
}

Int32 SWOSD_blendWindow(SWOSD_Obj *  hndl)
{
    SWOSD_TI_Obj *pObj = (SWOSD_TI_Obj *)hndl->algHndl;
    IRES_Fxns * resFxns = &SWOSD_TI_IRES;

    memcpy(&pObj->swOsdCtrl, hndl, sizeof(pObj->swOsdCtrl));

	/* Activate the Algorithm */
//	DSKT2_activateAlg(g_scratchIndex, (IALG_Handle)hndl->algHndl);

	/*
	* Activate All Resources
	*/
	RMAN_activateAllResources((IALG_Handle)hndl->algHndl, resFxns, g_scratchIndex);


    SWOSD_TI_algRun(pObj, hndl);

	/*
	* Deactivate All Resources
	*/
	RMAN_deactivateAllResources((IALG_Handle)hndl->algHndl, resFxns, g_scratchIndex);

	/* Deactivate algorithm */
//	DSKT2_deactivateAlg(g_scratchIndex, (IALG_Handle)hndl->algHndl);


    return 0;
}


