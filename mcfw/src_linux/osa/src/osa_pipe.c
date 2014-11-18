#include <unistd.h>
#include <stdio.h>
#include <osa.h>
#include <osa_pipe.h>

typedef struct OSA_PIPE {
    int pfd[2];
    unsigned long  pipeSize;
    unsigned long  messageSize;
    unsigned char  isFixedMessage;
    int messageCount;
    int totalBytesInPipe;
} OSA_PIPE;


int OSA_CreatePipe (OSA_PTR *pPipe, unsigned long  pipeSize,
                                          unsigned long  messageSize,
                                          unsigned char  isFixedMessage)
{
    int bReturnStatus = OSA_EFAIL;
    OSA_PIPE *pHandle = NULL;
    OSA_PIPE *pHandleBackup = NULL;

    *pPipe = NULL;

    pHandle = (OSA_PIPE *)OSA_memAlloc(sizeof(OSA_PIPE));

    if (NULL == pHandle) {
        goto EXIT;
    }

    if (0 != pipe(pHandle->pfd)) {
        goto EXIT;
    }

	if(pHandle->pfd[0] == 0 || pHandle->pfd[0] == 1 || pHandle->pfd[0] == 2 ||
       pHandle->pfd[1] == 0 || pHandle->pfd[1] == 1 || pHandle->pfd[1] == 2)
    {
        pHandleBackup = (OSA_PIPE *)OSA_memAlloc(sizeof(OSA_PIPE));
        if (NULL == pHandleBackup)
        {
            goto EXIT;
        }
		if (0 != pipe(pHandleBackup->pfd))
        {
            goto EXIT;
        }

		if(pHandleBackup->pfd[0] == 2 || pHandleBackup->pfd[1] == 2)
        {
            int pfdDummy[2];

            if (0 != close(pHandleBackup->pfd[0]))
            {
               goto EXIT;
            }
            if (0 != close(pHandleBackup->pfd[1]))
            {
                goto EXIT;
            }
            /*Allocating the reserved file descriptor to dummy*/
            if(0 != pipe(pfdDummy))
            {
                goto EXIT;
            }
            /*Now the backup pfd will not get a reserved value*/
            if (0 != pipe(pHandleBackup->pfd))
            {
                goto EXIT;
            }
            /*Closing the dummy pfd*/
            if (0 != close(pfdDummy[0]))
            {
               goto EXIT;
            }
            if (0 != close(pfdDummy[1]))
            {
                goto EXIT;
            }

        }
		if (0 != close(pHandle->pfd[0]))
        {
            goto EXIT;
        }
        if (0 != close(pHandle->pfd[1]))
        {
            goto EXIT;
        }
        OSA_memFree(pHandle);

        pHandle = NULL;

        pHandleBackup->pipeSize = pipeSize;
        pHandleBackup->messageSize = messageSize;
        pHandleBackup->isFixedMessage = isFixedMessage;
        pHandleBackup->messageCount = 0;
        pHandleBackup->totalBytesInPipe = 0;

		*pPipe = (OSA_PTR) pHandleBackup ;
	}
	else
    {
        pHandle->pipeSize = pipeSize;
        pHandle->messageSize = messageSize;
        pHandle->isFixedMessage = isFixedMessage;
        pHandle->messageCount = 0;
        pHandle->totalBytesInPipe = 0;

        *pPipe = (OSA_PTR) pHandle ;
    }

    bReturnStatus = OSA_SOK;

EXIT:
    if ((OSA_SOK != bReturnStatus) && (NULL != pHandle)) {
       OSA_memFree(pHandle);
    }

    if ((OSA_SOK != bReturnStatus) && (NULL != pHandleBackup)) {
       OSA_memFree(pHandleBackup);
    }
    return bReturnStatus;
}

int OSA_DeletePipe (OSA_PTR pPipe)
{
    int bReturnStatus = OSA_SOK;

    OSA_PIPE *pHandle = (OSA_PIPE *)pPipe;

    if(NULL == pHandle) {
        bReturnStatus = OSA_EFAIL;
        goto EXIT;
    }

    if (0 != close(pHandle->pfd[0])) {
        /*OSA_Error ("Delete_Pipe Read fd failed!!!");*/
        bReturnStatus = OSA_EFAIL;
    }
    if (0 != close(pHandle->pfd[1])) {
        /*OSA_Error ("Delete_Pipe Write fd failed!!!");*/
        bReturnStatus = OSA_EFAIL;
    }

    OSA_memFree(pHandle);
EXIT:
    return bReturnStatus;
}


int OSA_WriteToPipe (OSA_PTR pPipe, void *pMessage,
                                           unsigned long size,
                                           unsigned long timeout)
{
    int bReturnStatus = OSA_SOK;
    unsigned long lSizeWritten = -1;

    OSA_PIPE *pHandle = (OSA_PIPE *)pPipe;

    if(size == 0) {
        printf("Nothing to write");
        bReturnStatus = OSA_EFAIL;
        goto EXIT;
    }

    lSizeWritten = write(pHandle->pfd[1], pMessage, size);

    if(lSizeWritten != size){
        printf("Writing to Pipe failed");
        bReturnStatus = OSA_EFAIL;
        goto EXIT;
    }

    /*Update message count and size*/
    pHandle->messageCount++;
    pHandle->totalBytesInPipe += size;

    bReturnStatus = OSA_SOK;

EXIT:
    return bReturnStatus;
}

int OSA_ReadFromPipe (OSA_PTR pPipe, void *pMessage,
                                            unsigned long size,
                                            unsigned long *actualSize,
                                            long timeout)
{
        int bReturnStatus = OSA_SOK;
        unsigned long lSizeRead = -1;
        OSA_PIPE *pHandle = (OSA_PIPE *)pPipe;

        if((size == 0) || (pHandle->messageCount == 0)) {           /*|| size > SSIZE_MAX)*/
            /*OSA_Error("Read size has error.");*/
            bReturnStatus = OSA_EFAIL;
            goto EXIT;
        }

        *actualSize =  lSizeRead = read(pHandle->pfd[0], pMessage, size);
        if(0 == lSizeRead){
            /*OSA_Error("EOF reached or no data in pipe");*/
            bReturnStatus = OSA_EFAIL;
            goto EXIT;
        }

        bReturnStatus = OSA_SOK;

        /*Update message count and pipe size*/
        pHandle->messageCount--;
        pHandle->totalBytesInPipe -= size;

    EXIT:
        return bReturnStatus;

}




