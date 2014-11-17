
#include <SD_demo.h>
#include <ti_swosd_logo_224x30_yuv420sp.h>
#include <ti_swosd_logo_224x30_yuv422i.h>

#define OSD_BUF_HEAP_SR_ID          (0)

static UInt8   *osdBufBaseVirtAddr = NULL;
static UInt32   osdTotalBufSize = 0;
AlgLink_OsdChWinParams g_osdChParam[ALG_LINK_OSD_MAX_CH];

Int32 SD_Demo_osdInit(UInt32 numCh, UInt8 *osdFormat)
{
    int chId, winId, status;

    SD_Demo_AllocBufInfo bufInfo;
    UInt32 osdBufSize, osdBufSizeY, bufAlign;

    UInt32 bufOffset;
    UInt8 *curVirtAddr;

    assert(numCh <= ALG_LINK_OSD_MAX_CH);

    osdBufSizeY = SD_DEMO_OSD_WIN_PITCH*SD_DEMO_OSD_WIN_HEIGHT;

    osdBufSize = osdBufSizeY * 2 ;

    /* All channels share the same OSD window buffers, this is just for demo, actually each CH
        can have different OSD buffers
    */
    osdTotalBufSize = osdBufSize * SD_DEMO_OSD_NUM_WINDOWS;
    bufAlign = 128;

    status = SD_Demo_allocBuf(OSD_BUF_HEAP_SR_ID, osdTotalBufSize, bufAlign, &bufInfo);
    OSA_assert(status==OSA_SOK);

    osdBufBaseVirtAddr = bufInfo.virtAddr;

    for(chId = 0; chId < numCh; chId++)
    {
        AlgLink_OsdChWinParams * chWinPrm = &g_osdChParam[chId];

        chWinPrm->chId = chId;
        chWinPrm->numWindows = SD_DEMO_OSD_NUM_WINDOWS;


        bufInfo.virtAddr = osdBufBaseVirtAddr;

        for(winId=0; winId < chWinPrm->numWindows; winId++)
        {
            chWinPrm->winPrm[winId].startX             = SD_DEMO_OSD_WIN0_STARTX ;
            chWinPrm->winPrm[winId].startY             = SD_DEMO_OSD_WIN0_STARTY + (SD_DEMO_OSD_WIN_HEIGHT+SD_DEMO_OSD_WIN0_STARTY)*winId;
            chWinPrm->winPrm[winId].width              = SD_DEMO_OSD_WIN_WIDTH;
            chWinPrm->winPrm[winId].height             = SD_DEMO_OSD_WIN_HEIGHT;
            chWinPrm->winPrm[winId].lineOffset         = SD_DEMO_OSD_WIN_PITCH;
            chWinPrm->winPrm[winId].globalAlpha        = SD_DEMO_OSD_GLOBAL_ALPHA/(winId+1);
            chWinPrm->winPrm[winId].transperencyEnable = SD_DEMO_OSD_TRANSPARENCY;
            chWinPrm->winPrm[winId].enableWin          = SD_DEMO_OSD_ENABLE_WIN;


            bufOffset = osdBufSize * winId;

            chWinPrm->winPrm[winId].addr[0][0] = (bufInfo.physAddr + bufOffset);

            curVirtAddr = bufInfo.virtAddr + bufOffset;

            /* copy logo to buffer  */
            if(osdFormat[chId] == SYSTEM_DF_YUV422I_YUYV)
            {
                chWinPrm->winPrm[winId].format     = SYSTEM_DF_YUV422I_YUYV;
                chWinPrm->winPrm[winId].addr[0][1] = NULL;
                OSA_assert(sizeof(gMCFW_swosdTiLogoYuv422i)<=osdBufSize);
                memcpy(curVirtAddr, gMCFW_swosdTiLogoYuv422i, sizeof(gMCFW_swosdTiLogoYuv422i));
            }
            else
            {
                chWinPrm->winPrm[winId].format     = SYSTEM_DF_YUV420SP_UV;
                chWinPrm->winPrm[winId].addr[0][1] =  chWinPrm->winPrm[winId].addr[0][0] + osdBufSizeY;
                OSA_assert(sizeof(gMCFW_swosdTiLogoYuv420sp)<= osdBufSize);
                memcpy(curVirtAddr, gMCFW_swosdTiLogoYuv420sp, sizeof(gMCFW_swosdTiLogoYuv420sp));
            }
        }
    }

    for(chId = 0; chId < numCh; chId++)
    {
       AlgLink_OsdChWinParams * osdChWinPrm;

       osdChWinPrm = &g_osdChParam[chId];
        status = System_linkControl(
                                gSD_Demo_ctrl.dspAlgId[0],
                                ALG_LINK_OSD_CMD_SET_CHANNEL_WIN_PRM,
                                osdChWinPrm,
                                sizeof(AlgLink_OsdChWinParams),
                                TRUE
                                );
    }
    return status;
}

Void SD_Demo_osdDeinit()
{
	if(osdBufBaseVirtAddr != NULL)
	{
		SD_Demo_freeBuf(OSD_BUF_HEAP_SR_ID, osdBufBaseVirtAddr, osdTotalBufSize);
	}
}