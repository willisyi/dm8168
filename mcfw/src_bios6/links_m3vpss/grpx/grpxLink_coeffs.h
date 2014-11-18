
#ifndef _GRPX_LINK_COEFFS_H_
#define _GRPX_LINK_COEFFS_H_

#include "grpxLink_priv.h"

/** \brief GRPX Scalar coefficients for  Polyphase horizontal up scalar
 *  These coefficients are used for horizontal up scale only 5taps/8phases*/
#define GRPX_SC_HORIZONTAL_UP_SCALE_COEFF                      \
{                                                               \
    /*1.x - 8.x*/                                               \
    {0x03f6, 0x03fa, 0x03fd, 0x3fff, 0x0000, 0x0000, 0x0000, 0x0000},   \
    {0x0089, 0x005d, 0x0034, 0x0015, 0x0000, 0x03f5, 0x03f1, 0x03f2},   \
    {0x008a, 0x00b6, 0x00dd, 0x00f6, 0x00ff, 0x00f6, 0x00dd, 0x00b6},   \
    {0x03f6, 0x03f2, 0x03f1, 0x03f5, 0x0000, 0x0015, 0x0034, 0x005d},   \
    {0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x03ff, 0x03fd, 0x03fa}    \
}                                                               \

/** \brief GRPX Scalar coefficients for  Polyphase vertical up scalar
 *  These coefficients are used for vertical up scale only 4taps/8phases*/
#define GRPX_SC_VERTICAL_UP_SCALE_COEFF                        \
{                                                               \
    /* 1.x - 8.x*/                                              \
    {0x0000, 0x03fa, 0x03f8, 0x03fa, 0x03fd, 0x03ff, 0x0000, 0x0000},   \
    {0x00ff, 0x00f6, 0x00dc, 0x00b3, 0x0082, 0x0053, 0x002b, 0x000f},   \
    {0x0000, 0x000f, 0x002b, 0x0053, 0x0083, 0x00b3, 0x00dc, 0x00f6},   \
    {0x0000, 0x0000, 0x0000, 0x03ff, 0x03fd, 0x03fa, 0x03f8, 0x03fa}    \
}                                                               \


/** \brief GRPX Scalar coefficients for polyphase horizontal  anti-flicker
* This is bypass filter
*/
#define GRPX_SC_HORIZONTAL_AF_COEFF                                       \
{                                                                     \
    {0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000}, \
    {0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000}, \
    {0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000}, \
    {0x0100, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000}, \
    {0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000}  \
}                                                                     \


/** \brief GRPX Scalar coefficients for polyphase vertical  anti-flicker
* this is 0.5 downscaling coeff
*/
#define GRPX_SC_VERTICAL_AF_COEFF                   \
{                                                   \
    {0x0026, 0x0018, 0x000e, 0x0007, 0x0003, 0x0001, 0x0000, 0x0000},      \
    {0x00b3, 0x00af, 0x00a4, 0x0093, 0x007e, 0x0064, 0x004d, 0x0038},      \
    {0x0026, 0x0038, 0x004d, 0x0064, 0x007b, 0x0093, 0x00a4, 0x00af},      \
    {0x0000, 0x0000, 0x0000, 0x0001, 0x0003, 0x0007, 0x000e, 0x0018}       \
}                                                   \

/** \brief GRPX Scalar coefficients for  Polyphase horizontal down scalar
 *  These coefficients are used for vertical up scale only */
