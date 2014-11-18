
#include <sys_pri.h>

//#define L3_STATIC_PRESSURE_SET_ENABLE

SYS_PRI_Ctrl gSYS_PRI_ctrl;

int SYS_PRI_isValidBwReg(int bwRegId)
{
  #if defined(TI_814X_BUILD) || defined(TI_8107_BUILD)
  if(bwRegId==BW_REG_HDVICP1)
    return 0;
  if(bwRegId==BW_REG_HDVICP2)
    return 0;

    #if defined(TI_8107_BUILD)
    if(bwRegId==BW_REG_DSP)
        return 0;
    if(bwRegId==BW_REG_SGX)
        return 0;
    if(bwRegId==BW_REG_EDMA_RD0)
        return 0;
    if(bwRegId==BW_REG_EDMA_WR0)
        return 0;
    if(bwRegId==BW_REG_EDMA_RD1)
        return 0;
    if(bwRegId==BW_REG_EDMA_WR1)
        return 0;
    #endif
  #endif

  return 1;
}

int SYS_PRI_printCmdLineArgs(char *str)
{
  printf("\n");
  printf(" TI816x System Prioirty read/write utility, (c) Texas Instruments 2010\n");
  printf(" Built on %s %s\n", __DATE__, __TIME__);
  printf(" \n");
  printf(" %s --printAll # Prints all system priority setting info\n", str);
  printf(" %s --regDump  # Prints all register values related to system priority setting\n", str);
  printf(" \n");
  printf(" %s --dmm-pri-set <dmm-initiator-name> <dmm-priority> \n"
         " # Set a DMM priority for a specified DMM initiator \n", str);
  printf(" \n");
  #ifdef L3_STATIC_PRESSURE_SET_ENABLE
  printf(" %s --L3-pri-set <L3-initiator-name> <L3-static-pressure> \n"
         " # Set a L3 static pressure for a specified L3 initiator \n", str);
  printf(" \n");
  #endif
  printf(" %s --L3-bw-reg-set <L3-bw-reg-initiator-name> <L3-pressure-High> <L3-pressure-Low> <L3-Bandwidth> <L3-Watermark-cycles> \n"
         " # Setup L3 Bandwidth regulator for a specified L3 initiator \n",
                str);
  printf(" \n");
  printf(" Valid values for \"dmm-initiator-name\" are,\n");
  printf(" A8       SYS_MMU  DUCATI   EDMA_RD0 EDMA_RD1 EDMA_RD2 EDMA_RD3\n");
  printf(" EDMA_WR0 EDMA_WR1 EDMA_WR2 EDMA_WR3 SGX      HDVICP0  HDVICP1 \n");
  printf(" HDVICP2  GMAC0    GMAC1    USB_DMA  USB_QMGR SATA     PCIE    \n");
  printf(" \n");
  #ifdef L3_STATIC_PRESSURE_SET_ENABLE
  printf(" Valid values for \"L3-initiator-name\" are,\n");
  printf(" A8       DSP_CFG  HDVPSS0  HDVPSS1  EDMA_RD2 EDMA_WR2 EDMA_RD3\n");
  printf(" EDMA_WR3 GMAC0    GMAC1    USB_DMA  USB_QMGR SATA     DUCATI  \n");
  printf(" PCIE \n");
  printf(" \n");
  #endif
  printf(" Valid values for \"L3-bw-reg-initiator-name\" are,\n");
  printf(" HDVICP0  HDVICP1  HDVICP2  EDMA_RD0 EDMA_WR0 EDMA_RD1 EDMA_WR1\n");
  printf(" DSP      SGX   \n");
  printf(" \n");
  printf(" Valid values for \"dmm-priority\"       are, 0..7 (0 is highest priority)\n");
  #ifdef L3_STATIC_PRESSURE_SET_ENABLE
  printf(" Valid values for \"L3-static-pressure\" are, 0, 1, 3 (0 is lowest priority)\n");
  #endif
  printf(" Valid values for \"L3-pressure-High\"   are, 0, 1, 3 (0 is lowest priority)\n");
  printf(" Valid values for \"L3-pressure-Low\"    are, 0, 1, 3 (0 is lowest priority)\n");
  printf(" Valid values for \"L3-Bandwidth\" (in MB/s) are, 0..%u\n", 0x1FFF);
  printf(" Valid values for \"L3-Watermark\" (in cycles) are, 0..%d\n", 0xFFF);
  printf(" \n");
  printf(" IMPORTANT NOTE: \n"
         " - L3 Static pressure cannot be set via this utility.\n"
         " - Processor needs to be in supervisor mode to set L3 static priority.\n"
         " - L3 Static pressure can only be read via this utility.\n"
         );
  printf(" \n");

  return 0;
}

