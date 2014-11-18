
#include <mcfw/src_bios6/utils/utils.h>

/*
    EDMA resource allocation across different cores

    - Que Priority set from M3 VPSS side

        Que 0 - Pri 0 - typically used for audio from Linux/A8
        Que 1 - Pri 4
        Que 2 - Pri 4
        Que 3 - Pri 4

    - TC to Que mapping is 1:1

    HOST A8 - Defined in <linux kernel>\arch\arm\mach-omap2\devices.c,
        - ti816x_dma_rsv_chans[], ti816x_dma_rsv_slots[],
        - ti814x_dma_rsv_chans[], ti814x_dma_rsv_slots[],
            - Above array's define the chans, slots that are NOT used by A8
    =======
    Region    : 0
    Default Q : NA
    EDMA CHs  : 2-25, 32-47, 52-55          - ti816x_dma_rsv_chans[], ti814x_dma_rsv_chans[]
    TCCs      : Same as EDMA CHs
    QDMA Chs  : NOT USED
    PaRAMs    : Same as EDMA CHs, 192-255   - ti816x_dma_rsv_slots[], ti814x_dma_rsv_slots[]

    DSP
    =======
    Region    : 1                           (defined in FC_RMAN_IRES_c6xdsp.cfg)
    Default Q : 3                           (defined in FC_RMAN_IRES_c6xdsp.cfg)
    EDMA CHs  : see UTILS_C6XDSP_EDMACH_ALLOC_x below
    TCCs      : Same as EDMA CHs
    QDMA Chs  : see UTILS_C6XDSP_QDMACH_ALLOC_x below
    PaRAMs    : Same as EDMA CHs, see UTILS_C6XDSP_PARAM_ALLOC_x below

    M3 VPSS
    =======
    Region    : 5                           (defined in utils_dma_m3vpss_cfg.c)
    Default Q : 3                           (defined in utils_dma.h)
    EDMA CHs  : see UTILS_M3VPSS_EDMACH_ALLOC_x below
    TCCs      : Same as EDMA CHs
    QDMA Chs  : see UTILS_M3VPSS_QDMACH_ALLOC_x below
    PaRAMs    : Same as EDMA CHs, see UTILS_M3VPSS_PARAM_ALLOC_x below

    M3 VPSS
    =======
    Region    : 4                           (defined in utils_dma_m3video_cfg.c)
    Default Q : 3                           (defined in utils_dma.h)
    EDMA CHs  : see UTILS_M3VIDEO_EDMACH_ALLOC_x below
    TCCs      : Same as EDMA CHs
    QDMA Chs  : see UTILS_M3VIDEO_QDMACH_ALLOC_x below
    PaRAMs    : Same as EDMA CHs, see UTILS_M3VIDEO_PARAM_ALLOC_x below

    NOTE,

    - when region is changed EDMA3_CC_XFER_COMPLETION_INT should also be changed in utils_dma_xxxx_cfg.c
    - EDMA usage on processors and platforms is shown below
       -------------------------------------------------------
       |         |    A8   |   VPSS-M3  |   Video-M3 |   DSP |
       -------------------------------------------------------
       | DM816x  |    YES  |   YES      |   NO       |   YES |
       | DM814x  |    YES  |   YES      |   NO       |   YES |
       | DM810x  |    YES  |   YES      |   YES      |   NO  |
       -------------------------------------------------------
    - SIMCOP default EDMA queue used is defined in utils_dma_simcop_cfg.c
    - Static EDMA allocation check for Video-M3, VPSS-M3, DSP is done in Utils_dmaCheckStaticAllocationConlficts()
        during init. Modify this logic when supporting EDMA on a unsupported processor.
    - EDMA allocation conflict check with Linux/A8 MUST be done manually by integrator
        - In DVR RDK this check is done and EDMA allocation in this file is set to not conflict the
            EDMA allocation on Linux/A8
*/

/*
    This file in included in
    - utils_dma_c6xdsp_cfg.c
    - utils_dma_m3vpss_cfg.c
    - utils_dma_m3video_cfg.c
*/

#define UTILS_M3VPSS_QDMACH_ALLOC_0     (0x00000000)

