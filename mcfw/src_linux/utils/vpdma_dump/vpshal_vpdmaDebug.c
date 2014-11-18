/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
 *  \file vpshal_vpdmaDebug.c
 *
 *  \brief VPS VPDMA HAL debug functions implementation file.
 *
 */

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */

#include <string.h>

#include "vps_types.h"
#include "trace.h"
#include "vpshal_vpdma.h"
#include "vpshalVpdmaDebug.h"
#include "cslr_hd_vps_vpdma.h"
//#include <ti/psp/vps/common/vps_utils.h>


/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */

/* Value to be written to VPDMA PID to enable debug mode. */
#define VPDMA_DEBUG_ENABLE              (0xDEAD0000u)
/* Delay time in ms used between each access during VPDMA DEBUG read/write. */
#define VPDMA_DEBUG_DELAY               (1u)

/*
 * Mask to check for valid buffer address. Assuming buffers are only in DDR
 * address and/or in OCMC address from 0x40300000/0x40400000 to
 * 0x40340000/0x40440000.
 * When tiler is enabled, this check is not performed.
 */
#ifdef TI_816X_BUILD
/* For TI816x platform, we have 1024 MB DDR from 0x80000000 to 0xC0000000. */
#define VPDMA_DEBUG_DDR0_ADDR_START     (0x80000000u)
#define VPDMA_DEBUG_DDR0_ADDR_END       (0xA0000000u)
#define VPDMA_DEBUG_DDR1_ADDR_START     (0xA0000000u)
#define VPDMA_DEBUG_DDR1_ADDR_END       (0xC0000000u)
#else
/* For TI814x platform, we have 256 MB DDR from 0x80000000 to 0x90000000 and
 * another 256 MB DDR from 0xC0000000 to 0xD0000000. */
#define VPDMA_DEBUG_DDR0_ADDR_START     (0x80000000u)
#define VPDMA_DEBUG_DDR0_ADDR_END       (0x90000000u)
#define VPDMA_DEBUG_DDR1_ADDR_START     (0xC0000000u)
#define VPDMA_DEBUG_DDR1_ADDR_END       (0xD0000000u)
#endif

#ifdef TI_8107_BUILD
#define VPDMA_DEBUG_OCMC0_ADDR_START    (0x40300000u)
#define VPDMA_DEBUG_OCMC0_ADDR_END      (0x40340000u)
#else
#define VPDMA_DEBUG_OCMC0_ADDR_START    (0x40300000u)
#define VPDMA_DEBUG_OCMC0_ADDR_END      (0x40340000u)
#define VPDMA_DEBUG_OCMC1_ADDR_START    (0x40400000u)
#define VPDMA_DEBUG_OCMC1_ADDR_END      (0x40440000u)
#endif

#define VPDMA_DEBUG_MAX_OVLY_REG        (500u)


/* ========================================================================== */
/*                         Structure Declarations                             */
/* ========================================================================== */

/* None */

/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */

static Void VpsHal_vpdmaPrintOutBoundDesc(const Void *memPtr, UInt32 traceMask);
static Void VpsHal_vpdmaPrintConfigDesc(const Void *memPtr, UInt32 traceMask);
static Void VpsHal_vpdmaPrintSocCtrlDesc(const Void *memPtr, UInt32 traceMask);
static Void VpsHal_vpdmaPrintSolCtrlDesc(const Void *memPtr, UInt32 traceMask);
static Void VpsHal_vpdmaPrintSorCtrlDesc(const Void *memPtr, UInt32 traceMask);
static Void VpsHal_vpdmaPrintSotCtrlDesc(const Void *memPtr, UInt32 traceMask);
static Void VpsHal_vpdmaPrintSochCtrlDesc(const Void *memPtr, UInt32 traceMask);
static Void VpsHal_vpdmaPrintIntrChangeCtrlDesc(const Void *memPtr,
                                                UInt32 traceMask);
static Void VpsHal_vpdmaPrintSiCtrlDesc(const Void *memPtr, UInt32 traceMask);
static Void VpsHal_vpdmaPrintRlCtrlDesc(const Void *memPtr, UInt32 traceMask);
static Void VpsHal_vpdmaPrintAbtCtrlDesc(const Void *memPtr, UInt32 traceMask);
static Void VpsHal_vpdmaPrintToggleFidCtrlDesc(const Void *memPtr,
                                               UInt32 traceMask);

static char *VpsHal_vpdmaGetDataTypeStr(UInt32 dataType, UInt32 channel);

static char *VpsHal_vpdmaGetFidStr(UInt32 fid);
static char *VpsHal_vpdmaGetMemTypeStr(UInt32 memType);
static char *VpsHal_vpdmaGetClassStr(UInt32 class);
static char *VpsHal_vpdmaGetDestStr(UInt32 destination);
static char *VpsHal_vpdmaGetSocEvtStr(UInt32 socEvt);
static char *VpsHal_vpdmaGetLmFidCtrlStr(UInt32 lmFidCtrl);

static UInt32 VpsHal_vpdmaIsValidAddr(UInt32 addr, UInt32 memType);


/* ========================================================================== */
/*                            Global Variables                                */
/* ========================================================================== */

extern CSL_VpsVpdmaRegsOvly VpdmaBaseAddress;
extern UInt32 gVpsHal_vpsBaseAddress;


/* ========================================================================== */
/*                          Function Definitions                              */
/* ========================================================================== */

UInt32 VpsHal_vpdmaGetListStatus(UInt32 listNum)
{
    UInt32                  listStatus;
    CSL_VpsVpdmaRegsOvly    regOvly;

    /* Enable debug mode */
    regOvly = VpdmaBaseAddress;
    regOvly->PID = (VPDMA_DEBUG_ENABLE + listNum);
    Task_sleep(VPDMA_DEBUG_DELAY);          /* Wait for sometime */

    /* Read the list status from PID register */
    listStatus = regOvly->PID;

    return (listStatus);
}



UInt32 VpsHal_vpdmaReadLmReg(UInt32 listNum, UInt32 regOffset)
{
    UInt32                  lsb, msb, lmRegVal;
    CSL_VpsVpdmaRegsOvly    regOvly;

    /* Enable debug mode */
    regOvly = VpdmaBaseAddress;
    regOvly->PID = (VPDMA_DEBUG_ENABLE + listNum);
    Task_sleep(VPDMA_DEBUG_DELAY);          /* Wait for sometime */

    /* Stop list */
    VpsHal_vpdmaStopList(listNum, VPSHAL_VPDMA_LT_NORMAL);
    Task_sleep(VPDMA_DEBUG_DELAY);          /* Wait for sometime */

    /*
     *  Read LSB of LM register
     */
    /* Post list */
    VpsHal_vpdmaPostList(
        listNum,
        VPSHAL_VPDMA_LT_DEBUG,
        (Ptr) (regOffset * 2u),
        0u,
        FALSE);
    Task_sleep(VPDMA_DEBUG_DELAY);          /* Wait for sometime */
    lsb = regOvly->PID;
    VpsHal_vpdmaStopList(listNum, VPSHAL_VPDMA_LT_NORMAL);

    /*
     *  Read MSB of LM register
     */
    /* Post list */
    VpsHal_vpdmaPostList(
        listNum,
        VPSHAL_VPDMA_LT_DEBUG,
        (Ptr) (regOffset * 2u + 1u),
        0u,
        FALSE);
    Task_sleep(VPDMA_DEBUG_DELAY);          /* Wait for sometime */
    msb = regOvly->PID;
    VpsHal_vpdmaStopList(listNum, VPSHAL_VPDMA_LT_NORMAL);

    lmRegVal = (((msb & 0xFFu) << 8u) | (lsb & 0xFFu));
    return (lmRegVal);
}



Int32 VpsHal_vpdmaReadChMemory(UInt32 listNum, UInt32 chNum, UInt32 *chMemory)
{
    UInt32      index, regOffset;

    regOffset = 4u * chNum;
    for (index = 0u; index < 8u; index++)
    {
        chMemory[index] = VpsHal_vpdmaReadLmReg(listNum, regOffset);
        regOffset++;
    }

    return (VPS_SOK);
}



Int32 VpsHal_vpdmaSetPerfMonRegs(void)
{
    CSL_VpsVpdmaRegsOvly    regOvly;

    regOvly = VpdmaBaseAddress;
    regOvly->PERF_MON34 = 0x43050000u;
    regOvly->PERF_MON35 = 0x43050000u;
    regOvly->PERF_MON38 = 0x43050000u;
    regOvly->PERF_MON39 = 0x43050000u;

    return 0;
}

Int32 VpsHal_vpdmaPrintPerfMonRegs(void)
{
    CSL_VpsVpdmaRegsOvly    regOvly;

    regOvly = VpdmaBaseAddress;

    GT_4trace(VpsHalVpdmaDebugTrace, GT_INFO,
        " VPDMA: PERF_MON34 = 0x%08x, PERF_MON35 = 0x%08x, "
        "PERF_MON38 = 0x%08x, PERF_MON39 = 0x%08x \r\n",
        regOvly->PERF_MON34,
        regOvly->PERF_MON35,
        regOvly->PERF_MON38,
        regOvly->PERF_MON39);

    return 0;
}

Void VpsHal_vpdmaPrintList(UInt32 listAddr,
                           UInt32 listSize,
                           UInt32 *rlListAddr,
                           UInt32 *rlListSize,
                           UInt32 printLevel)
{
    UInt32                          index;
    Bool                            rlParsed = FALSE;
    UInt32                          traceMask;
    const UInt8                    *bytePtr;
    const VpsHal_VpdmaConfigDesc   *tempDesc;
    const VpsHal_VpdmaInDataDesc   *tempInDesc;
    const VpsHal_VpdmaReloadDesc   *tempCtrlDesc;

    /* Determine trace masks */
    if (0u == printLevel)
    {
        traceMask = (GT_ERR | GT_TraceState_Enable);
    }
    else if (1u == printLevel)
    {
        traceMask = (GT_INFO | GT_TraceState_Enable);
    }
    else if (2u == printLevel)
    {
        traceMask = (GT_INFO1 | GT_TraceState_Enable);
    }
    else
    {
        traceMask = VpsHalVpdmaDebugTrace;
    }

    if (NULL != rlListAddr)
    {
        *rlListAddr = 0u;
    }
    if (NULL != rlListSize)
    {
        *rlListSize = 0u;
    }

    if (listAddr & (VPSHAL_VPDMA_LIST_ADDR_ALIGN - 1u))
    {
        GT_0trace(traceMask, GT_ERR,
            "[Warning] List address not aligned!!\n");
    }

    bytePtr = (const UInt8 *) listAddr;
    for (index = 0u; index < listSize; )
    {
        tempDesc = (const VpsHal_VpdmaConfigDesc *) bytePtr;
        switch (tempDesc->descType)
        {
            case VPSHAL_VPDMA_PT_DATA:
                tempInDesc = (const VpsHal_VpdmaInDataDesc *) bytePtr;
                if (VPSHAL_VPDMA_INBOUND_DATA_DESC == tempInDesc->direction)
                {
                    VpsHal_vpdmaPrintInBoundDesc(bytePtr, traceMask);
                }
                else
                {
                    VpsHal_vpdmaPrintOutBoundDesc(bytePtr, traceMask);
                }
                bytePtr += VPSHAL_VPDMA_DATA_DESC_SIZE;
                index += 2u;
                break;

            case VPSHAL_VPDMA_PT_CONFIG:
                VpsHal_vpdmaPrintConfigDesc(bytePtr, traceMask);
                bytePtr += VPSHAL_VPDMA_CONFIG_DESC_SIZE;
                index += 1u;
                break;

            case VPSHAL_VPDMA_PT_CONTROL:
                tempCtrlDesc = (const VpsHal_VpdmaReloadDesc *) bytePtr;
                switch (tempCtrlDesc->ctrl)
                {
                    case VPSHAL_VPDMA_CDT_SOC:
                        VpsHal_vpdmaPrintSocCtrlDesc(bytePtr, traceMask);
                        break;

                    case VPSHAL_VPDMA_CDT_SOL:
                        VpsHal_vpdmaPrintSolCtrlDesc(bytePtr, traceMask);
                        break;

                    case VPSHAL_VPDMA_CDT_SOR:
                        VpsHal_vpdmaPrintSorCtrlDesc(bytePtr, traceMask);
                        break;

                    case VPSHAL_VPDMA_CDT_SOT:
                        VpsHal_vpdmaPrintSotCtrlDesc(bytePtr, traceMask);
                        break;

                    case VPSHAL_VPDMA_CDT_SOCH:
                        VpsHal_vpdmaPrintSochCtrlDesc(bytePtr, traceMask);
                        break;

                    case VPSHAL_VPDMA_CDT_INTR_CHANGE:
                        VpsHal_vpdmaPrintIntrChangeCtrlDesc(bytePtr, traceMask);
                        break;

                    case VPSHAL_VPDMA_CDT_SI:
                        VpsHal_vpdmaPrintSiCtrlDesc(bytePtr, traceMask);
                        break;

                    case VPSHAL_VPDMA_CDT_RL:
                        VpsHal_vpdmaPrintRlCtrlDesc(bytePtr, traceMask);
                        rlParsed = TRUE;
                        if (NULL != rlListAddr)
                        {
                            *rlListAddr = tempCtrlDesc->reloadAddr;
                        }
                        if (NULL != rlListSize)
                        {
                            *rlListSize = tempCtrlDesc->listSize;
                        }
                        break;

                    case VPSHAL_VPDMA_CDT_ABT_CHANNEL:
                        VpsHal_vpdmaPrintAbtCtrlDesc(bytePtr, traceMask);
                        break;

                    case VPSHAL_VPDMA_CDT_TOGGLE_FID:
                        VpsHal_vpdmaPrintToggleFidCtrlDesc(bytePtr, traceMask);
                        break;

                    default:
                        GT_0trace(traceMask, GT_ERR,
                            "[Error] Illegal Control Descriptor!!\n");
                        break;
                }
                bytePtr += VPSHAL_VPDMA_CTRL_DESC_SIZE;
                index += 1;
                break;

            default:
                GT_0trace(traceMask, GT_ERR,
                    "[Error] Illegal Descriptor!!\n");
                break;
        }

        /* Stop parsing when reload descriptor is detected */
        if (TRUE == rlParsed)
        {
            if (index != listSize)
            {
                GT_0trace(traceMask, GT_ERR,
                    "[Warning] Reload Descriptor detected before "
                    "the end of actual list size!!\n");
            }
            break;
        }
    }

    return;
}



