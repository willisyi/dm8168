/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
 * \file vpshal_vpdma.h
 *
 * \brief VPS VPDMA HAL header file
 * This file exposes the HAL APIs of the VPS VPDMA.
 *
 */

#ifndef _VPSHAL_VPDMA_H_
#define _VPSHAL_VPDMA_H_

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */

#include "vps_types.h"
#include "cslr_hd_vps_vpdma.h"
#include "cslr_vps_comp.h"

#ifdef TI_816X_BUILD
#include "vpshalVpdmaTI816x.h"
#endif

#ifdef TI_814X_BUILD
#include "vpshalVpdmaTI814x.h"
#endif

#ifdef TI_8107_BUILD
#include "vpshalVpdmaTI813x.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */

extern UInt32 CSL_TI81XX_VPS_BASE ;
extern UInt32 CSL_VPS_VPDMA_0_REGS;
extern UInt32 CSL_VPS_COMP_0_REGS ;


/** Descriptor Sizes */
#define VPSHAL_VPDMA_DATA_DESC_SIZE            (32u)
#define VPSHAL_VPDMA_CTRL_DESC_SIZE            (16u)
#define VPSHAL_VPDMA_CONFIG_DESC_SIZE          (16u)

/** Descriptor Alignments */
#define VPSHAL_VPDMA_DESC_BYTE_ALIGN           (16u)
#define VPSHAL_VPDMA_CONFIG_PAYLOAD_BYTE_ALIGN (16u)
#define VPSHAL_VPDMA_LIST_SIZE_SHIFT           (0x4u)
/**
 *  List size is always in terms of 128 bit words so it is aligned on 16
 *  bytes boundary
 */
#define VPSHAL_VPDMA_LIST_SIZE_ALIGN           (16u)
#define VPSHAL_VPDMA_LIST_ADDR_ALIGN           (16u)
#define VPSHAL_VPDMA_LINE_STRIDE_ALIGN         (16u)
#define VPSHAL_VPDMA_BUF_ADDR_ALIGN            (16u)
#define VPSHAL_VPDMA_WR_DESC_BUF_ADDR_ALIGN    (32u)

/* VPDMA Macros */
#define VPSHAL_VPDMA_MAX_LIST                  (8u)
#define VPSHAL_VPDMA_MAX_CLIENTS               (63u)
#define VPSHAL_VPDMA_MAX_SI_SOURCE             (16u)
#define VPSHAL_VPDMA_MAX_LM_FID                (3u)

#define VPSHAL_VPDMA_MIN_FREE_CHANNEL          (193u)
/* TODO: VPDMA free channel should start from 192 channel number */
#define VPSHAL_VPDMA_MAX_FREE_CHANNEL          (255u)
#define VPSHAL_VPDMA_NUM_FREE_CHANNELS      (VPSHAL_VPDMA_MAX_FREE_CHANNEL -   \
                                             VPSHAL_VPDMA_MIN_FREE_CHANNEL + 1)

/** Type of the Descriptors */
#define VPSHAL_VPDMA_PT_DATA                   (0xAu)
#define VPSHAL_VPDMA_PT_CONFIG                 (0xBu)
#define VPSHAL_VPDMA_PT_CONTROL                (0xCu)

#define VPSHAL_VPDMA_LIST_ATTR_LISTBUSYOFFSET  (16u)
#define VPSHAL_VPDMA_PAYLOADSIZEALIGN          (16u)

/** Masks for addresses for input and output buffers */
#define VPSHAL_VPDMA_DATADESC_INBUFFERMASK     (0xFFFFFFFCu)
#define VPSHAL_VPDMA_DATADESC_INBUFFEROFFSET   (2u)
#define VPSHAL_VPDMA_DATADESC_OUTBUFFERMASK    (0xFFFFFFE0u)
#define VPSHAL_VPDMA_DATADESC_OUTBUFFEROFFSET  (5u)

/* Max frame size in data descriptor */
#define VPSHAL_VPDMA_MAX_FRAME_WIDTH            (4096u)
#define VPSHAL_VPDMA_MAX_FRAME_HEIGHT           (2048u)

/* Minumum width is suggested by hardware team. This
 * is not the actual hardware restriction, but combination
 * of hardware paths like SC_H, DEI_H etc
 */
#define VPSHAL_VPDMA_MIN_FRAME_WIDTH            (24u)
#define VPSHAL_VPDMA_MIN_FRAME_HEIGHT           (8u)

#define VPSHAL_VPDMA_INBOUND_DATA_DESC          (0x0u)
#define VPSHAL_VPDMA_OUTBOUND_DATA_DESC         (0x1u)

#define VPSHAL_VPDMA_MAX_SKIP_DESC              (7u)

/** Graphics Flags Used in the graphics region datadescriptor to enable
 * specific feature */
#define VPSHAL_VPDMA_GRPX_START_REGION         (0x00000080u)
#define VPSHAL_VPDMA_GRPX_END_REGION           (0x00000100u)
#define VPSHAL_VPDMA_GRPX_SCALAR               (0x00000200u)
#define VPSHAL_VPDMA_GRPX_ANTI_FLICKER         (0x00000400u)
#define VPSHAL_VPDMA_GRPX_STENCIL              (0x00000800u)
#define VPSHAL_VPDMA_GRPX_BOUNDRY_BOX          (0x00008000u)
#define VPSHAL_VPDMA_GRPX_TRANSPARENCY         (0x00010000u)

/* Maximum client number used in reading/writing configuration through
 * VPI bus */
#define VPSHAL_VPDMA_MAX_CLIENT_NUM            (127u)


#define VPSHAL_VPDMA_LM_REG_START       (0x8000u)
#define VPSHAL_VPDMA_LM_REG_CNT         (0x100u)


//#define VPSHAL_VPDMA_ENABLE_VIRT_TO_PHY_MAP

#define VPSHAL_VPDMA_OCMC_PHYS_BASE            (0x40000000)

static inline UInt32 VpsHal_vpdmaVirtToPhy(Ptr virt)
{
    #ifdef VPSHAL_VPDMA_ENABLE_VIRT_TO_PHY_MAP
    if ( virt && (((UInt32)virt & 0xF0000000) == 0x0))
    {
        return ((UInt32)virt + VPSHAL_VPDMA_OCMC_PHYS_BASE);
    }
    #endif

    return (UInt32)(virt);
}

static inline UInt32 VpsHal_vpdmaPhyToVirt(Ptr phys)
{
    #ifdef VPSHAL_VPDMA_ENABLE_VIRT_TO_PHY_MAP
    if ( (((UInt32)phys & 0xF0000000) == VPSHAL_VPDMA_OCMC_PHYS_BASE))
    {
        return ((UInt32)phys & 0x0FFFFFFF);
    }
    #endif

    return (UInt32)(phys);
}
/**
 *  \brief Enumeration for VPDMA firmware versions
 */
