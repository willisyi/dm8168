
#include <osa.h>
#include <osa_dma.h>
#include <dev_dma.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>


typedef struct {

  int fd;
  Bool isInitDone;

} OSA_DmaCtrl;

OSA_DmaCtrl gOSA_dmaCtrl = { -1, FALSE };

int OSA_dmaInit()
{
  char deviceName[20];
     
  if(!gOSA_dmaCtrl.isInitDone) {

    sprintf(deviceName, "/dev/%s", DMA_DRV_NAME);

    gOSA_dmaCtrl.fd = open(deviceName, O_RDWR);
    if(gOSA_dmaCtrl.fd<0)
      return OSA_EFAIL;
    
    gOSA_dmaCtrl.isInitDone = TRUE;
  }
  
  return OSA_SOK;
}

int OSA_dmaExit()
{
  gOSA_dmaCtrl.isInitDone = FALSE;
  return close(gOSA_dmaCtrl.fd);
}

int OSA_dmaOpen(OSA_DmaChHndl *hndl, int dmaMode, int maxTransfers)
{
  DMA_OpenClosePrm prm;
  Uint32 cmd;
  int status;
    
  prm.chId = -1;
  prm.mode = dmaMode;
  prm.maxTransfers = maxTransfers;
  
  cmd = DMA_IOCTL_CMD_MAKE(DMA_CMD_CH_OPEN);
  status = ioctl(gOSA_dmaCtrl.fd, cmd, &prm);
  
  if(status==0)
    hndl->chId = prm.chId;
  else  
    hndl->chId = -1;
  
  return status;
}

int OSA_dmaClose(OSA_DmaChHndl *hndl) {
  DMA_OpenClosePrm prm;
  Uint32 cmd;
  int status;
  
  if(hndl->chId < 0)
    return OSA_EFAIL;
    
  prm.chId = hndl->chId;
  prm.mode = 0;
  prm.maxTransfers = 0;
  
  cmd = DMA_IOCTL_CMD_MAKE(DMA_CMD_CH_CLOSE);
  status = ioctl(gOSA_dmaCtrl.fd, cmd, &prm);
  
  hndl->chId = -1;
  
  return status;
}

int OSA_dmaCopyFillRun(OSA_DmaChHndl *hndl, void *pPrm, int count, int copyFillType)
{
  DMA_CopyFillPrm prm;
  Uint32 cmd;
  int status;

  if(hndl->chId < 0)
    return OSA_EFAIL;
  
  prm.chId = hndl->chId;
  prm.count = count;
  prm.copyFillType = copyFillType;
  prm.prm = pPrm;

  cmd = DMA_IOCTL_CMD_MAKE(prm.copyFillType);
  status = ioctl(gOSA_dmaCtrl.fd, cmd, &prm);
  
  return status;
}

int OSA_dmaCopy2D(OSA_DmaChHndl *hndl, OSA_DmaCopy2D *prm, int count )
{
  return OSA_dmaCopyFillRun(hndl, prm, count, DMA_CMD_COPY_2D);
}

int OSA_dmaCopy1D(OSA_DmaChHndl *hndl, OSA_DmaCopy1D *prm, int count )
{
  return OSA_dmaCopyFillRun(hndl, prm, count, DMA_CMD_COPY_1D);
}

int OSA_dmaFill2D(OSA_DmaChHndl *hndl, OSA_DmaFill2D *prm, int count )
{
  return OSA_dmaCopyFillRun(hndl, prm, count, DMA_CMD_FILL_2D);
}

int OSA_dmaFill1D(OSA_DmaChHndl *hndl, OSA_DmaFill1D *prm, int count )
{
  return OSA_dmaCopyFillRun(hndl, prm, count, DMA_CMD_FILL_1D);
}

Uint8 *OSA_dmaMapMem(Uint8 *physAddr, Uint32 size, Uint32 mapType)
{
    DMA_MmapPrm prm;
    Uint32 cmd;
    int status;
  
    prm.physAddr = (unsigned int)physAddr;
    prm.size     = size;
    prm.mapType  = mapType;
    prm.virtAddr = 0;
  
    cmd = DMA_IOCTL_CMD_MAKE(DMA_CMD_MMAP);
    status = ioctl(gOSA_dmaCtrl.fd, cmd, &prm);

	if (prm.virtAddr==0)
  {
		OSA_ERROR(" OSA_dmaMapMem() failed !!!\n");
    return NULL;
  }
 
	prm.virtAddr = (UInt32)mmap(
	        (void	*)physAddr,
	        size,
					PROT_READ|PROT_WRITE|PROT_EXEC,MAP_SHARED,
					gOSA_dmaCtrl.fd,
					(unsigned int)physAddr
					);

    return (Uint8*)prm.virtAddr;
}

