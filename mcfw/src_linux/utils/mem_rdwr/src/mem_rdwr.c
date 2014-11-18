
#include <mem_rdwr.h>

MEM_RDWR_Ctrl gMEM_RDWR_ctrl;

int MEM_RDWR_printCmdLineArgs(char *str)
{
  printf("\n");
  printf(" Memory read/write utility, (c) Texas Instruments 2010\n");
  printf(" Built on %s %s\n", __DATE__, __TIME__);  
  printf("\n");  
  printf(" MEM WRITE, %s --wr <memory physical address in Hex> <memory value to Write in Hex>\n", str);
  printf("\n");
  printf(" MEM READ , %s --rd <memory physical address in Hex> <memory size in 32-bit words in Dec> --cols <number of coloums of print in Dec>\n", str);
  printf("\n");
  printf(" LOAD FILE, %s --fload <memory physical address in Hex> <filename>\n", str);
  printf("\n");
  printf(" SAVE FILE, %s --fsave <memory physical address in Hex> <memory size in Hex in bytes> <filename>\n", str);
  printf("\n");
  printf(" Physical memory address MUST be 4byte aligned\n");
  printf("\n");  
  printf(" Example,\n");
  printf(" MEM WRITE, %s --wr 4810D000 1234abcd \n", str);
  printf(" MEM READ , %s --rd 4810D000 16 \n", str);
  printf(" MEM READ , %s --rd 4810D000 16 --cols 1 \n", str);
  printf(" FILE SAVE, %s --fsave 81000000 0x100 test.bin \n", str); 
  printf(" FILE LOAD, %s --fload 81000000 test.bin \n", str);   
  printf("\n");
  
  return 0;
}

int MEM_RDWR_parseCmdLineArgs(int argc, char **argv)
{
  int i, status;
  
  memset(&gMEM_RDWR_ctrl, 0, sizeof(gMEM_RDWR_ctrl));
  
  // set defaults
  gMEM_RDWR_ctrl.memAddr = 0;
  gMEM_RDWR_ctrl.memSize = 0;      
  gMEM_RDWR_ctrl.wrMemValue = 0;
  gMEM_RDWR_ctrl.numCols = 1;
  gMEM_RDWR_ctrl.cmdType = CMD_MEM_RD;   
   
  // parse and override with command line args  
  for(i=1; i<argc; i++)
  {
    if(strcmp(argv[i], "--wr")==0)
    {
      gMEM_RDWR_ctrl.cmdType = CMD_MEM_WR;
      i++;
      gMEM_RDWR_ctrl.memAddr = 0;
      gMEM_RDWR_ctrl.memSize = 1;      
      gMEM_RDWR_ctrl.wrMemValue = 0;
      if(i<argc)
      {
        gMEM_RDWR_ctrl.memAddr = xstrtoi(argv[i]);
        i++;        
      }
      if(i<argc)
      {
        gMEM_RDWR_ctrl.wrMemValue = xstrtoi(argv[i]);
      }
      break;
    } else
    if(strcmp(argv[i], "--rd")==0)
    {
      gMEM_RDWR_ctrl.cmdType = CMD_MEM_RD;
      i++;
      gMEM_RDWR_ctrl.memAddr = 0;
      gMEM_RDWR_ctrl.memSize = 0;      
      if(i<argc)
      { 
        gMEM_RDWR_ctrl.memAddr = xstrtoi(argv[i]);
        i++;        
      }
      if(i<argc)
      {
        gMEM_RDWR_ctrl.memSize = atoi(argv[i]);
      }
    } else
    if(strcmp(argv[i], "--cols")==0)    
    {
      i++;
      if(i<argc)
      {
        gMEM_RDWR_ctrl.numCols = atoi(argv[i]);
      }
    } else
    if(strcmp(argv[i], "--help")==0)    
    {
      MEM_RDWR_printCmdLineArgs(argv[0]);
      exit(0);      
    } else
    if(strcmp(argv[i], "--fsave")==0)    
    {
      gMEM_RDWR_ctrl.cmdType = CMD_FILE_SAVE;
      i++;
      gMEM_RDWR_ctrl.memAddr = 0;
      gMEM_RDWR_ctrl.memSize = 0;
      strcpy(gMEM_RDWR_ctrl.filename, "DATA.BIN");      
      if(i<argc)
      { 
        gMEM_RDWR_ctrl.memAddr = xstrtoi(argv[i]);
        i++;        
        if(i<argc)
        {
          gMEM_RDWR_ctrl.memSize = xstrtoi(argv[i]);
          i++;        
          if(i<argc)
          {          
            strcpy(gMEM_RDWR_ctrl.filename, argv[i]);
          }          
        }  
      }
    } else
    if(strcmp(argv[i], "--fload")==0)    
    {
      struct stat fileStat;
      
      gMEM_RDWR_ctrl.cmdType = CMD_FILE_LOAD;
      i++;
      gMEM_RDWR_ctrl.memAddr = 0;
      gMEM_RDWR_ctrl.memSize = 0x100;
      strcpy(gMEM_RDWR_ctrl.filename, "DATA.BIN");      
      if(i<argc)
      { 
        gMEM_RDWR_ctrl.memAddr = xstrtoi(argv[i]);
        i++;        
        if(i<argc)
        {
          strcpy(gMEM_RDWR_ctrl.filename, argv[i]);
          
          status = stat(gMEM_RDWR_ctrl.filename, &fileStat);
          
          if(status>=0)
          {
            gMEM_RDWR_ctrl.memSize = fileStat.st_size;
          }
        }  
      }
    }
  }

  if(gMEM_RDWR_ctrl.memAddr==0)
  {
      MEM_RDWR_printCmdLineArgs(argv[0]);
      exit(0);      
  }  
  
  // align to 4byte boundary
  gMEM_RDWR_ctrl.memAddr =   gMEM_RDWR_ctrl.memAddr & ~RDWR_ADDR_ALIGN;
  
  if(gMEM_RDWR_ctrl.memSize==0)
    gMEM_RDWR_ctrl.memSize = 1;
  if(gMEM_RDWR_ctrl.numCols==0)
    gMEM_RDWR_ctrl.numCols = 1;

  return 0;    
}