typedef enum
{
    VPSHAL_VPDMA_VER_195 = 0,
    VPSHAL_VPDMA_VER_19B,
    VPSHAL_VPDMA_VER_19F,
    VPSHAL_VPDMA_VER_1A0,
    VPSHAL_VPDMA_VER_1A1,
    VPSHAL_VPDMA_VER_1A3,
    VPSHAL_VPDMA_VER_1A4,
    VPSHAL_VPDMA_VER_1A5,
    VPSHAL_VPDMA_VER_1A6,
    VPSHAL_VPDMA_VER_1AD,
    VPSHAL_VPDMA_VER_1AE,
    VPSHAL_VPDMA_VER_1B0,
    VPSHAL_VPDMA_VER_1B2,
    VPSHAL_VPDMA_VER_1B5,
    VPSHAL_VPDMA_VER_1B6,
    VPSHAL_VPDMA_VER_1B7,
    VPSHAL_VPDMA_VER_1XX_MAX = 100,
    VPSHAL_VPDMA_VER_286 = VPSHAL_VPDMA_VER_1XX_MAX + 1,
    VPSHAL_VPDMA_VER_287,
    VPSHAL_VPDMA_VER_288,
    VPSHAL_VPDMA_VER_28A,
    VPSHAL_VPDMA_VER_28C,
    VPSHAL_VPDMA_VER_28D,
    VPSHAL_VPDMA_VER_290,
    VPSHAL_VPDMA_VER_MAX

}VpsHal_VpdmaVersion;

/**
 *  \brief Enums for data types that VPDMA channel can
 *  accept. This enum can be used directly to set the data type in data
 *  descriptor
 */
typedef enum
{
    VPSHAL_VPDMA_CHANDT_RGB565 = 0,
    /**< RGB  565 */
    VPSHAL_VPDMA_CHANDT_ARGB1555 = 1,
    /**< ARGB 1555 */
    VPSHAL_VPDMA_CHANDT_ARGB4444 = 2,
    /**< ARGB 4444 */
    VPSHAL_VPDMA_CHANDT_RGBA5551 = 3,
    /**< RGBA 5551 */
    VPSHAL_VPDMA_CHANDT_RGBA4444 = 4,
    /**< RGBA 4444 */
    VPSHAL_VPDMA_CHANDT_ARGB6666 = 5,
    /**< ARGB 6666 */
    VPSHAL_VPDMA_CHANDT_RGB888 = 6,
    /**< RGB  888 */
    VPSHAL_VPDMA_CHANDT_ARGB8888 = 7,
    /**< ARGB 8888 */
    VPSHAL_VPDMA_CHANDT_RGBA6666 = 8,
    /**< RGBA 6666 */
    VPSHAL_VPDMA_CHANDT_RGBA8888 = 9,
    /**< RGBA 8888 */
    /*new for PG2.0*/
    VPSHAL_VPDMA_CHANDT_BGR565 = 0x10,
    /**< BGR 565 */
    VPSHAL_VPDMA_CHANDT_ABGR1555 = 0x11,
    /**< ABGR 1555 */
    VPSHAL_VPDMA_CHANDT_ABGR4444 = 0x12,
    /**< ABGR 4444 */
    VPSHAL_VPDMA_CHANDT_BGRA5551 = 0x13,
    /**< BGRA 1555 */
    VPSHAL_VPDMA_CHANDT_BGRA4444 = 0x14,
    /**< BGRA 4444 */
    VPSHAL_VPDMA_CHANDT_ABGR6666 = 0x15,
    /**< ABGR 6666 */
    VPSHAL_VPDMA_CHANDT_BGR888 = 0x16,
    /**< BGR 888 */
    VPSHAL_VPDMA_CHANDT_ABGR8888 = 0x17,
    /**< ABGR 8888 */
    VPSHAL_VPDMA_CHANDT_BGRA6666 = 0x18,
    /**< BGRA 6666 */
    VPSHAL_VPDMA_CHANDT_BGRA8888 = 0x19,
    /**< BGRA 8888 */
    VPSHAL_VPDMA_CHANDT_BITMAP8 = 0x20,
    /**< 8 bit clut */
    VPSHAL_VPDMA_CHANDT_BITMAP4_LOWER = 0x22,
    /**< 4 bit clut with lower address */
    VPSHAL_VPDMA_CHANDT_BITMAP4_UPPER = 0x23,
    /**< 4 bit clut with upper address */
    VPSHAL_VPDMA_CHANDT_BITMAP2_OFFSET0 = 0x24,
    /**< 2 bit clut with offset0 */
    VPSHAL_VPDMA_CHANDT_BITMAP2_OFFSET1 = 0x25,
    /**< 2 bit clut with offset1 */
    VPSHAL_VPDMA_CHANDT_BITMAP2_OFFSET2 = 0x26,
    /**< 2 bit clut with offset2 */
    VPSHAL_VPDMA_CHANDT_BITMAP2_OFFSET3 = 0x27,
    /**< 2 bit clut with offset3 */
    VPSHAL_VPDMA_CHANDT_BITMAP1_OFFSET0 = 0x28,
    /**< 1 bit clut with offset0 */
    VPSHAL_VPDMA_CHANDT_BITMAP1_OFFSET1 = 0x29,
    /**< 1 bit clut with offset1 */
    VPSHAL_VPDMA_CHANDT_BITMAP1_OFFSET2 = 0x2A,
    /**< 1 bit clut with offset2 */
    VPSHAL_VPDMA_CHANDT_BITMAP1_OFFSET3 = 0x2B,
    /**< 1 bit clut with offset3 */
    VPSHAL_VPDMA_CHANDT_BITMAP1_OFFSET4 = 0x2C,
    /**< 1 bit clut with offset4 */
    VPSHAL_VPDMA_CHANDT_BITMAP1_OFFSET5 = 0x2D,
    /**< 1 bit clut with offset5 */
    VPSHAL_VPDMA_CHANDT_BITMAP1_OFFSET6 = 0x2E,
    /**< 1 bit clut with offset6 */
    VPSHAL_VPDMA_CHANDT_BITMAP1_OFFSET7 = 0x2F,
    /**< 1 bit clut with offset7 */
    /*new for PG2.0*/
    VPSHAL_VPDMA_CHANDT_BITMAP8_BGRA32 = 0x30,
    /**< 8 bit clut with BGRA32 format */
    VPSHAL_VPDMA_CHANDT_BITMAP4_LOWER_BGRA32 = 0x32,
    /**< 4 bit clut lower address with BGRA32 format */
    VPSHAL_VPDMA_CHANDT_BITMAP4_UPPER_BGRA32 = 0x33,
    /**< 4 bit clut UPPER address with BGRA32 format */
    VPSHAL_VPDMA_CHANDT_BITMAP2_OFFSET0_BGRA32 = 0x34,
    /**< 2 bit clut with offset0 BGRA32 format*/
    VPSHAL_VPDMA_CHANDT_BITMAP2_OFFSET1_BGRA32 = 0x35,
    /**< 2 bit clut with offset1 BGRA32 format*/
    VPSHAL_VPDMA_CHANDT_BITMAP2_OFFSET2_BGRA32 = 0x36,
    /**< 2 bit clut with offset2 BGRA32 format*/
    VPSHAL_VPDMA_CHANDT_BITMAP2_OFFSET3_BGRA32 = 0x37,
    /**< 2 bit clut with offset3 BGRA32 format*/
    VPSHAL_VPDMA_CHANDT_BITMAP1_OFFSET0_BGRA32 = 0x38,
    /**< 1 bit clut with offset0 BGRA32 format*/
    VPSHAL_VPDMA_CHANDT_BITMAP1_OFFSET1_BGRA32 = 0x39,
    /**< 1 bit clut with offset1 BGRA32 format*/
    VPSHAL_VPDMA_CHANDT_BITMAP1_OFFSET2_BGRA32 = 0x3A,
    /**< 1 bit clut with offset2 BGRA32 format*/
    VPSHAL_VPDMA_CHANDT_BITMAP1_OFFSET3_BGRA32 = 0x3B,
    /**< 1 bit clut with offset3 BGRA32 format*/
    VPSHAL_VPDMA_CHANDT_BITMAP1_OFFSET4_BGRA32 = 0x3C,
    /**< 1 bit clut with offset4 BGRA32 format*/
    VPSHAL_VPDMA_CHANDT_BITMAP1_OFFSET5_BGRA32 = 0x3D,
    /**< 1 bit clut with offset5 BGRA32 format*/
    VPSHAL_VPDMA_CHANDT_BITMAP1_OFFSET6_BGRA32 = 0x3E,
    /**< 1 bit clut with offset6 BGRA32 format*/
    VPSHAL_VPDMA_CHANDT_BITMAP1_OFFSET7_BGRA32 = 0x3F,
    /**< 1 bit clut with offset7 BGRA32 format*/
    VPSHAL_VPDMA_CHANDT_Y444 = 0,
    /**< Y 4:4:4 */
    VPSHAL_VPDMA_CHANDT_Y422 = 1,
    /**< Y 4:2:2 */
    VPSHAL_VPDMA_CHANDT_Y420 = 2,
    /**< Y 4:2:0 */
    VPSHAL_VPDMA_CHANDT_YC420 = 3,
    /**< YC 4:2:0 Y on LSB, C on MSB */
    VPSHAL_VPDMA_CHANDT_C444 = 4,
    /**< C 4:4:4 */
    VPSHAL_VPDMA_CHANDT_C422 = 5,
    /**< C 4:2:2 */
    VPSHAL_VPDMA_CHANDT_C420 = 6,
    /**< C 4:2:0 */
    VPSHAL_VPDMA_CHANDT_YC422 = 7,
    /**< YC 4:2:2 Y on LSB, C on MSB i.e. YUYV */
    VPSHAL_VPDMA_CHANDT_YC444 = 8,
    /**< YC 4:4:4 Y on LSB, C on MSB */
    VPSHAL_VPDMA_CHANDT_MV = 3,
    /**< Motion Vector and MVSTM are 4-bit data. Program one less. */
    VPSHAL_VPDMA_CHANDT_STENCIL = 0,
    /**< Stencil Data is 1-bit. Program one less. */
    VPSHAL_VPDMA_CHANDT_CLUT = 7,
    /**< CLUT Table is 8-bit. Program one less. */
    VPSHAL_VPDMA_CHANDT_ANC = 8,
    /**< Ancillary Data is 8-bit. Program one less. */
    VPSHAL_VPDMA_CHANDT_YCb422 = 0x17,
    /**< YC 4:2:2 Y on LSB, C on MSB i.e. YVYU */
    VPSHAL_VPDMA_CHANDT_CY422 = 0x27,
    /**< CY 4:2:2 C on LSB, Y on MSB i.e. UYVY */
    VPSHAL_VPDMA_CHANDT_CbY422 = 0x37,
    /**< CY 4:2:2 C on LSB, Y on MSB i.e. VYUY */
    VPSHAL_VPDMA_CHANDT_INVALID = 0xFF
    /**< Invalid Data Type */
} VpsHal_VpdmaChanDT;

