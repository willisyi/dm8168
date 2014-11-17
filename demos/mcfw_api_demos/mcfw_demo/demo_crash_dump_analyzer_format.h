/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2011 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
    \file demo_crash_dump_analyzer_format.h
    \brief Interface to slave core exception info format functions.
*/


#ifndef DEMO_CRASH_DUMP_ANALYZER_FORMAT_H_
#define DEMO_CRASH_DUMP_ANALYZER_FORMAT_H_

#include <stdio.h>
#include <stdlib.h>
#include <osa.h>
#include <mcfw/interfaces/ti_media_error_def.h>
#include <mcfw/interfaces/common_def/ti_vsys_common_def.h>

#define DEMO_CCS_CRASH_DUMP_M3_HEADER_STR                        "521177 14"
#define DEMO_CCS_CRASH_DUMP_M3_HEADER_PRINT_FMT(fp)              do { rewind(fp); fprintf(fp,"%s\n",DEMO_CCS_CRASH_DUMP_M3_HEADER_STR); } while(0)
#define DEMO_CCS_CRASH_DUMP_REG_PRINT_FMT(fp,regName,regVal)     fprintf(fp,"R %s 0x0000000B 0x%08x\n",regName,(UInt32)regVal)
#define DEMO_CCS_CRASH_DUMP_MEM_HDR_PRINT_FMT(fp,baseAddr,size)  fprintf(fp,"M 0 0x%08x 0x%08x\n",(UInt32)baseAddr,size)
#define DEMO_CCS_CRASH_DUMP_MEM_PRINT_FMT(fp,val)                fprintf(fp,"0x%02x\n",val)

Int32 Demo_CCSCrashDumpFormatSave(VSYS_SLAVE_CORE_EXCEPTION_INFO_S *excInfo,FILE *fp);

#endif /* DEMO_CRASH_DUMP_ANALYZER_FORMAT_H_ */