UInt32 VpsHal_vpdmaParseRegOverlay(const Void *memPtr,
                                   UInt32 payloadLength,
                                   UInt32 maxRegToParse,
                                   UInt32 *numRegParsed,
                                   UInt32 *regAddr,
                                   UInt32 *regVal,
                                   UInt32 traceMask)
{
    Int32                               retVal = VPS_SOK;
    UInt32                              addrOffset, regCnt, blockCnt;
    UInt32                              subBlockLength, numReg;
    UInt32                             *regValPtr;
    const VpsHal_VpdmaSubBlockHeader   *header;

    /* Null pointer check */
    GT_assert(VpsHalTrace, (NULL != memPtr));

    /* Convert payload length from VPDMA words to words */
    payloadLength *= 4u;
    numReg = 0u;
    blockCnt = 0u;
    while (payloadLength > 0u)
    {
        header = (VpsHal_VpdmaSubBlockHeader *) memPtr;
        addrOffset = header->nextClientAddr + gVpsHal_vpsBaseAddress;
        subBlockLength = header->subBlockLength;

        /* Check if sub block length is more than payload length (in words) */
        if (subBlockLength >
            (payloadLength - (sizeof(VpsHal_VpdmaSubBlockHeader) / 4u)))
        {
            GT_0trace(traceMask, GT_ERR,
                "[Error] Subblock length is more than payload length!!\n");
            retVal = VPS_EFAIL;
            break;
        }

        /* Check for errors */
        if (header->reserved1 != 0u)
        {
            GT_0trace(traceMask, GT_ERR, "[Warning] Reserved1 is not zero!!\n");
        }
        if (header->reserved2 != 0u)
        {
            GT_0trace(traceMask, GT_ERR, "[Warning] Reserved2 is not zero!!\n");
        }
        if (header->reserved3 != 0u)
        {
            GT_0trace(traceMask, GT_ERR, "[Warning] Reserved3 is not zero!!\n");
        }

        GT_1trace(traceMask, GT_INFO1, " Sub Block %d:\n", blockCnt);

        regValPtr =
            (UInt32 *) memPtr + (sizeof(VpsHal_VpdmaSubBlockHeader) / 4u);
        for (regCnt = 0u; regCnt < subBlockLength; regCnt++)
        {
            /* Check if we are exceeding the max allocated memory */
            if (numReg >= maxRegToParse)
            {
                GT_0trace(traceMask, GT_ERR,
                    "[Warning] Aborting parsing half way through!!\n");
                break;
            }

            GT_1trace(traceMask, GT_INFO1,
                " Reg Offset [0x%.8x]: ", addrOffset);
            GT_1trace(traceMask, GT_INFO1, "0x%.8x\n", *regValPtr);

            if (NULL != regAddr)
            {
                regAddr[numReg] = addrOffset;
            }
            if (NULL != regVal)
            {
                regVal[numReg] = *regValPtr;
            }
            regValPtr++;
            numReg++;
            addrOffset += 4u;
        }

        /* Make sub block length multiple of 4 words to go to next sub block */
        if (subBlockLength % 4u != 0u)
        {
            subBlockLength = ((subBlockLength + 4u) / 4u) * 4u;
        }
        memPtr = (((UInt8 *) memPtr) +
            (subBlockLength * 4u + sizeof(VpsHal_VpdmaSubBlockHeader)));
        payloadLength -= subBlockLength +
            (sizeof(VpsHal_VpdmaSubBlockHeader) / 4u);
        blockCnt++;
    }

    if (NULL != numRegParsed)
    {
        *numRegParsed = numReg;
    }

    return (retVal);
}



/**
 *  VpsHal_vpdmaPrintInBoundDesc
 *  \brief Prints the VPDMA in bound descriptor.
 */
Void VpsHal_vpdmaPrintInBoundDesc(const Void *memPtr, UInt32 traceMask)
{
    const UInt32                   *wordPtr;
    const VpsHal_VpdmaInDataDesc   *inDescPtr;
    const VpsHal_VpdmaRegionDataDesc *grpxDescptr;

    wordPtr = (const UInt32 *) memPtr;
    inDescPtr = (const VpsHal_VpdmaInDataDesc *) memPtr;

    GT_1trace(traceMask, GT_INFO,
        "[%.8x] Descriptor Type [Data:Inbound]\n", memPtr);

    /* Print word 0 */
    GT_1trace(traceMask, GT_INFO, "[%.8x  ", wordPtr[0u]);
    GT_2trace(traceMask, GT_INFO,
        "Data Type=0x%.1x (%s) ",
        inDescPtr->dataType,
        VpsHal_vpdmaGetDataTypeStr(inDescPtr->dataType, inDescPtr->channel));
    GT_1trace(traceMask, GT_INFO, "Notify=%d ", inDescPtr->notify);
    GT_2trace(traceMask, GT_INFO,
        "Field=%d (%s) ",
        inDescPtr->fieldId, VpsHal_vpdmaGetFidStr(inDescPtr->fieldId));
    GT_1trace(traceMask, GT_INFO, "1D=%d ", inDescPtr->oneD);
    GT_1trace(traceMask, GT_INFO, "Even Line Skip=%d ", inDescPtr->evenSkip);
    GT_1trace(traceMask, GT_INFO, "Odd Line Skip=%d ", inDescPtr->oddSkip);
    GT_2trace(traceMask, GT_INFO,
        "Line Stride=0x%x (%d)\n",
        inDescPtr->lineStride, inDescPtr->lineStride);

    /* Print word 1 */
    GT_1trace(traceMask, GT_INFO, " %.8x  ", wordPtr[1u]);
    if (inDescPtr->oneD)
    {
        GT_1trace(traceMask, GT_INFO, "Transfer Size=%d\n", wordPtr[1u]);
    }
    else
    {
        GT_1trace(traceMask, GT_INFO,
            "Line Length=%d ", inDescPtr->transferWidth);
        GT_1trace(traceMask, GT_INFO,
            "Transfer Height=%d\n", inDescPtr->transferHeight);
    }

    /* Print word 2 */
    GT_1trace(traceMask, GT_INFO, " %.8x  ", wordPtr[2u]);
    GT_1trace(traceMask, GT_INFO, "Start Address=0x%.8x\n", inDescPtr->address);

    /* Print word 3 */
    GT_1trace(traceMask, GT_INFO, " %.8x  ", wordPtr[3u]);
    GT_1trace(traceMask, GT_INFO, "Packet Type=0x%.1x ", inDescPtr->descType);
    GT_1trace(traceMask, GT_INFO,
        "Mem Type=%s ", VpsHal_vpdmaGetMemTypeStr(inDescPtr->memType));
    GT_1trace(traceMask, GT_INFO, "Dir=%d (Inbound) ", inDescPtr->direction);
    GT_2trace(traceMask, GT_INFO,
        "Channel=%d (%s) ",
        inDescPtr->channel, VpsHal_vpdmaGetChStr(inDescPtr->channel));
    GT_1trace(traceMask, GT_INFO, "Pri=%d ", inDescPtr->priority);
    if (inDescPtr->mosaicMode)
    {
        GT_0trace(traceMask, GT_INFO, "Mosaic Mode=Enabled ");
    }
    else
    {
        GT_0trace(traceMask, GT_INFO, "Mosaic Mode=Disabled ");
    }
    GT_2trace(traceMask, GT_INFO,
        "Next Channel=%d (%s)\n",
        inDescPtr->nextChannel, VpsHal_vpdmaGetChStr(inDescPtr->nextChannel));

    /* Print word 4 */
    GT_1trace(traceMask, GT_INFO, " %.8x  ", wordPtr[4u]);
    if (inDescPtr->oneD)
    {
        GT_1trace(traceMask, GT_INFO, "Frame Size=%d\n", wordPtr[4u]);
    }
    else
    {
        GT_1trace(traceMask, GT_INFO, "Frame Width=%d ", inDescPtr->frameWidth);
        GT_1trace(traceMask, GT_INFO,
            "Frame Height=%d\n", inDescPtr->frameHeight);
    }

    /* Print word 5 */
    GT_1trace(traceMask, GT_INFO, " %.8x  ", wordPtr[5u]);
    GT_1trace(traceMask, GT_INFO,
        "Horizontal Start=%d ", inDescPtr->horizontalStart);
    GT_1trace(traceMask, GT_INFO,
        "Vertical Start=%d\n", inDescPtr->verticalStart);

    if ((VPSHAL_VPDMA_CHANNEL_GRPX0 == inDescPtr->channel) ||
        (VPSHAL_VPDMA_CHANNEL_GRPX1 == inDescPtr->channel) ||
        (VPSHAL_VPDMA_CHANNEL_GRPX2 == inDescPtr->channel))
    {
        grpxDescptr = (const VpsHal_VpdmaRegionDataDesc *) memPtr;

        /* Print word 6 */
        GT_1trace(traceMask, GT_INFO, " %.8x  ", wordPtr[6u]);
        GT_1trace(traceMask, GT_INFO,
            "Blend Alpha=0x%.2x ", grpxDescptr->blendAlpha);
        GT_1trace(traceMask, GT_INFO, "BB Alpha=0x%.2x ", grpxDescptr->bbAlpha);
        if (wordPtr[6u] & VPSHAL_VPDMA_GRPX_BOUNDRY_BOX)
        {
            GT_0trace(traceMask, GT_INFO, "BB=Enabled ");
        }
        else
        {
            GT_0trace(traceMask, GT_INFO, "BB=Disabled ");
        }
        if (wordPtr[6u] & VPSHAL_VPDMA_GRPX_STENCIL)
        {
            GT_0trace(traceMask, GT_INFO, "Stencil=Enabled ");
        }
        else
        {
            GT_0trace(traceMask, GT_INFO, "Stencil=Disabled ");
        }
        if (wordPtr[6u] & VPSHAL_VPDMA_GRPX_SCALAR)
        {
            GT_0trace(traceMask, GT_INFO, "Scalar=Enabled ");
        }
        else
        {
            GT_0trace(traceMask, GT_INFO, "Scalar=Disabled ");
        }
        if (wordPtr[6u] & VPSHAL_VPDMA_GRPX_END_REGION)
        {
            GT_0trace(traceMask, GT_INFO, "Last Region=Yes ");
        }
        else
        {
            GT_0trace(traceMask, GT_INFO, "Last Region=No ");
        }
        if (wordPtr[6u] & VPSHAL_VPDMA_GRPX_START_REGION)
        {
            GT_0trace(traceMask, GT_INFO, "Start Region=Yes ");
        }
        else
        {
            GT_0trace(traceMask, GT_INFO, "Start Region=No ");
        }
        GT_1trace(traceMask, GT_INFO,
            "Region Priority=%d\n", grpxDescptr->regionPriority);

        /* Print word 7 */
        GT_1trace(traceMask, GT_INFO, " %.8x  ", wordPtr[7u]);
        GT_1trace(traceMask, GT_INFO,
            "Trans Color=0x%.8x ", grpxDescptr->transColor);
        GT_1trace(traceMask, GT_INFO, "Trans Mask=%d ", grpxDescptr->transMask);
        if (grpxDescptr->enableTransparency)
        {
            GT_0trace(traceMask, GT_INFO, "Trans=Enabled ");
        }
        else
        {
            GT_0trace(traceMask, GT_INFO, "Trans=Disabled ");
        }
        if (VPSHAL_VPDMA_GBT_NOBLENDING == grpxDescptr->blendType)
        {
            GT_0trace(traceMask, GT_INFO, "Blend Type=Disabled\n");
        }
        else if (VPSHAL_VPDMA_GBT_GLOBALBLENDING == grpxDescptr->blendType)
        {
            GT_0trace(traceMask, GT_INFO, "Blend Type=Global\n");
        }
        else if (VPSHAL_VPDMA_GBT_CLUTBLENDING == grpxDescptr->blendType)
        {
            GT_0trace(traceMask, GT_INFO, "Blend Type=Clut\n");
        }
        else if (VPSHAL_VPDMA_GBT_PIXELBLENDING == grpxDescptr->blendType)
        {
            GT_0trace(traceMask, GT_INFO, "Blend Type=Pixel\n");
        }
        else
        {
            GT_0trace(traceMask, GT_ERR, "[Warning] Unknown blending type!!\n");
        }
    }
    else
    {
        /* Print word 6 */
        GT_1trace(traceMask, GT_INFO, " %.8x  ", wordPtr[6u]);
        GT_1trace(traceMask, GT_INFO,
            "Client Specific Attributes=%.8x\n", inDescPtr->clientSpecific1);

        /* Print word 7 */
        GT_1trace(traceMask, GT_INFO, " %.8x  ", wordPtr[7u]);
        GT_1trace(traceMask, GT_INFO,
            "Client Specific Attributes=%.8x\n", inDescPtr->clientSpecific2);
    }

    /* Check for errors */
    if (inDescPtr->lineStride & (VPSHAL_VPDMA_LINE_STRIDE_ALIGN - 1u))
    {
        GT_0trace(traceMask, GT_ERR, "[Warning] Line stride not aligned!!\n");
    }
    if (!inDescPtr->oneD &&
        ((0u ==inDescPtr->transferWidth) || (0u ==inDescPtr->transferHeight)))
    {
        GT_0trace(traceMask, GT_INFO, "[Info] Dummy data descriptor!!\n");
    }
    else
    {
        if (inDescPtr->address & (VPSHAL_VPDMA_BUF_ADDR_ALIGN - 1u))
        {
            GT_0trace(traceMask, GT_ERR,
                "[Warning] Buffer address not aligned!!\n");
        }
        if (!VpsHal_vpdmaIsValidAddr(inDescPtr->address, inDescPtr->memType))
        {
            GT_0trace(traceMask, GT_ERR,
                "[Warning] Buffer address out of range!!\n");
        }
    }
    if ((inDescPtr->channel >= VPSHAL_VPDMA_CHANNEL_NUM_CHANNELS) &&
        (inDescPtr->mosaicMode))
    {
        GT_0trace(traceMask, GT_ERR,
            "[Warning] Mosaic Mode enabled for free channels!!\n");
    }
    if (inDescPtr->reserved1 != 0u)
    {
        GT_0trace(traceMask, GT_ERR, "[Warning] Reserved1 is not zero!!\n");
    }
    if (inDescPtr->reserved2 != 0u)
    {
        GT_0trace(traceMask, GT_ERR, "[Warning] Reserved2 is not zero!!\n");
    }
    if (inDescPtr->reserved3 != 0u)
    {
        GT_0trace(traceMask, GT_ERR, "[Warning] Reserved3 is not zero!!\n");
    }

    GT_0trace(traceMask, GT_INFO, "]\n");

    return;
}



