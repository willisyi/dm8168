
#include <mem_stats.h>

MEM_STATS_Ctrl gMEM_STATS_ctrl;

int MEM_STATS_mapMem()
{
    gMEM_STATS_ctrl.memDevFd = open("/dev/mem",O_RDWR|O_SYNC);
    if(gMEM_STATS_ctrl.memDevFd < 0)
    {
        printf(" ERROR: /dev/mem open failed !!!\n");
        return -1;
    }

    gMEM_STATS_ctrl.mmapMemAddr = PERF_CNT_PHYS_BASE_ADDR;
    gMEM_STATS_ctrl.mmapMemSize = PERF_CNT_MAP_SIZE;

	gMEM_STATS_ctrl.pMemVirtAddr = mmap(
	        (void	*)gMEM_STATS_ctrl.mmapMemAddr,
	        gMEM_STATS_ctrl.mmapMemSize,
					PROT_READ|PROT_WRITE|PROT_EXEC,MAP_SHARED,
					gMEM_STATS_ctrl.memDevFd,
					gMEM_STATS_ctrl.mmapMemAddr
					);

	if (gMEM_STATS_ctrl.pMemVirtAddr==NULL)
	{
		printf(" ERROR: mmap() failed !!!\n");
		return -1;
	}

    gMEM_STATS_ctrl.PERF_CNT_1 = gMEM_STATS_ctrl.pMemVirtAddr + PERF_CNT_1_OFFSET/sizeof(unsigned int);
    gMEM_STATS_ctrl.PERF_CNT_2 = gMEM_STATS_ctrl.pMemVirtAddr + PERF_CNT_2_OFFSET/sizeof(unsigned int);
    gMEM_STATS_ctrl.PERF_CNT_CFG = gMEM_STATS_ctrl.pMemVirtAddr + PERF_CNT_CFG_OFFSET/sizeof(unsigned int);
    gMEM_STATS_ctrl.PERF_CNT_SEL = gMEM_STATS_ctrl.pMemVirtAddr + PERF_CNT_SEL_OFFSET/sizeof(unsigned int);

    return 0;
}

int MEM_STATS_unmapMem()
{
  if(gMEM_STATS_ctrl.pMemVirtAddr)
    munmap((void*)gMEM_STATS_ctrl.pMemVirtAddr, gMEM_STATS_ctrl.mmapMemSize);

  if(gMEM_STATS_ctrl.memDevFd >= 0)
    close(gMEM_STATS_ctrl.memDevFd);

  return 0;
}

int MEM_STATS_showStats(
        UInt32 masterId1, UInt32 masterId2,
        char *masterName1, char *masterName2,
        UInt32 samplingPeriodMsec, UInt32 loopCount
        )
{
    Uint32 newCount[2], oldCount[2], curCount[2], curTime, elaspedTime;
    float value[2];
    Uint32 filter1, filter2;

    filter1 = filter2 = 1;

    if(masterId1==MASTERID_ALL)
        filter1 = 0;

    if(masterId2==MASTERID_ALL)
        filter2 = 0;

    *gMEM_STATS_ctrl.PERF_CNT_CFG =
            (((filter2) & 0x1) << 31)
          | (((filter1) & 0x1) << 15)
            ;

    *gMEM_STATS_ctrl.PERF_CNT_SEL =
            (((masterId2*4) & 0xFF) << 24)
          | (((masterId1*4) & 0xFF) << 8)
            ;

    oldCount[0] = *gMEM_STATS_ctrl.PERF_CNT_1;
    oldCount[1] = *gMEM_STATS_ctrl.PERF_CNT_2;

    curTime = OSA_getCurTimeInMsec();

    do
    {
        OSA_waitMsecs(samplingPeriodMsec);

        newCount[0] = *gMEM_STATS_ctrl.PERF_CNT_1;
        newCount[1] = *gMEM_STATS_ctrl.PERF_CNT_2;

        elaspedTime = OSA_getCurTimeInMsec() - curTime;

        curTime = OSA_getCurTimeInMsec();

        if(newCount[0] < oldCount[0])
        {
            // wrap around happended
            curCount[0] = (0xFFFFFFFF - oldCount[0])+newCount[0];
        }
        else
        {
            curCount[0] = newCount[0] - oldCount[0];
        }

        if(newCount[1] < oldCount[1])
        {
            // wrap around happended
            curCount[1] = (0xFFFFFFFF - oldCount[1])+newCount[1];
        }
        else
        {
            curCount[1] = newCount[1] - oldCount[1];
        }

        oldCount[0] = newCount[0];
        oldCount[1] = newCount[1];

        // in counts/secs
        value[0] = (curCount[0]*1000.0/elaspedTime);
        value[1] = (curCount[1]*1000.0/elaspedTime);

        // in million counts/sec
        value[0] /= 1000*1000;
        value[1] /= 1000*1000;


        printf(" # [%10s] %8.3f M req/sec\n", masterName1, value[0]);
        printf(" # [%10s] %8.3f M req/sec\n", masterName2, value[1]);

    } while(--loopCount);

    return 0;
}