/**
 *  \brief This defines the configuration destinations.
 *  This enum can be used directly to set the destination field of
 *  configuration descriptor
 */
typedef enum
{
    VPSHAL_VPDMA_CONFIG_DEST_MMR = 0,
    /**< Destination is MMR client */
    VPSHAL_VPDMA_CONFIG_DEST_SC_GRPX0,
    /**< Graphics 0 scalar */
    VPSHAL_VPDMA_CONFIG_DEST_SC_GRPX1,
    /**< Graphics 1 scalar */
    VPSHAL_VPDMA_CONFIG_DEST_SC_GRPX2,
    /**< Graphics 2 scalar */
    VPSHAL_VPDMA_CONFIG_DEST_SC1,
    /**< Primary path scalar */
    VPSHAL_VPDMA_CONFIG_DEST_SC2,
    /**< Aux path scalar */
    VPSHAL_VPDMA_CONFIG_DEST_SC5,
    /**< Write back 2 scalar */
    VPSHAL_VPDMA_CONFIG_DEST_SC3,
    /**< Video Input Port 0 scalar */
    VPSHAL_VPDMA_CONFIG_DEST_SC4
    /**< Video Input Port 1 scalar */
} VpsHal_VpdmaConfigDest;

/**
 *  \brief This defines the type of control descriptor.
 *  This can be directly used in descriptor to the set the control
 *  descriptor type */
typedef enum
{
    VPSHAL_VPDMA_CDT_SOC = 0,
    /**< Sync on Client */
    VPSHAL_VPDMA_CDT_SOL,
    /**< Sync on List */
    VPSHAL_VPDMA_CDT_SOR,
    /**< Sync on Register */
    VPSHAL_VPDMA_CDT_SOT,
    /**< Sync on LM Timer */
    VPSHAL_VPDMA_CDT_SOCH,
    /**< Sync on Channel */
    VPSHAL_VPDMA_CDT_INTR_CHANGE,
    /**< Client interrupt change */
    VPSHAL_VPDMA_CDT_SI,
    /**< Send Interrupt */
    VPSHAL_VPDMA_CDT_RL,
    /**< Reload List */
    VPSHAL_VPDMA_CDT_ABT_CHANNEL,
    /**< Abort Channel */
    VPSHAL_VPDMA_CDT_TOGGLE_FID
    /**< Toggle LM Fid */
} VpsHal_VpdmaCtrlDescType;

/**
 *  \brief This defines the sync events for the sync on client
 *  control descriptor
 */
typedef enum
{
    VPSHAL_VPDMA_SOC_EOF = 0,
    /**< End of Frame */
    VPSHAL_VPDMA_SOC_SOF,
    /**< Start of Frame */
    VPSHAL_VPDMA_SOC_EOL,
    /**< End of Line X */
    VPSHAL_VPDMA_SOC_SOL,
    /**< Start of Line X */
    VPSHAL_VPDMA_SOC_PIXEL,
    /**< Matching exact pixel */
    VPSHAL_VPDMA_SOC_ACTIVE,
    /**< Client is Active */
    VPSHAL_VPDMA_SOC_NOTACTIVE,
    /**< Client is not active */
    VPSHAL_VPDMA_SOC_FID_CHANGE01,
    /**< Field Change from 0 to 1 */
    VPSHAL_VPDMA_SOC_FID_CHANGE10,
    /**< Field Change from 1 to 0 */
    VPSHAL_VPDMA_SOC_EOEL
    /**< End of Every Line X */
} VpsHal_VpdmaSocEvent;

/**
 *  \brief Enum for type of the list supported by VPDMA
 */
typedef enum
{
    VPSHAL_VPDMA_LT_NORMAL = 0,
    /**< Normal List */
    VPSHAL_VPDMA_LT_SELFMODIFYING,
    /**< Self Modifying List */
    VPSHAL_VPDMA_LT_DEBUG = 7
    /**< List type for VPDMA debugging */
} VpsHal_VpdmaListType;

/**
 *  \brief Enum for type of memory
 */
typedef enum
{
    VPSHAL_VPDMA_MT_NONTILEDMEM = 0,
    /**< Non-Tiled Memory. */
    VPSHAL_VPDMA_MT_TILEDMEM
    /**< Tiled Memory. */
} VpsHal_VpdmaMemoryType;

