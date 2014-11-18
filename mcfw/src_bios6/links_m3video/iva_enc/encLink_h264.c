/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include <xdc/std.h>
#include <ti/xdais/xdas.h>
#include <ti/xdais/ialg.h>
#include <ti/sdo/fc/utils/api/alg.h>
#include <ti/sdo/fc/rman/rman.h>
#include <ti/xdais/dm/ividenc2.h>
#include <ih264enc.h>
#include <h264enc_ti.h>

#include "encLink_priv.h"
#include "encLink_h264_priv.h"
#include <mcfw/src_bios6/links_m3video/codec_utils/utils_encdec.h>
#include <mcfw/src_bios6/links_m3video/codec_utils/iresman_hdvicp2_earlyacquire.h>

#define ENCLINK_H264_SETNALU_MASK_SPS(naluMask) ((naluMask) |= (1 << IH264_NALU_TYPE_SPS_WITH_VUI))
#define ENCLINK_H264_SETNALU_MASK_PPS(naluMask) ((naluMask) |= (1 << IH264_NALU_TYPE_PPS))
#define ENCLINK_H264_SETNALU_MASK_SEI(naluMask) ((naluMask) |= (1 << IH264_NALU_TYPE_SEI))

#define ENCLINK_H264_PREFIX_NAL_UNIT_TYPE1 (14)
#define ENCLINK_H264_PREFIX_NAL_UNIT_TYPE2 (20)

static IH264ENC_Handle enc_link_h264_create(const IH264ENC_Fxns * fxns,
                                            const IH264ENC_Params * prms);
static Void enc_link_h264_delete(IH264ENC_Handle handle);
static Int32 enclink_h264_control(IH264ENC_Handle handle,
                                  IH264ENC_Cmd cmd,
                                  IH264ENC_DynamicParams * params,
                                  IH264ENC_Status * status);
static Int enclink_h264_set_static_params(IH264ENC_Params * staticParams,
                                          EncLink_AlgCreateParams *
                                          algCreateParams);
static Int enclink_h264_set_algObject(EncLink_H264Obj * algObj,
                                      EncLink_AlgCreateParams * algCreateParams,
                                      EncLink_AlgDynamicParams *
                                      algDynamicParams);
static Int enclink_h264_set_dynamic_params(IH264ENC_DynamicParams *
                                           dynamicParams,
                                           EncLink_AlgDynamicParams *
                                           algDynamicParams);
static Void enclink_h264_freersrc(EncLink_H264Obj * hObj, Int rsrcMask);

static UInt32 enc_link_h264_get_svc_temporalid(Int32 *frameAddr, 
                                                     Int32 frameBytes);
static Int32 EncLink_h264EncoderReset(EncLink_H264Obj * hObj);

extern IRES_Fxns H264ENC_TI_IRES;
extern const IH264ENC_DynamicParams H264ENC_TI_DYNAMICPARAMS;

typedef struct sErrorMapping{
  XDAS_Int8 *errorName;
}sErrorMapping;

static sErrorMapping gErrorStrings[32] =
{
  (XDAS_Int8 *)"IH264ENC_LEVEL_INCOMPLAINT_PARAMETER , 0, \0",
  (XDAS_Int8 *)"IH264ENC_PROFILE_INCOMPLAINT_CONTENTTYPE = 1,\0",
  (XDAS_Int8 *)"IH264ENC_PROFILE_INCOMPLAINT_FMO_SETTING = 2,",
  (XDAS_Int8 *)"IH264ENC_PROFILE_INCOMPLAINT_TRANSFORMBLOCKSIZE = 3,\0",
  (XDAS_Int8 *)"IH264ENC_PROFILE_INCOMPLAINT_INTERFRAMEINTERVAL = 4,\0",
  (XDAS_Int8 *)"IH264ENC_PROFILE_INCOMPLAINT_SCALINGMATRIXPRESET = 5,\0",
  (XDAS_Int8 *)"IH264ENC_PROFILE_INCOMPLAINT_ENTROPYCODINGMODE = 6,\0",
  (XDAS_Int8 *)"IH264ENC_MAX_BIT_RATE_VOILATION  = 7,\0",
  (XDAS_Int8 *)"XDM_PARAMSCHANGE = 8,\0",
  (XDAS_Int8 *)"XDM_APPLIEDCONCEALMENT = 9,\0",
  (XDAS_Int8 *)"XDM_INSUFFICIENTDATA = 10,\0",
  (XDAS_Int8 *)"XDM_CORRUPTEDDATA = 11,\0",
  (XDAS_Int8 *)"XDM_CORRUPTEDHEADER = 12,\0",
  (XDAS_Int8 *)"XDM_UNSUPPORTEDINPUT = 13,\0",
  (XDAS_Int8 *)"XDM_UNSUPPORTEDPARAM = 14,\0",
  (XDAS_Int8 *)"XDM_FATALERROR = 15\0",
  (XDAS_Int8 *)"IH264ENC_IMPROPER_HDVICP2_STATE = 16\0",
  (XDAS_Int8 *)"IH264ENC_IMPROPER_STREAMFORMAT = 17,\0",
  (XDAS_Int8 *)"IH264ENC_IMPROPER_POCTYPE = 18,\0",
  (XDAS_Int8 *)"IH264ENC_IMPROPER_DATASYNC_SETTING = 19,\0",
  (XDAS_Int8 *)"IH264ENC_UNSUPPORTED_VIDENC2PARAMS = 20,\0",
  (XDAS_Int8 *)"IH264ENC_UNSUPPORTED_RATECONTROLPARAMS = 21,\0",
  (XDAS_Int8 *)"IH264ENC_UNSUPPORTED_INTERCODINGPARAMS = 22,\0",
  (XDAS_Int8 *)"IH264ENC_UNSUPPORTED_INTRACODINGPARAMS = 23,\0",
  (XDAS_Int8 *)"IH264ENC_UNSUPPORTED_NALUNITCONTROLPARAMS = 24,\0",
  (XDAS_Int8 *)"IH264ENC_UNSUPPORTED_SLICECODINGPARAMS = 25,\0",
  (XDAS_Int8 *)"IH264ENC_UNSUPPORTED_LOOPFILTERPARAMS = 26,\0",
  (XDAS_Int8 *)"IH264ENC_UNSUPPORTED_FMOCODINGPARAMS = 27,\0",
  (XDAS_Int8 *)"IH264ENC_UNSUPPORTED_VUICODINGPARAMS = 28,\0",
  (XDAS_Int8 *)"IH264ENC_UNSUPPORTED_H264ENCPARAMS = 29,\0",
  (XDAS_Int8 *)"IH264ENC_UNSUPPORTED_VIDENC2DYNAMICPARAMS = 30,\0",
  (XDAS_Int8 *)"IH264ENC_UNSUPPORTED_H264ENCDYNAMICPARAMS = 31, \0"
};

//#define ENCLINK_H264_PERFORMANCE_LOGGING

#ifdef ENCLINK_H264_PERFORMANCE_LOGGING
#define ENCLINK_H264_PROFILER_NUM_FRAMES 30
#define ENCLINK_H264_MAX_NUM_BATCHES 500
#define ENC_LINK_NUM_CH_TO_PROFILE 32
/*Array to log 128 bytes from each of the channels for 30 frames*/
unsigned int gPerformanceTrace[ENC_LINK_NUM_CH_TO_PROFILE][ENCLINK_H264_PROFILER_NUM_FRAMES][32];

unsigned int gPerfNumFramesLogged[ENC_LINK_NUM_CH_TO_PROFILE] = {0};

/*Boundaries are logged as triplicates {start, end, size}*/
unsigned int gPerfBatchBoundaries[ENCLINK_H264_MAX_NUM_BATCHES*3];
unsigned int gPerfBatchNumber = 0;

Bool gEnablePerfLog[ENC_LINK_NUM_CH_TO_PROFILE];


static void enc_link_h264_initLogStructs ()
{
	int i;
	memset (gPerfBatchBoundaries, '0xFF', sizeof(gPerfBatchBoundaries));
	memset (gPerformanceTrace, 0, sizeof(gPerformanceTrace));


	for (i = 0; i < ENC_LINK_NUM_CH_TO_PROFILE; i++)
	{
		gEnablePerfLog[i] = TRUE;
		gPerfNumFramesLogged[i] = 0;
	}
}

static Bool enc_link_h264_chkPerfLogEnable (void)
{
	int i;
	Bool returnValue = FALSE;

	for (i = 0; i < ENC_LINK_NUM_CH_TO_PROFILE; i++)
	{
		returnValue = (returnValue || gEnablePerfLog[i]);
	}

	return returnValue;
}
#endif
/**
********************************************************************************
 *  @fn     H264ENC_TI_Report_Error
 *  @brief  This function will print error messages
 *
 *          This function will check for codec errors which are mapped to
 *          extended errors in videnc2status structure and prints them in cosole
 *          Returns XDM_EFAIL in case of fatal error
 *
 *  @param[in]  uiErrorMsg  : Extended error message
 *
 *  @param[in]  fTrace_file : File pointer to the trace log file
 *
 *  @return     XDM_EOK -  when there is no fatal error
 *              XDM_EFAIL - when it is fatal error
********************************************************************************
*/


XDAS_Int32 H264ENC_TI_Report_Error(XDAS_Int32 uiErrorMsg)
{
  int i;
  if(uiErrorMsg)
  {
    /*------------------------------------------------------------------------*/
    /* Loop through all the bits in error message and map to the glibal       */
    /* error string                                                           */
    /*------------------------------------------------------------------------*/
    for (i = 0; i < 32; i ++)
    {
      if (uiErrorMsg & (1 << i))
      {
        Vps_printf("ERROR: %s \n",  gErrorStrings[i].errorName);
      }
    }
  }
  if (XDM_ISFATALERROR(uiErrorMsg))
  {
    return XDM_EFAIL;
  }
  else
  {
    return XDM_EOK;
  }
}


/*
 *  ======== enc_link_h264_create ========
 *  Create an H264ENC instance object (using parameters specified by prms)
 */

static IH264ENC_Handle enc_link_h264_create(const IH264ENC_Fxns * fxns,
                                            const IH264ENC_Params * prms)
{
#ifdef ENCLINK_H264_PERFORMANCE_LOGGING
	enc_link_h264_initLogStructs();
#endif
    return ((IH264ENC_Handle) ALG_create((IALG_Fxns *) fxns,
                                         NULL, (IALG_Params *) prms));
}

/*
 *  ======== enc_link_h264_delete ========
 *  Delete the H264ENC instance object specified by handle
 */

static Void enc_link_h264_delete(IH264ENC_Handle handle)
{
    ALG_delete((IALG_Handle) handle);
}

/*
 *  ======== enc_link_h264_control ========
 */

