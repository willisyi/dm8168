
/******************************************************************************
* Includes
******************************************************************************/
#include <stdio.h>
#include <pthread.h>			/*for POSIX calls*/
#include <sys/time.h>
#include <errno.h>

#include "osa_event.h"


typedef struct {
    int bSignaled;
	unsigned long eFlags;
    pthread_mutex_t mutex;
    pthread_cond_t  condition;
} OSA_THREAD_EVENT;


/* ========================================================================== */
/**
* @fn OSA_EventCreate function
*
*
*/
/* ========================================================================== */
int OSA_EventCreate(OSA_PTR *pEvents)
{
    int bReturnStatus = OSA_EFAIL;
    OSA_THREAD_EVENT *plEvent = NULL;

    plEvent = (OSA_THREAD_EVENT *)OSA_memAlloc(sizeof(OSA_THREAD_EVENT));

	if(0 == plEvent) {
		bReturnStatus = OSA_EFAIL;
		goto EXIT;
	}
    plEvent->bSignaled = 0;
	plEvent->eFlags = 0;

	if(0 != pthread_mutex_init(&(plEvent->mutex), NULL)){
		/*OSA_ERROR("Event Create:Mutex Init failed !");*/
		goto EXIT;  /*bReturnStatus = TIMM_OSAL_ERR_UNKNOWN*/
	}

	if(0 != pthread_cond_init(&(plEvent->condition), NULL)){
		/*TIMM_OSAL_Error("Event Create:Conditional Variable  Init failed !");*/
		pthread_mutex_destroy(&(plEvent->mutex));
		/*TIMM_OSAL_Free(plEvent);*/
	}
	else {
    	*pEvents = (OSA_PTR)plEvent;
 		bReturnStatus = OSA_SOK;
	}
EXIT:
    if ((OSA_SOK != bReturnStatus) && (0 != plEvent)) {
        OSA_memFree(plEvent);
	}
	return bReturnStatus;
}


/* ========================================================================== */
/**
* @fn TIMM_OSAL_EventDelete function
*
*
*/
/* ========================================================================== */
int OSA_EventDelete(OSA_PTR pEvents)
{
    int bReturnStatus = OSA_SOK;
    OSA_THREAD_EVENT *plEvent = (OSA_THREAD_EVENT *)pEvents;

	if(0 == plEvent){
		bReturnStatus = OSA_EFAIL;
		goto EXIT;
	}

	if(0 != pthread_mutex_lock(&(plEvent->mutex))) {
		/*OSAL_ERROR("Event Delete: Mutex Lock failed !");*/
		bReturnStatus = OSA_EFAIL;
	}
    if(0 != pthread_cond_destroy(&(plEvent->condition))){
		/*OSAL_ERROR("Event Delete: Conditional Variable Destroy failed !");*/
		bReturnStatus = OSA_EFAIL;
	}

	if(0 != pthread_mutex_unlock(&(plEvent->mutex))) {
		/*OSAL_ERROR("Event Delete: Mutex Unlock failed !");*/
		bReturnStatus = OSA_EFAIL;
	}

	if(0 != pthread_mutex_destroy(&(plEvent->mutex))) {
		/*OSAL_ERROR("Event Delete: Mutex Destory failed !");*/
		bReturnStatus = OSA_EFAIL;
	}

    OSA_memFree(plEvent);
EXIT:
	return bReturnStatus;
}


/* ========================================================================== */
/**
* @fn OSA_EventSet function
*
*
*/
/* ========================================================================== */
int OSA_EventSet(OSA_PTR pEvents, unsigned long uEventFlags, OSA_EVENT_OPERATION eOperation)
{
    int bReturnStatus = OSA_EFAIL;
    OSA_THREAD_EVENT *plEvent = (OSA_THREAD_EVENT *)pEvents;

	if(0 == plEvent){
		bReturnStatus = OSA_EFAIL;
		goto EXIT;
	}

	if(0 != pthread_mutex_lock(&(plEvent->mutex))) {
		/*OSAL_ERROR("Event Set: Mutex Lock failed !");*/
		bReturnStatus = OSA_EFAIL;
		goto EXIT;
	}

    switch (eOperation) {
    case OSA_EVENT_AND:
        plEvent->eFlags = plEvent->eFlags & uEventFlags;
		break;
    case OSA_EVENT_OR:
        plEvent->eFlags = plEvent->eFlags | uEventFlags;
		break;
    default:
    	/*OSAL_ERROR("Event Set: Bad eOperation !");*/
        bReturnStatus = OSA_EFAIL;
        pthread_mutex_unlock(&plEvent->mutex);
        goto EXIT;
    }

    plEvent->bSignaled = TRUE;

	if(0 != pthread_cond_signal(&plEvent->condition)) {
		/*OSAL_ERROR("Event Set: Condition Variable Signal failed !");*/
		bReturnStatus = OSA_EFAIL;
		pthread_mutex_unlock(&plEvent->mutex);
		goto EXIT;
	}

	if(0 != pthread_mutex_unlock(&plEvent->mutex)){
		/*OSAL_ERROR("Event Set: Mutex Unlock failed !");*/
		bReturnStatus = OSA_EFAIL;
	}
	else
		bReturnStatus = OSA_SOK;

EXIT:
	return bReturnStatus;


}