/**
 *  \brief Enum for frame start event. This tells VPDMA
 *  which NF signal to use for a client. This has to be configured on the
 *  channel associated with client.
 */
typedef enum
{
    VPSHAL_VPDMA_FSEVENT_HDMI_FID = 0,
    /**< Change in value of hdmi_field_id */
    VPSHAL_VPDMA_FSEVENT_DVO2_FID,
    /**< Change in value of dvo2_field_id */
    VPSHAL_VPDMA_FSEVENT_HDCOMP_FID,
    /**< Change in value of hdcomp_field_id */
    VPSHAL_VPDMA_FSEVENT_SD_FID,
    /**< Change in value of sd_field_id */
    VPSHAL_VPDMA_FSEVENT_LM_FID0,
    /**< Use List Manager Internal Field - 0 */
    VPSHAL_VPDMA_FSEVENT_LM_FID1,
    /**< Use List Manager Internal Field - 1 */
    VPSHAL_VPDMA_FSEVENT_LM_FID2,
    /**< Use List Manager Internal Field - 2*/
    VPSHAL_VPDMA_FSEVENT_CHANNEL_ACTIVE
    /**< Start on channel active */
} VpsHal_VpdmaFSEvent;

/**
 *  Enum for number of line skip supported by VPDMA
 */
typedef enum
{
    VPSHAL_VPDMA_LS_1 = 0,
    /**< Skip 1 line */
    VPSHAL_VPDMA_LS_2
    /**< Skip 2 lines */
} VpsHal_VpdmaLineSkip;

/**
 *  VpsHal_VpdmaClient
 *  \brief Enum for the client used by the VPDMA
 */
typedef enum
{
    VPSHAL_VPDMA_CLIENT_INVALID = -1,
    /**< Invalid value, used as boundary */
    VPSHAL_VPDMA_CLIENT_DEI_HQ_1_CHROMA =   0,
    /**< DEI HQ Chroma port Field 1 */
    VPSHAL_VPDMA_CLIENT_DEI_HQ_1_LUMA,
    /**< DEI HQ Luma port Field 1 */
    VPSHAL_VPDMA_CLIENT_DEI_HQ_2_LUMA,
    /**< DEI HQ Luma port Field 2 */
    VPSHAL_VPDMA_CLIENT_DEI_HQ_2_CHROMA,
    /**< DEI HQ Chroma port Field 2 */
    VPSHAL_VPDMA_CLIENT_DEI_HQ_3_LUMA,
    /**< DEI HQ Luma port Field 3 */
    VPSHAL_VPDMA_CLIENT_DEI_HQ_3_CHROMA,
    /**< DEI HQ Chroma port Field 3 */

    VPSHAL_VPDMA_CLIENT_DEI_HQ_MV_IN    =   12,
    /**< DEI HQ MV Input port */

    VPSHAL_VPDMA_CLIENT_DEI_HQ_MV_OUT   =   15,
    /**< DEI HQ MV output port */

    VPSHAL_VPDMA_CLIENT_DEI_SC_OUT      =   17,
    /**< DEI SC output port */
    VPSHAL_VPDMA_CLIENT_PIP_WRBK,
    /**< PIP write back output port */
    VPSHAL_VPDMA_CLIENT_SC_IN_CHROMA,
    /**< SC Chroma input port */
    VPSHAL_VPDMA_CLIENT_SC_IN_LUMA,
    /**< SC Luma input port */

    VPSHAL_VPDMA_CLIENT_SC_OUT          =   29,
    /**< SC output port */
    VPSHAL_VPDMA_CLIENT_COMP_WRBK,
    /**< Compositor write back output port */
    VPSHAL_VPDMA_CLIENT_GRPX0,
    /**< Graphics instance 1 input port */
    VPSHAL_VPDMA_CLIENT_GRPX1,
    /**< Graphics instance 2 input port */
    VPSHAL_VPDMA_CLIENT_GRPX3,
    /**< Graphics instance 3 input port */
    VPSHAL_VPDMA_CLIENT_VIP0_LO_Y,
    /**< VIP instance 1 Luma Low output port */
    VPSHAL_VPDMA_CLIENT_VIP0_LO_UV,
    /**< VIP instance 1 Chroma Low output port */
    VPSHAL_VPDMA_CLIENT_VIP0_HI_Y,
    /**< VIP instance 1 Luma High output port */
    VPSHAL_VPDMA_CLIENT_VIP0_HI_UV,
    /**< VIP instance 1 Chroma High output port */
    VPSHAL_VPDMA_CLIENT_VIP1_LO_Y,
    /**< VIP instance 2 Luma Low output port */
    VPSHAL_VPDMA_CLIENT_VIP1_LO_UV,
    /**< VIP instance 2 Chroma Low output port */
    VPSHAL_VPDMA_CLIENT_VIP1_HI_Y,
    /**< VIP instance 2 Luma High output port */
    VPSHAL_VPDMA_CLIENT_VIP1_HI_UV,
    /**< VIP instance 2 Chroma High output port */
    VPSHAL_VPDMA_CLIENT_GRPX0_ST,
    /**< Graphics Instance 1 stencil input port */
    VPSHAL_VPDMA_CLIENT_GRPX1_ST,
    /**< Graphics Instance 2 stencil input port */
    VPSHAL_VPDMA_CLIENT_GRPX3_ST,
    /**< Graphics Instance 3 stencil input port */
    VPSHAL_VPDMA_CLIENT_NF_422_IN,
    /**< NF Input port */
    VPSHAL_VPDMA_CLIENT_NF_420_Y_IN,
    /**< NF YUV420 Luma Input port */
    VPSHAL_VPDMA_CLIENT_NF_420_UV_IN,
    /**< NF YUV420 Chroma Input port */
    VPSHAL_VPDMA_CLIENT_NF_420_Y_OUT,
    /**< NF YUV420 Luma Output port */
    VPSHAL_VPDMA_CLIENT_NF_420_UV_OUT,
    /**< NF YUV420 Chroma output port */

    VPSHAL_VPDMA_CLIENT_VBI_SDVENC      = 51,
    /**< SDVENC VBI output port */
    VPSHAL_VPDMA_CLIENT_VBI_CTL,
    /**< SDVENC VBI control output port */
    VPSHAL_VPDMA_CLIENT_HDMI_WRBK_OUT,
    /**< HDMI Writeback output port */
    VPSHAL_VPDMA_CLIENT_TRANS1_CHROMA,
    VPSHAL_VPDMA_CLIENT_TRANS1_LUMA,
    VPSHAL_VPDMA_CLIENT_TRANS2_CHROMA,
    VPSHAL_VPDMA_CLIENT_TRANS2_LUMA,
    VPSHAL_VPDMA_CLIENT_VIP0_ANC_A,
    /**< VIP0 Ancilary A output port */
    VPSHAL_VPDMA_CLIENT_VIP0_ANC_B,
    /**< VIP0 Ancilary B output port */
    VPSHAL_VPDMA_CLIENT_VIP1_ANC_A,
    /**< VIP1 Ancilary A output port */
    VPSHAL_VPDMA_CLIENT_VIP1_ANC_B,
    /**< VIP1 Ancilary B output port */
    VPSHAL_VPDMA_CLIENT_MAX
    /**< Enumeration Guard */
}VpsHal_VpdmaClient;

