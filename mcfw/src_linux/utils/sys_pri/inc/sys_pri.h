
#ifndef _SYS_PRI_H_
#define _SYS_PRI_H_

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#define REG32 volatile unsigned int

#define TRUE  1
#define FALSE 0

#define REG_INIT_PRESSURE_BASE_PHYS   (0x48140000)  
#define REG_INIT_PRESSURE_0_PHYS_OFF  (0x00000608)      
#define REG_INIT_PRESSURE_SIZE        (0x00001000)
#if defined(TI_814X_BUILD) || defined(TI_8107_BUILD)
#define REG_BW_REG_BASE_PHYS          (0x44400000)
#else
#define REG_BW_REG_BASE_PHYS          (0x44000000)  
#endif
#define REG_BW_REG_HDVICP0_PHYS_OFF   (0x00001C08)
#define REG_BW_REG_HDVICP0_SIZE       (0x00000100)
#define REG_BW_REG_SIZE               (0x00003000)

#define REG_DMM_PEG_PRI_BASE_PHYS     (0x4E000000)
#define REG_DMM_PEG_PRI0_PHYS_OFF     (0x00000620)
#define REG_DMM_PEG_SIZE              (0x00001000)

#define BW_REG_HDVICP0  (0)
#define BW_REG_HDVICP1  (1)
#define BW_REG_HDVICP2  (2)
#define BW_REG_EDMA_RD0 (3)
#define BW_REG_EDMA_WR0 (4)
#define BW_REG_EDMA_RD1 (5)
#define BW_REG_EDMA_WR1 (6)
#define BW_REG_DSP      (7)
#define BW_REG_SGX      (8)
#define BW_REG_MAX      (9)

typedef struct 
{
  REG32 bandwidth;
  REG32 watermark;
  REG32 pressure;
  REG32 clearHistory;
    
} SYS_PRI_BwReg;

typedef struct
{
  REG32 *initPressure[2];
  volatile SYS_PRI_BwReg *bwReg[BW_REG_MAX];
  REG32 *dmmPegPri[8];
  
  REG32 *initPressureMmapAddr;  
  REG32 *bwRegMmapAddr;
  REG32 *dmmPegPriMmapAddr;  
  
  int doRegDump;
  
  int initPressureA8      ;
  int initPressureDSP_CFG ;
  int initPressureHDVPSS0 ;
  int initPressureHDVPSS1 ;
  int initPressureEDMARD2 ;
  int initPressureEDMAWR2 ;
  int initPressureEDMARD3 ;
  int initPressureEDMAWR3 ;
  int initPressureGMAC0   ;
  int initPressureGMAC1   ;
  int initPressureUSB_DMA ;
  int initPressureUSB_QMGR;
  int initPressureSATA    ;
  int initPressureDUCATI  ;
  int initPressurePCIE    ;

  int dmmPegPriA8      ;
  int dmmPegPriSYS_MMU ;
  int dmmPegPriDUCATI  ; 
  int dmmPegPriEDMARD0 ;
  int dmmPegPriEDMARD1 ;
  int dmmPegPriEDMARD2 ;
  int dmmPegPriEDMARD3 ; 
  int dmmPegPriEDMAWR0 ; 
  int dmmPegPriEDMAWR1 ; 
  int dmmPegPriEDMAWR2 ; 
  int dmmPegPriEDMAWR3 ; 
  int dmmPegPriSGX     ;
  int dmmPegPriHDVICP0 ;
  int dmmPegPriHDVICP1 ; 
  int dmmPegPriHDVICP2 ; 
  int dmmPegPriGMAC0   ;
  int dmmPegPriGMAC1   ;
  int dmmPegPriUSB_DMA ; 
  int dmmPegPriUSB_QMGR; 
  int dmmPegPriSATA    ;
  int dmmPegPriPCIE    ;

} SYS_PRI_Ctrl;

int xstrtoi(char *hex);      // hex string to integer

int SYS_PRI_dmmPriSet(char *name, int pri);
int SYS_PRI_unmapMem();
int SYS_PRI_mapMem();

#endif
