/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2011 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
    \file chains_ipcBits.c
    \brief
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <sys/wait.h>
#include <sys/types.h>  // For stat().
#include <sys/stat.h>   // For stat().
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <osa_que.h>
#include <osa_mutex.h>
#include <osa_thr.h>
#include <osa_sem.h>
#include <demos/link_api_demos/common/chains_ipcBits.h>
#include <mcfw/interfaces/common_def/ti_vsys_common_def.h>
//20130128 Sue
#include"Pack_Header_Def.h"
#include "chains.h"
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>//guo for static

//20130319
//#include <pthread.h>
//#include <semaphore.h>
#include <sys/ipc.h>
#include "semaphore.h"
#define SERVER_PORT                 (1234)
#define LENGTH_OF_LISTEN_QUEUE      (10) //length of listen queue in server
/*
typedef struct FrameBuf 
{	
	unsigned int len;
	char buf[2000000];
}FrameBuf;
  struct FrameBuf Fbuf;
*/
#ifdef KB
#undef KB
#endif

#ifdef MB
#undef MB
#endif

#define KB                                                               (1024)
#define MB                                                               (KB*KB)

/* Set to TRUE to prevent doing fgets */
#define CHAINS_IPC_BITS_DISABLE_USER_INPUT                               (FALSE)


#define CHAINS_IPC_BITS_MAX_FILENAME_LENGTH                                 (64)
#define CHAINS_IPC_BITS_MAX_PATH_LENGTH                                     (256)
#define CHAINS_IPC_BITS_MAX_FULL_PATH_FILENAME_LENGTH                       (CHAINS_IPC_BITS_MAX_PATH_LENGTH+CHAINS_IPC_BITS_MAX_FILENAME_LENGTH)
#define CHAINS_IPC_BITS_FILE_STORE_DIR                                      "/dev/shm"
#define CHAINS_IPC_BITS_HDR_FILE_NAME                                       "VBITS_HDR"
#define CHAINS_IPC_BITS_DATA_FILE_NAME                                      "VBITS_DATA"
#define CHAINS_IPC_BITS_FILE_EXTENSION                                      "bin"

#define CHAINS_IPC_BITS_MAX_NUM_CHANNELS                                    (4)
#define CHAINS_IPC_BITS_NONOTIFYMODE_BITSIN                                 (FALSE)
#define CHAINS_IPC_BITS_NONOTIFYMODE_BITSOUT                                (FALSE)

#define CHAINS_IPC_BITS_MAX_DEFAULT_SIZE                                    (1000*MB)
#define CHAINS_IPC_BITS_FREE_SPACE_RETRY_MS                                 (16)
#define CHAINS_IPC_BITS_FREE_SPACE_MAX_RETRY_CNT                            (500)

#define CHAINS_IPC_BITS_MAX_BUFCONSUMEWAIT_MS                               (1000)

#define CHAINS_IPC_BITS_MAX_FILE_SIZE           (CHAINS_IPC_BITS_MAX_DEFAULT_SIZE)

#define CHAINS_IPC_BITS_INIT_FILEHANDLE(fp)                                    \
                                                   do {                        \
                                                       if (fp != NULL)         \
                                                       {                       \
                                                           fclose(fp);         \
                                                           fp = NULL;          \
                                                       }                       \
                                                   } while (0)

#define CHAINS_IPC_BITS_ENCODER_FPS                      (60)
#define CHAINS_IPC_BITS_ENCODER_BITRATE                  (20 * 1024 * 1024)
#define CHAINS_IPC_BITS_FILEBUF_SIZE_HDR                 (sizeof(Bitstream_Buf) * CHAINS_IPC_BITS_ENCODER_FPS)
#define CHAINS_IPC_BITS_FILEBUF_SIZE_DATA                (CHAINS_IPC_BITS_ENCODER_BITRATE)

#define CHAINS_IPCBITS_SENDFXN_TSK_PRI                   (2)
#define CHAINS_IPCBITS_RECVFXN_TSK_PRI                   (2)

#define CHAINS_IPCBITS_SENDFXN_TSK_STACK_SIZE            (0) /* 0 means system default will be used */
#define CHAINS_IPCBITS_RECVFXN_TSK_STACK_SIZE            (0) /* 0 means system default will be used */

#define CHAINS_IPCBITS_SENDFXN_PERIOD_MS                 (16/2)
#define CHAINS_IPCBITS_RECVFXN_PERIOD_MS                 (16/2)

#define CHAINS_IPCBITS_INFO_PRINT_INTERVAL               (1000)

/** @enum CHAINS_IPCBITS_GET_BITBUF_SIZE
 *  @brief Macro that returns max size of encoded bitbuffer for a given resolution
 */
#define CHAINS_IPCBITS_DEFAULT_WIDTH                   (720)
#define CHAINS_IPCBITS_DEFAULT_HEIGHT                  (576)

#define CHAINS_IPCBITS_GET_BITBUF_SIZE(width,height)   ((width) * (height)/2)

#define CHAINS_IPCBITS_MAX_PENDING_RECV_SEM_COUNT      (10)

#define CHAINS_IPCBITS_MAX_NUM_FREE_BUFS_PER_CHANNEL    (6)
#define CHAINS_IPCBITS_FREE_QUE_MAX_LEN                 (CHAINS_IPC_BITS_MAX_NUM_CHANNELS * \
                                                         CHAINS_IPCBITS_MAX_NUM_FREE_BUFS_PER_CHANNEL)
#define CHAINS_IPCBITS_FULL_QUE_MAX_LEN                 (CHAINS_IPCBITS_FREE_QUE_MAX_LEN)

#define CHAINS_IPC_BITS_ENABLE_FILE_WRITE               (TRUE)

#define CHAINS_IPC_BITS_FWRITE_ENABLE_BITMASK_CHANNEL_0     (1 << 0)
#define CHAINS_IPC_BITS_FWRITE_ENABLE_BITMASK_ALLCHANNELS   ((1 << CHAINS_IPC_BITS_MAX_NUM_CHANNELS) - 1)

#define CHAINS_IPC_BITS_FWRITE_ENABLE_BITMASK_DEFAULT   (CHAINS_IPC_BITS_FWRITE_ENABLE_BITMASK_CHANNEL_0)

#define CHAINS_IPC_BITS_TRACE_ENABLE_FXN_ENTRY_EXIT           (1)
#define CHAINS_IPC_BITS_TRACE_INFO_PRINT_INTERVAL             (8192)


#if CHAINS_IPC_BITS_TRACE_ENABLE_FXN_ENTRY_EXIT
#define CHAINS_IPC_BITS_TRACE_FXN(str,...)         do {                           \
                                                     static Int printInterval = 0;\
                                                     if ((printInterval % CHAINS_IPC_BITS_TRACE_INFO_PRINT_INTERVAL) == 0) \
                                                     {                                                          \
                                                         OSA_printf("CHAINS_IPCBITS:%s function:%s",str,__func__);     \
                                                         OSA_printf(__VA_ARGS__);                               \
                                                     }                                                          \
                                                     printInterval++;                                           \
                                                   } while (0)
#define CHAINS_IPC_BITS_TRACE_FXN_ENTRY(...)                  CHAINS_IPC_BITS_TRACE_FXN("Entered",__VA_ARGS__)
#define CHAINS_IPC_BITS_TRACE_FXN_EXIT(...)                   CHAINS_IPC_BITS_TRACE_FXN("Leaving",__VA_ARGS__)
#else
#define CHAINS_IPC_BITS_TRACE_FXN_ENTRY(...)
#define CHAINS_IPC_BITS_TRACE_FXN_EXIT(...)
#endif


enum Chains_IpcBitsFileType {
    CHAINS_IPC_BITS_FILETYPE_HDR,
    CHAINS_IPC_BITS_FILETYPE_BUF
} ;


typedef struct Chains_IpcBitsCtrlFileObj {
    FILE *fpWrHdr[CHAINS_IPC_BITS_MAX_NUM_CHANNELS];
    FILE *fpWrData[CHAINS_IPC_BITS_MAX_NUM_CHANNELS];
    char    fileDirPath[CHAINS_IPC_BITS_MAX_PATH_LENGTH];
    UInt32  maxFileSize;
    Bool    enableFWrite;
    UInt32  fwriteEnableBitMask;
} Chains_IpcBitsCtrlFileObj;

typedef struct Chains_IpcBitsCtrlThrObj {
    OSA_ThrHndl thrHandleBitsIn;
    OSA_ThrHndl thrHandleBitsInA;   //cmj set 3.11 for audio thread
    OSA_ThrHndl thrHandleBitsOut;
    OSA_QueHndl bufQFullBufs;
    OSA_QueHndl bufQFreeBufs;
    OSA_SemHndl bitsInNotifySem;
    volatile Bool exitBitsInThread;
    volatile Bool exitBitsOutThread;
} Chains_IpcBitsCtrlThrObj;

