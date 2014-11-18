

#ifndef _OSA_EVENT_H_
#define _OSA_EVENT_H_

#include <osa.h>

/* ===========================================================================*/
/**
 *  OSA_EVENT_OPERATION - Different operations possible while retrieving
 *                        or setting events.
 *
 *  @ param OSA_EVENT_AND          :The event will be retreived if all 
 *                                        the events specified in the mask
 *                                        are active. The events will not be 
 *                                        consumed. 
 *                                        While setting events, the flag 
 *                                        specified will be ANDed with the
 *                                        current flag.
 *
 *  @ param OSA_EVENT_AND_CONSUME  :The event will be retreived if all 
 *                                        the events specified in the mask
 *                                        are active. All events in the 
 *                                        mask will be consumed. 
 *                                        Not valid while setting events.
 *
 *  @ param OSA_EVENT_OR           :The event will be retreived if any 
 *                                        the events specified in the mask
 *                                        are active. The events will not be 
 *                                        consumed.
 *                                        While setting events, the flag 
 *                                        specified will be ORed with the
 *                                        current flag.
 *
 *  @ param OSA_EVENT_OR_CONSUME   :The event will be retreived if any 
 *                                        the events specified in the mask
 *                                        are active. All active events in the 
 *                                        mask will be consumed. 
 *                                        Not valid while setting events.
 */
/* ===========================================================================*/
typedef enum OSA_EVENT_OPERATION 
{
   OSA_EVENT_AND,
   OSA_EVENT_AND_CONSUME,
   OSA_EVENT_OR,
   OSA_EVENT_OR_CONSUME

} OSA_EVENT_OPERATION;


/* ===========================================================================*/
/**
 * @fn OSA_EventCreate() - Creates a new Event instance.
 *
 *  @ param pEvents              :Handle of the Event to be created.                                              
 */
/* ===========================================================================*/
int OSA_EventCreate(OSA_PTR *pEvents);


/* ===========================================================================*/
/**
 * @fn OSA_EventDelete() - Deletes a previously created Event instance.
 *
 *  @ param pEvents              :Handle of the Event to be deleted.                                              
 */
/* ===========================================================================*/
int OSA_EventDelete(OSA_PTR pEvents);


/* ===========================================================================*/
/**
 * @fn OSA_EventSet() - Signals the requested Event. Tasks waiting on
 *                            this event will wake up.
 *
 *  @ param pEvents           :Handle of previously created Event instance.
 *
 *  @ param uEventFlag        :Mask of Event IDs to set.
 *
 *  @ param eOperation        :Operation while setting events.                                    
 */
/* ===========================================================================*/   
int OSA_EventSet(OSA_PTR pEvents, unsigned long uEventFlag, OSA_EVENT_OPERATION eOperation);



/* ===========================================================================*/
/**
 * @fn OSA_EventRetrieve() - Waits for event.
 *
 *  @ param pEvents                :Handle of previously created Event instance
 *
 *  @ param uRequestedEvents       :Mask of Event IDs to wait on
 *
 *  @ param eOperation             :Operation for the wait
 *
 *  @ param pRetreivedEvents       :Mask of the Event IDs retreived on success
 *
 *  @ param uTimeOut               :Time in millisec to wait for the event
 */
/* ===========================================================================*/
int OSA_EventRetrieve(OSA_PTR pEvents,
                      unsigned long uRequestedEvents,
                      OSA_EVENT_OPERATION eOperation,
                      unsigned long *pRetrievedEvents,
                      unsigned long uTimeOut);             

#endif /* _OSA_EVENT_H_ */