int SYS_PRI_mapMem()
{
  int memDevFd, i;

  memDevFd = open("/dev/mem",O_RDWR|O_SYNC);

  if(memDevFd < 0)
  {
    printf(" ERROR: /dev/mem open failed !!!\n");
    return -1;
  }

	gSYS_PRI_ctrl.initPressureMmapAddr = mmap(
	        (void	*)REG_INIT_PRESSURE_BASE_PHYS,
	        REG_INIT_PRESSURE_SIZE,
					PROT_READ|PROT_WRITE|PROT_EXEC,MAP_SHARED,
					memDevFd,
					REG_INIT_PRESSURE_BASE_PHYS
					);

	if (gSYS_PRI_ctrl.initPressureMmapAddr==NULL)
	{
		printf(" ERROR: mmap() failed !!!\n");
		return -1;
	}

	gSYS_PRI_ctrl.bwRegMmapAddr = mmap(
	        (void	*)REG_BW_REG_BASE_PHYS,
	        REG_BW_REG_SIZE,
					PROT_READ|PROT_WRITE|PROT_EXEC,MAP_SHARED,
					memDevFd,
					REG_BW_REG_BASE_PHYS
					);

	if (gSYS_PRI_ctrl.bwRegMmapAddr==NULL)
	{
		printf(" ERROR: mmap() failed !!!\n");
		return -1;
	}

	gSYS_PRI_ctrl.dmmPegPriMmapAddr = mmap(
	        (void	*)REG_DMM_PEG_PRI_BASE_PHYS,
	        REG_DMM_PEG_SIZE,
					PROT_READ|PROT_WRITE|PROT_EXEC,MAP_SHARED,
					memDevFd,
					REG_DMM_PEG_PRI_BASE_PHYS
					);

	if (gSYS_PRI_ctrl.dmmPegPriMmapAddr==NULL)
	{
		printf(" ERROR: mmap() failed !!!\n");
		return -1;
	}

  close(memDevFd);

  gSYS_PRI_ctrl.initPressure[0] = gSYS_PRI_ctrl.initPressureMmapAddr + (REG_INIT_PRESSURE_0_PHYS_OFF/sizeof(REG32));
  gSYS_PRI_ctrl.initPressure[1] = (gSYS_PRI_ctrl.initPressure[0]+1);

  if(gSYS_PRI_ctrl.doRegDump)
  {
    printf(" \n");
    printf(" INIT_PRESSURE[0] @ 0x%08x = 0x%08x\n", (REG32)gSYS_PRI_ctrl.initPressure[0], *(REG32*)gSYS_PRI_ctrl.initPressure[0]);
    printf(" INIT_PRESSURE[1] @ 0x%08x = 0x%08x \n", (REG32)gSYS_PRI_ctrl.initPressure[1], *(REG32*)gSYS_PRI_ctrl.initPressure[1]);
  }

  if(gSYS_PRI_ctrl.doRegDump)
    printf(" \n");

  for(i=0; i<BW_REG_MAX; i++)
  {
    gSYS_PRI_ctrl.bwReg[i] = (SYS_PRI_BwReg*)((REG32)gSYS_PRI_ctrl.bwRegMmapAddr + REG_BW_REG_HDVICP0_PHYS_OFF + REG_BW_REG_HDVICP0_SIZE*i);

    if(gSYS_PRI_ctrl.doRegDump)
    {
      printf(" BW_REGULATOR[%d]  @ 0x%08x = 0x%08x 0x%08x 0x%08x 0x%08x\n", i, (REG32)gSYS_PRI_ctrl.bwReg[i],
                  gSYS_PRI_ctrl.bwReg[i]->bandwidth,
                  gSYS_PRI_ctrl.bwReg[i]->watermark,
                  gSYS_PRI_ctrl.bwReg[i]->pressure,
                  gSYS_PRI_ctrl.bwReg[i]->clearHistory
                  );
    }
  }

  if(gSYS_PRI_ctrl.doRegDump)
    printf(" \n");

  for(i=0; i<8; i++)
  {
    gSYS_PRI_ctrl.dmmPegPri[i] = (REG32*)((REG32)gSYS_PRI_ctrl.dmmPegPriMmapAddr + REG_DMM_PEG_PRI0_PHYS_OFF + sizeof(REG32)*i);

    if(gSYS_PRI_ctrl.doRegDump)
      printf(" DMM_PEG_PRI[%d]   @ 0x%08x = 0x%08x \n", i, (REG32)gSYS_PRI_ctrl.dmmPegPri[i], *(REG32*)gSYS_PRI_ctrl.dmmPegPri[i]);
  }

  if(gSYS_PRI_ctrl.doRegDump)
    printf(" \n");

  return 0;
}

