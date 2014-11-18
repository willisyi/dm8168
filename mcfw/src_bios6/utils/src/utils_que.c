/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include <mcfw/src_bios6/utils/utils_que.h>

/* See utils_que.h for function documentation */

Int32 Utils_queCreate(Utils_QueHandle * handle,
                      UInt32 maxElements, Ptr queueMem, UInt32 flags)
{
    Semaphore_Params semParams;

    /*
     * init handle to 0's
     */
    memset(handle, 0, sizeof(*handle));

    /*
     * init handle with user parameters
     */
    handle->maxElements = maxElements;
    handle->flags = flags;

    /*
     * queue data element memory cannot be NULL
     */
    UTILS_assert(queueMem != NULL);

    handle->queue = queueMem;

    if (handle->flags & UTILS_QUE_FLAG_BLOCK_QUE_GET)
    {
        /*
         * user requested block on que get
         */

        /*
         * create semaphore for it
         */

        Semaphore_Params_init(&semParams);

        Semaphore_construct(&handle->semRdMem, 0, &semParams);

        handle->semRd = Semaphore_handle(&handle->semRdMem);

        UTILS_assert(handle->semRd != NULL);
    }

    if (handle->flags & UTILS_QUE_FLAG_BLOCK_QUE_PUT)
    {
        /*
         * user requested block on que put
         */

        /*
         * create semaphore for it
         */

        Semaphore_Params_init(&semParams);

        Semaphore_construct(&handle->semWrMem, 0, &semParams);

        handle->semWr = Semaphore_handle(&handle->semWrMem);

        UTILS_assert(handle->semWr != NULL);
    }
    handle->blockedOnGet = FALSE;
    handle->blockedOnPut = FALSE;
    handle->forceUnblockGet = FALSE;
    handle->forceUnblockPut = FALSE;

    return 0;
}

Int32 Utils_queDelete(Utils_QueHandle * handle)
{
    if (handle->flags & UTILS_QUE_FLAG_BLOCK_QUE_GET)
    {
        /*
         * user requested block on que get
         */

        /*
         * delete associated semaphore
         */

        Semaphore_destruct(&handle->semRdMem);

        handle->semRd = NULL;
    }
    if (handle->flags & UTILS_QUE_FLAG_BLOCK_QUE_PUT)
    {
        /*
         * user requested block on que put
         */

        /*
         * delete associated semaphore
         */

        Semaphore_destruct(&handle->semWrMem);

        handle->semWr = NULL;
    }

    return 0;
}

Int32 Utils_quePut(Utils_QueHandle * handle, Ptr data, Int32 timeout)
{
    Int32 status = -1;                                     /* init status to
                                                            * error */
    UInt32 cookie;

    do
    {
        /*
         * disable interrupts
         */
        cookie = Hwi_disable();
	// Vps_printf("handle->count : %d  ,handle->maxElements: %d \r\n",handle->count,handle->maxElements);
        if (handle->count < handle->maxElements)
        {
            /*
             * free space available in que
             */

            /*
             * insert element
             */
            handle->queue[handle->curWr] = data;

            /*
             * increment put pointer
             */
            handle->curWr = (handle->curWr + 1) % handle->maxElements;

            /*
             * increment count of number element in que
             */
            handle->count++;
	    // if(handle->count == handle->maxElements)
	    //  handle->count=0;


            /*
             * restore interrupts
             */
            Hwi_restore(cookie);

            /*
             * mark status as success
             */
            status = 0;

            if (handle->flags & UTILS_QUE_FLAG_BLOCK_QUE_GET)
            {
                /*
                 * blocking on que get enabled
                 */

                /*
                 * post semaphore to unblock, blocked tasks
                 */
                Semaphore_post(handle->semRd);
            }

            /*
             * exit, with success
             */
            break;

        }
        else
        {
	  //  Vps_printf(" que is full\r\n");
	  //  Vps_printf("handle->count : %d  ,handle->maxElements: %d \r\n",handle->count,handle->maxElements);
            /*
             * que is full
             */

            /*
             * restore interrupts
             */
            Hwi_restore(cookie);

            if (timeout == BIOS_NO_WAIT)
                break;                                     /* non-blocking
                                                            * function call,
                                                            * exit with error
                                                            */

            if (handle->flags & UTILS_QUE_FLAG_BLOCK_QUE_PUT)
            {
                Bool semPendStatus;

                /*
                 * blocking on que put enabled
                 */

                /*
                 * take semaphore and block until timeout occurs or
                 * semaphore is posted
                 */
                handle->blockedOnPut = TRUE;
                semPendStatus = Semaphore_pend(handle->semWr, timeout);
                handle->blockedOnPut = FALSE;
                if (!semPendStatus || handle->forceUnblockPut)
                {
                    handle->forceUnblockPut = FALSE;
                    Vps_printf("forceunblockput false \r\n");
                    break;                                 /* timeout
                                                            * happend, exit
                                                            * with error */
                }
                /*
                 * received semaphore, recheck for available space in the que
                 */
            }
            else
            {
                /*
                 * blocking on que put disabled
                 */

                /*
                 * exit with error
                 */
	      Vps_printf("blocking on que put disalbe\r\n");
                break;
            }
        }
    }
    while (1);

    return status;
}

