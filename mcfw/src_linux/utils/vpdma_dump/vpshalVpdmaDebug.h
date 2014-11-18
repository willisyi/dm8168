/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/


/**
 *  \file vpshalVpdmaDebug.h
 *
 *  \brief VPS VPDMA HAL debug internal header file defining the needed strings
 *  and macros.
 */

#ifndef _VPSHAL_VPDMA_DEBUG_H
#define _VPSHAL_VPDMA_DEBUG_H

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

/* None */

/* ========================================================================== */
/*                         Structure Declarations                             */
/* ========================================================================== */

/* VPDMA channel names. */
#ifdef TI_816X_BUILD
/* TI816X platform */
static char *gVpsVpdmaHalChStr[] =
{
    "PRI_LUMA",                     /* 0   */
    "PRI_CHROMA",                   /* 1   */
    "PRI_FLD1_LUMA",                /* 2   */
    "PRI_FLD1_CHROMA",              /* 3   */
    "PRI_FLD2_LUMA",                /* 4   */
    "PRI_FLD2_CHROMA",              /* 5   */
    "PRI_FLD3_LUMA",                /* 6   */
    "PRI_FLD3_CHROMA",              /* 7   */
    "PRI_FLD_WR_TOP_LUMA",          /* 8   */
    "PRI_FLD_WR_TOP_CHROMA",        /* 9   */
    "PRI_FLD_WR_BOT_LUMA",          /* 10  */
    "PRI_FLD_WR_BOT_CHROMA",        /* 11  */
    "PRI_MV0",                      /* 12  */
    "PRI_MV1",                      /* 13  */
    "PRI_MVSTM",                    /* 14  */
    "PRI_MV_OUT",                   /* 15  */
    "PRI_MVSTM_OUT",                /* 16  */
    "WB0",                          /* 17  */
    "AUX_LUMA",                     /* 18  */
    "AUX_CHROMA",                   /* 19  */
    "AUX_FLD1_LUMA",                /* 20  */
    "AUX_FLD1_CHROMA",              /* 21  */
    "AUX_FLD2_LUMA",                /* 22  */
    "AUX_FLD2_CHROMA",              /* 23  */
    "AUX_FLD_WR_LUMA",              /* 24  */
    "AUX_FLD_WR_CHROMA",            /* 25  */
    "AUX_MV",                       /* 26  */
    "AUX_MV_OUT",                   /* 27  */
    "WB1",                          /* 28  */
    "GRPX0",                        /* 29  */
    "GRPX1",                        /* 30  */
    "GRPX2",                        /* 31  */
    "STENCIL0",                     /* 32  */
    "STENCIL1",                     /* 33  */
    "STENCIL2",                     /* 34  */
    "CLUT0",                        /* 35  */
    "CLUT1",                        /* 36  */
    "CLUT2",                        /* 37  */
    "VIP0_MULT_PORTA_SRC0",         /* 38  */
    "VIP0_MULT_PORTA_SRC1",         /* 39  */
    "VIP0_MULT_PORTA_SRC2",         /* 40  */
    "VIP0_MULT_PORTA_SRC3",         /* 41  */
    "VIP0_MULT_PORTA_SRC4",         /* 42  */
    "VIP0_MULT_PORTA_SRC5",         /* 43  */
    "VIP0_MULT_PORTA_SRC6",         /* 44  */
    "VIP0_MULT_PORTA_SRC7",         /* 45  */
    "VIP0_MULT_PORTA_SRC8",         /* 46  */
    "VIP0_MULT_PORTA_SRC9",         /* 47  */
    "VIP0_MULT_PORTA_SRC10",        /* 48  */
    "VIP0_MULT_PORTA_SRC11",        /* 49  */
    "VIP0_MULT_PORTA_SRC12",        /* 50  */
    "VIP0_MULT_PORTA_SRC13",        /* 51  */
    "VIP0_MULT_PORTA_SRC14",        /* 52  */
    "VIP0_MULT_PORTA_SRC15",        /* 53  */
    "VIP0_MULT_PORTB_SRC0",         /* 54  */
    "VIP0_MULT_PORTB_SRC1",         /* 55  */
    "VIP0_MULT_PORTB_SRC2",         /* 56  */
    "VIP0_MULT_PORTB_SRC3",         /* 57  */
    "VIP0_MULT_PORTB_SRC4",         /* 58  */
    "VIP0_MULT_PORTB_SRC5",         /* 59  */
    "VIP0_MULT_PORTB_SRC6",         /* 60  */
    "VIP0_MULT_PORTB_SRC7",         /* 61  */
    "VIP0_MULT_PORTB_SRC8",         /* 62  */
    "VIP0_MULT_PORTB_SRC9",         /* 63  */
    "VIP0_MULT_PORTB_SRC10",        /* 64  */
    "VIP0_MULT_PORTB_SRC11",        /* 65  */
    "VIP0_MULT_PORTB_SRC12",        /* 66  */
    "VIP0_MULT_PORTB_SRC13",        /* 67  */
    "VIP0_MULT_PORTB_SRC14",        /* 68  */
    "VIP0_MULT_PORTB_SRC15",        /* 69  */
    "VIP0_MULT_ANCA_SRC0",          /* 70  */
    "VIP0_MULT_ANCA_SRC1",          /* 71  */
    "VIP0_MULT_ANCA_SRC2",          /* 72  */
    "VIP0_MULT_ANCA_SRC3",          /* 73  */
    "VIP0_MULT_ANCA_SRC4",          /* 74  */
    "VIP0_MULT_ANCA_SRC5",          /* 75  */
    "VIP0_MULT_ANCA_SRC6",          /* 76  */
    "VIP0_MULT_ANCA_SRC7",          /* 77  */
    "VIP0_MULT_ANCA_SRC8",          /* 78  */
    "VIP0_MULT_ANCA_SRC9",          /* 79  */
    "VIP0_MULT_ANCA_SRC10",         /* 80  */
    "VIP0_MULT_ANCA_SRC11",         /* 81  */
    "VIP0_MULT_ANCA_SRC12",         /* 82  */
    "VIP0_MULT_ANCA_SRC13",         /* 83  */
    "VIP0_MULT_ANCA_SRC14",         /* 84  */
    "VIP0_MULT_ANCA_SRC15",         /* 85  */
    "VIP0_MULT_ANCB_SRC0",          /* 86  */
    "VIP0_MULT_ANCB_SRC1",          /* 87  */
    "VIP0_MULT_ANCB_SRC2",          /* 88  */
    "VIP0_MULT_ANCB_SRC3",          /* 89  */
    "VIP0_MULT_ANCB_SRC4",          /* 90  */
    "VIP0_MULT_ANCB_SRC5",          /* 91  */
    "VIP0_MULT_ANCB_SRC6",          /* 92  */
    "VIP0_MULT_ANCB_SRC7",          /* 93  */
    "VIP0_MULT_ANCB_SRC8",          /* 94  */
    "VIP0_MULT_ANCB_SRC9",          /* 95  */
    "VIP0_MULT_ANCB_SRC10",         /* 96  */
    "VIP0_MULT_ANCB_SRC11",         /* 97  */
    "VIP0_MULT_ANCB_SRC12",         /* 98  */
    "VIP0_MULT_ANCB_SRC13",         /* 99  */
    "VIP0_MULT_ANCB_SRC14",         /* 100 */
    "VIP0_MULT_ANCB_SRC15",         /* 101 */
    "VIP0_PORTA_LUMA",              /* 102 */
    "VIP0_PORTA_CHROMA",            /* 103 */
    "VIP0_PORTB_LUMA",              /* 104 */
    "VIP0_PORTB_CHROMA",            /* 105 */
    "VIP0_PORTA_RGB",               /* 106 */
    "VIP0_PORTB_RGB",               /* 107 */
    "VIP1_MULT_PORTA_SRC0",         /* 108 */
    "VIP1_MULT_PORTA_SRC1",         /* 109 */
    "VIP1_MULT_PORTA_SRC2",         /* 110 */
    "VIP1_MULT_PORTA_SRC3",         /* 111 */
    "VIP1_MULT_PORTA_SRC4",         /* 112 */
    "VIP1_MULT_PORTA_SRC5",         /* 113 */
    "VIP1_MULT_PORTA_SRC6",         /* 114 */
    "VIP1_MULT_PORTA_SRC7",         /* 115 */
    "VIP1_MULT_PORTA_SRC8",         /* 116 */
    "VIP1_MULT_PORTA_SRC9",         /* 117 */
    "VIP1_MULT_PORTA_SRC10",        /* 118 */
    "VIP1_MULT_PORTA_SRC11",        /* 119 */
    "VIP1_MULT_PORTA_SRC12",        /* 120 */
    "VIP1_MULT_PORTA_SRC13",        /* 121 */
    "VIP1_MULT_PORTA_SRC14",        /* 122 */
    "VIP1_MULT_PORTA_SRC15",        /* 123 */
    "VIP1_MULT_PORTB_SRC0",         /* 124 */
    "VIP1_MULT_PORTB_SRC1",         /* 125 */
    "VIP1_MULT_PORTB_SRC2",         /* 126 */
    "VIP1_MULT_PORTB_SRC3",         /* 127 */
    "VIP1_MULT_PORTB_SRC4",         /* 128 */
    "VIP1_MULT_PORTB_SRC5",         /* 129 */
    "VIP1_MULT_PORTB_SRC6",         /* 130 */
    "VIP1_MULT_PORTB_SRC7",         /* 131 */
    "VIP1_MULT_PORTB_SRC8",         /* 132 */
    "VIP1_MULT_PORTB_SRC9",         /* 133 */
    "VIP1_MULT_PORTB_SRC10",        /* 134 */
    "VIP1_MULT_PORTB_SRC11",        /* 135 */
    "VIP1_MULT_PORTB_SRC12",        /* 136 */
    "VIP1_MULT_PORTB_SRC13",        /* 137 */
    "VIP1_MULT_PORTB_SRC14",        /* 138 */
    "VIP1_MULT_PORTB_SRC15",        /* 139 */
    "VIP1_MULT_ANCA_SRC0",          /* 140 */
    "VIP1_MULT_ANCA_SRC1",          /* 141 */
    "VIP1_MULT_ANCA_SRC2",          /* 142 */
    "VIP1_MULT_ANCA_SRC3",          /* 143 */
    "VIP1_MULT_ANCA_SRC4",          /* 144 */
    "VIP1_MULT_ANCA_SRC5",          /* 145 */
    "VIP1_MULT_ANCA_SRC6",          /* 146 */
    "VIP1_MULT_ANCA_SRC7",          /* 147 */
    "VIP1_MULT_ANCA_SRC8",          /* 148 */
    "VIP1_MULT_ANCA_SRC9",          /* 149 */
    "VIP1_MULT_ANCA_SRC10",         /* 150 */
    "VIP1_MULT_ANCA_SRC11",         /* 151 */
    "VIP1_MULT_ANCA_SRC12",         /* 152 */
    "VIP1_MULT_ANCA_SRC13",         /* 153 */
    "VIP1_MULT_ANCA_SRC14",         /* 154 */
    "VIP1_MULT_ANCA_SRC15",         /* 155 */
    "VIP1_MULT_ANCB_SRC0",          /* 156 */
    "VIP1_MULT_ANCB_SRC1",          /* 157 */
    "VIP1_MULT_ANCB_SRC2",          /* 158 */
    "VIP1_MULT_ANCB_SRC3",          /* 159 */
    "VIP1_MULT_ANCB_SRC4",          /* 160 */
    "VIP1_MULT_ANCB_SRC5",          /* 161 */
    "VIP1_MULT_ANCB_SRC6",          /* 162 */
    "VIP1_MULT_ANCB_SRC7",          /* 163 */
    "VIP1_MULT_ANCB_SRC8",          /* 164 */
    "VIP1_MULT_ANCB_SRC9",          /* 165 */
    "VIP1_MULT_ANCB_SRC10",         /* 166 */
    "VIP1_MULT_ANCB_SRC11",         /* 167 */
    "VIP1_MULT_ANCB_SRC12",         /* 168 */
    "VIP1_MULT_ANCB_SRC13",         /* 169 */
    "VIP1_MULT_ANCB_SRC14",         /* 170 */
    "VIP1_MULT_ANCB_SRC15",         /* 171 */
    "VIP1_PORTA_LUMA",              /* 172 */
    "VIP1_PORTA_CHROMA",            /* 173 */
    "VIP1_PORTB_LUMA",              /* 174 */
    "VIP1_PORTB_CHROMA",            /* 175 */
    "VIP1_PORTA_RGB",               /* 176 */
    "VIP1_PORTB_RGB",               /* 177 */
    "NF_RD",                        /* 178 */
    "NF_WR_LUMA",                   /* 179 */
    "NF_WR_CHROMA",                 /* 180 */
    "NF_LAST_LUMA",                 /* 181 */
    "NF_LAST_CHROMA",               /* 182 */
    "VBI_HD_VENC",                  /* 183 */
    "VBI_SD_VENC",                  /* 184 */
    "WB2",                          /* 185 */
    "BP0",                          /* 186 */
    "BP1",                          /* 187 */
    "SEC0_LUMA",                    /* 188 */
    "SEC0_CHROMA",                  /* 189 */
    "SEC1_LUMA",                    /* 190 */
    "SEC1_CHROMA"                   /* 191 */
};
#else
/* TI814X platform */
static char *gVpsVpdmaHalChStr[] =
{
    "PRI_LUMA",                     /* 0   */
    "PRI_CHROMA",                   /* 1   */
    "PRI_FLD1_LUMA",                /* 2   */
    "PRI_FLD1_CHROMA",              /* 3   */
    "PRI_FLD2_LUMA",                /* 4   */
    "PRI_FLD2_CHROMA",              /* 5   */
    "INVALID_VPDMA_CH [Warning]",   /* 6   */
    "INVALID_VPDMA_CH [Warning]",   /* 7   */
    "INVALID_VPDMA_CH [Warning]",   /* 8   */
    "INVALID_VPDMA_CH [Warning]",   /* 9   */
    "INVALID_VPDMA_CH [Warning]",   /* 10  */
    "INVALID_VPDMA_CH [Warning]",   /* 11  */
    "PRI_MV0",                      /* 12  */
    "INVALID_VPDMA_CH [Warning]",   /* 13  */
    "INVALID_VPDMA_CH [Warning]",   /* 14  */
    "PRI_MV_OUT",                   /* 15  */
    "INVALID_VPDMA_CH [Warning]",   /* 16  */
    "WB0",                          /* 17  */
    "AUX_LUMA",                     /* 18  */
    "AUX_CHROMA",                   /* 19  */
    "INVALID_VPDMA_CH [Warning]",   /* 20  */
    "INVALID_VPDMA_CH [Warning]",   /* 21  */
    "INVALID_VPDMA_CH [Warning]",   /* 22  */
    "INVALID_VPDMA_CH [Warning]",   /* 23  */
    "INVALID_VPDMA_CH [Warning]",   /* 24  */
    "INVALID_VPDMA_CH [Warning]",   /* 25  */
    "INVALID_VPDMA_CH [Warning]",   /* 26  */
    "INVALID_VPDMA_CH [Warning]",   /* 27  */
    "WB1",                          /* 28  */
    "GRPX0",                        /* 29  */
    "GRPX1",                        /* 30  */
    "GRPX2",                        /* 31  */
    "STENCIL0",                     /* 32  */
    "STENCIL1",                     /* 33  */
    "STENCIL2",                     /* 34  */
    "CLUT0",                        /* 35  */
    "CLUT1",                        /* 36  */
    "CLUT2",                        /* 37  */
    "VIP0_MULT_PORTA_SRC0",         /* 38  */
    "VIP0_MULT_PORTA_SRC1",         /* 39  */
    "VIP0_MULT_PORTA_SRC2",         /* 40  */
    "VIP0_MULT_PORTA_SRC3",         /* 41  */
    "VIP0_MULT_PORTA_SRC4",         /* 42  */
    "VIP0_MULT_PORTA_SRC5",         /* 43  */
    "VIP0_MULT_PORTA_SRC6",         /* 44  */
    "VIP0_MULT_PORTA_SRC7",         /* 45  */
    "VIP0_MULT_PORTA_SRC8",         /* 46  */
    "VIP0_MULT_PORTA_SRC9",         /* 47  */
    "VIP0_MULT_PORTA_SRC10",        /* 48  */
    "VIP0_MULT_PORTA_SRC11",        /* 49  */
    "VIP0_MULT_PORTA_SRC12",        /* 50  */
    "VIP0_MULT_PORTA_SRC13",        /* 51  */
    "VIP0_MULT_PORTA_SRC14",        /* 52  */
    "VIP0_MULT_PORTA_SRC15",        /* 53  */
    "VIP0_MULT_PORTB_SRC0",         /* 54  */
    "VIP0_MULT_PORTB_SRC1",         /* 55  */
    "VIP0_MULT_PORTB_SRC2",         /* 56  */
    "VIP0_MULT_PORTB_SRC3",         /* 57  */
    "VIP0_MULT_PORTB_SRC4",         /* 58  */
    "VIP0_MULT_PORTB_SRC5",         /* 59  */
    "VIP0_MULT_PORTB_SRC6",         /* 60  */
    "VIP0_MULT_PORTB_SRC7",         /* 61  */
    "VIP0_MULT_PORTB_SRC8",         /* 62  */
    "VIP0_MULT_PORTB_SRC9",         /* 63  */
    "VIP0_MULT_PORTB_SRC10",        /* 64  */
    "VIP0_MULT_PORTB_SRC11",        /* 65  */
    "VIP0_MULT_PORTB_SRC12",        /* 66  */
    "VIP0_MULT_PORTB_SRC13",        /* 67  */
    "VIP0_MULT_PORTB_SRC14",        /* 68  */
    "VIP0_MULT_PORTB_SRC15",        /* 69  */
    "VIP0_MULT_ANCA_SRC0",          /* 70  */
    "VIP0_MULT_ANCA_SRC1",          /* 71  */
    "VIP0_MULT_ANCA_SRC2",          /* 72  */
    "VIP0_MULT_ANCA_SRC3",          /* 73  */
    "VIP0_MULT_ANCA_SRC4",          /* 74  */
    "VIP0_MULT_ANCA_SRC5",          /* 75  */
    "VIP0_MULT_ANCA_SRC6",          /* 76  */
    "VIP0_MULT_ANCA_SRC7",          /* 77  */
    "VIP0_MULT_ANCA_SRC8",          /* 78  */
    "VIP0_MULT_ANCA_SRC9",          /* 79  */
    "VIP0_MULT_ANCA_SRC10",         /* 80  */
    "VIP0_MULT_ANCA_SRC11",         /* 81  */
    "VIP0_MULT_ANCA_SRC12",         /* 82  */
    "VIP0_MULT_ANCA_SRC13",         /* 83  */
    "VIP0_MULT_ANCA_SRC14",         /* 84  */
    "VIP0_MULT_ANCA_SRC15",         /* 85  */
    "VIP0_MULT_ANCB_SRC0",          /* 86  */
    "VIP0_MULT_ANCB_SRC1",          /* 87  */
    "VIP0_MULT_ANCB_SRC2",          /* 88  */
    "VIP0_MULT_ANCB_SRC3",          /* 89  */
    "VIP0_MULT_ANCB_SRC4",          /* 90  */
    "VIP0_MULT_ANCB_SRC5",          /* 91  */
    "VIP0_MULT_ANCB_SRC6",          /* 92  */
    "VIP0_MULT_ANCB_SRC7",          /* 93  */
    "VIP0_MULT_ANCB_SRC8",          /* 94  */
    "VIP0_MULT_ANCB_SRC9",          /* 95  */
    "VIP0_MULT_ANCB_SRC10",         /* 96  */
    "VIP0_MULT_ANCB_SRC11",         /* 97  */
    "VIP0_MULT_ANCB_SRC12",         /* 98  */
    "VIP0_MULT_ANCB_SRC13",         /* 99  */
    "VIP0_MULT_ANCB_SRC14",         /* 100 */
    "VIP0_MULT_ANCB_SRC15",         /* 101 */
    "VIP0_PORTA_LUMA",              /* 102 */
    "VIP0_PORTA_CHROMA",            /* 103 */
    "VIP0_PORTB_LUMA",              /* 104 */
    "VIP0_PORTB_CHROMA",            /* 105 */
    "VIP0_PORTA_RGB",               /* 106 */
    "VIP0_PORTB_RGB",               /* 107 */
    "VIP1_MULT_PORTA_SRC0",         /* 108 */
    "VIP1_MULT_PORTA_SRC1",         /* 109 */
    "VIP1_MULT_PORTA_SRC2",         /* 110 */
    "VIP1_MULT_PORTA_SRC3",         /* 111 */
    "VIP1_MULT_PORTA_SRC4",         /* 112 */
    "VIP1_MULT_PORTA_SRC5",         /* 113 */
    "VIP1_MULT_PORTA_SRC6",         /* 114 */
    "VIP1_MULT_PORTA_SRC7",         /* 115 */
    "VIP1_MULT_PORTA_SRC8",         /* 116 */
    "VIP1_MULT_PORTA_SRC9",         /* 117 */
    "VIP1_MULT_PORTA_SRC10",        /* 118 */
    "VIP1_MULT_PORTA_SRC11",        /* 119 */
    "VIP1_MULT_PORTA_SRC12",        /* 120 */
    "VIP1_MULT_PORTA_SRC13",        /* 121 */
    "VIP1_MULT_PORTA_SRC14",        /* 122 */
    "VIP1_MULT_PORTA_SRC15",        /* 123 */
    "VIP1_MULT_PORTB_SRC0",         /* 124 */
    "VIP1_MULT_PORTB_SRC1",         /* 125 */
    "VIP1_MULT_PORTB_SRC2",         /* 126 */
    "VIP1_MULT_PORTB_SRC3",         /* 127 */
    "VIP1_MULT_PORTB_SRC4",         /* 128 */
    "VIP1_MULT_PORTB_SRC5",         /* 129 */
    "VIP1_MULT_PORTB_SRC6",         /* 130 */
    "VIP1_MULT_PORTB_SRC7",         /* 131 */
    "VIP1_MULT_PORTB_SRC8",         /* 132 */
    "VIP1_MULT_PORTB_SRC9",         /* 133 */
    "VIP1_MULT_PORTB_SRC10",        /* 134 */
    "VIP1_MULT_PORTB_SRC11",        /* 135 */
    "VIP1_MULT_PORTB_SRC12",        /* 136 */
    "VIP1_MULT_PORTB_SRC13",        /* 137 */
    "VIP1_MULT_PORTB_SRC14",        /* 138 */
    "VIP1_MULT_PORTB_SRC15",        /* 139 */
    "VIP1_MULT_ANCA_SRC0",          /* 140 */
    "VIP1_MULT_ANCA_SRC1",          /* 141 */
    "VIP1_MULT_ANCA_SRC2",          /* 142 */
    "VIP1_MULT_ANCA_SRC3",          /* 143 */
    "VIP1_MULT_ANCA_SRC4",          /* 144 */
    "VIP1_MULT_ANCA_SRC5",          /* 145 */
    "VIP1_MULT_ANCA_SRC6",          /* 146 */
    "VIP1_MULT_ANCA_SRC7",          /* 147 */
    "VIP1_MULT_ANCA_SRC8",          /* 148 */
    "VIP1_MULT_ANCA_SRC9",          /* 149 */
    "VIP1_MULT_ANCA_SRC10",         /* 150 */
    "VIP1_MULT_ANCA_SRC11",         /* 151 */
    "VIP1_MULT_ANCA_SRC12",         /* 152 */
    "VIP1_MULT_ANCA_SRC13",         /* 153 */
    "VIP1_MULT_ANCA_SRC14",         /* 154 */
    "VIP1_MULT_ANCA_SRC15",         /* 155 */
    "VIP1_MULT_ANCB_SRC0",          /* 156 */
    "VIP1_MULT_ANCB_SRC1",          /* 157 */
    "VIP1_MULT_ANCB_SRC2",          /* 158 */
    "VIP1_MULT_ANCB_SRC3",          /* 159 */
    "VIP1_MULT_ANCB_SRC4",          /* 160 */
    "VIP1_MULT_ANCB_SRC5",          /* 161 */
    "VIP1_MULT_ANCB_SRC6",          /* 162 */
    "VIP1_MULT_ANCB_SRC7",          /* 163 */
    "VIP1_MULT_ANCB_SRC8",          /* 164 */
    "VIP1_MULT_ANCB_SRC9",          /* 165 */
    "VIP1_MULT_ANCB_SRC10",         /* 166 */
    "VIP1_MULT_ANCB_SRC11",         /* 167 */
    "VIP1_MULT_ANCB_SRC12",         /* 168 */
    "VIP1_MULT_ANCB_SRC13",         /* 169 */
    "VIP1_MULT_ANCB_SRC14",         /* 170 */
    "VIP1_MULT_ANCB_SRC15",         /* 171 */
    "VIP1_PORTA_LUMA",              /* 172 */
    "VIP1_PORTA_CHROMA",            /* 173 */
    "VIP1_PORTB_LUMA",              /* 174 */
    "VIP1_PORTB_CHROMA",            /* 175 */
    "VIP1_PORTA_RGB",               /* 176 */
    "VIP1_PORTB_RGB",               /* 177 */
    "NF_RD",                        /* 178 */
    "NF_WR_LUMA",                   /* 179 */
    "NF_WR_CHROMA",                 /* 180 */
    "NF_LAST_LUMA",                 /* 181 */
    "NF_LAST_CHROMA",               /* 182 */
    "INVALID_VPDMA_CH [Warning]",   /* 183 */
    "VBI_SD_VENC",                  /* 184 */
    "WB2",                          /* 185 */
    "BP0",                          /* 186 */
    "BP1",                          /* 187 */
    "SEC0_LUMA",                    /* 188 */
    "SEC0_CHROMA",                  /* 189 */
    "SEC1_LUMA",                    /* 190 */
    "SEC1_CHROMA"                   /* 191 */
};
#endif