typedef struct Chains_IpcBitsCtrl {
    Bool  noNotifyBitsInHLOS;
    Bool  noNotifyBitsOutHLOS;;
    Chains_IpcBitsCtrlFileObj fObj;
    Chains_IpcBitsCtrlThrObj  thrObj;

} Chains_IpcBitsCtrl;

Chains_IpcBitsCtrl gChains_ipcBitsCtrl =
{
    .fObj.fpWrHdr  = {NULL},
    .fObj.fpWrData = {NULL},
    .fObj.maxFileSize    = CHAINS_IPC_BITS_MAX_FILE_SIZE,
    .fObj.enableFWrite   = CHAINS_IPC_BITS_ENABLE_FILE_WRITE,
    .fObj.fwriteEnableBitMask = CHAINS_IPC_BITS_FWRITE_ENABLE_BITMASK_DEFAULT,
    .noNotifyBitsInHLOS  = CHAINS_IPC_BITS_NONOTIFYMODE_BITSIN,
    .noNotifyBitsOutHLOS = CHAINS_IPC_BITS_NONOTIFYMODE_BITSOUT,
};

///////added by guo for RTP wis-streamer//////
#define NUM_RTSP 2
shared_use *shared_stuff;//struct  used for networkshare
int shmid[NUM_RTSP];
int semEmpty[NUM_RTSP],semFull[NUM_RTSP]; 
int semEmpty_audio,semFull_audio;
void *Videoptr[NUM_RTSP];//Pointers to shared mem of network sharing
void *audioptr = (void *)0;

void Chains_ipcBitsShareMemInit()
{
   int i;
  for(i=0;i<NUM_RTSP;i++)
  {
     shmid[i]=ShareMemInit(shmid[i],SHM_KEY+i);
    semEmpty[i]= semget((key_t)(SEMV_E0+4*i), 1, 0666 | IPC_CREAT);
    if (!set_semvalue(semEmpty[i],1)) 
	{
		fprintf(stderr, "Failed to initialize video empty semaphore\n");
		exit(EXIT_FAILURE);
	}
	
    semFull[i]= semget((key_t)(SEMV_F0+4*i), 1, 0666 | IPC_CREAT);
    if (!set_semvalue(semFull[i],0)) 
	{
		fprintf(stderr, "Failed to initialize video full semaphore\n");
		exit(EXIT_FAILURE);
	}
      Videoptr[i]= shmat(shmid[i],0,0);
	if(Videoptr[i] == (void *)-1)
    	{
    	    OSA_printf("Video shmat error\n");
		exit(EXIT_FAILURE);
    	}

  }
}
void Chains_ipcBitsShareMemDeInit()
{
   int i;
  for(i=0;i<NUM_RTSP;i++)
 {
    semEmpty[i] = 1;
    semFull[i] = 0;

   if(shmdt(Videoptr[i]) == -1)
   {
        printf("   shmdt  videoptr  failed\n");
	 exit(EXIT_FAILURE);
   }

    Del_ShareMem(shmid[i]);
    del_semvalue(semEmpty[i]);
    del_semvalue(semFull[i]);
  }
   semEmpty_audio = 1;
    semFull_audio = 0;
    if(gChains_ctrl.channelConf[0].audioEnable)
    {
           if(shmdt(audioptr) == -1)
	   {
	     printf("   shmdt audioptr failed\n");
	     exit(EXIT_FAILURE);
	  }	

	   del_semvalue(semEmpty_audio);
           del_semvalue(semFull_audio);
    }
}
void inline Copy2SendBuf(Bitstream_BufList*  fullBufList)
{
    Bitstream_Buf *pFullBuf;
    Bitstream_Buf *pEmptyBuf;
    int i,ch;
      for (i = 0; i < fullBufList->numBufs; i++)
      {
            pFullBuf = fullBufList->bufs[i];
	    if ( (pFullBuf->fillLength > 0) &&(pFullBuf->channelNum<NUM_RTSP) )//only rtp NUM-RTSP channels
	   { 
	       ch = pFullBuf->channelNum;
		shared_stuff = (shared_use *)Videoptr[ch];
		if (!semaphore_p(semEmpty[ch]))     //P() -1
	  	{
	  		  printf("semaphore_p()  video error ! \n");
                          exit(EXIT_FAILURE);		 	
  		}	
	   	 memcpy(shared_stuff->frame.buf_encode , pFullBuf->addr,pFullBuf->fillLength);
	          shared_stuff->frame.data_size = pFullBuf->fillLength;
	          shared_stuff->frame.I_Frame = pFullBuf->isKeyFrame;
                  shared_stuff->frame.timestamp = pFullBuf->timeStamp;		  
	          if (!semaphore_v(semFull[ch]))     //V() +1
	  	    {
	  		printf("semaphore_v() video error ! \n");
                        exit(EXIT_FAILURE);
  		    }
	      }
        }
}
static Void *Chains_ipcBitsRtspServerFxn(Void * prm)
{
    Chains_IpcBitsCtrl *ipcBitsCtrl = (Chains_IpcBitsCtrl *) prm;
    Chains_IpcBitsCtrlThrObj *thrObj = &ipcBitsCtrl->thrObj;
    Bitstream_BufList fullBufList;

    static Int printStats;
    //视频共享内存及信号量初始化
    //for rtsp
 	Chains_ipcBitsShareMemInit();
   
    OSA_printf("CHAINS_IPCBITS:%s:Entered...",__func__);
    while (FALSE == thrObj->exitBitsInThread)
    {
        //OSA_semWait(&thrObj->bitsInNotifySem,OSA_TIMEOUT_FOREVER);
        OSA_waitMsecs(CHAINS_IPCBITS_SENDFXN_PERIOD_MS/2);

        IpcBitsInLink_getFullVideoBitStreamBufs(SYSTEM_HOST_LINK_ID_IPC_BITS_IN_0,
                                                &fullBufList);
//process
      	Copy2SendBuf(&fullBufList);
	   
        IpcBitsInLink_putEmptyVideoBitStreamBufs(SYSTEM_HOST_LINK_ID_IPC_BITS_IN_0,
                                                 &fullBufList);
	if ((printStats % CHAINS_IPCBITS_INFO_PRINT_INTERVAL) == 0)
        {
            OSA_printf("CHAINS_IPCBITS:%s:INFO: periodic print..",__func__);
        }
        printStats++;
    }    
	//for rtsp	
    Chains_ipcBitsShareMemDeInit();
    OSA_printf("CHAINS_IPCBITS:%s:Leaving...",__func__);
    return NULL;
}


static Void *Chains_ipcBitsRtspAudioServerFxn(Void * prm)
{
    Chains_IpcBitsCtrl *ipcBitsCtrl = (Chains_IpcBitsCtrl *) prm;
    Chains_IpcBitsCtrlThrObj *thrObj = &ipcBitsCtrl->thrObj;

    static Int printStats;
    int startflag = 1;
    int audiocnt = 0;
  shared_use *shmPtr1;
	if(gChains_ctrl.channelConf[0].audioEnable)
	{
		    if(AudioOn==AUDIO_ON)
		  {
		      Audio_captureStart(0);
		      AudioOn=AUDIO_XX;
		   }
		  else if(AudioOn==AUDIO_OFF)	
		  {
		      Audio_captureStop();
		      Audio_captureDelete ();
		      AudioOn=AUDIO_XX;
		  }
                audioptr = shmat(shmid[0],0,0);//only ch0 has sound
		shmPtr1 =( shared_use *)audioptr;			
	         if(audioptr == (void *)-1)
	    	  {
	    	         OSA_printf("Audio shmat error\n");
			   exit(EXIT_FAILURE);
	    	  }
			 
                semEmpty_audio = semget((key_t)1222, 1, 0666 | IPC_CREAT);
	        if (!set_semvalue(semEmpty_audio,1)) 
		{
			fprintf(stderr, "Failed to initialize audio empty semaphore\n");
			exit(EXIT_FAILURE);
		}
	        semFull_audio = semget((key_t)1223, 1, 0666 | IPC_CREAT);
	        if (!set_semvalue(semFull_audio,0)) 
		{
			fprintf(stderr, "Failed to initialize audio full semaphore\n");
			exit(EXIT_FAILURE);
		}	
	   			
	}

  
    OSA_printf("CHAINS_IPCBITS:%s:Entered...",__func__);

    while (FALSE == thrObj->exitBitsInThread)
    {
        //OSA_semWait(&thrObj->bitsInNotifySem,OSA_TIMEOUT_FOREVER);
        OSA_waitMsecs(CHAINS_IPCBITS_SENDFXN_PERIOD_MS);
	//   OSA_waitMsecs(100);

	    if(gChains_ctrl.channelConf[0].audioEnable)
		{
	      	     if(Audioflag)
 	      	     {	    
	 	      	            if (!semaphore_p(semEmpty_audio))     //P() -1
		  	           {
		  		          printf("semaphore_p()  audio error ! \n");
	                              exit(EXIT_FAILURE);		 	
	  		           }
	                        memcpy(shmPtr1->buf.buf_encode, audiobag.buf_encode, audiobag.data_size);  
				    shmPtr1->buf.data_size = audiobag.data_size;
				    shmPtr1->buf.timestamp = audiobag.timestamp;

				    	//    printf("[8168]audio size is %d\n",audiobag.data_size);
			     
					    if (!semaphore_v(semFull_audio))     //V() +1
		  	                {
		  		               printf("semaphore_v() audio error ! \n");
	                                   exit(EXIT_FAILURE);
	  		                }			  				 		  
				    Audioflag = 0;
	 
                     }
	    	}
	 if ((printStats % CHAINS_IPCBITS_INFO_PRINT_INTERVAL) == 0)
        {
            OSA_printf("CHAINS_IPCBITS:%s:INFO: periodic print..",__func__);
        }
        printStats++;
      }

    OSA_printf("CHAINS_IPCBITS:%s:Leaving...",__func__);
    return NULL;
}
////////////////////////////////////////
UInt32 readTcp(int sockfd, UInt8* recv, UInt32 bytes)
{
    UInt32 nleft;
    UInt32 nread;

    UInt8 *ptr = recv;
    nleft = bytes;
    while(nleft > 0)
    {
        if((nread = read(sockfd, ptr, nleft)) < 0)
        {
            if(errno == EINTR)
                nread = 0;
            else
                return -1;
        }
        else if(nread == 0)
            break;
        nleft -= nread;
        ptr += nread;
    }
    return(bytes - nleft);
}