#define GRPX_SC_HORIZONTAL_DOWN_SCALE_COEFF                         \
{                                                                   \
    {                                                               \
        /* horizontal decimation [ 0.25 - 0.375) use 0.25 coeff*/                  \
        {0x0014, 0x000e, 0x0009, 0x0005, 0x0003, 0x0001, 0x0000, 0x0000},   \
        {0x006c, 0x0060, 0x0054, 0x0047, 0x003b, 0x002f, 0x0025, 0x001c},   \
        {0x006b, 0x0075, 0x007d, 0x0083, 0x0083, 0x0083, 0x007d, 0x0075},   \
        {0x0014, 0x001c, 0x0025, 0x002f, 0x003b, 0x0047, 0x0054, 0x0060},   \
        {0x0000, 0x0000, 0x0000, 0x0001, 0x0003, 0x0005, 0x0009, 0x000e}    \
    },                                                              \
    {                                                               \
        /* horizontal decimation [ 0.375 - 0.5), use 0.4 coeff*/                   \
        {0x000e, 0x0009, 0x0005, 0x0002, 0x0001, 0x0000, 0x0000, 0x0000},   \
        {0x0072, 0x0063, 0x0054, 0x0045, 0x0036, 0x002a, 0x001e, 0x0015},   \
        {0x0071, 0x007e, 0x0088, 0x008e, 0x0091, 0x008e, 0x0088, 0x007e},   \
        {0x000e, 0x0015, 0x001e, 0x002a, 0x0036, 0x0045, 0x0054, 0x0063},   \
        {0x0000, 0x0000, 0x0000, 0x0000, 0x0001, 0x0002, 0x0005, 0x0009}    \
    },                                                              \
    {                                                               \
        /* horizontal decimation [ 0.5 - 0.75), use 0.6 coeff*/                    \
        {0x0003, 0x0000, 0x0000, 0x03ff, 0x03ff, 0x0000, 0x0000, 0x0000},   \
        {0x007e, 0x0067, 0x0051, 0x003d, 0x002b, 0x001c, 0x0011, 0x0009},   \
        {0x007b, 0x008f, 0x009d, 0x00a7, 0x00ab, 0x00a7, 0x009d, 0x008f},   \
        {0x0003, 0x0009, 0x0011, 0x001c, 0x002b, 0x003d, 0x0051, 0x0067},   \
        {0x0000, 0x0000, 0x0000, 0x0000, 0x03ff, 0x03ff, 0x0000, 0x0000}    \
    },                                                              \
    {                                                               \
        /* horizontal decimation [ 0.75 - 1.0), use 0.75 coeff*/                    \
        {0x03fc, 0x03fc, 0x03fc, 0x03fe, 0x03ff, 0x0000, 0x0000, 0x0000},   \
        {0x0084, 0x0067, 0x004b, 0x0032, 0x001e, 0x000e, 0x0004, 0x03ff},   \
        {0x0083, 0x009d, 0x00b4, 0x00c1, 0x00c5, 0x00c1, 0x00b4, 0x009d},   \
        {0x03fc, 0x03ff, 0x0004, 0x000e, 0x001e, 0x0032, 0x004b, 0x0067},   \
        {0x0000, 0x0000, 0x0000, 0x0000, 0x03ff, 0x03fe, 0x03fc, 0x03fc}    \
    },                                                      \
    {                                                               \
        /* CUSTOM - horizontal decimation [ 0.375 - 0.5), use 0.4 coeff*/                   \
        {0x000e, 0x0009, 0x0005, 0x0002, 0x0001, 0x0000, 0x0000, 0x0000},   \
        {0x0072, 0x0063, 0x0054, 0x0045, 0x0036, 0x002a, 0x001e, 0x0015},   \
        {0x0071, 0x007e, 0x0088, 0x008e, 0x0091, 0x008e, 0x0088, 0x007e},   \
        {0x000e, 0x0015, 0x001e, 0x002a, 0x0036, 0x0045, 0x0054, 0x0063},   \
        {0x0000, 0x0000, 0x0000, 0x0000, 0x0001, 0x0002, 0x0005, 0x0009}    \
    },                                                              \
                                                               \
}                                                                   \

 /** \brief GRPX Scalar coefficients for  Polyphase vertical down scalar
 *  These coefficients are used for vertical up scale only */