static Int32 enclink_h264_control(IH264ENC_Handle handle,
                                  IH264ENC_Cmd cmd,
                                  IH264ENC_DynamicParams * params,
                                  IH264ENC_Status * status)
{
    int error = 0;
    IALG_Fxns *fxns = (IALG_Fxns *) handle->fxns;

    fxns->algActivate((IALG_Handle) handle);

    error = handle->fxns->ividenc.control((IVIDENC2_Handle) handle, cmd,
                                          (IVIDENC2_DynamicParams *) params,
                                          (IVIDENC2_Status *) status);
    fxns->algDeactivate((IALG_Handle) handle);

    if (error != XDM_EOK)
    {
        ENCLINK_INTERNAL_ERROR_LOG(error, "ALGCONTROL FAILED:CMD:%d\n", cmd);
        H264ENC_TI_Report_Error(status->videnc2Status.extendedError);
    }
    return error;
}

/*
 *  ======== Enclink_h264CalcSecondFieldOffsets ========
 */

Int32 Enclink_h264CalcSecondFieldOffsets(IVIDEO2_BufDesc *inputBufDesc,
                                         FVID2_Frame *secField,
                                         Bool tilerEnable)
{
    Int retVal = ENC_LINK_S_SUCCESS;
    UInt32 addr, i;
    Int32 addrOffset, secondFieldOffsetHeight, secondFieldOffsetWidth;

    for (i=0; i<inputBufDesc->numPlanes; i++)
    {
        if (tilerEnable)
        {
            addr = Utils_tilerAddr2CpuAddr((UInt32) (secField->addr[0][i]));
        }
        else
        {
            addr = (UInt32) secField->addr[0][i];
        }

        addrOffset = addr - (UInt32)inputBufDesc->planeDesc[i].buf;


        secondFieldOffsetHeight = addrOffset/inputBufDesc->imagePitch[i];

        secondFieldOffsetWidth = addrOffset -
                       (secondFieldOffsetHeight * inputBufDesc->imagePitch[i]);

        inputBufDesc->secondFieldOffsetHeight[i] = secondFieldOffsetHeight;
        inputBufDesc->secondFieldOffsetWidth[i] = secondFieldOffsetWidth;
    }

    inputBufDesc->secondFieldOffsetHeight[2] =
                  inputBufDesc->secondFieldOffsetHeight[1];
    inputBufDesc->secondFieldOffsetWidth[2] =
                  inputBufDesc->secondFieldOffsetWidth[1];

    return (retVal);
}

/*
 *  ======== Enclink_h264EncodeFrame ========
 */
Int32 Enclink_h264EncodeFrameBatch(EncLink_Obj * pObj,
                                   EncLink_ReqBatch * reqObjBatch,
                                   Int32 tskId)
{
    int error = XDM_EFAIL;
    Int32 i;
    Int32 reqObjIndex, processListArrayIdx;
    IH264ENC_InArgs *inArgs;
    IH264ENC_OutArgs *outArgs;
    IVIDEO2_BufDesc *inputBufDesc;
    XDM2_BufDesc *outputBufDesc;
    IH264ENC_Handle handle = NULL;
    EncLink_ReqObj *pReqObj;
    IVIDEO_ContentType contentType;
    IH264ENC_ProcessParamsList processList;
    EncLink_ChObj * pChObj;
    Int32 status = ENC_LINK_S_SUCCESS;

#ifdef SYSTEM_DEBUG_MULTI_CHANNEL_ENC
    Vps_printf ("IVAHDID : %d Entering Enclink_h264EncodeFrameBatch\n", tskId);
#endif
    processList.numEntries = 0;

    /*Make sure that the req Object Batch is not empty*/
     UTILS_assert (reqObjBatch->numReqObjsInBatch > 0);

    /*Prepare the Process List that can be queued in via multiProcess call*/
    for (reqObjIndex = 0; reqObjIndex < reqObjBatch->numReqObjsInBatch;
         reqObjIndex++)
    {

        pReqObj = reqObjBatch->pReqObj[reqObjIndex];
        UTILS_assert(ENC_LINK_REQ_OBJECT_TYPE_REGULAR == pReqObj->type);

        pChObj  = &pObj->chObj[pReqObj->OutBuf->channelNum];
        
        status = EncLinkH264_algSetConfig(&pChObj->algObj);
        status = EncLinkH264_algGetConfig(&pChObj->algObj);

        if (ENC_LINK_S_SUCCESS != status)
        {
          return XDM_EFAIL;
        }

        
        inArgs = &pChObj->algObj.u.h264AlgIfObj.inArgs;
        outArgs = &pChObj->algObj.u.h264AlgIfObj.outArgs;
        inputBufDesc = &pChObj->algObj.u.h264AlgIfObj.inBufs;
        outputBufDesc = &pChObj->algObj.u.h264AlgIfObj.outBufs;
        handle = pChObj->algObj.u.h264AlgIfObj.algHandle;

        inArgs->videnc2InArgs.inputID =
                (UInt32) pReqObj->InFrameList.frames[0]->addr[0][0];

        for (i = 0; i < inputBufDesc->numPlanes; i++)
        {
            /* Set proper buffer addresses for Frame data */
          /*---------------------------------------------------------------*/
            if (pChObj->algObj.algCreateParams.tilerEnable)
            {
                inputBufDesc->planeDesc[i].buf =
                    (Ptr)
                    Utils_tilerAddr2CpuAddr((UInt32)
                          (pReqObj->InFrameList.frames[0]->addr[0][i]));
            }
            else
            {
                inputBufDesc->planeDesc[i].buf =
                              pReqObj->InFrameList.frames[0]->addr[0][i];
            }
        }

        if (pReqObj->InFrameList.numFrames == 2)
        {
            UTILS_assert (FVID2_FID_TOP == (FVID2_Fid)pReqObj->InFrameList.frames[0]->fid);
            UTILS_assert (FVID2_FID_BOTTOM == (FVID2_Fid)pReqObj->InFrameList.frames[1]->fid);
            Enclink_h264CalcSecondFieldOffsets(inputBufDesc,
                                   pReqObj->InFrameList.frames[1],
                                   pChObj->algObj.algCreateParams.tilerEnable);


            UTILS_assert ((UInt32) pReqObj->InFrameList.frames[0]->addr[0][0] +
                          inputBufDesc->imagePitch[0] *
                          (inputBufDesc->secondFieldOffsetHeight[0])
                          + inputBufDesc->secondFieldOffsetWidth[0] ==
                          (UInt32) pReqObj->InFrameList.frames[1]->addr[0][0]);

            UTILS_assert ((UInt32) pReqObj->InFrameList.frames[0]->addr[0][1] +
                          inputBufDesc->imagePitch[1]*
                          (inputBufDesc->secondFieldOffsetHeight[1])
                          + inputBufDesc->secondFieldOffsetWidth[1] ==
                          (UInt32) pReqObj->InFrameList.frames[1]->addr[0][1]);
        }

        pReqObj->OutBuf->mvDataFilledSize = 0;
        pReqObj->OutBuf->temporalId = 0;
        pReqObj->OutBuf->numTemporalLayerSetInCodec = 0;

        for (i = 0; i < outputBufDesc->numBufs; i++)
        {
            if(i == 0)
            {
              /* Set proper buffer addresses for bitstream data */
              /*---------------------------------------------------------------*/
                outputBufDesc->descs[i].buf = pReqObj->OutBuf->addr;
                /*outputBufDesc->descs[1].bufSize.bytes has been populated
                  before hand with value returned by decoder codec via the
                  GETBUFINFO XDM Control Call for the motion vector data buffer
                  size.
                */
                UTILS_assert (pReqObj->OutBuf->bufSize >
                              outputBufDesc->descs[1].bufSize.bytes);

                outputBufDesc->descs[i].bufSize.bytes = pReqObj->OutBuf->bufSize
                                        - outputBufDesc->descs[1].bufSize.bytes;
            }
            if(i == 1)
            {
                /*-------------------------------------------------------------------*/
                    /* Set proper buffer addresses for MV & SAD data */
                /*-------------------------------------------------------------------*/
                outputBufDesc->descs[1].buf = outputBufDesc->descs[0].buf +
                                           outputBufDesc->descs[0].bufSize.bytes;

                pReqObj->OutBuf->mvDataFilledSize = outputBufDesc->descs[1].
                                                    bufSize.bytes;
                pReqObj->OutBuf->mvDataOffset = outputBufDesc->descs[0].
                                                bufSize.bytes;

            }
        }
        processList.processParams[processList.numEntries].handle =
            (IVIDENC2_Handle) handle;
        processList.processParams[processList.numEntries].inArgs =
            (IVIDENC2_InArgs *) inArgs;
        processList.processParams[processList.numEntries].outArgs =
            (IVIDENC2_OutArgs *) outArgs;
        processList.processParams[processList.numEntries].inBufs =
            inputBufDesc;
        processList.processParams[processList.numEntries].outBufs =
            outputBufDesc;

        processList.numEntries++;

    }

#ifdef SYSTEM_DEBUG_MULTI_CHANNEL_ENC
   Vps_printf ("ENC : IVAHDID : %d processMulti Call with BatchSize : %d!!\n",
               tskId, processList.numEntries);
#endif

   if (handle != NULL)
   {
        error =  handle->fxns->processMulti(&processList);
   }
   else
   {
        UTILS_assert (FALSE);
   }
#ifdef SYSTEM_DEBUG_MULTI_CHANNEL_ENC
   Vps_printf ("ENC : IVAHDID : %d Returned from  processMulti Call!!\n", tskId);
#endif
    if (error != XDM_EOK)
    {
        ENCLINK_INTERNAL_ERROR_LOG(error, "ALGPROCESS FAILED");
        Vps_printf ("Number of Entries in Process List : %d \n", processList.
                    numEntries);
        for (i = 0; i < processList.numEntries; i++)
        {

          if (processList.processParams[i].outArgs->extendedError)
          {
            Vps_printf ("Extended error for entry %d : 0x%x\n", i,
                        processList.processParams[i].outArgs->extendedError);
          }
        }

    }

#ifdef ENCLINK_H264_PERFORMANCE_LOGGING

    if (enc_link_h264_chkPerfLogEnable() == TRUE)
    {
        gPerfBatchBoundaries[gPerfBatchNumber*3] = reqObjBatch->pReqObj[0]->OutBuf->channelNum;
        gPerfBatchBoundaries[gPerfBatchNumber*3 + 1] = reqObjBatch->pReqObj[reqObjBatch->numReqObjsInBatch - 1]->OutBuf->channelNum;
        gPerfBatchBoundaries[gPerfBatchNumber*3 + 2] = reqObjBatch->numReqObjsInBatch;
        gPerfBatchNumber++;
    }


#endif
    processListArrayIdx = 0;
    for (reqObjIndex = 0; reqObjIndex < reqObjBatch->numReqObjsInBatch;
         reqObjIndex++)
    {

        /*Every ReqObj corresponds to one Process List Element.*/
        /*Assumption!!! : Entries in ProcessList get returned in the same order
          as they were queued in.*/
        pReqObj = reqObjBatch->pReqObj[reqObjIndex];
        pChObj  = &pObj->chObj[pReqObj->OutBuf->channelNum];

        outArgs =
          (IH264ENC_OutArgs *) processList.processParams[processListArrayIdx].outArgs;
        inArgs =
          (IH264ENC_InArgs *) processList.processParams[processListArrayIdx].inArgs;
        inputBufDesc =
          processList.processParams[processListArrayIdx].inBufs;
        outputBufDesc =
          processList.processParams[processListArrayIdx].outBufs;

        pReqObj->OutBuf->fillLength = outArgs->videnc2OutArgs.bytesGenerated;

        if(Utils_encdecIsH264(pChObj->algObj.u.h264AlgIfObj.format) == TRUE)
            pReqObj->OutBuf->codingType = VCODEC_TYPE_H264;

        pReqObj->OutBuf->startOffset = 0;

        pReqObj->OutBuf->bottomFieldBitBufSize = outArgs->bytesGeneratedBotField;


        if (pChObj->algObj.u.h264AlgIfObj.staticParams.videnc2Params.inputContentType
            ==
        IVIDEO_PROGRESSIVE)
        {
            contentType = IVIDEO_PROGRESSIVE;
        }
        else
        {
            contentType = Utils_encdecMapFVID2FID2XDMContentType(
                         (FVID2_Fid)pReqObj->InFrameList.frames[0]->fid);
        }

        pReqObj->OutBuf->isKeyFrame = Utils_encdecIsGopStart(outArgs->
                                                             videnc2OutArgs.
                                                             encodedFrameType,
                                                             contentType);
        pReqObj->OutBuf->frameWidth =
          inputBufDesc->imageRegion.bottomRight.x - inputBufDesc->imageRegion.topLeft.x;
        pReqObj->OutBuf->frameHeight =
          inputBufDesc->imageRegion.bottomRight.y - inputBufDesc->imageRegion.topLeft.y;


        /* svcCodingParams.svcExtensionFlag Needs to be enabled to
              IH264_SVC_EXTENSION_FLAG_ENABLE for the svc headers to be present in the stream*/
        /*!!!Note: The above flag needs to be enabled for the temporalId to be parsed out 
                       from the stream.*/
        pReqObj->OutBuf->temporalId = enc_link_h264_get_svc_temporalid(
                                      pReqObj->OutBuf->addr,
                                      outArgs->videnc2OutArgs.bytesGenerated);

        pReqObj->OutBuf->numTemporalLayerSetInCodec = pChObj->algObj.u.h264AlgIfObj
                                                     .staticParams.numTemporalLayer;

        processListArrayIdx++;

#ifdef ENCLINK_H264_PERFORMANCE_LOGGING

        unsigned int *debugLog = pChObj->algObj.u.h264AlgIfObj.status.extMemoryDebugTraceAddr;
        unsigned int debugLogSize = pChObj->algObj.u.h264AlgIfObj.status.extMemoryDebugTraceSize;
        unsigned int numDebugLog = pChObj->algObj.u.h264AlgIfObj.status.lastNFramesToLog;
        unsigned int debugPerFrameLogSize = debugLogSize /(numDebugLog + 1);
        unsigned int channelNum = pReqObj->OutBuf->channelNum;

        if (gEnablePerfLog[channelNum] == TRUE)
        {


			if (gPerfNumFramesLogged[channelNum] == ENCLINK_H264_PROFILER_NUM_FRAMES)
			{
			   gEnablePerfLog[channelNum] = FALSE;

			   for (i = 0; i < ENCLINK_H264_PROFILER_NUM_FRAMES; i++)
			   {
				   /*Copy 128 bytes or 32 words. Valid here as debugLog pointer is word aligned*/
				   memcpy (gPerformanceTrace[channelNum][i], debugLog, 128);
				   debugLog += debugPerFrameLogSize;

			   }
			}
			gPerfNumFramesLogged[channelNum]++;
        }
#endif
    }


#ifdef SYSTEM_DEBUG_MULTI_CHANNEL_ENC
   Vps_printf ("IVAHDID : %d Leaving Enclink_h264EncodeFrameBatch with error code : %d\n",
               tskId, error);
#endif
    return (error);
}

