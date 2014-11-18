

#include <fw_load.h>

#define NUM_ARGS 0

Int FirmwareLoad_ipcStop(UInt16 procId)
{
    Int status;

    status = Ipc_control (procId,
                          Ipc_CONTROLCMD_STOPCALLBACK,
                          NULL);
    printf ("After Ipc_stopCallback status: [0x%08x]\n",
                 status);

    return status;
}

Int FirmwareLoad_ipcLoad(UInt16 procId)
{
    Int status;
    
    status = Ipc_control (procId,
                          Ipc_CONTROLCMD_LOADCALLBACK,
                          NULL);
    if (status < 0) {
        printf ("Error in Ipc_control "
                     "Ipc_CONTROLCMD_LOADCALLBACK [0x%08x]\n",
                      status);
    }
    else {
        printf ("After Ipc_loadcallback status [0x%08x]\n",
                     status);
    }

    return status;
}

Int FirmwareLoad_ipcStart(UInt16 procId)
{
    Int status=0;

    status = FirmwareLoad_ipcLoad(procId);
    
    if (status >= 0) {
        status = Ipc_control (procId,
                              Ipc_CONTROLCMD_STARTCALLBACK,
                              NULL);
        if (status < 0) {
            printf ("Error in Ipc_control "
                         "Ipc_CONTROLCMD_STARTCALLBACK [0x%08x]\n",
                         status);
        }
        else {
            printf ("After Ipc_startcallback status [0x%08x]\n",
                         status);
        }
    }
            
    return status;            
}

Int FirmwareLoad_startup (UInt16 procId, String filePath)
{
    Int                          status = 0;
    ProcMgr_Handle               handle = NULL;
    ProcMgr_AttachParams         attachParams;
    ProcMgr_StartParams          startParams;
    //String                       args [NUM_ARGS];
    UInt32                       fileId;

    status = ProcMgr_open (&handle, procId);

    if (status >= 0) {

        ProcMgr_getAttachParams (NULL, &attachParams);
        
        /* Default params will be used if NULL is passed. */
        status = ProcMgr_attach (handle, &attachParams);
        if (status < 0) 
            printf ("ProcMgr_attach failed [0x%x]\n", status);
        else 
            printf ("Attached to slave procId %d.\n", procId);

        if(status >= 0)
        {
            /* Send filePath as the args to the slave to demonstrate how args
             * are used.
             */
            //args [0] = filePath;
            status = ProcMgr_load (handle,
                                   filePath,
                                   0,
                                   NULL,
                                   NULL,
                                   &fileId);
            if (status < 0) 
                printf ("Error in ProcMgr_load [0x%x]\n", status);
            else 
                printf ("Loaded file %s on slave procId %d.\n", filePath, procId);
        }

        if (status >= 0)
        {
            ProcMgr_getStartParams (handle, &startParams);
            status = ProcMgr_start (handle, &startParams);
            if (status < 0) 
                printf ("ProcMgr_start failed [0x%x]\n", status);
            else 
                printf ("Started slave procId %d.\n", procId);
        }

        status = ProcMgr_close (&handle);
    }

    if(status>=0)    
        status = FirmwareLoad_ipcStart(procId);    

    return status;
}


Int FirmwareLoad_shutdown (UInt16 procId)
{
    Int             status = 0;
    ProcMgr_Handle  handle = NULL;
    UInt32          fileId;

    if(status>=0)
        status = FirmwareLoad_ipcStop(procId);    

    if(status>=0)    
        status = ProcMgr_open (&handle, procId);

    if (status >= 0) {
        status = ProcMgr_stop (handle);
        printf ("Stopped slave procId %d.\n", procId);

        fileId = ProcMgr_getLoadedFileId (handle);
        status = ProcMgr_unload (handle, fileId) ;

        printf ("Unloaded slave procId %d.\n", procId);
        status = ProcMgr_detach (handle);

        printf ("Detached from slave procId %d.\n", procId);

        status = ProcMgr_close (&handle);
    }

    return status;
}

Void FirmwareLoad_printStatus (Void)
{
    Int             status = 0;
    ProcMgr_Handle  handle = NULL;
    UInt16          numProcs;
    UInt16          i;
    ProcMgr_State   state;
    Char            strState [32u];

    printf ("Current status of slave cores:\n");
    numProcs = MultiProc_getNumProcessors ();
    for (i = 0 ; (i < numProcs) && (i != MultiProc_self ()) ; i++) {
        {
            status = ProcMgr_open (&handle, i);
            if (status >= 0) {
                state = ProcMgr_getState (handle);
                switch (state)
                {
                    case ProcMgr_State_Unknown:
                        strncpy (strState, "Unknown", 32u);
                        break;
                    case ProcMgr_State_Powered:
                        strncpy (strState, "Powered", 32u);
                        break;
                    case ProcMgr_State_Reset:
                        strncpy (strState, "Reset", 32u);
                        break;
                    case ProcMgr_State_Loaded:
                        strncpy (strState, "Loaded", 32u);
                        break;
                    case ProcMgr_State_Running:
                        strncpy (strState, "Running", 32u);
                        break;
                    case ProcMgr_State_Unavailable:
                        strncpy (strState, "Unavailable", 32u);
                        break;
                    case ProcMgr_State_EndValue:
                        /* Not a valid value. */
                        break;
                }
                printf ("Slave core %d: %8s [%s]\n",
                             i,
                             MultiProc_getName (i),
                             strState);
                status = ProcMgr_close (&handle);
            }
        }
    }
}

