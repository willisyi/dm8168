/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include "system_priv_common.h"
#include "system_priv_ipc.h"
#include <ti/psp/devices/vps_device.h>
#include <ti/psp/platforms/vps_platform.h>
#if defined(TI816X_EVM) || defined(TI8107_EVM) || defined(TI8107_DVR)
#include <ti/psp/devices/vps_thsfilters.h>
#endif

#pragma DATA_ALIGN(gSystem_tskStack, 32)
#pragma DATA_SECTION(gSystem_tskStack, ".bss:taskStackSection")
UInt8 gSystem_tskStack[SYSTEM_TSK_STACK_SIZE];

System_CommonObj gSystem_objCommon;

Void System_main(UArg arg0, UArg arg1)
{
    System_enumAssertCheck();

    System_initCommon();
    System_init();

    Utils_remoteSendChar('s');

    UTILS_assert(gSystem_objCommon.chainsMainFunc != NULL);

    gSystem_objCommon.chainsMainFunc(NULL, NULL);

    System_deInit();
    System_deInitCommon();

    Utils_remoteSendChar('e');

    System_ipcStop();
}

/* Create test task */
Int32 System_start(Task_FuncPtr chainsMainFunc)
{
    Task_Params tskParams;

    System_ipcStart();

    memset(&gSystem_objCommon, 0, sizeof(gSystem_objCommon));

    gSystem_objCommon.chainsMainFunc = chainsMainFunc;

    /*
     * Create test task
     */
    Task_Params_init(&tskParams);

    tskParams.priority = SYSTEM_TSK_PRI;
    tskParams.stack = gSystem_tskStack;
    tskParams.stackSize = sizeof(gSystem_tskStack);

    gSystem_objCommon.tsk = Task_create(System_main, &tskParams, NULL);

    UTILS_assert(gSystem_objCommon.tsk != NULL);

    return FVID2_SOK;
}

Int32 System_initCommon()
{
    Int32 status;

#ifdef SYSTEM_DEBUG
    Vps_printf(" %d: SYSTEM  : System Common Init in progress !!!\n",
               Utils_getCurTimeInMsec());
#endif
    Utils_prfInit();

    System_ipcInit();

    Utils_mbxInit();

    /*
     * Init memory allocator
     */
    Utils_memInit();

    Utils_memClearOnAlloc(TRUE);

    status = Utils_mbxCreate(&gSystem_objCommon.mbx);
    UTILS_assert(status == FVID2_SOK);

    Utils_prfLoadRegister(gSystem_objCommon.tsk, "SYSTEM  ");

    SystemLink_init();

#ifdef SYSTEM_DEBUG
    Vps_printf(" %d: SYSTEM  : System Common Init Done !!!\n",
               Utils_getCurTimeInMsec());
#endif

    return status;

}

Int32 System_deInitCommon()
{
#ifdef SYSTEM_DEBUG
    Vps_printf(" %d: SYSTEM  : System Common De-Init in progress !!!\n",
               Utils_getCurTimeInMsec());
#endif

    SystemLink_deInit();

    System_ipcDeInit();

    Utils_prfLoadUnRegister(gSystem_objCommon.tsk);

    Utils_mbxDelete(&gSystem_objCommon.mbx);

    /*
     * De-init memory allocator
     */
    Utils_memDeInit();

    Utils_mbxDeInit();

    Utils_prfDeInit();

#ifdef SYSTEM_DEBUG
    Vps_printf(" %d: SYSTEM  : System Common De-Init Done !!!\n",
               Utils_getCurTimeInMsec());
#endif

    return FVID2_SOK;
}

