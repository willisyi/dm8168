/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include "osdLink_priv.h"


Int32 AlgLink_OsdalgCreate(AlgLink_OsdObj * pObj)
{
	Int32 status, chId;
    SWOSD_OpenPrm algCreatePrm;
    AlgLink_OsdChObj *pChObj;
    SWOSD_Obj *pSwOsdObj;
    AlgLink_OsdChWinParams *pChWinPrm;


    algCreatePrm.maxWidth =
        pObj->osdChCreateParams[0].maxWidth;
    algCreatePrm.maxHeight =
		pObj->osdChCreateParams[0].maxHeight;

    /* Create algorithm instance and get algo handle  */
	status = SWOSD_open(&pObj->osdObj, &algCreatePrm);

	UTILS_assert(status == 0);

    for(chId=0; chId<pObj->inQueInfo->numCh; chId++)
    {
        pChObj = &pObj->chObj[chId];
        pChObj->colorKey[0] = 0xFF; /* Y */
        pChObj->colorKey[1] = 0xFF; /* U */
        pChObj->colorKey[2] = 0xFF; /* V */

        pChWinPrm = &pObj->osdChCreateParams[chId].chDefaultParams;

        pSwOsdObj = &pChObj->osdObj;

        pChWinPrm->chId = chId;

        status = AlgLink_OsdalgSetChOsdWinPrm(pObj, pChWinPrm);

        UTILS_assert(status==0);

        pSwOsdObj->algHndl = pObj->osdObj.algHndl;

        pSwOsdObj->openPrm = pObj->osdObj.openPrm;

        pSwOsdObj->videoWindowPrm.format = SWOSD_FORMAT_YUV422i;
        pSwOsdObj->videoWindowPrm.startX = pObj->inQueInfo->chInfo[chId].startX;
        pSwOsdObj->videoWindowPrm.startY = pObj->inQueInfo->chInfo[chId].startY;
        pSwOsdObj->videoWindowPrm.width  = pObj->inQueInfo->chInfo[chId].width;
        pSwOsdObj->videoWindowPrm.height = pObj->inQueInfo->chInfo[chId].height;
        pSwOsdObj->videoWindowPrm.lineOffset = pObj->inQueInfo->chInfo[chId].pitch[0];

    }

	return FVID2_SOK;
}

Int32 AlgLink_OsdalgDelete(AlgLink_OsdObj * pObj)
{
    SWOSD_close(&pObj->osdObj);

	return FVID2_SOK;
}

/*
    Returns 32-bit color key thats needs to be programmed to the SW OSD algorithm

    colorKey[0] = Y color Key
    colorKey[1] = U color Key
    colorKey[2] = V color Key

    dataFormat - SWOSD_FORMAT_YUV422i or SWOSD_FORMAT_YUV420sp

    place: 0 - Y plane, 1: C plane
*/
Int32 AlgLink_OsdalgGetColorKey(UInt32 *colorKey, UInt32 dataFormat, UInt32 plane)
{
    UInt32 colorKeyY;
    UInt32 colorKeyU;
    UInt32 colorKeyV;
    UInt32 value;

    colorKeyY = (UInt8)colorKey[0];
    colorKeyU = (UInt8)colorKey[1];
    colorKeyV = (UInt8)colorKey[2];

    if(dataFormat == SWOSD_FORMAT_YUV422i)
    {
        value =
             (colorKeyY <<0)
            |(colorKeyU <<8)
            |(colorKeyY <<16)
            |(colorKeyV <<24)
            ;
    }
    else
    {
        if(plane==0)
        {
            value =
                 (colorKeyY <<0)
                |(colorKeyY <<8)
                |(colorKeyY <<16)
                |(colorKeyY <<24)
                ;
        }
        else
        {
            value =
                 (colorKeyU <<0)
                |(colorKeyV <<8)
                |(colorKeyU <<16)
                |(colorKeyV <<24)
                ;
        }
    }

    return value;
}