typedef enum
{
    VPSHAL_VPDMA_LM_0 = 0,
    /**< repeat lines twice each output data line gets 2 times the number
         of frame lines */
    VPSHAL_VPDMA_LM_1,
    /**< each line once with Line Buffer Disabled, so no mirroring. Each
         line gets frame lines with identical data */
    VPSHAL_VPDMA_LM_2,
    /**< Each line seen once Mirroring is enabled so the top lines get
         the top lines repeated at the top of the frame and the bottom
         lines have the bottom lines repeated. Each line of data gets
         frame lines + number of buffered lines */
    VPSHAL_VPDMA_LM_3
    /**< Each line once only on one line. Each data line gets number of
         frame lines divided by number of buffered lines */
} VpsHal_VpdmaLineMode;

/**
 *  \brief Enum for specifying transparency mask for graphics region
 */
typedef enum
{
    VPSHAL_VPDMA_GTM_NOMASKING = 0,
    /**< No Masking */
    VPSHAL_VPDMA_GTM_MASK1BIT,
    /**< Mask 0 bit */
    VPSHAL_VPDMA_GTM_MASK2BIT,
    /**< Mask [1:0] bits */
    VPSHAL_VPDMA_GTM_MASK3BIT
    /**< Mask [2:0] bits */
} VpsHal_VpdmaGrpxTransMask;

/**
 *  Enum for specifying blending type for graphics region
 */
typedef enum
{
    VPSHAL_VPDMA_GBT_NOBLENDING = 0,
    /**< No Blending */
    VPSHAL_VPDMA_GBT_GLOBALBLENDING,
    /**< Global Blending */
    VPSHAL_VPDMA_GBT_CLUTBLENDING,
    /**< Clut Blending */
    VPSHAL_VPDMA_GBT_PIXELBLENDING
    /**< Pixel Alpha based blending */
} VpsHal_VpdmaGrpxBlendType;

/**
 *  Enum for specifying configuration command type i.e. Direct or
 *  indirect
 */
typedef enum
{
    VPSHAL_VPDMA_CCT_INDIRECT = 0,
    /**< Payload is stored somewhere in memory */
    VPSHAL_VPDMA_CCT_DIRECT
    /**< Payload is contiguous with config desc */
} VpsHal_VpdmaConfigCmdType;

/**
 *  \brief Enum for specifying configuration descriptor payload type
 *  i.e. Address set or Block set
 */
typedef enum VpsHal_VpdmaConfigPayloadType_T
{
    VPSHAL_VPDMA_CPT_ADDR_DATA_SET = 0,
    /**< Payload consists of Blocks of addresses */
    VPSHAL_VPDMA_CPT_BLOCK_SET
    /**< Payload consists of simple a block of data */
} VpsHal_VpdmaConfigPayloadType;

typedef enum
{
    VPSHAL_VPDMA_OUTBOUND_MAX_WIDTH_UNLIMITED = 0,
    /**< Unlimited line size */
    VPSHAL_VPDMA_OUTBOUND_MAX_WIDTH_352_PIXELS = 4,
    /**< Maximum 352 Pixels per line */
    VPSHAL_VPDMA_OUTBOUND_MAX_WIDTH_768_PIXELS = 5,
    /**< Maximum 768 Pixels per line */
    VPSHAL_VPDMA_OUTBOUND_MAX_WIDTH_1280_PIXELS = 6,
    /**< Maximum 1280 Pixels per line */
    VPSHAL_VPDMA_OUTBOUND_MAX_WIDTH_1920_PIXELS = 7
    /**< Maximum 1920 Pixels per line */
} VpsHal_VpdmaOutBoundMaxWidth;

typedef enum
{
    VPSHAL_VPDMA_OUTBOUND_MAX_HEIGHT_UNLIMITED = 0,
    /**< Unlimited frame size */
    VPSHAL_VPDMA_OUTBOUND_MAX_HEIGHT_288_LINES = 4,
    /**< Maximum 288 lines per frame */
    VPSHAL_VPDMA_OUTBOUND_MAX_HEIGHT_576_LINES = 5,
    /**< Maximum 576 lines per frame */
    VPSHAL_VPDMA_OUTBOUND_MAX_HEIGHT_720_LINES = 6,
    /**< Maximum 720 lines per frame */
    VPSHAL_VPDMA_OUTBOUND_MAX_HEIGHT_1080_LINES = 7
    /**< Maximum 1080 lines per frame */
} VpsHal_VpdmaOutBoundMaxHeight;

/**
 *  \brief Enum for OCP bus priority in the data descriptor
 */
typedef enum
{
    VPSHAL_VPDMA_DATADESCPRIO_0 = 0,
    /**< Highest Priority */
    VPSHAL_VPDMA_DATADESCPRIO_1,
    VPSHAL_VPDMA_DATADESCPRIO_2,
    VPSHAL_VPDMA_DATADESCPRIO_3,
    VPSHAL_VPDMA_DATADESCPRIO_4,
    VPSHAL_VPDMA_DATADESCPRIO_5,
    VPSHAL_VPDMA_DATADESCPRIO_6,
    VPSHAL_VPDMA_DATADESCPRIO_7
    /**< Lowest Priority */
} VpsHal_VpdmaDataDescPrio;

/**
 * \brief VpsHal_VpdmaLmFidCtrl
 *  Enum defines possible value for LM FID control descriptor
 */
typedef enum
{
    VPSHAL_VPDMA_LM_FID_UNCHANGED = 0,
    /**< FID remains unchanged */
    VPSHAL_VPDMA_LM_FID_TOGGLE,
    /**< FID Toggles */
    VPSHAL_VPDMA_LM_FID_CHANGE_0,
    /**< FID becomse 0 */
    VPSHAL_VPDMA_LM_FID_CHANGE_1
    /**< FID becomse 1 */
} VpsHal_VpdmaLmFidCtrl;

/**
 * \brief VpsHal_VpdmaPath
 *  Enum defining VPDMA path
 */
typedef enum
{
    VPSHAL_VPDMA_PATH_VIP0_LO_UV = 0,
    VPSHAL_VPDMA_PATH_VIP0_LO_Y,
    VPSHAL_VPDMA_PATH_VIP0_UP_UV,
    VPSHAL_VPDMA_PATH_VIP0_UP_Y,
    VPSHAL_VPDMA_PATH_VIP1_LO_UV,
    VPSHAL_VPDMA_PATH_VIP1_LO_Y,
    VPSHAL_VPDMA_PATH_VIP1_UP_UV,
    VPSHAL_VPDMA_PATH_VIP1_UP_Y,
    VPSHAL_VPDMA_PATH_VIP0_ANCA,
    VPSHAL_VPDMA_PATH_VIP0_ANCB,
    VPSHAL_VPDMA_PATH_VIP1_ANCA,
    VPSHAL_VPDMA_PATH_VIP1_ANCB,
    VPSHAL_VPDMA_PATH_SEC0,
    VPSHAL_VPDMA_PATH_SEC1,
    VPSHAL_VPDMA_PATH_PRI,
    VPSHAL_VPDMA_PATH_WB0,
    VPSHAL_VPDMA_PATH_AUX,
    VPSHAL_VPDMA_PATH_WB1,
    VPSHAL_VPDMA_PATH_BP0,
    VPSHAL_VPDMA_PATH_BP1,
    VPSHAL_VPDMA_PATH_WB2,
    VPSHAL_VPDMA_PATH_GRPX0,
    VPSHAL_VPDMA_PATH_GRPX0_STENC,
    VPSHAL_VPDMA_PATH_GRPX1,
    VPSHAL_VPDMA_PATH_GRPX1_STENC,
    VPSHAL_VPDMA_PATH_GRPX2,
    VPSHAL_VPDMA_PATH_GRPX2_STENC,
    VPSHAL_VPDMA_PATH_VBI_HD,
    VPSHAL_VPDMA_PATH_VBI_SD,
    VPSHAL_VPDMA_PATH_NF,
    VPSHAL_VPDMA_MAX_PATH
} VpsHal_VpdmaPath;