static UInt32 enc_link_h264_get_svc_temporalid(Int32 *frameAddr, 
	                                                 Int32 frameBytes)
{
    UInt16 newNalType = 0;
    UInt  wordCount = 0;
    UInt32 temporalId = 0;
    Int32 svc_extension_flag = 0;

    unsigned char* stream = (unsigned char*) frameAddr;

    /* 8 is the minimum number of bytes required to get the temporalId
            from the NAL Header */
    if(frameBytes > 7)
    {
        /*Search for sync bits in the stream and move forward - 0x0001 or 0x001*/
        if ((stream[0] == 0 && stream[1] == 0 &&
                    stream[2] == 0 && stream[3] == 0x01)  ||
                (stream[0] == 0 && stream[1] == 0 &&
                 stream[2] == 0x01))
        {
            if(stream[3] == 0x01)
                wordCount += 4;
            else if(stream[2] == 0x01)
                wordCount += 3;

            /*Extract the nal_unit_type from the 1st Byte*/
            newNalType = stream[wordCount] & 0x1f;

            /*if nal_unit_type is not of this type, then return temporalId as 0*/
            if (newNalType == ENCLINK_H264_PREFIX_NAL_UNIT_TYPE1
                || newNalType == ENCLINK_H264_PREFIX_NAL_UNIT_TYPE2)
            {
                /* Move to the 2nd Byte */
                wordCount++;
                /* Extract the svc_extension_flag from the 2nd Byte */
                svc_extension_flag = stream[wordCount] & 0x80;

                /* Move to the 3rd Byte -  has the dependancyId & qualityId*/
                wordCount++;

                if(svc_extension_flag)
                {
                    /* Move to the 4th Byte */
                    wordCount++;
                    /* Extract the temporalId from the 4th Byte of the NAL header */
                    temporalId = (stream[wordCount] & 0xE0)>>5;
                }
            }
        }
    }

    return temporalId;
}

static Int enclink_h264_set_static_params(IH264ENC_Params * staticParams,
                                          EncLink_AlgCreateParams *
                                          algCreateParams)
{
    /* Initialize default values for static params */
    *staticParams = H264ENC_TI_PARAMS;

    /* Both width & height needs to be align with 2 bytes */
    staticParams->videnc2Params.maxHeight =
                  VpsUtils_align(algCreateParams->maxHeight, 2);

    staticParams->videnc2Params.maxWidth =
                  VpsUtils_align(algCreateParams->maxWidth, 16);

    staticParams->videnc2Params.maxInterFrameInterval =
        algCreateParams->maxInterFrameInterval;

    staticParams->videnc2Params.inputContentType =
        algCreateParams->inputContentType;

    staticParams->videnc2Params.inputChromaFormat =
        algCreateParams->inputChromaFormat;

    staticParams->videnc2Params.profile = algCreateParams->profile;

    staticParams->videnc2Params.level = algCreateParams->level;

    staticParams->videnc2Params.encodingPreset = algCreateParams->encodingPreset;

    if ((staticParams->videnc2Params.encodingPreset == XDM_USER_DEFINED) &&
        (algCreateParams->enableHighSpeed == TRUE))
    {
        staticParams->interCodingParams.interCodingPreset =
            IH264_INTERCODING_HIGH_SPEED;
        staticParams->intraCodingParams.intraCodingPreset =
            IH264_INTRACODING_HIGH_SPEED;
        staticParams->transformBlockSize = IH264_TRANSFORM_8x8;
    }
    staticParams->enableAnalyticinfo = algCreateParams->enableAnalyticinfo;
    staticParams->enableWatermark = algCreateParams->enableWaterMarking;
    staticParams->videnc2Params.rateControlPreset = IVIDEO_USER_DEFINED;
    staticParams->rateControlParams.rateControlParamsPreset = IH264_RATECONTROLPARAMS_USERDEFINED;
    staticParams->rateControlParams.rcAlgo = algCreateParams->rateControlPreset;
    staticParams->rateControlParams.enablePartialFrameSkip = TRUE;
    staticParams->videnc2Params.maxBitRate = algCreateParams->maxBitRate;

    staticParams->videnc2Params.inputDataMode = IVIDEO_ENTIREFRAME;
    staticParams->videnc2Params.outputDataMode = IVIDEO_ENTIREFRAME;
    

    /* Number of temporal layeers set to 1. This is the default value  */
    /* in the codec*/
    if (0 == algCreateParams->numTemporalLayer)
    {
        staticParams->numTemporalLayer = IH264_TEMPORAL_LAYERS_1;
    }
    else
    {
        staticParams->numTemporalLayer = algCreateParams->numTemporalLayer;
    }

    /*Note: Enabling this flag adds svc enxtension header to the stream, not all decoders 
          are generally able to play back such a stream. */
    /* Needs to be enabled to IH264_SVC_EXTENSION_FLAG_ENABLE for the 
          svc extension headers to be present in the stream*/
    /*!!! Note: This flag needs to be enabled for the temporalId to be parsed 
         out from the stream.*/
    staticParams->svcCodingParams.svcExtensionFlag =
        algCreateParams->enableSVCExtensionFlag;

    /*Slice Coding Parameters*/
    staticParams->sliceCodingParams.sliceCodingPreset = IH264_SLICECODING_DEFAULT;
    staticParams->sliceCodingParams.sliceMode = IH264_SLICEMODE_NONE;
    staticParams->sliceCodingParams.streamFormat = IH264_STREAM_FORMAT_DEFAULT;


    /* To set IDR frame periodically instead of I Frame */
    staticParams->IDRFrameInterval = 1;

    /*To trigger workaround inside codec, where SAME_CODEC is overridden as same
      codec type*/
    staticParams->reservedParams[1] = 0x5A3EC0DE;
     
    /* Enabling debug logging inside the codec. Details in appendix E in H.264 
     * encoder user guide.
     */
#ifdef ENCLINK_H264_PERFORMANCE_LOGGING
     staticParams->debugTraceLevel = 1;
     staticParams->lastNFramesToLog = ENCLINK_H264_PROFILER_NUM_FRAMES;
#endif
    
    /* We want SPS and PPS to be set for every intra frame. Hence configure the
     * the NALU control params to force encoder to insert SPS/PPS on every
     * I frame
     */
    staticParams->nalUnitControlParams.naluControlPreset =
                                             IH264_NALU_CONTROL_USERDEFINED;
    ENCLINK_H264_SETNALU_MASK_SPS(staticParams->nalUnitControlParams.
                                                naluPresentMaskIntraPicture);
    ENCLINK_H264_SETNALU_MASK_PPS(staticParams->nalUnitControlParams.
                                                naluPresentMaskIntraPicture);

    ENCLINK_H264_SETNALU_MASK_SPS(staticParams->nalUnitControlParams.
                                                naluPresentMaskIDRPicture);
    ENCLINK_H264_SETNALU_MASK_PPS(staticParams->nalUnitControlParams.
                                                naluPresentMaskIDRPicture);
    ENCLINK_H264_SETNALU_MASK_SEI(staticParams->nalUnitControlParams.
                                                naluPresentMaskIDRPicture);

    ENCLINK_H264_SETNALU_MASK_SPS(staticParams->nalUnitControlParams.
                                                naluPresentMaskStartOfSequence);
    ENCLINK_H264_SETNALU_MASK_PPS(staticParams->nalUnitControlParams.
                                                naluPresentMaskStartOfSequence);
    ENCLINK_H264_SETNALU_MASK_PPS(staticParams->nalUnitControlParams.
                                                naluPresentMaskStartOfSequence);

    staticParams->entropyCodingMode = IH264_ENTROPYCODING_CABAC;
    if (algCreateParams->profile != IH264_HIGH_PROFILE)
    {
        memset (&staticParams->intraCodingParams, 0, 
                               sizeof(IH264ENC_IntraCodingParams));
        staticParams->transformBlockSize = IH264_TRANSFORM_4x4;
        if (algCreateParams->profile == IH264_BASELINE_PROFILE)
        {
            staticParams->entropyCodingMode = IH264_ENTROPYCODING_CAVLC;
        }
    }

#if 1
    staticParams->vuiCodingParams.vuiCodingPreset = IH264_VUICODING_USERDEFINED;
    staticParams->vuiCodingParams.hrdParamsPresentFlag = 1;
    staticParams->vuiCodingParams.timingInfoPresentFlag = 1;
#endif

    return 0;
}