UInt32 writeTcp(int sockfd, UInt8 *send, UInt32 bytes)
{
    UInt32 nleft;
    UInt32 nwritten;
    UInt8 *ptr = send;
    nleft = bytes;
    while(nleft > 0)
    {
        if((nwritten = write(sockfd, ptr, nleft)) <= 0)
        {
            if(nwritten < 0 && errno == EINTR)
                nwritten = 0;
            else
                return -1;
        }
        nleft -= nwritten;
        ptr += nwritten;
    }
    return bytes-nleft;
}

static
Void Chains_ipcBitsGenerateFileName(char *dirPath,
                                    char *fname,
                                    UInt32 chNum,
                                    char *fsuffix,
                                    char *dstBuf,
                                    UInt32 maxLen);

static Void Chains_ipcBitsWriteWrap(FILE  * fp,
                                    UInt32 bytesToWrite,
                                    UInt32 maxFileSize)
{
    static Int printStatsIterval = 0;

    if (maxFileSize != CHAINS_IPC_BITS_MAX_FILE_SIZE_INFINITY)
    {
        if (((ftell(fp)) + bytesToWrite) > maxFileSize)
        {
            if ((printStatsIterval % CHAINS_IPCBITS_INFO_PRINT_INTERVAL) == 0)
                OSA_printf("CHAINS_IPCBITS:File wrap @ [%ld],MaxFileSize [%d]",
                           ftell(fp),maxFileSize);
            rewind(fp);
            printStatsIterval++;
        }
    }
}

static Void Chains_ipcBitsWriteBitsToFile (FILE  * fpHdr[CHAINS_IPC_BITS_MAX_NUM_CHANNELS],
                                           FILE  * fpBuf[CHAINS_IPC_BITS_MAX_NUM_CHANNELS],
                                           Bitstream_BufList *bufList,
                                           UInt32 maxFileSize,
                                           UInt32 fwriteEnableChannelBitmask)
{
    Int i;
    Bitstream_Buf *pBuf;
    size_t write_cnt;
	OSA_printf(" Here is Chains_ipcBitsWriteBitsToFile\n");

    for (i = 0; i < bufList->numBufs;i++)
    {
        UInt32 fileIdx;

        pBuf = bufList->bufs[i];
        OSA_assert(pBuf->channelNum < CHAINS_IPC_BITS_MAX_NUM_CHANNELS);
        fileIdx = pBuf->channelNum;
        if (fwriteEnableChannelBitmask & (1 << fileIdx))
        {
            OSA_printf("writting...\n");//20130117
            Chains_ipcBitsWriteWrap(fpBuf[fileIdx],pBuf->fillLength,maxFileSize);
            write_cnt = fwrite(pBuf->addr,sizeof(char),pBuf->fillLength,fpBuf[fileIdx]);
            OSA_assert(write_cnt == pBuf->fillLength);
            Chains_ipcBitsWriteWrap(fpHdr[fileIdx],sizeof(*pBuf),maxFileSize);
            write_cnt = fwrite(pBuf,sizeof(*pBuf),1,fpHdr[fileIdx]);
            OSA_assert(write_cnt == 1);
        }
    }
}

static Void Chains_ipcBitsCopyBitBufInfo (Bitstream_Buf *dst,
                                          const Bitstream_Buf *src)
{
    OSA_printf("Here is Chains_ipcBitsCopyBitBufInfo \n");//20130117
    dst->channelNum = src->channelNum;
    dst->codingType = src->codingType;
    dst->fillLength = src->fillLength;
    dst->isKeyFrame = src->isKeyFrame;
    dst->timeStamp  = src->timeStamp;
    dst->mvDataFilledSize = src->mvDataFilledSize;
    dst->bottomFieldBitBufSize = src->bottomFieldBitBufSize;
    dst->inputFileChanged = src->inputFileChanged;
    CHAINS_IPC_BITS_TRACE_FXN_EXIT("BitBufInfo:"
                         "virt:%p,"
                         "bufSize:%d,"
                         "chnId:%d,"
                         "codecType:%d,"
                         "filledBufSize:%d,"
                         "mvDataFilledSize:%d,"
                         "timeStamp:%d,"
                         "isKeyFrame:%d,"
                         "phy:%x,"
                         "width:%d"
                         "height:%d",
                         src->addr,
                         src->bufSize,
                         src->channelNum,
                         src->codingType,
                         src->fillLength,
                         src->mvDataFilledSize,
                         src->timeStamp,
                         src->isKeyFrame,
                         src->phyAddr,
                         src->frameWidth,
                         src->frameHeight);
}


static Void Chains_ipcBitsCopyBitBufDataMem2Mem(Bitstream_Buf *dstBuf,
                                                Bitstream_Buf *srcBuf)
{
    OSA_printf("Here is Chains_ipcBitsCopyBitBufDataMem2Mem");//20130117

    OSA_assert(srcBuf->fillLength < dstBuf->bufSize);
    memcpy(dstBuf->addr,srcBuf->addr,srcBuf->fillLength);
}

static Void Chains_ipcBitsQueEmptyBitBufs(UInt32            ipcBitsInLinkId,
                                          OSA_QueHndl       *emptyQue)
{
    Bitstream_BufList emptyBufList;
    Bitstream_Buf *pBuf;
    IpcBitsOutLinkHLOS_BitstreamBufReqInfo reqInfo;
    Int i;
    Int status;
    UInt32 bitBufSize;

    bitBufSize = CHAINS_IPCBITS_GET_BITBUF_SIZE(CHAINS_IPCBITS_DEFAULT_WIDTH,
                                                CHAINS_IPCBITS_DEFAULT_HEIGHT);
    emptyBufList.numBufs = 0;
    reqInfo.numBufs = VIDBITSTREAM_MAX_BITSTREAM_BUFS;
    reqInfo.reqType = IPC_BITSOUTHLOS_BITBUFREQTYPE_BUFSIZE;
    for (i = 0; i < VIDBITSTREAM_MAX_BITSTREAM_BUFS; i++)
    {
        reqInfo.u[i].minBufSize = bitBufSize;
    }
    IpcBitsOutLink_getEmptyVideoBitStreamBufs(ipcBitsInLinkId,
                                              &emptyBufList,
                                              &reqInfo);
    for (i = 0; i < emptyBufList.numBufs; i++)
    {
        pBuf = emptyBufList.bufs[i];
        OSA_assert(pBuf->bufSize >= bitBufSize );

        status = OSA_quePut(emptyQue,(Int32)pBuf,OSA_TIMEOUT_NONE);
        OSA_assert(status == 0);
    }
}

static Void Chains_ipcBitsSendFullBitBufs(UInt32            ipcBitsInLinkId,
                                          OSA_QueHndl       *fullQue)
{
    Bitstream_BufList fullBufList;
    Bitstream_Buf *pBuf;
    Int status;

    fullBufList.numBufs = 0;
    while((status = OSA_queGet(fullQue,(Int32 *)(&pBuf),OSA_TIMEOUT_NONE)) == 0)
    {
    
        OSA_assert(fullBufList.numBufs < VIDBITSTREAM_MAX_BITSTREAM_BUFS);
        fullBufList.bufs[fullBufList.numBufs] = pBuf;
        fullBufList.numBufs++;
        if (fullBufList.numBufs == VIDBITSTREAM_MAX_BITSTREAM_BUFS)
        {
            break;
    	 }
    }
    if (fullBufList.numBufs)
    {
        IpcBitsOutLink_putFullVideoBitStreamBufs(ipcBitsInLinkId,
                                                 &fullBufList);
    }
}