#define UTILS_M3VPSS_EDMACH_ALLOC_0     (0x00000000)
#define UTILS_M3VPSS_EDMACH_ALLOC_1     (0xFC000000)

#define UTILS_M3VPSS_PARAM_ALLOC_0      (0x0000000F)
#define UTILS_M3VPSS_PARAM_ALLOC_1      (0x00000000)
#define UTILS_M3VPSS_PARAM_ALLOC_2      (0x00000000)
#define UTILS_M3VPSS_PARAM_ALLOC_3      (0x00000000)
#define UTILS_M3VPSS_PARAM_ALLOC_4      (0x00000000)
#define UTILS_M3VPSS_PARAM_ALLOC_5      (0x00000000)
#define UTILS_M3VPSS_PARAM_ALLOC_6      (0x00000000)
#define UTILS_M3VPSS_PARAM_ALLOC_7      (0x00000000)
#define UTILS_M3VPSS_PARAM_ALLOC_8      (0x00000000)
#define UTILS_M3VPSS_PARAM_ALLOC_9      (0x00000000)
#define UTILS_M3VPSS_PARAM_ALLOC_10     (0x00000000)
#define UTILS_M3VPSS_PARAM_ALLOC_11     (0x00000000)
#define UTILS_M3VPSS_PARAM_ALLOC_12     (0x00000000)
#define UTILS_M3VPSS_PARAM_ALLOC_13     (0x00000000)

#define UTILS_C6XDSP_QDMACH_ALLOC_0     (0x0000000F)

#define UTILS_C6XDSP_EDMACH_ALLOC_0     (0xFC000003)
#define UTILS_C6XDSP_EDMACH_ALLOC_1     (0x030F0000)

#define UTILS_C6XDSP_PARAM_ALLOC_0      (0xFFFFFFF0)
#define UTILS_C6XDSP_PARAM_ALLOC_1      (0xFFFFFFFF)
#define UTILS_C6XDSP_PARAM_ALLOC_2      (0xFFFFFFFF)
#define UTILS_C6XDSP_PARAM_ALLOC_3      (0xFFFFFFFF)
#define UTILS_C6XDSP_PARAM_ALLOC_4      (0x00000000)
#define UTILS_C6XDSP_PARAM_ALLOC_5      (0x00000000)
#define UTILS_C6XDSP_PARAM_ALLOC_6      (0x00000000)
#define UTILS_C6XDSP_PARAM_ALLOC_7      (0x00000000)
#define UTILS_C6XDSP_PARAM_ALLOC_8      (0x00000000)
#define UTILS_C6XDSP_PARAM_ALLOC_9      (0x00000000)
#define UTILS_C6XDSP_PARAM_ALLOC_10     (0x00000000)
#define UTILS_C6XDSP_PARAM_ALLOC_11     (0x00000000)
#define UTILS_C6XDSP_PARAM_ALLOC_12     (0x00000000)
#define UTILS_C6XDSP_PARAM_ALLOC_13     (0x00000000)

#define UTILS_M3VIDEO_QDMACH_ALLOC_0     (0x00000000)

#define UTILS_M3VIDEO_EDMACH_ALLOC_0     (0xC0000000)
#define UTILS_M3VIDEO_EDMACH_ALLOC_1     (0x030F0000)

#define UTILS_M3VIDEO_PARAM_ALLOC_0      (0x00000000)
#define UTILS_M3VIDEO_PARAM_ALLOC_1      (0x00000000)
#define UTILS_M3VIDEO_PARAM_ALLOC_2      (0x00000000)
#define UTILS_M3VIDEO_PARAM_ALLOC_3      (0x00000000)
#define UTILS_M3VIDEO_PARAM_ALLOC_4      (0x00000000)
#define UTILS_M3VIDEO_PARAM_ALLOC_5      (0xFF000000)
#define UTILS_M3VIDEO_PARAM_ALLOC_6      (0xFFFFFFFF)
#define UTILS_M3VIDEO_PARAM_ALLOC_7      (0xFFFFFFFF)
#define UTILS_M3VIDEO_PARAM_ALLOC_8      (0xFFFFFFFF)
#define UTILS_M3VIDEO_PARAM_ALLOC_9      (0xFFFFFFFF)
#define UTILS_M3VIDEO_PARAM_ALLOC_10     (0xFFFFFFFF)
#define UTILS_M3VIDEO_PARAM_ALLOC_11     (0xFFFFFFFF)
#define UTILS_M3VIDEO_PARAM_ALLOC_12     (0xFFFFFFFF)
#define UTILS_M3VIDEO_PARAM_ALLOC_13     (0xFFFFFFFF)