static Int enclink_h264_set_algObject(EncLink_H264Obj * algObj,
                                      EncLink_AlgCreateParams * algCreateParams,
                                      EncLink_AlgDynamicParams *
                                      algDynamicParams)
{
    IH264ENC_InArgs *inArgs;
    IH264ENC_OutArgs *outArgs;
    IVIDEO2_BufDesc *inputBufDesc;
    XDM2_BufDesc *outputBufDesc;
    IH264ENC_Status *status;
    Int i;

    inArgs = &algObj->inArgs;
    outArgs = &algObj->outArgs;
    inputBufDesc = &algObj->inBufs;
    outputBufDesc = &algObj->outBufs;
    status = &algObj->status;

     /*-----------------------------------------------------------------------*/
    /* Initialize the input ID in input arguments to the bufferid of the */
    /* buffer element returned from getfreebuffer() function.  */
     /*-----------------------------------------------------------------------*/
    /* inputID need to update before every encode process call */
    inArgs->videnc2InArgs.inputID = 0;
    inArgs->videnc2InArgs.control = IVIDENC2_CTRL_DEFAULT;

    outArgs->control = IH264ENC_CTRL_WRITE_NOREFUPDATE;
    outArgs->numStaticMBs = 0;
    outArgs->vbvBufferLevel = 0;
    outArgs->bytesGeneratedBotField = 1;
    outArgs->videnc2OutArgs.extendedError = 0;
    outArgs->videnc2OutArgs.bytesGenerated = 0;
    outArgs->videnc2OutArgs.encodedFrameType = IVIDEO_I_FRAME;
    outArgs->videnc2OutArgs.inputFrameSkip = 0;
    memset(&outArgs->videnc2OutArgs.freeBufID, 0,
           sizeof(outArgs->videnc2OutArgs.freeBufID));
    outArgs->videnc2OutArgs.reconBufs.planeDesc[0].buf = NULL;
    outArgs->videnc2OutArgs.reconBufs.planeDesc[1].buf = NULL;
    outArgs->videnc2OutArgs.reconBufs.imagePitch[0] = 0;

    /*------------------------------------------------------------------------*/
    /* Initialise output discriptors */
    /*------------------------------------------------------------------------*/
    outputBufDesc->numBufs = 0;
    for (i = 0; i < algObj->status.videnc2Status.bufInfo.minNumOutBufs; i++)
    {

        outputBufDesc->numBufs++;
        outputBufDesc->descs[i].memType = XDM_MEMTYPE_RAW;
        outputBufDesc->descs[i].bufSize.bytes =
            algObj->status.videnc2Status.bufInfo.minOutBufSize[i].bytes;

        if (i == 0)
        {
        /*-------------------------------------------------------------------*/
            /* Set proper buffer addresses for bitstream data */
        /*-------------------------------------------------------------------*/
            outputBufDesc->descs[0].buf = NULL;
        }
        else
        {

            /* Check for required size vs Memory allocated for Analytic info buffer.*/
            UTILS_assert((status->videnc2Status.bufInfo.minOutBufSize[i].bytes <
                        algCreateParams->mvDataSize));

        /*-------------------------------------------------------------------*/
            /* Set proper buffer addresses for MV & SAD data */
        /*-------------------------------------------------------------------*/
            outputBufDesc->descs[i].buf = NULL;
        }
    }

    /*------------------------------------------------------------------------*/
    /* Video buffer layout, field interleaved or field separated */
    /* Only IVIDEO_FIELD_INTERLEAVED and IVIDEO_FIELD_SEPARATED are supported
     */
    /*------------------------------------------------------------------------*/
    inputBufDesc->dataLayout = algCreateParams->dataLayout;

    /*------------------------------------------------------------------------*/
    /* Flag to indicate field order in interlaced content */
    /* Supported values are */
    /* 0 - Bottom field first */
    /* 1 - Top filed first */
    /* TODO : need to find defalut parameter */
    /*------------------------------------------------------------------------*/
    inputBufDesc->topFieldFirstFlag = 1;

    /*------------------------------------------------------------------------*/
    /* Initialize the input buffer properties as required by algorithm */
    /* based on info received by preceding GETBUFINFO call.  */
    /*------------------------------------------------------------------------*/
    inputBufDesc->numPlanes = 2;/* status.videnc2Status.bufInfo.minNumInBufs; */
    inputBufDesc->numMetaPlanes = 0;
    /*------------------------------------------------------------------------*/
    /* Set entire Image region in the buffer by using config parameters */
    /*------------------------------------------------------------------------*/
    inputBufDesc->imageRegion.topLeft.x = algDynamicParams->startX;
    inputBufDesc->imageRegion.topLeft.y = algDynamicParams->startY;
    inputBufDesc->imageRegion.bottomRight.x = algDynamicParams->startX +
        algObj->dynamicParams.videnc2DynamicParams.inputWidth;
    inputBufDesc->imageRegion.bottomRight.y = algDynamicParams->startY +
        algObj->dynamicParams.videnc2DynamicParams.inputHeight;
    /*------------------------------------------------------------------------*/
    /* Set proper Image region in the buffer by using config parameters */
    /*------------------------------------------------------------------------*/
    inputBufDesc->activeFrameRegion.topLeft.x = algDynamicParams->startX;
    inputBufDesc->activeFrameRegion.topLeft.y = algDynamicParams->startY;
    inputBufDesc->activeFrameRegion.bottomRight.x = algDynamicParams->startX +
        algObj->dynamicParams.videnc2DynamicParams.inputWidth;
    inputBufDesc->activeFrameRegion.bottomRight.y = algDynamicParams->startY +
        algObj->dynamicParams.videnc2DynamicParams.inputHeight;
    /*------------------------------------------------------------------------*/
    /* Image pitch is capture width */
    /*------------------------------------------------------------------------*/
    if (algCreateParams->tilerEnable)
    {
        inputBufDesc->imagePitch[0] = VPSUTILS_TILER_CNT_8BIT_PITCH;
    }
    else
    {
        inputBufDesc->imagePitch[0] =
            algObj->dynamicParams.videnc2DynamicParams.captureWidth;
    }
    if (algCreateParams->tilerEnable)
    {
        inputBufDesc->imagePitch[1] = VPSUTILS_TILER_CNT_16BIT_PITCH;
    }
    else
    {
        inputBufDesc->imagePitch[1] =
            algObj->dynamicParams.videnc2DynamicParams.captureWidth;
    }
    /*------------------------------------------------------------------------*/
    /* Set Content type and chroma format from encoder parameters */
    /*------------------------------------------------------------------------*/
    inputBufDesc->contentType =
        algObj->staticParams.videnc2Params.inputContentType;
    inputBufDesc->chromaFormat =
        algObj->staticParams.videnc2Params.inputChromaFormat;

    /*------------------------------------------------------------------------*/
    /* Assign memory pointers adn sizes for the all the input buffers */
    /*------------------------------------------------------------------------*/
    for (i = 0; i < algObj->status.videnc2Status.bufInfo.minNumInBufs; i++)
    {
        inputBufDesc->planeDesc[i].buf = NULL;
        if (algCreateParams->tilerEnable)
        {
            if (i & 0x1)
            {
                inputBufDesc->planeDesc[i].memType = XDM_MEMTYPE_TILED16;
            }
            else
            {
                inputBufDesc->planeDesc[i].memType = XDM_MEMTYPE_TILED8;
            }
            inputBufDesc->planeDesc[i].bufSize.tileMem.width =
                algObj->status.videnc2Status.bufInfo.minInBufSize[i].tileMem.
                width;
            inputBufDesc->planeDesc[i].bufSize.tileMem.height =
                algObj->status.videnc2Status.bufInfo.minInBufSize[i].tileMem.
                height;
        }
        else
        {
            inputBufDesc->planeDesc[i].memType = XDM_MEMTYPE_RAW;
            inputBufDesc->planeDesc[i].bufSize.bytes =
                algObj->status.videnc2Status.bufInfo.minInBufSize[i].tileMem.
                width *
                algObj->status.videnc2Status.bufInfo.minInBufSize[i].tileMem.
                height;

        }
    }
    /*------------------------------------------------------------------------*/
    /* Set second field offset width and height according to input data */
    /* When second field of the input data starts at 0 it represents 2 fields
     */
    /* are seperated and provided at 2 diff process calls (60 process call) */
    /*------------------------------------------------------------------------*/
    if ((inputBufDesc->dataLayout == IVIDEO_FIELD_SEPARATED) &&
        (algCreateParams->singleBuf == FALSE) &&
        (algObj->staticParams.videnc2Params.inputContentType ==
         IVIDEO_INTERLACED))
    {
        inputBufDesc->secondFieldOffsetHeight[0] = 0;
        inputBufDesc->secondFieldOffsetHeight[1] = 0;
        inputBufDesc->secondFieldOffsetHeight[2] = 0;
    }
    else
    {
        inputBufDesc->secondFieldOffsetHeight[0] =
            algObj->dynamicParams.videnc2DynamicParams.inputHeight;
        inputBufDesc->secondFieldOffsetHeight[1] =
            (algObj->dynamicParams.videnc2DynamicParams.inputHeight >> 1);
        inputBufDesc->secondFieldOffsetHeight[2] =
            (algObj->dynamicParams.videnc2DynamicParams.inputHeight >> 1);
    }
    inputBufDesc->secondFieldOffsetWidth[0] = 0;
    inputBufDesc->secondFieldOffsetWidth[1] = 0;
    inputBufDesc->secondFieldOffsetWidth[2] = 0;

    /*------------------------------------------------------------------------*/
    /* Set The address of unregistered user data in meta data plane desc */
    /*------------------------------------------------------------------------*/
    inputBufDesc->numMetaPlanes = 0;
    /*------------------------------------------------------------------------*/
    /* Provide approprate buffer addresses for both the supported meta data: */
    /* A. USer defined SEI message */
    /* B. User Defined Scaling MAtrices */
    /*------------------------------------------------------------------------*/
    if (algObj->staticParams.videnc2Params.
        metadataType[inputBufDesc->numMetaPlanes] ==
        IH264_SEI_USER_DATA_UNREGISTERED)
    {
        inputBufDesc->metadataPlaneDesc[inputBufDesc->numMetaPlanes].buf = NULL;
        inputBufDesc->metadataPlaneDesc[inputBufDesc->numMetaPlanes].bufSize.
            bytes = -1;
        inputBufDesc->numMetaPlanes++;
    }
    /*------------------------------------------------------------------------*/
    /* Set proper buffer addresses for user defined scaling matrix */
    /*------------------------------------------------------------------------*/
    if (algObj->staticParams.videnc2Params.
        metadataType[inputBufDesc->numMetaPlanes] ==
        IH264_USER_DEFINED_SCALINGMATRIX)
    {
        inputBufDesc->metadataPlaneDesc[inputBufDesc->numMetaPlanes].buf = NULL;
        inputBufDesc->metadataPlaneDesc[inputBufDesc->numMetaPlanes].bufSize.
            bytes =
            /* -1; */
            896;
        inputBufDesc->numMetaPlanes++;
    }

    return 0;
}
static XDAS_Int32 enclink_h264_dummy_get_buffer_fxn(XDM_DataSyncHandle dataSyncHandle,
                                                    XDM_DataSyncDesc *dataSyncDesc)
{
    Vps_printf("%d:ENCLINK:H264Enc !!WARNING!!!Unable to handle runtime output buffer request");
    return -1;
}