static Void *Chains_ipcBitsSendFxn(Void * prm)
{
    Chains_IpcBitsCtrlThrObj *thrObj = (Chains_IpcBitsCtrlThrObj *) prm;
    static Int printStatsInterval = 0;

    OSA_printf("CHAINS_IPCBITS:%s:Entered...",__func__);
    while (FALSE == thrObj->exitBitsOutThread)
    {
        OSA_waitMsecs(CHAINS_IPCBITS_SENDFXN_PERIOD_MS);
        Chains_ipcBitsQueEmptyBitBufs(SYSTEM_HOST_LINK_ID_IPC_BITS_OUT_0,
                                      &thrObj->bufQFreeBufs);
        Chains_ipcBitsSendFullBitBufs(SYSTEM_HOST_LINK_ID_IPC_BITS_OUT_0,
                                      &thrObj->bufQFullBufs);
        if ((printStatsInterval % CHAINS_IPCBITS_INFO_PRINT_INTERVAL) == 0)
        {
            OSA_printf("CHAINS_IPCBITS:%s:INFO: periodic print..",__func__);
        }
        printStatsInterval++;
    }
    OSA_printf("CHAINS_IPCBITS:%s:Leaving...",__func__);
    return NULL;
}

static Void Chains_ipcBitsProcessFullBufs(UInt32            ipcBitsInLinkId,
                                          Chains_IpcBitsCtrlThrObj *thrObj,
                                          Chains_IpcBitsCtrlFileObj *fObj)
{
   //OSA_printf(" Here is Chains_ipcBitsProcessFullBufs\n");//20130116
    Bitstream_BufList fullBufList;
    Bitstream_Buf *pFullBuf;
    Bitstream_Buf *pEmptyBuf;
    Int i,status;
    IpcBitsInLink_getFullVideoBitStreamBufs(ipcBitsInLinkId,
                                            &fullBufList);
	//OSA_printf("fullBufList.numBufs=%d\n",fullBufList.numBufs);//20130117
    for (i = 0; i < fullBufList.numBufs; i++)
    {
        status = OSA_queGet(&thrObj->bufQFreeBufs,(Int32 *)(&pEmptyBuf),
                            OSA_TIMEOUT_FOREVER);
        OSA_assert(status == 0);
        pFullBuf = fullBufList.bufs[i];
        Chains_ipcBitsCopyBitBufInfo(pEmptyBuf,pFullBuf);
        Chains_ipcBitsCopyBitBufDataMem2Mem(pEmptyBuf,pFullBuf);
        status = OSA_quePut(&thrObj->bufQFullBufs,
                            (Int32)pEmptyBuf,OSA_TIMEOUT_NONE);
        OSA_assert(status == 0);
    }

    if (fObj->enableFWrite)
    {
        Chains_ipcBitsWriteBitsToFile(fObj->fpWrHdr,
                                      fObj->fpWrData,
                                      &fullBufList,
                                      fObj->maxFileSize,
                                      fObj->fwriteEnableBitMask);

    }
    IpcBitsInLink_putEmptyVideoBitStreamBufs(ipcBitsInLinkId,
                                             &fullBufList);
}

static Void *Chains_ipcBitsRecvFxn(Void * prm)
{

    Chains_IpcBitsCtrl *ipcBitsCtrl = (Chains_IpcBitsCtrl *) prm;
    Chains_IpcBitsCtrlThrObj *thrObj = &ipcBitsCtrl->thrObj;
    Chains_IpcBitsCtrlFileObj *fObj =  &ipcBitsCtrl->fObj;
    static Int printStats;

    OSA_printf("CHAINS_IPCBITS:%s:Entered...\n",__func__);
    while (FALSE == thrObj->exitBitsInThread)
    {
        OSA_semWait(&thrObj->bitsInNotifySem,OSA_TIMEOUT_FOREVER);
	//OSA_waitMsecs(CHAINS_IPCBITS_SENDFXN_PERIOD_MS);//20130117
        Chains_ipcBitsProcessFullBufs(SYSTEM_HOST_LINK_ID_IPC_BITS_IN_0,
                                      thrObj,
                                      fObj);
        if ((printStats % CHAINS_IPCBITS_INFO_PRINT_INTERVAL) == 0)
        {
            OSA_printf("CHAINS_IPCBITS:%s:INFO: periodic print..",__func__);
        }
        printStats++;
    }
    OSA_printf("CHAINS_IPCBITS:%s:Leaving...",__func__);
    return NULL;
}
static Void *Chains_ipcBitsRecvTcpSendFxn(Void * prm)
{
    Chains_IpcBitsCtrl *ipcBitsCtrl = (Chains_IpcBitsCtrl *) prm;
    Chains_IpcBitsCtrlThrObj *thrObj = &ipcBitsCtrl->thrObj;
    Bitstream_BufList fullBufList;
    Bitstream_Buf *pFullBuf;
    Bitstream_Buf *pEmptyBuf;
    static Int printStats;

    Int i;
    int sockfd, clifd;
    struct sockaddr_in serv_addr, cli_addr;
    UInt32 sendBytes;
    UInt32 flag=1, len=sizeof(UInt32);

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("error!");
        exit(1);
    }
    //setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &bitBufSize, sizeof(bitBufSize));
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &flag, len);

    bzero(&serv_addr,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);
    serv_addr.sin_addr.s_addr = htons(INADDR_ANY);

    if (bind(sockfd,(struct sockaddr*)&serv_addr,sizeof(serv_addr))<0)
    {
        OSA_printf("bind to port %d failure!\n",SERVER_PORT);
        exit(1);
    }

    if (listen(sockfd, LENGTH_OF_LISTEN_QUEUE) < 0)
    {
        OSA_printf("call listen failure!\n");
        exit(1);
    }

    socklen_t length = sizeof(cli_addr);
    clifd = accept(sockfd,(struct sockaddr*)&cli_addr,&length);
    if (clifd < 0)
    {
        printf("error comes when call accept!\n");
        exit(1);
    }
    printf("from client,IP:%s,Port:%d\n",inet_ntoa(cli_addr.sin_addr),ntohs(cli_addr.sin_port));

    close(sockfd);

    OSA_printf("CHAINS_IPCBITS:%s:Entered...",__func__);
    while (FALSE == thrObj->exitBitsInThread)
    {
        OSA_semWait(&thrObj->bitsInNotifySem,OSA_TIMEOUT_FOREVER);

        IpcBitsInLink_getFullVideoBitStreamBufs(SYSTEM_HOST_LINK_ID_IPC_BITS_IN_0,
                                                &fullBufList);

        for (i = 0; i < fullBufList.numBufs; i++)
        {
            pFullBuf = fullBufList.bufs[i];
            if (pFullBuf->fillLength < 0)
                continue;

            sendBytes = writeTcp(clifd, (UInt8 *)pFullBuf, sizeof(Bitstream_Buf));
            if (sendBytes != sizeof(Bitstream_Buf)) {
                OSA_printf("send error %d %d\n", sendBytes, sizeof(Bitstream_Buf));
            }

            sendBytes = writeTcp(clifd, pFullBuf->addr, pFullBuf->fillLength);
            if (sendBytes != pFullBuf->fillLength) {
                OSA_printf("send error %d %d\n", sendBytes, pFullBuf->fillLength);
            }
        }

        IpcBitsInLink_putEmptyVideoBitStreamBufs(SYSTEM_HOST_LINK_ID_IPC_BITS_IN_0,
                                                 &fullBufList);

        if ((printStats % CHAINS_IPCBITS_INFO_PRINT_INTERVAL) == 0)
        {
            OSA_printf("CHAINS_IPCBITS:%s:INFO: periodic print..",__func__);
        }
        printStats++;
    }
    OSA_printf("CHAINS_IPCBITS:%s:Leaving...",__func__);
    close(clifd);
    return NULL;
}
/*
static Void *Chains_ipcBitsRecvRtspServerFxn(Void * prm)
{
    OSA_printf(" Here is ipcBitRecvRtspServer\n");//Sue add 20130115
    Chains_IpcBitsCtrl *ipcBitsCtrl = (Chains_IpcBitsCtrl *) prm;
    Chains_IpcBitsCtrlThrObj *thrObj = &ipcBitsCtrl->thrObj;
    Bitstream_BufList fullBufList;
    Bitstream_Buf *pFullBuf;
    Bitstream_Buf *pEmptyBuf;
    static Int printStats;
    int fdLiveServer[16];
    char filename[256];

    Int i;
    for (i=0; i<gChains_ctrl.channelNum; i++) 
   {
        sprintf(filename, "/tmp/liveServer%d.264", i);
        fdLiveServer[i] = open(filename, O_WRONLY);
        if (fdLiveServer < 0) 
	 {
            OSA_printf("Can't open file %s\n", filename);
            return NULL;
        }
        OSA_printf("open file %s", filename);
    }

    OSA_printf("CHAINS_IPCBITS:%s:Entered...",__func__);
    while (FALSE == thrObj->exitBitsInThread)
    {
        //OSA_semWait(&thrObj->bitsInNotifySem,OSA_TIMEOUT_FOREVER);
        OSA_waitMsecs(CHAINS_IPCBITS_SENDFXN_PERIOD_MS);

         IpcBitsInLink_getFullVideoBitStreamBufs(SYSTEM_HOST_LINK_ID_IPC_BITS_IN_0,
                                                &fullBufList);


        for (i = 0; i < fullBufList.numBufs; i++)
        {
            pFullBuf = fullBufList.bufs[i];
            if (pFullBuf->fillLength > 0) {
                write(fdLiveServer[pFullBuf->channelNum], pFullBuf->addr, pFullBuf->fillLength);
            }
        }

        IpcBitsInLink_putEmptyVideoBitStreamBufs(SYSTEM_HOST_LINK_ID_IPC_BITS_IN_0,
                                                 &fullBufList);

        if ((printStats % CHAINS_IPCBITS_INFO_PRINT_INTERVAL) == 0)
        {
            OSA_printf("CHAINS_IPCBITS:%s:INFO: periodic print..",__func__);
        }
        printStats++;
    }
    OSA_printf("CHAINS_IPCBITS:%s:Leaving...",__func__);
    for (i=0; i<gChains_ctrl.channelNum; i++) {
        close(fdLiveServer[i]);
    }
    return NULL;
}
*/
//use fifo write for RTSP send   //origin
static Void *Chains_ipcBitsRecvRtspServerFxn(Void * prm)
{
    Chains_IpcBitsCtrl *ipcBitsCtrl = (Chains_IpcBitsCtrl *) prm;
    Chains_IpcBitsCtrlThrObj *thrObj = &ipcBitsCtrl->thrObj;
    Bitstream_BufList fullBufList;
    Bitstream_Buf *pFullBuf;
    Bitstream_Buf *pEmptyBuf;
    static Int printStats;
    int fdLiveServer[16];
    char filename[256];

    Int i;
    for (i=0; i<gChains_ctrl.channelNum; i++) {
        sprintf(filename, "/tmp/liveServer%d.264", i);
        fdLiveServer[i] = open(filename, O_WRONLY);
        if (fdLiveServer < 0) {
            OSA_printf("Can't open file %s\n", filename);
            return NULL;
        }
        OSA_printf("open file %s", filename);
    }

    OSA_printf("CHAINS_IPCBITS:%s:Entered...",__func__);
    while (FALSE == thrObj->exitBitsInThread)
    {
        //OSA_semWait(&thrObj->bitsInNotifySem,OSA_TIMEOUT_FOREVER);
        OSA_waitMsecs(CHAINS_IPCBITS_SENDFXN_PERIOD_MS);

        IpcBitsInLink_getFullVideoBitStreamBufs(SYSTEM_HOST_LINK_ID_IPC_BITS_IN_0,
                                                &fullBufList);

        for (i = 0; i < fullBufList.numBufs; i++)
        {
            pFullBuf = fullBufList.bufs[i];
            if (pFullBuf->fillLength > 0) {
                write(fdLiveServer[pFullBuf->channelNum], pFullBuf->addr, pFullBuf->fillLength);
            }
        }

        IpcBitsInLink_putEmptyVideoBitStreamBufs(SYSTEM_HOST_LINK_ID_IPC_BITS_IN_0,
                                                 &fullBufList);

        if ((printStats % CHAINS_IPCBITS_INFO_PRINT_INTERVAL) == 0)
        {
            OSA_printf("CHAINS_IPCBITS:%s:INFO: periodic print..",__func__);
        }
        printStats++;
    }
    OSA_printf("CHAINS_IPCBITS:%s:Leaving...",__func__);
    for (i=0; i<gChains_ctrl.channelNum; i++) {
        close(fdLiveServer[i]);
    }
    return NULL;
}