int SYS_PRI_unmapMem()
{
  if(gSYS_PRI_ctrl.initPressureMmapAddr)
    munmap((void*)gSYS_PRI_ctrl.initPressureMmapAddr, REG_BW_REG_SIZE);

  if(gSYS_PRI_ctrl.bwRegMmapAddr)
    munmap((void*)gSYS_PRI_ctrl.bwRegMmapAddr, REG_BW_REG_SIZE);

  if(gSYS_PRI_ctrl.dmmPegPriMmapAddr)
    munmap((void*)gSYS_PRI_ctrl.dmmPegPriMmapAddr, REG_DMM_PEG_SIZE);

  return 0;
}

int SYS_PRI_dmmPriSet(char *name, int pri)
{
  REG32 value, shift;
  REG32 *pAddr;
  int status, setReg;

  status = SYS_PRI_mapMem();
  if(status!=0)
    SYS_PRI_unmapMem();

  setReg = 1;
  value = 0x8 | (pri & 0x7);
  pAddr = gSYS_PRI_ctrl.dmmPegPri[0];
  shift = 0;

  if(strcmp(name,"A8")==0)
  {
    pAddr = gSYS_PRI_ctrl.dmmPegPri[0];
    shift = 0;
  } else
  if(strcmp(name,"SYS_MMU")==0)
  {
    pAddr = gSYS_PRI_ctrl.dmmPegPri[1];
    shift = 8;
  } else
  if(strcmp(name,"DUCATI")==0)
  {
    pAddr = gSYS_PRI_ctrl.dmmPegPri[1];
    shift = 24;
  } else
  if(strcmp(name,"EDMA_RD0")==0)
  {
    pAddr = gSYS_PRI_ctrl.dmmPegPri[3];
    shift = 0;
  } else
  if(strcmp(name,"EDMA_RD1")==0)
  {
    pAddr = gSYS_PRI_ctrl.dmmPegPri[3];
    shift = 4;
  } else
  if(strcmp(name,"EDMA_RD2")==0)
  {
    pAddr = gSYS_PRI_ctrl.dmmPegPri[3];
    shift = 8;
  } else
  if(strcmp(name,"EDMA_RD3")==0)
  {
    pAddr = gSYS_PRI_ctrl.dmmPegPri[3];
    shift = 12;
  } else
  if(strcmp(name,"EDMA_WR0")==0)
  {
    pAddr = gSYS_PRI_ctrl.dmmPegPri[3];
    shift = 16;
  } else
  if(strcmp(name,"EDMA_WR1")==0)
  {
    pAddr = gSYS_PRI_ctrl.dmmPegPri[3];
    shift = 20;
  } else
  if(strcmp(name,"EDMA_WR2")==0)
  {
    pAddr = gSYS_PRI_ctrl.dmmPegPri[3];
    shift = 24;
  } else
  if(strcmp(name,"EDMA_WR3")==0)
  {
    pAddr = gSYS_PRI_ctrl.dmmPegPri[3];
    shift = 28;
  } else
  if(strcmp(name,"SGX")==0)
  {
    pAddr = gSYS_PRI_ctrl.dmmPegPri[4];
    shift = 0;
  } else
  if(strcmp(name,"HDVICP0")==0)
  {
    pAddr = gSYS_PRI_ctrl.dmmPegPri[5];
    shift = 0;
  } else
  if(strcmp(name,"HDVICP1")==0)
  {
    pAddr = gSYS_PRI_ctrl.dmmPegPri[5];
    shift = 16;
  } else
  if(strcmp(name,"HDVICP2")==0)
  {
    pAddr = gSYS_PRI_ctrl.dmmPegPri[5];
    shift = 20;
  } else
  if(strcmp(name,"GMAC0")==0)
  {
    pAddr = gSYS_PRI_ctrl.dmmPegPri[6];
    shift = 0;
  } else
  if(strcmp(name,"GMAC1")==0)
  {
    pAddr = gSYS_PRI_ctrl.dmmPegPri[6];
    shift = 4;
  } else
  if(strcmp(name,"USB_DMA")==0)
  {
    pAddr = gSYS_PRI_ctrl.dmmPegPri[6];
    shift = 16;
  } else
  if(strcmp(name,"USB_QMGR")==0)
  {
    pAddr = gSYS_PRI_ctrl.dmmPegPri[6];
    shift = 20;
  } else
  if(strcmp(name,"SATA")==0)
  {
    pAddr = gSYS_PRI_ctrl.dmmPegPri[7];
    shift = 0;
  } else
  if(strcmp(name,"PCIE")==0)
  {
    pAddr = gSYS_PRI_ctrl.dmmPegPri[7];
    shift = 4;
  } else
  {
    printf(" ERROR: Unknown DMM initiator [%s] \n", name);
    setReg = 0;
  }

  if(setReg)
  {
    value <<= shift;

    printf(" Setting DMM priority for [%-8s] to [%d] ( 0x%08x = 0x%08x )\n", name, pri, (unsigned int)pAddr, value);

    *pAddr = value;


  }

  SYS_PRI_unmapMem();

  return 0;
}

