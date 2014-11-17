#ifndef _SHARE_MEM__H
#define  _SHARE_MEM__H

#define BUF_MAX_SZ 500000
#define SHM_KEY 33
#define SHM_KEY2 34
//static const int SEM_KEYA[4] = {1220,1221,1222,1223};
//static const int SEM_KEYB[4] = {1224,1225,1226,1227};
//static const int SHM_KEYV[4] = {33,34,35,36};
//#define SHM_KEY_A 99

int ShareMemInit(int shmid,int key);  //intial shared memory
void Del_ShareMem(int shmid);


typedef struct VideoBuf {
    int     I_Frame;
    int     samplingRate;
    int     timestamp;
    int     data_size;
    char    buf_encode[BUF_MAX_SZ];
} VideoBuf;


typedef struct AudioBuf {
    unsigned char   numChannels;
    int     samplingRate;
    int     timestamp;
    int      data_size;
    char        buf_encode[3000];
} AudioBuf;


typedef struct shared_use{ 
     VideoBuf  frame;
     AudioBuf  buf;
}shared_use;
#endif



