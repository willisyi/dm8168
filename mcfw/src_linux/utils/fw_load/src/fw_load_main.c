
#include <fw_load.h>
#include <ti/syslink/SysLink.h>



Void FirmwareLoad_printUsageInfo (Void);

int main (int argc, char ** argv)
{
    Int     status       = 0;
    Bool    startupProc  = FALSE;
    Bool    shutdownProc = FALSE;
    UInt16  procId       = MultiProc_INVALIDID;

    /* To enable caching of shared region, the below string should
     * be set.
     * TODO: This is currently hard coded to shared region 1.
     * This parameter should come as an argument to the firmware
     * load application
     */
    SysLink_params  = "SharedRegion.entry[1].cacheEnable=TRUE;" ;
    SysLink_setup ();

    if ((argc == 2) && (strcmp (argv [1], "--help") == 0)) {
        status = -1;
        FirmwareLoad_printUsageInfo ();
    }
    else if ((argc < 2) || (argc > 4)) {
        status = -1;
        FirmwareLoad_printUsageInfo ();
    }
    else {
        if (strcmp (argv [1], "startup") == 0)
        {
            if (argc != 4) {
                status = -1;
                printf("[File path] and <Core name> need to be provided"
                            " for 'startup' !\n");
                FirmwareLoad_printUsageInfo ();
            }
            /* Only startup slave core. */
            startupProc = TRUE;
        }
        else
        if (strcmp (argv [1], "shutdown")  == 0)
        {
            if (argc < 3) {
                status = -1;
                printf("<Core name> needs to be provided for"
                            " 'shutdown' !\n");
                FirmwareLoad_printUsageInfo ();
            }

            /* Only shutdown slave core. */
            shutdownProc = TRUE;
        }
        else {
            status = -1;
            FirmwareLoad_printUsageInfo ();
        }

        /* Validate passed core name and get its ID. */
        if (status >= 0) {
            procId = MultiProc_getId (argv [2]);
            if (procId == MultiProc_INVALIDID) {
                status = -1;
                printf("Invalid <Core name> specified!\n");
                FirmwareLoad_printUsageInfo ();
            }
        }
    }

    if (status >= 0) {
        if (startupProc == TRUE)
            status = FirmwareLoad_startup (procId, argv [3]);
    }

    if (status >= 0) {
        if ((procId != MultiProc_INVALIDID) && (shutdownProc == TRUE))
            status = FirmwareLoad_shutdown (procId);
    }

    SysLink_destroy ();

    return 0;
}


Void FirmwareLoad_printUsageInfo (Void)
{
    UInt16 numProcs;
    UInt16 i;

    printf ("Usage : ./fw_load.out"
       " <startup|shutdown>"
       " <Core name> [File path]\n\n"
       "Supported core names:\n");
    numProcs = MultiProc_getNumProcessors ();
    for (i = 0 ; (i < numProcs) && (i != MultiProc_self ()) ; i++) {
        printf ("- %s\n", MultiProc_getName (i));
    }
    printf ("Note: [File path] argument is only required when first"
                 " argument is 'startup' \n");
}