void AlgLink_OsdalgPrintInfo(SWOSD_Obj *pSwOsdObj, FVID2_Frame *pFrame)
{
    Vps_printf(" SWOSD: CH%d: VID: addr=0x%X start=%d,%d %dx%d, pitch=%d ; GRPX: start=%d,%d %dx%d, pitch=%d\n",
        pFrame->channelNum,
        pSwOsdObj->videoWindowAddr,
        pSwOsdObj->videoWindowPrm.startX,
        pSwOsdObj->videoWindowPrm.startY,
        pSwOsdObj->videoWindowPrm.width,
        pSwOsdObj->videoWindowPrm.height,
        pSwOsdObj->videoWindowPrm.lineOffset,
        pSwOsdObj->graphicsWindowPrm.startX,
        pSwOsdObj->graphicsWindowPrm.startY,
        pSwOsdObj->graphicsWindowPrm.width,
        pSwOsdObj->graphicsWindowPrm.height,
        pSwOsdObj->graphicsWindowPrm.lineOffset
    );
}

Int32 AlgLink_OsdalgProcessFrame(AlgLink_OsdObj * pObj, FVID2_Frame *pFrame)
{
    UInt32 winId, fid, scaleX, divY;
    AlgLink_OsdChObj *pChObj;
    SWOSD_Obj *pSwOsdObj;
    System_FrameInfo *pFrameInfo;
    UInt32 algColorKey[2];
    Bool isInterlaced, isTiled;

    isInterlaced    = FALSE;
    isTiled         = FALSE;
    fid             = 0;
    scaleX          = 1;
    divY            = 1;

    pChObj = &pObj->chObj[pFrame->channelNum];

    pSwOsdObj = &pChObj->osdObj;


    if(pObj->inQueInfo->chInfo[pFrame->channelNum].scanFormat == SYSTEM_SF_INTERLACED)
        isInterlaced = TRUE;

    if(SYSTEM_MT_TILEDMEM == pObj->inQueInfo->chInfo[pFrame->channelNum].memType)
        isTiled = TRUE;

    if(isInterlaced)
    {
        /* OSD plane is always progressive
           Input can be interlaced in this case we need to skip alternate lines in OSD plane
           and feed for blending
        */
        if(pFrame->fid==1)
            fid = 1;

        /* this will half the processing height */
        divY = 2;
    }

    if(pSwOsdObj->graphicsWindowPrm.format == SWOSD_FORMAT_YUV422i)
        scaleX = 2;

    algColorKey[0] = AlgLink_OsdalgGetColorKey(
                        pChObj->colorKey,
                        pSwOsdObj->graphicsWindowPrm.format,
                        0
                     );

    algColorKey[1] = 0;

    if(pSwOsdObj->graphicsWindowPrm.format == SWOSD_FORMAT_YUV420sp)
    {
        algColorKey[1] = AlgLink_OsdalgGetColorKey(
                        pChObj->colorKey,
                        pSwOsdObj->graphicsWindowPrm.format,
                        1
                      );
    }

    /* NOT SUPPORTED */
    pSwOsdObj->alphaWindowAddr = NULL;

    pFrameInfo = (System_FrameInfo*)pFrame->appData;

    UTILS_assert(pFrameInfo!=NULL);

    if(pFrameInfo->rtChInfoUpdate)
    {
        pSwOsdObj->videoWindowPrm.startX = pFrameInfo->rtChInfo.startX;
        pSwOsdObj->videoWindowPrm.startY = pFrameInfo->rtChInfo.startY;
        pSwOsdObj->videoWindowPrm.width  = pFrameInfo->rtChInfo.width;
        pSwOsdObj->videoWindowPrm.height = pFrameInfo->rtChInfo.height;
        pSwOsdObj->videoWindowPrm.lineOffset = pFrameInfo->rtChInfo.pitch[0];
    }

    for(winId=0; winId<pChObj->numWindows; winId++)
    {
        if(!pChObj->osdWinObj[winId].enableWin)
            continue;

        /* YUV422i or YUV420SP - Y-plane processing */

        pSwOsdObj->videoWindowAddr = pFrame->addr[0][0];

        if (isTiled)
        {
            pSwOsdObj->videoWindowAddr =
                (Ptr)Utils_tilerAddr2CpuAddr((UInt32)pFrame->addr[0][0]);
            pSwOsdObj->videoWindowPrm.lineOffset =  VPSUTILS_TILER_CNT_8BIT_PITCH;
        }

        pSwOsdObj->globalPrm.globalAlpha        = pChObj->osdWinObj[winId].globalAlpha;
        pSwOsdObj->globalPrm.transperencyEnable = pChObj->osdWinObj[winId].transperencyEnable;
        pSwOsdObj->globalPrm.transperencyColor32= algColorKey[0];

        pSwOsdObj->graphicsWindowPrm            = pChObj->osdWinObj[winId].osdWinPrm;

        pSwOsdObj->graphicsWindowAddr           =
                        pChObj->osdWinObj[winId].addr[0][0] + fid*pSwOsdObj->graphicsWindowPrm.lineOffset*scaleX;

        pSwOsdObj->graphicsWindowPrm.startX     *= scaleX;
        pSwOsdObj->graphicsWindowPrm.startY     /= divY;
        pSwOsdObj->graphicsWindowPrm.width      *=scaleX;
        pSwOsdObj->graphicsWindowPrm.height     /= divY;
        pSwOsdObj->graphicsWindowPrm.lineOffset *= (scaleX * divY); // double line offset

        #if 0
        AlgLink_OsdalgPrintInfo(pSwOsdObj, pFrame);
        #endif

        SWOSD_blendWindow(pSwOsdObj);

        /* YUV420SP - C -plane processing */
        if(pSwOsdObj->graphicsWindowPrm.format == SWOSD_FORMAT_YUV420sp)
        {
            pSwOsdObj->videoWindowAddr = pFrame->addr[0][1];
            if (isTiled)
            {
                pSwOsdObj->videoWindowAddr =
                    (Ptr)Utils_tilerAddr2CpuAddr((UInt32)pFrame->addr[0][1]);
                pSwOsdObj->videoWindowPrm.lineOffset =  VPSUTILS_TILER_CNT_16BIT_PITCH;
            }

            pSwOsdObj->graphicsWindowAddr           =
                        pChObj->osdWinObj[winId].addr[0][1] + fid*pSwOsdObj->graphicsWindowPrm.lineOffset*scaleX;

            pSwOsdObj->graphicsWindowPrm.startY /= 2;    // half width  for C plane
            pSwOsdObj->graphicsWindowPrm.height /= 2;    // half height for C plane

            pSwOsdObj->globalPrm.transperencyColor32= algColorKey[1];

            #if 0
            AlgLink_OsdalgPrintInfo(pSwOsdObj, pFrame);
            #endif

            SWOSD_blendWindow(pSwOsdObj);
        }
    }

    return 0;
}