int SYS_PRI_l3StaticPressureSet(char *name, int staticPressure)
{
  REG32 shift, mask;
  REG32 *pAddr;
  int status, regNum;
  int setReg;

  status = SYS_PRI_mapMem();
  if(status!=0)
    SYS_PRI_unmapMem();

  setReg = 1;
  regNum = 0;
  mask  = 0x3;
  shift = 0;

  if(strcmp(name,"A8")==0)
  {
    regNum = 0;
    shift = 0;
  } else
  if(strcmp(name,"DSP_CFG")==0)
  {
    regNum = 0;
    shift = 4;
  } else
  if(strcmp(name,"HDVPSS0")==0)
  {
    regNum = 0;
    shift = 8;
    mask = 0x1;
  } else
  if(strcmp(name,"HDVPSS1")==0)
  {
    regNum = 0;
    shift = 10;
    mask = 0x1;
  } else
  if(strcmp(name,"EDMA_RD2")==0)
  {
    regNum = 0;
    shift = 24;
  } else
  if(strcmp(name,"EDMA_WR2")==0)
  {
    regNum = 0;
    shift = 26;
  } else
  if(strcmp(name,"EDMA_RD3")==0)
  {
    regNum = 0;
    shift = 28;
  } else
  if(strcmp(name,"EDMA_WR3")==0)
  {
    regNum = 0;
    shift = 30;
  } else
  if(strcmp(name,"GMAC0")==0)
  {
    regNum = 1;
    shift = 0;
  } else
  if(strcmp(name,"GMAC1")==0)
  {
    regNum = 1;
    shift = 2;
  } else
  if(strcmp(name,"USB_DMA")==0)
  {
    regNum = 1;
    shift = 4;
  } else
  if(strcmp(name,"USB_QMGR")==0)
  {
    regNum = 1;
    shift = 6;
  } else
  if(strcmp(name,"SATA")==0)
  {
    regNum = 1;
    shift = 8;
  } else
  if(strcmp(name,"DUCATI")==0)
  {
    regNum = 1;
    shift = 14;
  } else
  if(strcmp(name,"PCIE")==0)
  {
    regNum = 1;
    shift = 16;
  } else
  {
    printf(" ERROR: Unknown L3 static pressure initiator [%s] \n", name);
    setReg = FALSE;
  }

  if(setReg)
  {
    printf(" Setting L3 static pressure for [%-8s] to [%d]\n", name, staticPressure);

    pAddr = gSYS_PRI_ctrl.initPressure[regNum];

    /* clear value */
    *pAddr &= (mask << shift);

    /* set value */
    *pAddr |= ( (staticPressure & mask) << shift);
  }

  SYS_PRI_unmapMem();

  return 0;
}

