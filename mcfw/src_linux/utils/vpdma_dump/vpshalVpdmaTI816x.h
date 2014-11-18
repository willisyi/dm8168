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

#ifndef _VPSHAL_VPDMA_TI816X_H
#define _VPSHAL_VPDMA_TI816X_H

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */

/* None */

#ifdef __cplusplus
extern "C"
{
#endif

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */

/**
 *  Enum for channel numbers. These channel numbers will be directly
 *  used by the list manager clients to configure data descriptor
 */
typedef enum
{
    VPSHAL_VPDMA_CHANNEL_INVALID = -1,
    VPSHAL_VPDMA_CHANNEL_PRI_LUMA = 0,                  /* 0   */
    VPSHAL_VPDMA_CHANNEL_PRI_CHROMA,                    /* 1   */
    VPSHAL_VPDMA_CHANNEL_PRI_FLD1_LUMA,                 /* 2   */
    VPSHAL_VPDMA_CHANNEL_PRI_FLD1_CHROMA,               /* 3   */
    VPSHAL_VPDMA_CHANNEL_PRI_FLD2_LUMA,                 /* 4   */
    VPSHAL_VPDMA_CHANNEL_PRI_FLD2_CHROMA,               /* 5   */
    VPSHAL_VPDMA_CHANNEL_PRI_FLD3_LUMA,                 /* 6   */
    VPSHAL_VPDMA_CHANNEL_PRI_FLD3_CHROMA,               /* 7   */
    VPSHAL_VPDMA_CHANNEL_PRI_FLD_WR_TOP_LUMA,           /* 8   */
    VPSHAL_VPDMA_CHANNEL_PRI_FLD_WR_TOP_CHROMA,         /* 9   */
    VPSHAL_VPDMA_CHANNEL_PRI_FLD_WR_BOT_LUMA,           /* 10  */
    VPSHAL_VPDMA_CHANNEL_PRI_FLD_WR_BOT_CHROMA,         /* 11  */
    VPSHAL_VPDMA_CHANNEL_PRI_MV0 = 12,                  /* 12  */
    VPSHAL_VPDMA_CHANNEL_PRI_MV1 = 13,                  /* 13  */
    VPSHAL_VPDMA_CHANNEL_PRI_MVSTM,                     /* 14  */
    VPSHAL_VPDMA_CHANNEL_PRI_MV_OUT = 15,               /* 15  */
    VPSHAL_VPDMA_CHANNEL_PRI_MVSTM_OUT = 16,            /* 16  */
    VPSHAL_VPDMA_CHANNEL_WB0 = 17,                      /* 17  */
    VPSHAL_VPDMA_CHANNEL_AUX_LUMA = 18,                 /* 18  */
    VPSHAL_VPDMA_CHANNEL_AUX_CHROMA,                    /* 19  */
    VPSHAL_VPDMA_CHANNEL_AUX_FLD1_LUMA,                 /* 20  */
    VPSHAL_VPDMA_CHANNEL_AUX_FLD1_CHROMA,               /* 21  */
    VPSHAL_VPDMA_CHANNEL_AUX_FLD2_LUMA,                 /* 22  */
    VPSHAL_VPDMA_CHANNEL_AUX_FLD2_CHROMA,               /* 23  */
    VPSHAL_VPDMA_CHANNEL_AUX_FLD_WR_LUMA,               /* 24  */
    VPSHAL_VPDMA_CHANNEL_AUX_FLD_WR_CHROMA,             /* 25  */
    VPSHAL_VPDMA_CHANNEL_AUX_MV,                        /* 26  */
    VPSHAL_VPDMA_CHANNEL_AUX_MV_OUT,                    /* 27  */
    VPSHAL_VPDMA_CHANNEL_WB1 = 28,                      /* 28  */
    VPSHAL_VPDMA_CHANNEL_GRPX0,                         /* 29  */
    VPSHAL_VPDMA_CHANNEL_GRPX1,                         /* 30  */
    VPSHAL_VPDMA_CHANNEL_GRPX2,                         /* 31  */
    VPSHAL_VPDMA_CHANNEL_STENCIL0,                      /* 32  */
    VPSHAL_VPDMA_CHANNEL_STENCIL1,                      /* 33  */
    VPSHAL_VPDMA_CHANNEL_STENCIL2,                      /* 34  */
    VPSHAL_VPDMA_CHANNEL_CLUT0,                         /* 35  */
    VPSHAL_VPDMA_CHANNEL_CLUT1,                         /* 36  */
    VPSHAL_VPDMA_CHANNEL_CLUT2,                         /* 37  */
    VPSHAL_VPDMA_CHANNEL_VIP0_MULT_PORTA_SRC0,          /* 38  */
    VPSHAL_VPDMA_CHANNEL_VIP0_MULT_PORTA_SRC1,          /* 39  */
    VPSHAL_VPDMA_CHANNEL_VIP0_MULT_PORTA_SRC2,          /* 40  */
    VPSHAL_VPDMA_CHANNEL_VIP0_MULT_PORTA_SRC3,          /* 41  */
    VPSHAL_VPDMA_CHANNEL_VIP0_MULT_PORTA_SRC4,          /* 42  */
    VPSHAL_VPDMA_CHANNEL_VIP0_MULT_PORTA_SRC5,          /* 43  */
    VPSHAL_VPDMA_CHANNEL_VIP0_MULT_PORTA_SRC6,          /* 44  */
    VPSHAL_VPDMA_CHANNEL_VIP0_MULT_PORTA_SRC7,          /* 45  */
    VPSHAL_VPDMA_CHANNEL_VIP0_MULT_PORTA_SRC8,          /* 46  */
    VPSHAL_VPDMA_CHANNEL_VIP0_MULT_PORTA_SRC9,          /* 47  */
    VPSHAL_VPDMA_CHANNEL_VIP0_MULT_PORTA_SRC10,         /* 48  */
    VPSHAL_VPDMA_CHANNEL_VIP0_MULT_PORTA_SRC11,         /* 49  */
    VPSHAL_VPDMA_CHANNEL_VIP0_MULT_PORTA_SRC12,         /* 50  */
    VPSHAL_VPDMA_CHANNEL_VIP0_MULT_PORTA_SRC13,         /* 51  */
    VPSHAL_VPDMA_CHANNEL_VIP0_MULT_PORTA_SRC14,         /* 52  */
    VPSHAL_VPDMA_CHANNEL_VIP0_MULT_PORTA_SRC15,         /* 53  */
    VPSHAL_VPDMA_CHANNEL_VIP0_MULT_PORTB_SRC0,          /* 54  */
    VPSHAL_VPDMA_CHANNEL_VIP0_MULT_PORTB_SRC1,          /* 55  */
    VPSHAL_VPDMA_CHANNEL_VIP0_MULT_PORTB_SRC2,          /* 56  */
    VPSHAL_VPDMA_CHANNEL_VIP0_MULT_PORTB_SRC3,          /* 57  */
    VPSHAL_VPDMA_CHANNEL_VIP0_MULT_PORTB_SRC4,          /* 58  */
    VPSHAL_VPDMA_CHANNEL_VIP0_MULT_PORTB_SRC5,          /* 59  */
    VPSHAL_VPDMA_CHANNEL_VIP0_MULT_PORTB_SRC6,          /* 60  */
    VPSHAL_VPDMA_CHANNEL_VIP0_MULT_PORTB_SRC7,          /* 61  */
    VPSHAL_VPDMA_CHANNEL_VIP0_MULT_PORTB_SRC8,          /* 62  */
    VPSHAL_VPDMA_CHANNEL_VIP0_MULT_PORTB_SRC9,          /* 63  */
    VPSHAL_VPDMA_CHANNEL_VIP0_MULT_PORTB_SRC10,         /* 64  */
    VPSHAL_VPDMA_CHANNEL_VIP0_MULT_PORTB_SRC11,         /* 65  */
    VPSHAL_VPDMA_CHANNEL_VIP0_MULT_PORTB_SRC12,         /* 66  */
    VPSHAL_VPDMA_CHANNEL_VIP0_MULT_PORTB_SRC13,         /* 67  */
    VPSHAL_VPDMA_CHANNEL_VIP0_MULT_PORTB_SRC14,         /* 68  */
    VPSHAL_VPDMA_CHANNEL_VIP0_MULT_PORTB_SRC15,         /* 69  */
    VPSHAL_VPDMA_CHANNEL_VIP0_MULT_ANCA_SRC0,           /* 70  */
    VPSHAL_VPDMA_CHANNEL_VIP0_MULT_ANCA_SRC1,           /* 71  */
    VPSHAL_VPDMA_CHANNEL_VIP0_MULT_ANCA_SRC2,           /* 72  */
    VPSHAL_VPDMA_CHANNEL_VIP0_MULT_ANCA_SRC3,           /* 73  */
    VPSHAL_VPDMA_CHANNEL_VIP0_MULT_ANCA_SRC4,           /* 74  */
    VPSHAL_VPDMA_CHANNEL_VIP0_MULT_ANCA_SRC5,           /* 75  */
    VPSHAL_VPDMA_CHANNEL_VIP0_MULT_ANCA_SRC6,           /* 76  */
    VPSHAL_VPDMA_CHANNEL_VIP0_MULT_ANCA_SRC7,           /* 77  */
    VPSHAL_VPDMA_CHANNEL_VIP0_MULT_ANCA_SRC8,           /* 78  */
    VPSHAL_VPDMA_CHANNEL_VIP0_MULT_ANCA_SRC9,           /* 79  */
    VPSHAL_VPDMA_CHANNEL_VIP0_MULT_ANCA_SRC10,          /* 80  */
    VPSHAL_VPDMA_CHANNEL_VIP0_MULT_ANCA_SRC11,          /* 81  */
    VPSHAL_VPDMA_CHANNEL_VIP0_MULT_ANCA_SRC12,          /* 82  */
    VPSHAL_VPDMA_CHANNEL_VIP0_MULT_ANCA_SRC13,          /* 83  */
    VPSHAL_VPDMA_CHANNEL_VIP0_MULT_ANCA_SRC14,          /* 84  */
    VPSHAL_VPDMA_CHANNEL_VIP0_MULT_ANCA_SRC15,          /* 85  */
    VPSHAL_VPDMA_CHANNEL_VIP0_MULT_ANCB_SRC0,           /* 86  */
    VPSHAL_VPDMA_CHANNEL_VIP0_MULT_ANCB_SRC1,           /* 87  */
    VPSHAL_VPDMA_CHANNEL_VIP0_MULT_ANCB_SRC2,           /* 88  */
    VPSHAL_VPDMA_CHANNEL_VIP0_MULT_ANCB_SRC3,           /* 89  */
    VPSHAL_VPDMA_CHANNEL_VIP0_MULT_ANCB_SRC4,           /* 90  */
    VPSHAL_VPDMA_CHANNEL_VIP0_MULT_ANCB_SRC5,           /* 91  */
    VPSHAL_VPDMA_CHANNEL_VIP0_MULT_ANCB_SRC6,           /* 92  */
    VPSHAL_VPDMA_CHANNEL_VIP0_MULT_ANCB_SRC7,           /* 93  */
    VPSHAL_VPDMA_CHANNEL_VIP0_MULT_ANCB_SRC8,           /* 94  */
    VPSHAL_VPDMA_CHANNEL_VIP0_MULT_ANCB_SRC9,           /* 95  */
    VPSHAL_VPDMA_CHANNEL_VIP0_MULT_ANCB_SRC10,          /* 96  */
    VPSHAL_VPDMA_CHANNEL_VIP0_MULT_ANCB_SRC11,          /* 97  */
    VPSHAL_VPDMA_CHANNEL_VIP0_MULT_ANCB_SRC12,          /* 98  */
    VPSHAL_VPDMA_CHANNEL_VIP0_MULT_ANCB_SRC13,          /* 99  */
    VPSHAL_VPDMA_CHANNEL_VIP0_MULT_ANCB_SRC14,          /* 100 */
    VPSHAL_VPDMA_CHANNEL_VIP0_MULT_ANCB_SRC15,          /* 101 */
    VPSHAL_VPDMA_CHANNEL_VIP0_PORTA_LUMA,               /* 102 */
    VPSHAL_VPDMA_CHANNEL_VIP0_PORTA_CHROMA,             /* 103 */
    VPSHAL_VPDMA_CHANNEL_VIP0_PORTB_LUMA,               /* 104 */
    VPSHAL_VPDMA_CHANNEL_VIP0_PORTB_CHROMA,             /* 105 */
    VPSHAL_VPDMA_CHANNEL_VIP0_PORTA_RGB,                /* 106 */
    VPSHAL_VPDMA_CHANNEL_VIP0_PORTB_RGB,                /* 107 */
    VPSHAL_VPDMA_CHANNEL_VIP1_MULT_PORTA_SRC0,          /* 108 */
    VPSHAL_VPDMA_CHANNEL_VIP1_MULT_PORTA_SRC1,          /* 109 */
    VPSHAL_VPDMA_CHANNEL_VIP1_MULT_PORTA_SRC2,          /* 110 */
    VPSHAL_VPDMA_CHANNEL_VIP1_MULT_PORTA_SRC3,          /* 111 */
    VPSHAL_VPDMA_CHANNEL_VIP1_MULT_PORTA_SRC4,          /* 112 */
    VPSHAL_VPDMA_CHANNEL_VIP1_MULT_PORTA_SRC5,          /* 113 */
    VPSHAL_VPDMA_CHANNEL_VIP1_MULT_PORTA_SRC6,          /* 114 */
    VPSHAL_VPDMA_CHANNEL_VIP1_MULT_PORTA_SRC7,          /* 115 */
    VPSHAL_VPDMA_CHANNEL_VIP1_MULT_PORTA_SRC8,          /* 116 */
    VPSHAL_VPDMA_CHANNEL_VIP1_MULT_PORTA_SRC9,          /* 117 */
    VPSHAL_VPDMA_CHANNEL_VIP1_MULT_PORTA_SRC10,         /* 118 */
    VPSHAL_VPDMA_CHANNEL_VIP1_MULT_PORTA_SRC11,         /* 119 */
    VPSHAL_VPDMA_CHANNEL_VIP1_MULT_PORTA_SRC12,         /* 120 */
    VPSHAL_VPDMA_CHANNEL_VIP1_MULT_PORTA_SRC13,         /* 121 */
    VPSHAL_VPDMA_CHANNEL_VIP1_MULT_PORTA_SRC14,         /* 122 */
    VPSHAL_VPDMA_CHANNEL_VIP1_MULT_PORTA_SRC15,         /* 123 */
    VPSHAL_VPDMA_CHANNEL_VIP1_MULT_PORTB_SRC0,          /* 124 */
    VPSHAL_VPDMA_CHANNEL_VIP1_MULT_PORTB_SRC1,          /* 125 */
    VPSHAL_VPDMA_CHANNEL_VIP1_MULT_PORTB_SRC2,          /* 126 */
    VPSHAL_VPDMA_CHANNEL_VIP1_MULT_PORTB_SRC3,          /* 127 */
    VPSHAL_VPDMA_CHANNEL_VIP1_MULT_PORTB_SRC4,          /* 128 */
    VPSHAL_VPDMA_CHANNEL_VIP1_MULT_PORTB_SRC5,          /* 129 */
    VPSHAL_VPDMA_CHANNEL_VIP1_MULT_PORTB_SRC6,          /* 130 */
    VPSHAL_VPDMA_CHANNEL_VIP1_MULT_PORTB_SRC7,          /* 131 */
    VPSHAL_VPDMA_CHANNEL_VIP1_MULT_PORTB_SRC8,          /* 132 */
    VPSHAL_VPDMA_CHANNEL_VIP1_MULT_PORTB_SRC9,          /* 133 */
    VPSHAL_VPDMA_CHANNEL_VIP1_MULT_PORTB_SRC10,         /* 134 */
    VPSHAL_VPDMA_CHANNEL_VIP1_MULT_PORTB_SRC11,         /* 135 */
    VPSHAL_VPDMA_CHANNEL_VIP1_MULT_PORTB_SRC12,         /* 136 */
    VPSHAL_VPDMA_CHANNEL_VIP1_MULT_PORTB_SRC13,         /* 137 */
    VPSHAL_VPDMA_CHANNEL_VIP1_MULT_PORTB_SRC14,         /* 138 */
    VPSHAL_VPDMA_CHANNEL_VIP1_MULT_PORTB_SRC15,         /* 139 */
    VPSHAL_VPDMA_CHANNEL_VIP1_MULT_ANCA_SRC0,           /* 140 */
    VPSHAL_VPDMA_CHANNEL_VIP1_MULT_ANCA_SRC1,           /* 141 */
    VPSHAL_VPDMA_CHANNEL_VIP1_MULT_ANCA_SRC2,           /* 142 */
    VPSHAL_VPDMA_CHANNEL_VIP1_MULT_ANCA_SRC3,           /* 143 */
    VPSHAL_VPDMA_CHANNEL_VIP1_MULT_ANCA_SRC4,           /* 144 */
    VPSHAL_VPDMA_CHANNEL_VIP1_MULT_ANCA_SRC5,           /* 145 */
    VPSHAL_VPDMA_CHANNEL_VIP1_MULT_ANCA_SRC6,           /* 146 */
    VPSHAL_VPDMA_CHANNEL_VIP1_MULT_ANCA_SRC7,           /* 147 */
    VPSHAL_VPDMA_CHANNEL_VIP1_MULT_ANCA_SRC8,           /* 148 */
    VPSHAL_VPDMA_CHANNEL_VIP1_MULT_ANCA_SRC9,           /* 149 */
    VPSHAL_VPDMA_CHANNEL_VIP1_MULT_ANCA_SRC10,          /* 150 */
    VPSHAL_VPDMA_CHANNEL_VIP1_MULT_ANCA_SRC11,          /* 151 */
    VPSHAL_VPDMA_CHANNEL_VIP1_MULT_ANCA_SRC12,          /* 152 */
    VPSHAL_VPDMA_CHANNEL_VIP1_MULT_ANCA_SRC13,          /* 153 */
    VPSHAL_VPDMA_CHANNEL_VIP1_MULT_ANCA_SRC14,          /* 154 */
    VPSHAL_VPDMA_CHANNEL_VIP1_MULT_ANCA_SRC15,          /* 155 */
    VPSHAL_VPDMA_CHANNEL_VIP1_MULT_ANCB_SRC0,           /* 156 */
    VPSHAL_VPDMA_CHANNEL_VIP1_MULT_ANCB_SRC1,           /* 157 */
    VPSHAL_VPDMA_CHANNEL_VIP1_MULT_ANCB_SRC2,           /* 158 */
    VPSHAL_VPDMA_CHANNEL_VIP1_MULT_ANCB_SRC3,           /* 159 */
    VPSHAL_VPDMA_CHANNEL_VIP1_MULT_ANCB_SRC4,           /* 160 */
    VPSHAL_VPDMA_CHANNEL_VIP1_MULT_ANCB_SRC5,           /* 161 */
    VPSHAL_VPDMA_CHANNEL_VIP1_MULT_ANCB_SRC6,           /* 162 */
    VPSHAL_VPDMA_CHANNEL_VIP1_MULT_ANCB_SRC7,           /* 163 */
    VPSHAL_VPDMA_CHANNEL_VIP1_MULT_ANCB_SRC8,           /* 164 */
    VPSHAL_VPDMA_CHANNEL_VIP1_MULT_ANCB_SRC9,           /* 165 */
    VPSHAL_VPDMA_CHANNEL_VIP1_MULT_ANCB_SRC10,          /* 166 */
    VPSHAL_VPDMA_CHANNEL_VIP1_MULT_ANCB_SRC11,          /* 167 */
    VPSHAL_VPDMA_CHANNEL_VIP1_MULT_ANCB_SRC12,          /* 168 */
    VPSHAL_VPDMA_CHANNEL_VIP1_MULT_ANCB_SRC13,          /* 169 */
    VPSHAL_VPDMA_CHANNEL_VIP1_MULT_ANCB_SRC14,          /* 170 */
    VPSHAL_VPDMA_CHANNEL_VIP1_MULT_ANCB_SRC15,          /* 171 */
    VPSHAL_VPDMA_CHANNEL_VIP1_PORTA_LUMA,               /* 172 */
    VPSHAL_VPDMA_CHANNEL_VIP1_PORTA_CHROMA,             /* 173 */
    VPSHAL_VPDMA_CHANNEL_VIP1_PORTB_LUMA,               /* 174 */
    VPSHAL_VPDMA_CHANNEL_VIP1_PORTB_CHROMA,             /* 175 */
    VPSHAL_VPDMA_CHANNEL_VIP1_PORTA_RGB,                /* 176 */
    VPSHAL_VPDMA_CHANNEL_VIP1_PORTB_RGB,                /* 177 */
    VPSHAL_VPDMA_CHANNEL_NF_RD,                         /* 178 */
    VPSHAL_VPDMA_CHANNEL_NF_WR_LUMA,                    /* 179 */
    VPSHAL_VPDMA_CHANNEL_NF_WR_CHROMA,                  /* 180 */
    VPSHAL_VPDMA_CHANNEL_NF_PREV_LUMA,                  /* 181 */
    VPSHAL_VPDMA_CHANNEL_NF_PREV_CHROMA,                /* 182 */
    VPSHAL_VPDMA_CHANNEL_VBI_HD = 183,                  /* 183 */
    VPSHAL_VPDMA_CHANNEL_VBI_SD = 184,                  /* 184 */
    VPSHAL_VPDMA_CHANNEL_WB2,                           /* 185 */
    VPSHAL_VPDMA_CHANNEL_BP0,                           /* 186 */
    VPSHAL_VPDMA_CHANNEL_BP1,                           /* 187 */
    VPSHAL_VPDMA_CHANNEL_SEC0_LUMA,                     /* 188 */
    VPSHAL_VPDMA_CHANNEL_SEC0_CHROMA,                   /* 189 */
    VPSHAL_VPDMA_CHANNEL_SEC1_LUMA,                     /* 190 */
    VPSHAL_VPDMA_CHANNEL_SEC1_CHROMA,                   /* 191 */
    VPSHAL_VPDMA_CHANNEL_NUM_CHANNELS
} VpsHal_VpdmaChannel;


/* ========================================================================== */
/*                         Structure Declarations                             */
/* ========================================================================== */

/* None */

/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */

/* None */

#endif /* End of #ifndef _VPSHAL_VPDMA_TI816X_H */
