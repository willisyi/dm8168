/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include <mcfw/src_bios6/utils/utils.h>
#include <mcfw/interfaces/link_api/system.h>
#include <mcfw/src_bios6/links_common/system/system_priv_common.h>

#include <ti/psp/proxyServer/vps_proxyServer.h>
#include <ti/ipc/MultiProc.h>
#include <ti/ipc/Notify.h>
#include <ti/syslink/SysLink.h>

#define VPS_PROXY_SERVER_SUPPORT_A8
#define VPS_PS_MAX_NO_OF_CORES              (0x02u)
#define VPS_PS_STARTING_EVENT_NO            (0x0Au)
#define VPS_PS_TASK_PRIORITY_TO_START       (0x08u)
#define VPS_PS_RESERVED_EVENT_NO            (0x09u)


static  Char coreA8[]       ={"HOST"};
static UInt8 hostNotifyMem[100*1024];
#pragma DATA_SECTION(hostNotifyMem,".vpss:extMemNonCache:notify")

Int32 System_start(Task_FuncPtr chainsMainFunc);

/*
 * Initialize the proxy server
 */

static Int32 initProxyServer(void)
{
    Int32   rtnVal;
    UInt32  noOfCores, noOfEvents, eventNos, taskIndex, taskPri;
    VPS_PSrvInitParams  psInitParams;
    UInt16 remoteProcId;

    psInitParams.sysLnkNoOfCores        =   0x01;

    for(noOfCores = 0x0; noOfCores < VPS_PS_MAX_NO_OF_CORES; noOfCores++)
    {
        eventNos    =   VPS_PS_STARTING_EVENT_NO;
        psInitParams.sysLnkNoOfNtyEvt[noOfCores]    =   16;
        for(noOfEvents = 0x0;
                noOfEvents < psInitParams.sysLnkNoOfNtyEvt[noOfCores];
                        noOfEvents++)
        {
            psInitParams.sysLnkNtyEvtNo[noOfCores][noOfEvents]  =   eventNos;
            eventNos++;
        }
        psInitParams.resSysLnkNtyNo[noOfCores]      =   VPS_PS_RESERVED_EVENT_NO;
    }
    taskPri =   VPS_PS_TASK_PRIORITY_TO_START;
    for (taskIndex = 0x0; taskIndex < 0x05u; taskIndex++)
    {
        psInitParams.taskPriority[taskIndex]        =   taskPri;
        taskPri--;
    }

    psInitParams.completionCbTaskPri = VPS_PS_TASK_PRIORITY_TO_START + 1;

    remoteProcId = MultiProc_getId(coreA8);
    do {
        rtnVal = Notify_attach(remoteProcId, (Ptr)hostNotifyMem);
        Vps_printf ("notify_attach  rtnVal  %d\n", rtnVal);
    } while (rtnVal < 0);

    psInitParams.sysLnkCoreNames[0x0]       =   coreA8;
    rtnVal  =   VPS_PSrvInit(&psInitParams);

    return (rtnVal);
}

Void M3VPSS_main(UArg arg0, UArg arg1)
{
    char ch;

    while (1)
    {
        Task_sleep(100);
        Utils_getChar(&ch, BIOS_NO_WAIT);
        if (ch == 'x')
            break;
    }
}

Int32 main(void)
{
    Int32 retVal;

    Utils_setCpuFrequency(SYSTEM_M3VPSS_FREQ);

    SysLink_setup ();

    retVal = initProxyServer();
    Vps_printf ("initProxyServer  rtnVal  %d\n", retVal);

    System_start(M3VPSS_main);
    BIOS_start();

    return (0);
}

Int32 StartupEmulatorWaitFxn (void)
{
    volatile int i = 0;
    while (i);
    return 0;
}