/**
 *  VpsHal_vpdmaPrintOutBoundDesc
 *  \brief Prints the VPDMA out bound descriptor.
 */
static Void VpsHal_vpdmaPrintOutBoundDesc(const Void *memPtr, UInt32 traceMask)
{
    const UInt32                   *wordPtr;
    const VpsHal_VpdmaOutDataDesc  *outDescPtr;
    UInt32                          outDescAddr;

    wordPtr = (const UInt32 *) memPtr;
    outDescPtr = (const VpsHal_VpdmaOutDataDesc *) memPtr;

    GT_1trace(traceMask, GT_INFO,
        "[0x%.8x] Descriptor Type [Data:Outbound]\n", memPtr);

    /* Print word 0 */
    GT_1trace(traceMask, GT_INFO, "[%.8x  ", wordPtr[0u]);
    GT_2trace(traceMask, GT_INFO,
        "Data Type=0x%.1x (%s) ",
        outDescPtr->dataType,
        VpsHal_vpdmaGetDataTypeStr(outDescPtr->dataType, outDescPtr->channel));
    GT_1trace(traceMask, GT_INFO, "Notify=%d ", outDescPtr->notify);
    GT_1trace(traceMask, GT_INFO, "1D=%d ", outDescPtr->oneD);
    GT_1trace(traceMask, GT_INFO, "Even Line Skip=%d ", outDescPtr->evenSkip);
    GT_1trace(traceMask, GT_INFO, "Odd Line Skip=%d ", outDescPtr->oddSkip);
    GT_2trace(traceMask, GT_INFO,
        "Line Stride=0x%x (%d)\n",
        outDescPtr->lineStride, outDescPtr->lineStride);

    /* Print word 1 */
    GT_1trace(traceMask, GT_INFO, " %.8x  ", wordPtr[1u]);
    GT_0trace(traceMask, GT_INFO, "\n");

    /* Print word 2 */
    GT_1trace(traceMask, GT_INFO, " %.8x  ", wordPtr[2u]);
    GT_1trace(traceMask, GT_INFO,
        "Start Address=0x%.8x\n", outDescPtr->address);

    /* Print word 3 */
    GT_1trace(traceMask, GT_INFO, " %.8x  ", wordPtr[3u]);
    GT_1trace(traceMask, GT_INFO, "Packet Type=0x%.1x ", outDescPtr->descType);
    GT_1trace(traceMask, GT_INFO,
        "Mem Type=%s ", VpsHal_vpdmaGetMemTypeStr(outDescPtr->memType));
    GT_1trace(traceMask, GT_INFO, "Dir=%d (Outbound) ", outDescPtr->direction);
    GT_2trace(traceMask, GT_INFO,
        "Channel=%d (%s) ",
        outDescPtr->channel, VpsHal_vpdmaGetChStr(outDescPtr->channel));
    if (outDescPtr->descSkip)
    {
        GT_0trace(traceMask, GT_INFO, "Desc Skip=ON ");
    }
    else
    {
        GT_0trace(traceMask, GT_INFO, "Desc Skip=OFF ");
    }
    GT_1trace(traceMask, GT_INFO, "Pri=%d ", outDescPtr->priority);
    GT_2trace(traceMask, GT_INFO,
        "Next Channel=%d (%s)\n",
        outDescPtr->nextChannel, VpsHal_vpdmaGetChStr(outDescPtr->nextChannel));

    /* Print word 4 */
    GT_1trace(traceMask, GT_INFO, " %.8x  ", wordPtr[4u]);
    outDescAddr = outDescPtr->outDescAddress << 5u;
    GT_1trace(traceMask, GT_INFO,
        "Descriptor Write Address=0x%.8x ", outDescAddr);
    GT_1trace(traceMask, GT_INFO, "Wr Desc=%d ", outDescPtr->writeDesc);
    GT_1trace(traceMask, GT_INFO, "Drop Data=%d ", outDescPtr->dropData);
    GT_1trace(traceMask, GT_INFO, "Desc Reg=%d\n", outDescPtr->useDescReg);

    /* Print word 5 */
    GT_1trace(traceMask, GT_INFO, " %.8x  ", wordPtr[5u]);
    GT_1trace(traceMask, GT_INFO, "Max Width=%d ", outDescPtr->maxWidth);
    GT_1trace(traceMask, GT_INFO, "Max Height=%d\n", outDescPtr->maxHeight);

    /* Print word 6 */
    GT_1trace(traceMask, GT_INFO, " %.8x  ", wordPtr[6u]);
    GT_1trace(traceMask, GT_INFO,
        "Client Specific Attributes=%.8x\n", outDescPtr->clientSpecific1);

    /* Print word 7 */
    GT_1trace(traceMask, GT_INFO, " %.8x  ", wordPtr[7u]);
    GT_1trace(traceMask, GT_INFO,
        "Client Specific Attributes=%.8x\n", outDescPtr->clientSpecific2);

    /* Check for errors */
    if (outDescPtr->lineStride & (VPSHAL_VPDMA_LINE_STRIDE_ALIGN - 1u))
    {
        GT_0trace(traceMask, GT_ERR, "[Warning] Line stride not aligned!!\n");
    }
    if (outDescPtr->address & (VPSHAL_VPDMA_BUF_ADDR_ALIGN - 1u))
    {
        GT_0trace(traceMask, GT_ERR,
            "[Warning] Buffer address not aligned!!\n");
    }
    if (!VpsHal_vpdmaIsValidAddr(outDescPtr->address, outDescPtr->memType))
    {
        GT_0trace(traceMask, GT_ERR,
            "[Warning] Buffer address out of range!!\n");
    }
    if (outDescAddr & (VPSHAL_VPDMA_WR_DESC_BUF_ADDR_ALIGN - 1u))
    {
        GT_0trace(traceMask, GT_ERR,
            "[Warning] Write Buffer address not aligned!!\n");
    }
    if (outDescPtr->reserved1 != 0u)
    {
        GT_0trace(traceMask, GT_ERR, "[Warning] Reserved1 is not zero!!\n");
    }
    if (outDescPtr->reserved2 != 0u)
    {
        GT_0trace(traceMask, GT_ERR, "[Warning] Reserved2 is not zero!!\n");
    }
    if (outDescPtr->reserved3 != 0u)
    {
        GT_0trace(traceMask, GT_ERR, "[Warning] Reserved3 is not zero!!\n");
    }
    if (outDescPtr->reserved4 != 0u)
    {
        GT_0trace(traceMask, GT_ERR, "[Warning] Reserved4 is not zero!!\n");
    }
    if (outDescPtr->reserved5 != 0u)
    {
        GT_0trace(traceMask, GT_ERR, "[Warning] Reserved5 is not zero!!\n");
    }
    if (outDescPtr->reserved6 != 0u)
    {
        GT_0trace(traceMask, GT_ERR, "[Warning] Reserved6 is not zero!!\n");
    }
    if (outDescPtr->reserved7 != 0u)
    {
        GT_0trace(traceMask, GT_ERR, "[Warning] Reserved7 is not zero!!\n");
    }

    GT_0trace(traceMask, GT_INFO, "]\n");

    return;
}



/**
 *  VpsHal_vpdmaPrintConfigDesc
 *  \brief Prints the VPDMA configuration descriptor.
 */
