

#ifndef _OSA_PIPE_H_
#define _OSA_PIPE_H_

#include <osa.h>

int OSA_CreatePipe (OSA_PTR *pPipe, unsigned long  pipeSize,
                                          unsigned long  messageSize,
                                          unsigned char  isFixedMessage) ;
int OSA_DeletePipe (OSA_PTR pPipe) ;
int OSA_WriteToPipe (OSA_PTR pPipe, void *pMessage,
                                           unsigned long size,
                                           unsigned long timeout) ;
int OSA_ReadFromPipe (OSA_PTR pPipe, void *pMessage,
                                            unsigned long size,
                                            unsigned long *actualSize,
                                            long timeout) ;

#endif /* _OSA_PIPE_H_ */