/* ========================================================================== */
/*                         Structure Declarations                             */
/* ========================================================================== */

/**
 *  \brief VPDMA InBound Data Descriptor. Used for display to submit the
 *  data descriptor to VPDMA
 */
typedef struct
{
    UInt32 lineStride:16;
    UInt32 oddSkip:3;
    UInt32 reserved1:1;
    UInt32 evenSkip:3;
    UInt32 oneD:1;
    UInt32 fieldId:1;
    UInt32 notify:1;
    UInt32 dataType:6;
    UInt32 transferHeight:16;
    /**< Maximum transfer height is 2048 */
    UInt32 transferWidth:16;
    /**< Maximum transfer width is 4096 */
    UInt32 address:32;
    UInt32 nextChannel:9;
    UInt32 priority:3;
    UInt32 reserved2:2;
    UInt32 mosaicMode:1;
    UInt32 reserved3:1;
    UInt32 channel:9;
    UInt32 direction:1;
    UInt32 memType:1;
    UInt32 descType:5;
    UInt32 frameHeight:16;
    /**< Maximum transfer height is 2048 */
    UInt32 frameWidth:16;
    /**< Maximum transfer width is 4096 */
    UInt32 verticalStart:16;
    UInt32 horizontalStart:16;
    UInt32 clientSpecific1:32;
    UInt32 clientSpecific2:32;
} VpsHal_VpdmaInDataDesc;

/**
 *  \brief VPDMA OutBound Data Descriptor. Used for capture to submit the
 *  data descriptor to VPDMA. It tells VPDMA whether to drop the data,
 *  whether to write back the data descriptor for captured buffers.
 */
typedef struct
{
    UInt32 lineStride:16;
    UInt32 oddSkip:3;
    UInt32 reserved1:1;
    UInt32 evenSkip:3;
    UInt32 oneD:1;
    UInt32 reserved2:1;
    UInt32 notify:1;
    UInt32 dataType:6;
    UInt32 reserved3:32;
    UInt32 address:32;
    UInt32 nextChannel:9;
    UInt32 priority:3;
    UInt32 reserved4:3;
    UInt32 descSkip:1;
    UInt32 channel:9;
    UInt32 direction:1;
    UInt32 memType:1;
    UInt32 descType:5;
    UInt32 useDescReg:1;
    UInt32 dropData:1;
    UInt32 writeDesc:1;
    UInt32 reserved5:2;
    UInt32 outDescAddress:27;
    UInt32 maxHeight:3;
    /**< The Maximum allowable lines per frame */
    UInt32 reserved6:1;
    UInt32 maxWidth:3;
    /**< The Maximum allowable pixels per line */
    UInt32 reserved7:25;
    UInt32 clientSpecific1:32;
    UInt32 clientSpecific2:32;
} VpsHal_VpdmaOutDataDesc;

/**
 * Non bit-field based data descriptor overlay for efficient access to
 * descriptor in memory
*/
typedef struct
{
  UInt32 dataInfo;
  UInt32 reserved;
  UInt32 startAddr;
  UInt32 channelInfo;
  UInt32 descWriteInfo;
  UInt32 maxWidthHeight;
  UInt32 clientSpecific[2];
} VpsHal_VpdmaOutDataDescOverlay;

/**
 * Non bit-field based data descriptor overlay for efficient access
 * to descriptor in memory
 */
typedef struct
{
  UInt32 dataInfo;
  UInt32 transferWidthHeight;
  UInt32 startAddr;
  UInt32 channelInfo;
  UInt32 frameWidthHeight;
  UInt32 horzVertStart;
  UInt32 clientSpecific[2];
} VpsHal_VpdmaInDataDescOverlay;

/**
 *  \brief VPDMA Data Descriptor for the region of the graphics frame. Used
 *  for graphics driver to submit region attributes along with the
 *  region buffer to the attributes.
 */
typedef struct
{
    UInt32 lineStride:16;
    UInt32 oddSkip:3;
    UInt32 reserved1:1;
    UInt32 evenSkip:3;
    UInt32 reserved2:1;
    UInt32 fieldId:1;
    UInt32 notify:1;
    UInt32 dataType:6;
    UInt32 transferHeight:16;
    UInt32 transferWidth:16;
    UInt32 address:32;
    UInt32 nextChannel:9;
    UInt32 priority:3;
    UInt32 defaultSize:4;
    UInt32 channel:9;
    UInt32 direction:1;
    UInt32 memType:1;
    UInt32 descType:5;
    UInt32 regionHeight:16;
    UInt32 regionWidth:16;
    UInt32 verticalStart:16;
    UInt32 horizontalStart:16;
    UInt32 regionPriority:4;
    UInt32 reserved3:3;
    UInt32 regionAttr:9;
    UInt32 bbAlpha:8;
    UInt32 blendAlpha:8;
    UInt32 blendType:2;
    UInt32 reserved5:3;
    UInt32 enableTransparency:1;
    UInt32 transMask:2;
    UInt32 transColor:24;
} VpsHal_VpdmaRegionDataDesc;

/**
 *  \brief VPDMA Configuration Descriptor. Used for sending configuration to
 *  the modules. This configuration can be used to setting MMR
 *  registers or scalar coefficients.
 */
typedef struct
{
    UInt32 destAddr:32;
    UInt32 dataLength:16;
    UInt32 reserved1:16;
    UInt32 payloadAddress:32;
    UInt32 payloadLength:16;
    UInt32 destination:8;
    UInt32 class:2;
    UInt32 direct:1;
    UInt32 descType:5;
} VpsHal_VpdmaConfigDesc;

/**
 *  \brief VPDMA address data sub block header in configuration overlay memory.
 */
typedef struct
{
    UInt32 nextClientAddr:32;
    UInt32 subBlockLength:16;
    UInt32 reserved1:16;
    UInt32 reserved2:32;
    UInt32 reserved3:32;
} VpsHal_VpdmaSubBlockHeader;

/**
 *  \brief VPDMA configuration descriptor for the graphics frame. For the
 *  graphics, frame configuration is specified by configuration
 *  descriptor and region configuration is specifed by data
 *  descriptor. Scalar coefficients are also specified along with the frame
 *  configuration.
 */
typedef struct
{
    UInt32 payloadAddress:32;
    UInt32 address:24;
    UInt32 length:8;
    UInt32 payloadLength:16;
    UInt32 destination:8;
    UInt32 class:2;
    UInt32 direct:1;
    UInt32 descType:5;
} VpsHal_VpdmaGrpxFrameConfigDesc;

/**
 *  \brief Sync On Client control descriptor. Used for changing interrupt
 *  generation event and then waiting for that event to occur.
 */