int MEM_RDWR_mapMem()
{
  gMEM_RDWR_ctrl.memDevFd = open("/dev/mem",O_RDWR|O_SYNC);
  if(gMEM_RDWR_ctrl.memDevFd < 0)
  {
    printf(" ERROR: /dev/mem open failed !!!\n");
    return -1;
  }

  gMEM_RDWR_ctrl.memOffset   = gMEM_RDWR_ctrl.memAddr & MMAP_MEM_PAGE_ALIGN;
   
  gMEM_RDWR_ctrl.mmapMemAddr = gMEM_RDWR_ctrl.memAddr - gMEM_RDWR_ctrl.memOffset;

  gMEM_RDWR_ctrl.mmapMemSize = gMEM_RDWR_ctrl.memSize + gMEM_RDWR_ctrl.memOffset;
 
	gMEM_RDWR_ctrl.pMemVirtAddr = mmap(	
	        (void	*)gMEM_RDWR_ctrl.mmapMemAddr,
	        gMEM_RDWR_ctrl.mmapMemSize,
					PROT_READ|PROT_WRITE|PROT_EXEC,MAP_SHARED,
					gMEM_RDWR_ctrl.memDevFd,
					gMEM_RDWR_ctrl.mmapMemAddr
					);

	if (gMEM_RDWR_ctrl.pMemVirtAddr==NULL)
	{
		printf(" ERROR: mmap() failed !!!\n");
		return -1;
	}
    
  return 0;
}

int MEM_RDWR_unmapMem()
{
  if(gMEM_RDWR_ctrl.pMemVirtAddr)
    munmap((void*)gMEM_RDWR_ctrl.pMemVirtAddr, gMEM_RDWR_ctrl.mmapMemSize);
    
  if(gMEM_RDWR_ctrl.memDevFd >= 0)
    close(gMEM_RDWR_ctrl.memDevFd);
    
  return 0;
}

