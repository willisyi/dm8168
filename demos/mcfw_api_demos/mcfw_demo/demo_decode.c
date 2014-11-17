
#include <demo.h>
#include <demo_vdec_vdis.h>

char gDemo_decodeSettingsMenu[] = {
    "\r\n ===================="
    "\r\n Decode Settings Menu"
    "\r\n ===================="
    "\r\n"
    "\r\n 1: Disable channel"
    "\r\n 2: Enable  channel"
    "\r\n 3: Create channel"
    "\r\n 4: Delete channel"
    "\r\n 5: Switch File - Works only for Decode->Display usecase"
    "\r\n 6: Enable/Disable Decoder Error Reporting"
    "\r\n b: Get decoder buffer stats"
    "\r\n"
    "\r\n p: Previous Menu"
    "\r\n"
    "\r\n Enter Choice: "
};

/**
*******************************************************************************
 *  @struct sEnumToStringMapping
 *  @brief  Error Name Mapping to give error message.
 *          This structure contains error reporting strings which are mapped to
 *          Codec errors
 *
 *  @param  errorName : Pointer to the error string
 * 
*******************************************************************************
*/
typedef struct _sEnumToStringMapping
{
  char *errorName;
}sEnumToStringMapping;

/*----------------------------------------------------------------------------*/
/* Error strings which are mapped to codec errors                             */
/* Please refer User guide for more details on error strings                  */
/*----------------------------------------------------------------------------*/
static sEnumToStringMapping gDecoderErrorStrings[32] = 
{
  {(char *)"IH264VDEC_ERR_NOSLICE : 0, \0"},
  {(char *)"IH264VDEC_ERR_SPS : 1,"},
  {(char *)"IH264VDEC_ERR_PPS : 2,\0"},
  {(char *)"IH264VDEC_ERR_SLICEHDR : 3,\0"},
  {(char *)"IH264VDEC_ERR_MBDATA : 4,\0"},
  {(char *)"IH264VDEC_ERR_UNAVAILABLESPS : 5,\0"},
  {(char *)"IH264VDEC_ERR_UNAVAILABLEPPS  : 6,\0"},
  {(char *)"IH264VDEC_ERR_INVALIDPARAM_IGNORE : 7\0"},
  {(char *)"XDM_PARAMSCHANGE : 8,\0"},
  {(char *)"XDM_APPLIEDCONCEALMENT : 9,\0"},
  {(char *)"XDM_INSUFFICIENTDATA : 10,\0"},
  {(char *)"XDM_CORRUPTEDDATA : 11,\0"},
  {(char *)"XDM_CORRUPTEDHEADER : 12,\0"},
  {(char *)"XDM_UNSUPPORTEDINPUT : 13,\0"},
  {(char *)"XDM_UNSUPPORTEDPARAM : 14,\0"},
  {(char *)"XDM_FATALERROR : 15\0"},
  {(char *)"IH264VDEC_ERR_UNSUPPFEATURE : 16,\0"},
  {(char *)"IH264VDEC_ERR_METADATA_BUFOVERFLOW : 17,\0"},
  {(char *)"IH264VDEC_ERR_STREAM_END : 18,\0"},
  {(char *)"IH264VDEC_ERR_NO_FREEBUF : 19,\0"},
  {(char *)"IH264VDEC_ERR_PICSIZECHANGE : 20,\0"},
  {(char *)"IH264VDEC_ERR_UNSUPPRESOLUTION : 21,\0"},
  {(char *)"IH264VDEC_ERR_NUMREF_FRAMES : 22,\0"},
  {(char *)"IH264VDEC_ERR_INVALID_MBOX_MESSAGE : 23,\0"},
  {(char *)"IH264VDEC_ERR_DATA_SYNC : 24,\0"},
  {(char *)"IH264VDEC_ERR_MISSINGSLICE : 25,\0"},
  {(char *)"IH264VDEC_ERR_INPUT_DATASYNC_PARAMS : 26,\0"},
  {(char *)"IH264VDEC_ERR_HDVICP2_IMPROPER_STATE : 27,\0"},
  {(char *)"IH264VDEC_ERR_TEMPORAL_DIRECT_MODE : 28,\0"},
  {(char *)"IH264VDEC_ERR_DISPLAYWIDTH : 29,\0"},
  {(char *)"IH264VDEC_ERR_NOHEADER : 30,\0"},
  {(char *)"IH264VDEC_ERR_GAPSINFRAMENUM : 31, \0"}
};


