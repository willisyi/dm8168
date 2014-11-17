/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2011 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/*----------------------------------------------------------------------------
  Defines referenced header files
  -----------------------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <linux/ti81xxhdmi.h>

#include <osa_thr.h>
#include <osa.h>
#include "demos/mcfw_api_demos/mcfw_demo/demo.h"
#include "display_process.h"

#define HDMI_NAME_0			    "/dev/TI81XX_HDMI"
#define HDMI_EDID_MAX_LENGTH    512
#define SLEEP_TIME_MS           1000

#define HDMI_EDID_EX_DATABLOCK_LEN_MASK		0x1F

struct DIS_PARAM {
    UInt32  id;
    UInt32  pixelClock; // KHz
    UInt16  width;
    Uint16  hFrontPorch;
    Uint16  hBackPorch;
    Uint16  hSyncWidth;
    Uint16  height;
    Uint16  vFrontPorch;
    Uint16  vBackPorch;
    Uint16  vSyncWidth;
};
#define PANEL_MAX 5
struct DIS_PARAM pannel[PANEL_MAX] = {
{VSYS_STD_1080P_60,148500,  1920,  88, 148,  44,    1080, 4,36,5},
{VSYS_STD_1080P_50,148500,  1920,  528, 148,  44,    1080, 4,36,5},
{VSYS_STD_720P_60,  74250,  1280, 110, 220,  40,     720, 5,20,5},
{VSYS_STD_XGA_60,   65000,  1024,  24, 160, 136,     768, 3,29,6},
{VSYS_STD_SXGA_60, 108000,  1280,  48, 248, 112,    1024, 1,38,3},
};


static OSA_ThrHndl task;
static int threadExit = FALSE;
static Uint8 edid[HDMI_EDID_MAX_LENGTH] = {0};

static int memfd;
static volatile unsigned int *pMemVirtAddr;
static volatile unsigned int *pHDMI_CTRL;
static unsigned int mmapMemSize;


#define HDMI_CORE_AV_HDMI_CTRL      0x46C009BC
#define MMAP_MEM_PAGE_ALIGN         (32*1024-1)
#define HDMI_CTRL                   1

//#define DEBUG_EN    1

//auto select preferred Detailed Timing.
int autoSelect = 0;

Uint32 hdmi_get_datablock_offset(Uint8 *edid, Uint8 datablock,
        int *offset)
{
    int current_byte, disp, i = 0, length = 0;

    if (edid[0x7e] == 0x00)
        return 1;

    disp = edid[(0x80) + 2];

    if (disp == 0x4)
        return 1;

    i = 0x80 + 0x4;

    while (i < (0x80 + disp))
    {
        current_byte = edid[i];
#if DEBUG_EN
        printf("i = %x cur_byte = %x\n",
                i, current_byte);
#endif
        if ((current_byte >> 5)	== datablock)
        {
            *offset = i;
#if DEBUG_EN
            printf("datablock %d %d\n", datablock, *offset);
#endif
            return 0;
        }
        else
        {
            length = (current_byte &
                    HDMI_EDID_EX_DATABLOCK_LEN_MASK) + 1;
            i += length;
        }
    }
    return 1;
}


int hdmi_get_vendor_ieee_code(Uint8 *edid, Uint8 *code)
{
    int offset;
    Uint8 vsdb =  3;

    if (!hdmi_get_datablock_offset(edid, vsdb, &offset))
    {
        code[0] = edid[offset + 1];
        code[1] = edid[offset + 2];
        code[2] = edid[offset + 3];
        return 0;
    }

    return -1;
}


int check_display_interface(Bool *isHDMI)
{
    int rev = 0, hdmi_sink = 0;
    int i,r,read_block;
    Uint8 code[3];

    *isHDMI = FALSE;

    if (0 != edid[0x7e])
    {
        if (edid[0x7e] > 3)
            read_block = 3;
        else
            read_block = edid[0x7e];

        for (i = 1; i <= edid[0x7e]; i ++)
        {
            /* Check for the revision */
            if ((2 == edid[128]) && (3 == edid[129]))
                rev = 1;

            r = hdmi_get_vendor_ieee_code(edid, code);

                if (0 == r)
                {
                    if ((0x03 ==code[0]) && (0x0C == code[1]) &&
                            (0x0 == code[2]))
                    {
                        hdmi_sink = 1;
                    }
                }
                else
                {
                    break;
                }
        }
    }

    if (rev && hdmi_sink)
    {
        *isHDMI = TRUE;
    }

    return 0;
}