#define GRPX_SC_VERTICAL_DOWN_SCALE_COEFF                           \
{                                                                   \
    {                                                               \
        /* vertical decimation [ 0.25 - 0.375) use 0.25 coeff*/                    \
        {0x0030, 0x0022, 0x0017, 0x000e, 0x0008, 0x0004, 0x0001, 0x0000}, \
        {0x009f, 0x009d, 0x0095, 0x0088, 0x0078, 0x0065, 0x0052, 0x0040}, \
        {0x0030, 0x0040, 0x0052, 0x0065, 0x0077, 0x0088, 0x0095, 0x009d}, \
        {0x0000, 0x0000, 0x0001, 0x0004, 0x0008, 0x000e, 0x0017, 0x0022}  \
    },                                                              \
    {                                                               \
        /* vertical decimation [ 0.375 - 0.5) use 0.4 coeff*/                     \
        {0x002b, 0x001d, 0x0012, 0x000a, 0x0005, 0x0002, 0x0000, 0x0000},   \
        {0x00a9, 0x00a6, 0x009d, 0x008e, 0x007c, 0x0065, 0x0050, 0x003c},   \
        {0x002b, 0x003c, 0x0050, 0x0065, 0x0079, 0x008e, 0x009d, 0x00a6},   \
        {0x0000, 0x0000, 0x0000, 0x0002, 0x0005, 0x000a, 0x0012, 0x001d}    \
    },                                                              \
    {                                                               \
        /* vertical decimation [ 0.5 - 0.75) use 0.6 coeff*/                      \
        {0x0020, 0x0012, 0x0009, 0x0004, 0x0001, 0x0000, 0x0000, 0x0000},      \
        {0x00bf, 0x00bb, 0x00ad, 0x0098, 0x007f, 0x0063, 0x0049, 0x0032},      \
        {0x0020, 0x0032, 0x0049, 0x0063, 0x007e, 0x0098, 0x00ad, 0x00bb},      \
        {0x0000, 0x0000, 0x0000, 0x0000, 0x0001, 0x0004, 0x0009, 0x0012}       \
    },                                                              \
    {                                                               \
        /* vertical decimation [ 0.75 -1.0), use 0.75 coeff*/                       \
        {0x0015, 0x0009, 0x0002, 0x0000, 0x03ff, 0x03ff, 0x0000, 0x0000},   \
        {0x00d5, 0x00ce, 0x00bc, 0x00a1, 0x0080, 0x005f, 0x0041, 0x0028},   \
        {0x0015, 0x0028, 0x0041, 0x005f, 0x0081, 0x00a1, 0x00bc, 0x00ce},   \
        {0x0000, 0x0000, 0x0000, 0x03ff, 0x03ff, 0x0000, 0x0002, 0x0009}    \
    },                                                              \
    {   \
        /* CUSTOM */                \
        {0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40},   \
        {0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40},   \
        {0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40},   \
        {0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40}    \
    }                                                               \
}                                                                   \



/**
 *  Grpx_ScaledSet
 *
 */
typedef enum
{
    GRPX_SC_DS_SET_0 = 0,
    /**< Coefficient for the down sampling (1/4) =< Factor < (3/8). */
    GRPX_SC_DS_SET_1,
    /**< Coefficient for the down sampling (3/8) =< Factor < (1/2). */
    GRPX_SC_DS_SET_2,
    /**< Coefficient for the down sampling (1/2) =< Factor < (3/4). */
    GRPX_SC_DS_SET_3,
    /**< Coefficient for the down sampling (3/4) =< Factor < (1). */
    GRPX_SC_AF,
    /**< Coefficient for the down sampling (3/4) =< Factor < (1). */
    GRPX_SC_CUSTOM,
    /** <Coefficient for anti-flicker case */
    GRPX_SC_US_SET
    /** < Coefficient for up sample 1.0 -> 4.0x */
}Grpx_ScScaleSet;

/** \brief horizontal up-scaling coefficients*/
static UInt16 GrpxScHorzUpScaleCoeff[5][8] =
                    GRPX_SC_HORIZONTAL_UP_SCALE_COEFF   ;
/** \brief verticl up-scaling coefficients*/
static UInt16 GrpxScVertUpScaleCoeff[4][8] =
                    GRPX_SC_VERTICAL_UP_SCALE_COEFF;
/** \brief horizontal down-scaling coefficients*/
static UInt16 GrpxScHorzDownScaleCoeff[5][5][8] =
                    GRPX_SC_HORIZONTAL_DOWN_SCALE_COEFF;
/** \brief vertical down-scaling coefficients*/
static UInt16 GrpxScVertDownScaleCoeff[5][4][8] =
                    GRPX_SC_VERTICAL_DOWN_SCALE_COEFF;
/** \brief horizontal anti-flicker coefficients */
static UInt16 GrpxScHorzAFCoeff[5][8] =
                    GRPX_SC_HORIZONTAL_AF_COEFF;
/** \brief vertical anti-flicker coefficients */
static UInt16 GrpxScVertAFCoeff[4][8] =
                    GRPX_SC_VERTICAL_AF_COEFF;

#endif