static Int enclink_h264_set_dynamic_params(IH264ENC_DynamicParams *
                                           dynamicParams,
                                           EncLink_AlgDynamicParams *
                                           algDynamicParams)
{
    *dynamicParams = H264ENC_TI_DYNAMICPARAMS;

    dynamicParams->videnc2DynamicParams.inputWidth =
        algDynamicParams->inputWidth;
    dynamicParams->videnc2DynamicParams.inputHeight =
        algDynamicParams->inputHeight;
    dynamicParams->videnc2DynamicParams.captureWidth =
        algDynamicParams->inputPitch;
    dynamicParams->videnc2DynamicParams.targetBitRate =
        algDynamicParams->targetBitRate;
    dynamicParams->videnc2DynamicParams.targetFrameRate =
        algDynamicParams->targetFrameRate;
    dynamicParams->videnc2DynamicParams.interFrameInterval =
        algDynamicParams->interFrameInterval;
    dynamicParams->videnc2DynamicParams.intraFrameInterval =
        algDynamicParams->intraFrameInterval;
    dynamicParams->videnc2DynamicParams.mvAccuracy =
        algDynamicParams->mvAccuracy;
    dynamicParams->rateControlParams.VBRDuration =
        algDynamicParams->vbrDuration;
    dynamicParams->rateControlParams.VBRsensitivity =
        algDynamicParams->vbrSensitivity;
    dynamicParams->videnc2DynamicParams.refFrameRate =
        algDynamicParams->refFrameRate;
    dynamicParams->videnc2DynamicParams.ignoreOutbufSizeFlag = XDAS_TRUE;
    dynamicParams->videnc2DynamicParams.getBufferFxn =
                                       enclink_h264_dummy_get_buffer_fxn;

    dynamicParams->rateControlParams.rateControlParamsPreset
        = IH264_RATECONTROLPARAMS_USERDEFINED;
    dynamicParams->rateControlParams.qpMinI = algDynamicParams->qpMinI;
    dynamicParams->rateControlParams.qpMaxI = algDynamicParams->qpMaxI;
    dynamicParams->rateControlParams.qpI    = algDynamicParams->qpInitI;
    dynamicParams->rateControlParams.qpMinP = algDynamicParams->qpMinP;
    dynamicParams->rateControlParams.qpMaxP = algDynamicParams->qpMaxP;
    dynamicParams->rateControlParams.qpP    = algDynamicParams->qpInitP;
    dynamicParams->rateControlParams.rcAlgo = algDynamicParams->rcAlg;

    dynamicParams->rateControlParams.discardSavedBits = 1;

    if(dynamicParams->rateControlParams.rcAlgo == IH264_RATECONTROL_PRC)
    {
        dynamicParams->rateControlParams.HRDBufferSize
            = 2 * algDynamicParams->targetBitRate;
    }
    else
    {
        dynamicParams->rateControlParams.HRDBufferSize
            = algDynamicParams->targetBitRate;
    }

    dynamicParams->rateControlParams.initialBufferLevel
        = dynamicParams->rateControlParams.HRDBufferSize;

    dynamicParams->rateControlParams.frameSkipThMulQ5 = 0;
    dynamicParams->rateControlParams.vbvUseLevelThQ5 = 0;

    dynamicParams->rateControlParams.maxPicSizeRatioI = 960;
    dynamicParams->rateControlParams.skipDistributionWindowLength = 5;
    dynamicParams->rateControlParams.numSkipInDistributionWindow = 2;

    return 0;
}

#define ENCLINKH264_ALGREATE_RSRC_NONE                                       (0)
#define ENCLINKH264_ALGREATE_RSRC_ALGCREATED                           (1 <<  0)
#define ENCLINKH264_ALGREATE_RSRC_IRES_ASSIGNED                        (1 <<  1)
#define ENCLINKH264_ALGREATE_RSRC_ALL (                                        \
                                       ENCLINKH264_ALGREATE_RSRC_ALGCREATED |  \
                                       ENCLINKH264_ALGREATE_RSRC_IRES_ASSIGNED \
                                      )

static Void enclink_h264_freersrc(EncLink_H264Obj * hObj, Int rsrcMask)
{
    if (rsrcMask & ENCLINKH264_ALGREATE_RSRC_IRES_ASSIGNED)
    {
        IRES_Status iresStatus;

        IRESMAN_TiledMemoryForceDisableTileAlloc_UnRegister((IALG_Handle) hObj->algHandle);
        iresStatus =
            RMAN_freeResources((IALG_Handle) hObj->algHandle,
                               &H264ENC_TI_IRES, hObj->scratchID);
        if (iresStatus != IRES_OK)
        {
            ENCLINK_INTERNAL_ERROR_LOG(iresStatus, "RMAN_freeResources");
        }
    }
    if (rsrcMask & ENCLINKH264_ALGREATE_RSRC_ALGCREATED)
    {
        enc_link_h264_delete(hObj->algHandle);
        hObj->algHandle = NULL;
    }
}

static Int enclink_print_dynamic_params(IVIDENC2_DynamicParams *videnc2DynamicParams)
{
    Vps_printf("videnc2DynamicParams -> inputHeight             : %d\n", videnc2DynamicParams->inputHeight);
    Vps_printf("videnc2DynamicParams -> inputWidth              : %d\n", videnc2DynamicParams->inputWidth);
    Vps_printf("videnc2DynamicParams -> refFrameRate            : %d\n", videnc2DynamicParams->refFrameRate);
    Vps_printf("videnc2DynamicParams -> targetFrameRate         : %d\n", videnc2DynamicParams->targetFrameRate);
    Vps_printf("videnc2DynamicParams -> targetBitRate           : %d\n", videnc2DynamicParams->targetBitRate);
    Vps_printf("videnc2DynamicParams -> intraFrameInterval      : %d\n", videnc2DynamicParams->intraFrameInterval);
    Vps_printf("videnc2DynamicParams -> generateHeader          : %d\n", videnc2DynamicParams->generateHeader);
    Vps_printf("videnc2DynamicParams -> captureWidth            : %d\n", videnc2DynamicParams->captureWidth);
    Vps_printf("videnc2DynamicParams -> forceFrame              : %d\n", videnc2DynamicParams->forceFrame);
    Vps_printf("videnc2DynamicParams -> interFrameInterval      : %d\n", videnc2DynamicParams->interFrameInterval);
    Vps_printf("videnc2DynamicParams -> mvAccuracy              : %d\n", videnc2DynamicParams->mvAccuracy);
    Vps_printf("videnc2DynamicParams -> sampleAspectRatioHeight : %d\n", videnc2DynamicParams->sampleAspectRatioHeight);
    Vps_printf("videnc2DynamicParams -> sampleAspectRatioWidth  : %d\n", videnc2DynamicParams->sampleAspectRatioWidth);
    Vps_printf("videnc2DynamicParams -> ignoreOutbufSizeFlag    : %d\n", videnc2DynamicParams->ignoreOutbufSizeFlag);
    Vps_printf("videnc2DynamicParams -> lateAcquireArg          : %d\n", videnc2DynamicParams->lateAcquireArg);

    return 0;
}