int mapMem()
{
    unsigned int memAddr;
    unsigned int memSize = 4;
    unsigned int mmapMemAddr;
    unsigned int memOffset;

    memfd = open("/dev/mem",O_RDWR|O_SYNC);
    if(memfd < 0)
    {
        printf(" ERROR: /dev/mem open failed !!!\n");
        return -1;
    }

    memAddr = HDMI_CORE_AV_HDMI_CTRL;

    memOffset   = memAddr & MMAP_MEM_PAGE_ALIGN;

    mmapMemAddr = memAddr - memOffset;

    mmapMemSize = memSize + memOffset;

    pMemVirtAddr = mmap(
            (void	*)mmapMemAddr,
            mmapMemSize,
            PROT_READ|PROT_WRITE|PROT_EXEC,MAP_SHARED,
            memfd,
            mmapMemAddr
            );

    if (pMemVirtAddr==NULL)
    {
        perror(" ERROR: mmap() failed !!!\n");
        close(memfd);
        return -1;
    }

    pHDMI_CTRL = pMemVirtAddr + memOffset/4;
    return 0;
}

int unmapMem()
{
    if(pMemVirtAddr)
        munmap((void*)pMemVirtAddr, mmapMemSize);

    if(memfd >= 0)
        close(memfd);

    return 0;
}


