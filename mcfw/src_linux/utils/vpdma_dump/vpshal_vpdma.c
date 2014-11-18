/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
 *  \file vpshal_vpdma.c
 *
 *  \brief VPS VPDMA HAL Source file.
 *  This file implements the HAL APIs of the VPS VPDMA.
 *
 */

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */

#include <stdio.h>
#include <string.h>

#include "vps_types.h"
#include "trace.h"
#include "cslr_hd_vps_vpdma.h"
#include "vpshal_vpdma.h"
#include "vpshalVpdma.h"

char *VpsHal_vpdmaGetChStr(UInt32 channel);
char *VpsHal_vpdmaGetFsEventStr(UInt32 fsEvent);


/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */

#define VPSHAL_VPDMA_BUSY_TIMEOUT                      (500000u)

/**
 *  Macro to check whether VPDMA is busy reading the list and whether new list
 *  can be submitted to the VPDMA
 */
#define VPSHAL_VPDMA_ISBUSY              ((VpdmaBaseAddress->LIST_ATTR &       \
                               CSL_HD_VPS_VPDMA_LIST_ATTR_RDY_MASK) >>   \
                               CSL_HD_VPS_VPDMA_LIST_ATTR_RDY_SHIFT)


/* Macro to check if VPDMA firmware is loaded or not  */
#define VPSHAL_VPDMA_FW_IS_LOADED   (VpdmaBaseAddress->PID & 0x80)

/**
 * Macro to check whether given list is busy or not
 */
#define VPSHAL_VPDMA_ISLISTBUSY(listNum) ((VpdmaBaseAddress->LIST_STAT_SYNC >> \
                                     (VPSHAL_VPDMA_LIST_ATTR_LISTBUSYOFFSET +  \
                                     (listNum))) &  0x1)

#define VPSHAL_VPDMA_PAYLOAD_SIZE_ALIGN                  (16u)
#define VPSHAL_CONFIG_PAYLOAD_ADDR_ALIGN                 (16u)

#define VPSHAL_VPDMA_CONFIG_DATASIZE_MASK                (0x0000FFFFu)
#define VPSHAL_VPDMA_OUTBOUND_DESC_WRITE_ADDR_SHIFT      (0x5u)

#define VPSHAL_VPDMA_VPI_SIZE_ALIGN                      (0x4u)

#define VPSHAL_VPDMA_ADDR_SET_ALIGN                      (4u)
#define VPSHAL_VPDMA_REG_OFFSET_DIFF                     (4u)
#define VPSHAL_VPDMA_ADDR_SET_SIZE                       (4u)
#define VPSHAL_VPDMA_WORD_SIZE                           (4u)
#define VPSHAL_VPDMA_MIN_REG_SET_SIZE                    (4u)

#define VPSHAL_VPDMA_MAX_DATA_TYPE                       (4u)
#define VPSHAL_VPDMA_CONFIG_PAYLOADSIZE_SHIFT            (4u)

#define VPSHAL_VPDMA_GRPX_REGION_ATTR_MASK               (0x11Fu)

#define VPSHAL_VPDMA_MAX_TRACE_COUNT                     (100u)

/* Maximum number of register in a block in MMR configured */
#define VPSHAL_VPDMA_CONFIG_MMR_MAX_BLOCK_REG            (28u)

/* VPDMA firmware size in bytes. */
#define VPDMA_FIRWARE_SIZE          (8u * 1024u)

/* VPDMA number Control of control descriptor to be used for
   cleaning out channels */
#define VPSHAL_VPDMA_MAX_CTRL_DESC   (VPSHAL_VPDMA_CHANNEL_NUM_CHANNELS)

#define VPSHAL_VPDMA_BUSY_WAIT       (5000u)

/* ========================================================================== */
/*                         Structure Declarations                             */
/* ========================================================================== */

/**
 *  struct Vpdma_ChannelInfo
 *  \brief This structure keeps track of the channel information like channel
 *  number, direction of the channel, data type supported on the channel etc.
 *  When client of the VPDMA asks to create data descriptor in the memory, VPDMA
 *  HAL will fill up the memory with these default information
 *
 */
typedef struct
{
    VpsHal_VpdmaChannel         channelNum;
    /**< VPDMA Channel Number */
    UInt32                      clientCtrlReg;
    /**< Register into which frame start event for channel can be set */
    VpsHal_VpdmaMemoryType      memType;
    /**< Type of the memory from which data can be taken for this channel
         ---- Can we assume all 422p paths will uPossible value for this fields
         are 0 for 1D and 1 for 2D memory se non-tiled memory ---- */
    UInt8                       assigned;
    /**< Flag to indicate whether channel is free or not. This flag will
         be used for free channel only */
} Vpdma_ChannelInfo;


/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */

/* None */

/* ========================================================================== */
/*                            Global Variables                                */
/* ========================================================================== */