Int32 Utils_queGet(Utils_QueHandle * handle, Ptr * data,
                   UInt32 minCount, Int32 timeout)
{
    Int32 status = -1;                                     /* init status to
                                                            * error */
    UInt32 cookie;

    /*
     * adjust minCount between 1 and handle->maxElements
     */
    if (minCount == 0)
        minCount = 1;
    if (minCount > handle->maxElements)
        minCount = handle->maxElements;

    do
    {
        /*
         * disable interrupts
         */
        cookie = Hwi_disable();

        if (handle->count >= minCount)
        {
            /*
             * data elements available in que is >=
             * minimum data elements requested by user
             */

            /*
             * extract the element
             */
            *data = handle->queue[handle->curRd];

            /*
             * increment get pointer
             */
            handle->curRd = (handle->curRd + 1) % handle->maxElements;

            /*
             * decrmeent number of elements in que
             */
            handle->count--;

            /*
             * restore interrupts
             */
            Hwi_restore(cookie);

            /*
             * set status as success
             */
            status = 0;

            if (handle->flags & UTILS_QUE_FLAG_BLOCK_QUE_PUT)
            {
                /*
                 * blocking on que put enabled
                 */

                /*
                 * post semaphore to unblock, blocked tasks
                 */
                Semaphore_post(handle->semWr);
            }

            /*
             * exit with success
             */
            break;

        }
        else
        {
            /*
             * no elements or not enough element (minCount) in que to extract
             */

            /*
             * restore interrupts
             */
            Hwi_restore(cookie);

            if (timeout == BIOS_NO_WAIT)
                break;                                     /* non-blocking
                                                            * function call,
                                                            * exit with error
                                                            */

            if (handle->flags & UTILS_QUE_FLAG_BLOCK_QUE_GET)
            {
                Bool semPendStatus;

                /*
                 * blocking on que get enabled
                 */

                /*
                 * take semaphore and block until timeout occurs or
                 * semaphore is posted
                 */

                handle->blockedOnGet = TRUE;
                semPendStatus = Semaphore_pend(handle->semRd, timeout);
                handle->blockedOnGet = FALSE;
                if (!semPendStatus || (handle->forceUnblockGet == TRUE))
                {
                    handle->forceUnblockGet = FALSE;
                    break;                                 /* timeout
                                                            * happened, exit
                                                            * with error */
                }

                /*
                 * received semaphore, check que again
                 */
            }
            else
            {
                /*
                 * blocking on que get disabled
                 */

                /*
                 * exit with error
                 */
                break;
            }
        }
    }
    while (1);

    return status;
}

UInt32 Utils_queIsEmpty(Utils_QueHandle * handle)
{
    UInt32 isEmpty;
    UInt32 cookie;

    /*
     * disable interrupts
     */
    cookie = Hwi_disable();

    /*
     * check if que is empty
     */
    if (handle->count)
        isEmpty = FALSE;                                   /* not empty */
    else
        isEmpty = TRUE;                                    /* empty */

    /*
     * restore interrupts
     */
    Hwi_restore(cookie);

    return isEmpty;
}

Int32 Utils_quePeek(Utils_QueHandle * handle, Ptr * data)
{
    Int32 status = -1;                                     /* init status as
                                                            * error */
    UInt32 cookie;

    *data = NULL;

    /*
     * disable interrupts
     */
    cookie = Hwi_disable();

    if (handle->count)
    {
        /*
         * que is not empty
         */

        /*
         * get value of top element, but do not extract it from que
         */
        *data = handle->queue[handle->curRd];

        /*
         * set status as success
         */
        status = 0;
    }

    /*
     * restore interrupts
     */
    Hwi_restore(cookie);

    return status;
}

UInt32 Utils_queGetQueuedCount(Utils_QueHandle * handle)
{
    UInt32 count;
    UInt32 cookie;

    /*
     * disable interrupts
     */
    cookie = Hwi_disable();

    count = handle->count;

    /*
     * restore interrupts
     */
    Hwi_restore(cookie);

    return count;
}

UInt32 Utils_queUnBlock(Utils_QueHandle * handle)
{
    UInt tskKey, hwiKey;

    hwiKey = Hwi_disable();
    /* lock task scheduler */
    tskKey = Task_disable();

    if (handle->flags & UTILS_QUE_FLAG_BLOCK_QUE_GET)
    {
        if (handle->semRd)
        {
            if ((Semaphore_getCount(handle->semRd) == 0)
                && (TRUE == handle->blockedOnGet))
            {
                handle->forceUnblockGet = TRUE;
                Semaphore_post(handle->semRd);
            }
        }
    }

    if (handle->flags & UTILS_QUE_FLAG_BLOCK_QUE_PUT)
    {
        if (handle->semWr)
        {
            if ((Semaphore_getCount(handle->semWr) == 0)
                && (TRUE == handle->blockedOnPut))
            {
                handle->forceUnblockPut = TRUE;
                Semaphore_post(handle->semWr);
            }
        }
    }

    Hwi_restore(hwiKey);

    Task_restore(tskKey);
    return 0;
}

UInt32 Utils_queIsFull(Utils_QueHandle * handle)
{
    UInt32 isFull;
    UInt32 cookie;

    /*
     * disable interrupts
     */
    cookie = Hwi_disable();

    /*
     * check if que is empty
     */
    if (handle->count < handle->maxElements)
        isFull = FALSE;                                    /* not full */
    else
        isFull = TRUE;                                     /* full */

    /*
     * restore interrupts
     */
    Hwi_restore(cookie);

    return isFull;
}


UInt32 Utils_queReset(Utils_QueHandle * handle)
{
    UInt32 cookie;
    Int32 status = 0;
    /*
     * disable interrupts
     */
    cookie = Hwi_disable();

    /*
     * Reset the queue
     */
    handle->count = 0;
    handle->curRd = 0;
    handle->curWr = 0;
    handle->blockedOnGet = FALSE;
    handle->blockedOnPut = FALSE;
    handle->forceUnblockGet = FALSE;
    handle->forceUnblockPut = FALSE;

    /*
     * restore interrupts
     */
    Hwi_restore(cookie);

    return status;
}