/** 
********************************************************************************
 *  @fn     TestApp_errorReport
 *  @brief  Printing all the errors that are set by codec
 *
 *  @param[in] errMsg : Error Message
 *          
 *  @return None
********************************************************************************
*/

int Demo_decodeErrorStatus(Ptr pPrm)
{

   int errBits;
   int firstTime = 1;
   VDEC_CH_ERROR_MSG * decodeErrMsg = (VDEC_CH_ERROR_MSG *) pPrm;
   int chId;
   int decodeErr;

   chId      = decodeErrMsg->chId;
   decodeErr = decodeErrMsg->errorMsg;

   for(errBits = 0; errBits < 32; errBits++)
   {
     if(decodeErr & (1 << errBits))
     {
       {
        if(firstTime)
        {
//          printf("Error Name: \t BitPositon in ErrorMessage\n");
            firstTime = 0;
        }
        printf("[DECODER ERROR] %d: DECODE CH <%d> ERROR: %s \n", OSA_getCurTimeInMsec(), chId, gDecoderErrorStrings[errBits].errorName);           
       }
     }
   }

    return 0;
}

int Demo_decodeSettings(int demoId)
{
    Bool done = FALSE;
    char ch;
    int chId;


    if(gDemo_info.maxVdecChannels<=0)
    {
        printf(" \n");
        printf(" WARNING: Decode NOT enabled, this menu is NOT valid !!!\n");
        return -1;
    }

    while(!done)
    {
        printf(gDemo_decodeSettingsMenu);

        ch = Demo_getChar();

        switch(ch)
        {
            case '1':
                chId = Demo_getChId("DECODE", gDemo_info.maxVdecChannels);

                Vdec_disableChn(chId);

                /* disable playback channel on display as well */
                /* playback channel ID are after capture channel IDs */
                chId += gDemo_info.maxVcapChannels;

                Demo_displayChnEnable(chId, FALSE);
                break;

            case '2':
                chId = Demo_getChId("DECODE", gDemo_info.maxVdecChannels);

                /* primary stream */
                Vdec_enableChn(chId);

                /* enable playback channel on display as well */
                /* playback channel ID are after capture channel IDs */
                chId += gDemo_info.maxVcapChannels;

                OSA_waitMsecs(500);

                Demo_displayChnEnable(chId, TRUE);
                break;

            case '3':
                {
                  int value, format;
                  VDEC_CHN_CREATE_PARAMS_S vdecCrePrm;
                  
                  vdecCrePrm.chNum = 
                      Demo_getChId("\n\nDECODE", gDemo_info.maxVdecChannels);
                  format = 
                      Demo_getIntValue("\n\nEnter Codec Type (0:H264, 1:MPEG4, 2:MJPEG)",0,2,0);
                  value = 
                      Demo_getIntValue("\n\nSelect Resolution (0:HMP, 1:HD, 2:720P, 3:D1, 4:CIF)", 0, 4, 1);

                  switch (format)
                  {
                      case 0: /* H264 */
                          vdecCrePrm.decFormat = VDEC_CHN_H264;
                          break;
                      case 1: /* MPEG4 */
                          vdecCrePrm.decFormat = VDEC_CHN_MPEG4;
                          break;
                      case 2: /* MJPEG */
                          vdecCrePrm.decFormat = VDEC_CHN_MJPEG;
                          break;
                      default:
                          break;
                  }

                  switch (value)
                  {
                      case 0: /* HMP */
                          vdecCrePrm.targetMaxWidth = 4096;
                          vdecCrePrm.targetMaxHeight = 4096;
                          vdecCrePrm.targetBitRate = 10 * 1000 * 1000;
                          break;
                      case 1: /* HD */
                          vdecCrePrm.targetMaxWidth = 1920;
                          vdecCrePrm.targetMaxHeight = 1080;
                          vdecCrePrm.targetBitRate = 4 * 1000 * 1000;
                          break;
                      case 2: /* 720P */
                          vdecCrePrm.targetMaxWidth = 1280;
                          vdecCrePrm.targetMaxHeight = 720;
                          vdecCrePrm.targetBitRate = 4 * 1000 * 1000;
                          break;
                      case 3: /* D1 */
                          vdecCrePrm.targetMaxWidth = 720;
                          vdecCrePrm.targetMaxHeight = 576;
                          vdecCrePrm.targetBitRate = 2 * 1000 * 1000;
                          break;
                      case 4: /* CIF */
                          vdecCrePrm.targetMaxWidth = 352;
                          vdecCrePrm.targetMaxHeight = 288;
                          vdecCrePrm.targetBitRate = 2 * 1000 * 1000;
                          break;
                      default: /* D1 */
                          vdecCrePrm.targetMaxWidth = 1920;
                          vdecCrePrm.targetMaxHeight = 1080;
                          vdecCrePrm.targetBitRate = 4 * 1000 * 1000;
                          break;
                  }
                  printf ("New resolution for CH %d is %d x %d\n", 
                           vdecCrePrm.chNum, vdecCrePrm.targetMaxWidth, 
                           vdecCrePrm.targetMaxHeight);
                  
                  if ((format == 1) && (value == 0))
                  {
                          vdecCrePrm.targetMaxWidth = 2048;
                          vdecCrePrm.targetMaxHeight = 2048;
                  }

                  vdecCrePrm.targetFrameRate = 30;
                  vdecCrePrm.scanFormat = 1;
                  /* displayDelay valid only for H264, stream specific value */
                  vdecCrePrm.displayDelay = 2;
                  vdecCrePrm.numBufPerCh = 6;
                  vdecCrePrm.numBitBufPerCh = VDEC_NUM_BUF_PER_CH_DEFAULT;

                  Vdec_createChn(&vdecCrePrm);
                }
                break;
            case '4':
                {
                  int chId;
                  chId = Demo_getChId("DECODE", gDemo_info.maxVdecChannels);

                  Vdec_deleteChn(chId);
                }
                break;

            case '5':
                {
                  int FileIndex;
                  int chId;
                  chId = Demo_getChId("DECODE", gDemo_info.maxVdecChannels);
                  FileIndex = 
                      Demo_getIntValue("\n\nEnter the file sequence number (File ID)",
                                       0,(gDemo_info.maxVdecChannels-1),0);
                  VdecVdis_switchInputFile(chId, FileIndex);
                }
                break;
            case 'b':
            {
                int i;
                VDEC_BUFFER_STATS_S bufStats;

                bufStats.numCh = gDemo_info.maxVdecChannels;
                for (i = 0; i < gDemo_info.maxVdecChannels;i++)
                {
                    bufStats.chId[i] = i;
                }
                Vdec_getBufferStatistics(&bufStats);
                printf("\r\n VDEC:Buffer Statistics");
                printf("\r\n ChId | InBufCnt | OutBufCnt");
                for (i = 0; i < gDemo_info.maxVdecChannels;i++)
                {
                    printf("\r\n %5d|%10d|%10d",
                           bufStats.chId[i],
                           bufStats.stats[i].numInBufQueCount,
                           bufStats.stats[i].numOutBufQueCount);
                }
                printf("\n");
                break;
            }
            case '6':
                {
                  Bool flag;
                  int chId;

                  chId = Demo_getChId("DECODE", gDemo_info.maxVdecChannels);

                  flag = Demo_getIntValue("Disable/Enable Decoder Error Report", 0, 1, 1);

                  Vdec_decErrReport(chId, flag);
                }
                break;
            case 'p':
                done = TRUE;
                break;
        }
    }

    return 0;
}