static Void VpsHal_vpdmaPrintConfigDesc(const Void *memPtr, UInt32 traceMask)
{
    const UInt32                   *wordPtr;
    const UInt32                   *grpxCfgWordPtr;
    const VpsHal_VpdmaConfigDesc   *cfgDescPtr;

    wordPtr = (const UInt32 *) memPtr;
    cfgDescPtr = (const VpsHal_VpdmaConfigDesc *) memPtr;

    GT_1trace(traceMask, GT_INFO,
        "[0x%.8x] Descriptor Type [Configuration]\n", memPtr);

    /* Print word 0 */
    GT_1trace(traceMask, GT_INFO, "[%.8x  ", wordPtr[0u]);
    GT_1trace(traceMask, GT_INFO,
        "Address Offset of Destination=0x%.8x\n", cfgDescPtr->destAddr);

    /* Print word 1 */
    GT_1trace(traceMask, GT_INFO, " %.8x  ", wordPtr[1u]);
    GT_1trace(traceMask, GT_INFO, "Data Length=%d\n", cfgDescPtr->dataLength);

    /* Print word 2 */
    GT_1trace(traceMask, GT_INFO, " %.8x  ", wordPtr[2u]);
    GT_1trace(traceMask, GT_INFO,
        "Payload Address=0x%.8x\n", cfgDescPtr->payloadAddress);

    /* Print word 3 */
    GT_1trace(traceMask, GT_INFO, " %.8x  ", wordPtr[3u]);
    GT_1trace(traceMask, GT_INFO,
        "Packet Type=0x%.1x (Configuration) ", cfgDescPtr->descType);
    GT_1trace(traceMask, GT_INFO, "Direct=%d ", cfgDescPtr->direct);
    GT_2trace(traceMask, GT_INFO,
        "Class=%d (%s) ",
        cfgDescPtr->class, VpsHal_vpdmaGetClassStr(cfgDescPtr->class));
    GT_1trace(traceMask, GT_INFO,
        "Destination=%s ", VpsHal_vpdmaGetDestStr(cfgDescPtr->destination));
    GT_1trace(traceMask, GT_INFO,
        "Payload Length=%d\n", cfgDescPtr->payloadLength);

    /* Check for errors */
    if (0u == cfgDescPtr->payloadLength)
    {
        GT_0trace(traceMask, GT_INFO, "[Info] Dummy descriptor!!\n");
    }
    else
    {
        if (cfgDescPtr->payloadAddress & (VPSHAL_VPDMA_PAYLOADSIZEALIGN - 1u))
        {
            GT_0trace(traceMask, GT_ERR,
                "[Warning] Payload address not aligned!!\n");
        }
        if (!VpsHal_vpdmaIsValidAddr(cfgDescPtr->payloadAddress,
                                     VPSHAL_VPDMA_MT_NONTILEDMEM))
        {
            GT_0trace(traceMask, GT_ERR,
                "[Warning] Payload address out of range!!\n");
        }
        if (VPSHAL_VPDMA_CCT_DIRECT == cfgDescPtr->direct)
        {
            GT_0trace(traceMask, GT_ERR,
                "[Warning] Direct command is used. "
                "Generally only INDIRECT command type is used!!\n");
        }
        if ((VPSHAL_VPDMA_CONFIG_DEST_MMR == cfgDescPtr->destination) &&
            (VPSHAL_VPDMA_CPT_ADDR_DATA_SET != cfgDescPtr->class))
        {
            GT_0trace(traceMask, GT_ERR,
                "[Warning] MMR configuration is done using block set. "
                "Generally it is done only through Address-Data set!!\n");
        }
    }
    if ((VPSHAL_VPDMA_CONFIG_DEST_MMR != cfgDescPtr->destination) &&
        (VPSHAL_VPDMA_CPT_BLOCK_SET != cfgDescPtr->class))
    {
        GT_0trace(traceMask, GT_ERR,
            "[Error] Coeff configuration is done using Address-Data set. "
            "It should be done only through Block set!!\n");
    }
    if (cfgDescPtr->reserved1 != 0u)
    {
        GT_0trace(traceMask, GT_ERR, "[Warning] Reserved1 is not zero!!\n");
    }

    /* Parse the register overlay */
    if ((VPSHAL_VPDMA_CONFIG_DEST_MMR == cfgDescPtr->destination) &&
        (VPSHAL_VPDMA_CPT_ADDR_DATA_SET == cfgDescPtr->class))
    {
        VpsHal_vpdmaParseRegOverlay(
            (UInt32 *) cfgDescPtr->payloadAddress,
            cfgDescPtr->payloadLength,
            VPDMA_DEBUG_MAX_OVLY_REG,
            NULL,
            NULL,
            NULL,
            traceMask);
    }

    /* Parse GRPX config descriptor */
    if ((VPSHAL_VPDMA_CONFIG_DEST_SC_GRPX0 == cfgDescPtr->destination) ||
        (VPSHAL_VPDMA_CONFIG_DEST_SC_GRPX1 == cfgDescPtr->destination) ||
        (VPSHAL_VPDMA_CONFIG_DEST_SC_GRPX2 == cfgDescPtr->destination))
    {
        grpxCfgWordPtr = (UInt32 *) cfgDescPtr->payloadAddress;

        /* Check for errors */
        if ((1u != cfgDescPtr->payloadLength) &&
            (12u != cfgDescPtr->payloadLength))
        {
            GT_0trace(traceMask, GT_ERR,
                "[Warning] GRPX Config payload should be 1 or 12 !!\n");
        }

        if (1u != cfgDescPtr->destAddr)
        {
            GT_0trace(traceMask, GT_ERR,
                "[Warning] Address offset should be 1 for "
                "GRPX Config descriptor!!\n");
        }

        GT_0trace(traceMask, GT_INFO1, " GRPX Config Descriptor:\n");

        /* Print word 1 */
        GT_1trace(traceMask, GT_INFO1, " %.8x  ", grpxCfgWordPtr[1u]);
        GT_1trace(traceMask, GT_INFO1,
            "Frame Width=%d ", (grpxCfgWordPtr[1u] & 0xFFFF0000u) >> 16);
        GT_1trace(traceMask, GT_INFO1,
            "Frame Height=%d\n", grpxCfgWordPtr[1u] & 0x0000FFFFu);

        if (12u == cfgDescPtr->payloadLength)
        {
            /* Print word 4 */
            GT_1trace(traceMask, GT_INFO1, " %.8x  ", grpxCfgWordPtr[20u]);
            GT_1trace(traceMask, GT_INFO1,
                "Coefh0_p0=0x%.4x ", grpxCfgWordPtr[20u] & 0x0000FFFFu);
            GT_1trace(traceMask, GT_INFO1,
                "Coefh0_p1=0x%.4x\n", (grpxCfgWordPtr[20u] & 0xFFFF0000u) >> 16);

            /* Print word 5 */
            GT_1trace(traceMask, GT_INFO1, " %.8x  ", grpxCfgWordPtr[21u]);
            GT_1trace(traceMask, GT_INFO1,
                "Coefh0_p2=0x%.4x ", grpxCfgWordPtr[21] & 0x0000FFFFu);
            GT_1trace(traceMask, GT_INFO1,
                "Coefh0_p3=0x%.4x\n", (grpxCfgWordPtr[21u] & 0xFFFF0000u) >> 16);

            /* Print word 6 */
            GT_1trace(traceMask, GT_INFO1, " %.8x  ", grpxCfgWordPtr[22u]);
            GT_1trace(traceMask, GT_INFO1,
                "Coefh0_p4=0x%.4x ", grpxCfgWordPtr[22u] & 0x0000FFFFu);
            GT_1trace(traceMask, GT_INFO1,
                "Coefh0_p5=0x%.4x\n", (grpxCfgWordPtr[22u] & 0xFFFF0000u) >> 16);

            /* Print word 7 */
            GT_1trace(traceMask, GT_INFO1, " %.8x  ", grpxCfgWordPtr[23u]);
            GT_1trace(traceMask, GT_INFO1,
                "Coefh0_p6=0x%.4x ", grpxCfgWordPtr[23u] & 0x0000FFFFu);
            GT_1trace(traceMask, GT_INFO1,
                "Coefh0_p7=0x%.4x\n", (grpxCfgWordPtr[23u] & 0xFFFF0000u) >> 16);

            /* Print word 8 */
            GT_1trace(traceMask, GT_INFO1, " %.8x  ", grpxCfgWordPtr[16u]);
            GT_1trace(traceMask, GT_INFO1,
                "Coefh1_p0=0x%.4x ", grpxCfgWordPtr[16u] & 0x0000FFFFu);
            GT_1trace(traceMask, GT_INFO1,
                "Coefh1_p1=0x%.4x\n", (grpxCfgWordPtr[16u] & 0xFFFF0000u) >> 16);

            /* Print word 9 */
            GT_1trace(traceMask, GT_INFO1, " %.8x  ", grpxCfgWordPtr[17u]);
            GT_1trace(traceMask, GT_INFO1,
                "Coefh1_p2=0x%.4x ", grpxCfgWordPtr[17u] & 0x0000FFFFu);
            GT_1trace(traceMask, GT_INFO1,
                "Coefh1_p3=0x%.4x\n", (grpxCfgWordPtr[17u] & 0xFFFF0000u) >> 16);

            /* Print word 10 */
            GT_1trace(traceMask, GT_INFO1, " %.8x  ", grpxCfgWordPtr[18u]);
            GT_1trace(traceMask, GT_INFO1,
                "Coefh1_p4=0x%.4x ", grpxCfgWordPtr[18u] & 0x0000FFFFu);
            GT_1trace(traceMask, GT_INFO1,
                "Coefh1_p5=0x%.4x\n", (grpxCfgWordPtr[18u] & 0xFFFF0000u) >> 16);

            /* Print word 11 */
            GT_1trace(traceMask, GT_INFO1, " %.8x  ", grpxCfgWordPtr[19u]);
            GT_1trace(traceMask, GT_INFO1,
                "Coefh1_p6=0x%.4x ", grpxCfgWordPtr[19u] & 0x0000FFFFu);
            GT_1trace(traceMask, GT_INFO1,
                "Coefh1_p7=0x%.4x\n", (grpxCfgWordPtr[19u] & 0xFFFF0000u) >> 16);

            /* Print word 12 */
            GT_1trace(traceMask, GT_INFO1, " %.8x  ", grpxCfgWordPtr[12u]);
            GT_1trace(traceMask, GT_INFO1,
                "Coefh2_p0=0x%.4x ", grpxCfgWordPtr[12u] & 0x0000FFFFu);
            GT_1trace(traceMask, GT_INFO1,
                "Coefh2_p1=0x%.4x\n", (grpxCfgWordPtr[12u] & 0xFFFF0000u) >> 16);

            /* Print word 13 */
            GT_1trace(traceMask, GT_INFO1, " %.8x  ", grpxCfgWordPtr[13u]);
            GT_1trace(traceMask, GT_INFO1,
                "Coefh2_p2=0x%.4x ", grpxCfgWordPtr[13u] & 0x0000FFFFu);
            GT_1trace(traceMask, GT_INFO1,
                "Coefh2_p3=0x%.4x\n", (grpxCfgWordPtr[13u] & 0xFFFF0000u) >> 16);

            /* Print word 14 */
            GT_1trace(traceMask, GT_INFO1, " %.8x  ", grpxCfgWordPtr[14u]);
            GT_1trace(traceMask, GT_INFO1,
                "Coefh2_p4=0x%.4x ", grpxCfgWordPtr[14u] & 0x0000FFFFu);
            GT_1trace(traceMask, GT_INFO1,
                "Coefh2_p5=0x%.4x\n", (grpxCfgWordPtr[14u] & 0xFFFF0000u) >> 16);

            /* Print word 15 */
            GT_1trace(traceMask, GT_INFO1, " %.8x  ", grpxCfgWordPtr[15u]);
            GT_1trace(traceMask, GT_INFO1,
                "Coefh2_p6=0x%.4x ", grpxCfgWordPtr[15u] & 0x0000FFFFu);
            GT_1trace(traceMask, GT_INFO1,
                "Coefh2_p7=0x%.4x\n", (grpxCfgWordPtr[15u] & 0xFFFF0000u) >> 16);

            /* Print word 16 */
            GT_1trace(traceMask, GT_INFO1, " %.8x  ", grpxCfgWordPtr[8u]);
            GT_1trace(traceMask, GT_INFO1,
                "Coefh3_p0=0x%.4x ", grpxCfgWordPtr[8u] & 0x0000FFFFu);
            GT_1trace(traceMask, GT_INFO1,
                "Coefh3_p1=0x%.4x\n", (grpxCfgWordPtr[8u] & 0xFFFF0000u) >> 16);

            /* Print word 17 */
            GT_1trace(traceMask, GT_INFO1, " %.8x  ", grpxCfgWordPtr[9u]);
            GT_1trace(traceMask, GT_INFO1,
                "Coefh3_p2=0x%.4x ", grpxCfgWordPtr[9u] & 0x0000FFFFu);
            GT_1trace(traceMask, GT_INFO1,
                "Coefh3_p3=0x%.4x\n", (grpxCfgWordPtr[9u] & 0xFFFF0000u) >> 16);

            /* Print word 18 */
            GT_1trace(traceMask, GT_INFO1, " %.8x  ", grpxCfgWordPtr[10u]);
            GT_1trace(traceMask, GT_INFO1,
                "Coefh3_p4=0x%.4x ", grpxCfgWordPtr[10u] & 0x0000FFFFu);
            GT_1trace(traceMask, GT_INFO1,
                "Coefh3_p5=0x%.4x\n", (grpxCfgWordPtr[10u] & 0xFFFF0000u) >> 16);

            /* Print word 19 */
            GT_1trace(traceMask, GT_INFO1, " %.8x  ", grpxCfgWordPtr[11u]);
            GT_1trace(traceMask, GT_INFO1,
                "Coefh3_p6=0x%.4x ", grpxCfgWordPtr[11u] & 0x0000FFFFu);
            GT_1trace(traceMask, GT_INFO1,
                "Coefh3_p7=0x%.4x\n", (grpxCfgWordPtr[11u] & 0xFFFF0000u) >> 16);

            /* Print word 20 */
            GT_1trace(traceMask, GT_INFO1, " %.8x  ", grpxCfgWordPtr[4u]);
            GT_1trace(traceMask, GT_INFO1,
                "Coefh4_p0=0x%.4x ", grpxCfgWordPtr[4u] & 0x0000FFFFu);
            GT_1trace(traceMask, GT_INFO1,
                "Coefh4_p1=0x%.4x\n", (grpxCfgWordPtr[4u] & 0xFFFF0000u) >> 16);

            /* Print word 21 */
            GT_1trace(traceMask, GT_INFO1, " %.8x  ", grpxCfgWordPtr[5u]);
            GT_1trace(traceMask, GT_INFO1,
                "Coefh4_p2=0x%.4x ", grpxCfgWordPtr[5u] & 0x0000FFFFu);
            GT_1trace(traceMask, GT_INFO1,
                "Coefh4_p3=0x%.4x\n", (grpxCfgWordPtr[5u] & 0xFFFF0000u) >> 16);

            /* Print word 22 */
            GT_1trace(traceMask, GT_INFO1, " %.8x  ", grpxCfgWordPtr[6u]);
            GT_1trace(traceMask, GT_INFO1,
                "Coefh4_p4=0x%.4x ", grpxCfgWordPtr[6u] & 0x0000FFFFu);
            GT_1trace(traceMask, GT_INFO1,
                "Coefh4_p5=0x%.4x\n", (grpxCfgWordPtr[6u] & 0xFFFF0000u) >> 16);

            /* Print word 23 */
            GT_1trace(traceMask, GT_INFO1, " %.8x  ", grpxCfgWordPtr[7u]);
            GT_1trace(traceMask, GT_INFO1,
                "Coefh4_p6=0x%.4x ", grpxCfgWordPtr[7u] & 0x0000FFFFu);
            GT_1trace(traceMask, GT_INFO1,
                "Coefh4_p7=0x%.4x\n", (grpxCfgWordPtr[7u] & 0xFFFF0000u) >> 16);

            /* Print word 24 */
            GT_1trace(traceMask, GT_INFO1, " %.8x  ", grpxCfgWordPtr[24u]);
            GT_1trace(traceMask, GT_INFO1,
                "CountValHorz=0x%.4x ", grpxCfgWordPtr[24u] & 0x0000FFFFu);
            GT_1trace(traceMask, GT_INFO1,
                "FineOffsetHorz=0x%.4x\n", (grpxCfgWordPtr[24u] & 0xFFFF0000u) >> 16);

            /* Print word 25 */
            GT_1trace(traceMask, GT_INFO1, " %.8x  ", grpxCfgWordPtr[40u]);
            GT_1trace(traceMask, GT_INFO1,
                "Coefv0_p0=0x%.4x ", grpxCfgWordPtr[40u] & 0x0000FFFFu);
            GT_1trace(traceMask, GT_INFO1,
                "Coefv0_p1=0x%.4x\n", (grpxCfgWordPtr[40u] & 0xFFFF0000u) >> 16);

            /* Print word 26 */
            GT_1trace(traceMask, GT_INFO1, " %.8x  ", grpxCfgWordPtr[41u]);
            GT_1trace(traceMask, GT_INFO1,
                "Coefv0_p2=0x%.4x ", grpxCfgWordPtr[41u] & 0x0000FFFFu);
            GT_1trace(traceMask, GT_INFO1,
                "Coefv0_p3=0x%.4x\n", (grpxCfgWordPtr[41u] & 0xFFFF0000u) >> 16);

            /* Print word 27 */
            GT_1trace(traceMask, GT_INFO1, " %.8x  ", grpxCfgWordPtr[42u]);
            GT_1trace(traceMask, GT_INFO1,
                "Coefv0_p4=0x%.4x ", grpxCfgWordPtr[42u] & 0x0000FFFFu);
            GT_1trace(traceMask, GT_INFO1,
                "Coefv0_p5=0x%.4x\n", (grpxCfgWordPtr[42u] & 0xFFFF0000u) >> 16);

            /* Print word 28 */
            GT_1trace(traceMask, GT_INFO1, " %.8x  ", grpxCfgWordPtr[43u]);
            GT_1trace(traceMask, GT_INFO1,
                "Coefv0_p6=0x%.4x ", grpxCfgWordPtr[43u] & 0x0000FFFFu);
            GT_1trace(traceMask, GT_INFO1,
                "Coefv0_p7=0x%.4x\n", (grpxCfgWordPtr[43u] & 0xFFFF0000u) >> 16);

            /* Print word 29 */
            GT_1trace(traceMask, GT_INFO1, " %.8x  ", grpxCfgWordPtr[36u]);
            GT_1trace(traceMask, GT_INFO1,
                "Coefv1_p0=0x%.4x ", grpxCfgWordPtr[36u] & 0x0000FFFFu);
            GT_1trace(traceMask, GT_INFO1,
                "Coefv1_p1=0x%.4x\n", (grpxCfgWordPtr[36u] & 0xFFFF0000u) >> 16);

            /* Print word 30 */
            GT_1trace(traceMask, GT_INFO1, " %.8x  ", grpxCfgWordPtr[37u]);
            GT_1trace(traceMask, GT_INFO1,
                "Coefv1_p2=0x%.4x ", grpxCfgWordPtr[37u] & 0x0000FFFFu);
            GT_1trace(traceMask, GT_INFO1,
                "Coefv1_p3=0x%.4x\n", (grpxCfgWordPtr[37u] & 0xFFFF0000u) >> 16);

            /* Print word 31 */
            GT_1trace(traceMask, GT_INFO1, " %.8x  ", grpxCfgWordPtr[38u]);
            GT_1trace(traceMask, GT_INFO1,
                "Coefv1_p4=0x%.4x ", grpxCfgWordPtr[38u] & 0x0000FFFFu);
            GT_1trace(traceMask, GT_INFO1,
                "Coefv1_p5=0x%.4x\n", (grpxCfgWordPtr[38u] & 0xFFFF0000u) >> 16);

            /* Print word 32 */
            GT_1trace(traceMask, GT_INFO1, " %.8x  ", grpxCfgWordPtr[39u]);
            GT_1trace(traceMask, GT_INFO1,
                "Coefv1_p6=0x%.4x ", grpxCfgWordPtr[39u] & 0x0000FFFFu);
            GT_1trace(traceMask, GT_INFO1,
                "Coefv1_p7=0x%.4x\n", (grpxCfgWordPtr[39u] & 0xFFFF0000u) >> 16);

            /* Print word 33 */
            GT_1trace(traceMask, GT_INFO1, " %.8x  ", grpxCfgWordPtr[32u]);
            GT_1trace(traceMask, GT_INFO1,
                "Coefv2_p0=0x%.4x ", grpxCfgWordPtr[32u] & 0x0000FFFFu);
            GT_1trace(traceMask, GT_INFO1,
                "Coefv2_p1=0x%.4x\n", (grpxCfgWordPtr[32u] & 0xFFFF0000u) >> 16);

            /* Print word 34 */
            GT_1trace(traceMask, GT_INFO1, " %.8x  ", grpxCfgWordPtr[33u]);
            GT_1trace(traceMask, GT_INFO1,
                "Coefv2_p2=0x%.4x ", grpxCfgWordPtr[33u] & 0x0000FFFFu);
            GT_1trace(traceMask, GT_INFO1,
                "Coefv2_p3=0x%.4x\n", (grpxCfgWordPtr[33u] & 0xFFFF0000u) >> 16);

            /* Print word 35 */
            GT_1trace(traceMask, GT_INFO1, " %.8x  ", grpxCfgWordPtr[34u]);
            GT_1trace(traceMask, GT_INFO1,
                "Coefv2_p4=0x%.4x ", grpxCfgWordPtr[34u] & 0x0000FFFFu);
            GT_1trace(traceMask, GT_INFO1,
                "Coefv2_p5=0x%.4x\n", (grpxCfgWordPtr[34u] & 0xFFFF0000u) >> 16);

            /* Print word 36 */
            GT_1trace(traceMask, GT_INFO1, " %.8x  ", grpxCfgWordPtr[35u]);
            GT_1trace(traceMask, GT_INFO1,
                "Coefv2_p6=0x%.4x ", grpxCfgWordPtr[35u] & 0x0000FFFFu);
            GT_1trace(traceMask, GT_INFO1,
                "Coefv2_p7=0x%.4x\n", (grpxCfgWordPtr[35u] & 0xFFFF0000u) >> 16);

            /* Print word 37 */
            GT_1trace(traceMask, GT_INFO1, " %.8x  ", grpxCfgWordPtr[28u]);
            GT_1trace(traceMask, GT_INFO1,
                "Coefv3_p0=0x%.4x ", grpxCfgWordPtr[28u] & 0x0000FFFFu);
            GT_1trace(traceMask, GT_INFO1,
                "Coefv3_p1=0x%.4x\n", (grpxCfgWordPtr[28u] & 0xFFFF0000u) >> 16);

            /* Print word 38 */
            GT_1trace(traceMask, GT_INFO1, " %.8x  ", grpxCfgWordPtr[29u]);
            GT_1trace(traceMask, GT_INFO1,
                "Coefv3_p2=0x%.4x ", grpxCfgWordPtr[29u] & 0x0000FFFFu);
            GT_1trace(traceMask, GT_INFO1,
                "Coefv3_p3=0x%.4x\n", (grpxCfgWordPtr[29u] & 0xFFFF0000u) >> 16);

            /* Print word 39 */
            GT_1trace(traceMask, GT_INFO1, " %.8x  ", grpxCfgWordPtr[30u]);
            GT_1trace(traceMask, GT_INFO1,
                "Coefv3_p4=0x%.4x ", grpxCfgWordPtr[30u] & 0x0000FFFFu);
            GT_1trace(traceMask, GT_INFO1,
                "Coefv3_p5=0x%.4x\n", (grpxCfgWordPtr[30u] & 0xFFFF0000u) >> 16);

            /* Print word 40 */
            GT_1trace(traceMask, GT_INFO1, " %.8x  ", grpxCfgWordPtr[31u]);
            GT_1trace(traceMask, GT_INFO1,
                "Coefv3_p6=0x%.4x ", grpxCfgWordPtr[31u] & 0x0000FFFFu);
            GT_1trace(traceMask, GT_INFO1,
                "Coefv3_p7=0x%.4x\n", (grpxCfgWordPtr[31u] & 0xFFFF0000u) >> 16);

            /* Print word 41 */
            GT_1trace(traceMask, GT_INFO1, " %.8x  ", grpxCfgWordPtr[44u]);
            GT_1trace(traceMask, GT_INFO1,
                "CountValVert=0x%.4x ", grpxCfgWordPtr[44u] & 0x0000FFFFu);
            GT_1trace(traceMask, GT_INFO1,
                "FineOffsetVert=0x%.4x\n", (grpxCfgWordPtr[44u] & 0xFFFF0000u) >> 16);

        }
    }

    GT_0trace(traceMask, GT_INFO, "]\n");

    return;
}