int SYS_PRI_l3BwRegSet(char *name, int pressH, int pressL, int bandwidth, int watermark)
{
  volatile SYS_PRI_BwReg *pReg;
  int status, bwRegId, setReg;

  status = SYS_PRI_mapMem();
  if(status!=0)
    SYS_PRI_unmapMem();

  setReg = 1;
  bwRegId = 0;

  if(strcmp(name,"HDVICP0")==0)
  {
    bwRegId = 0;
  } else
  if(strcmp(name,"HDVICP1")==0)
  {
    bwRegId = 1;
  } else
  if(strcmp(name,"HDVICP2")==0)
  {
    bwRegId = 2;
  } else
  if(strcmp(name,"EDMA_RD0")==0)
  {
    bwRegId = 3;
  } else
  if(strcmp(name,"EDMA_WR0")==0)
  {
    bwRegId = 4;
  } else
  if(strcmp(name,"EDMA_RD1")==0)
  {
    bwRegId = 5;
  } else
  if(strcmp(name,"EDMA_WR1")==0)
  {
    bwRegId = 6;
  } else
  if(strcmp(name,"DSP")==0)
  {
    bwRegId = 7;
  } else
  if(strcmp(name,"SGX")==0)
  {
    bwRegId = 8;
  } else
  {
    printf(" ERROR: Unknown L3 initiator [%s] \n", name);
    setReg = FALSE;
  }

  if(!SYS_PRI_isValidBwReg(bwRegId))
  {
    printf(" ERROR: L3 initiator [%s] NOT supported on this platform\n", name);
    setReg = FALSE;
  }

  if(setReg)
  {
    printf(" Setting L3 bandwidth regulator for [%-8s] to [press=[%d,%d] BW=%d, WM Cycles=%d]\n",
      name, pressH, pressL, bandwidth, watermark
    );

    bandwidth = (REG32)((float)bandwidth/15.625+0.999);

    pReg = gSYS_PRI_ctrl.bwReg[bwRegId];

    pReg->bandwidth = bandwidth & 0x1FFFF;
    pReg->watermark = watermark & 0xFFF;
    pReg->pressure  = (pressH & 3) | ((pressL & 3) << 2);
    pReg->clearHistory = 0x1; // apply settings
  }

  SYS_PRI_unmapMem();

  return 0;
}

int SYS_PRI_printBwReg(int bwRegId)
{
  int bandwidth, watermark, pressH, pressL;

  float cycles, bandwidthMBs;

  static char *name[] = {
    "HDVICP0 ", "HDVICP1 ", "HDVICP2 ", "EDMA_RD0",
    "EDMA_WR0", "EDMA_RD1", "EDMA_WR1", "DSP     ",
    "SGX     ",
  };

  if(!SYS_PRI_isValidBwReg(bwRegId))
    return 0;

  bandwidth = gSYS_PRI_ctrl.bwReg[bwRegId]->bandwidth;
  watermark = gSYS_PRI_ctrl.bwReg[bwRegId]->watermark & 0xFFF;
  pressH    = (gSYS_PRI_ctrl.bwReg[bwRegId]->pressure >> 0) & 3;
  pressL    = (gSYS_PRI_ctrl.bwReg[bwRegId]->pressure >> 2) & 3;

  bandwidthMBs = bandwidth*15.625;

  if(bandwidth==0)
    bandwidth=1;

  cycles = (watermark*1000)/(2*bandwidth*15.625);

  printf(" %s : %-8d %-8d %-17.3f %d (%d)\n",
    name[bwRegId], pressH, pressL, bandwidthMBs, watermark, (int)cycles
    );

  return 0;
}

