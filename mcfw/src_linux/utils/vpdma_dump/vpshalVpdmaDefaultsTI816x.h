/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
 * \file vpshalVpdmaDefaultsTI816x.h
 *
 * \brief VPS VPDMA HAL default configuration file
 * This file contains default configuration for VPDMA.
 *
 */

#ifndef _VPSSHALVPDMADEFAULTS_TI816X_H
#define _VPSSHALVPDMADEFAULTS_TI816X_H

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */

/* None */

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */

/**
 * Expert value for the anchor and interpolated pixel
 */
#define VPSHAL_VPDMA_DEFAULT_CHANNEL_INFO                                      \
    {VPSHAL_VPDMA_CHANNEL_PRI_LUMA,                                            \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->PRI_LUMA_CSTAT),               \
        VPSHAL_VPDMA_MT_TILEDMEM, 0},                                          \
    {VPSHAL_VPDMA_CHANNEL_PRI_CHROMA,                                          \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->PRI_CHROMA_CSTAT),             \
        VPSHAL_VPDMA_MT_TILEDMEM, 0},                                          \
    {VPSHAL_VPDMA_CHANNEL_PRI_FLD1_LUMA,                                       \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->PRI_FLD1_LUMA_CSTAT),          \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_PRI_FLD1_CHROMA,                                     \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->PRI_FLD1_CHROMA_CSTAT),        \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_PRI_FLD2_LUMA,                                       \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->PRI_FLD2_LUMA_CSTAT),          \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_PRI_FLD2_CHROMA,                                     \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->PRI_FLD2_CHROMA_CSTAT),        \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_PRI_FLD3_LUMA,                                       \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->PRI_FLD3_LUMA_CSTAT),          \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_PRI_FLD3_CHROMA,                                     \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->PRI_FLD3_CHROMA_CSTAT),        \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_PRI_FLD_WR_TOP_LUMA,                                 \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->PRI_FLD_WR_TOP_LUMA_CSTAT),    \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_PRI_FLD_WR_TOP_CHROMA,                               \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->PRI_FLD_WR_TOP_CHROMA_CSTAT),  \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_PRI_FLD_WR_BOT_LUMA,                                 \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->PRI_FLD_WR_BOT_LUMA_CSTAT),    \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_PRI_FLD_WR_BOT_CHROMA,                               \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->PRI_FLD_WR_BOT_CHROMA_CSTAT),  \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_PRI_MV0,                                             \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->PRI_MV0_CSTAT),                \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_PRI_MV1,                                             \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->PRI_MV1_CSTAT),                \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_PRI_MVSTM,                                           \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->PRI_MVSTM_CSTAT),              \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_PRI_MV_OUT,                                          \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->PRI_MV_OUT_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_PRI_MVSTM_OUT,                                       \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->PRI_MVSTM_OUT_CSTAT),          \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_WB0,                                                 \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->WB0_CSTAT),                    \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_AUX_LUMA,                                            \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->AUX_LUMA_CSTAT),               \
        VPSHAL_VPDMA_MT_TILEDMEM, 0},                                          \
    {VPSHAL_VPDMA_CHANNEL_AUX_CHROMA,                                          \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->AUX_CHROMA_CSTAT),             \
        VPSHAL_VPDMA_MT_TILEDMEM, 0},                                          \
    {VPSHAL_VPDMA_CHANNEL_AUX_FLD1_LUMA,                                       \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->AUX_FLD1_LUMA_CSTAT),          \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_AUX_FLD1_CHROMA,                                     \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->AUX_FLD1_CHROMA_CSTAT),        \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_AUX_FLD2_LUMA,                                       \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->AUX_FLD2_LUMA_CSTAT),          \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_AUX_FLD2_CHROMA,                                     \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->AUX_FLD2_CHROMA_CSTAT),        \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_AUX_FLD_WR_LUMA,                                     \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->AUX_FLD_WR_LUMA_CSTAT),        \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_AUX_FLD_WR_CHROMA,                                   \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->AUX_FLD_WR_CHROMA_CSTAT),      \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_AUX_MV,                                              \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->AUX_MV_CSTAT),                 \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_AUX_MV_OUT,                                          \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->AUX_MV_OUT_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_WB1,                                                 \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->WB1_CSTAT),                    \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_GRPX0,                                               \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->GRPX0_CSTAT),                  \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_GRPX1,                                               \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->GRPX1_CSTAT),                  \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_GRPX2,                                               \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->GRPX2_CSTAT),                  \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_STENCIL0,                                            \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->STENCIL0_CSTAT),               \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_STENCIL1,                                            \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->STENCIL1_CSTAT),               \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_STENCIL2,                                            \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->STENCIL2_CSTAT),               \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_CLUT0,                                               \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->GRPX0_CSTAT),                  \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_CLUT1,                                               \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->GRPX1_CSTAT),                  \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_CLUT2,                                               \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->GRPX2_CSTAT),                  \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP0_MULT_PORTA_SRC0,                                \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_LO_Y_CSTAT),              \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP0_MULT_PORTA_SRC1,                                \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_LO_Y_CSTAT),              \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP0_MULT_PORTA_SRC2,                                \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_LO_Y_CSTAT),              \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP0_MULT_PORTA_SRC3,                                \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_LO_Y_CSTAT),              \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP0_MULT_PORTA_SRC4,                                \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_LO_Y_CSTAT),              \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP0_MULT_PORTA_SRC5,                                \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_LO_Y_CSTAT),              \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP0_MULT_PORTA_SRC6,                                \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_LO_Y_CSTAT),              \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP0_MULT_PORTA_SRC7,                                \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_LO_Y_CSTAT),              \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP0_MULT_PORTA_SRC8,                                \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_LO_Y_CSTAT),              \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP0_MULT_PORTA_SRC9,                                \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_LO_Y_CSTAT),              \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP0_MULT_PORTA_SRC10,                               \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_LO_Y_CSTAT),              \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP0_MULT_PORTA_SRC11,                               \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_LO_Y_CSTAT),              \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP0_MULT_PORTA_SRC12,                               \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_LO_Y_CSTAT),              \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP0_MULT_PORTA_SRC13,                               \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_LO_Y_CSTAT),              \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP0_MULT_PORTA_SRC14,                               \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_LO_Y_CSTAT),              \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP0_MULT_PORTA_SRC15,                               \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_LO_Y_CSTAT),              \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP0_MULT_PORTB_SRC0,                                \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_LO_UV_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP0_MULT_PORTB_SRC1,                                \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_LO_UV_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP0_MULT_PORTB_SRC2,                                \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_LO_UV_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP0_MULT_PORTB_SRC3,                                \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_LO_UV_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP0_MULT_PORTB_SRC4,                                \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_LO_UV_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP0_MULT_PORTB_SRC5,                                \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_LO_UV_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP0_MULT_PORTB_SRC6,                                \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_LO_UV_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP0_MULT_PORTB_SRC7,                                \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_LO_UV_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP0_MULT_PORTB_SRC8,                                \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_LO_UV_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP0_MULT_PORTB_SRC9,                                \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_LO_UV_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP0_MULT_PORTB_SRC10,                               \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_LO_UV_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP0_MULT_PORTB_SRC11,                               \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_LO_UV_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP0_MULT_PORTB_SRC12,                               \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_LO_UV_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP0_MULT_PORTB_SRC13,                               \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_LO_UV_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP0_MULT_PORTB_SRC14,                               \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_LO_UV_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP0_MULT_PORTB_SRC15,                               \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_LO_UV_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP0_MULT_ANCA_SRC0,                                 \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_ANC_A_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP0_MULT_ANCA_SRC1,                                 \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_ANC_A_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP0_MULT_ANCA_SRC2,                                 \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_ANC_A_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP0_MULT_ANCA_SRC3,                                 \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_ANC_A_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP0_MULT_ANCA_SRC4,                                 \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_ANC_A_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP0_MULT_ANCA_SRC5,                                 \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_ANC_A_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP0_MULT_ANCA_SRC6,                                 \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_ANC_A_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP0_MULT_ANCA_SRC7,                                 \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_ANC_A_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP0_MULT_ANCA_SRC8,                                 \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_ANC_A_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP0_MULT_ANCA_SRC9,                                 \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_ANC_A_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP0_MULT_ANCA_SRC10,                                \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_ANC_A_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP0_MULT_ANCA_SRC11,                                \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_ANC_A_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP0_MULT_ANCA_SRC12,                                \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_ANC_A_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP0_MULT_ANCA_SRC13,                                \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_ANC_A_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP0_MULT_ANCA_SRC14,                                \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_ANC_A_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP0_MULT_ANCA_SRC15,                                \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_ANC_A_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP0_MULT_ANCB_SRC0,                                 \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_ANC_B_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP0_MULT_ANCB_SRC1,                                 \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_ANC_B_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP0_MULT_ANCB_SRC2,                                 \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_ANC_B_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP0_MULT_ANCB_SRC3,                                 \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_ANC_B_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP0_MULT_ANCB_SRC4,                                 \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_ANC_B_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP0_MULT_ANCB_SRC5,                                 \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_ANC_B_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP0_MULT_ANCB_SRC6,                                 \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_ANC_B_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP0_MULT_ANCB_SRC7,                                 \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_ANC_B_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP0_MULT_ANCB_SRC8,                                 \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_ANC_B_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP0_MULT_ANCB_SRC9,                                 \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_ANC_B_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP0_MULT_ANCB_SRC10,                                \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_ANC_B_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP0_MULT_ANCB_SRC11,                                \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_ANC_B_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP0_MULT_ANCB_SRC12,                                \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_ANC_B_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP0_MULT_ANCB_SRC13,                                \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_ANC_B_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP0_MULT_ANCB_SRC14,                                \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_ANC_B_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP0_MULT_ANCB_SRC15,                                \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_ANC_B_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP0_PORTA_LUMA,                                     \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_LO_Y_CSTAT),              \
        VPSHAL_VPDMA_MT_TILEDMEM, 0},                                          \
    {VPSHAL_VPDMA_CHANNEL_VIP0_PORTA_CHROMA,                                   \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_LO_UV_CSTAT),             \
        VPSHAL_VPDMA_MT_TILEDMEM, 0},                                          \
    {VPSHAL_VPDMA_CHANNEL_VIP0_PORTB_LUMA,                                     \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_UP_Y_CSTAT),              \
        VPSHAL_VPDMA_MT_TILEDMEM, 0},                                          \
    {VPSHAL_VPDMA_CHANNEL_VIP0_PORTB_CHROMA,                                   \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_UP_UV_CSTAT),             \
        VPSHAL_VPDMA_MT_TILEDMEM, 0},                                          \
    {VPSHAL_VPDMA_CHANNEL_VIP0_PORTA_RGB,                                      \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_LO_Y_CSTAT),              \
        VPSHAL_VPDMA_MT_TILEDMEM, 0},                                          \
    {VPSHAL_VPDMA_CHANNEL_VIP0_PORTB_RGB,                                      \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP0_UP_Y_CSTAT),              \
        VPSHAL_VPDMA_MT_TILEDMEM, 0},                                          \
    {VPSHAL_VPDMA_CHANNEL_VIP1_MULT_PORTA_SRC0,                                \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_LO_Y_CSTAT),              \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP1_MULT_PORTA_SRC1,                                \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_LO_Y_CSTAT),              \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP1_MULT_PORTA_SRC2,                                \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_LO_Y_CSTAT),              \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP1_MULT_PORTA_SRC3,                                \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_LO_Y_CSTAT),              \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP1_MULT_PORTA_SRC4,                                \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_LO_Y_CSTAT),              \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP1_MULT_PORTA_SRC5,                                \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_LO_Y_CSTAT),              \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP1_MULT_PORTA_SRC6,                                \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_LO_Y_CSTAT),              \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP1_MULT_PORTA_SRC7,                                \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_LO_Y_CSTAT),              \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP1_MULT_PORTA_SRC8,                                \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_LO_Y_CSTAT),              \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP1_MULT_PORTA_SRC9,                                \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_LO_Y_CSTAT),              \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP1_MULT_PORTA_SRC10,                               \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_LO_Y_CSTAT),              \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP1_MULT_PORTA_SRC11,                               \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_LO_Y_CSTAT),              \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP1_MULT_PORTA_SRC12,                               \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_LO_Y_CSTAT),              \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP1_MULT_PORTA_SRC13,                               \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_LO_Y_CSTAT),              \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP1_MULT_PORTA_SRC14,                               \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_LO_Y_CSTAT),              \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP1_MULT_PORTA_SRC15,                               \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_LO_Y_CSTAT),              \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP1_MULT_PORTB_SRC0,                                \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_LO_UV_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP1_MULT_PORTB_SRC1,                                \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_LO_UV_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP1_MULT_PORTB_SRC2,                                \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_LO_UV_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP1_MULT_PORTB_SRC3,                                \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_LO_UV_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP1_MULT_PORTB_SRC4,                                \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_LO_UV_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP1_MULT_PORTB_SRC5,                                \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_LO_UV_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP1_MULT_PORTB_SRC6,                                \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_LO_UV_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP1_MULT_PORTB_SRC7,                                \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_LO_UV_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP1_MULT_PORTB_SRC8,                                \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_LO_UV_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP1_MULT_PORTB_SRC9,                                \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_LO_UV_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP1_MULT_PORTB_SRC10,                               \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_LO_UV_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP1_MULT_PORTB_SRC11,                               \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_LO_UV_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP1_MULT_PORTB_SRC12,                               \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_LO_UV_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP1_MULT_PORTB_SRC13,                               \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_LO_UV_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP1_MULT_PORTB_SRC14,                               \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_LO_UV_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP1_MULT_PORTB_SRC15,                               \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_LO_UV_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP1_MULT_ANCA_SRC0,                                 \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_ANC_A_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP1_MULT_ANCA_SRC1,                                 \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_ANC_A_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP1_MULT_ANCA_SRC2,                                 \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_ANC_A_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP1_MULT_ANCA_SRC3,                                 \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_ANC_A_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP1_MULT_ANCA_SRC4,                                 \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_ANC_A_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP1_MULT_ANCA_SRC5,                                 \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_ANC_A_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP1_MULT_ANCA_SRC6,                                 \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_ANC_A_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP1_MULT_ANCA_SRC7,                                 \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_ANC_A_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP1_MULT_ANCA_SRC8,                                 \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_ANC_A_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP1_MULT_ANCA_SRC9,                                 \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_ANC_A_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP1_MULT_ANCA_SRC10,                                \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_ANC_A_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP1_MULT_ANCA_SRC11,                                \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_ANC_A_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP1_MULT_ANCA_SRC12,                                \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_ANC_A_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP1_MULT_ANCA_SRC13,                                \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_ANC_A_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP1_MULT_ANCA_SRC14,                                \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_ANC_A_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP1_MULT_ANCA_SRC15,                                \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_ANC_A_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP1_MULT_ANCB_SRC0,                                 \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_ANC_B_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP1_MULT_ANCB_SRC1,                                 \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_ANC_B_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP1_MULT_ANCB_SRC2,                                 \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_ANC_B_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP1_MULT_ANCB_SRC3,                                 \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_ANC_B_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP1_MULT_ANCB_SRC4,                                 \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_ANC_B_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP1_MULT_ANCB_SRC5,                                 \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_ANC_B_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP1_MULT_ANCB_SRC6,                                 \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_ANC_B_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP1_MULT_ANCB_SRC7,                                 \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_ANC_B_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP1_MULT_ANCB_SRC8,                                 \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_ANC_B_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP1_MULT_ANCB_SRC9,                                 \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_ANC_B_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP1_MULT_ANCB_SRC10,                                \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_ANC_B_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP1_MULT_ANCB_SRC11,                                \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_ANC_B_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP1_MULT_ANCB_SRC12,                                \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_ANC_B_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP1_MULT_ANCB_SRC13,                                \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_ANC_B_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP1_MULT_ANCB_SRC14,                                \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_ANC_B_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP1_MULT_ANCB_SRC15,                                \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_ANC_B_CSTAT),             \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_VIP1_PORTA_LUMA,                                     \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_LO_Y_CSTAT),              \
        VPSHAL_VPDMA_MT_TILEDMEM, 0},                                          \
    {VPSHAL_VPDMA_CHANNEL_VIP1_PORTA_CHROMA,                                   \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_LO_UV_CSTAT),             \
        VPSHAL_VPDMA_MT_TILEDMEM, 0},                                          \
    {VPSHAL_VPDMA_CHANNEL_VIP1_PORTB_LUMA,                                     \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_UP_Y_CSTAT),              \
        VPSHAL_VPDMA_MT_TILEDMEM, 0},                                          \
    {VPSHAL_VPDMA_CHANNEL_VIP1_PORTB_CHROMA,                                   \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_UP_UV_CSTAT),             \
        VPSHAL_VPDMA_MT_TILEDMEM, 0},                                          \
    {VPSHAL_VPDMA_CHANNEL_VIP1_PORTA_RGB,                                      \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_LO_Y_CSTAT),              \
        VPSHAL_VPDMA_MT_TILEDMEM, 0},                                          \
    {VPSHAL_VPDMA_CHANNEL_VIP1_PORTB_RGB,                                      \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VIP1_UP_Y_CSTAT),              \
        VPSHAL_VPDMA_MT_TILEDMEM, 0},                                          \
    {VPSHAL_VPDMA_CHANNEL_NF_RD,                                               \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->NF_RD_CSTAT),                  \
        VPSHAL_VPDMA_MT_TILEDMEM, 0},                                          \
    {VPSHAL_VPDMA_CHANNEL_NF_WR_LUMA,                                          \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->NF_WR_LUMA_CSTAT),             \
        VPSHAL_VPDMA_MT_TILEDMEM, 0},                                          \
    {VPSHAL_VPDMA_CHANNEL_NF_WR_CHROMA,                                        \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->NF_WR_CHROMA_CSTAT),           \
        VPSHAL_VPDMA_MT_TILEDMEM, 0},                                          \
    {VPSHAL_VPDMA_CHANNEL_NF_PREV_LUMA,                                        \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->NF_PREV_LUMA_CSTAT),           \
        VPSHAL_VPDMA_MT_TILEDMEM, 0},                                          \
    {VPSHAL_VPDMA_CHANNEL_NF_PREV_CHROMA,                                      \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->NF_PREV_CHROMA_CSTAT),         \
        VPSHAL_VPDMA_MT_TILEDMEM, 0},                                          \
    {VPSHAL_VPDMA_CHANNEL_VBI_HD,                                              \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VBI_HD_CSTAT),                 \
        VPSHAL_VPDMA_MT_TILEDMEM, 0},                                          \
    {VPSHAL_VPDMA_CHANNEL_VBI_SD,                                              \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->VBI_SD_CSTAT),                 \
        VPSHAL_VPDMA_MT_TILEDMEM, 0},                                          \
    {VPSHAL_VPDMA_CHANNEL_WB2,                                                 \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->WB2_CSTAT),                    \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_BP0,                                                 \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->BP0_CSTAT),                    \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_BP1,                                                 \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->BP1_CSTAT),                    \
        VPSHAL_VPDMA_MT_NONTILEDMEM, 0},                                       \
    {VPSHAL_VPDMA_CHANNEL_SEC0_LUMA,                                           \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->SEC0_LUMA_CSTAT),              \
        VPSHAL_VPDMA_MT_TILEDMEM, 0},                                          \
    {VPSHAL_VPDMA_CHANNEL_SEC0_CHROMA,                                         \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->SEC0_CHROMA_CSTAT),            \
        VPSHAL_VPDMA_MT_TILEDMEM, 0},                                          \
    {VPSHAL_VPDMA_CHANNEL_SEC1_LUMA,                                           \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->SEC1_LUMA_CSTAT),              \
        VPSHAL_VPDMA_MT_TILEDMEM, 0},                                          \
    {VPSHAL_VPDMA_CHANNEL_SEC1_CHROMA,                                         \
        (UInt32)&(((CSL_VpsVpdmaRegsOvly)0x0u)->SEC1_CHROMA_CSTAT),            \
        VPSHAL_VPDMA_MT_TILEDMEM, 0}

