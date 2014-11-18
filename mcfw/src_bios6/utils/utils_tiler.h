/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
    \ingroup UTILS_API
    \defgroup UTILS_TILER_API Tiler allocator API
    @{
*/

#ifndef __TILER_H_
#define __TILER_H_

#include <mcfw/src_bios6/utils/utils_mem.h>

#define UTILS_TILER_CONTAINER_MAXSIZE                (128*MB)
#define UTILS_TILER_INVALID_ADDR                     (~(0u))

#define UTILS_TILER_CPU_VIRT_ADDR  (0x60000000)

#define UTILS_TILER_CNT_FIRST  (0)
#define UTILS_TILER_CNT_8BIT   (UTILS_TILER_CNT_FIRST)
#define UTILS_TILER_CNT_16BIT  (1)
#define UTILS_TILER_CNT_32BIT  (2)
#define UTILS_TILER_CNT_LAST   (UTILS_TILER_CNT_32BIT)
#define UTILS_TILER_CNT_MAX    (UTILS_TILER_CNT_LAST + 1)

#define UTILS_TILER_COPY_TO_DDR       (0)
#define UTILS_TILER_COPY_FROM_DDR     (1)

#define UTILS_TILER_ORI_NONE     (0x00u)
#define UTILS_TILER_ORI_X_FLIP   (0x01u)
#define UTILS_TILER_ORI_Y_FLIP   (0x02u)
#define UTILS_TILER_ORI_XY_SWAP  (0x04u)

#define UTILS_TILER_CNT_8BIT_MAX_LINES   (8192u)
#define UTILS_TILER_CNT_16BIT_MAX_LINES  (4096u)
#define UTILS_TILER_CNT_32BIT_MAX_LINES  (4096u)

#define UTILS_TILER_ORI_MODE_SHIFT   (29u)
#define UTILS_TILER_ORI_MODE_MASK    (0x07u << UTILS_TILER_ORI_MODE_SHIFT)

#define UTILS_TILER_GET_CNT_MODE(tilerAddr) (((tilerAddr) >> 27) & 0x3)
#define UTILS_TILER_PUT_CNT_MODE(tilerAddr, cntMode) (((tilerAddr) | (((cntMode) & 0x3) << 27)))

/*
 * Convert tilerAddr to CPU addr
 *
 * tilerAddr - address got via Utils_tilerAlloc
 *
 * return CPU virtual address */
UInt32 Utils_tilerAddr2CpuAddr(UInt32 tilerAddr);

/**
  \brief Allocate a frame in tiler space

  Use FVID2_Format to allocate a frame.
  Fill FVID2_Frame fields like channelNum based on FVID2_Format

  \param  pFormat   [IN] Data format information
  \param  pFrame    [OUT] Initialzed FVID2_Frame structure
  \param  numFrames [IN] Number of frames to allocate

  \return 0 on sucess, else failure
*/
Int32 Utils_tilerFrameAlloc(FVID2_Format * pFormat,
                            FVID2_Frame * pFrame, UInt16 numFrames);

/**
  \brief Copy between tiler Address space and non-tiler DDR address

  This API internal converts tilerAddr to cpuAddr

  \param dir        [IN] UTILS_TILER_COPY_TO_DDR or UTILS_TILER_COPY_FROM_DDR
  \param tilerAddr  [IN] tiler address returned during Utils_tilerAlloc() or Utils_tilerGetAddr()
  \param dataWidth  [IN] data width in bytes
  \param dataHeight [IN] data height in lines
  \param ddrAddr    [IN] Non tiled DDR address
  \param ddrPitch   [IN] Pitch to be used for data in non-tiled space in bytes

  \return 0 on sucess, else failure
*/
Int32 Utils_tilerCopy(UInt32 dir, UInt32 tilerAddr, UInt32 dataWidth,
                      UInt32 dataHeight, UInt8 * ddrAddr, UInt32 ddrPitch);

/**
  \brief Get tiler Addr

  \param cntMode [IN] container mode
  \param startX [IN] X-coordinate in BYTES
  \param startY [IN] Y-coordinate in LINES

  \return tiler address
*/
UInt32 Utils_tilerGetAddr(UInt32 cntMode, UInt32 startX, UInt32 startY);

/**
 *  \brief Get tiler address after applying the orientation.
 *
 *  \param tilerAddr    [IN] 0 degree tiler address returned during
 *                           Utils_tilerAlloc()
 *  \param cntMode      [IN] Container mode.
 *  \param oriFlag      [IN] Orientation flag representing S, Y', X' bits
 *  \param width        [IN] Buffer width
 *  \param width        [IN] Buffer width
 *
 *  \return Tiler address after applying the necessary orientation.
*/
UInt32 Utils_tilerGetOriAddr(UInt32 tilerAddr,
                             UInt32 cntMode,
                             UInt32 oriFlag, UInt32 width, UInt32 height);

#endif

/* @} */