Int32 AlgLink_OsdalgSetChOsdWinPrm(AlgLink_OsdObj * pObj,
                            AlgLink_OsdChWinParams * params)
{
	Int32 status = 0;
	AlgLink_OsdChObj *pChObj;
    UInt32 i;

    if(params->chId >= pObj->inQueInfo->numCh)
        return -1;

	pChObj = &pObj->chObj[params->chId];

	pChObj->numWindows         = params->numWindows;
	for(i = 0; i < params->numWindows; i++)
	{
		AlgLink_OsdWinObj *osdWinObj = &pChObj->osdWinObj[i];

		osdWinObj->enableWin          = params->winPrm[i].enableWin;
		osdWinObj->transperencyEnable = params->winPrm[i].transperencyEnable;
		osdWinObj->globalAlpha        = params->winPrm[i].globalAlpha;


		osdWinObj->addr[0][0] = params->winPrm[i].addr[0][0];
		osdWinObj->addr[0][1] = params->winPrm[i].addr[0][1];

		if(params->winPrm[i].format == FVID2_DF_YUV422I_YUYV)
		{
			osdWinObj->osdWinPrm.format = SWOSD_FORMAT_YUV422i;
		}
		else if(params->winPrm[i].format == FVID2_DF_YUV420SP_UV)
		{
			osdWinObj->osdWinPrm.format = SWOSD_FORMAT_YUV420sp;
		}
		else
		{
			osdWinObj->osdWinPrm.format = -1;
		}

		osdWinObj->osdWinPrm.startX     = params->winPrm[i].startX;
		osdWinObj->osdWinPrm.startY     = params->winPrm[i].startY;
		osdWinObj->osdWinPrm.width      = params->winPrm[i].width;
		osdWinObj->osdWinPrm.height     = params->winPrm[i].height;
		osdWinObj->osdWinPrm.lineOffset = params->winPrm[i].lineOffset;

        pChObj->colorKey[0] = params->colorKey[0];
        pChObj->colorKey[1] = params->colorKey[1];
        pChObj->colorKey[2] = params->colorKey[2];
	}

	return (status);
}




