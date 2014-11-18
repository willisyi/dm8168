/* ======================================================================== */
/*  TEXAS INSTRUMENTS, INC.                                                 */
/*                                                                          */
/*  CPIS layer prototypes                                                   */
/*                                                                          */
/*  This library contains proprietary intellectual property of Texas        */
/*  Instruments, Inc.  The library and its source code are protected by     */
/*  various copyrights, and portions may also be protected by patents or    */
/*  other legal protections.                                                */
/*                                                                          */
/*  This software is licensed for use with Texas Instruments TMS320         */
/*  family DSPs.  This license was provided to you prior to installing      */
/*  the software.  You may review this license by consulting the file       */
/*  TI_license.PDF which accompanies the files in this library.             */
/*                                                                          */
/* ------------------------------------------------------------------------ */
/*                                                                          */
/*     NAME                                                                 */
/*        cpisCore.h --                                                     */
/*                                                                          */
/*     DESCRIPTION                                                          */
/*        This file includes the definitions and the interfaces supported   */
/*        by the core functions belonging to the                            */
/*        CoProcessor InfraStructure (CPIS)'s core library                  */
/*                                                                          */
/*     REV                                                                  */
/*        version 0.1  6 June , 2011                                        */
/*        Initial version                                                   */
/*                                                                          */
/* ------------------------------------------------------------------------ */
/*            Copyright (c) 2011 Texas Instruments, Incorporated.           */
/*                           All Rights Reserved.                           */
/* ======================================================================== */

#ifndef _CPISCORE_H
#define _CPISCORE_H