static Int enclink_h264_print_dynamic_params(UInt32 chId, IH264ENC_DynamicParams *
                                           dynamicParams)
{
    Vps_printf(" \n");
    Vps_printf("--------- CH %d : H264 ENC : Dynamic Params -------\n", chId);
    Vps_printf(" \n");
    enclink_print_dynamic_params(&dynamicParams->videnc2DynamicParams);
    Vps_printf(" \n");
    Vps_printf("rateControlParams -> rateControlParamsPreset        : %d\n", dynamicParams->rateControlParams.rateControlParamsPreset);
    Vps_printf("rateControlParams -> scalingMatrixPreset            : %d\n", dynamicParams->rateControlParams.scalingMatrixPreset);
    Vps_printf("rateControlParams -> rcAlgo                         : %d\n", dynamicParams->rateControlParams.rcAlgo);
    Vps_printf("rateControlParams -> qpI                            : %d\n", dynamicParams->rateControlParams.qpI);
    Vps_printf("rateControlParams -> qpMaxI                         : %d\n", dynamicParams->rateControlParams.qpMaxI);
    Vps_printf("rateControlParams -> qpMinI                         : %d\n", dynamicParams->rateControlParams.qpMinI);
    Vps_printf("rateControlParams -> qpP                            : %d\n", dynamicParams->rateControlParams.qpP);
    Vps_printf("rateControlParams -> qpMaxP                         : %d\n", dynamicParams->rateControlParams.qpMaxP);
    Vps_printf("rateControlParams -> qpMinP                         : %d\n", dynamicParams->rateControlParams.qpMinP);
    Vps_printf("rateControlParams -> qpOffsetB                      : %d\n", dynamicParams->rateControlParams.qpOffsetB);
    Vps_printf("rateControlParams -> qpMaxB                         : %d\n", dynamicParams->rateControlParams.qpMaxB);
    Vps_printf("rateControlParams -> qpMinB                         : %d\n", dynamicParams->rateControlParams.qpMinB);
    Vps_printf("rateControlParams -> allowFrameSkip                 : %d\n", dynamicParams->rateControlParams.allowFrameSkip);
    Vps_printf("rateControlParams -> removeExpensiveCoeff           : %d\n", dynamicParams->rateControlParams.removeExpensiveCoeff);
    Vps_printf("rateControlParams -> chromaQPIndexOffset            : %d\n", dynamicParams->rateControlParams.chromaQPIndexOffset);
    Vps_printf("rateControlParams -> IPQualityFactor                : %d\n", dynamicParams->rateControlParams.IPQualityFactor);
    Vps_printf("rateControlParams -> initialBufferLevel             : %d\n", dynamicParams->rateControlParams.initialBufferLevel);
    Vps_printf("rateControlParams -> HRDBufferSize                  : %d\n", dynamicParams->rateControlParams.HRDBufferSize);
    Vps_printf("rateControlParams -> minPicSizeRatioI               : %d\n", dynamicParams->rateControlParams.minPicSizeRatioI);
    Vps_printf("rateControlParams -> maxPicSizeRatioI               : %d\n", dynamicParams->rateControlParams.maxPicSizeRatioI);
    Vps_printf("rateControlParams -> minPicSizeRatioP               : %d\n", dynamicParams->rateControlParams.minPicSizeRatioP);
    Vps_printf("rateControlParams -> maxPicSizeRatioP               : %d\n", dynamicParams->rateControlParams.maxPicSizeRatioP);
    Vps_printf("rateControlParams -> minPicSizeRatioB               : %d\n", dynamicParams->rateControlParams.minPicSizeRatioB);
    Vps_printf("rateControlParams -> maxPicSizeRatioB               : %d\n", dynamicParams->rateControlParams.maxPicSizeRatioB);
    Vps_printf("rateControlParams -> enablePRC                      : %d\n", dynamicParams->rateControlParams.enablePRC);
    Vps_printf("rateControlParams -> enablePartialFrameSkip         : %d\n", dynamicParams->rateControlParams.enablePartialFrameSkip);
    Vps_printf("rateControlParams -> discardSavedBits               : %d\n", dynamicParams->rateControlParams.discardSavedBits);
    Vps_printf("rateControlParams -> VBRDuration                    : %d\n", dynamicParams->rateControlParams.VBRDuration);
    Vps_printf("rateControlParams -> VBRsensitivity                 : %d\n", dynamicParams->rateControlParams.VBRsensitivity);
    Vps_printf("rateControlParams -> skipDistributionWindowLength   : %d\n", dynamicParams->rateControlParams.skipDistributionWindowLength);
    Vps_printf("rateControlParams -> numSkipInDistributionWindow    : %d\n", dynamicParams->rateControlParams.numSkipInDistributionWindow);
    Vps_printf("rateControlParams -> enableHRDComplianceMode        : %d\n", dynamicParams->rateControlParams.enableHRDComplianceMode);
    Vps_printf("rateControlParams -> frameSkipThMulQ5               : %d\n", dynamicParams->rateControlParams.frameSkipThMulQ5);
    Vps_printf("rateControlParams -> vbvUseLevelThQ5                : %d\n", dynamicParams->rateControlParams.vbvUseLevelThQ5);
    Vps_printf(" \n");
    Vps_printf("interCodingParams -> interCodingPreset  : %d\n", dynamicParams->interCodingParams.interCodingPreset);
    Vps_printf("interCodingParams -> searchRangeHorP    : %d\n", dynamicParams->interCodingParams.searchRangeHorP);
    Vps_printf("interCodingParams -> searchRangeVerP    : %d\n", dynamicParams->interCodingParams.searchRangeVerP);
    Vps_printf("interCodingParams -> searchRangeHorB    : %d\n", dynamicParams->interCodingParams.searchRangeHorB);
    Vps_printf("interCodingParams -> searchRangeVerB    : %d\n", dynamicParams->interCodingParams.searchRangeVerB);
    Vps_printf("interCodingParams -> interCodingBias    : %d\n", dynamicParams->interCodingParams.interCodingBias);
    Vps_printf("interCodingParams -> skipMVCodingBias   : %d\n", dynamicParams->interCodingParams.skipMVCodingBias);
    Vps_printf("interCodingParams -> minBlockSizeP      : %d\n", dynamicParams->interCodingParams.minBlockSizeP);
    Vps_printf("interCodingParams -> minBlockSizeB      : %d\n", dynamicParams->interCodingParams.minBlockSizeB);
    Vps_printf("interCodingParams -> meAlgoMode         : %d\n", dynamicParams->interCodingParams.meAlgoMode);
    Vps_printf(" \n");
    Vps_printf("intraCodingParams -> intraCodingPreset          : %d\n", dynamicParams->intraCodingParams.intraCodingPreset);
    Vps_printf("intraCodingParams -> lumaIntra4x4Enable         : %d\n", dynamicParams->intraCodingParams.lumaIntra4x4Enable);
    Vps_printf("intraCodingParams -> lumaIntra8x8Enable         : %d\n", dynamicParams->intraCodingParams.lumaIntra8x8Enable);
    Vps_printf("intraCodingParams -> lumaIntra16x16Enable       : %d\n", dynamicParams->intraCodingParams.lumaIntra16x16Enable);
    Vps_printf("intraCodingParams -> chromaIntra8x8Enable       : %d\n", dynamicParams->intraCodingParams.chromaIntra8x8Enable);
    Vps_printf("intraCodingParams -> chromaComponentEnable      : %d\n", dynamicParams->intraCodingParams.chromaComponentEnable);
    Vps_printf("intraCodingParams -> intraRefreshMethod         : %d\n", dynamicParams->intraCodingParams.intraRefreshMethod);
    Vps_printf("intraCodingParams -> intraRefreshRate           : %d\n", dynamicParams->intraCodingParams.intraRefreshRate);
    Vps_printf("intraCodingParams -> gdrOverlapRowsBtwFrames    : %d\n", dynamicParams->intraCodingParams.gdrOverlapRowsBtwFrames);
    Vps_printf("intraCodingParams -> constrainedIntraPredEnable : %d\n", dynamicParams->intraCodingParams.constrainedIntraPredEnable);
    Vps_printf("intraCodingParams -> intraCodingBias            : %d\n", dynamicParams->intraCodingParams.intraCodingBias);
    Vps_printf(" \n");
    Vps_printf("sliceCodingParams -> sliceCodingPreset  : %d\n", dynamicParams->sliceCodingParams.sliceCodingPreset);
    Vps_printf("sliceCodingParams -> sliceMode          : %d\n", dynamicParams->sliceCodingParams.sliceMode);
    Vps_printf("sliceCodingParams -> sliceUnitSize      : %d\n", dynamicParams->sliceCodingParams.sliceUnitSize);
    Vps_printf("sliceCodingParams -> sliceStartOffset   : [%d %d %d]\n",
            dynamicParams->sliceCodingParams.sliceStartOffset[0],
            dynamicParams->sliceCodingParams.sliceStartOffset[1],
            dynamicParams->sliceCodingParams.sliceStartOffset[2]
        );
    Vps_printf("sliceCodingParams -> streamFormat       : %d\n", dynamicParams->sliceCodingParams.streamFormat);
    Vps_printf(" \n");
    Vps_printf("sliceGroupChangeCycle           : %d\n", dynamicParams->sliceGroupChangeCycle);
    Vps_printf("searchCenter                    : %d\n", dynamicParams->searchCenter);
    Vps_printf("enableStaticMBCount             : %d\n", dynamicParams->enableStaticMBCount);
    Vps_printf("intraRefreshRateGDRDynamic      : %d\n", dynamicParams->intraRefreshRateGDRDynamic);
    Vps_printf("gdrOverlapRowsBtwFramesDynamic  : %d\n", dynamicParams->gdrOverlapRowsBtwFramesDynamic);
    Vps_printf("enableROI                       : %d\n", dynamicParams->enableROI);
    Vps_printf(" \n");
    Vps_printf(" \n");

    return 0;
}


