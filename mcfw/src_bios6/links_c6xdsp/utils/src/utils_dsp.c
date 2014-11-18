/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include <xdc/std.h>
#include <ti/xdais/xdas.h>
#include <ti/xdais/ialg.h>
#include <ti/sdo/fc/rman/rman.h>
#include <ti/xdais/dm/ividanalytics.h>

#include <xdc/runtime/Assert.h>
#include <xdc/runtime/Diags.h>
#include <xdc/runtime/Log.h>
#include <ti/xdais/ires.h>
#include <ti/sdo/fc/dskt2/dskt2.h>

/* IRES resources */
#include <ti/sdo/fc/ires/edma3chan/iresman_edma3Chan.h>
#include <ti/sdo/fc/ires/edma3chan/ires_edma3Chan.h>
#include <ti/sdo/fc/global/FCSettings.h>

#include <mcfw/src_bios6/links_c6xdsp/utils/utils_dsp.h>

Int initDone = FALSE;

Int Utils_dspInit()
{
    IRES_Status iresStatus;
    IRESMAN_Edma3ChanParams initArgs;

    if (FALSE == initDone)
    {
        iresStatus = RMAN_init();
        UTILS_assert(iresStatus == IRES_OK);

        /* Set default mask for FC modules 
        FCSettings_init();
        Diags_setMask(FCSETTINGS_MODNAME"+EX467");*/

        initArgs.baseConfig.size = sizeof(IRESMAN_Edma3ChanParams);
        initArgs.baseConfig.allocFxn = &DSKT2_allocPersistent;
        initArgs.baseConfig.freeFxn = &DSKT2_freePersistent;
        iresStatus = RMAN_register(&IRESMAN_EDMA3CHAN, (IRESMAN_Params *)&initArgs);
        if (IRES_EEXISTS == iresStatus)
        {
            Vps_printf("!!WARNING.Resource already registered:%d",iresStatus);
            /* Unregister Tiler Resource with the RMAN so that we can register with
             * the new plugins (gethandle & freehandle functions) */
            UTILS_assert(RMAN_unregister(&IRESMAN_EDMA3CHAN) == IRES_OK);
            initArgs.baseConfig.size = sizeof(IRESMAN_Edma3ChanParams);
            initArgs.baseConfig.allocFxn = &DSKT2_allocPersistent;
            initArgs.baseConfig.freeFxn = &DSKT2_freePersistent;
            iresStatus = RMAN_register(&IRESMAN_EDMA3CHAN, (IRESMAN_Params *)&initArgs);
        }
        UTILS_assert(iresStatus == IRES_OK);

        initDone = TRUE;
   }
   return 0;
}

Int Utils_dspDeInit()
{
    IRES_Status iresStatus;

    if (TRUE == initDone)
    {
        iresStatus = RMAN_unregister(&IRESMAN_EDMA3CHAN);
        UTILS_assert(iresStatus == IRES_OK);

        iresStatus = RMAN_exit();
        UTILS_assert(iresStatus == IRES_OK);
        initDone = FALSE;
    }

    return 0;
}