void System_memPrintHeapStatus()
{
#ifdef SYSTEM_DEBUG
    Vps_printf
            (" %d: SYSTEM  : FREE SPACE : System Heap      = %d B, Mbx = %d msgs) \r\n",
                Utils_getCurTimeInMsec(),
                Utils_memGetSystemHeapFreeSpace(),
                Utils_mbxGetFreeMsgCount()
            );

    if(System_getSelfProcId()==SYSTEM_PROC_M3VPSS ||
        System_getSelfProcId()==SYSTEM_PROC_M3VIDEO)
    {
        /* print SR heap free space info from VPSS side only */
        Vps_printf
            (" %d: SYSTEM  : FREE SPACE : SR0 Heap         = %d B (%d MB) \r\n",
                Utils_getCurTimeInMsec(),
                Utils_memGetSR0HeapFreeSpace(),
                Utils_memGetSR0HeapFreeSpace()/(1024*1024)
            );

        Vps_printf
            (" %d: SYSTEM  : FREE SPACE : Frame Buffer     = %d B (%d MB) \r\n",
                Utils_getCurTimeInMsec(),
                Utils_memGetBufferHeapFreeSpace(),
                Utils_memGetBufferHeapFreeSpace()/(1024*1024)
            );

        Vps_printf
            (" %d: SYSTEM  : FREE SPACE : Bitstream Buffer = %d B (%d MB) \r\n",
                Utils_getCurTimeInMsec(),
                Utils_memGetBitBufferHeapFreeSpace(),
                Utils_memGetBitBufferHeapFreeSpace()/(1024*1024)
            );

        {
            SystemCommon_TilerGetFreeSize tilerFreeSize;

            SystemTiler_getFreeSize(&tilerFreeSize);

            if(SystemTiler_isAllocatorDisabled())
            {
                Vps_printf
                    (" %d: SYSTEM  : FREE SPACE : Tiler Buffer     = %d B (%d MB)  - TILER OFF \r\n",
                        Utils_getCurTimeInMsec(),
                        tilerFreeSize.freeSizeRaw/(1024*1024)
                    );
            }
            else
            {
                Vps_printf
                    (" %d: SYSTEM  : FREE SPACE : Tiler 8-bit      = %d B (%d MB)  - TILER ON \r\n",
                        Utils_getCurTimeInMsec(),
                        tilerFreeSize.freeSize8b,
                        tilerFreeSize.freeSize8b/(1024*1024)
                    );
                Vps_printf
                    (" %d: SYSTEM  : FREE SPACE : Tiler 16-bit     = %d B (%d MB)  - TILER ON \r\n",
                        Utils_getCurTimeInMsec(),
                        tilerFreeSize.freeSize16b,
                        tilerFreeSize.freeSize16b/(1024*1024)
                    );
            }
        }
    }

#endif

    return;
}

int System_resumeExecution()
{
    gSystem_objCommon.haltExecution = FALSE;

    return 0;
}

int System_haltExecution()
{
    gSystem_objCommon.haltExecution = TRUE;
    Vps_rprintf(" %d: SYSTEM: Executing Halted !!!\n", Utils_getCurTimeInMsec());
    while (gSystem_objCommon.haltExecution)
    {
        Task_sleep(100);
    }

    return 0;
}

/*
 * enum's defined in system_const.h MUST match the enum's defined in FVID2
 * driver's since they are passed directly to the driver without conversion
 *
 * Anytime a new const is added in system_const.h, MAKE sure to put a assert
 * check in this function */