int SYS_PRI_doPrintAll()
{
  REG32 value=0;
  int i, status;

  status = SYS_PRI_mapMem();
  if(status != 0)
    SYS_PRI_unmapMem();

  value = *(REG32*)gSYS_PRI_ctrl.initPressure[0];

  gSYS_PRI_ctrl.initPressureA8      = (value >> 0 ) & 3;
  gSYS_PRI_ctrl.initPressureDSP_CFG = (value >> 4 ) & 3;
  gSYS_PRI_ctrl.initPressureHDVPSS0 = (value >> 8 ) & 1;
  gSYS_PRI_ctrl.initPressureHDVPSS1 = (value >> 10) & 1;
  gSYS_PRI_ctrl.initPressureEDMARD2 = (value >> 24) & 3;
  gSYS_PRI_ctrl.initPressureEDMAWR2 = (value >> 26) & 3;
  gSYS_PRI_ctrl.initPressureEDMARD3 = (value >> 28) & 3;
  gSYS_PRI_ctrl.initPressureEDMAWR3 = (value >> 30) & 3;

  value = *(REG32*)gSYS_PRI_ctrl.initPressure[1];

  gSYS_PRI_ctrl.initPressureGMAC0   = (value >> 0 ) & 3;
  gSYS_PRI_ctrl.initPressureGMAC1   = (value >> 2 ) & 3;
  gSYS_PRI_ctrl.initPressureUSB_DMA = (value >> 4 ) & 3;
  gSYS_PRI_ctrl.initPressureUSB_QMGR= (value >> 6 ) & 3;
  gSYS_PRI_ctrl.initPressureSATA    = (value >> 8 ) & 3;
  gSYS_PRI_ctrl.initPressureDUCATI  = (value >> 14) & 3;
  gSYS_PRI_ctrl.initPressurePCIE    = (value >> 16) & 3;

  printf(" \n");
  printf(" L3 Static Pressure \n");
  printf(" ================== \n");
  printf(" A8       DSP_CFG  HDVPSS0  HDVPSS1  EDMA_RD2 EDMA_WR2 EDMA_RD3 EDMA_WR3 \n");
  printf(" %-8d %-8d %-8d %-8d %-8d %-8d %-8d %-8d\n",
    gSYS_PRI_ctrl.initPressureA8,
    gSYS_PRI_ctrl.initPressureDSP_CFG,
    gSYS_PRI_ctrl.initPressureHDVPSS0,
    gSYS_PRI_ctrl.initPressureHDVPSS1,
    gSYS_PRI_ctrl.initPressureEDMARD2,
    gSYS_PRI_ctrl.initPressureEDMAWR2,
    gSYS_PRI_ctrl.initPressureEDMARD3,
    gSYS_PRI_ctrl.initPressureEDMAWR3
      );
  printf(" \n");
  printf(" GMAC0    GMAC1    USB_DMA  USB_QMGR SATA     DUCATI   PCIE\n");
  printf(" %-8d %-8d %-8d %-8d %-8d %-8d %-8d\n",
    gSYS_PRI_ctrl.initPressureGMAC0 ,
    gSYS_PRI_ctrl.initPressureGMAC1 ,
    gSYS_PRI_ctrl.initPressureUSB_DMA,
    gSYS_PRI_ctrl.initPressureUSB_QMGR,
    gSYS_PRI_ctrl.initPressureSATA,
    gSYS_PRI_ctrl.initPressureDUCATI,
    gSYS_PRI_ctrl.initPressurePCIE
      );

  printf(" \n");

  value = *(REG32*)gSYS_PRI_ctrl.dmmPegPri[0];

  gSYS_PRI_ctrl.dmmPegPriA8      = (value >> 0 ) & 7;

  value = *(REG32*)gSYS_PRI_ctrl.dmmPegPri[1];

  gSYS_PRI_ctrl.dmmPegPriSYS_MMU = (value >> 8 ) & 7;
  gSYS_PRI_ctrl.dmmPegPriDUCATI  = (value >> 24) & 7;

  value = *(REG32*)gSYS_PRI_ctrl.dmmPegPri[3];

  gSYS_PRI_ctrl.dmmPegPriEDMARD0 = (value >> 0 ) & 7;
  gSYS_PRI_ctrl.dmmPegPriEDMARD1 = (value >> 4 ) & 7;
  gSYS_PRI_ctrl.dmmPegPriEDMARD2 = (value >> 8 ) & 7;
  gSYS_PRI_ctrl.dmmPegPriEDMARD3 = (value >> 12) & 7;
  gSYS_PRI_ctrl.dmmPegPriEDMAWR0 = (value >> 16) & 7;
  gSYS_PRI_ctrl.dmmPegPriEDMAWR1 = (value >> 20) & 7;
  gSYS_PRI_ctrl.dmmPegPriEDMAWR2 = (value >> 24) & 7;
  gSYS_PRI_ctrl.dmmPegPriEDMAWR3 = (value >> 28) & 7;

  value = *(REG32*)gSYS_PRI_ctrl.dmmPegPri[4];

  gSYS_PRI_ctrl.dmmPegPriSGX     = (value >> 0 ) & 7;

  value = *(REG32*)gSYS_PRI_ctrl.dmmPegPri[5];

  gSYS_PRI_ctrl.dmmPegPriHDVICP0 = (value >> 0 ) & 7;
  gSYS_PRI_ctrl.dmmPegPriHDVICP1 = (value >> 16) & 7;
  gSYS_PRI_ctrl.dmmPegPriHDVICP2 = (value >> 20) & 7;

  value = *(REG32*)gSYS_PRI_ctrl.dmmPegPri[6];

  gSYS_PRI_ctrl.dmmPegPriGMAC0   = (value >> 0 ) & 7;
  gSYS_PRI_ctrl.dmmPegPriGMAC1   = (value >> 4 ) & 7;
  gSYS_PRI_ctrl.dmmPegPriUSB_DMA = (value >> 16) & 7;
  gSYS_PRI_ctrl.dmmPegPriUSB_QMGR= (value >> 20) & 7;

  value = *(REG32*)gSYS_PRI_ctrl.dmmPegPri[7];

  gSYS_PRI_ctrl.dmmPegPriSATA    = (value >> 0) & 7;
  gSYS_PRI_ctrl.dmmPegPriPCIE    = (value >> 4) & 7;

  printf(" \n");
  printf(" DMM Priority \n");
  printf(" ============ \n");
  printf(" A8       SYS_MMU  DUCATI   EDMA_RD0 EDMA_RD1 EDMA_RD2 EDMA_RD3 EDMA_WR0 \n");
  printf(" %-8d %-8d %-8d %-8d %-8d %-8d %-8d %-8d\n",
    gSYS_PRI_ctrl.dmmPegPriA8      ,
    gSYS_PRI_ctrl.dmmPegPriSYS_MMU ,
    gSYS_PRI_ctrl.dmmPegPriDUCATI  ,
    gSYS_PRI_ctrl.dmmPegPriEDMARD0 ,
    gSYS_PRI_ctrl.dmmPegPriEDMARD1 ,
    gSYS_PRI_ctrl.dmmPegPriEDMARD2 ,
    gSYS_PRI_ctrl.dmmPegPriEDMARD3 ,
    gSYS_PRI_ctrl.dmmPegPriEDMAWR0
      );

  printf(" \n");
  printf(" EDMA_WR1 EDMA_WR2 EDMA_WR3 SGX      HDVICP0  HDVICP1  HDVICP2  GMAC0    \n");
  printf(" %-8d %-8d %-8d %-8d %-8d %-8d %-8d %-8d\n",
    gSYS_PRI_ctrl.dmmPegPriEDMAWR1 ,
    gSYS_PRI_ctrl.dmmPegPriEDMAWR2 ,
    gSYS_PRI_ctrl.dmmPegPriEDMAWR3 ,
    gSYS_PRI_ctrl.dmmPegPriSGX     ,
    gSYS_PRI_ctrl.dmmPegPriHDVICP0 ,
    gSYS_PRI_ctrl.dmmPegPriHDVICP1 ,
    gSYS_PRI_ctrl.dmmPegPriHDVICP2 ,
    gSYS_PRI_ctrl.dmmPegPriGMAC0
      );

  printf(" \n");
  printf(" GMAC1    USB_DMA  USB_QMGR SATA     PCIE\n");
  printf(" %-8d %-8d %-8d %-8d %-8d\n",
    gSYS_PRI_ctrl.dmmPegPriGMAC1   ,
    gSYS_PRI_ctrl.dmmPegPriUSB_DMA ,
    gSYS_PRI_ctrl.dmmPegPriUSB_QMGR,
    gSYS_PRI_ctrl.dmmPegPriSATA    ,
    gSYS_PRI_ctrl.dmmPegPriPCIE
      );
  printf(" \n");


  printf(" \n");
  printf(" L3 Bandwith Regulator \n");
  printf(" ===================== \n");
  printf(" Module   : PressH   PressL   Bandwidth in MB/s Watermark (Cycles) \n");
  for(i=0; i<BW_REG_MAX; i++)
  {
    SYS_PRI_printBwReg(i);
  }
  printf(" \n");

  SYS_PRI_unmapMem();

  return 0;
}