/**
 * Variable which will be used internally for referring transfer completion
 * interrupt.
 */
unsigned int gUtils_ccXferCompInt[EDMA3_MAX_REGIONS] = {
                            EDMA3_CC_XFER_COMPLETION_INT, EDMA3_CC_XFER_COMPLETION_INT, EDMA3_CC_XFER_COMPLETION_INT, EDMA3_CC_XFER_COMPLETION_INT,
                            EDMA3_CC_XFER_COMPLETION_INT, EDMA3_CC_XFER_COMPLETION_INT, EDMA3_CC_XFER_COMPLETION_INT, EDMA3_CC_XFER_COMPLETION_INT,
                        };

/**
 * Variable which will be used internally for referring channel controller's
 * error interrupt.
 */
unsigned int gUtils_ccErrorInt = EDMA3_CC_ERROR_INT;

/**
 * Variable which will be used internally for referring transfer controllers'
 * error interrupts.
 */
unsigned int gUtils_tcErrorInt[EDMA3_MAX_REGIONS] =
                                {
                                EDMA3_TC0_ERROR_INT, EDMA3_TC1_ERROR_INT,
                                EDMA3_TC2_ERROR_INT, EDMA3_TC3_ERROR_INT,
                                EDMA3_TC4_ERROR_INT, EDMA3_TC5_ERROR_INT,
                                EDMA3_TC6_ERROR_INT, EDMA3_TC7_ERROR_INT,
                                };