int System_enumAssertCheck()
{
    UTILS_COMPILETIME_ASSERT(SYSTEM_BUFFER_ALIGNMENT == VPS_BUFFER_ALIGNMENT);
    UTILS_COMPILETIME_ASSERT(SYSTEM_MAX_PLANES == FVID2_MAX_PLANES);

    UTILS_COMPILETIME_ASSERT(SYSTEM_CAPTURE_INST_VIP0_PORTA == VPS_CAPT_INST_VIP0_PORTA);
    UTILS_COMPILETIME_ASSERT(SYSTEM_CAPTURE_INST_VIP0_PORTB == VPS_CAPT_INST_VIP0_PORTB);
    UTILS_COMPILETIME_ASSERT(SYSTEM_CAPTURE_INST_VIP1_PORTA == VPS_CAPT_INST_VIP1_PORTA);
    UTILS_COMPILETIME_ASSERT(SYSTEM_CAPTURE_INST_VIP1_PORTB == VPS_CAPT_INST_VIP1_PORTB);
    UTILS_COMPILETIME_ASSERT(SYSTEM_CAPTURE_INST_MAX == VPS_CAPT_INST_MAX);

    UTILS_COMPILETIME_ASSERT(SYSTEM_DEVICE_VID_DEC_DRV_BASE == VPS_VID_DEC_DRV_BASE);
    UTILS_COMPILETIME_ASSERT(SYSTEM_DEVICE_VID_DEC_TVP5158_DRV ==
                 FVID2_VPS_VID_DEC_TVP5158_DRV);
    UTILS_COMPILETIME_ASSERT(SYSTEM_DEVICE_VID_DEC_TVP7002_DRV ==
                 FVID2_VPS_VID_DEC_TVP7002_DRV);
    UTILS_COMPILETIME_ASSERT(SYSTEM_DEVICE_VID_DEC_SII9135_DRV ==
                 FVID2_VPS_VID_DEC_SII9135_DRV);
    UTILS_COMPILETIME_ASSERT(SYSTEM_DEVICE_VID_DEC_SII9233A_DRV ==
                 FVID2_VPS_VID_DEC_SII9233A_DRV);

    UTILS_COMPILETIME_ASSERT(SYSTEM_DF_YUV422I_UYVY == FVID2_DF_YUV422I_UYVY);
    UTILS_COMPILETIME_ASSERT(SYSTEM_DF_YUV422I_YUYV == FVID2_DF_YUV422I_YUYV);
    UTILS_COMPILETIME_ASSERT(SYSTEM_DF_YUV422I_YVYU == FVID2_DF_YUV422I_YVYU);
    UTILS_COMPILETIME_ASSERT(SYSTEM_DF_YUV422I_VYUY == FVID2_DF_YUV422I_VYUY);
    UTILS_COMPILETIME_ASSERT(SYSTEM_DF_YUV422SP_UV == FVID2_DF_YUV422SP_UV);
    UTILS_COMPILETIME_ASSERT(SYSTEM_DF_YUV422SP_VU == FVID2_DF_YUV422SP_VU);
    UTILS_COMPILETIME_ASSERT(SYSTEM_DF_YUV422P == FVID2_DF_YUV422P);
    UTILS_COMPILETIME_ASSERT(SYSTEM_DF_YUV420SP_UV == FVID2_DF_YUV420SP_UV);
    UTILS_COMPILETIME_ASSERT(SYSTEM_DF_YUV420SP_VU == FVID2_DF_YUV420SP_VU);
    UTILS_COMPILETIME_ASSERT(SYSTEM_DF_YUV420P == FVID2_DF_YUV420P);
    UTILS_COMPILETIME_ASSERT(SYSTEM_DF_YUV444P == FVID2_DF_YUV444P);
    UTILS_COMPILETIME_ASSERT(SYSTEM_DF_YUV444I == FVID2_DF_YUV444I);
    UTILS_COMPILETIME_ASSERT(SYSTEM_DF_RGB16_565 == FVID2_DF_RGB16_565);
    UTILS_COMPILETIME_ASSERT(SYSTEM_DF_RGB24_888 == FVID2_DF_RGB24_888);
    UTILS_COMPILETIME_ASSERT(SYSTEM_DF_BGRA32_8888 == FVID2_DF_BGRA32_8888);
    UTILS_COMPILETIME_ASSERT(SYSTEM_DF_BITMAP8 == FVID2_DF_BITMAP8);
    UTILS_COMPILETIME_ASSERT(SYSTEM_DF_BITMAP1_BGRA32_OFFSET7 ==
                 FVID2_DF_BITMAP1_BGRA32_OFFSET7);
    UTILS_COMPILETIME_ASSERT(SYSTEM_DF_BAYER_RAW == FVID2_DF_BAYER_RAW);
    UTILS_COMPILETIME_ASSERT(SYSTEM_DF_RAW_VBI == FVID2_DF_RAW_VBI);
    UTILS_COMPILETIME_ASSERT(SYSTEM_DF_RAW == FVID2_DF_RAW);
    UTILS_COMPILETIME_ASSERT(SYSTEM_DF_MISC == FVID2_DF_MISC);
    UTILS_COMPILETIME_ASSERT(SYSTEM_DF_INVALID == FVID2_DF_INVALID);


    UTILS_COMPILETIME_ASSERT(SYSTEM_SF_INTERLACED == FVID2_SF_INTERLACED);
    UTILS_COMPILETIME_ASSERT(SYSTEM_SF_PROGRESSIVE == FVID2_SF_PROGRESSIVE);
    UTILS_COMPILETIME_ASSERT(SYSTEM_SF_MAX == FVID2_SF_MAX);

    UTILS_COMPILETIME_ASSERT(SYSTEM_MT_NONTILEDMEM == VPS_VPDMA_MT_NONTILEDMEM);
    UTILS_COMPILETIME_ASSERT(SYSTEM_MT_TILEDMEM == VPS_VPDMA_MT_TILEDMEM);
    UTILS_COMPILETIME_ASSERT(SYSTEM_MT_MAX == VPS_VPDMA_MT_MAX);

    UTILS_COMPILETIME_ASSERT(SYSTEM_STD_NTSC == FVID2_STD_NTSC);
    UTILS_COMPILETIME_ASSERT(SYSTEM_STD_PAL == FVID2_STD_PAL);
    UTILS_COMPILETIME_ASSERT(SYSTEM_STD_480I == FVID2_STD_480I);
    UTILS_COMPILETIME_ASSERT(SYSTEM_STD_576I == FVID2_STD_576I);
    UTILS_COMPILETIME_ASSERT(SYSTEM_STD_CIF == FVID2_STD_CIF);
    UTILS_COMPILETIME_ASSERT(SYSTEM_STD_HALF_D1 == FVID2_STD_HALF_D1);
    UTILS_COMPILETIME_ASSERT(SYSTEM_STD_D1 == FVID2_STD_D1);
    UTILS_COMPILETIME_ASSERT(SYSTEM_STD_480P == FVID2_STD_480P);
    UTILS_COMPILETIME_ASSERT(SYSTEM_STD_576P == FVID2_STD_576P);
    UTILS_COMPILETIME_ASSERT(SYSTEM_STD_720P_60 == FVID2_STD_720P_60);
    UTILS_COMPILETIME_ASSERT(SYSTEM_STD_720P_50 == FVID2_STD_720P_50);
    UTILS_COMPILETIME_ASSERT(SYSTEM_STD_720P_30 == FVID2_STD_720P_30);
    UTILS_COMPILETIME_ASSERT(SYSTEM_STD_720P_25 == FVID2_STD_720P_25);
    UTILS_COMPILETIME_ASSERT(SYSTEM_STD_720P_24 == FVID2_STD_720P_24);
    UTILS_COMPILETIME_ASSERT(SYSTEM_STD_1080I_60 == FVID2_STD_1080I_60);
    UTILS_COMPILETIME_ASSERT(SYSTEM_STD_1080I_50 == FVID2_STD_1080I_50);
    UTILS_COMPILETIME_ASSERT(SYSTEM_STD_1080P_60 == FVID2_STD_1080P_60);
    UTILS_COMPILETIME_ASSERT(SYSTEM_STD_1080P_50 == FVID2_STD_1080P_50);
    UTILS_COMPILETIME_ASSERT(SYSTEM_STD_1080P_30 == FVID2_STD_1080P_30);
    UTILS_COMPILETIME_ASSERT(SYSTEM_STD_1080P_25 == FVID2_STD_1080P_25);
    UTILS_COMPILETIME_ASSERT(SYSTEM_STD_1080P_24 == FVID2_STD_1080P_24);
    UTILS_COMPILETIME_ASSERT(SYSTEM_STD_VGA_60 == FVID2_STD_VGA_60);
    UTILS_COMPILETIME_ASSERT(SYSTEM_STD_VGA_72 == FVID2_STD_VGA_72);
    UTILS_COMPILETIME_ASSERT(SYSTEM_STD_VGA_75 == FVID2_STD_VGA_75);
    UTILS_COMPILETIME_ASSERT(SYSTEM_STD_VGA_85 == FVID2_STD_VGA_85);
    UTILS_COMPILETIME_ASSERT(SYSTEM_STD_SVGA_60 == FVID2_STD_SVGA_60);
    UTILS_COMPILETIME_ASSERT(SYSTEM_STD_SVGA_72 == FVID2_STD_SVGA_72);
    UTILS_COMPILETIME_ASSERT(SYSTEM_STD_SVGA_75 == FVID2_STD_SVGA_75);
    UTILS_COMPILETIME_ASSERT(SYSTEM_STD_SVGA_85 == FVID2_STD_SVGA_85);
    UTILS_COMPILETIME_ASSERT(SYSTEM_STD_XGA_60 == FVID2_STD_XGA_60);
    UTILS_COMPILETIME_ASSERT(SYSTEM_STD_XGA_70 == FVID2_STD_XGA_70);
    UTILS_COMPILETIME_ASSERT(SYSTEM_STD_XGA_75 == FVID2_STD_XGA_75);
    UTILS_COMPILETIME_ASSERT(SYSTEM_STD_XGA_85 == FVID2_STD_XGA_85);
    UTILS_COMPILETIME_ASSERT(SYSTEM_STD_WXGA_60 == FVID2_STD_WXGA_60);
    UTILS_COMPILETIME_ASSERT(SYSTEM_STD_WXGA_75 == FVID2_STD_WXGA_75);
    UTILS_COMPILETIME_ASSERT(SYSTEM_STD_WXGA_85 == FVID2_STD_WXGA_85);
    UTILS_COMPILETIME_ASSERT(SYSTEM_STD_SXGA_60 == FVID2_STD_SXGA_60);
    UTILS_COMPILETIME_ASSERT(SYSTEM_STD_SXGA_75 == FVID2_STD_SXGA_75);
    UTILS_COMPILETIME_ASSERT(SYSTEM_STD_SXGA_85 == FVID2_STD_SXGA_85);
    UTILS_COMPILETIME_ASSERT(SYSTEM_STD_SXGAP_60 == FVID2_STD_SXGAP_60);
    UTILS_COMPILETIME_ASSERT(SYSTEM_STD_SXGAP_75 == FVID2_STD_SXGAP_75);
    UTILS_COMPILETIME_ASSERT(SYSTEM_STD_UXGA_60 == FVID2_STD_UXGA_60);
    UTILS_COMPILETIME_ASSERT(SYSTEM_STD_WUXGA_60 == FVID2_STD_WUXGA_60);
    UTILS_COMPILETIME_ASSERT(SYSTEM_STD_MUX_2CH_D1 == FVID2_STD_MUX_2CH_D1);
    UTILS_COMPILETIME_ASSERT(SYSTEM_STD_MUX_2CH_HALF_D1 == FVID2_STD_MUX_2CH_HALF_D1);
    UTILS_COMPILETIME_ASSERT(SYSTEM_STD_MUX_2CH_CIF == FVID2_STD_MUX_2CH_CIF);
    UTILS_COMPILETIME_ASSERT(SYSTEM_STD_MUX_4CH_D1 == FVID2_STD_MUX_4CH_D1);
    UTILS_COMPILETIME_ASSERT(SYSTEM_STD_MUX_4CH_CIF == FVID2_STD_MUX_4CH_CIF);
    UTILS_COMPILETIME_ASSERT(SYSTEM_STD_MUX_4CH_HALF_D1 == FVID2_STD_MUX_4CH_HALF_D1);
    UTILS_COMPILETIME_ASSERT(SYSTEM_STD_MUX_8CH_CIF == FVID2_STD_MUX_8CH_CIF);
    UTILS_COMPILETIME_ASSERT(SYSTEM_STD_MUX_8CH_HALF_D1 == FVID2_STD_MUX_8CH_HALF_D1);
    UTILS_COMPILETIME_ASSERT(SYSTEM_STD_AUTO_DETECT == FVID2_STD_AUTO_DETECT);
    UTILS_COMPILETIME_ASSERT(SYSTEM_STD_CUSTOM == FVID2_STD_CUSTOM);


    UTILS_COMPILETIME_ASSERT(SYSTEM_PLATFORM_ID_UNKNOWN == VPS_PLATFORM_ID_UNKNOWN);
    UTILS_COMPILETIME_ASSERT(SYSTEM_PLATFORM_ID_EVM_TI816x == VPS_PLATFORM_ID_EVM_TI816x);
    UTILS_COMPILETIME_ASSERT(SYSTEM_PLATFORM_ID_SIM_TI816x == VPS_PLATFORM_ID_SIM_TI816x);
    UTILS_COMPILETIME_ASSERT(SYSTEM_PLATFORM_ID_EVM_TI814x == VPS_PLATFORM_ID_EVM_TI814x);
    UTILS_COMPILETIME_ASSERT(SYSTEM_PLATFORM_ID_SIM_TI814x == VPS_PLATFORM_ID_SIM_TI814x);
    UTILS_COMPILETIME_ASSERT(SYSTEM_PLATFORM_ID_EVM_TI8107 == VPS_PLATFORM_ID_EVM_TI8107);
    UTILS_COMPILETIME_ASSERT(SYSTEM_PLATFORM_ID_MAX == VPS_PLATFORM_ID_MAX);

#if 0 // KC: commented since this is not needed, VPS_PLATFORM_xxx is not used in our system
    UTILS_COMPILETIME_ASSERT(SYSTEM_PLATFORM_BOARD_UNKNOWN == VPS_PLATFORM_BOARD_UNKNOWN);
    UTILS_COMPILETIME_ASSERT(SYSTEM_PLATFORM_BOARD_VS == VPS_PLATFORM_BOARD_VS);
    UTILS_COMPILETIME_ASSERT(SYSTEM_PLATFORM_BOARD_VC == VPS_PLATFORM_BOARD_VC);
    UTILS_COMPILETIME_ASSERT(SYSTEM_PLATFORM_BOARD_CATALOG == VPS_PLATFORM_BOARD_CATALOG);
    UTILS_COMPILETIME_ASSERT(SYSTEM_PLATFORM_BOARD_MAX == VPS_PLATFORM_BOARD_MAX);
#endif

    UTILS_COMPILETIME_ASSERT(SYSTEM_PLATFORM_CPU_REV_1_0 == VPS_PLATFORM_CPU_REV_1_0);
    UTILS_COMPILETIME_ASSERT(SYSTEM_PLATFORM_CPU_REV_1_1 == VPS_PLATFORM_CPU_REV_1_1);
    UTILS_COMPILETIME_ASSERT(SYSTEM_PLATFORM_CPU_REV_2_0 == VPS_PLATFORM_CPU_REV_2_0);
    UTILS_COMPILETIME_ASSERT(SYSTEM_PLATFORM_CPU_REV_2_1 == VPS_PLATFORM_CPU_REV_2_1);
    UTILS_COMPILETIME_ASSERT(SYSTEM_PLATFORM_CPU_REV_UNKNOWN ==
                 VPS_PLATFORM_CPU_REV_UNKNOWN);
    UTILS_COMPILETIME_ASSERT(SYSTEM_PLATFORM_CPU_REV_MAX == VPS_PLATFORM_CPU_REV_MAX);

#if 0 // KC: commented since this is not needed, VPS_PLATFORM_xxx is not used in our system
    UTILS_COMPILETIME_ASSERT(SYSTEM_PLATFORM_BOARD_REV_UNKNOWN ==
                 VPS_PLATFORM_BOARD_REV_UNKNOWN);
    UTILS_COMPILETIME_ASSERT(SYSTEM_PLATFORM_BOARD_REV_A == VPS_PLATFORM_BOARD_REV_A);
    UTILS_COMPILETIME_ASSERT(SYSTEM_PLATFORM_BOARD_REV_B == VPS_PLATFORM_BOARD_REV_B);
    UTILS_COMPILETIME_ASSERT(SYSTEM_PLATFORM_BOARD_REV_C == VPS_PLATFORM_BOARD_REV_C);
    UTILS_COMPILETIME_ASSERT(SYSTEM_PLATFORM_BOARD_REV_MAX == VPS_PLATFORM_BOARD_REV_MAX);
#endif

#if defined(TI816X_EVM) || defined(TI8107_EVM) || defined(TI8107_DVR)
    UTILS_COMPILETIME_ASSERT(SYSTEM_THS7360_DISABLE_SF == VPS_THS7360_DISABLE_SF);
    UTILS_COMPILETIME_ASSERT(SYSTEM_THS7360_BYPASS_SF == VPS_THS7360_BYPASS_SF);
    UTILS_COMPILETIME_ASSERT(SYSTEM_THS7360_SF_SD_MODE == VPS_THS7360_SF_SD_MODE);
    UTILS_COMPILETIME_ASSERT(SYSTEM_THS7360_SF_ED_MODE == VPS_THS7360_SF_ED_MODE);
    UTILS_COMPILETIME_ASSERT(SYSTEM_THS7360_SF_HD_MODE == VPS_THS7360_SF_HD_MODE);
    UTILS_COMPILETIME_ASSERT(SYSTEM_THS7360_SF_TRUE_HD_MODE == VPS_THS7360_SF_TRUE_HD_MODE);

    UTILS_COMPILETIME_ASSERT(SYSTEM_THSFILTER_ENABLE_MODULE == VPS_THSFILTER_ENABLE_MODULE);
    UTILS_COMPILETIME_ASSERT(SYSTEM_THSFILTER_BYPASS_MODULE == VPS_THSFILTER_BYPASS_MODULE);
    UTILS_COMPILETIME_ASSERT(SYSTEM_THSFILTER_DISABLE_MODULE ==
                 VPS_THSFILTER_DISABLE_MODULE);
#endif

    return 0;
}