int SYS_PRI_parseCmdLineArgs(int argc, char **argv)
{
  int i;
  char name[20];
  REG32 value[4];

  memset(&gSYS_PRI_ctrl, 0, sizeof(gSYS_PRI_ctrl));

  if(argc==1)
  {
      SYS_PRI_printCmdLineArgs(argv[0]);
      exit(0);
  }

  // parse and override with command line args
  for(i=1; i<argc; i++)
  {
    if(strcmp(argv[i], "--printAll")==0)
    {
      SYS_PRI_doPrintAll();
    } else
    if(strcmp(argv[i], "--regDump")==0)
    {
      gSYS_PRI_ctrl.doRegDump=TRUE;
      SYS_PRI_mapMem();
      SYS_PRI_unmapMem();
      gSYS_PRI_ctrl.doRegDump=FALSE;
    } else
    if(strcmp(argv[i], "--help")==0)
    {
      SYS_PRI_printCmdLineArgs(argv[0]);
      exit(0);
    } else
    if(strcmp(argv[i], "--dmm-pri-set")==0)
    {
      i++;
      if(i<argc)
      {
        strcpy(name, argv[i]);
        i++;
        if(i<argc)
        {
          value[0] = atoi(argv[i]);
          SYS_PRI_dmmPriSet(name, value[0]);
        }
      }
    } else
    #ifdef L3_STATIC_PRESSURE_SET_ENABLE
    if(strcmp(argv[i], "--L3-pri-set")==0)
    {
      i++;
      if(i<argc)
      {
        strcpy(name, argv[i]);
        i++;
        if(i<argc)
        {
          value[0] = atoi(argv[i]);
          SYS_PRI_l3StaticPressureSet(name, value[0]);
        }
      }
    } else
    #endif
    if(strcmp(argv[i], "--L3-bw-reg-set")==0)
    {
      i++;
      if(i<argc)
      {
        strcpy(name, argv[i]);
        i++;
        if(i<argc)
        {
          value[0] = atoi(argv[i]);
          i++;
          if(i<argc)
          {
            value[1] = atoi(argv[i]);
            i++;
            if(i<argc)
            {
              value[2] = atoi(argv[i]);
              i++;
              if(i<argc)
              {
                value[3] = atoi(argv[i]);
                SYS_PRI_l3BwRegSet(name, value[0], value[1], value[2], value[3]);
              }
            }
          }
        }
      }
    }
  }
  return 0;
}

int main(int argc, char **argv)
{
  SYS_PRI_parseCmdLineArgs(argc, argv);
  return 0;
}

static char xtod(char c) {
  if (c>='0' && c<='9') return c-'0';
  if (c>='A' && c<='F') return c-'A'+10;
  if (c>='a' && c<='f') return c-'a'+10;
  return c=0;        // not Hex digit
}

static int HextoDec(char *hex, int l)
{
  if (*hex==0)
    return(l);

  return HextoDec(hex+1, l*16+xtod(*hex)); // hex+1?
}

int xstrtoi(char *hex)      // hex string to integer
{
  return HextoDec(hex,0);
}