/* VPDMA free channel name. */
static char *gVpsVpdmaHalFreeChStr = "FREE_CHANNEL";

/* Invalid string. */
static char *gVpsVpdmaHalInvalidStr = "INVALID [Warning]";

/* VPDMA data type names. */
static char *gVpsVpdmaHalDataTypeStr[] =
{
    "Y444_RGB565",
    "Y422_ARGB1555",
    "Y420_ARGB4444",
    "YC420_RGBA5551",
    "C444_RGBA4444",
    "C422_ARGB6666",
    "C420_RGB888",
    "YUYV422_ARGB8888",
    "YC444_RGBA6666",
    "RGBA8888",
    "INVALID [Warning]",
    "INVALID [Warning]",
    "INVALID [Warning]",
    "INVALID [Warning]",
    "INVALID [Warning]",
    "INVALID [Warning]",
    "BGR565",
    "ABGR1555",
    "ABGR4444",
    "BGRA5551",
    "BGRA4444",
    "ABGR6666",
    "BGR888",
    "YVYU422_ABGR8888",
    "BGRA6666",
    "BGRA8888",
};

/* VPDMA bitmap data type names. */
static char *gVpsVpdmaHalBmpDataTypeStr[] =
{
    "BITMAP8",
    "INVALID [Warning]",
    "BITMAP4_LOWER",
    "BITMAP4_UPPER",
    "BITMAP2_OFFSET0",
    "BITMAP2_OFFSET1",
    "BITMAP2_OFFSET2",
    "UYVY422_BITMAP2_OFFSET3",
    "BITMAP1_OFFSET0",
    "BITMAP1_OFFSET1",
    "BITMAP1_OFFSET2",
    "BITMAP1_OFFSET3",
    "BITMAP1_OFFSET4",
    "BITMAP1_OFFSET5",
    "BITMAP1_OFFSET6",
    "BITMAP1_OFFSET7",
    "BITMAP8_BGRA32",
    "INVALID [Warning]",
    "BITMAP4_LOWER_BGRA32",
    "BITMAP4_UPPER_BGRA32",
    "BITMAP2_OFFSET0_BGRA32",
    "BITMAP2_OFFSET1_BGRA32",
    "BITMAP2_OFFSET2_BGRA32",
    "VYUY422_BITMAP2_OFFSET3_BGRA32",
    "BITMAP1_OFFSET0_BGRA32",
    "BITMAP1_OFFSET1_BGRA32",
    "BITMAP1_OFFSET2_BGRA32",
    "BITMAP1_OFFSET3_BGRA32",
    "BITMAP1_OFFSET4_BGRA32",
    "BITMAP1_OFFSET5_BGRA32",
    "BITMAP1_OFFSET6_BGRA32",
    "BITMAP1_OFFSET7_BGRA32",
};