int OSA_dmaUnmapMem(Uint8 *virtAddr, Uint32 size)
{
  if(virtAddr)
    munmap((void*)virtAddr, size);    
    
  return 0;
}


#include <osa_prf.h>

int OSA_dmaTestCopyFill(Uint8 *srcPhysAddr, Uint8 *dstPhysAddr, int size, int width, int height)
{
  int status, diff;
  OSA_DmaChHndl dmaHndl;  
  OSA_DmaCopy2D copy2D;
  OSA_DmaCopy1D copy1D;  
  OSA_PrfHndl dmaPrf;
  int count=1, k, i;
  Uint32 *pCurAddr;
  Uint8 *srcVirtAddr, *dstVirtAddr;
  
  OSA_printf(" DMA: DMA Test (SRC = 0x%08x, DST = 0x%08x)!!!\n", (Uint32)srcPhysAddr, (Uint32)dstPhysAddr);  
    
  if(srcPhysAddr==NULL||dstPhysAddr==NULL)
  {
    OSA_printf(" DMA: Illegal SRC or DST memory physical addresses (0x%08x, 0x%08x)!!!\n", (Uint32)srcPhysAddr, (Uint32)dstPhysAddr);    
    return -1;
  }
  
  srcVirtAddr = OSA_dmaMapMem(srcPhysAddr, size, OSA_DMA_MMAP_TYPE_NOCACHE);
  dstVirtAddr = OSA_dmaMapMem(dstPhysAddr, size, OSA_DMA_MMAP_TYPE_NOCACHE);

  if(srcVirtAddr==NULL||dstVirtAddr==NULL)
  {
    OSA_printf(" DMA: Illegal SRC or DST memory physical addresses (0x%08x, 0x%08x)!!!\n", (Uint32)srcPhysAddr, (Uint32)dstPhysAddr);    
    return -1;
  }

  memset(srcVirtAddr, 0, size);
  memset(dstVirtAddr, 0x55, size);  
  
  pCurAddr = (Uint32*)srcVirtAddr;
  for(i=0; i<size/4; i++) {
    *pCurAddr++ = i;
  }
    
  OSA_dmaInit();
   
  status = OSA_dmaOpen(&dmaHndl, OSA_DMA_MODE_NORMAL, 1);  
  if(status==OSA_SOK) {
  
    #if 1
    copy2D.srcPhysAddr = (unsigned long)srcPhysAddr;
    copy2D.dstPhysAddr = (unsigned long)dstPhysAddr;
    
    copy2D.copyWidth   = width;
    copy2D.copyHeight  = height;
    
    copy2D.srcOffsetH = width;
    copy2D.dstOffsetH = width;
    
    copy2D.skipH = 0; 

    OSA_prfReset(&dmaPrf);
    OSA_prfBegin(&dmaPrf);    

    for(k=0; k<count; k++)       
      OSA_dmaCopy2D(&dmaHndl, &copy2D, 1);
    
    OSA_prfEnd(&dmaPrf, width*height*count);        
    
    diff = memcmp(srcVirtAddr, dstVirtAddr, width*height);
    
    OSA_printf("\n");    
    if(diff) {
      OSA_printf(" DMA: OSA_dmaCopy2D() Verify ERROR (%d)\n", diff);
    } else {
      OSA_printf(" DMA: OSA_dmaCopy2D() Verify SUCCESS \n");    
    }
    
    OSA_prfPrint(&dmaPrf, "DMA Copy2D", OSA_PRF_PRINT_DEFAULT);    
    #endif
    
    #if 1
    memset(dstVirtAddr, 0x55, size);    
        
    copy1D.srcPhysAddr = (unsigned long)srcPhysAddr;
    copy1D.dstPhysAddr = (unsigned long)dstPhysAddr;
    
    copy1D.size   = size;

    OSA_prfReset(&dmaPrf); 
    OSA_prfBegin(&dmaPrf);    
    
    for(k=0; k<count; k++)       
      OSA_dmaCopy1D(&dmaHndl, &copy1D, 1);
    
    OSA_prfEnd(&dmaPrf, copy1D.size*count);        
    
    diff = memcmp(srcVirtAddr, dstVirtAddr, copy1D.size);
    
    OSA_printf("\n");    
    if(diff) {
      OSA_printf(" DMA: OSA_dmaCopy1D() Verify ERROR (%d)\n", diff);
    } else {
      OSA_printf(" DMA: OSA_dmaCopy1D() Verify SUCCESS \n");    
    }

    OSA_prfPrint(&dmaPrf, "DMA Copy1D", OSA_PRF_PRINT_DEFAULT);
    
    #endif       
    
    status = OSA_dmaClose(&dmaHndl);
  }

  OSA_dmaExit();
  
  return status;
}