/**
 *  VpsHal_vpdmaPrintSocCtrlDesc
 *  \brief Prints the VPDMA Sync on Client control descriptor.
 */
static Void VpsHal_vpdmaPrintSocCtrlDesc(const Void *memPtr, UInt32 traceMask)
{
    const UInt32                       *wordPtr;
    const VpsHal_VpdmaSyncOnClientDesc *socDescPtr;

    wordPtr = (const UInt32 *) memPtr;
    socDescPtr = (const VpsHal_VpdmaSyncOnClientDesc *) memPtr;

    GT_1trace(traceMask, GT_INFO,
        "[0x%.8x] Descriptor Type [Control:Sync On Client]\n", memPtr);

    /* Print word 0 */
    GT_1trace(traceMask, GT_INFO, "[%.8x  ", wordPtr[0u]);
    GT_0trace(traceMask, GT_INFO, "\n");

    /* Print word 1 */
    GT_1trace(traceMask, GT_INFO, " %.8x  ", wordPtr[1u]);
    GT_1trace(traceMask, GT_INFO, "Pixel Count=%d ", socDescPtr->pixelCount);
    GT_1trace(traceMask, GT_INFO, "Line Count=%d\n", socDescPtr->lineCount);

    /* Print word 2 */
    GT_1trace(traceMask, GT_INFO, " %.8x  ", wordPtr[2u]);
    GT_2trace(traceMask, GT_INFO,
        "Event=%d (%s)\n",
        socDescPtr->event, VpsHal_vpdmaGetSocEvtStr(socDescPtr->event));

    /* Print word 3 */
    GT_1trace(traceMask, GT_INFO, " %.8x  ", wordPtr[3u]);
    GT_1trace(traceMask, GT_INFO,
        "Packet Type=0x%.1x (Control) ", socDescPtr->descType);
    GT_2trace(traceMask, GT_INFO,
        "Source=%d (%s) ",
        socDescPtr->channel, VpsHal_vpdmaGetChStr(socDescPtr->channel));
    GT_1trace(traceMask, GT_INFO,
        "Control=%d (Sync On Client)\n", socDescPtr->ctrl);

    /* Check for errors */
    if (socDescPtr->reserved1 != 0u)
    {
        GT_0trace(traceMask, GT_ERR, "[Warning] Reserved1 is not zero!!\n");
    }
    if (socDescPtr->reserved2 != 0u)
    {
        GT_0trace(traceMask, GT_ERR, "[Warning] Reserved2 is not zero!!\n");
    }
    if (socDescPtr->reserved3 != 0u)
    {
        GT_0trace(traceMask, GT_ERR, "[Warning] Reserved3 is not zero!!\n");
    }
    if (socDescPtr->reserved4 != 0u)
    {
        GT_0trace(traceMask, GT_ERR, "[Warning] Reserved4 is not zero!!\n");
    }

    GT_0trace(traceMask, GT_INFO, "]\n");

    return;
}