void Chains_ipcBitsLocalTime()
{
   time ( &rawtime );
   LocalTime = localtime ( &rawtime );
   printf("%4d-%02d-%02d %02d:%02d:%02d\n",
   	1900+LocalTime->tm_year, 1+LocalTime->tm_mon,LocalTime->tm_mday,
        LocalTime->tm_hour,LocalTime->tm_min,LocalTime->tm_sec);

}
static void Chains_ipcWriteFiles(FILE  * fdLocalstorage[CHAINS_IPC_BITS_MAX_NUM_CHANNELS],
                                           FILE  * fdst,FILE *fdaudio,
                                           Bitstream_BufList  *fullBufList,
                                           struct NALU_t nalu)
{	int n;
       Bitstream_Buf *pFullBuf;
	unsigned int Frame_Index=0;//帧序号

   	   for (n = 0; n < fullBufList->numBufs; n++)//pFullBuf->channelNumber
	   {
		    pFullBuf = fullBufList->bufs[n];
		    if (pFullBuf->fillLength > 0)
		    {
			//printf("\n pFullBuf->channelNum:%d\n",pFullBuf->channelNum);
		       fwrite(pFullBuf->addr,pFullBuf->fillLength,1,fdLocalstorage[pFullBuf->channelNum]);
			//printf("\n pFullBuf 长度为:%d\n",pFullBuf->fillLength);
 	           }		
		//only write channel 0
		//OSA_printf("channelNum = %d\n",pFullBuf->channelNum);
		if(pFullBuf->channelNum ==0)
		{
		
		     ps_package( fdst,nalu,pFullBuf->addr,pFullBuf->fillLength,&Frame_Index);
		    
		     if(gChains_ctrl.channelConf[0].audioEnable)
		    {
	      	   	  if(Audioflag){ 	
		       		fwrite(audiobag.buf_encode, audiobag.data_size,1,fdaudio);   
				ps_package( fdst,nalu,audiobag.buf_encode,audiobag.data_size,&Frame_Index);
	              		Audioflag=0;}
		    }	
		}
	    }
}
static Void *Chains_ipcBitsLocalStorage(Void * prm)
{

    Chains_IpcBitsCtrl *ipcBitsCtrl = (Chains_IpcBitsCtrl *) prm;
    Chains_IpcBitsCtrlThrObj *thrObj = &ipcBitsCtrl->thrObj;
    Bitstream_BufList fullBufList;
    int n;
    //Bitstream_Buf *pEmptyBuf;
    static Int printStats;
    FILE* fdLocalstorage[CHAINS_IPC_BITS_MAX_NUM_CHANNELS];
    char fileName_Local[256];
    FILE* fdst;
    char fileNamePs[256];
    FILE* fdaudio;
    char fileName_audio[256];
	struct NALU_t nalu;
        nalu.buf = (char *)malloc(2000000);
    Chains_ipcBitsLocalTime();
    for (n=0; n<gChains_ctrl.channelNum; n++) 
   {     //存放h.264  的文件
        sprintf(fileName_Local, "/mnt/Localstorage%d_%4d-%02d-%02d_%02d:%02d:%02d.264",
      		 	 n,1900+LocalTime->tm_year, 1+LocalTime->tm_mon,LocalTime->tm_mday,
        		LocalTime->tm_hour,LocalTime->tm_min,LocalTime->tm_sec);
	   
	 //fdLocalstorage[n] = open(fileName_Local, O_WRONLY|O_CREAT|O_TRUNC);
	 fdLocalstorage[n] = fopen(fileName_Local, "wb");//guo
	 if (fdLocalstorage[n] < 0) {
            OSA_printf("Can't open file %s\n", fileName_Local);
            return NULL;
        }
   }
     // now only record one channel---guo
   sprintf(fileNamePs, "/mnt/record_%4d-%02d-%02d_%02d:%02d:%02d.mpg",
	   	1900+LocalTime->tm_year, 1+LocalTime->tm_mon,LocalTime->tm_mday,
              LocalTime->tm_hour,LocalTime->tm_min,LocalTime->tm_sec);
  // fdst = open(fileNamePs, O_WRONLY|O_CREAT|O_TRUNC);
   fdst = fopen(fileNamePs, "wb");
    //audio
   sprintf(fileName_audio, "/mnt/audio.pcm");
   fdaudio = fopen(fileName_audio, "wb");
   if (fdaudio< 0) {
        OSA_printf("Can't open file %s\n", fileName_audio);
          return NULL;
   }
   if (fdst< 0) {
       OSA_printf("Can't open file %s\n", fileNamePs);
        return NULL;
   }
   while(FALSE == thrObj->exitBitsInThread)
   {
           /*xte_Sue Audio Control*/
	     if(gChains_ctrl.channelConf[0].audioEnable)
           {
                if(AudioOn==AUDIO_ON)
		  {
		      Audio_captureStart(0);
		      AudioOn=AUDIO_XX;
		   }
		  else if(AudioOn==AUDIO_OFF)	
		  {
		      Audio_captureStop();
		      Audio_captureDelete ();
		      AudioOn=AUDIO_XX;
		  }
	     }
            /*******************/
      	   OSA_waitMsecs(CHAINS_IPCBITS_SENDFXN_PERIOD_MS);
      	   //OSA_semWait(&thrObj->bitsInNotifySem,OSA_TIMEOUT_FOREVER);
	   IpcBitsInLink_getFullVideoBitStreamBufs(SYSTEM_HOST_LINK_ID_IPC_BITS_IN_0,
			                                                &fullBufList);  
	  Chains_ipcWriteFiles(fdLocalstorage,fdst, fdaudio, &fullBufList, nalu);//guo
	
	    IpcBitsInLink_putEmptyVideoBitStreamBufs(SYSTEM_HOST_LINK_ID_IPC_BITS_IN_0,
			                                                 &fullBufList);  
#define PRINT_INTERVAL
 #ifdef PRINT_INTERVAL
	   if ((printStats % CHAINS_IPCBITS_INFO_PRINT_INTERVAL) == 0)
          {
            OSA_printf("CHAINS_IPCBITS:%s:INFO: periodic print..",__func__);
          }
          printStats++;
#endif
      }
	
	for (n=0; n<gChains_ctrl.channelNum; n++) 
	{
	   fclose(fdLocalstorage[n]);
	}
	 fclose(fdaudio);
	 fclose(fdst);
	 free(nalu.buf);
	 OSA_printf("CHAINS_IPCBITS:%s:Leaving...",__func__);
    return NULL;
}
static Void *Chains_tcpRecvFxn(Void * prm)
{
    Chains_IpcBitsCtrlThrObj *thrObj = (Chains_IpcBitsCtrlThrObj *) prm;
    Bitstream_Buf *pEmptyBuf;
    static Int printStats;
    Bitstream_Buf tmpBuf;
    UInt32 bufStructSize = sizeof(Bitstream_Buf);
    UInt32 bitBufSize;

    int sockfd;
    struct sockaddr_in serv_addr;
    UInt32 recvBytes;
    UInt8 pRecvBuf[4 * 1024 * 1024];

    bitBufSize = CHAINS_IPCBITS_GET_BITBUF_SIZE(CHAINS_IPCBITS_DEFAULT_WIDTH,
                                                CHAINS_IPCBITS_DEFAULT_HEIGHT);

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        OSA_printf("error!");
        exit(1);
    }

    bzero(&serv_addr,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);
    if (gChains_ctrl.channelConf[0].enableTcp == TRUE &&
    		gChains_ctrl.channelConf[0].enableServer == TRUE &&
    		gChains_ctrl.channelConf[0].enableClient == TRUE) {
        serv_addr.sin_addr.s_addr = htons(INADDR_ANY);
    } else {
        serv_addr.sin_addr.s_addr = inet_addr(gChains_ctrl.channelConf[0].serverIp);
    }

    OSA_printf("Connect %s ...", gChains_ctrl.channelConf[0].serverIp);
    while (1) {
        if (connect(sockfd, (struct sockaddr *)&serv_addr,
                sizeof(struct sockaddr)) == -1) {
            OSA_waitMsecs(1000);
        }
        else {
            OSA_printf("Connect success!\n");
            break;
        }
    }

    OSA_printf("CHAINS_IPCBITS:%s:Entered...",__func__);
    Bitstream_BufList emptyBufList;
    IpcBitsOutLinkHLOS_BitstreamBufReqInfo reqInfo;

    while (FALSE == thrObj->exitBitsOutThread)
    {
        //OSA_waitMsecs(CHAINS_IPCBITS_RECVFXN_PERIOD_MS);
        OSA_waitMsecs(12);

        recvBytes = readTcp(sockfd, pRecvBuf, bufStructSize);
        if (recvBytes != bufStructSize) {
            OSA_printf("receive error %d %d", recvBytes, bufStructSize);
        }
        memcpy(&tmpBuf, pRecvBuf, sizeof(Bitstream_Buf));

        recvBytes = readTcp(sockfd, pRecvBuf, tmpBuf.fillLength);
        if (recvBytes != tmpBuf.fillLength) {
               OSA_printf("receive error %d %d\n", recvBytes, tmpBuf.fillLength);
        }

        emptyBufList.numBufs = 0;
        reqInfo.numBufs = 1;
        reqInfo.u[0].minBufSize = bitBufSize;
        reqInfo.reqType = IPC_BITSOUTHLOS_BITBUFREQTYPE_BUFSIZE;
        IpcBitsOutLink_getEmptyVideoBitStreamBufs(SYSTEM_HOST_LINK_ID_IPC_BITS_OUT_0,
                                                  &emptyBufList,
                                                  &reqInfo);
        //OSA_assert(emptyBufList.numBufs == 1);
        if (emptyBufList.numBufs == 0) {
        	OSA_printf("Can not get an empty buffer\n");
        	continue;
        }
        pEmptyBuf = emptyBufList.bufs[0];

        Chains_ipcBitsCopyBitBufInfo(pEmptyBuf, &tmpBuf);
        memcpy(pEmptyBuf->addr, pRecvBuf, pEmptyBuf->fillLength);

        IpcBitsOutLink_putFullVideoBitStreamBufs(SYSTEM_HOST_LINK_ID_IPC_BITS_OUT_0,
                                                 &emptyBufList);

        if ((printStats % CHAINS_IPCBITS_INFO_PRINT_INTERVAL) == 0)
        {
            OSA_printf("CHAINS_IPCBITS:%s:INFO: periodic print..",__func__);
        }
        printStats++;
    }
    OSA_printf("CHAINS_IPCBITS:%s:Leaving...",__func__);
    close(sockfd);
    return NULL;
}