#define VPSHAL_VPDMA_DEFAULT_PATH_INFO                                         \
{                                                                              \
    {VPSHAL_VPDMA_PATH_VIP0_LO_UV,  TRUE, 1920, 4096,   4096},                 \
    {VPSHAL_VPDMA_PATH_VIP0_LO_Y,   TRUE, 1920, 4096,   4096},                 \
    {VPSHAL_VPDMA_PATH_VIP0_UP_UV,  TRUE, 1920, 4096,   4096},                 \
    {VPSHAL_VPDMA_PATH_VIP0_UP_Y,   TRUE, 1920, 4096,   4096},                 \
    {VPSHAL_VPDMA_PATH_VIP1_LO_UV,  TRUE, 1920, 4096,   4096},                 \
    {VPSHAL_VPDMA_PATH_VIP1_LO_Y,   TRUE, 1920, 4096,   4096},                 \
    {VPSHAL_VPDMA_PATH_VIP1_UP_UV,  TRUE, 1920, 4096,   4096},                 \
    {VPSHAL_VPDMA_PATH_VIP1_UP_Y,   TRUE, 1920, 4096,   4096},                 \
    {VPSHAL_VPDMA_PATH_VIP0_ANCA,   FALSE,   0, 4096,   4096},                 \
    {VPSHAL_VPDMA_PATH_VIP0_ANCB,   FALSE,   0, 4096,   4096},                 \
    {VPSHAL_VPDMA_PATH_VIP1_ANCA,   FALSE,   0, 4096,   4096},                 \
    {VPSHAL_VPDMA_PATH_VIP1_ANCB,   FALSE,   0, 4096,   4096},                 \
    {VPSHAL_VPDMA_PATH_SEC0,        TRUE, 1920, 1920,   4096},                 \
    {VPSHAL_VPDMA_PATH_SEC1,        TRUE, 1920, 1920,   4096},                 \
    {VPSHAL_VPDMA_PATH_PRI,         TRUE, 1920, 1920,   4096},                 \
    {VPSHAL_VPDMA_PATH_WB0,         FALSE,   0, 4096,   4096},                 \
    {VPSHAL_VPDMA_PATH_AUX,         TRUE, 1920, 1920,   4096},                 \
    {VPSHAL_VPDMA_PATH_WB1,         FALSE,   0, 4096,   4096},                 \
    {VPSHAL_VPDMA_PATH_BP0,         FALSE,   0, 4096,   4096},                 \
    {VPSHAL_VPDMA_PATH_BP1,         FALSE,   0, 4096,   4096},                 \
    {VPSHAL_VPDMA_PATH_WB2,         FALSE,   0, 4096,   4096},                 \
    {VPSHAL_VPDMA_PATH_GRPX0,       FALSE,   0, 4096,   4096},                 \
    {VPSHAL_VPDMA_PATH_GRPX0_STENC, FALSE,   0, 4096,   4096},                 \
    {VPSHAL_VPDMA_PATH_GRPX1,       FALSE,   0, 4096,   4096},                 \
    {VPSHAL_VPDMA_PATH_GRPX1_STENC, FALSE,   0, 4096,   4096},                 \
    {VPSHAL_VPDMA_PATH_GRPX2,       FALSE,   0, 4096,   4096},                 \
    {VPSHAL_VPDMA_PATH_GRPX2_STENC, FALSE,   0, 4096,   4096},                 \
    {VPSHAL_VPDMA_PATH_VBI_HD,      FALSE,   0, 4096,   4096},                 \
    {VPSHAL_VPDMA_PATH_VBI_SD,      FALSE,   0, 4096,   4096},                 \
    {VPSHAL_VPDMA_PATH_NF,          TRUE, 1920, 4096,   4096}                  \
}


/* ========================================================================== */
/*                         Structure Declarations                             */
/* ========================================================================== */

/* None*/

/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */

/* None */


#ifdef __cplusplus
}
#endif

#endif /* #ifndef _VPSSHALVPDMADEFAULTS_TI816X_H */