/* VPDMA MV data type name. */
static char *gVpsVpdmaHalMvDataTypeStr = "MV";

/* VPDMA stencil data type name. */
static char *gVpsVpdmaHalStenDataTypeStr = "STENCIL";

/* VPDMA CLUT data type name. */
static char *gVpsVpdmaHalClutDataTypeStr = "CLUT";

/* VPDMA ancillary data type name. */
static char *gVpsVpdmaHalAncDataTypeStr = "ANC";

/* VPDMA FID names. */
static char *gVpsVpdmaHalFidStr[] =
{
    "EVEN",
    "ODD"
};

/* VPDMA memory type names. */
static char *gVpsVpdmaHalMemTypeStr[] =
{
    "NON_TILED",
    "TILED"
};

/* VPDMA configuration class names. */
static char *gVpsVpdmaHalClassStr[] =
{
    "ADDR_DATA_SET",
    "BLOCK_SET"
};

/* VPDMA configuration destination names. */
static char *gVpsVpdmaHalDestStr[] =
{
    "MMR",
    "SC_GRPX0",
    "SC_GRPX1",
    "SC_GRPX2",
    "SC1",
    "SC2",
    "SC5",
    "SC3",
    "SC4"
};

/* VPDMA SOC event names. */
static char *gVpsVpdmaHalSocEvtStr[] =
{
    "EOF",
    "SOF",
    "EOL",
    "SOL",
    "PIXEL",
    "ACTIVE",
    "NOTACTIVE",
    "FID_CHANGE01",
    "FID_CHANGE10",
    "EOEL"
};

/* VPDMA LM FID control names. */
static char *gVpsVpdmaHalLmFidCtrlStr[] =
{
    "UNCHANGED",
    "TOGGLE",
    "CHANGE_0",
    "CHANGE_1"
};

/* VPDMA frame start event names. */
static char *gVpsVpdmaFsEventStr[] =
{
    "HDMI_FID",
    "DVO2_FID",
    "HDCOMP_FID",
    "SD_FID",
    "LM_FID0",
    "LM_FID1",
    "LM_FID2",
    "CH_ACTIVE"
};


/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */

char *VpsHal_vpdmaGetChStr(UInt32 channel);
char *VpsHal_vpdmaGetFsEventStr(UInt32 fsEvent);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _VPSHAL_VPDMA_DEBUG_H */