static Void Chains_ipcBitsInitThrObj(Chains_IpcBitsCtrlThrObj *thrObj)
{

   
    OSA_semCreate(&thrObj->bitsInNotifySem,
                  CHAINS_IPCBITS_MAX_PENDING_RECV_SEM_COUNT,0);
    thrObj->exitBitsInThread = FALSE;
    thrObj->exitBitsOutThread = FALSE;
    OSA_queCreate(&thrObj->bufQFreeBufs,CHAINS_IPCBITS_FREE_QUE_MAX_LEN);
    OSA_queCreate(&thrObj->bufQFullBufs,CHAINS_IPCBITS_FULL_QUE_MAX_LEN);


   //******dyx20131108******
   /* if (gChains_ctrl.channelConf[0].enableServer) 
	{
        	Void *(*bitsRecvFxn)(Void *prm) = Chains_ipcBitsRecvFxn;
	      /*********************Sue 20120120*******************/
		
       	/*if (gChains_ctrl.channelConf[0].enableTcp) 
		 {
           		bitsRecvFxn = Chains_ipcBitsRecvTcpSendFxn;
        	  }
             else if (gChains_ctrl.channelConf[0].enableRtsp) 
		{
            		bitsRecvFxn = Chains_ipcBitsRecvRtspServerFxn;
        	 }
	       //当TCP RTSP 全未开启时执行本地存储
	      else if (gChains_ipcBitsCtrl.fObj.enableFWrite == TRUE)
		{
		       OSA_printf("gChains_ipcBitsCtrl.fObj.enableFWrite=TRUE\n");
                    bitsRecvFxn = Chains_ipcBitsLocalStorage;//本地存储
                    
              }

		/***************************************************/
   /* if(enablelocst!=0&&disablelocst==0)
    	{
    	       printf("here LOCST\n\n\n\n");
		OSA_thrCreate(&thrObj->thrHandleBitsIn,
                      bitsRecvFxn,
                      CHAINS_IPCBITS_RECVFXN_TSK_PRI,
                      CHAINS_IPCBITS_RECVFXN_TSK_STACK_SIZE,
                      &gChains_ipcBitsCtrl);
       }
		
    }
	

    if (gChains_ctrl.channelConf[0].enableClient) 
   {
        Void *(*bitsSendFxn)(Void *prm) = Chains_ipcBitsSendFxn;
        if (gChains_ctrl.channelConf[0].enableTcp) {
            bitsSendFxn = Chains_tcpRecvFxn;
        }
        else if (gChains_ctrl.channelConf[0].enableRtsp) {
        }
   if(enablelocst!=0&&disablelocst==0)
       {
        OSA_thrCreate(&thrObj->thrHandleBitsOut,
                      bitsSendFxn,
                      CHAINS_IPCBITS_SENDFXN_TSK_PRI,
                      CHAINS_IPCBITS_SENDFXN_TSK_STACK_SIZE,            
                      &gChains_ipcBitsCtrl);
       }
    }*/
}