/* Driver Object Initialization Configuration */
EDMA3_DRV_GblConfigParams gUtils_dmaGblCfgParams =

	    {
	    /** Total number of DMA Channels supported by the EDMA3 Controller */
	    64u,
	    /** Total number of QDMA Channels supported by the EDMA3 Controller */
	    8u,
	    /** Total number of TCCs supported by the EDMA3 Controller */
	    64u,
	    /** Total number of PaRAM Sets supported by the EDMA3 Controller */
	    512u,
	    /** Total number of Event Queues in the EDMA3 Controller */
	    4u,
	    /** Total number of Transfer Controllers (TCs) in the EDMA3 Controller */
	    4u,
	    /** Number of Regions on this EDMA3 controller */
	    6u,

	    /**
	     * \brief Channel mapping existence
	     * A value of 0 (No channel mapping) implies that there is fixed association
	     * for a channel number to a parameter entry number or, in other words,
	     * PaRAM entry n corresponds to channel n.
	     */
	    1u,

	    /** Existence of memory protection feature */
	    1u,

	    /** Global Register Region of CC Registers */
	    (void *)0x49000000u,
	    /** Transfer Controller (TC) Registers */
	        {
	        (void *)0x49800000u,
	        (void *)0x49900000u,
	        (void *)0x49A00000u,
	        (void *)0x49B00000u,
	        (void *)NULL,
	        (void *)NULL,
	        (void *)NULL,
	        (void *)NULL
	        },
	    /** Interrupt no. for Transfer Completion */
	    EDMA3_CC_XFER_COMPLETION_INT,
	    /** Interrupt no. for CC Error */
	    EDMA3_CC_ERROR_INT,
	    /** Interrupt no. for TCs Error */
	        {
	        EDMA3_TC0_ERROR_INT,
	        EDMA3_TC1_ERROR_INT,
	        EDMA3_TC2_ERROR_INT,
	        EDMA3_TC3_ERROR_INT,
	        EDMA3_TC4_ERROR_INT,
	        EDMA3_TC5_ERROR_INT,
	        EDMA3_TC6_ERROR_INT,
	        EDMA3_TC7_ERROR_INT
	        },

	    /**
	     * \brief EDMA3 TC priority setting
	     *
	     * User can program the priority of the Event Queues
	     * at a system-wide level.  This means that the user can set the
	     * priority of an IO initiated by either of the TCs (Transfer Controllers)
	     * relative to IO initiated by the other bus masters on the
	     * device (ARM, DSP, USB, etc)
	     */
	        {
			0u,
	        4u,
	        4u,
	        4u,
	        0u,
	        0u,
	        0u,
	        0u
	        },
	    /**
	     * \brief To Configure the Threshold level of number of events
	     * that can be queued up in the Event queues. EDMA3CC error register
	     * (CCERR) will indicate whether or not at any instant of time the
	     * number of events queued up in any of the event queues exceeds
	     * or equals the threshold/watermark value that is set
	     * in the queue watermark threshold register (QWMTHRA).
	     */
	        {
	        16u,
	        16u,
	        16u,
	        16u,
	        0u,
	        0u,
	        0u,
	        0u
	        },

	    /**
	     * \brief To Configure the Default Burst Size (DBS) of TCs.
	     * An optimally-sized command is defined by the transfer controller
	     * default burst size (DBS). Different TCs can have different
	     * DBS values. It is defined in Bytes.
	     */
	        {
	        16u,
	        16u,
	        16u,
	        16u,
	        0u,
	        0u,
	        0u,
	        0u
	        },

	    /**
	     * \brief Mapping from each DMA channel to a Parameter RAM set,
	     * if it exists, otherwise of no use.
	     */
            {
            0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u,
            8u, 9u, 10u, 11u, 12u, 13u, 14u, 15u,
            16u, 17u, 18u, 19u, 20u, 21u, 22u, 23u,
            24u, 25u, 26u, 27u, 28u, 29u, 30u, 31u,
            32u, 33u, 34u, 35u, 36u, 37u, 38u, 39u,
            40u, 41u, 42u, 43u, 44u, 45u, 46u, 47u,
            48u, 49u, 50u, 51u, 52u, 53u, 54u, 55u,
            56u, 57u, 58u, 59u, 60u, 61u, 62u, 63u
            },

	     /**
	      * \brief Mapping from each DMA channel to a TCC. This specific
	      * TCC code will be returned when the transfer is completed
	      * on the mapped channel.
	      */
            {
            0u, 1u, 2u, 3u,
            4u, 5u, 6u, 7u,
            8u, 9u, 10u, 11u,
            12u, 13u, 14u, 15u,
            16u, 17u, 18u, 19u,
            20u, 21u, 22u, 23u,
            24u, 25u, 26u, 27u,
            28u, 29u, 30u, 31u,
            32u, 33u, 34u, 35u,
            36u, 37u, 38u, 39u,
            40u, 41u, 42u, 43u,
            44u, 45u, 46u, 47u,
            48u, 49u, 50u, 51u,
            52u, 53u, 54u, 55u,
            56u, 57u, 58u, 59u,
            60u, 61u, 62u, 63u
            },


	    /**
	     * \brief Mapping of DMA channels to Hardware Events from
	     * various peripherals, which use EDMA for data transfer.
	     * All channels need not be mapped, some can be free also.
	     */
	        {
	        0x00000000u,
	        0x00000000u
	        },
};

static inline void Utils_dmaCheckClonfict(UInt32 dspMask, UInt32 m3vpssMask, UInt32 m3videoMask)
{
    UInt32 mask[3];

    mask[0] = mask[1] = mask[2] = 0;

    #ifdef TI_8107_BUILD
    /* no need to check DSP since DSP is not present in DM810x */
    mask[0] = m3vpssMask & m3videoMask;
    #endif

    #ifdef TI_816X_BUILD
    /* no need to check Video-M3 since DMA init is not done for Video-M3 */
    mask[0] = m3vpssMask & dspMask;
    #endif

    #ifdef TI_814X_BUILD
    /* no need to check Video-M3 since DMA init is not done for Video-M3 */
    mask[0] = m3vpssMask & dspMask;
    #endif

    if(mask[0]||mask[1]||mask[2])
    {
        Vps_printf(" #### ERROR: UTILS: DMA: Conflict in EDMA static allocation !!!\n"
                   " ####        Check static EDMA allocation and try again.\n"
                   " ####        See file %s\n", __FILE__
            );
        UTILS_assert(0);
    }
}