int main(int argc, char **argv)
{
    int status;
    int sampleTime = 1000; // in msecs

    status = MEM_STATS_mapMem();

    if(argc>1)
    {
        sampleTime = atoi(argv[1]);

        if(sampleTime > MAX_SAMPLE_TIME_IN_MSECS)
            sampleTime = MAX_SAMPLE_TIME_IN_MSECS;
    }

    if(status==0)
    {

        while(1)
        {
            printf(" # \n");
            printf(" # MEM STATS Started !!! (%d msecs sampling time) \n", sampleTime);
            printf(" # \n");
            MEM_STATS_showStats
                (
                MASTERID_ALL,
                MASTERID_IVA_0,
                MASTER_NAME_ALL,
                MASTER_NAME_IVA_0,
                sampleTime,
                1
            );
            MEM_STATS_showStats
                (
                MASTERID_IVA_1,
                MASTERID_IVA_2,
                MASTER_NAME_IVA_1,
                MASTER_NAME_IVA_2,
                sampleTime,
                1
            );
            MEM_STATS_showStats
                (
                MASTERID_HDVPSS_0,
                MASTERID_HDVPSS_1,
                MASTER_NAME_HDVPSS_0,
                MASTER_NAME_HDVPSS_1,
                sampleTime,
                1
            );
            MEM_STATS_showStats
                (
                MASTERID_A8,
                MASTERID_M3,
                MASTER_NAME_A8,
                MASTER_NAME_M3,
                sampleTime,
                1
            );
            MEM_STATS_showStats
                (
                MASTERID_EDMA_0_RD,
                MASTERID_EDMA_0_WR,
                MASTER_NAME_EDMA_0_RD,
                MASTER_NAME_EDMA_0_WR,
                sampleTime,
                1
            );
            MEM_STATS_showStats
                (
                MASTERID_EDMA_1_RD,
                MASTERID_EDMA_1_WR,
                MASTER_NAME_EDMA_1_RD,
                MASTER_NAME_EDMA_1_WR,
                sampleTime,
                1
            );
            MEM_STATS_showStats
                (
                MASTERID_EDMA_2_RD,
                MASTERID_EDMA_2_WR,
                MASTER_NAME_EDMA_2_RD,
                MASTER_NAME_EDMA_2_WR,
                sampleTime,
                1
            );
            MEM_STATS_showStats
                (
                MASTERID_EDMA_3_RD,
                MASTERID_EDMA_3_WR,
                MASTER_NAME_EDMA_3_RD,
                MASTER_NAME_EDMA_3_WR,
                sampleTime,
                1
            );
            MEM_STATS_showStats
                (
                MASTERID_EMAC_0,
                MASTERID_EMAC_1,
                MASTER_NAME_EMAC_0,
                MASTER_NAME_EMAC_1,
                sampleTime,
                1
            );
            MEM_STATS_showStats
                (
                MASTERID_USB_DMA,
                MASTERID_USB_QMGR,
                MASTER_NAME_USB_DMA,
                MASTER_NAME_USB_QMGR,
                sampleTime,
                1
            );
            MEM_STATS_showStats
                (
                MASTERID_SATA,
                MASTERID_PCIE,
                MASTER_NAME_SATA,
                MASTER_NAME_PCIE,
                sampleTime,
                1
            );
            MEM_STATS_showStats
                (
                MASTERID_DSP_MDMA,
                MASTERID_DSP_CFG,
                MASTER_NAME_DSP_MDMA,
                MASTER_NAME_DSP_CFG,
                sampleTime,
                1
            );
            MEM_STATS_showStats
                (
                MASTERID_SYSTEM_MMU,
                MASTERID_ALL,
                MASTER_NAME_SYSTEM_MMU,
                MASTER_NAME_ALL,
                sampleTime,
                1
            );
            printf(" # \n");
            printf(" # MEM STATS Done !!! (%d msecs sampling time) \n", sampleTime);
            printf(" # \n");


        }
    }

    MEM_STATS_unmapMem();

    return 0;
}

