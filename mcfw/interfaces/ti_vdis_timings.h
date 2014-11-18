/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
    \ingroup MCFW_API
    \defgroup MCFW_VDIS_API McFW Video Display (VDIS) API

    @{
*/

/**
    \file ti_vdis_timings".h
    \brief Venc timing params
*/

#ifndef __TI_VDIS_TIMINGS_H__
#define __TI_VDIS_TIMINGS_H__

static inline int Vdis_sysfsRead(char *filename, char *buf, int length)
{
    FILE *fp;
    int ret;

    fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("failed to open files %s.\n", filename);
        return -1;
    }

    ret = fread(buf, 1, length, fp);
    if (ret < 0)
    {
        printf("failed to read %s\n", filename);
        return -1;
    }
    buf[ret] = '\0';
    fclose(fp);
    return 0;

}

static inline int Vdis_sysfsWrite(char *fileName, char *val)
{
    FILE *fp;
    int ret;

    fp = fopen(fileName, "w");
    if (fp == NULL) {
                printf("Failed to open %s for writing\n", fileName);
                return -1;
    }
    ret = fwrite(val, strlen(val) + 1, 1, fp);
    if ( ret != 1) {
                printf("Failed to write sysfs variable %s to %s\n", fileName, val);
                fclose(fp);
                return -1;
    }
    fflush(fp);
    fclose(fp);
    return 0;
}

#define VDIS_SYSFS_HDMI   0
#define VDIS_SYSFS_DVO2   1
#define VDIS_SYSFS_SD     2
#define VDIS_SYSFS_HDCOMP 3

#define VDIS_SYSFS_GRPX0  0
#define VDIS_SYSFS_GRPX1  1
#define VDIS_SYSFS_GRPX2  2



#define VDIS_SYSFSCMD_SETTIMINGS      "/sys/devices/platform/vpss/display%d/timings"
#define VDIS_SYSFSCMD_SETTIEDVENCS    "/sys/devices/platform/vpss/system/tiedvencs"
#define VDIS_SYSFSCMD_SETVENC         "/sys/devices/platform/vpss/display%d/enabled"
#define VDIS_SYSFSCMD_SET_GRPX        "/sys/devices/platform/vpss/graphics%d/enabled"
#define VDIS_SYSFSCMD_SET_MODE        "/sys/devices/platform/vpss/display%d/mode"
#define VDIS_SYSFSCMD_SET_GRPX_NODE   "/sys/devices/platform/vpss/graphics%d/node"
#define VDIS_SYSFSCMD_SETVENC_CLKSRC  "/sys/devices/platform/vpss/display%d/clksrc"

#define VDIS_SYSFSCMD_GET_GRPX        VDIS_SYSFSCMD_SET_GRPX
#define VDIS_SYSFSCMD_GET_GRPX_NODE   VDIS_SYSFSCMD_SET_GRPX_NODE

#define VDIS_TIMINGS_1080P_60 "148500,1920/88/148/44,1080/4/36/5,1"
#define VDIS_TIMINGS_1080P_50 "148500,1920/528/148/44,1080/4/36/5,1"
#define VDIS_TIMINGS_720P_60  "74250,1280/110/220/40,720/5/20/5,1"
#define VDIS_TIMINGS_XGA_60   "65000,1024/24/160/136,768/3/29/6,1"
#define VDIS_TIMINGS_SXGA_60  "108000,1280/48/248/112,1024/1/38/3,1"
#define VDIS_TIMINGS_480P     "27027,720/16/60/62,480/9/30/6,1"
#define VDIS_TIMINGS_576P     "27000,720/12/68/64,576/5/39/5,1"
#define VDIS_MODE_NTSC        "ntsc"
#define VDIS_MODE_PAL         "pal"

#define VDIS_ON               "1"
#define VDIS_OFF              "0"

#define VDIS_SYSFSCMD_ARG2(buff, x, y) do { sprintf(buff, x); \
                                         /* printf ("*** system command -> %s ***\n", buff);*/\
                                            Vdis_sysfsWrite(buff, y);\
                                       } while(0)\

#define VDIS_SYSFSCMD_ARG3(buff, x, y, z) do { sprintf(buff, x, y); \
                                              /* printf ("*** system command -> %s ***\n", buff);*/\
                                               Vdis_sysfsWrite(buff, z);\
                                          } while(0)\

#define VDIS_SYSFSCMD_ARG4(buff, w, x, y, z) do { sprintf(buff, w, x, y); \
                                              /* printf ("*** system command -> %s ***\n", buff);*/\
                                               Vdis_sysfsWrite(buff, z);\
                                          } while(0)\

#define VDIS_CMD_IS_GRPX_ON(file, buffer, sysfs, x, y, z)  do {sprintf(file, sysfs, x); z = Vdis_sysfsRead(file, buffer, y); } while(0)

#endif  /* __TI_VDIS_TIMINGS_H__ */

/* @} */