typedef struct
{
    UInt32 reserved1:32;
    UInt32 lineCount:16;
    UInt32 pixelCount:16;
    UInt32 event:4;
    UInt32 reserved2:28;
    UInt32 ctrl:4;
    UInt32 reserved3:12;
    UInt32 channel:9;
    UInt32 reserved4:2;
    UInt32 descType:5;
} VpsHal_VpdmaSyncOnClientDesc;

/**
 *  \brief Sync On List control descriptor. Used to ensure that multiple lists
 *  have all reached a common point. Need to specify the bit-mask of
 *  all list to wait for.
 */
typedef struct
{
    UInt32 reserved1:32;
    UInt32 reserved2:32;
    UInt32 reserved3:32;
    UInt32 ctrl:4;
    UInt32 reserved4:12;
    UInt32 lists:9;
    UInt32 reserved5:2;
    UInt32 descType:5;
} VpsHal_VpdmaSyncOnListDesc;

/**
 *  \brief Sync On Register Control Descriptor. Used to wait for write to the
 *  LIST_STAT_SYNC register to the field for the list that
 *  the control descriptor is in.
 */
typedef struct
{
    UInt32 reserved1:32;
    UInt32 reserved2:32;
    UInt32 reserved3:32;
    UInt32 ctrl:4;
    UInt32 reserved4:12;
    UInt32 listNum:9;
    UInt32 reserved5:2;
    UInt32 descType:5;
} VpsHal_VpdmaSyncOnRegDesc;

/**
 *  \brief Sync on List Manager Timer Control Descriptor. Used to wait for
 *  number of cycles to elapse from the current time position.
 */
typedef struct
{
    UInt32 numCycles:16;
    UInt32 reserved1:16;
    UInt32 reserved2:32;
    UInt32 reserved3:32;
    UInt32 ctrl:4;
    UInt32 reserved4:23;
    UInt32 descType:5;
} VpsHal_VpdmaSyncOnLmTimerDesc;

/**
 *  \brief Sync on Channel Control Descriptor. Used to wait for the specified
 *  channel to become free.
 */
typedef struct
{
    UInt32 reserved1:32;
    UInt32 reserved2:32;
    UInt32 reserved3:32;
    UInt32 ctrl:4;
    UInt32 reserved4:12;
    UInt32 channel:9;
    UInt32 reserved5:2;
    UInt32 descType:5;
} VpsHal_VpdmaSyncOnChannelDesc;

/**
 * \brief  Send Interrupt Control Descriptor. It causes the VPDMA to generate
 *  an interrupt on the list manager controlled interrupts as specified
 *  by the Source Field.
 */
typedef struct
{
    UInt32 reserved1:32;
    UInt32 reserved2:32;
    UInt32 reserved3:32;
    UInt32 ctrl:4;
    UInt32 reserved4:12;
    UInt32 source:9;
    UInt32 reserved5:2;
    UInt32 descType:5;
} VpsHal_VpdmaSendIntrDesc;

/**
 *  \brief Reload Control Descriptor. It causes descriptors after this
 *  descriptor in the original list to be dropped and a new list at
 *  the location and of the size specified in the descriptor.
 */
typedef struct
{
    UInt32 reloadAddr:32;
    UInt32 listSize:16;
    UInt32 reserved1:16;
    UInt32 reserved2:32;
    UInt32 ctrl:4;
    UInt32 reserved3:23;
    UInt32 descType:5;
} VpsHal_VpdmaReloadDesc;

/**
 *  \brief Abort Control Descriptor. Used to clear channel from issuing any
 *  more request. Any outstanding requests for that channel
 *  will complete as originally scheduled.
 */
typedef struct
{
    UInt32 reserved1:32;
    UInt32 reserved2:32;
    UInt32 reserved3:32;
    UInt32 ctrl:4;
    UInt32 reserved4:12;
    UInt32 channel:9;
    UInt32 reserved5:2;
    UInt32 descType:5;
} VpsHal_VpdmaAbortDesc;

/**
 *  \brief Toggle LM FID control descriptor. used if the clients set their
 *  frame source to LM FID. The read clients will start transmitting
 *  data upon the FID signal inside the LM changing value. This
 *  descriptor will cause the LM to toggle the value of the internal FID.
 */
typedef struct
{
    UInt32 reserved1:32;
    UInt32 reserved2:32;
    UInt32 lmFidCtrl0:2;
    UInt32 lmFidCtrl1:2;
    UInt32 lmFidCtrl2:2;
    UInt32 reserved3:26;
    UInt32 ctrl:4;
    UInt32 reserved4:23;
    UInt32 descType:5;
} VpsHal_VpdmaToggleLmFidDesc;

/**
 * \brief Change Client Interrupt Source control descriptor.
 *        Expected to be used to configure the VPDMA to generate interrupt after
 *        specified number of lines or specified number of pixels. If required
 *        to get an interrupt after specified number of lines, update lineCount
 *        and set pixelCount to 0x0
 */
typedef struct
{
    UInt32 reserved1:32;
    UInt32 lineCount:16;
    UInt32 pixelCount:16;
    UInt32 event:4;
    UInt32 reserved2:28;
    UInt32 ctrl:4;
    UInt32 reserved3:12;
    UInt32 source:9;
    UInt32 reserved4:2;
    UInt32 descType:5;

}VpsHal_VpdmaChangeClientIntSrc;


/**
 *  struct VpsHal_VpdmaInstParams
 *  \brief Structure containing instance specific parameters used at the init.
 *
 */
typedef struct
{
    UInt32             vpdmaBaseAddress;
    /**< Base address of the VPDMA instance */
    UInt32             vpsBaseAddress;
    /**< Base address of the VPS */
} VpsHal_VpdmaInstParams;

/**
 *  struct VpsHal_VpdmaInDescParams
 *  \brief This structure is for creating
 *  inbound data descriptor. Upper layer passes pointer of this structure to
 *  the createInBoundDataDesc function to create the data data descriptor in
 *  the given memory.
 *
 */
typedef struct
{
    VpsHal_VpdmaChannel channel;
    /**< VPDMA Channel number for which data descriptor is to be created */
    VpsHal_VpdmaChanDT dataType;
    /**< Channel Data Type */
    UInt16 transWidth;
    /**< Width of the buffer. VPDMA provides feature using which it can read
         small buffer and provide larger frame to the downstream module by
         filing up the remaining buffer with the background color. Transfer size
         refers to the input buffer size and frame size refers to the target
         buffer provided to the downstream module.*/
    UInt16 transHeight;
    /**< Height of the buffer */
    UInt16 frameWidth;
    /**< Width of frame */
    UInt16 frameHeight;
    /**< Height of the frame */
    UInt16 startX;
    /**< Horizontal start position of the transfer window in frame window */
    UInt16 startY;
    /**< Vertical start position of the transfer window in frame window */
    UInt16 lineStride;
    /**< Line stride in bytes between two lines in transfer window */
    VpsHal_VpdmaLineSkip lineSkip;
    /**< Number of lines to skip after each line in transfer window. This along
         with the lineStride is used to calculate next line address */
    VpsHal_VpdmaChannel nextChannel;
    /**< For virtual video buffer or region base graphics, this parameter is
         used to the specify channel number of the next free channel */
    UInt32 notify;
    /**< Fires notify interrupt for this list at end of data transfer */
    VpsHal_VpdmaDataDescPrio priority;
    /**< Data descriptor priority */
    VpsHal_VpdmaMemoryType  memType;
    /**< Type of memory i.e. Tiled or Non-Tiled. */
    UInt32 is1DMem;
    /**< Memory type */
} VpsHal_VpdmaInDescParams;