/**
 *  VpsHal_vpdmaPrintSolCtrlDesc
 *  \brief Prints the VPDMA Sync on List control descriptor.
 */
static Void VpsHal_vpdmaPrintSolCtrlDesc(const Void *memPtr, UInt32 traceMask)
{
    const UInt32                       *wordPtr;
    const VpsHal_VpdmaSyncOnListDesc   *solDescPtr;

    wordPtr = (const UInt32 *) memPtr;
    solDescPtr = (const VpsHal_VpdmaSyncOnListDesc *) memPtr;

    GT_1trace(traceMask, GT_INFO,
        "[0x%.8x] Descriptor Type [Control:Sync On List]\n", memPtr);

    /* Print word 0 */
    GT_1trace(traceMask, GT_INFO, "[%.8x  ", wordPtr[0u]);
    GT_0trace(traceMask, GT_INFO, "\n");

    /* Print word 1 */
    GT_1trace(traceMask, GT_INFO, " %.8x  ", wordPtr[1u]);
    GT_0trace(traceMask, GT_INFO, "\n");

    /* Print word 2 */
    GT_1trace(traceMask, GT_INFO, " %.8x  ", wordPtr[2u]);
    GT_0trace(traceMask, GT_INFO, "\n");

    /* Print word 3 */
    GT_1trace(traceMask, GT_INFO, " %.8x  ", wordPtr[3u]);
    GT_1trace(traceMask, GT_INFO,
        "Packet Type=0x%.1x (Control) ", solDescPtr->descType);
    GT_1trace(traceMask, GT_INFO, "List=%d ", solDescPtr->lists);
    GT_1trace(traceMask, GT_INFO,
        "Control=%d (Sync On List)\n", solDescPtr->ctrl);

    /* Check for errors */
    if (solDescPtr->reserved1 != 0u)
    {
        GT_0trace(traceMask, GT_ERR, "[Warning] Reserved1 is not zero!!\n");
    }
    if (solDescPtr->reserved2 != 0u)
    {
        GT_0trace(traceMask, GT_ERR, "[Warning] Reserved2 is not zero!!\n");
    }
    if (solDescPtr->reserved3 != 0u)
    {
        GT_0trace(traceMask, GT_ERR, "[Warning] Reserved3 is not zero!!\n");
    }
    if (solDescPtr->reserved4 != 0u)
    {
        GT_0trace(traceMask, GT_ERR, "[Warning] Reserved4 is not zero!!\n");
    }
    if (solDescPtr->reserved5 != 0u)
    {
        GT_0trace(traceMask, GT_ERR, "[Warning] Reserved5 is not zero!!\n");
    }

    GT_0trace(traceMask, GT_INFO, "]\n");

    return;
}



/**
 *  VpsHal_vpdmaPrintSorCtrlDesc
 *  \brief Prints the VPDMA Sync on Register control descriptor.
 */
static Void VpsHal_vpdmaPrintSorCtrlDesc(const Void *memPtr, UInt32 traceMask)
{
    const UInt32                       *wordPtr;
    const VpsHal_VpdmaSyncOnRegDesc    *sorDescPtr;

    wordPtr = (const UInt32 *) memPtr;
    sorDescPtr = (const VpsHal_VpdmaSyncOnRegDesc *) memPtr;

    GT_1trace(traceMask, GT_INFO,
        "[0x%.8x] Descriptor Type [Control:Sync On External Event]\n", memPtr);

    /* Print word 0 */
    GT_1trace(traceMask, GT_INFO, "[%.8x  ", wordPtr[0u]);
    GT_0trace(traceMask, GT_INFO, "\n");

    /* Print word 1 */
    GT_1trace(traceMask, GT_INFO, " %.8x  ", wordPtr[1u]);
    GT_0trace(traceMask, GT_INFO, "\n");

    /* Print word 2 */
    GT_1trace(traceMask, GT_INFO, " %.8x  ", wordPtr[2u]);
    GT_0trace(traceMask, GT_INFO, "\n");

    /* Print word 3 */
    GT_1trace(traceMask, GT_INFO, " %.8x  ", wordPtr[3u]);
    GT_1trace(traceMask, GT_INFO,
        "Packet Type=0x%.1x (Control) ", sorDescPtr->descType);
    GT_1trace(traceMask, GT_INFO, "List=%d ", sorDescPtr->listNum);
    GT_1trace(traceMask, GT_INFO,
        "Control=%d (Sync On External Event)\n", sorDescPtr->ctrl);

    /* Check for errors */
    if (sorDescPtr->listNum >= VPSHAL_VPDMA_MAX_LIST)
    {
        GT_0trace(traceMask, GT_ERR, "[Warning] Invalid list number!!\n");
    }
    if (sorDescPtr->reserved1 != 0u)
    {
        GT_0trace(traceMask, GT_ERR, "[Warning] Reserved1 is not zero!!\n");
    }
    if (sorDescPtr->reserved2 != 0u)
    {
        GT_0trace(traceMask, GT_ERR, "[Warning] Reserved2 is not zero!!\n");
    }
    if (sorDescPtr->reserved3 != 0u)
    {
        GT_0trace(traceMask, GT_ERR, "[Warning] Reserved3 is not zero!!\n");
    }
    if (sorDescPtr->reserved4 != 0u)
    {
        GT_0trace(traceMask, GT_ERR, "[Warning] Reserved4 is not zero!!\n");
    }
    if (sorDescPtr->reserved5 != 0u)
    {
        GT_0trace(traceMask, GT_ERR, "[Warning] Reserved5 is not zero!!\n");
    }

    GT_0trace(traceMask, GT_INFO, "]\n");

    return;
}



/**
 *  VpsHal_vpdmaPrintSotCtrlDesc
 *  \brief Prints the VPDMA Sync on Timer control descriptor.
 */
static Void VpsHal_vpdmaPrintSotCtrlDesc(const Void *memPtr, UInt32 traceMask)
{
    const UInt32                        *wordPtr;
    const VpsHal_VpdmaSyncOnLmTimerDesc *sotDescPtr;

    wordPtr = (const UInt32 *) memPtr;
    sotDescPtr = (const VpsHal_VpdmaSyncOnLmTimerDesc *) memPtr;

    GT_1trace(traceMask, GT_INFO,
        "[0x%.8x] Descriptor Type [Control:Sync on LM Timer]\n", memPtr);

    /* Print word 0 */
    GT_1trace(traceMask, GT_INFO, "[%.8x  ", wordPtr[0u]);
    GT_1trace(traceMask, GT_INFO,
        "Number of Cycles=%d\n", sotDescPtr->numCycles);

    /* Print word 1 */
    GT_1trace(traceMask, GT_INFO, " %.8x  ", wordPtr[1u]);
    GT_0trace(traceMask, GT_INFO, "\n");

    /* Print word 2 */
    GT_1trace(traceMask, GT_INFO, " %.8x  ", wordPtr[2u]);
    GT_0trace(traceMask, GT_INFO, "\n");

    /* Print word 3 */
    GT_1trace(traceMask, GT_INFO, " %.8x  ", wordPtr[3u]);
    GT_1trace(traceMask, GT_INFO,
        "Packet Type=0x%.1x (Control) ", sotDescPtr->descType);
    GT_1trace(traceMask, GT_INFO,
        "Control=%d (Sync on LM Timer)\n", sotDescPtr->ctrl);

    /* Check for errors */
    if (sotDescPtr->reserved1 != 0u)
    {
        GT_0trace(traceMask, GT_ERR, "[Warning] Reserved1 is not zero!!\n");
    }
    if (sotDescPtr->reserved2 != 0u)
    {
        GT_0trace(traceMask, GT_ERR, "[Warning] Reserved2 is not zero!!\n");
    }
    if (sotDescPtr->reserved3 != 0u)
    {
        GT_0trace(traceMask, GT_ERR, "[Warning] Reserved3 is not zero!!\n");
    }
    if (sotDescPtr->reserved4 != 0u)
    {
        GT_0trace(traceMask, GT_ERR, "[Warning] Reserved4 is not zero!!\n");
    }

    GT_0trace(traceMask, GT_INFO, "]\n");

    return;
}



/**
 *  VpsHal_vpdmaPrintSochCtrlDesc
 *  \brief Prints the VPDMA Sync on Channel control descriptor.
 */
