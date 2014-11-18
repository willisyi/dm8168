/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
 *  \file VpdmaUtils_main.c
 *
 *  \brief Main file to dump the VPDMA list manager debug information and
 *  to print the VPDMA descriptor.
 *
 */

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "vpshal_vpdma.h"


UInt32 CSL_TI81XX_VPS_BASE ;
UInt32 CSL_VPS_VPDMA_0_REGS;
UInt32 CSL_VPS_COMP_0_REGS ;


/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */

/* Test application stack size. */
#define APP_TSK_STACK_SIZE              (10u * 1024u)

#ifdef PLATFORM_SIM

#ifdef VPS_CFG_DESC_IN_OCMC
/* OCMC (0x403XXXXX or 0x404XXXXX) is not supported on simulator.
 * Hence data should be loaded from 0x80000000 memory location. */
#define DESC_ADDR_MASK                  (0x000FFFFFu)
#define DESC_ADDR_OFFSET                (0x80000000u)
#else
#define DESC_ADDR_MASK                  (0xFFFFFFFFu)
#define DESC_ADDR_OFFSET                (0x00000000u)
#endif

#else

/* One to one mapping on EVM */
#define DESC_ADDR_MASK                  (0xFFFFFFFFu)
#define DESC_ADDR_OFFSET                (0x00000000u)

#endif

#define MAX_RELOADS                     (300u)


/* ========================================================================== */
/*                         Structure Declarations                             */
/* ========================================================================== */

/* None */

/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */

/* None */

/* ========================================================================== */
/*                            Global Variables                                */
/* ========================================================================== */

/* Test application stack. */

/* History of list addresses to check for loops in list reloads. */
static UInt32 AppListAddrHistory[MAX_RELOADS];


/* ========================================================================== */
/*                          Function Definitions                              */
/* ========================================================================== */


static void App_dumpListStatus(void)
{
    UInt32      listNum, listStatus;

    Vps_printf(" \n");
    Vps_printf("List Status Dump\n");
    Vps_printf("----------------\n");

    /* Dump the status of all the lists */
    for (listNum = 0u; listNum < VPSHAL_VPDMA_MAX_LIST; listNum++)
    {
        listStatus = VpsHal_vpdmaGetListStatus(listNum);
        Vps_printf("List Status %d: 0x%08x\n", listNum, listStatus);
    }

    return;
}

static void App_printCompStatus(void)
{
    UInt32              regVal, flag = FALSE;
    CSL_Vps_compRegs   *compRegOvly;

    Vps_printf(" \n");
    Vps_printf("Comp Status:\n");
    Vps_printf("------------\n");

    compRegOvly = (CSL_Vps_compRegs *) CSL_VPS_COMP_0_REGS;

    /* Check HDMI */
    regVal = compRegOvly->HDMI_SETTINGS;
    if (regVal & CSL_VPS_COMP_HDMI_SETTINGS_ERROR_MASK)
    {
        Vps_printf("HDMI COMP Error!! One or more of the enabled input "
            "sources is not the proper size. Reg Value (0x%08x)\n", regVal);
        flag = TRUE;
    }

#if defined(TI_816X_BUILD) || defined(TI_8107_BUILD)
    /* Check HDCOMP */
    regVal = compRegOvly->HDCOMP_SETTINGS;
    if (regVal & CSL_VPS_COMP_HDCOMP_SETTINGS_ERROR_MASK)
    {
        Vps_printf("HDCOMP COMP Error!! One or more of the enabled input "
            "sources is not the proper size. Reg Value (0x%08x)\n", regVal);
        flag = TRUE;
    }
#endif

    /* Check DVO2 */
    regVal = compRegOvly->DVO2_SETTINGS;
    if (regVal & CSL_VPS_COMP_DVO2_SETTINGS_ERROR_MASK)
    {
        Vps_printf("DVO2 COMP Error!! One or more of the enabled input "
            "sources is not the proper size. Reg Value (0x%08x)\n", regVal);
        flag = TRUE;
    }

    /* Check SD */
    regVal = compRegOvly->SD_SETTINGS;
    if (regVal & CSL_VPS_COMP_SD_SETTINGS_ERROR_MASK)
    {
        Vps_printf("DVO2 COMP Error!! One or more of the enabled input "
            "sources is not the proper size. Reg Value (0x%08x)\n", regVal);
        flag = TRUE;
    }

    if (!flag)
    {
        Vps_printf("No COMP errors!!!\n");
    }

    return;
}

