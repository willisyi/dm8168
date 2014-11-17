/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2011 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
    \file demo_crash_dump_analyzer_format.c
    \brief Format the Slave core exception dump into a format that can be
           analyzed by CCS crash dump analyzer
*/

#include "demo_crash_dump_analyzer_format.h"

static
Void  Demo_CCSCrashDumpFormatSaveRegsM3(VSYS_SLAVE_CORE_EXCEPTION_CONTEXT_M3_S *excCtxM3,FILE *fp)
{
    DEMO_CCS_CRASH_DUMP_REG_PRINT_FMT(fp,"R0",excCtxM3->r0);
    DEMO_CCS_CRASH_DUMP_REG_PRINT_FMT (fp,"R1",excCtxM3->r1);
    DEMO_CCS_CRASH_DUMP_REG_PRINT_FMT (fp,"R2",excCtxM3->r2);
    DEMO_CCS_CRASH_DUMP_REG_PRINT_FMT (fp,"R3",excCtxM3->r3);
    DEMO_CCS_CRASH_DUMP_REG_PRINT_FMT (fp,"R4",excCtxM3->r4);
    DEMO_CCS_CRASH_DUMP_REG_PRINT_FMT (fp,"R5",excCtxM3->r5);
    DEMO_CCS_CRASH_DUMP_REG_PRINT_FMT (fp,"R6",excCtxM3->r6);
    DEMO_CCS_CRASH_DUMP_REG_PRINT_FMT (fp,"R7",excCtxM3->r7);
    DEMO_CCS_CRASH_DUMP_REG_PRINT_FMT (fp,"R8",excCtxM3->r8);
    DEMO_CCS_CRASH_DUMP_REG_PRINT_FMT (fp,"R9",excCtxM3->r9);
    DEMO_CCS_CRASH_DUMP_REG_PRINT_FMT (fp,"R10",excCtxM3->r10);
    DEMO_CCS_CRASH_DUMP_REG_PRINT_FMT (fp,"R11",excCtxM3->r11);
    DEMO_CCS_CRASH_DUMP_REG_PRINT_FMT (fp,"R12",excCtxM3->r12);
    DEMO_CCS_CRASH_DUMP_REG_PRINT_FMT (fp,"SP",excCtxM3->sp);
    DEMO_CCS_CRASH_DUMP_REG_PRINT_FMT (fp,"LR",excCtxM3->lr);
    DEMO_CCS_CRASH_DUMP_REG_PRINT_FMT (fp,"PC", excCtxM3->pc);
    DEMO_CCS_CRASH_DUMP_REG_PRINT_FMT (fp,"PSR", excCtxM3->psr);
    DEMO_CCS_CRASH_DUMP_REG_PRINT_FMT (fp,"ICSR", excCtxM3->ICSR);
    DEMO_CCS_CRASH_DUMP_REG_PRINT_FMT (fp,"MMFSR", excCtxM3->MMFSR);
    DEMO_CCS_CRASH_DUMP_REG_PRINT_FMT (fp,"BFSR", excCtxM3->BFSR);
    DEMO_CCS_CRASH_DUMP_REG_PRINT_FMT (fp,"UFSR", excCtxM3->UFSR);
    DEMO_CCS_CRASH_DUMP_REG_PRINT_FMT (fp,"HFSR", excCtxM3->HFSR);
    DEMO_CCS_CRASH_DUMP_REG_PRINT_FMT (fp,"DFSR", excCtxM3->DFSR);
    DEMO_CCS_CRASH_DUMP_REG_PRINT_FMT (fp,"MMAR", excCtxM3->MMAR);
    DEMO_CCS_CRASH_DUMP_REG_PRINT_FMT (fp,"BFAR", excCtxM3->BFAR);
    DEMO_CCS_CRASH_DUMP_REG_PRINT_FMT (fp,"AFSR", excCtxM3->AFSR);
}

static
Void  Demo_CCSCrashDumpFormatSaveStackM3(VSYS_SLAVE_CORE_EXCEPTION_CONTEXT_M3_S *excCtxM3,
                                               FILE *fp,
                                               UInt8 *excStack,
                                               UInt32 maxLen)
{
    Int i;
    #ifdef min
    #undef min
    #endif
    #define min(a,b) ((a) < (b) ? (a) : (b))

    DEMO_CCS_CRASH_DUMP_MEM_HDR_PRINT_FMT(fp,
                                                  excCtxM3->threadStack,
                                                  excCtxM3->threadStackSize);
    for (i = 0; i < min(excCtxM3->threadStackSize,maxLen); i++)
    {
        DEMO_CCS_CRASH_DUMP_MEM_PRINT_FMT(fp,excStack[i]);
    }
}

Int32 Demo_CCSCrashDumpFormatSave(VSYS_SLAVE_CORE_EXCEPTION_INFO_S *excInfo,FILE *fp)
{

    if (fp == NULL)
    {
        return ERROR_FAIL;
    }
    if (strstr ( excInfo->excCoreName, "M3"))
    {
        rewind(fp);
        DEMO_CCS_CRASH_DUMP_M3_HEADER_PRINT_FMT(fp);
        Demo_CCSCrashDumpFormatSaveRegsM3(&excInfo->excContextM3,fp);
        Demo_CCSCrashDumpFormatSaveStackM3(&excInfo->excContextM3,
                                                 fp,
                                                 excInfo->excStack,
                                                 sizeof(excInfo->excStack));
        fflush(fp);
    }
    return ERROR_NONE;
}