void *display_process_main()
{
    int fd = 0;
    struct ti81xxhdmi_status status;
    Bool readEdid = TRUE;
    Bool isHDMI;
    int i;

    fd = open(HDMI_NAME_0, O_RDWR);
    if (fd <= 0)
    {
        perror("Could not open device /dev/TI81XX_HDMI\n");
        exit(1);
    }

    if (mapMem())
    {
        perror("can't do memMap!!!\n");
        exit(1);
    }

    while (threadExit != TRUE)
    {
        if (ioctl(fd, TI81XXHDMI_GET_STATUS, &status) < 0)
        {
            perror("TI81XXHDMI_GET_STATUS\n");
            close(fd);
        }

        if (status.is_hpd_detected)
        {

            if (readEdid)
            {
                if (ioctl(fd, TI81XXHDMI_READ_EDID, &edid) < 0)
                {
//                    perror("TI81XXHDMI_READ_EDID\n");
                    continue;
                }

                check_display_interface(&isHDMI);

                if (isHDMI)
                {
                    *pHDMI_CTRL |= HDMI_CTRL;
                }
                else
                {
                    *pHDMI_CTRL &= 0xFFFFFFFE;
                }

#if DEBUG_EN
                printf("isHDMI %d\n",isHDMI);
                for (i=0;i<HDMI_EDID_MAX_LENGTH;i++)
                {
                    if (i%16 == 0)
                        printf("%02d: ",i/16);
                    printf("%02X ",edid[i]);
                    if (i%16 == 15)
                        printf("\n");
                }
                printf("\n");
                printf("-------------------------------------\n");
#endif

                if (autoSelect)
                {
                    struct HDMI_EDID *data = (struct HDMI_EDID *)edid;

                    if(data->power_features & 0x02)
                    {
                        UInt32 pixelClock;
                        UInt16 width,height,hFrontPorch,hBackPorch,vFrontPorch,vBackPorch,hSyncWidth,
                                vSyncWidth;
                        pixelClock = data->video[0].pixel_clock * 10;// KHz

                        width = data->video[0].horiz_active + ((data->video[0].horiz_high & 0xF0) << 4);
                        hFrontPorch = data->video[0].horiz_sync_offset + ((data->video[0].sync_pulse_high & 0xC0) << 2)
                                        - data->video[0].horiz_border;
                        hSyncWidth = data->video[0].horiz_sync_pulse + ((data->video[0].sync_pulse_high & 0x30) << 4);
                        hBackPorch = data->video[0].horiz_blanking + ((data->video[0].horiz_high & 0x0F) << 8) - hFrontPorch
                                        - 2*data->video[0].horiz_border - hSyncWidth;

                        height = data->video[0].vert_active + ((data->video[0].vert_high & 0xF0) << 4);
                        vFrontPorch = ((data->video[0].vert_sync_pulse & 0xF0)>>4) + ((data->video[0].sync_pulse_high & 0x0C) << 2)
                                        - data->video[0].vert_border;
                        vSyncWidth = (data->video[0].vert_sync_pulse & 0x0F) + ((data->video[0].sync_pulse_high & 0x03) << 4);
                        vBackPorch = data->video[0].vert_blanking + ((data->video[0].vert_high & 0x0F) << 8) - vFrontPorch
                                        - 2*data->video[0].vert_border - vSyncWidth;

                        #if DEBUG_EN
                        printf("pixelclock %d\n",pixelClock);
                        printf("horz width %d front porch %d, sync width %d, back porch %d\n",
                                width,hFrontPorch,hSyncWidth,hBackPorch);
                        printf("vert width %d front porch %d, sync width %d, back porch %d\n",
                                height,vFrontPorch,vSyncWidth,vBackPorch);
                        #endif
                        if(data->video[0].horiz_border || data->video[0].vert_border)
                            printf("HDMI: boarder is h %d v %d, will cause black bar at the side of screen!\n",
                                    data->video[0].horiz_border,data->video[0].vert_border);

                        for (i = 0;i < PANEL_MAX;i++)
                        {
                            //check pixel clock first, then width and height
                            if((pixelClock == pannel[i].pixelClock) &&
                                (width == pannel[i].width) &&
                                (height == pannel[i].height))
                            {

                                Demo_displaySetResolution(VDIS_DEV_HDMI,pannel[i].id);

                                printf("preferred timing parameter:\n");
                                printf("<--pixelclock %d\n",pixelClock);
                                printf("<--horz width %d front porch %d, sync width %d, back porch %d\n",
                                        width,hFrontPorch,hSyncWidth,hBackPorch);
                                printf("<--vert width %d front porch %d, sync width %d, back porch %d\n",
                                        height,vFrontPorch,vSyncWidth,vBackPorch);

                                printf("timing parameter we set: %d id %d\n",i,pannel[i].id);
                                printf("-->pixelclock %d\n",pannel[i].pixelClock);
                                printf("-->horz width %d front porch %d, sync width %d, back porch %d\n",
                                        pannel[i].width,pannel[i].hFrontPorch,pannel[i].hSyncWidth,pannel[i].hBackPorch);
                                printf("-->vert width %d front porch %d, sync width %d, back porch %d\n",
                                        pannel[i].height,pannel[i].vFrontPorch,pannel[i].vSyncWidth,pannel[i].vBackPorch);
                                break;
                            }

                        }

                        if(i >= PANEL_MAX)
                        {
                            printf("preferred timing not found\n");
                            printf("preferred timing parameter:\n");
                            printf("pixelclock %d\n",pixelClock);
                            printf("horz width %d front porch %d, sync width %d, back porch %d\n",
                                    width,hFrontPorch,hSyncWidth,hBackPorch);
                            printf("vert width %d front porch %d, sync width %d, back porch %d\n",
                                    height,vFrontPorch,vSyncWidth,vBackPorch);
                        }
                    }
                    else
                    {
                        printf("EDID: no preferred timing mode\n");
                    }
                }



                readEdid = FALSE;
            }
        }
        else
        {
#if DEBUG_EN
            printf("cable unpluged\n");
#endif
            readEdid = TRUE;
        }

        OSA_waitMsecs(SLEEP_TIME_MS);
    }
    unmapMem();
    close(fd);

    return NULL;
}

void display_process_init()
{
    memset(&task, 0, sizeof(task));

    #ifndef DISABLE_DISPLAY_PROCESS_THREAD
    OSA_thrCreate(&task,display_process_main,OSA_THR_PRI_DEFAULT,OSA_THR_STACK_SIZE_DEFAULT,NULL);
    #endif
}

void display_process_deinit()
{
    #ifndef DISABLE_DISPLAY_PROCESS_THREAD
    threadExit = TRUE;
    OSA_thrDelete(&task);
    #endif
}