static Vpdma_ChannelInfo Channels[VPSHAL_VPDMA_CHANNEL_NUM_CHANNELS] =
                                        {VPSHAL_VPDMA_DEFAULT_CHANNEL_INFO};

CSL_VpsVpdmaRegsOvly VpdmaBaseAddress;

/**
 *  \brief Storing VPS Base address locally within VPDMA. This address will be
 *  subtracted from all the registers programmed in the overlay memory. This is
 *  done because overlay requires memory to starting from the offset/base
 *  address zero.
 */
UInt32 gVpsHal_vpsBaseAddress;


/* ========================================================================== */
/*                          Function Definitions                              */
/* ========================================================================== */

UInt32 VpsHal_vpdmaInitDebug(UInt32 baseAddr)
{
    /* Set VPDMA base address since FVID2_init is not called from application */
    VpdmaBaseAddress = (CSL_VpsVpdmaRegsOvly) baseAddr;
    gVpsHal_vpsBaseAddress = baseAddr - 0x0000D000u;

    return (VPS_SOK);
}

/**
 *  VpsHal_vpdmaPostList
 *  \brief Function for posting the list to the VPDMA. Once posted, VPDMA will
 *  start reading and processing the list. It is interrupt protected so
 *  can be called from the interrupt context also.
 *
 *  \param listNum          List Number
 *  \param listType         List Type i.e. Normal or Self Modifying
 *  \param listAddr         Physical address of the contiguous memory
 *                          containing list
 *  \param listSize         List size in bytes
 *  \param enableCheck      Flag to indicate whether parameter check needs to
 *                          be done or not.
 *  \return                 Returns 0 on success else returns error value
 */
Int VpsHal_vpdmaPostList(UInt8 listNum,
                         VpsHal_VpdmaListType listType,
                         Ptr listAddr,
                         UInt32 listSize,
                         UInt32 enableCheck)
{
    UInt32                  time = 0u;
    UInt32                  listAttr = 0u;
    UInt32                  lt = (UInt32) listType;
    Int                     ret = 0;
    CSL_VpsVpdmaRegsOvly    regOvly;


    if (TRUE == enableCheck)
    {
        GT_assert(VpsHalTrace, (NULL != listAddr));
        GT_assert(VpsHalTrace, (0u == ((UInt32)listAddr &
                      (VPSHAL_VPDMA_LIST_ADDR_ALIGN - 1u))));
        GT_assert(VpsHalTrace, (0u == ((UInt32)listSize &
                      (VPSHAL_VPDMA_LIST_SIZE_ALIGN - 1u))));
        GT_assert(VpsHalTrace, (listNum < VPSHAL_VPDMA_MAX_LIST));
    }

    listAddr = (Ptr)VpsHal_vpdmaVirtToPhy(listAddr);
    regOvly = VpdmaBaseAddress;

    /* Number of 16 byte word in the new list of descriptors */
    listSize = (listSize & (~(VPSHAL_VPDMA_LIST_SIZE_ALIGN - 1u))) >>
                    VPSHAL_VPDMA_LIST_SIZE_SHIFT;

    while ((0u == (VPSHAL_VPDMA_ISBUSY)) && (time < VPSHAL_VPDMA_BUSY_TIMEOUT))
    {
        /* ----- Delay for some time ----- */
        time += 1u;
    }

    if(0u == (VPSHAL_VPDMA_ISBUSY))
    {
        ret = -1;
    }

    if(0 == ret)
    {
        time = 0u;
        while ((0u != (VPSHAL_VPDMA_ISLISTBUSY(listNum))) &&
               (time < VPSHAL_VPDMA_BUSY_TIMEOUT))
        {
            /* ----- Delay for some time ----- */
            time += 1u;
        }

        if(0u != (VPSHAL_VPDMA_ISLISTBUSY(listNum)))
        {
            ret = -1;
        }

        if(0 == ret)
        {
            listAttr = (((UInt32)listNum <<
                         CSL_HD_VPS_VPDMA_LIST_ATTR_LIST_NUM_SHIFT) &
                         CSL_HD_VPS_VPDMA_LIST_ATTR_LIST_NUM_MASK);
            listAttr |= ((lt <<
                          CSL_HD_VPS_VPDMA_LIST_ATTR_LIST_TYPE_SHIFT) &
                          CSL_HD_VPS_VPDMA_LIST_ATTR_LIST_TYPE_MASK);
            listAttr |= ((listSize <<
                          CSL_HD_VPS_VPDMA_LIST_ATTR_LIST_SIZE_SHIFT) &
                          CSL_HD_VPS_VPDMA_LIST_ATTR_LIST_SIZE_MASK);


            /* Set the List Address */
            regOvly->LIST_ADDR = (UInt32)listAddr;

#ifdef VPSHAL_VPDMA_DO_PID_REG_CHECK /* defined in vpshalVpdmaFirmware_vNNN.h */

            /* Post the list only when other list is not getting cleared */
            time = 0u;
            do
            {
                /* Wait for some time if other list is getting
                   cleared by checking bit 15th of the PID register */
                if ((regOvly->PID & (1 << 15)) == 0)
                {
                    break;
                }

                for (cnt = 0u; cnt < VPSHAL_VPDMA_BUSY_WAIT; cnt ++);

                time ++;
            } while (time < VPSHAL_VPDMA_BUSY_TIMEOUT);
#endif

            /* Set the List Attributes */
            regOvly->LIST_ATTR = listAttr;
        }
    }

    return (ret);
}


