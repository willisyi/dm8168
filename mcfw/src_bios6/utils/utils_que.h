/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
  \file utils_que.h
  \brief Utils layer - Array based queue
*/

#ifndef _UTILS_QUE_H_
#define _UTILS_QUE_H_

#include <mcfw/src_bios6/utils/utils.h>

/**
  \ingroup _DRV_UTIL_API
  \defgroup _DRV_UTIL_AQUE_API Utils layer - Array based queue

  Implementation of a array based queue with support for optional
  blocking on queue empty or queue full.
  Interally takes care of critical section locking so that it can be
  used from task, ISR context without any additional mutex logic.

  @{
*/

/** Do not block on que get and que put,
  returns error if que is empty or full respectively
*/
#define UTILS_QUE_FLAG_NO_BLOCK_QUE    (0x00000000)

/** Block on que put if que is full */
#define UTILS_QUE_FLAG_BLOCK_QUE_PUT   (0x00000001)

/** Block on que get if que is empty */
#define UTILS_QUE_FLAG_BLOCK_QUE_GET   (0x00000002)

/** Block on que put if que is full, Block on que get if que is empty  */
#define UTILS_QUE_FLAG_BLOCK_QUE       (0x00000003)

/**
  \brief Queue Handle

  Typically user does not need to know internals of queue handle
  data structure
*/
typedef struct {
    UInt32 curRd;
  /**< Current read index */

    UInt32 curWr;
  /**< Current write index  */

    UInt32 count;
  /**< Count of element in queue  */

    UInt32 maxElements;
  /**< Max elements that be present in the queue  */

    Ptr *queue;
  /**< Address of data area of the queue elements */

    Semaphore_Handle semRd;
  /**< Read semaphore */

    Semaphore_Struct semRdMem;
  /**< Memory for read semaphore */

    Semaphore_Handle semWr;
  /**< Write semaphore  */

    Semaphore_Struct semWrMem;
  /**< Memory for write semaphore */

    UInt32 flags;
  /**< Controls how APIs behave internally,
    i.e blocking wait or non-blocking */

    volatile Bool blockedOnGet;
  /**< Flag indicating queue is blocked on get operation */

    volatile Bool blockedOnPut;
   /**< Flag indicating queue is blocked on put operation */

    volatile Bool forceUnblockGet;
   /**< Flag indicating forced unblock of queueGet */

    volatile Bool forceUnblockPut;
   /**< Flag indicating forced unblock of queuePut */

} Utils_QueHandle;

/**
  \brief Create a queue handle

  The size of queueMem llocate by the user should be maxElements*sizeof(Ptr)

  \param  handle        [ O] Initialized queue handle
  \param  maxElements   [I ] Maximum elements that can reside in the queue
                          at any given point of time
  \param  queueMem      [I ] Address of queue element data area
  \param  flags         [I ] UTILS_QUE_FLAG_xxxx

  \return 0 on success, else failure
*/
Int32 Utils_queCreate(Utils_QueHandle * handle,
                      UInt32 maxElements, Ptr queueMem, UInt32 flags);

/**
  \brief Delete queue handle

  Releases all resources allocated during queue create

  \param  handle        [I ] Queue handle

  \return 0 on success, else failure
*/
Int32 Utils_queDelete(Utils_QueHandle * handle);

/**
  \brief Add a element into the queue

  \param handle   [I ] Queue Handle
  \param data     [I ] data element to insert
  \param timeout  [I ] BIOS_NO_WAIT: non-blocking,
                        if queue is full error is returned \n
                       BIOS_WAIT_FOREVER: Blocking,
                        if queue is full function blocks until
                        atleast one element in the queue is free
                        for inserting new element

  \return 0 on success, else failure
*/
Int32 Utils_quePut(Utils_QueHandle * handle, Ptr data, Int32 timeout);

/**
  \brief Get a element from the queue

  \param handle   [I ] Queue Handle
  \param data     [ O] extracted data element from the queue
  \param minCount [I ] Data will be extracted only if
                        atleast 'minCount' elements are present in the queue
  \param timeout  [I ] BIOS_NO_WAIT: non-blocking,
                       if queue is empty error is returned \n
                       BIOS_WAIT_FOREVER: Blocking, if queue is
                       empty function blocks until
                       atleast 'minCount' elemetns in the queue are available

  \return 0 on success, else failure
*/
Int32 Utils_queGet(Utils_QueHandle * handle,
                   Ptr * data, UInt32 minCount, Int32 timeout);

/**
  \brief Peek at the first element from the queue, but do not extract it

  \param handle   [I ] Queue Handle
  \param data     [ O] "peeked" data element from the queue

  \return 0 on success, else failure
*/
Int32 Utils_quePeek(Utils_QueHandle * handle, Ptr * data);

/**
  \brief Returns TRUE is queue is empty else retunrs false

  \param handle   [I ] Queue Handle

  \return Returns TRUE is queue is empty else retunrs FALSE
*/
UInt32 Utils_queIsEmpty(Utils_QueHandle * handle);

/**
  \brief Returns number of elements queued

  \param handle   [I ] Queue Handle

  \return Returns number of elements queued
*/
UInt32 Utils_queGetQueuedCount(Utils_QueHandle * handle);

/**
  \brief Force unblock of queue.If any entity was waiting on queue get/queue put
         it will get unblocked with error code indicating queue was unblocked

  \param handle   [I ] Queue Handle

  \return 0 on success, else failure
*/
UInt32 Utils_queUnBlock(Utils_QueHandle * handle);

/**
  \brief Returns TRUE is queue is full else retunrs false

  \param handle   [I ] Queue Handle

  \return Returns TRUE is queue is full else retunrs FALSE
*/
UInt32 Utils_queIsFull(Utils_QueHandle * handle);

/**
  \brief Reset the Queue elements

  \param handle   [I ] Queue Handle

  \return 0 on success, else failure
*/
UInt32 Utils_queReset(Utils_QueHandle * handle);

#endif

/* @} */