static void App_printVpdmaLmReg(UInt32 regOffset, UInt32 numReg)
{
    UInt32      loop, lmRegVal;
    static char tmpBuf[64];
    static char buf[1024];

    /* Change print's to do Vps_printf() as a single function call
     * in order to make the output appear correctly when doing LM dump from
     * A8 side */
    buf[0] = 0;
    sprintf(tmpBuf, "0x%04X:", regOffset);
    strcat(buf, tmpBuf);
    for (loop = 0u; loop < numReg; loop++)
    {
        lmRegVal = VpsHal_vpdmaReadLmReg(0u, regOffset);
        sprintf(tmpBuf, " 0x%04X", lmRegVal);
        strcat(buf, tmpBuf);
        regOffset++;
    }
    strcat(buf, "\n");
    Vps_printf(buf);

    return;
}

static void App_dumpAllLmReg(void)
{
    UInt32      regOffset;

    Vps_printf(" \n");
    Vps_printf("LM Register Dump\n");
    Vps_printf("----------------\n");

    /* Print internal LM registers */
    for (regOffset = 0x8000u; regOffset <= 0x80FFu;)
    {
        App_printVpdmaLmReg(regOffset, 16u);
        regOffset += 16u;
    }

    /* Print the shared memory space */
    for (regOffset = 0x0000u; regOffset <= 0x2000u;)
    {
        App_printVpdmaLmReg(regOffset, 16u);
        regOffset += 16u;
    }

    return;
}

/**
 *  App_printAllStatus
 *  Prints all the status information.
 */
static Void App_printAllStatus(void)
{
    CSL_VpsVpdmaRegsOvly    vpdmaRegOvly;

    Vps_printf(" \n");
    Vps_printf("***********************************************************\n");
    Vps_printf("VPDMA Dump Start...\n");
    Vps_printf(" \n");

    vpdmaRegOvly = (CSL_VpsVpdmaRegsOvly) CSL_VPS_VPDMA_0_REGS;
    Vps_printf("VPDMA Firmware Version: 0x%08x\n", vpdmaRegOvly->PID);
    Vps_printf(
        "VPDMA List Busy Status: 0x%08x\n", vpdmaRegOvly->LIST_STAT_SYNC);

    App_printCompStatus();
    VpsHal_vpdmaPrintCStat();
    App_dumpListStatus();
    App_dumpAllLmReg();

    Vps_printf(" \n");
    Vps_printf("VPDMA Dump Successful!!\n");
    Vps_printf("***********************************************************\n");
    Vps_printf(" \n");

    return;
}

static UInt32 isDescAddrAlreadyPresent(UInt32 listAddr, UInt32 curIndex)
{
    UInt32      index;
    UInt32      foundFlag = FALSE;

    for (index = 0u; index < curIndex; index++)
    {
        if (listAddr == AppListAddrHistory[index])
        {
            foundFlag = TRUE;
            break;
        }
    }

    return (foundFlag);
}

static Void App_printList(UInt32 listAddr, UInt32 listSize, UInt32 printLevel)
{
    UInt32      curIndex;
    UInt32      rlListAddr, rlListSize;

    curIndex = 0u;
    memset(AppListAddrHistory, 0u, sizeof (AppListAddrHistory));

    while (1u)
    {
        /* Convert list address as per how descriptors are loaded in CCS */
        listAddr = (listAddr & DESC_ADDR_MASK) + DESC_ADDR_OFFSET;

        /* Check for wrap around as happens in the case of DLM */
        if ((TRUE == isDescAddrAlreadyPresent(listAddr, curIndex)) ||
            (curIndex >= MAX_RELOADS))
        {
            break;
        }
        AppListAddrHistory[curIndex] = listAddr;

        /* Print the list */
        VpsHal_vpdmaPrintList(
            listAddr,
            listSize,
            &rlListAddr,
            &rlListSize,
            printLevel);
        if (0u == rlListAddr)
        {
            break;
        }

        listAddr = rlListAddr;
        listSize = rlListSize;
        curIndex++;
    }

    return;
}