/**
 *  struct VpsHal_VpdmaOutDescParams
 *  \brief This structure is for creating
 *  outbound data descriptor. Upper layer passes pointer of this structure to
 *  the createOutBoundDataDesc function to create the data data descriptor in
 *  the given memory.
 *
 */
typedef struct
{
    VpsHal_VpdmaChannel channel;
    /**< VPDMA Channel number for which data descriptor is to be created */
    VpsHal_VpdmaChanDT dataType;
    /**< Channel Data Type */
    UInt16 lineStride;
    /**< Line stride in bytes between two lines in transfer window */
    VpsHal_VpdmaLineSkip lineSkip;
    /**< Number of lines to skip after each line in transfer window.This along
         with the lineStride is used to calculate next line address */
    UInt32 notify;
    /**< Fires notify interrupt for this list at end of data transfer */
    VpsHal_VpdmaDataDescPrio priority;
    /**< Data descriptor priority */
    VpsHal_VpdmaMemoryType  memType;
    /**< Type of memory i.e. Tiled or Non-Tiled. */
    VpsHal_VpdmaChannel nextChannel;
    /**< Next channel to be programmed in the outbound descriptor */
    VpsHal_VpdmaOutBoundMaxWidth maxWidth;
    /**< The maximum number of pixes per line */
    VpsHal_VpdmaOutBoundMaxHeight maxHeight;
    /**< The maximum number of lines per frame */
    UInt32 is1DMem;
} VpsHal_VpdmaOutDescParams;

/**
 *  struct VpsHal_VpdmaRegionDataDescParams
 *  \brief This structure is for creating
 *  graphics region data descriptor. Upper layer passes pointer of this
 *  structure to the createRegionDataDesc function to create the data
 *  descriptor in the given memory. Parametes starting from channel to
 *  nextChannel in the below structure are same as the inbound data descriptor.
 *
 */
typedef struct
{
    VpsHal_VpdmaChannel channel;
    VpsHal_VpdmaChanDT dataType;
    UInt16 regionWidth;
    UInt16 regionHeight;
    UInt16 startX;
    UInt16 startY;
    UInt16 lineStride;
    VpsHal_VpdmaLineSkip lineSkip;
    VpsHal_VpdmaChannel nextChannel;
    UInt8  regionPriority;
    /**< Priority of the region */
    UInt32 regionAttr;
    /**< Region Attributes used to set the graphics specific features */
    UInt8  blendAlpha;
    /**< Alpha value for the blending */
    UInt8  bbAlpha;
    /**< Boundry Box alpha Value */
    UInt32 transColor;
    /**< Transparency color */
    VpsHal_VpdmaGrpxTransMask transMask;
    /**< Transparency Mask */
    VpsHal_VpdmaGrpxBlendType blendType;
    /**< Type of the blending to be used for this region */
    UInt32 notify;
    /**< Fires notify interrupt for this list at end of data transfer */
    VpsHal_VpdmaDataDescPrio priority;
    /**< Data descriptor priority */
    VpsHal_VpdmaMemoryType  memType;
    /**< Type of memory i.e. Tiled or Non-Tiled. */
} VpsHal_VpdmaRegionDataDescParams;


/* ========================================================================== */
/*                         Structure Declarations                             */
/* ========================================================================== */

/* None */

/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */

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
                         UInt32 enableCheck);

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
Void VpsHal_vpdmaStopList(UInt8 listNum, VpsHal_VpdmaListType listType);

/**
 *  VpsHal_vpdmaInitDebug
 *  \brief VPDMA Debug function initialization. This function should be called
 *  Before calling any debug functions.
 *
 *  \param baseAddr         VPDMA base address.
 *
 *  \return                 Returns 0 on success else returns error value.
 */
UInt32 VpsHal_vpdmaInitDebug(UInt32 baseAddr);

/**
 *  VpsHal_vpdmaGetListStatus
 *  \brief VPDMA Debug function - Reads and returns the current list status.
 *
 *  VpsHal_vpdmaInitDebug should be called before calling this function.
 *
 *  \param listNum          List number for which status to be returned.
 *
 *  \return                 List status.
 */
UInt32 VpsHal_vpdmaGetListStatus(UInt32 listNum);

/**
 *  VpsHal_vpdmaReadLmReg
 *  \brief VPDMA Debug function - Reads and returns the list manager
 *  internal register.
 *
 *  VpsHal_vpdmaInitDebug should be called before calling this function.
 *
 *  \param listNum          List number to be used to perform the read.
 *  \param regOffset        Register offset to read.
 *
 *  \return                 List manager register value.
 */
UInt32 VpsHal_vpdmaReadLmReg(UInt32 listNum, UInt32 regOffset);

/**
 *  VpsHal_vpdmaPrintList
 *  \brief VPDMA Debug function - Parses and prints the VPDMA descriptors
 *  till a reload is encountered or till the list size.
 *
 *  \param listAddr   [IN]  List address to start printing from.
 *  \param listSize   [IN]  List size in VPDMA words.
 *  \param rlListAddr [OUT] Reload list address for next parsing. A value of 0
 *                          means the list ends.
 *  \param rlListSize [OUT] Reload list size in VPDMA words.
 *  \param printLevel [IN]  0 - Prints only Errors and Warnings.
 *                          1 - Prints descriptor information in addition
 *                              to level 0.
 *                          2 - Prints register overlay in addition to level 1.
 */
Void VpsHal_vpdmaPrintList(UInt32 listAddr,
                           UInt32 listSize,
                           UInt32 *rlListAddr,
                           UInt32 *rlListSize,
                           UInt32 printLevel);

/**
 *  VpsHal_vpdmaParseRegOverlay
 *  \brief VPDMA Debug function - Parses the MMR config overlay memory
 *  and extract the address-data value.
 *
 *  \param memPtr        [IN]  Overlay memory pointer.
 *  \param payloadLength [IN]  Overlay memory size in VPDMA words.
 *  \param maxRegToParse [IN]  Represents the array size of below pointers
 *                             in words.
 *  \param numRegParsed  [OUT] Number of registers parsed. This paramter could
 *                             be NULL if no return is needed.
 *  \param regAddr       [OUT] Pointer to register address array where the
 *                             parsed register address is populated. This
 *                             paramter could be NULL if no return is needed.
 *  \param regVal        [OUT] Pointer to register value array where the
 *                             parsed register value is populated. This
 *                             paramter could be NULL if no return is needed.
 *  \param traceMask     [IN]  Trace mask for printing different level of debug
 *                             messages.
 *
 *  \return     Returns VPS_SOK if parsing is successful else returns error.
 */
UInt32 VpsHal_vpdmaParseRegOverlay(const Void *memPtr,
                                   UInt32 payloadLength,
                                   UInt32 maxRegToParse,
                                   UInt32 *numRegParsed,
                                   UInt32 *regAddr,
                                   UInt32 *regVal,
                                   UInt32 traceMask);

Void VpsHal_vpdmaPrintCStat(void);

Void VpsHal_vpdmaPrintInBoundDesc(const Void *memPtr, UInt32 traceMask);

#ifdef __cplusplus
}
#endif

#endif