#ifdef __cplusplus
    extern "C" {
#endif

#include <ti/psp/iss/alg/evf/inc/tistdtypes.h>

#ifdef _4MACS

#ifdef _SIMCOP

#define CPIS_MAX_SRC_CHANNELS 12
#define CPIS_MAX_DST_CHANNELS 12

/*
CPIS_MAX_CHANNELS does not have to be necessarily equal to
CPIS_MAX_SRC_CHANNELS + CPIS_MAX_DST_CHANNELS
It represents the number of total channels allocatable
by the vicp library, including source an destination channels
hence you can have a combination of: 8 SRC channels + 2 DST channels
or 2 SRC channels + 8 DST channels
*/

#define CPIS_MAX_CHANNELS     12

typedef struct {
    void *src;
    void *dst;
    Uint16 numCols;
    Uint16 numRows;
    Uint32 strideSrc;
    Uint32 strideDst;
    Uint16 chan;
} CPIS_Dma2dCopy;

Int32 CPIS_setupDma2dCopy(Uint16 chan, CPIS_Dma2dCopy *dma2dCopy);
Int32 CPIS_updateAddrDma2dCopy(Uint16 chan, CPIS_Dma2dCopy *dma2dCopy);
Int32 CPIS_dma2dCopyStartChan(Uint16 chan);
Int32 CPIS_dma2dCopyWaitChan(Uint16 chan);

#else

#define CPIS_MAX_SRC_CHANNELS 8
#define CPIS_MAX_DST_CHANNELS 6

/* 
CPIS_MAX_CHANNELS does not have to be necessarily equal to 
CPIS_MAX_SRC_CHANNELS + CPIS_MAX_DST_CHANNELS
It represents the number of total channels allocatable
by the vicp library, including source an destination channels
hence you can have a combination of: 8 SRC channels + 2 DST channels
or 2 SRC channels + 8 DST channels
*/

#define CPIS_MAX_CHANNELS     14

#endif

#else

#define CPIS_MAX_SRC_CHANNELS 4
#define CPIS_MAX_DST_CHANNELS 4

/* On DM64x/DM644x
CPIS_MAX_CHANNELS must be equal to CPIS_MAX_SRC_CHANNELS + CPIS_MAX_DST_CHANNELS
*/
#define CPIS_MAX_CHANNELS     (CPIS_MAX_SRC_CHANNELS + CPIS_MAX_DST_CHANNELS)

#endif

/* Error symbols used by the library */
#define CPIS_INIT_ERROR                 1
#define CPIS_NOTINIT_ERROR              2
#define CPIS_UNPACK_ERROR               3
#define CPIS_NOSUPPORTFORMAT_ERROR      4
#define CPIS_NOSUPPORTDIM_ERROR         5
#define CPIS_PACK_ERROR                 6
#define CPIS_MAXNUMFUNCREACHED          7
#define CPIS_OUTOFMEM                   8
#define CPIS_NOSUPPORTANGLE_ERROR       9
#define CPIS_NOSUPPORTOP_ERROR          10
#define CPIS_NOT_ENOUGH_EDMACHAN_ERROR  11
#define CPIS_ALGO_REGISTRATION_ERROR    12

/* 
    Maximum processing block size for different functions. 
    For a given <functionName>: 
    procBlockSize.width x procBlockSize.height < MAX_<functionName>_BLOCKSIZE
*/
#ifdef _4MACS
    #define PLAT_DIV 2
#else
    #define PLAT_DIV 1
#endif

extern Uint32 CPIS_errno;

/* Various data formats supported by the library */
typedef enum {
    CPIS_YUV_420P=0, /* Planar symbols must be listed first */
    CPIS_YUV_422P,
    CPIS_YUV_444P,
    CPIS_YUV_411P,
    CPIS_YUV_422VP,  /* Vertical subsampling */
    CPIS_RGB_P,
    CPIS_BAYER_P,
    CPIS_YUV_422IBE,
    CPIS_YUV_422ILE,
    CPIS_RGB_555,
    CPIS_RGB_565,
    CPIS_BAYER,
    CPIS_YUV_444IBE,
    CPIS_YUV_444ILE,
    CPIS_RGB_888,
    CPIS_YUV_GRAY,
    CPIS_8BIT,
    CPIS_16BIT,
    CPIS_32BIT,
    CPIS_64BIT,
    CPIS_U8BIT,
    CPIS_U16BIT,
    CPIS_U32BIT,
    CPIS_U64BIT,
    CPIS_1BIT,
    CPIS_YUV_420SP,
    CPIS_COMPLEX_8BIT,
    CPIS_COMPLEX_16BIT,
    CPIS_COMPLEX_32BIT,
    CPIS_COMPLEX_U8BIT,
    CPIS_COMPLEX_U16BIT,
    CPIS_COMPLEX_U32BIT,
    CPIS_ALPHA_TYPE=0x8000
} CPIS_Format;


typedef enum {
    CPIS_DS_NONE,
    CPIS_DS_SKIP,
    CPIS_DS_AVERAGE,
    CPIS_DS_FOR_ALPHABLEND
} CPIS_ColorDsMode;

#define CPIS_ALPHA 0x8000
#define CPIS_ALPHA_SEPARATE_PLANE 0x4000
#define CPIS_FOREGROUND_ALPHA_ARGB888_PLANE 0x2000
#define CPIS_BACKGROUND_ALPHA_ARGB888_PLANE 0x1000

/* The various arithmetic and logical operations supported by the library */
typedef enum {
    CPIS_OP_MPY=0, 
    CPIS_OP_ABDF,     
    CPIS_OP_ADD,     
    CPIS_OP_SUB,     
    CPIS_OP_TLU,     
    CPIS_OP_AND,     
    CPIS_OP_OR,      
    CPIS_OP_XOR,     
    CPIS_OP_MIN,     
    CPIS_OP_MAX, 
    CPIS_OP_MINSAD,     /* Only available on _8MACS platform */  
    CPIS_OP_MAXSAD,     /* Only available on _8MACS platform */
    CPIS_OP_MEDIAN,     /* Only available on _8MACS platform */
    CPIS_OP_BINLOG,     /* Only available on _8MACS platform */
    CPIS_OP_3DLUT,      /* Only available on _8MACS platform */
    CPIS_OP_CONDWR,     /* Only available on _8MACS platform */
    CPIS_OP_NOT         /* Only available on _4MACS platform */  
} CPIS_Operation;

/* Enum that controls the synchronous or async operation of the library APIs */
typedef enum {
    CPIS_SYNC,
    CPIS_ASYNC
} CPIS_ExecType;

/* Initialization structure for the library */
typedef void (*CacheWbInv)(void *, Uint32, Bool);
typedef void (*VicpLock)(void*);
typedef void (*VicpUnlock)(void*);

#define CPIS_INIT_CE                1
#define CPIS_INIT_RMAN              (1<<1)
#define CPIS_INIT_VICP_IRES         (1<<2)
#define CPIS_INIT_EDMA3_IRES        (1<<3)
#define CPIS_INIT_ADDRSPACE_IRES    (1<<4)
#define CPIS_INIT_FC_ALL            (CPIS_INIT_CE | CPIS_INIT_RMAN | CPIS_INIT_VICP_IRES |CPIS_INIT_EDMA3_IRES | CPIS_INIT_ADDRSPACE_IRES)
#define CPIS_INIT_FC_NONE           0

typedef struct {
    Uint16 maxNumProcFunc;
    void *mem;
    Uint32 memSize;
    CacheWbInv cacheWbInv;
    Uint16 staticDmaAlloc;
    Uint16 initFC;          /* Initialize framework components according to bitmask combination:
                                   CPIS_INIT_CE: initialize CE, need to set member engineName as well
                                   CPIS_INIT_RMAN: initialize RMAN
                                   CPIS_INIT_VICP_IRES: register VICP res manager 
                                   CPIS_INIT_EDMA3_IRES: register EDMA3 res manager
                                   CPIS_INIT_ADDRSPACE_IRES: register ADDRSPACE res manager
                                All these previous symbols can be ORed to form a bitmask.
                                In addition the symbol CPIS_INIT_FC_ALL can be used to initialize
                                all the Framework components.
                                CPIS_INIT_FC_NONE can be used to disable all FC initialization by
                                the VICP library.   
                             */
    char *engineName;        /* Codec engine name, must match name used in vicpLib365.cfg
                                normally, it is "alg_server"  
                                This member is irrelevant if initFC's bit #0 is set to 0   
                             */
    void *codecEngineHandle; /* Codec engine handle */
    VicpLock lock;
    void *lockArg;
    VicpUnlock unlock;
    void *unlockArg;
    Uint16 maxNumDma;       /* Maximum number of EDMA channels that vicplib can use.
                               This allow the application integrator to optimize
                               the EDMA channel usage with the rest of the system */
} CPIS_Init;


/* Structures used to convey information regarding the various blocks */
typedef struct {
    Uint32 width;   /* In number of pixels */
    Uint32 height;  /* In number of lines */
} CPIS_Size;

typedef struct {
    Uint8 *ptr;
    Uint32 stride;  /* In number of pixels */
} CPIS_Buffer;


/* Base paramters common to all APIs */

typedef struct {
    CPIS_Format srcFormat[CPIS_MAX_SRC_CHANNELS];
    CPIS_Buffer srcBuf[CPIS_MAX_SRC_CHANNELS];
    CPIS_Format dstFormat[CPIS_MAX_DST_CHANNELS];
    CPIS_Buffer dstBuf[CPIS_MAX_DST_CHANNELS];
    CPIS_Size roiSize;
    CPIS_Size procBlockSize;
    Uint16 numInput;    /* Number of input buffers */
    Uint16 numOutput;   /* Number of ouput buffers */
} CPIS_BaseParms;

typedef void* CPIS_Handle;

typedef struct {
    Uint32 srcAddr;
    Uint16 blockWidth;
    Uint16 blockHeight;
    Uint32 dstAddr;
    Int16 srcStride;
    Int16 dstStride;
} ScatterGatherEntry;

typedef struct {
    Uint32 numOfROIs;
    ScatterGatherEntry *arrayOfROIs;
} CPIS_ScatterGatherParms;

/* APIs supported by the library */
Int32 CPIS_getMemSize(Uint16 maxNumProc);
Int32 CPIS_init(CPIS_Init *init);
Int32 CPIS_deInit();

Int16 CPIS_sizeof(CPIS_Format format);

Int32 CPIS_start(CPIS_Handle handle);

Int32 CPIS_wait(CPIS_Handle handle);

Int32 CPIS_reset(CPIS_Handle handle);

Int32 CPIS_updateCmdOfst(CPIS_Handle handle, Uint16 cmdOfst);

Int32 CPIS_updateSrcDstPtr(CPIS_Handle handle, CPIS_BaseParms *base);

Int32 CPIS_isBusy();

Int32 CPIS_setWaitCB(Int32 (*waitCB)(void*arg), void* waitCBarg);

Int32 CPIS_setWaitCBArg(void* waitCBarg);

Int32 CPIS_delete(CPIS_Handle handle);

void CPIS_lock(void);

void CPIS_unlock(void);

Int32 CPIS_getBlockXYxyPos(Uint8 *addr, CPIS_BaseParms *base, Uint16 index, Uint16 *X, Uint16 *Y, Uint16 *x, Uint16 *y);

Int32 CPIS_scatterGather(
        CPIS_Handle *handle,
        CPIS_BaseParms *base,
        CPIS_ScatterGatherParms *params,
        CPIS_ExecType execType
);

Int32 CPIS_startScatterGather(CPIS_Handle handle);

Int32 CPIS_waitScatterGather(CPIS_Handle handle);

#ifdef __cplusplus
 }
#endif

#endif /* #define _CPISCORE_H */

/* ======================================================================== */
/*                       End of file                                        */
/* ------------------------------------------------------------------------ */
/*            Copyright (c) 2008 Texas Instruments, Incorporated.           */
/*                           All Rights Reserved.                           */
/* ======================================================================== */