/**
 *  App_printListDescriptor
 *  Prints the descriptor list.
 */
static Void App_printListDescriptor(void)
{
    UInt32      printLevel;
    UInt32      listAddr, listSize;
    char        inputStr[20];

    Vps_printf("\n************************************************************\n");
    Vps_printf("VPDMA Descriptor Print Start...\n\n");

#ifdef VPS_CFG_DESC_IN_OCMC
#ifdef PLATFORM_SIM
    Vps_printf("Simulator build: Kindly load descriptors to 0x80000000 location\n");
#endif
#endif

    Vps_printf("Enter list address in hex: ");
    scanf("%s",  inputStr);
    listAddr = (UInt32)xstrtoi(inputStr);
    if (0u == listAddr)
    {
        Vps_printf("NULL address!!\n");
        return;
    }

    Vps_printf("Enter list size in VPDMA words: ");
    scanf("%s",  inputStr);
    listSize = (UInt32)atoi(inputStr);

    Vps_printf("Print levels:\n");
    Vps_printf("0 - Prints only Errors and Warnings\n");
    Vps_printf("1 - Prints descriptor information in addition to level 0\n");
    Vps_printf("2 - Prints register overlay in addition to level 1\n");
    Vps_printf("Enter print level: ");
    scanf("%s",  inputStr);
    printLevel = (UInt32)atoi(inputStr);

    App_printList(listAddr, listSize, printLevel);

    Vps_printf("\nVPDMA Descriptor Print Successful!!\n");
    Vps_printf("************************************************************\n");

    return;
}

/**
 *  App_mainTestTask
 *  Application test task.
 */
Void App_mainTestTask(void)
{
    Int32   input;

    Vps_printf("\n************************************************************\n");
    Vps_printf("VPDMA Utils App Start...\n\n");

    VpsHal_vpdmaInitDebug(CSL_VPS_VPDMA_0_REGS);

    while (1u)
    {
        Vps_printf("Menu Options:\n");
        Vps_printf("-------------\n");
        Vps_printf("1 - Prints VPDMA List Manger Dump\n");
        Vps_printf("2 - Prints VPDMA descriptor\n");
        Vps_printf("0 - To exit\n");
        Vps_printf("\nEnter Option: ");
        scanf("%d", &input);

        if (input == 1)
        {
            App_printAllStatus();
        }
        else if (input == 2)
        {
            App_printListDescriptor();
        }
        else if (input == 0)
        {
            break;
        }
        else
        {
            Vps_printf("Invalid Option. Try Agian!!\n");
        }
    }

    Vps_printf("\nVPDMA Utils App Successful!!\n");
    Vps_printf("************************************************************\n");

    return;
}

#define CSL_TI81XX_VPS_SIZE         0x10000
#define CSL_TI81XX_VPS_PHYS_ADDR    0x48100000u
/**
 *  main
 *  Application main function.
 */
int main(void)
{
    CSL_TI81XX_VPS_BASE             = (UInt32)OSA_dmaMapMem((unsigned char*)CSL_TI81XX_VPS_PHYS_ADDR, CSL_TI81XX_VPS_SIZE);
    CSL_VPS_VPDMA_0_REGS            = (CSL_TI81XX_VPS_BASE + 0x0000D000u);
    CSL_VPS_COMP_0_REGS             = (CSL_TI81XX_VPS_BASE + 0x00005200u);

    VpsHal_vpdmaInitDebug(CSL_VPS_VPDMA_0_REGS);
    App_printAllStatus();

    OSA_dmaUnmapMem((unsigned char*)CSL_TI81XX_VPS_BASE, CSL_TI81XX_VPS_SIZE);

    return 0;
}

