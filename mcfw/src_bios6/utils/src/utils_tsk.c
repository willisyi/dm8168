/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include <mcfw/src_bios6/utils/utils_tsk.h>
#include <ti/sysbios/knl/Task.h>

#define UTILS_TSK_CMD_EXIT   (0xFFFFFFFF)

Void Utils_tskMain(UArg arg0, UArg arg1)
{
    Utils_TskHndl *pHndl = (Utils_TskHndl *) arg0;
    Utils_MsgHndl *pMsg;
    Int32 status;
    UInt32 cmd;

    UTILS_assert(pHndl != NULL);

    while (1)
    {
        status = Utils_mbxRecvMsg(&pHndl->mbx, &pMsg, BIOS_WAIT_FOREVER);
        if (status != FVID2_SOK)
            break;

        cmd = Utils_msgGetCmd(pMsg);
        if (cmd == UTILS_TSK_CMD_EXIT)
        {
            Utils_tskAckOrFreeMsg(pMsg, FVID2_SOK);
            break;
        }

        if (pHndl->funcMain)
            pHndl->funcMain(pHndl, pMsg);
    }
}

Int32 Utils_tskCreate(Utils_TskHndl * pHndl,
                      Utils_TskFuncMain funcMain,
                      UInt32 tskPri,
                      UInt8 * stackAddr,
                      UInt32 stackSize, Ptr appData, char *tskName)
{
    Int32 status;

    UTILS_assert(pHndl != NULL);
    UTILS_assert(funcMain != NULL);

    pHndl->funcMain = funcMain;
    pHndl->stackSize = stackSize;
    pHndl->stackAddr = stackAddr;
    pHndl->tskPri = tskPri;
    pHndl->appData = appData;

    status = Utils_mbxCreate(&pHndl->mbx);

    UTILS_assert(status == FVID2_SOK);

    Task_Params_init(&pHndl->tskParams);

    pHndl->tskParams.arg0 = (UArg) pHndl;
    pHndl->tskParams.arg1 = (UArg) pHndl;
    pHndl->tskParams.priority = pHndl->tskPri;
    pHndl->tskParams.stack = pHndl->stackAddr;
    pHndl->tskParams.stackSize = pHndl->stackSize;

    pHndl->tsk = Task_create(Utils_tskMain, &pHndl->tskParams, NULL);

    UTILS_assert(pHndl->tsk != NULL);

    Utils_prfLoadRegister(pHndl->tsk, tskName);
    return status;
}

Int32 Utils_tskDelete(Utils_TskHndl * pHndl)
{
    UInt32 sleepTime = 8;                                  /* in OS ticks */

    Utils_tskSendCmd(pHndl, UTILS_TSK_CMD_EXIT);

    /* wait for command to be received and task to be exited */

    Task_sleep(1);

    while (Task_Mode_TERMINATED != Task_getMode(pHndl->tsk))
    {

        Task_sleep(sleepTime);

        sleepTime >>= 1;

        if (sleepTime == 0)
        {
            char name[64];

            strcpy(name, "INVALID_TSK");
            Utils_prfGetTaskName(pHndl->tsk, name);
            Vps_printf("Task Delete Error!!!!!!, task %s not deleted\n", name);
            UTILS_assert(0);
        }
    }

    Utils_prfLoadUnRegister(pHndl->tsk);

    Task_delete(&pHndl->tsk);
    Utils_mbxDelete(&pHndl->mbx);

    return FVID2_SOK;
}

Int32 Utils_tskFlushMsg(Utils_TskHndl * pHndl, UInt32 *flushCmdId, UInt32 numCmds)
{
	Utils_MsgHndl *pMsg;
	UInt32 i, cmd;
	Int32 status;
	Bool done;
	
	do {
		status = Utils_tskPeekMsg(pHndl, &pMsg);
		if (status != FVID2_SOK)
			break;
			
		cmd = Utils_msgGetCmd(pMsg);

		done = TRUE;

		// search the commands the need to be flushed
		for(i=0; i<numCmds; i++)
		{
			if(cmd==flushCmdId[i])
			{
				// same command in queue to pull it out of the queue and free it
				status = Utils_tskRecvMsg(pHndl, &pMsg, BIOS_NO_WAIT);
				UTILS_assert(status==FVID2_SOK);
				Utils_tskAckOrFreeMsg(pMsg, status);

				done = FALSE;
				break;
			}
		}
		
	} while(!done);

	return FVID2_SOK;
}

Int32 Utils_tskSetPri(Utils_TskHndl * pHndl, UInt32 newPri)
{

    if((Task_Mode_TERMINATED != Task_getMode(pHndl->tsk)) && (newPri != 0) && (newPri < (UInt32)Task_numPriorities))
    {
       Task_setPri(pHndl->tsk, newPri);
       pHndl->tskPri = newPri;
    }
    else
       return FVID2_EFAIL;
    
    return FVID2_SOK;
}

