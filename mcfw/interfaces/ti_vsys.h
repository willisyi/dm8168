/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
    \ingroup MCFW_API
    \defgroup MCFW_SYS_API McFW System Level (VSYS) API

    @{
*/

/**
    \file ti_vsys.h
    \brief McFW System Level (VSYS) API
*/

#ifndef __TI_VSYS_H__
#define __TI_VSYS_H__


#ifdef __cplusplus
extern "C" {
#endif

#include "ti_media_common_def.h"
#include "common_def/ti_vsys_common_def.h"

/* =============================================================================
 * Defines
 * =============================================================================
 */


/** \brief Floor a integer value. */
#define VsysUtils_floor(val, align)  (((val) / (align)) * (align))

/** \brief Align a integer value. */
#define VsysUtils_align(val, align)  VsysUtils_floor(((val) + (align)-1), (align))

/* =============================================================================
 * Enums
 * =============================================================================
 */


/**
    \brief User specified callback that gets called when a event occurs on the slave processor

    \param eventId  [OUT] This tells the user which event occured
                    based on this user can call other McFW APIs to get more details about the event

    \param appData  [OUT] User specified app data pointer during Vsys_registerEventHandler()
                    is returned here to the user.

    \return ERROR_NONE on sucess
*/
typedef Int32 (*VSYS_EVENT_HANDLER_CALLBACK)(UInt32 eventId, Ptr pPrm, Ptr appData);

/**
    \brief McFW Sub-system's or modules
*/
typedef enum
{
    VCAP,
    /**< Video Capture Sub-system */

    VENC,
    /**< Video Encode Sub-system */

    VDEC,
    /**< Video Decode Sub-system */

    VDIS,
    /**< Video Display Sub-system */

    VSYS_MODULES_MAX
    /**< Max sub-system's in McFW */

} VSYS_MODULES_E;

/**
    \brief System use-case's

    Inter-connection of sub-system's and internal HW processing
    block's depends on the system use-case that is selected.
*/
typedef enum
{
    VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC,
    /**< Video Capture to Video Encode(Progressive)+Video Display and Video Decode+Video Display use-case. All sub-systems are enabled. \n
        This is for Surveillence multi-ch D1 DVR use-case.
        Scaled <CIF or any resolution> Secondary output, encode / bits out is enabled based on runtime flag.
    */

    VSYS_USECASE_MULTICHN_INTERLACED_VCAP_VDIS_VENC_VDEC,
    /**< Video Capture to Video Encode(Inetrlaced)+Video Display and Video Decode+Video Display use-case. All sub-systems are enabled. \n
        This is for Survillence multi-ch D1 DVR use-case.
    */

    VSYS_USECASE_MULTICHN_VCAP_VENC,
    /**< Video Capture to Video Encode use-case. \n
        This is for Survillence multi-ch D1 DVS use-case.

        One HD Display (On-chip HDMI) can be optionally enabled for local preview
     */

    VSYS_USECASE_MULTICHN_HD_VCAP_VENC,
    /**< Video Capture to Video Encode use-case for 4x 1080p HD. \n
        This is for Survillence multi-ch HD-DVS use-case.

        One HD Display (On-chip HDMI) can be optionally enabled for local preview
     */

    VSYS_USECASE_MULTICHN_VDEC_VDIS,
    /**< A8 to Video Display use-case,\n
        It will support up to 32D1 to display.
    */

    VSYS_USECASE_MULTICHN_VCAP_VDIS,
    /**< Video Capture to Video Display use-case, Video Encode and Decode is kept disabled. \n
        Use-full for HW Board Check out for multi-ch D1 system.
    */

    VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_D1_AND_CIF,
    /**< Video Capture to Video Encode(Progressive)+Video Display and Video Decode+Video Display use-case. All sub-systems are enabled. \n
        This is for Surveillence multi-ch D1 + CIF DVR use-case.
    */

    VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_D1_AND_CIF_8CH,
    /**< On DM810X only, Video Capture to Video Encode(Progressive)+Video
         Display and Video Decode+Video Display use-case. All sub-systems are
         enabled. \n
        This is for Surveillence multi-ch D1 + CIF DVR use-case.
    */

    VSYS_USECASE_MULTICHN_CUSTOM,
    /**<
    */

    VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_4CH,
    /**< Video Capture to Video Encode(Progressive)+Video Display and Video Decode+Video Display use-case. All sub-systems are enabled. \n
        This is for Surveillence multi-ch D1 DVR use-case.
        Scaled <CIF or any resolution> Secondary output, encode / bits out is enabled based on runtime flag.
        Capture is limited to 4D1 in this usecase
    */

    VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_8CH,
    /**< On DM814X only, Video Capture to Video Encode(Progressive)+Video Display and Video Decode+Video Display use-case. All sub-systems are enabled. \n
        This is for Surveillence multi-ch D1 DVR use-case.
    */

    VSYS_USECASE_MULTICHN_HYBRID_DVR_16CH,
    /**< On DM816X only, Video Capture to Video Encode(Progressive)+Video Display and Video Decode+Video Display use-case. All sub-systems are enabled. \n
        This is for Surveillence multi-ch Hybrid DVR use-case.
    */

    VSYS_USECASE_MULTICHN_CAR_DVR_4CH,
    /**< On DM816X only, Video Capture to Video Encode(Progressive)+Video Display and Video Decode+Video Display use-case. All sub-systems are enabled. \n
        This is for Surveillence multi-ch Car DVR use-case.
    */

    VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_16CH_NRT,
    /**< On DM814X only, Video Capture to Video Encode(Progressive)+Video Display and Video Decode+Video Display use-case. All sub-systems are enabled. \n
        This is for Surveillence multi-ch D1 DVR use-case.
    */

    VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_8CH_NRT,
    /**< On DM810X only, Video Capture to Video Encode(Progressive)+Video Display and Video Decode+Video Display use-case. All sub-systems are enabled. \n
        This is for Surveillence multi-ch D1 DVR use-case.
    */

    VSYS_USECASE_MULTICHN_HYBRID_ENC_36CH,
    /**< On DM816X only, Video Capture to Video Encode(Progressive)+Video Display and Video Decode+Video Display use-case. All sub-systems are enabled. \n
        This is for Surveillence multi-ch multi resolution Hybrid Encoder card use-case.
     */

    VSYS_USECASE_MAX
    /**< Maximum use-case ID */

} VSYS_USECASES_E;

/* =============================================================================
 * Structures
 * =============================================================================
 */

/**
    \brief Top level System Configuration structure
*/
typedef struct
{
    Bool   enableCapture;
    /**< Enable/Disable VCAP subsystem */

    Bool   enableNsf;
    /**< Enable/Disable Noise Filter HW Block in VCAP subsystem */

    Bool   enableOsd;
    /**< Enable/Disable Osd in VCAP subsystem */

    Bool   osdFormat;
    /**< Osd Format, 422p or 420p in VCAP subsystem 
      * TRUE  : SYSTEM_DF_YUV422I_YUYV
      * FALSE : SYSTEM_DF_YUV420SP_UV */

    Bool   enableScd;
    /**< Enable/Disable Scd (basic tamper detection) in VCAP subsystem */

    Bool   enableEncode;
    /**< Enable/Disable VENC subsystem */

    Bool   enableDecode;
    /**< Enable/Disable VDEC subsystem */

    Bool   enableNullSrc;
    /**< Enable/Disable NULL Source in framework */

    UInt32 enableAVsync;
    /**<Enable/Disable AvSync  */

    /**< Number of channels */
    UInt32 numChs;

    UInt32 numDeis;
    /**< Number of DEIs to use in VCAP Sub-system */

    UInt32 numSwMs;
    /**< Number of SW Mosaic's to use in VDIS Sub-system */

    UInt32 numDisplays;
    /**< Number of Display's to use in VDIS Sub-system */

    VSYS_USECASES_E systemUseCase;
    /**< System use-case to use */

    Bool enableSecondaryOut;
    /**< System use-case to use */

    Bool enableMjpegEnc;
    /**< System use-case to use */

    Bool enableSclr;
    /** Enable scalar 5 for scaling **/

} VSYS_PARAMS_S;


/**
    \brief Allocated buffer info
*/
typedef struct {

    UInt8  *physAddr;
    /**< Physical address */

    UInt8  *virtAddr;
    /**< Virtual address */

    UInt32  srPtr;
    /**< Shared region Pointer SRPtr */

} Vsys_AllocBufInfo;


/**
    \brief Board Type detect call
*/
typedef struct {

    UInt32  boardRev;
    /**< boardRev */

    UInt32  cpuRev;
    /**< CPU Rev */

    UInt32  boardId;
    /** < Board ID */
} VSYS_PLATFORM_INFO_S;


/* =============================================================================
 * APIs
 * =============================================================================
 */

/**
    \brief Set VSYS_PARAMS_S to default parameters

    Call this before calling Vsys_init() and then overide
    the required parameters in user application.

    \param pContext     [OUT] VSYS_PARAMS_S initialized to default values

    \return ERROR_NONE on success
*/
Int32 Vsys_params_init(VSYS_PARAMS_S * pContext);

/**
    \brief Initialize system for a given system level configuration

    This should be the first McFW API that should be happen

    \param pContext     [IN] System configuration

    \return ERROR_NONE on success
*/
Int32 Vsys_init(VSYS_PARAMS_S * pContext);

/**
    \brief Create and setup processing block's based on sub-system configuration

    This API should be called after sub-system init's ( Vxxx_init() ).
    This API will allocate and setup all HW resource in order for the system
    use-case to run.
    Once this API succeds now the sub-system's can be started to start the system execution

    \return ERROR_NONE on success
*/
Int32 Vsys_create();

/**
    \brief Delete and de-init processing block's based on sub-system configuration

    This API should be called after all sub-system have been stopped ( Vxxx_stop() ).
    This API will de-allocate and bring all HW resources to a known state.

    Once this API succeds now the sub-system's can be exited using Vxxx_exit() APIs

    \return ERROR_NONE on success
*/
Int32 Vsys_delete();


/**
    \brief System de-init

    This is last API to be called. This API should be called after all sus-system exits have been
    called (Vxxx_exit()).

    \return ERROR_NONE on success
*/
Int32 Vsys_exit();


/**
    \brief Print detailed system statistics

    This is useful system debugging.

    \return ERROR_NONE on success
*/
Int32 Vsys_printDetailedStatistics();

/**
    \brief Print detailed buffer statistics of links <useful for debugging>

    This is useful system debugging.

    \return ERROR_NONE on success
*/
Int32 Vsys_printBufferStatistics();

/**
    \brief Allocate contigous buffer from a shared memory

    \param srRegId  [IN] Shared region ID
    \param bufSize  [IN] Buffer size in bytes
    \param bufAlign [IN] Buffer alignment in bytes
    \param bufInfo  [OUT] Allocated buffer info

    \return ERROR_NONE on success
*/
Int32 Vsys_allocBuf(UInt32 srRegId, UInt32 bufSize, UInt32 bufAlign, Vsys_AllocBufInfo *bufInfo);

/**
    \brief Free the buffer allocated from the shared region

    \param srRegId  [IN] Shared region ID
    \param virtAddr [IN] Buffer virtual address
    \param bufSize  [IN] Size of the buffer in bytes

    \return ERROR_NONE on success
*/
Int32 Vsys_freeBuf(UInt32 srRegId, UInt8 *virtAddr, UInt32 bufSize);

/**
    \brief This function is called by McFW when it receives any event from the slave processor

    \param callback [IN] User specified callback
    \param appData  [IN] User specified application data pointer which is also returned during the callback

    \return ERROR_NONE on success
*/

/**
 * \brief:
 *      This function gives CPU ID, board type and revision.
 * \input:
 *      VSYS_PLATFORM_INFO_S sturcture
 * \output:
 *      NA
 * \return
*       ERROR_NOERROR       --  while success
*       ERROR_CODE          --  refer for err defination
*/
Int32 Vsys_getPlatformInfo(VSYS_PLATFORM_INFO_S *pInfo);

Int32 Vsys_registerEventHandler(VSYS_EVENT_HANDLER_CALLBACK callback, Ptr appData);

/**
 * \brief:
 *      This function gives core status. It can be called to check if all the 
 *      cores are alive or any of the cores is crashed.
 * \input:
        coreStatus structure to be populated
 * \output:
 *      coreStatus structure updated with info on each core
 * \return
*       ERROR_NOERROR       --  while success
*       ERROR_CODE          --  refer for err defination
*/
Int32 Vsys_getCoreStatus(VSYS_CORE_STATUS_TBL_S *coreStatusTbl);

/* Set to TRUE to enable fast switching between
    VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_D1_AND_CIF and
    VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC_16CH_NRT
    use-cases.

    This reset to FALSE after Vsys_init() and Vsys_exit().

    Set to TRUE before Vsys_create() and Vsys_delete()

    This will skip over syslink init and Display controller init
    so that GRPX can stay active during the switch.
*/
Int32 Vsys_enableFastUsecaseSwitch(Bool enable);

/**
 * \brief:
 *      This function gives context info.
 * \input:
 *      VSYS_PARAMS_S sturcture
 * \output:
 *      NA
 * \return
*       ERROR_NOERROR       --  while success
*       ERROR_CODE          --  refer for err defination
*/
Void Vsys_getContext(VSYS_PARAMS_S * contextInfo);

/**
 * \brief:
 *      This function configures display controller.
 * \input:
 *      VSYS_PARAMS_S sturcture
 * \output:
 *      NA
 * \return
*       ERROR_NOERROR       --  while success
*       ERROR_CODE          --  refer for err defination
*/
Int32 Vsys_configureDisplay();

/**
 * \brief:
 *      This function de-configures display controller.
 * \input:
 *      VSYS_PARAMS_S sturcture
 * \output:
 *      NA
 * \return
*       ERROR_NOERROR       --  while success
*       ERROR_CODE          --  refer for err defination
*/
Int32 Vsys_deConfigureDisplay();


/**
 * \brief:
 *      This function sets the channel to IVA HD mapping for all encoder and decoder channels.
 * \input:
 *      VSYS_IVA2CHMAP_TBL_S sturcture
 * \output:
 *      NA
 * \return
*       ERROR_NOERROR       --  while success
*       ERROR_CODE          --  refer for err definition
*/
Int32 Vsys_setIVAMap(VSYS_IVA2CHMAP_TBL_S *ivaTbl);

/**
 * \brief:
 *      This function gets the channel the current configured channel to IVA HD mapping.
 * \input:
 *      VSYS_IVA2CHMAP_TBL_S sturcture
 * \output:
 *      NA
 * \return
*       ERROR_NOERROR       --  while success
*       ERROR_CODE          --  refer for err definition
*/
Int32 Vsys_getIVAMap(VSYS_IVA2CHMAP_TBL_S *ivaTbl);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif


/* @} */