Int EncLinkH264_algCreate(EncLink_H264Obj * hObj,
                          EncLink_AlgCreateParams * algCreateParams,
                          EncLink_AlgDynamicParams * algDynamicParams,
                          Int linkID, Int channelID, Int scratchGroupID)
{
    Int retVal = ENC_LINK_S_SUCCESS;
    Int rsrcMask = ENCLINKH264_ALGREATE_RSRC_NONE;
    Int algStatus;

    UTILS_assert(Utils_encdecIsH264(algCreateParams->format) == TRUE);
    hObj->format = algCreateParams->format;
    hObj->linkID = linkID;
    hObj->channelID = channelID;
    hObj->scratchID = scratchGroupID;

    memset(&hObj->inArgs, 0, sizeof(hObj->inArgs));
    memset(&hObj->outArgs, 0, sizeof(hObj->outArgs));
    memset(&hObj->inBufs, 0, sizeof(hObj->inBufs));
    memset(&hObj->outBufs, 0, sizeof(hObj->outBufs));
    memset(&hObj->status, 0, sizeof(hObj->status));
    memset(&hObj->memUsed, 0, sizeof(hObj->memUsed));

    hObj->status.videnc2Status.size = sizeof(IH264ENC_Status);
    hObj->inArgs.videnc2InArgs.size = sizeof(IH264ENC_InArgs);
    hObj->outArgs.videnc2OutArgs.size = sizeof(IH264ENC_OutArgs);
    hObj->staticParams.videnc2Params.size = sizeof(IH264ENC_Params);
    hObj->dynamicParams.videnc2DynamicParams.size =
        sizeof(IH264ENC_DynamicParams);

    enclink_h264_set_static_params(&hObj->staticParams, algCreateParams);
    enclink_h264_set_dynamic_params(&hObj->dynamicParams, algDynamicParams);

    if(hObj->staticParams.rateControlParams.rcAlgo == IH264_RATECONTROL_PRC)
    {
        /* [IH264_RATECONTROL_PRC] Variable Bitrate*/
        hObj->staticParams.rateControlParams.HRDBufferSize
            = 2 * hObj->dynamicParams.videnc2DynamicParams.targetBitRate;
        hObj->staticParams.rateControlParams.initialBufferLevel
            =     hObj->staticParams.rateControlParams.HRDBufferSize;
    }
    else if(hObj->staticParams.rateControlParams.rcAlgo == IH264_RATECONTROL_PRC_LOW_DELAY)
    {
        hObj->staticParams.rateControlParams.HRDBufferSize
            = hObj->dynamicParams.videnc2DynamicParams.targetBitRate;
        hObj->staticParams.rateControlParams.initialBufferLevel
            = hObj->staticParams.rateControlParams.HRDBufferSize;
    }

    UTILS_MEMLOG_USED_START();
    hObj->algHandle =
        enc_link_h264_create((IH264ENC_Fxns *) & H264ENC_TI_IH264ENC,
                             &hObj->staticParams);
    UTILS_assertError((NULL != hObj->algHandle),
                      retVal, ENC_LINK_E_ALGCREATEFAILED, linkID, channelID);
    if (!UTILS_ISERROR(retVal))
    {
        IRES_Status iresStatus;

        rsrcMask |= ENCLINKH264_ALGREATE_RSRC_ALGCREATED;
        if (FALSE == algCreateParams->tilerEnable)
        {
            IRESMAN_TiledMemoryForceDisableTileAlloc_Register((IALG_Handle) hObj->algHandle);
        }
        iresStatus = RMAN_assignResources((IALG_Handle) hObj->algHandle,
                                          &H264ENC_TI_IRES, scratchGroupID);
        UTILS_assertError((iresStatus == IRES_OK), retVal,
                          ENC_LINK_E_RMANRSRCASSIGNFAILED, linkID, channelID);
    }
    if (!UTILS_ISERROR(retVal))
    {

        rsrcMask |= ENCLINKH264_ALGREATE_RSRC_IRES_ASSIGNED;

        hObj->status.videnc2Status.data.buf = &(hObj->versionInfo[0]);
        hObj->status.videnc2Status.data.bufSize = sizeof(hObj->versionInfo);
        algStatus = enclink_h264_control(hObj->algHandle, XDM_GETVERSION,
                                         &(hObj->dynamicParams),
                                         &(hObj->status));
        if (algStatus == XDM_EOK)
        {
            ENCLINK_VERBOSE_INFO_LOG(hObj->linkID, hObj->channelID,
                                     "H264EncCreated:%s", hObj->versionInfo);

        }
        algStatus = enclink_h264_control(hObj->algHandle,
                                         XDM_SETDEFAULT,
                                         &hObj->dynamicParams, &hObj->status);
        UTILS_assertError((algStatus == XDM_EOK), retVal,
                          ENC_LINK_E_ALGSETPARAMSFAILED, linkID, channelID);
    }
    if (!UTILS_ISERROR(retVal))
    {
        algStatus = enclink_h264_control(hObj->algHandle,
                                         XDM_SETPARAMS,
                                         &hObj->dynamicParams, &hObj->status);
        UTILS_assertError((algStatus == XDM_EOK), retVal,
                          ENC_LINK_E_ALGSETPARAMSFAILED, linkID, channelID);
    }

    if (!UTILS_ISERROR(retVal))
    {
        enclink_h264_control(hObj->algHandle,
                             XDM_GETSTATUS,
                             &hObj->dynamicParams, &hObj->status);
    }
    if (!UTILS_ISERROR(retVal))
    {
        algStatus =
            enclink_h264_control(hObj->algHandle,
                                 XDM_GETBUFINFO,
                                 &hObj->dynamicParams, &hObj->status);
        UTILS_assertError((algStatus == XDM_EOK), retVal,
                          ENC_LINK_E_ALGGETBUFINFOFAILED, linkID, channelID);
    }
    if (UTILS_ISERROR(retVal))
    {
        enclink_h264_freersrc(hObj, rsrcMask);
    }
    /* Initialize the Inarg, OutArg, InBuf & OutBuf objects */
    enclink_h264_set_algObject(hObj, algCreateParams, algDynamicParams);

    UTILS_MEMLOG_USED_END(hObj->memUsed);
    UTILS_MEMLOG_PRINT("ENCLINK_H264",
                       hObj->memUsed,
                       (sizeof(hObj->memUsed) / sizeof(hObj->memUsed[0])));

    return retVal;
}

Void EncLinkH264_algDelete(EncLink_H264Obj * hObj)
{
    UTILS_MEMLOG_FREE_START();
    if (hObj->algHandle)
    {
        enclink_h264_freersrc(hObj, ENCLINKH264_ALGREATE_RSRC_ALL);
    }

    if (hObj->algHandle)
    {
        enc_link_h264_delete(hObj->algHandle);
    }
    UTILS_MEMLOG_FREE_END(hObj->memUsed, 0 /* dont care */ );
}