static Void Chains_ipcBitsDeInitThrObj(Chains_IpcBitsCtrlThrObj *thrObj)
{
    thrObj->exitBitsInThread = TRUE;
    thrObj->exitBitsOutThread = TRUE;
//    OSA_thrDelete(&thrObj->thrHandleBitsOut);
 //   OSA_thrDelete(&thrObj->thrHandleBitsIn);
	//added for rtsp
//	OSA_thrDelete(&thrObj->thrHandleBitsInA);//moved to Locststop
    OSA_semDelete(&thrObj->bitsInNotifySem);
    OSA_queDelete(&thrObj->bufQFreeBufs);
    OSA_queDelete(&thrObj->bufQFullBufs);


}

//******dyx20131108******
static Void Chains_ipcBitsLocStThrObj(Chains_IpcBitsCtrlThrObj *thrObj)
{

    if (gChains_ctrl.channelConf[0].enableServer) 
	{
        	Void *(*bitsRecvFxn)(Void *prm) = Chains_ipcBitsRecvFxn;
		Void *(*bitsRecvFxnA)(Void *prm)= Chains_ipcBitsRecvFxn;
	      /*********************Sue 20120120*******************/
		
       		if (gChains_ctrl.channelConf[0].enableTcp) 
		 {
           		bitsRecvFxn = Chains_ipcBitsRecvTcpSendFxn;
        	  }
             else if (gChains_ctrl.channelConf[0].enableRtsp) 
		{
#define MY_RTSP
#ifdef MY_RTSP
			bitsRecvFxn = Chains_ipcBitsRtspServerFxn;
		if(gChains_ctrl.channelConf[0].audioEnable)
	 	 {
	 	 		bitsRecvFxnA = Chains_ipcBitsRtspAudioServerFxn;
	        		OSA_thrCreate(&thrObj->thrHandleBitsInA,
                            bitsRecvFxnA,
                            CHAINS_IPCBITS_RECVFXN_TSK_PRI,
                            CHAINS_IPCBITS_RECVFXN_TSK_STACK_SIZE,
                            &gChains_ipcBitsCtrl);
	   	}
	  
#else
            		bitsRecvFxn = Chains_ipcBitsRecvRtspServerFxn;
#endif
        	 }
	       //当TCP RTSP 全未开启时执行本地存储
	      else if (gChains_ipcBitsCtrl.fObj.enableFWrite == TRUE)
		{
		       OSA_printf("gChains_ipcBitsCtrl.fObj.enableFWrite=TRUE\n");
                    bitsRecvFxn = Chains_ipcBitsLocalStorage;//本地存储
                    
              }

		/***************************************************/
    
    	       printf("Start LocalStorage\n\n");
		OSA_thrCreate(&thrObj->thrHandleBitsIn,
                      bitsRecvFxn,
                      CHAINS_IPCBITS_RECVFXN_TSK_PRI,
                      CHAINS_IPCBITS_RECVFXN_TSK_STACK_SIZE,
                      &gChains_ipcBitsCtrl);
		
       
		
    }
	

    if (gChains_ctrl.channelConf[0].enableClient) 
   {
        Void *(*bitsSendFxn)(Void *prm) = Chains_ipcBitsSendFxn;
        if (gChains_ctrl.channelConf[0].enableTcp) {
            bitsSendFxn = Chains_tcpRecvFxn;
        }
        else if (gChains_ctrl.channelConf[0].enableRtsp) {
        }
   
        OSA_thrCreate(&thrObj->thrHandleBitsOut,
                      bitsSendFxn,
                      CHAINS_IPCBITS_SENDFXN_TSK_PRI,
                      CHAINS_IPCBITS_SENDFXN_TSK_STACK_SIZE,            
                      &gChains_ipcBitsCtrl);
       
    }
}


static
Void Chains_ipcBitsGenerateFileName(char *dirPath,
                                    char *fname,
                                    UInt32 chNum,
                                    char *fsuffix,
                                    char *dstBuf,
                                    UInt32 maxLen)
{
    snprintf(dstBuf,
             (maxLen - 2),
             "%s/%s_%d.%s",
             dirPath,
             fname,
             chNum,
             fsuffix);
    dstBuf[(maxLen - 1)] = 0;
}

static Void Chains_ipcBitsInCbFxn (Ptr cbCtx)
{
    Chains_IpcBitsCtrl *chains_ipcBitsCtrl;
    static Int printInterval;

    OSA_assert(cbCtx = &gChains_ipcBitsCtrl);
    chains_ipcBitsCtrl = cbCtx;
    OSA_semSignal(&chains_ipcBitsCtrl->thrObj.bitsInNotifySem);
    if ((printInterval % CHAINS_IPCBITS_INFO_PRINT_INTERVAL) == 0)
    {
        OSA_printf("CHAINS_IPCBITS: Callback function:%s",__func__);
    }
    printInterval++;
}

Void Chains_ipcBitsInitCreateParams_BitsInHLOS(IpcBitsInLinkHLOS_CreateParams *cp)
{
    cp->baseCreateParams.noNotifyMode = gChains_ipcBitsCtrl.noNotifyBitsInHLOS;
    cp->cbFxn = Chains_ipcBitsInCbFxn;
    cp->cbCtx = &gChains_ipcBitsCtrl;
    cp->baseCreateParams.notifyNextLink = FALSE;
    /* Previous link of bitsInHLOS is bitsOutRTOS. So, notifyPrevLink
     * should be set to false if bitsInHLOS is to operate in
     * NO_NOTIFY_MODE
     */
    cp->baseCreateParams.notifyPrevLink = TRUE;
}

static Void Chains_ipcBitsInCbFxnScd (Ptr cbCtx)
{
    Chains_IpcBitsCtrl *chains_ipcBitsCtrl;
	static Int printInterval;
	//do my work!!
	        OSA_printf("!!!!Here to realize Scd status get!!");

    if ((printInterval % CHAINS_IPCBITS_INFO_PRINT_INTERVAL) == 0)
    {
        OSA_printf("CHAINS_IPCBITS: Callback function:%s",__func__);
    }
    printInterval++;
}
Void Chains_ipcBitsInitCreateParams_BitsInHLOSVCap(IpcBitsInLinkHLOS_CreateParams *cp)
{
    cp->baseCreateParams.noNotifyMode = gChains_ipcBitsCtrl.noNotifyBitsInHLOS;
    cp->cbFxn = Chains_ipcBitsInCbFxnScd;
    cp->cbCtx = NULL;
    cp->baseCreateParams.notifyNextLink = FALSE;
    /* Previous link of bitsInHLOS is bitsOutRTOS. So, notifyPrevLink
     * should be set to false if bitsInHLOS is to operate in
     * NO_NOTIFY_MODE
     */
    cp->baseCreateParams.notifyPrevLink = TRUE;
}

Void Chains_ipcBitsInitCreateParams_BitsInRTOS(IpcBitsInLinkRTOS_CreateParams *cp,
                                                Bool notifyNextLink)
{
    /* Previous link of bitsInRTOS is bitsOutHLOSE. So, notifyPrevLink
     * should be set to false if bitsOutHLOS is to operate in
     * NO_NOTIFY_MODE
     */
    cp->baseCreateParams.noNotifyMode = gChains_ipcBitsCtrl.noNotifyBitsOutHLOS;
    cp->baseCreateParams.notifyNextLink = notifyNextLink;
    cp->baseCreateParams.notifyPrevLink = FALSE;
}

Void Chains_ipcBitsInitSetBitsInNoNotifyMode(Bool noNotifyMode)
{
    gChains_ipcBitsCtrl.noNotifyBitsInHLOS = noNotifyMode;
}

Void Chains_ipcBitsInitSetBitsOutNoNotifyMode(Bool noNotifyMode)
{
    gChains_ipcBitsCtrl.noNotifyBitsOutHLOS = noNotifyMode;
}

Void Chains_ipcBitsInitCreateParams_BitsOutHLOS(IpcBitsOutLinkHLOS_CreateParams *cp,
                                                System_LinkQueInfo *inQueInfo)
{
    /* Next link of bitsOutRTOS is bitsInHLOS. So, notifyPrevLink
     * should be set to false if bitsInHLOS is to operate in
     * NO_NOTIFY_MODE
     */
    cp->baseCreateParams.noNotifyMode = !(gChains_ipcBitsCtrl.noNotifyBitsOutHLOS);
    cp->baseCreateParams.notifyNextLink = !(gChains_ipcBitsCtrl.noNotifyBitsOutHLOS);
    cp->baseCreateParams.notifyPrevLink = FALSE;
    cp->inQueInfo = *inQueInfo;
}

Void Chains_ipcBitsInitCreateParams_BitsOutRTOS(IpcBitsOutLinkRTOS_CreateParams *cp,
                                                Bool notifyPrevLink)
{
    /* Next link of bitsOutRTOS is bitsInHLOS. So, notifyPrevLink
     * should be set to false if bitsInHLOS is to operate in
     * NO_NOTIFY_MODE
     */
    cp->baseCreateParams.noNotifyMode = gChains_ipcBitsCtrl.noNotifyBitsInHLOS;
    cp->baseCreateParams.notifyNextLink = !(gChains_ipcBitsCtrl.noNotifyBitsOutHLOS);
    cp->baseCreateParams.notifyPrevLink = notifyPrevLink;
}