static Void VpsHal_vpdmaPrintSochCtrlDesc(const Void *memPtr, UInt32 traceMask)
{
    const UInt32                        *wordPtr;
    const VpsHal_VpdmaSyncOnChannelDesc *sochDescPtr;

    wordPtr = (const UInt32 *) memPtr;
    sochDescPtr = (const VpsHal_VpdmaSyncOnChannelDesc *) memPtr;

    GT_1trace(traceMask, GT_INFO,
        "[0x%.8x] Descriptor Type [Control:Sync on Channel]\n", memPtr);

    /* Print word 0 */
    GT_1trace(traceMask, GT_INFO, "[%.8x  ", wordPtr[0u]);
    GT_0trace(traceMask, GT_INFO, "\n");

    /* Print word 1 */
    GT_1trace(traceMask, GT_INFO, " %.8x  ", wordPtr[1u]);
    GT_0trace(traceMask, GT_INFO, "\n");

    /* Print word 2 */
    GT_1trace(traceMask, GT_INFO, " %.8x  ", wordPtr[2u]);
    GT_0trace(traceMask, GT_INFO, "\n");

    /* Print word 3 */
    GT_1trace(traceMask, GT_INFO, " %.8x  ", wordPtr[3u]);
    GT_1trace(traceMask, GT_INFO,
        "Packet Type=0x%.1x (Control) ", sochDescPtr->descType);
    GT_2trace(traceMask, GT_INFO,
        "Channel=%d (%s) ",
        sochDescPtr->channel, VpsHal_vpdmaGetChStr(sochDescPtr->channel));
    GT_1trace(traceMask, GT_INFO,
        "Control=%d (Sync On Channel)\n", sochDescPtr->ctrl);

    /* Check for errors */
    if (sochDescPtr->reserved1 != 0u)
    {
        GT_0trace(traceMask, GT_ERR, "[Warning] Reserved1 is not zero!!\n");
    }
    if (sochDescPtr->reserved2 != 0u)
    {
        GT_0trace(traceMask, GT_ERR, "[Warning] Reserved2 is not zero!!\n");
    }
    if (sochDescPtr->reserved3 != 0u)
    {
        GT_0trace(traceMask, GT_ERR, "[Warning] Reserved3 is not zero!!\n");
    }
    if (sochDescPtr->reserved4 != 0u)
    {
        GT_0trace(traceMask, GT_ERR, "[Warning] Reserved4 is not zero!!\n");
    }
    if (sochDescPtr->reserved5 != 0u)
    {
        GT_0trace(traceMask, GT_ERR, "[Warning] Reserved5 is not zero!!\n");
    }

    GT_0trace(traceMask, GT_INFO, "]\n");

    return;
}



/**
 *  VpsHal_vpdmaPrintIntrChangeCtrlDesc
 *  \brief Prints the VPDMA interrupt change control descriptor.
 */
static Void VpsHal_vpdmaPrintIntrChangeCtrlDesc(const Void *memPtr,
                                                UInt32 traceMask)
{
    const UInt32   *wordPtr;

    wordPtr = (const UInt32 *) memPtr;

    GT_1trace(traceMask, GT_INFO,
        "[0x%.8x] Descriptor Type [Control:Change Client Interrupt]\n", memPtr);

    /* Print word 0 */
    GT_1trace(traceMask, GT_INFO, "[%.8x  ", wordPtr[0u]);
    GT_0trace(traceMask, GT_INFO, "\n");

    /* Print word 1 */
    GT_1trace(traceMask, GT_INFO, " %.8x  ", wordPtr[1u]);
    GT_0trace(traceMask, GT_INFO, "\n");

    /* Print word 2 */
    GT_1trace(traceMask, GT_INFO, " %.8x  ", wordPtr[2u]);
    GT_0trace(traceMask, GT_INFO, "\n");

    /* Print word 3 */
    GT_1trace(traceMask, GT_INFO, " %.8x  ", wordPtr[3u]);
    GT_0trace(traceMask, GT_INFO, "\n");

    GT_0trace(traceMask, GT_INFO, "]\n");

    return;
}



/**
 *  VpsHal_vpdmaPrintSiCtrlDesc
 *  \brief Prints the VPDMA Send Interrupt control descriptor.
 */
static Void VpsHal_vpdmaPrintSiCtrlDesc(const Void *memPtr, UInt32 traceMask)
{
    const UInt32                   *wordPtr;
    const VpsHal_VpdmaSendIntrDesc *siDescPtr;

    wordPtr = (const UInt32 *) memPtr;
    siDescPtr = (const VpsHal_VpdmaSendIntrDesc *) memPtr;

    GT_1trace(traceMask, GT_INFO,
        "[0x%.8x] Descriptor Type [Control:Send Interrupt]\n", memPtr);

    /* Print word 0 */
    GT_1trace(traceMask, GT_INFO, "[%.8x  ", wordPtr[0u]);
    GT_0trace(traceMask, GT_INFO, "\n");

    /* Print word 1 */
    GT_1trace(traceMask, GT_INFO, " %.8x  ", wordPtr[1u]);
    GT_0trace(traceMask, GT_INFO, "\n");

    /* Print word 2 */
    GT_1trace(traceMask, GT_INFO, " %.8x  ", wordPtr[2u]);
    GT_0trace(traceMask, GT_INFO, "\n");

    /* Print word 3 */
    GT_1trace(traceMask, GT_INFO, " %.8x  ", wordPtr[3u]);
    GT_1trace(traceMask, GT_INFO,
        "Packet Type=0x%.1x (Control) ", siDescPtr->descType);
    GT_2trace(traceMask, GT_INFO,
        "Source=0x%.1x (%d) ", siDescPtr->source, siDescPtr->source);
    GT_1trace(traceMask, GT_INFO,
        "Control=%d (Send Interrupt)\n", siDescPtr->ctrl);

    /* Check for errors */
    if (siDescPtr->source >= VPSHAL_VPDMA_MAX_SI_SOURCE)
    {
        GT_0trace(traceMask, GT_ERR, "[Warning] Invalid SI number!!\n");
    }
    if (siDescPtr->reserved1 != 0u)
    {
        GT_0trace(traceMask, GT_ERR, "[Warning] Reserved1 is not zero!!\n");
    }
    if (siDescPtr->reserved2 != 0u)
    {
        GT_0trace(traceMask, GT_ERR, "[Warning] Reserved2 is not zero!!\n");
    }
    if (siDescPtr->reserved3 != 0u)
    {
        GT_0trace(traceMask, GT_ERR, "[Warning] Reserved3 is not zero!!\n");
    }
    if (siDescPtr->reserved4 != 0u)
    {
        GT_0trace(traceMask, GT_ERR, "[Warning] Reserved4 is not zero!!\n");
    }
    if (siDescPtr->reserved5 != 0u)
    {
        GT_0trace(traceMask, GT_ERR, "[Warning] Reserved5 is not zero!!\n");
    }

    GT_0trace(traceMask, GT_INFO, "]\n");

    return;
}



/**
 *  VpsHal_vpdmaPrintRlCtrlDesc
 *  \brief Prints the VPDMA Reload control descriptor.
 */
static Void VpsHal_vpdmaPrintRlCtrlDesc(const Void *memPtr, UInt32 traceMask)
{
    const UInt32                   *wordPtr;
    const VpsHal_VpdmaReloadDesc   *rlDescPtr;

    wordPtr = (const UInt32 *) memPtr;
    rlDescPtr = (const VpsHal_VpdmaReloadDesc *) memPtr;

    if ((GT_INFO | GT_TraceState_Enable) == traceMask)
    {
        GT_3trace(traceMask, GT_INFO,
            "[0x%.8x] Reload Descriptor (RL Addr: 0x%.8x, Size: %d)\n",
            memPtr, rlDescPtr->reloadAddr, rlDescPtr->listSize);
    }

    GT_1trace(traceMask, GT_INFO1,
        "[0x%.8x] Descriptor Type [Control:Reload List]\n", memPtr);

    /* Print word 0 */
    GT_1trace(traceMask, GT_INFO1, "[%.8x  ", wordPtr[0u]);
    GT_1trace(traceMask, GT_INFO1,
        "Reload Address=0x%.8x\n", rlDescPtr->reloadAddr);

    /* Print word 1 */
    GT_1trace(traceMask, GT_INFO1, " %.8x  ", wordPtr[1u]);
    GT_1trace(traceMask, GT_INFO1, "List Size=%d\n", rlDescPtr->listSize);

    /* Print word 2 */
    GT_1trace(traceMask, GT_INFO1, " %.8x  ", wordPtr[2u]);
    GT_0trace(traceMask, GT_INFO1, "\n");

    /* Print word 3 */
    GT_1trace(traceMask, GT_INFO1, " %.8x  ", wordPtr[3u]);
    GT_1trace(traceMask, GT_INFO1,
        "Packet Type=0x%.1x (Control) ", rlDescPtr->descType);
    GT_1trace(traceMask, GT_INFO1,
        "Control=%d (Reload List)\n", rlDescPtr->ctrl);

    /* Check for errors */
    if (rlDescPtr->reloadAddr & (VPSHAL_VPDMA_LIST_ADDR_ALIGN - 1u))
    {
        GT_0trace(traceMask, GT_ERR,
            "[Warning] Reload list address not aligned!!\n");
    }
    if (rlDescPtr->reserved1 != 0u)
    {
        GT_0trace(traceMask, GT_ERR, "[Warning] Reserved1 is not zero!!\n");
    }
    if (rlDescPtr->reserved2 != 0u)
    {
        GT_0trace(traceMask, GT_ERR, "[Warning] Reserved2 is not zero!!\n");
    }
    if (rlDescPtr->reserved3 != 0u)
    {
        GT_0trace(traceMask, GT_ERR, "[Warning] Reserved3 is not zero!!\n");
    }

    GT_0trace(traceMask, GT_INFO1, "]\n");

    return;
}



/**
 *  VpsHal_vpdmaPrintAbtCtrlDesc
 *  \brief Prints the VPDMA abort control descriptor.
 */
static Void VpsHal_vpdmaPrintAbtCtrlDesc(const Void *memPtr, UInt32 traceMask)
{
    const UInt32                   *wordPtr;
    const VpsHal_VpdmaAbortDesc    *abtDescPtr;

    wordPtr = (const UInt32 *) memPtr;
    abtDescPtr = (const VpsHal_VpdmaAbortDesc *) memPtr;

    GT_1trace(traceMask, GT_INFO,
        "[0x%.8x] Descriptor Type [Control:Abort Channel]\n", memPtr);

    /* Print word 0 */
    GT_1trace(traceMask, GT_INFO, "[%.8x  ", wordPtr[0u]);
    GT_0trace(traceMask, GT_INFO, "\n");

    /* Print word 1 */
    GT_1trace(traceMask, GT_INFO, " %.8x  ", wordPtr[1u]);
    GT_0trace(traceMask, GT_INFO, "\n");

    /* Print word 2 */
    GT_1trace(traceMask, GT_INFO, " %.8x  ", wordPtr[2u]);
    GT_0trace(traceMask, GT_INFO, "\n");

    /* Print word 3 */
    GT_1trace(traceMask, GT_INFO, " %.8x  ", wordPtr[3u]);
    GT_1trace(traceMask, GT_INFO,
        "Packet Type=0x%.1x (Control) ", abtDescPtr->descType);
    GT_2trace(traceMask, GT_INFO,
        "Channel=%d (%s) ",
        abtDescPtr->channel, VpsHal_vpdmaGetChStr(abtDescPtr->channel));
    GT_1trace(traceMask, GT_INFO,
        "Control=%d (Abort Channel)\n", abtDescPtr->ctrl);

    /* Check for errors */
    if (abtDescPtr->reserved1 != 0u)
    {
        GT_0trace(traceMask, GT_ERR, "[Warning] Reserved1 is not zero!!\n");
    }
    if (abtDescPtr->reserved2 != 0u)
    {
        GT_0trace(traceMask, GT_ERR, "[Warning] Reserved2 is not zero!!\n");
    }
    if (abtDescPtr->reserved3 != 0u)
    {
        GT_0trace(traceMask, GT_ERR, "[Warning] Reserved3 is not zero!!\n");
    }
    if (abtDescPtr->reserved4 != 0u)
    {
        GT_0trace(traceMask, GT_ERR, "[Warning] Reserved4 is not zero!!\n");
    }
    if (abtDescPtr->reserved5 != 0u)
    {
        GT_0trace(traceMask, GT_ERR, "[Warning] Reserved5 is not zero!!\n");
    }

    GT_0trace(traceMask, GT_INFO, "]\n");

    return;
}