Int32 EncLinkH264_algSetConfig(EncLink_algObj * algObj)
{
    Int32 status = ENC_LINK_S_SUCCESS;
    UInt32 bitMask;
    Bool setConfigFlag = FALSE;
    UInt key;

    key = Hwi_disable();
    bitMask = algObj->setConfigBitMask;

    /* Set the modified encoder bitRate value */
    if ((bitMask >>  ENC_LINK_SETCONFIG_BITMASK_BITRARATE) & 0x1)
    {

        algObj->u.h264AlgIfObj.dynamicParams.videnc2DynamicParams.
                targetBitRate = algObj->algDynamicParams.targetBitRate;
/*        Vps_printf("\n ENCLINK: new targetbitrate to set:%d \n",
                algObj->algDynamicParams.targetBitRate);*/
        algObj->setConfigBitMask &= (ENC_LINK_SETCONFIG_BITMASK_RESET_VALUE ^
                                    (1 << ENC_LINK_SETCONFIG_BITMASK_BITRARATE));
        EncLink_h264EncoderReset(&algObj->u.h264AlgIfObj);
        setConfigFlag = TRUE;
    }

    /* Set the modified encoder Fps value */
    if ((bitMask >>  ENC_LINK_SETCONFIG_BITMASK_FPS) & 0x1)
    {
        algObj->u.h264AlgIfObj.dynamicParams.videnc2DynamicParams.
                targetFrameRate = algObj->algDynamicParams.targetFrameRate;
        algObj->u.h264AlgIfObj.dynamicParams.videnc2DynamicParams.
                targetBitRate = algObj->algDynamicParams.targetBitRate;
/*        Vps_printf("\n ENCLINK: new targetbitrate to set:%d \n",
                algObj->algDynamicParams.targetBitRate);
        Vps_printf("\n ENCLINK: new targetframerate to set:%d \n",
                algObj->algDynamicParams.targetFrameRate);*/
        algObj->setConfigBitMask &= (ENC_LINK_SETCONFIG_BITMASK_RESET_VALUE ^
                                    (1 << ENC_LINK_SETCONFIG_BITMASK_FPS));
        EncLink_h264EncoderReset(&algObj->u.h264AlgIfObj);
        setConfigFlag = TRUE;
    }

    /* Set the modified encoder Intra Frame Interval(GOP) value */
    if ((bitMask >>  ENC_LINK_SETCONFIG_BITMASK_INTRAI) & 0x1)
    {
        algObj->u.h264AlgIfObj.dynamicParams.videnc2DynamicParams.
                intraFrameInterval = algObj->algDynamicParams.intraFrameInterval;
        #ifdef SYSTEM_VERBOSE_PRINTS
        Vps_printf("\n ENCLINK: new intraFrameInterval to set:%d \n",
                algObj->algDynamicParams.intraFrameInterval);
        #endif
        algObj->setConfigBitMask &= (ENC_LINK_SETCONFIG_BITMASK_RESET_VALUE ^
                                    (1 << ENC_LINK_SETCONFIG_BITMASK_INTRAI));
        setConfigFlag = TRUE;
    }

    /* toggle Force IDR */
    if ((bitMask >>  ENC_LINK_SETCONFIG_BITMASK_FORCEI) & 0x1)
    {

        algObj->algDynamicParams.forceFrame = TRUE;
        algObj->algDynamicParams.forceFrameStatus = FALSE;

        algObj->setConfigBitMask &= (ENC_LINK_SETCONFIG_BITMASK_RESET_VALUE ^
                                    (1 << ENC_LINK_SETCONFIG_BITMASK_FORCEI));
        setConfigFlag = TRUE;
    }
    /** to support Force IDR frame: Entry **/
    if ((algObj->algDynamicParams.forceFrame == TRUE) &&
        (algObj->algDynamicParams.forceFrameStatus == FALSE))
    {
        /** SET forceIDR **/
        algObj->u.h264AlgIfObj.dynamicParams.videnc2DynamicParams.forceFrame =
                IVIDEO_IDR_FRAME;
        algObj->algDynamicParams.forceFrameStatus = TRUE;
    }
    else if((algObj->algDynamicParams.forceFrame == TRUE) &&
            (algObj->algDynamicParams.forceFrameStatus == TRUE))
    {
        /** UNSET forceIDR **/
        algObj->u.h264AlgIfObj.dynamicParams.videnc2DynamicParams.forceFrame =
                IVIDEO_NA_FRAME;
        algObj->algDynamicParams.forceFrame = FALSE;

        setConfigFlag = TRUE;
    }
    /** to support Force IDR frame: Exit **/

    /* Set the modified encoder RC Alg values*/
    if ((bitMask >>  ENC_LINK_SETCONFIG_BITMASK_RCALGO) & 0x1)
    {
        algObj->u.h264AlgIfObj.dynamicParams.rateControlParams.rateControlParamsPreset
            = IH264_RATECONTROLPARAMS_USERDEFINED;
        algObj->u.h264AlgIfObj.dynamicParams.rateControlParams.rcAlgo
            = algObj->algDynamicParams.rcAlg;

        if(algObj->algDynamicParams.rcAlg == IH264_RATECONTROL_PRC)
        {
            algObj->u.h264AlgIfObj.dynamicParams.rateControlParams.HRDBufferSize
                = 2 * algObj->algDynamicParams.targetBitRate;
            algObj->u.h264AlgIfObj.dynamicParams.rateControlParams.initialBufferLevel
                = algObj->u.h264AlgIfObj.dynamicParams.rateControlParams.HRDBufferSize;
        }
        else if(algObj->algDynamicParams.rcAlg == IH264_RATECONTROL_PRC_LOW_DELAY)
        {
            algObj->u.h264AlgIfObj.dynamicParams.rateControlParams.HRDBufferSize
                = algObj->algDynamicParams.targetBitRate;
            algObj->u.h264AlgIfObj.dynamicParams.rateControlParams.initialBufferLevel
                = algObj->u.h264AlgIfObj.dynamicParams.rateControlParams.HRDBufferSize;
        }

        #ifdef SYSTEM_VERBOSE_PRINTS
        Vps_printf("\n ENCLINK: new RcAlg Param to set:%d\n",
                algObj->algDynamicParams.rcAlg);
        #endif
        algObj->setConfigBitMask &= (ENC_LINK_SETCONFIG_BITMASK_RESET_VALUE ^
                                    (1 << ENC_LINK_SETCONFIG_BITMASK_RCALGO));
        setConfigFlag = TRUE;
    }

    /* Set the modified encoder QP range values for Intra Frame */
    if ((bitMask >>  ENC_LINK_SETCONFIG_BITMASK_QPI) & 0x1)
    {
        algObj->u.h264AlgIfObj.dynamicParams.rateControlParams.rateControlParamsPreset = IH264_RATECONTROLPARAMS_USERDEFINED;
        algObj->u.h264AlgIfObj.dynamicParams.rateControlParams.qpMinI   = algObj->algDynamicParams.qpMinI;
        algObj->u.h264AlgIfObj.dynamicParams.rateControlParams.qpMaxI   = algObj->algDynamicParams.qpMaxI;
        algObj->u.h264AlgIfObj.dynamicParams.rateControlParams.qpI      = algObj->algDynamicParams.qpInitI;
        #ifdef SYSTEM_VERBOSE_PRINTS
        Vps_printf("\n ENCLINK: new QP I Param to set:%d %d %d\n",
                algObj->algDynamicParams.qpMinI, algObj->algDynamicParams.qpMaxI, algObj->algDynamicParams.qpInitI);
        #endif
        algObj->setConfigBitMask &= (ENC_LINK_SETCONFIG_BITMASK_RESET_VALUE ^
                                    (1 << ENC_LINK_SETCONFIG_BITMASK_QPI));
        setConfigFlag = TRUE;
    }

    /* Set the modified encoder QP range values for Inter Frame */
    if ((bitMask >>  ENC_LINK_SETCONFIG_BITMASK_QPP) & 0x1)
    {
        algObj->u.h264AlgIfObj.dynamicParams.rateControlParams.rateControlParamsPreset = IH264_RATECONTROLPARAMS_USERDEFINED;
        algObj->u.h264AlgIfObj.dynamicParams.rateControlParams.qpMinP   = algObj->algDynamicParams.qpMinP;
        algObj->u.h264AlgIfObj.dynamicParams.rateControlParams.qpMaxP   = algObj->algDynamicParams.qpMaxP;
        algObj->u.h264AlgIfObj.dynamicParams.rateControlParams.qpP      = algObj->algDynamicParams.qpInitP;
        #ifdef SYSTEM_VERBOSE_PRINTS
        Vps_printf("\n ENCLINK: new QP P Param to set:%d %d %d\n",
                algObj->algDynamicParams.qpMinP, algObj->algDynamicParams.qpMaxP, algObj->algDynamicParams.qpInitP);
        #endif

        /* This is a workaround for the codec bug of SDOCM00087325*/
        {
            algObj->u.h264AlgIfObj.dynamicParams.rateControlParams.qpOffsetB = 0;
            algObj->u.h264AlgIfObj.dynamicParams.rateControlParams.qpMaxB = algObj->u.h264AlgIfObj.dynamicParams.rateControlParams.qpMaxP;
            algObj->u.h264AlgIfObj.dynamicParams.rateControlParams.qpMinB = algObj->u.h264AlgIfObj.dynamicParams.rateControlParams.qpMaxB;
        }

        algObj->setConfigBitMask &= (ENC_LINK_SETCONFIG_BITMASK_RESET_VALUE ^
                                    (1 << ENC_LINK_SETCONFIG_BITMASK_QPP));
        setConfigFlag = TRUE;
    }
    /* Set the modified encoder VBRDuration value for CVBR */
    if ((bitMask >>  ENC_LINK_SETCONFIG_BITMASK_VBRD) & 0x1)
    {
        algObj->u.h264AlgIfObj.dynamicParams.rateControlParams.rateControlParamsPreset = IH264_RATECONTROLPARAMS_USERDEFINED;
        algObj->u.h264AlgIfObj.dynamicParams.rateControlParams.VBRDuration = algObj->algDynamicParams.vbrDuration;
        #ifdef SYSTEM_VERBOSE_PRINTS
        Vps_printf("\n ENCLINK: new VBR Duration Param to set:%d\n",
                   algObj->algDynamicParams.vbrDuration);
        #endif
        algObj->setConfigBitMask &= (ENC_LINK_SETCONFIG_BITMASK_RESET_VALUE ^
                                    (1 << ENC_LINK_SETCONFIG_BITMASK_VBRD));
        setConfigFlag = TRUE;
    }
    /* Set the modified encoder VBRsensitivity value for CVBR */
    if ((bitMask >>  ENC_LINK_SETCONFIG_BITMASK_VBRS) & 0x1)
    {
        algObj->u.h264AlgIfObj.dynamicParams.rateControlParams.rateControlParamsPreset = IH264_RATECONTROLPARAMS_USERDEFINED;
        algObj->u.h264AlgIfObj.dynamicParams.rateControlParams.VBRsensitivity = algObj->algDynamicParams.vbrSensitivity;
        #ifdef SYSTEM_VERBOSE_PRINTS
        Vps_printf("\n ENCLINK: new VBR Sensitivity Param to set:%d\n",
                   algObj->algDynamicParams.vbrSensitivity);
        #endif
        algObj->setConfigBitMask &= (ENC_LINK_SETCONFIG_BITMASK_RESET_VALUE ^
                                    (1 << ENC_LINK_SETCONFIG_BITMASK_VBRS));
        setConfigFlag = TRUE;
    }

    /* Set the toggle for privacy mask ROI setting */
    if ((bitMask >>  ENC_LINK_SETCONFIG_BITMASK_ROI) & 0x1)
    {
        int i = 0;
        algObj->u.h264AlgIfObj.dynamicParams.enableROI =
            algObj->algDynamicParams.roiParams.roiNumOfRegion;
        IH264ENC_InArgs *inArgs = &algObj->u.h264AlgIfObj.inArgs;

        inArgs->roiInputParams.numOfROI =
            algObj->u.h264AlgIfObj.dynamicParams.enableROI;

        for (i = 0; i < inArgs->roiInputParams.numOfROI; i++)
        {
            inArgs->roiInputParams.listROI[i].topLeft.x =
                algObj->algDynamicParams.roiParams.roiStartX[i];
            inArgs->roiInputParams.listROI[i].topLeft.y =
                algObj->algDynamicParams.roiParams.roiStartY[i];
            inArgs->roiInputParams.listROI[i].bottomRight.x =
                algObj->algDynamicParams.roiParams.roiStartX[i] +
                algObj->algDynamicParams.roiParams.roiWidth[i];
            inArgs->roiInputParams.listROI[i].bottomRight.y =
                algObj->algDynamicParams.roiParams.roiStartY[i] +
                algObj->algDynamicParams.roiParams.roiHeight[i];
            inArgs->roiInputParams.roiType[i] = algObj->algDynamicParams.roiParams.roiType[i];
            inArgs->roiInputParams.roiPriority[i] = algObj->algDynamicParams.roiParams.roiPriority[i];
        }

        algObj->setConfigBitMask &= (ENC_LINK_SETCONFIG_BITMASK_RESET_VALUE ^
                                    (1 << ENC_LINK_SETCONFIG_BITMASK_ROI));
        setConfigFlag = TRUE;
    }

    Hwi_restore(key);

    if (setConfigFlag)
    {
        status = enclink_h264_control(algObj->u.h264AlgIfObj.algHandle,
                                         XDM_SETPARAMS,
                                         &algObj->u.h264AlgIfObj.dynamicParams,
                                         &algObj->u.h264AlgIfObj.status);
        if (UTILS_ISERROR(status))
        {
            UTILS_warn("\n ENCLINK: ERROR in Run time parameters changes, "
                  "Extended Error code:%d \n",
                  algObj->u.h264AlgIfObj.status.videnc2Status.extendedError);
        }
        /*else
        {
            Vps_printf("\n ENCLINK: Run time parameters changed %d\n",
                algObj->u.h264AlgIfObj.status.videnc2Status.extendedError);
        }*/
     }

    return (status);
}

Int32 EncLinkH264_algGetConfig(EncLink_algObj * algObj)
{
    Int retVal = ENC_LINK_S_SUCCESS, chId;
    IH264ENC_DynamicParams dynamicParams;
    IH264ENC_Status status;

    if(algObj->getConfigFlag == TRUE)
    {
        status.videnc2Status.size = sizeof(IH264ENC_Status);
        dynamicParams.videnc2DynamicParams.size = sizeof(IH264ENC_DynamicParams);

        retVal = enclink_h264_control(algObj->u.h264AlgIfObj.algHandle,
                                         XDM_GETSTATUS,
                                         &dynamicParams,
                                         &status);
        if (UTILS_ISERROR(retVal))
        {
            UTILS_warn("\n ENCLINK: ERROR in Run time parameters changes,"
                  "Extended Error code:%d \n",
            status.videnc2Status.extendedError);
        }

        chId = algObj->u.h264AlgIfObj.channelID;

        enclink_h264_print_dynamic_params(chId, (IH264ENC_DynamicParams*)&status.videnc2Status.encDynamicParams);

        algObj->getConfigFlag = FALSE;

        algObj->algDynamicParams.inputWidth =
              status.videnc2Status.encDynamicParams.inputWidth;
        algObj->algDynamicParams.inputHeight =
              status.videnc2Status.encDynamicParams.inputHeight;
        algObj->algDynamicParams.targetBitRate =
              status.videnc2Status.encDynamicParams.targetBitRate;
        algObj->algDynamicParams.targetFrameRate =
              status.videnc2Status.encDynamicParams.targetFrameRate;
        algObj->algDynamicParams.intraFrameInterval =
              status.videnc2Status.encDynamicParams.intraFrameInterval;
        algObj->algDynamicParams.forceFrame =
              status.videnc2Status.encDynamicParams.forceFrame;
        algObj->algDynamicParams.refFrameRate =
              status.videnc2Status.encDynamicParams.refFrameRate;
    }

    return (retVal);
}

Int EncLinkH264_algDynamicParamUpdate(EncLink_H264Obj * hObj,
                               EncLink_AlgCreateParams * algCreateParams,
                               EncLink_AlgDynamicParams * algDynamicParams)
{
    Int retVal = ENC_LINK_S_SUCCESS;

    enclink_h264_set_dynamic_params(&hObj->dynamicParams, algDynamicParams);
    enclink_h264_set_algObject(hObj, algCreateParams, algDynamicParams);

    return (retVal);
}

static Int32 EncLink_h264EncoderReset(EncLink_H264Obj * hObj)
{
    int error;

    IH264ENC_Handle handle;
    IALG_Fxns *fxns = NULL;

    handle = hObj->algHandle;
    fxns = (IALG_Fxns *) handle->fxns;

    fxns->algActivate((IALG_Handle) handle);
    error = enclink_h264_control(handle,
                                XDM_RESET,
                                &(hObj->dynamicParams),
                                &(hObj->status));

    fxns->algDeactivate((IALG_Handle) handle);

    return (error);
}