/**
 *  VpsHal_vpdmaStopList
 *  \brief Function to stop the self modiyfing list. Self modifying list is a
 *  free running list. It is like a circular list which runs on its own.
 *  This function is used to stop self modifying list. When stop bit is set,
 *  it completes the current transfer and stops the list.
 *
 *  \param listNum          List to be stopped
 *  \param listType         NONE
 *  \return                 None
 */
Void VpsHal_vpdmaStopList(UInt8 listNum, VpsHal_VpdmaListType listType)
{
    UInt32 value;

    value = (((UInt32)listNum <<
             CSL_HD_VPS_VPDMA_LIST_ATTR_LIST_NUM_SHIFT) &
             CSL_HD_VPS_VPDMA_LIST_ATTR_LIST_NUM_MASK) |
             (((UInt32)listType <<
             CSL_HD_VPS_VPDMA_LIST_ATTR_LIST_TYPE_SHIFT) &
             CSL_HD_VPS_VPDMA_LIST_ATTR_LIST_TYPE_MASK) |
             ((1u << CSL_HD_VPS_VPDMA_LIST_ATTR_STOP_SHIFT) &
             CSL_HD_VPS_VPDMA_LIST_ATTR_STOP_MASK);

    (VpdmaBaseAddress)->LIST_ATTR = value;
}

Void VpsHal_vpdmaPrintCStat(void)
{
    UInt32                  chNum, regVal, flag = FALSE, fsEvent;
    volatile UInt32        *tempPtr;
    Vpdma_ChannelInfo      *chInfo;

    GT_0trace(VpsHalVpdmaDebugTrace, GT_INFO, " \n");
    GT_0trace(VpsHalVpdmaDebugTrace, GT_INFO,
        "CStat Register Dump (Client Busy):\n");
    GT_0trace(VpsHalVpdmaDebugTrace, GT_INFO,
        "---------------------------------\n");

    for (chNum = 0u; chNum < VPSHAL_VPDMA_CHANNEL_NUM_CHANNELS; chNum++)
    {
        chInfo = &Channels[chNum];

        if ((VPSHAL_VPDMA_CHANNEL_INVALID == chInfo->channelNum) ||
            (0u == chInfo->clientCtrlReg))
        {
            continue;
        }

        tempPtr = (volatile UInt32 *)
            ((UInt32)VpdmaBaseAddress + (UInt32) chInfo->clientCtrlReg);
        regVal = *tempPtr;
        if ((regVal & 0x0000C000u) != 0u)
        {
            GT_3trace(VpsHalVpdmaDebugTrace, GT_INFO,
                "CSTAT (0x%0.8X): 0x%0.8X (%s)\n",
                tempPtr, regVal, VpsHal_vpdmaGetChStr(chInfo->channelNum));
            flag = TRUE;
        }
    }

    if (!flag)
    {
        GT_0trace(VpsHalVpdmaDebugTrace, GT_INFO,
            "None of the clients are busy!!!\n");
    }

    GT_0trace(VpsHalVpdmaDebugTrace, GT_INFO, " \n");
    GT_0trace(VpsHalVpdmaDebugTrace, GT_INFO,
        "CStat Register Dump (Frame Start):\n");
    GT_0trace(VpsHalVpdmaDebugTrace, GT_INFO,
        "----------------------------------\n");

    for (chNum = 0u; chNum < VPSHAL_VPDMA_CHANNEL_NUM_CHANNELS; chNum++)
    {
        chInfo = &Channels[chNum];

        if ((VPSHAL_VPDMA_CHANNEL_INVALID == chInfo->channelNum) ||
            (0u == chInfo->clientCtrlReg))
        {
            continue;
        }

        tempPtr = (volatile UInt32 *)
            ((UInt32)VpdmaBaseAddress + (UInt32) chInfo->clientCtrlReg);
        regVal = *tempPtr;

        fsEvent = (regVal & CSL_HD_VPS_VPDMA_PRI_CHROMA_CSTAT_FRAME_START_MASK);
        fsEvent >>= CSL_HD_VPS_VPDMA_PRI_CHROMA_CSTAT_FRAME_START_SHIFT;

        GT_4trace(VpsHalVpdmaDebugTrace, GT_INFO,
            "%-22s (0x%0.8X): %-10s (0x%0.8X)\n",
            VpsHal_vpdmaGetChStr(chInfo->channelNum), tempPtr,
            VpsHal_vpdmaGetFsEventStr(fsEvent), regVal);
    }

    return;
}
