/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include <mcfw/src_bios6/utils/utils_tiler.h>
#include <mcfw/interfaces/link_api/system_tiler.h>
#include <mcfw/interfaces/link_api/system_debug.h>

UInt32 Utils_tilerAddr2CpuAddr(UInt32 tilerAddr)
{
    UInt32 cpuAddr, cntMode;

    cntMode = UTILS_TILER_GET_CNT_MODE(tilerAddr);

    cpuAddr =
        UTILS_TILER_CPU_VIRT_ADDR + UTILS_TILER_CONTAINER_MAXSIZE * cntMode;
    cpuAddr += (tilerAddr & 0x07FFFFFF);

#ifdef SYSTEM_DEBUG_TILER
    {
        Vps_printf(" [TILER] Tiler Addr = 0x%08x, CPU Addr = 0x%08x\n",
                   tilerAddr, cpuAddr);
    }
#endif

    return cpuAddr;
}

Int32 Utils_tilerGetMaxPitch(UInt32 cntMode, UInt32 * maxPitch)
{
    *maxPitch = 0;

    switch (cntMode)
    {
        case UTILS_TILER_CNT_8BIT:
            *maxPitch = VPSUTILS_TILER_CNT_8BIT_PITCH;
            break;
        case UTILS_TILER_CNT_16BIT:
            *maxPitch = VPSUTILS_TILER_CNT_16BIT_PITCH;
            break;
        case UTILS_TILER_CNT_32BIT:
            *maxPitch = VPSUTILS_TILER_CNT_32BIT_PITCH;
            break;
        default:
            return -1;
    }

    return 0;
}

UInt32 Utils_tilerGetOriAddr(UInt32 tilerAddr,
                             UInt32 cntMode,
                             UInt32 oriFlag, UInt32 width, UInt32 height)
{
    UInt32 oriAddr;
    UInt32 hOffset, vOffset;
    UInt32 hStride, vStride;

    /* Get the base address without orientation and container modes */
    oriAddr = tilerAddr;
    oriAddr &= ~(0x1Fu << 27u);
    oriFlag &= (UTILS_TILER_ORI_X_FLIP |
                UTILS_TILER_ORI_Y_FLIP | UTILS_TILER_ORI_XY_SWAP);

    /* Figure out horizontal stride and max lines as per container mode */
    if (UTILS_TILER_CNT_8BIT == cntMode)
    {
        hStride = VPSUTILS_TILER_CNT_8BIT_PITCH;
        vStride = UTILS_TILER_CNT_8BIT_MAX_LINES;
    }
    else if (UTILS_TILER_CNT_16BIT == cntMode)
    {
        hStride = VPSUTILS_TILER_CNT_16BIT_PITCH;
        vStride = UTILS_TILER_CNT_16BIT_MAX_LINES;
    }
    else
    {
        hStride = VPSUTILS_TILER_CNT_32BIT_PITCH;
        vStride = UTILS_TILER_CNT_32BIT_MAX_LINES;
    }

    /* Calculate X' address */
    if (oriFlag & UTILS_TILER_ORI_X_FLIP)
    {
        hOffset = oriAddr & (hStride - 1u);
        oriAddr &= ~(hStride - 1u);
        UTILS_assert((hStride > (hOffset + width)));
        oriAddr += hStride - (hOffset + width);
    }

    /* Calculate Y' address */
    if (oriFlag & UTILS_TILER_ORI_Y_FLIP)
    {
        hOffset = oriAddr & (hStride - 1u);
        vOffset = (oriAddr / hStride);
        UTILS_assert((vStride > (vOffset + height)));
        oriAddr = (vStride - (vOffset + height)) * hStride;
        oriAddr += hOffset;
    }

    /* Set the orientation modes */
    oriAddr &= ~UTILS_TILER_ORI_MODE_MASK;
    oriAddr |= (oriFlag << UTILS_TILER_ORI_MODE_SHIFT);

    /* Set the container mode */
    oriAddr = UTILS_TILER_PUT_CNT_MODE(oriAddr, cntMode);

    return (oriAddr);
}

extern void System_memPrintHeapStatus();