static
Bool Chains_ipcBitsDirectoryExists( const char* absolutePath )
{

    if(access( absolutePath, F_OK ) == 0 ){

        struct stat status;
        stat( absolutePath, &status );

        return (status.st_mode & S_IFDIR) != 0;
    }
    return FALSE;
}

static
Int   Chains_ipcBitsOpenFileHandles()
{
    OSA_printf(" Here is Chains_ipcBitsOpenFileHandles\n");//--Sue 20120115
    Int status = OSA_SOK;
    Int i;
    char fileNameHdr[128];
    char fileNameBuffer[128];


    for (i = 0; i < CHAINS_IPC_BITS_MAX_NUM_CHANNELS; i++)
    {
    //fileNameHdr  赋值(起文件名，后缀.bin)
        Chains_ipcBitsGenerateFileName(gChains_ipcBitsCtrl.fObj.fileDirPath,
                                       CHAINS_IPC_BITS_HDR_FILE_NAME,
                                       i,
                                       CHAINS_IPC_BITS_FILE_EXTENSION,
                                       fileNameHdr,
                                       sizeof(fileNameHdr));
	//创建并打开文件
        gChains_ipcBitsCtrl.fObj.fpWrHdr[i] = fopen(fileNameHdr,"wb");
	
        OSA_assert(gChains_ipcBitsCtrl.fObj.fpWrHdr[i] != NULL);
        status =  setvbuf(gChains_ipcBitsCtrl.fObj.fpWrHdr[i],
                          NULL,
                          _IOFBF,
                          CHAINS_IPC_BITS_FILEBUF_SIZE_HDR);//创建缓冲
        OSA_assert(status != -1);

	Chains_ipcBitsGenerateFileName(gChains_ipcBitsCtrl.fObj.fileDirPath,
                                       CHAINS_IPC_BITS_DATA_FILE_NAME,
                                       i,
                                       CHAINS_IPC_BITS_FILE_EXTENSION,
                                       fileNameBuffer,
                                       sizeof(fileNameBuffer));

       

        gChains_ipcBitsCtrl.fObj.fpWrData[i] = fopen(fileNameBuffer,"wb");
        OSA_assert(gChains_ipcBitsCtrl.fObj.fpWrData[i] != NULL);
        status =  setvbuf(gChains_ipcBitsCtrl.fObj.fpWrData[i],
                          NULL,
                          _IOFBF,
                          CHAINS_IPC_BITS_FILEBUF_SIZE_DATA);//创建缓冲
        OSA_assert(status != -1);

    }
    return status;
}

static
Int   Chains_ipcBitsInitFileHandles()
{
    Int i;
    for (i = 0; i < CHAINS_IPC_BITS_MAX_NUM_CHANNELS; i++)
    {
        CHAINS_IPC_BITS_INIT_FILEHANDLE (gChains_ipcBitsCtrl.fObj.fpWrHdr[i]);
        CHAINS_IPC_BITS_INIT_FILEHANDLE (gChains_ipcBitsCtrl.fObj.fpWrData[i]);
    }
    return OSA_SOK;
}

static Int32 Chains_ipcBitsInitFObj()
{
    static Bool fileDirInputDone = FALSE;

    Chains_ipcBitsInitFileHandles();
#if (CHAINS_IPC_BITS_DISABLE_USER_INPUT != TRUE)
    if (!fileDirInputDone)
    {
    strcpy(gChains_ipcBitsCtrl.fObj.fileDirPath,"/media/mmcblk0p2");//--2012/1/14 Sue
        if (Chains_ipcBitsDirectoryExists(gChains_ipcBitsCtrl.fObj.fileDirPath) == 0)
        {
        //如果设置路径打不开存入默认路径/dev/shm
            OSA_printf(" CHAINS:INVALID DIR PATH!!!!\n"
                       "\nUsing default file store path:%s \n",
                       CHAINS_IPC_BITS_FILE_STORE_DIR);
            strncpy(gChains_ipcBitsCtrl.fObj.fileDirPath,CHAINS_IPC_BITS_FILE_STORE_DIR,
                    (sizeof(gChains_ipcBitsCtrl.fObj.fileDirPath) - 1));
            gChains_ipcBitsCtrl.fObj.fileDirPath[(sizeof(gChains_ipcBitsCtrl.fObj.fileDirPath) - 1)] = 0;
            fileDirInputDone = TRUE;
        }
    }
#else
        //这里gChains_ipcBitsCtrl.fObj.fileDirPath全写成了gChains_ipcBitsCtrl.fileDirPath
        //已修改
    OSA_printf("\n\nCHAINS:Using default file store path:%s \n",
               CHAINS_IPC_BITS_FILE_STORE_DIR);
    strncpy(gChains_ipcBitsCtrl.fObj.fileDirPath,CHAINS_IPC_BITS_FILE_STORE_DIR,
            (sizeof(gChains_ipcBitsCtrl.fObj.fileDirPath) - 1));
    gChains_ipcBitsCtrl.fObj.fileDirPath[(sizeof(gChains_ipcBitsCtrl.fObj.fileDirPath) - 1)] = 0;
#endif
    OSA_printf("\n\nCHAINS:Selected File store path:%s",gChains_ipcBitsCtrl.fObj.fileDirPath);
    Chains_ipcBitsOpenFileHandles();
    return OSA_SOK;
}

Int32 Chains_ipcBitsInit()
{
    char opMode[128];
	

   // OSA_printf("CHAINS:Enable file write :(y -- yes/n -- no):");
    //fflush(stdin);
    //fscanf(stdin,"%s",&(opMode[0]));

   //本地存储设置--2013/1/7  Sue
    opMode[0]='y';//--2013/1/7  Sue
   
	if (strcmp("y",opMode) == 0)
    {
        gChains_ipcBitsCtrl.fObj.enableFWrite = TRUE;
    }
    else
    {
        gChains_ipcBitsCtrl.fObj.enableFWrite = FALSE;
    }
/*
//本地存储直接拿到Chains_ipcBitsInitThrObj()中处理
    if (gChains_ipcBitsCtrl.fObj.enableFWrite == TRUE){
        Chains_ipcBitsInitFObj();
    }
*/
    Chains_ipcBitsInitThrObj(&gChains_ipcBitsCtrl.thrObj);

//GUO  for RTSP
   
 
    return OSA_SOK;
}


//******dyx20131108******
Int32 Chains_ipcBitsLocSt()
{
    char opMode[128];
	

   // OSA_printf("CHAINS:Enable file write :(y -- yes/n -- no):");
    //fflush(stdin);
    //fscanf(stdin,"%s",&(opMode[0]));

   //本地存储设置--2013/1/7  Sue
    opMode[0]='y';//--2013/1/7  Sue
   
	if (strcmp("y",opMode) == 0)
    {
        gChains_ipcBitsCtrl.fObj.enableFWrite = TRUE;
    }
    else
    {
        gChains_ipcBitsCtrl.fObj.enableFWrite = FALSE;
    }
/*
//本地存储直接拿到Chains_ipcBitsInitThrObj()中处理
    if (gChains_ipcBitsCtrl.fObj.enableFWrite == TRUE){
        Chains_ipcBitsInitFObj();
    }
*/
    Chains_ipcBitsLocStThrObj(&gChains_ipcBitsCtrl.thrObj);
 
    return OSA_SOK;
}



Void Chains_ipcBitsStop(void)
{
    gChains_ipcBitsCtrl.thrObj.exitBitsInThread = TRUE;
    gChains_ipcBitsCtrl.thrObj.exitBitsOutThread = TRUE;
	
}

//******dyx20131108******
Void Chains_ipcBitsLocStStop(void)
{
    gChains_ipcBitsCtrl.thrObj.exitBitsInThread = FALSE;
   gChains_ipcBitsCtrl.thrObj.exitBitsOutThread = FALSE;
    OSA_thrDelete(&gChains_ipcBitsCtrl.thrObj);
    //guo repair debug/
 //   OSA_thrDelete(&gChains_ipcBitsCtrl.thrObj.thrHandleBitsIn);
	//added for rtsp
  // if(gChains_ctrl.channelConf[0].audioEnable)
 // 	OSA_thrDelete(&gChains_ipcBitsCtrl.thrObj.thrHandleBitsInA);//segemention err
	
}

Int32 Chains_ipcBitsExit()
{
    OSA_printf("Entered:%s...",__func__);
    if (gChains_ipcBitsCtrl.fObj.enableFWrite == TRUE){
        Chains_ipcBitsInitFileHandles();
    }
    Chains_ipcBitsDeInitThrObj(&gChains_ipcBitsCtrl.thrObj);
    OSA_printf("Leaving:%s...",__func__);
    return OSA_SOK;
}


