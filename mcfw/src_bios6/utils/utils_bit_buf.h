/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
    \ingroup UTILS_API
    \defgroup UTILS_BITBUF_API Bit buffer exchange API

    APIs defined in this file are used by links-and-chains example to exchange
    buffers between two tasks

    Internally this consists of two queues
    - empty or input queue
    - full or output queue

    The queue implementation uses fixed size array based queue data structure,
    with mutual exclusion built inside the queue implementation.

    Optional blocking of Get and/or Put operation is possible

    The element that can be inserted/extracted from the queue is of
    type Bitstream_Buf *

    The basic operation in the example is as below

    - When a producer task needs to output some data, it first 'gets' a empty buf
      to output the data from the buffer handle.
    - The task outputs the data to the empty buf
    - The task then 'puts' this data as full data into the buffer handle
    - The consumer task, then 'gets' this full buf from the buffer handle
    - After using or consuming this buf, it 'puts' this buf as empty buf into
      this buffer handle.
    - This way buf are exchanged between a producer and consumer.

    @{
*/

/**
    \file utils_bit_buf.h
    \brief buffer exchange API
*/

#ifndef _UTILS_BIT_BUF_H_
#define _UTILS_BIT_BUF_H_

#include <mcfw/src_bios6/utils/utils_buf.h>
#include <mcfw/interfaces/link_api/vidbitstream.h>

#define UTILS_BITBUF_MAX_ALLOC_POOLS     (5)

/**
    \brief Bit Buffer Handle
*/
typedef struct Utils_BitBufHndl {
    UInt32 numAllocPools;
    /**< Number of allocator pools configured */
    Utils_QueHandle emptyQue[UTILS_BITBUF_MAX_ALLOC_POOLS];
    /**< Empty or input queue */
    Utils_QueHandle fullQue;
    /**< Full or output queue */
     Bitstream_Buf
        * emptyQueMem[UTILS_BITBUF_MAX_ALLOC_POOLS][UTILS_BUF_MAX_QUE_SIZE];
    /**< Memory for empty que data - bitstreamBuf */
    Bitstream_Buf *fullQueMem[UTILS_BUF_MAX_QUE_SIZE];
    /**< Memory for empty que data - bitstreamBuf */
} Utils_BitBufHndl;

/**
    \brief Create a bit buffer handle

    When blockOnGet/blockOnPut is TRUE a semaphore gets allocated internally.
    In order to reduce resource usuage keep this as FALSE if application
    doesnt plan to use the blocking API feature.

    \param pHndl          [OUT] Created handle
    \param blockOnGet     [IN]  Enable blocking on 'get' API
    \param blockOnPut     [IN]  Enable blocking on 'put' API
    \param numAllocPools  [IN]  Number of allocator pools to create

    \return FVID2_SOK on success, else failure
*/
Int32 Utils_bitbufCreate(Utils_BitBufHndl * pHndl,
                         Bool blockOnGet,
                         Bool blockOnPut, UInt32 numAllocPools);

/**
    \brief Delete bit buffer handle

    Free's resources like semaphore allocated during create

    \param pHndl    [IN] Buffer handle

    \return FVID2_SOK on success, else failure
*/
Int32 Utils_bitbufDelete(Utils_BitBufHndl * pHndl);


/**
    \brief Print buffer status of full & empty queues

    \param str            [IN] prefix string to print
    \param pHndl        [IN] Buffer handle

    \return None
*/
Void Utils_bitbufPrintStatus(UInt8 *str, Utils_BitBufHndl * pHndl);

/**
    \brief Get bit's from empty queue

    This API is used to get multiple bufs in a single API call.
    Bitstream_BufList.numbufs is set to number of bufs
    that are returned.

    When during create
    - 'blockOnGet' = TRUE
      - timeout can be BIOS_WAIT_FOREVER or BIOS_NO_WAIT
    - 'blockOnGet' = FALSE
      - timeout must be BIOS_NO_WAIT

    \param pHndl        [IN] Buffer handle
    \param pBufList   [OUT] buf's returned by the API
    \param timeout      [IN] BIOS_NO_WAIT or BIOS_WAIT_FOREVER

    \return FVID2_SOK on success, else failure
*/
Int32 Utils_bitbufGetEmpty(Utils_BitBufHndl * pHndl,
                           Bitstream_BufList * pBufList,
                           UInt32 allocPoolID, UInt32 timeout);

/**
    \brief Put buf's into full queue

    This API is used to return multiple bufs in a single API call.
    Bitstream_BufList.numbufs is set to number of bufs
    that are to be returned.

    When during create
    - 'blockOnPut' = TRUE
      - API will block until space is available in the queue to put the bufs
    - 'blockOnPut' = FALSE
      - API will return error in case space is not available in the queue
        to put the bufs

    \param pHndl        [IN] Buffer handle
    \param pBufList   [IN] buf's to be put
    \param timeout      [IN] BIOS_NO_WAIT or BIOS_WAIT_FOREVER

    \return FVID2_SOK on success, else failure
*/
Int32 Utils_bitbufPutFull(Utils_BitBufHndl * pHndl,
                          Bitstream_BufList * pBufList);

/**
    \brief Get buf's from full queue

    This API is used to get multiple buf's in a single API call.
    Bitstream_BufList.numbufs is set to number of bufs
    that are returned.

    When during create
    - 'blockOnGet' = TRUE
      - timeout can be BIOS_WAIT_FOREVER or BIOS_NO_WAIT
    - 'blockOnGet' = FALSE
      - timeout must be BIOS_NO_WAIT

    \param pHndl        [IN] Buffer handle
    \param pBufList   [OUT] buf's returned by the API
    \param timeout      [IN] BIOS_NO_WAIT or BIOS_WAIT_FOREVER

    \return FVID2_SOK on success, else failure
*/
Int32 Utils_bitbufGetFull(Utils_BitBufHndl * pHndl,
                          Bitstream_BufList * pBufList, UInt32 timeout);

/**
    \brief Put buf's into empty queue

    This API is used to return multiple bufs in a single API call.
    Bitstream_BufList.numbufs is set to number of bufs
    that are to be returned.

    When during create
    - 'blockOnPut' = TRUE
      - API will block until space is available in the queue to put the bufs
    - 'blockOnPut' = FALSE
      - API will return error in case space is not available in the queue
        to put the bufs

    \param pHndl        [IN] Buffer handle
    \param pBufList   [IN] buf's to be put
    \param timeout      [IN] BIOS_NO_WAIT or BIOS_WAIT_FOREVER

    \return FVID2_SOK on success, else failure
*/
Int32 Utils_bitbufPutEmpty(Utils_BitBufHndl * pHndl,
                           Bitstream_BufList * pBufList);

/**
    \brief Get a buf from empty queue

    Same as Utils_bitbufGetEmpty() except that only a single buf is returned

    \param pHndl        [IN] Buffer handle
    \param pBuf       [OUT] buf that is returned by the API
    \param timeout      [IN] BIOS_NO_WAIT or BIOS_WAIT_FOREVER

    \return FVID2_SOK on success, else failure
*/
Int32 Utils_bitbufGetEmptyBuf(Utils_BitBufHndl * pHndl,
                              Bitstream_Buf ** pBuf,
                              UInt32 allocPoolID, UInt32 timeout);

/**
    \brief Get a buf from full queue

    Same as Utils_bitbufGetFull() except that only a single buf is returned

    \param pHndl        [IN] Buffer handle
    \param pBuf       [OUT] buf that is returned by the API
    \param timeout      [IN] BIOS_NO_WAIT or BIOS_WAIT_FOREVER

    \return FVID2_SOK on success, else failure
*/
Int32 Utils_bitbufGetFullBuf(Utils_BitBufHndl * pHndl,
                             Bitstream_Buf ** pBuf, UInt32 timeout);

/**
    \brief Put a buf into full queue

    Same as Utils_bitbufPutFull() except that only a single buf is put

    \param pHndl        [IN] Buffer handle
    \param pBuf       [OUT] buf that is to be returned to the queue
    \param timeout      [IN] BIOS_NO_WAIT or BIOS_WAIT_FOREVER

    \return FVID2_SOK on success, else failure
*/
Int32 Utils_bitbufPutFullBuf(Utils_BitBufHndl * pHndl, Bitstream_Buf * pBuf);

/**
    \brief Put a buf into empty queue

    Same as Utils_bitbufPutEmpty() except that only a single buf is put

    \param pHndl        [IN] Buffer handle
    \param pBuf       [OUT] buf that is to be returned to the queue
    \param timeout      [IN] BIOS_NO_WAIT or BIOS_WAIT_FOREVER

    \return FVID2_SOK on success, else failure
*/
Int32 Utils_bitbufPutEmptyBuf(Utils_BitBufHndl * pHndl, Bitstream_Buf * pBuf);

/**
    \brief Peek into empty queue

    This only peeks at the top of the queue but does not remove the
    buf from the queue

    \param pHndl        [IN] Buffer handle

    \param allocPoolID  [IN] Allocator pool ID to peek

    \return buf pointer is buf is present in the queue, else NULL
*/
static inline Bitstream_Buf *Utils_bitbufPeekEmpty(Utils_BitBufHndl * pHndl,
                                                   UInt32 allocPoolID)
{
    Bitstream_Buf *pBuf;

    UTILS_assert(allocPoolID < UTILS_BITBUF_MAX_ALLOC_POOLS);
    Utils_quePeek(&pHndl->emptyQue[allocPoolID], (Ptr *) & pBuf);

    return pBuf;
}

/**
    \brief Peek into full queue

    This only peeks at the top of the queue but does not remove the
    buf from the queue

    \param pHndl        [IN] Buffer handle

    \return buf pointer is buf is present in the queue, else NULL
*/
static inline Bitstream_Buf *Utils_bitbufPeekFull(Utils_BitBufHndl * pHndl)
{
    Bitstream_Buf *pBuf;

    Utils_quePeek(&pHndl->fullQue, (Ptr *) & pBuf);

    return pBuf;
}

#endif

/* @} */

/**
    \defgroup EXAMPLE_API Sample Example API

    The API defined in this module are utility APIs OUTSIDE of the FVID2 drivers.

    Example code makes use of these APIs to implement sample application which
    demonstrate the driver in different ways.
*/

/**
    \ingroup EXAMPLE_API
    \defgroup UTILS_API Sample Example - Utility library API
*/