Int32 Utils_tilerFrameAlloc(FVID2_Format * pFormat,
                            FVID2_Frame * pFrame, UInt16 numFrames)
{
    UInt32 frameId;

    /* align height to multiple of 2 */
    pFormat->height = VpsUtils_align(pFormat->height, 2);

    for (frameId = 0; frameId < numFrames; frameId++)
    {
        /* init FVID2_Frame to 0's */
        memset(pFrame, 0, sizeof(*pFrame));

        /* copy channelNum to FVID2_Frame from FVID2_Format */
        pFrame->channelNum = pFormat->channelNum;

        switch (pFormat->dataFormat)
        {
            case FVID2_DF_YUV422SP_UV:

                /* Y plane */
              
                pFrame->addr[0][0] =
                    (Ptr) SystemTiler_alloc(SYSTEM_TILER_CNT_8BIT,
                                            pFormat->width, pFormat->height);
                UTILS_assert((UInt32)(pFrame->addr[0][0]) != SYSTEM_TILER_INVALID_ADDR);
                /* C plane */
                
                pFrame->addr[0][1] =
                    (Ptr) SystemTiler_alloc(SYSTEM_TILER_CNT_16BIT,
                                            pFormat->width, pFormat->height);
                UTILS_assert((UInt32)(pFrame->addr[0][1]) != SYSTEM_TILER_INVALID_ADDR);
                break;
            case FVID2_DF_YUV420SP_UV:

                /* Y plane */

                pFrame->addr[0][0] =
                    (Ptr) SystemTiler_alloc(SYSTEM_TILER_CNT_8BIT,
                                            pFormat->width, pFormat->height);
                if ((UInt32)pFrame->addr[0][0] == SYSTEM_TILER_INVALID_ADDR)
                    System_memPrintHeapStatus();

                UTILS_assert((UInt32)(pFrame->addr[0][0]) != SYSTEM_TILER_INVALID_ADDR);
                /* C plane */
             
                pFrame->addr[0][1] =
                    (Ptr) SystemTiler_alloc(SYSTEM_TILER_CNT_16BIT,
                                            pFormat->width,
                                            pFormat->height / 2);
                if ((UInt32)pFrame->addr[0][1] == SYSTEM_TILER_INVALID_ADDR)
                    System_memPrintHeapStatus();
                UTILS_assert((UInt32)(pFrame->addr[0][1]) != SYSTEM_TILER_INVALID_ADDR);
                break;
            default:
                UTILS_assert(0);
                break;

        }

        pFrame++;
    }

    return 0;
}

Int32 Utils_tilerCopy(UInt32 dir, UInt32 tilerAddr, UInt32 dataWidth,
                      UInt32 dataHeight, UInt8 * ddrAddr, UInt32 ddrPitch)
{
    UInt32 cntMode;
    UInt32 tilerCpuAddr, tilerPitch;
    UInt32 inc;
    UInt32 dstAddrBase, dstAddr, dstPitch;
    UInt32 srcAddrBase, srcAddr, srcPitch;
    UInt32 h, w;

    cntMode = UTILS_TILER_GET_CNT_MODE(tilerAddr);
    tilerCpuAddr = Utils_tilerAddr2CpuAddr(tilerAddr);

    Utils_tilerGetMaxPitch(cntMode, &tilerPitch);

    inc = 1 << cntMode;

    if (dir == UTILS_TILER_COPY_TO_DDR)
    {
        dstAddrBase = (UInt32) ddrAddr;
        dstPitch = ddrPitch;
        srcAddrBase = tilerCpuAddr;
        srcPitch = tilerPitch;
    }
    else
    {
        srcAddrBase = (UInt32) ddrAddr;
        srcPitch = ddrPitch;
        dstAddrBase = tilerCpuAddr;
        dstPitch = tilerPitch;
    }

    for (h = 0; h < dataHeight; h++)
    {
        dstAddr = dstAddrBase;
        srcAddr = srcAddrBase;

        switch (cntMode)
        {
            case UTILS_TILER_CNT_8BIT:
                for (w = 0; w < dataWidth;
                     w += inc, dstAddr += inc, srcAddr += inc)
                    *(volatile UInt8 *) dstAddr = *(volatile UInt8 *) srcAddr;
                break;
            case UTILS_TILER_CNT_16BIT:
                for (w = 0; w < dataWidth;
                     w += inc, dstAddr += inc, srcAddr += inc)
                    *(volatile UInt16 *) dstAddr = *(volatile UInt16 *) srcAddr;
                break;
            case UTILS_TILER_CNT_32BIT:
                for (w = 0; w < dataWidth;
                     w += inc, dstAddr += inc, srcAddr += inc)
                    *(volatile UInt32 *) dstAddr = *(volatile UInt32 *) srcAddr;
                break;
        }

        dstAddrBase += dstPitch;
        srcAddrBase += srcPitch;
    }

    return 0;
}