/* ========================================================================== */
/**
* @fn OSA_EventRetrieve function
*
*Spurious  wakeups  from  the  pthread_cond_timedwait() or pthread_cond_wait() functions  may  occur.
*
*A representative sequence for using condition variables is shown below
*
*Thread A (Retrieve Events)							|Thread B (Set Events)
*------------------------------------------------------------------------------------------------------------
*1) Do work up to the point where a certain condition 	|1)Do work
*  must occur (such as "count" must reach a specified 	|2)Lock associated mutex
*  value)											|3)Change the value of the global variable
*2) Lock associated mutex and check value of a global 	|  that Thread-A is waiting upon.
*  variable										|4)Check value of the global Thread-A wait
*3) Call pthread_cond_wait() to perform a blocking wait 	|  variable. If it fulfills the desired
*  for signal from Thread-B. Note that a call to 			|  condition, signal Thread-A.
*  pthread_cond_wait() automatically and atomically 		|5)Unlock mutex.
*  unlocks the associated mutex variable so that it can 	|6)Continue 
*  be used by Thread-B.							|
*4) When signalled, wake up. Mutex is automatically and 	|
*  atomically locked.								|
*5) Explicitly unlock mutex							|
*6) Continue										|	
*
* ========================================================================== */
int OSA_EventRetrieve(OSA_PTR pEvents,
                      unsigned long uRequestedEvents,
                      OSA_EVENT_OPERATION eOperation,
                      unsigned long *pRetrievedEvents,
                      unsigned long uTimeOutMsec)
{
	int bReturnStatus = OSA_EFAIL;
	struct timespec timeout;
	struct timeval now;
	unsigned timeout_us;
	unsigned isolatedFlags;
	int status = -1;
	int and_operation;
	OSA_THREAD_EVENT *plEvent = (OSA_THREAD_EVENT *)pEvents;

	if(0 == plEvent){
		bReturnStatus = OSA_EFAIL;
		goto EXIT;
	}

	/* Lock the mutex for access to the eFlags global variable*/
	if(0 != pthread_mutex_lock(&(plEvent->mutex))){
		/*OSAL_ERROR("Event Retrieve: Mutex Lock failed !");*/
		bReturnStatus = OSA_EFAIL;
		goto EXIT;
	}

	/*Check the eOperation and put it in a variable*/
	and_operation = ((OSA_EVENT_AND == eOperation) || (OSA_EVENT_AND_CONSUME ==  eOperation));

	/* Isolate the flags. The & operation is suffice for an TIMM_OSAL_EVENT_OR eOperation*/
	isolatedFlags = plEvent->eFlags & uRequestedEvents;

	/*Check if it is the AND operation. If yes then, all the flags must match*/
	if(and_operation){
		isolatedFlags =  (isolatedFlags == uRequestedEvents);
	}


	if(isolatedFlags) {

		/*We have got required combination of the eFlags bits and will return it back*/
		*pRetrievedEvents = plEvent->eFlags;
		bReturnStatus = OSA_SOK;
	}
	else {

		/*Required combination of bits is not yet available*/
		if ( OSA_NO_SUSPEND == uTimeOutMsec){
				*pRetrievedEvents = 0;
				bReturnStatus = OSA_SOK;
		}

		else if (OSA_SUSPEND == uTimeOutMsec){

			/*Wait till we get the required combination of bits. We we get the required
			*bits then we go out of the while loop
			*/
			while(!isolatedFlags){

				/*Wait on the conditional variable for another thread to set the eFlags and signal*/
				pthread_cond_wait(&(plEvent->condition), &(plEvent->mutex));

				/* eFlags set by some thread. Now, isolate the flags.
				 * The & operation is suffice for an TIMM_OSAL_EVENT_OR eOperation
				 */
				isolatedFlags = plEvent->eFlags & uRequestedEvents;

				/*Check if it is the AND operation. If yes then, all the flags must match*/
				if(and_operation){
					isolatedFlags =  (isolatedFlags == uRequestedEvents);
				}
			}

			/* Obtained the requested combination of bits on eFlags*/
			*pRetrievedEvents = plEvent->eFlags;
			bReturnStatus = OSA_SOK;

		}
		else {

			/* Calculate uTimeOutMsec in terms of the absolute time. uTimeOutMsec is in milliseconds*/
			gettimeofday(&now, NULL);
			timeout_us = now.tv_usec + 1000 * uTimeOutMsec;
			timeout.tv_sec = now.tv_sec + timeout_us / 1000000;
			timeout.tv_nsec = (timeout_us % 1000000) * 1000;

			while(!isolatedFlags){

				/* Wait till uTimeOutMsec for a thread to signal on the conditional variable*/
				status = pthread_cond_timedwait(&(plEvent->condition), &(plEvent->mutex), &timeout);

				/*Timedout or error and returned without being signalled*/
				if (0 != status) {
					if(ETIMEDOUT == status)
           	    		bReturnStatus = OSA_SOK;
       	    		*pRetrievedEvents = 0;
            	    break;
	            }

				/* eFlags set by some thread. Now, isolate the flags.
				 * The & operation is suffice for an TIMM_OSAL_EVENT_OR eOperation
				 */
				isolatedFlags = plEvent->eFlags & uRequestedEvents;

				/*Check if it is the AND operation. If yes then, all the flags must match*/
				if(and_operation){
						isolatedFlags =  (isolatedFlags == uRequestedEvents);
				}

			}
		}
	}

	/*If we have got the required combination of bits, we will have to reset the eFlags if CONSUME is mentioned
	*in the eOperations
	*/
	if (isolatedFlags && ((eOperation == OSA_EVENT_AND_CONSUME) || (eOperation == OSA_EVENT_OR_CONSUME))){
			plEvent->eFlags =  0;
	}

	/*Manually unlock the mutex*/
	if(0 != pthread_mutex_unlock(&(plEvent->mutex))) {
		/*OSAL_ERROR("Event Retrieve: Mutex Unlock failed !");*/
		bReturnStatus = OSA_EFAIL;
	}

EXIT:
    return bReturnStatus;

}