/**
 *  VpsHal_vpdmaPrintToggleFidCtrlDesc
 *  \brief Prints the VPDMA Toggle FID control descriptor.
 */
static Void VpsHal_vpdmaPrintToggleFidCtrlDesc(const Void *memPtr,
                                               UInt32 traceMask)
{
    const UInt32                       *wordPtr;
    const VpsHal_VpdmaToggleLmFidDesc  *tfidDescPtr;

    wordPtr = (const UInt32 *) memPtr;
    tfidDescPtr = (const VpsHal_VpdmaToggleLmFidDesc *) memPtr;

    GT_1trace(traceMask, GT_INFO,
        "[0x%.8x] Descriptor Type [Control:Toggle LM Field]\n", memPtr);

    /* Print word 0 */
    GT_1trace(traceMask, GT_INFO, "[%.8x  ", wordPtr[0u]);
    GT_0trace(traceMask, GT_INFO, "\n");

    /* Print word 1 */
    GT_1trace(traceMask, GT_INFO, " %.8x  ", wordPtr[1u]);
    GT_0trace(traceMask, GT_INFO, "\n");

    /* Print word 2 */
    GT_1trace(traceMask, GT_INFO, " %.8x  ", wordPtr[2u]);
    GT_2trace(traceMask, GT_INFO,
        "FID2_CTL=0x%.1x (%s) ",
        tfidDescPtr->lmFidCtrl2,
        VpsHal_vpdmaGetLmFidCtrlStr(tfidDescPtr->lmFidCtrl2));
    GT_2trace(traceMask, GT_INFO,
        "FID1_CTL=0x%.1x (%s) ",
        tfidDescPtr->lmFidCtrl1,
        VpsHal_vpdmaGetLmFidCtrlStr(tfidDescPtr->lmFidCtrl1));
    GT_2trace(traceMask, GT_INFO,
        "FID0_CTL=0x%.1x (%s)\n",
        tfidDescPtr->lmFidCtrl0,
        VpsHal_vpdmaGetLmFidCtrlStr(tfidDescPtr->lmFidCtrl0));

    /* Print word 3 */
    GT_1trace(traceMask, GT_INFO, " %.8x  ", wordPtr[3u]);
    GT_1trace(traceMask, GT_INFO,
        "Packet Type=0x%.1x (Control) ", tfidDescPtr->descType);
    GT_1trace(traceMask, GT_INFO,
        "Control=%d (Toggle LM Field)\n", tfidDescPtr->ctrl);

    /* Check for errors */
    if (tfidDescPtr->reserved1 != 0u)
    {
        GT_0trace(traceMask, GT_ERR, "[Warning] Reserved1 is not zero!!\n");
    }
    if (tfidDescPtr->reserved2 != 0u)
    {
        GT_0trace(traceMask, GT_ERR, "[Warning] Reserved2 is not zero!!\n");
    }
    if (tfidDescPtr->reserved3 != 0u)
    {
        GT_0trace(traceMask, GT_ERR, "[Warning] Reserved3 is not zero!!\n");
    }
    if (tfidDescPtr->reserved4 != 0u)
    {
        GT_0trace(traceMask, GT_ERR, "[Warning] Reserved4 is not zero!!\n");
    }

    GT_0trace(traceMask, GT_INFO, "]\n");

    return;
}



/**
 *  VpsHal_vpdmaGetDataTypeStr
 *  \brief Return the string for the VPDMA data type.
 */
static char *VpsHal_vpdmaGetDataTypeStr(UInt32 dataType, UInt32 channel)
{
    char       *str = gVpsVpdmaHalInvalidStr;

    switch (channel)
    {
        case VPSHAL_VPDMA_CHANNEL_PRI_MV0:
        case VPSHAL_VPDMA_CHANNEL_PRI_MV_OUT:
#ifdef TI_816X_BUILD
        case VPSHAL_VPDMA_CHANNEL_PRI_MV1:
        case VPSHAL_VPDMA_CHANNEL_PRI_MVSTM:
        case VPSHAL_VPDMA_CHANNEL_PRI_MVSTM_OUT:
        case VPSHAL_VPDMA_CHANNEL_AUX_MV:
        case VPSHAL_VPDMA_CHANNEL_AUX_MV_OUT:
#endif
        {
            if (VPSHAL_VPDMA_CHANDT_MV == dataType)
            {
                str = gVpsVpdmaHalMvDataTypeStr;
            }
        }
        break;

        case VPSHAL_VPDMA_CHANNEL_STENCIL0:
        case VPSHAL_VPDMA_CHANNEL_STENCIL1:
        case VPSHAL_VPDMA_CHANNEL_STENCIL2:
        {
            if (VPSHAL_VPDMA_CHANDT_STENCIL == dataType)
            {
                str = gVpsVpdmaHalStenDataTypeStr;
            }
        }
        break;

        case VPSHAL_VPDMA_CHANNEL_CLUT0:
        case VPSHAL_VPDMA_CHANNEL_CLUT1:
        case VPSHAL_VPDMA_CHANNEL_CLUT2:
        {
            if (VPSHAL_VPDMA_CHANDT_CLUT == dataType)
            {
                str = gVpsVpdmaHalClutDataTypeStr;
            }
        }
        break;

        default:
        {
            if (((channel >= VPSHAL_VPDMA_CHANNEL_VIP0_MULT_ANCA_SRC0) &&
                    (channel <= VPSHAL_VPDMA_CHANNEL_VIP0_MULT_ANCB_SRC15)) ||
                ((channel >= VPSHAL_VPDMA_CHANNEL_VIP1_MULT_ANCA_SRC0) &&
                    (channel <= VPSHAL_VPDMA_CHANNEL_VIP1_MULT_ANCB_SRC15)))
            {
                if (VPSHAL_VPDMA_CHANDT_ANC == dataType)
                {
                    str = gVpsVpdmaHalAncDataTypeStr;
                }
            }
            else if (dataType <= VPSHAL_VPDMA_CHANDT_BGRA8888)
            {
                str = gVpsVpdmaHalDataTypeStr[dataType];
            }
            else if ((dataType >= VPSHAL_VPDMA_CHANDT_BITMAP8) &&
                (dataType <= VPSHAL_VPDMA_CHANDT_BITMAP1_OFFSET7_BGRA32))
            {
                str = gVpsVpdmaHalBmpDataTypeStr
                        [dataType - VPSHAL_VPDMA_CHANDT_BITMAP8];
            }
        }
        break;
    }

    return (str);
}



/**
 *  VpsHal_vpdmaGetChStr
 *  \brief Return the string for the VPDMA channel.
 */
char *VpsHal_vpdmaGetChStr(UInt32 channel)
{
    char       *str = gVpsVpdmaHalInvalidStr;

    if (channel < VPSHAL_VPDMA_CHANNEL_NUM_CHANNELS)
    {
        str = gVpsVpdmaHalChStr[channel];
    }
    else if (channel <= VPSHAL_VPDMA_MAX_FREE_CHANNEL)
    {
        str = gVpsVpdmaHalFreeChStr;
    }

    return (str);
}



/**
 *  VpsHal_vpdmaGetFidStr
 *  \brief Return the string for the VPDMA FID.
 */
static char *VpsHal_vpdmaGetFidStr(UInt32 fid)
{
    char       *str = gVpsVpdmaHalInvalidStr;

    if (fid <= FVID2_FID_BOTTOM)
    {
        str = gVpsVpdmaHalFidStr[fid];
    }

    return (str);
}



/**
 *  VpsHal_vpdmaGetMemTypeStr
 *  \brief Return the string for the VPDMA memory type.
 */
static char *VpsHal_vpdmaGetMemTypeStr(UInt32 memType)
{
    char       *str = gVpsVpdmaHalInvalidStr;

    if (memType <= VPSHAL_VPDMA_MT_TILEDMEM)
    {
        str = gVpsVpdmaHalMemTypeStr[memType];
    }

    return (str);
}



/**
 *  VpsHal_vpdmaGetClassStr
 *  \brief Return the string for the VPDMA configuration class.
 */
static char *VpsHal_vpdmaGetClassStr(UInt32 class)
{
    char       *str = gVpsVpdmaHalInvalidStr;

    if (class <= VPSHAL_VPDMA_CPT_BLOCK_SET)
    {
        str = gVpsVpdmaHalClassStr[class];
    }

    return (str);
}



/**
 *  VpsHal_vpdmaGetDestStr
 *  \brief Return the string for the VPDMA configuration destination.
 */
static char *VpsHal_vpdmaGetDestStr(UInt32 destination)
{
    char       *str = gVpsVpdmaHalInvalidStr;

    if (destination <= VPSHAL_VPDMA_CONFIG_DEST_SC4)
    {
        str = gVpsVpdmaHalDestStr[destination];
    }

    return (str);
}



/**
 *  VpsHal_vpdmaGetSocEvtStr
 *  \brief Return the string for the VPDMA SOC event.
 */
static char *VpsHal_vpdmaGetSocEvtStr(UInt32 socEvt)
{
    char       *str = gVpsVpdmaHalInvalidStr;

    if (socEvt <= VPSHAL_VPDMA_SOC_EOEL)
    {
        str = gVpsVpdmaHalSocEvtStr[socEvt];
    }

    return (str);
}



/**
 *  VpsHal_vpdmaGetLmFidCtrlStr
 *  \brief Return the string for the VPDMA LM FID control.
 */
static char *VpsHal_vpdmaGetLmFidCtrlStr(UInt32 lmFidCtrl)
{
    char       *str = gVpsVpdmaHalInvalidStr;

    if (lmFidCtrl <= VPSHAL_VPDMA_LM_FID_CHANGE_1)
    {
        str = gVpsVpdmaHalLmFidCtrlStr[lmFidCtrl];
    }

    return (str);
}



/**
 *  VpsHal_vpdmaGetFsEventStr
 *  \brief Return the string for the VPDMA Frame Start Event.
 */
char *VpsHal_vpdmaGetFsEventStr(UInt32 fsEvent)
{
    char       *str = gVpsVpdmaHalInvalidStr;

    if (fsEvent <= VPSHAL_VPDMA_FSEVENT_CHANNEL_ACTIVE)
    {
        str = gVpsVpdmaFsEventStr[fsEvent];
    }

    return (str);
}



/**
 *  VpsHal_vpdmaIsValidAddr
 *  \brief Checks the address for validity and returns TRUE if valid else
 *  returns FALSE.
 */
static UInt32 VpsHal_vpdmaIsValidAddr(UInt32 addr, UInt32 memType)
{
    UInt32      isValidAddr = FALSE;

    if (VPSHAL_VPDMA_MT_NONTILEDMEM == memType)
    {
#ifdef TI_8107_BUILD
        if (((addr >= VPDMA_DEBUG_DDR0_ADDR_START) &&
                (addr < VPDMA_DEBUG_DDR0_ADDR_END)) ||
            ((addr >= VPDMA_DEBUG_DDR1_ADDR_START) &&
                (addr < VPDMA_DEBUG_DDR1_ADDR_END)) ||
            ((addr >= VPDMA_DEBUG_OCMC0_ADDR_START) &&
                (addr < VPDMA_DEBUG_OCMC0_ADDR_END)))
#else
        if (((addr >= VPDMA_DEBUG_DDR0_ADDR_START) &&
                (addr < VPDMA_DEBUG_DDR0_ADDR_END)) ||
            ((addr >= VPDMA_DEBUG_DDR1_ADDR_START) &&
                (addr < VPDMA_DEBUG_DDR1_ADDR_END)) ||
            ((addr >= VPDMA_DEBUG_OCMC0_ADDR_START) &&
                (addr < VPDMA_DEBUG_OCMC0_ADDR_END)) ||
            ((addr >= VPDMA_DEBUG_OCMC1_ADDR_START) &&
                (addr < VPDMA_DEBUG_OCMC1_ADDR_END)))
#endif
        {
            isValidAddr = TRUE;
        }
    }
    else
    {
        isValidAddr = TRUE;
    }

    return (isValidAddr);
}