int MEM_RDWR_readMem()
{
  unsigned int memPhysAddr, i;
  volatile unsigned int *memVirtAddr;
  unsigned int memSize;
  unsigned int numPrintCols;
  
  numPrintCols = gMEM_RDWR_ctrl.numCols;
  memSize      = gMEM_RDWR_ctrl.memSize;
  memPhysAddr  = gMEM_RDWR_ctrl.memAddr;
  memVirtAddr  = (unsigned int*)((unsigned int)gMEM_RDWR_ctrl.pMemVirtAddr + gMEM_RDWR_ctrl.memOffset);
  

  if(numPrintCols<1)
    numPrintCols = 1;
    
  if(numPrintCols>16)    
    numPrintCols = 16;
  
  if(memSize<1) {
    memSize = 1;
  }
  if(memSize > MAX_READ_SIZE)
  {
    memSize = MAX_READ_SIZE;
  }
  
  printf(" \n");  

  while(memSize) {
    printf(" 0x%08x:", memPhysAddr);  
    for(i=0; i<numPrintCols; i++) {
    
      printf(" %08X", *memVirtAddr);  
      
      memVirtAddr++;
      memPhysAddr+=sizeof(unsigned int);
      memSize--;
      
      if(memSize==0)
        break;
    }
    printf("\n");    
  }
  
  printf(" \n");    

  return 0;
}

int MEM_RDWR_writeMem()
{
  unsigned int memPhysAddr;
  volatile unsigned int *memVirtAddr;

  memPhysAddr  = gMEM_RDWR_ctrl.memAddr;
  memVirtAddr  = (unsigned int*)((unsigned int)gMEM_RDWR_ctrl.pMemVirtAddr + gMEM_RDWR_ctrl.memOffset);
  
  printf(" \n");
  printf(" ORG 0x%08x: %x\n", memPhysAddr, *memVirtAddr);  
    
  *memVirtAddr = gMEM_RDWR_ctrl.wrMemValue;

  printf(" NEW 0x%08x: %x\n", memPhysAddr, *memVirtAddr);  
  printf(" \n");  

  return 0;
}

int MEM_RDWR_saveFile()
{
  volatile unsigned int *memVirtAddr;
  
  memVirtAddr  = (unsigned int*)((unsigned int)gMEM_RDWR_ctrl.pMemVirtAddr + gMEM_RDWR_ctrl.memOffset);

  OSA_fileWriteFile(gMEM_RDWR_ctrl.filename, (Uint8*)memVirtAddr, gMEM_RDWR_ctrl.memSize);

  return 0;
}

int MEM_RDWR_loadFile()
{
  volatile unsigned int *memVirtAddr;
  Uint32 actualReadSize;
  
  memVirtAddr  = (unsigned int*)((unsigned int)gMEM_RDWR_ctrl.pMemVirtAddr + gMEM_RDWR_ctrl.memOffset);

  OSA_fileReadFile(gMEM_RDWR_ctrl.filename, (Uint8*)memVirtAddr, gMEM_RDWR_ctrl.memSize, &actualReadSize);

  return 0;
}

int main(int argc, char **argv)
{
  int status;
  
  MEM_RDWR_parseCmdLineArgs(argc, argv);
 
  status = MEM_RDWR_mapMem();

  if(status==0)  
  {
    if(gMEM_RDWR_ctrl.cmdType==CMD_MEM_RD)
      MEM_RDWR_readMem();
    else
    if(gMEM_RDWR_ctrl.cmdType==CMD_MEM_WR)    
      MEM_RDWR_writeMem();  
    else  
    if(gMEM_RDWR_ctrl.cmdType==CMD_FILE_SAVE)    
      MEM_RDWR_saveFile();  
    else  
    if(gMEM_RDWR_ctrl.cmdType==CMD_FILE_LOAD)    
      MEM_RDWR_loadFile();  
      
  }
  
  MEM_RDWR_unmapMem();
      
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