void Utils_dmaCheckStaticAllocationConlficts()
{
    Utils_dmaCheckClonfict(
        UTILS_C6XDSP_QDMACH_ALLOC_0,
        UTILS_M3VPSS_QDMACH_ALLOC_0,
        UTILS_M3VIDEO_QDMACH_ALLOC_0
        );

    Utils_dmaCheckClonfict(
        UTILS_C6XDSP_EDMACH_ALLOC_0,
        UTILS_M3VPSS_EDMACH_ALLOC_0,
        UTILS_M3VIDEO_EDMACH_ALLOC_0
        );
    Utils_dmaCheckClonfict(
        UTILS_C6XDSP_EDMACH_ALLOC_1,
        UTILS_M3VPSS_EDMACH_ALLOC_1,
        UTILS_M3VIDEO_EDMACH_ALLOC_1
        );

    Utils_dmaCheckClonfict(
        UTILS_C6XDSP_PARAM_ALLOC_0,
        UTILS_M3VPSS_PARAM_ALLOC_0,
        UTILS_M3VIDEO_PARAM_ALLOC_0
        );
    Utils_dmaCheckClonfict(
        UTILS_C6XDSP_PARAM_ALLOC_1,
        UTILS_M3VPSS_PARAM_ALLOC_1,
        UTILS_M3VIDEO_PARAM_ALLOC_1
        );
    Utils_dmaCheckClonfict(
        UTILS_C6XDSP_PARAM_ALLOC_2,
        UTILS_M3VPSS_PARAM_ALLOC_2,
        UTILS_M3VIDEO_PARAM_ALLOC_2
        );
    Utils_dmaCheckClonfict(
        UTILS_C6XDSP_PARAM_ALLOC_3,
        UTILS_M3VPSS_PARAM_ALLOC_3,
        UTILS_M3VIDEO_PARAM_ALLOC_3
        );
    Utils_dmaCheckClonfict(
        UTILS_C6XDSP_PARAM_ALLOC_4,
        UTILS_M3VPSS_PARAM_ALLOC_4,
        UTILS_M3VIDEO_PARAM_ALLOC_4
        );
    Utils_dmaCheckClonfict(
        UTILS_C6XDSP_PARAM_ALLOC_5,
        UTILS_M3VPSS_PARAM_ALLOC_5,
        UTILS_M3VIDEO_PARAM_ALLOC_5
        );
    Utils_dmaCheckClonfict(
        UTILS_C6XDSP_PARAM_ALLOC_6,
        UTILS_M3VPSS_PARAM_ALLOC_6,
        UTILS_M3VIDEO_PARAM_ALLOC_6
        );
    Utils_dmaCheckClonfict(
        UTILS_C6XDSP_PARAM_ALLOC_7,
        UTILS_M3VPSS_PARAM_ALLOC_7,
        UTILS_M3VIDEO_PARAM_ALLOC_7
        );
    Utils_dmaCheckClonfict(
        UTILS_C6XDSP_PARAM_ALLOC_8,
        UTILS_M3VPSS_PARAM_ALLOC_8,
        UTILS_M3VIDEO_PARAM_ALLOC_8
        );
    Utils_dmaCheckClonfict(
        UTILS_C6XDSP_PARAM_ALLOC_9,
        UTILS_M3VPSS_PARAM_ALLOC_9,
        UTILS_M3VIDEO_PARAM_ALLOC_9
        );
    Utils_dmaCheckClonfict(
        UTILS_C6XDSP_PARAM_ALLOC_10,
        UTILS_M3VPSS_PARAM_ALLOC_10,
        UTILS_M3VIDEO_PARAM_ALLOC_10
        );
    Utils_dmaCheckClonfict(
        UTILS_C6XDSP_PARAM_ALLOC_11,
        UTILS_M3VPSS_PARAM_ALLOC_11,
        UTILS_M3VIDEO_PARAM_ALLOC_11
        );
    Utils_dmaCheckClonfict(
        UTILS_C6XDSP_PARAM_ALLOC_12,
        UTILS_M3VPSS_PARAM_ALLOC_12,
        UTILS_M3VIDEO_PARAM_ALLOC_12
        );
    Utils_dmaCheckClonfict(
        UTILS_C6XDSP_PARAM_ALLOC_13,
        UTILS_M3VPSS_PARAM_ALLOC_13,
        UTILS_M3VIDEO_PARAM_ALLOC_13
        );


}



