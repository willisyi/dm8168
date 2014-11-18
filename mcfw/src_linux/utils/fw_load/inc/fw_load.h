
#ifndef _FW_LOAD_H_
#define _FW_LOAD_H_


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ti/syslink/Std.h>
#include <ti/syslink/IpcHost.h>
#include <ti/syslink/ProcMgr.h>
#include <ti/syslink/inc/_MultiProc.h>
#include <ti/ipc/MultiProc.h>

Int FirmwareLoad_startup (UInt16 procId, String filePath);
Int FirmwareLoad_shutdown (UInt16 procId);
Void FirmwareLoad_printStatus (Void);


#endif


