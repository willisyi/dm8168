
#ifndef _OSA_DMA_H_
#define _OSA_DMA_H_

#define OSA_DMA_NON_CACHE_ALLOC_BASE_PHYS_ADDR    (0x90000000)

#define OSA_DMA_MODE_NORMAL     0   // normal memcpy or memfill type DMA's

#define OSA_DMA_MAX_NORMAL_TRANSFERS    (16)


#define OSA_DMA_MMAP_TYPE_NOCACHE           (0)
#define OSA_DMA_MMAP_TYPE_CACHE             (1)
#define OSA_DMA_MMAP_TYPE_WRITE_COMBINED    (2)

typedef struct {

  unsigned long srcPhysAddr;
  unsigned long dstPhysAddr;
  
  int copyWidth;   // in bytes
  int copyHeight;  // in lines
  
  int srcOffsetH;  // in bytes
  int dstOffsetH;  // in bytes
  
  int skipH; // Horizontal Skip, 0: No skip, >= 1 copy 'skipH' bytes, skip 'skipH' bytes. Eg to skip alternate bytes set skipH to 1  
  
} OSA_DmaCopy2D;

typedef struct {

  unsigned long srcPhysAddr;
  unsigned long dstPhysAddr;
  
  int size;  // in bytes
  
} OSA_DmaCopy1D;

typedef struct {

  int fillValue;  // 32-bit fill value
  
  unsigned long dstPhysAddr;
  unsigned long dstVirtAddr;
  
  int copyWidth;   // in bytes, must be multiple of 4 bytes, must be >= 16 bytes
  int copyHeight;  // in lines

  int dstOffsetH; // in bytes, must be multiple of 16 bytes
  
} OSA_DmaFill2D;

typedef struct {

  int fillValue; // 32-bit fill value
  
  unsigned long dstPhysAddr;  
  unsigned long dstVirtAddr;  
  
  int size; // in bytes, must be multiple of 4 bytes, must be >= 32 bytes
  
} OSA_DmaFill1D;

#ifndef KERNEL_LEVEL
// only used in user space

typedef struct {

  int chId; // DMA channel ID, initialzed internally when OSA_dmaOpen() is called

} OSA_DmaChHndl;

int OSA_dmaInit();
int OSA_dmaExit();

int OSA_dmaOpen(OSA_DmaChHndl *hndl, int dmaMode, int maxTransfers);
int OSA_dmaClose(OSA_DmaChHndl *hndl);

// count must less <= maxTransfers
int OSA_dmaCopy2D(OSA_DmaChHndl *hndl, OSA_DmaCopy2D *prm, int count );
int OSA_dmaCopy1D(OSA_DmaChHndl *hndl, OSA_DmaCopy1D *prm, int count );
int OSA_dmaFill2D(OSA_DmaChHndl *hndl, OSA_DmaFill2D *prm, int count );
int OSA_dmaFill1D(OSA_DmaChHndl *hndl, OSA_DmaFill1D *prm, int count );

unsigned char *OSA_dmaMapMem(unsigned char *physAddr, unsigned int size, unsigned int mapType);
int OSA_dmaUnmapMem(unsigned char *virtAddr, unsigned int size);

#endif  /* KERNEL_LEVEL */ 

#endif  /* _OSA_DMA_H_ */

