/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2011 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
    \file utils_dmtimer_regcfg.h
    \brief Register configuration of dual mode timer.

    This file contains device specific register configuration of
    dual mode timer.
    The base addresses have to be populated for each SOC.
*/


#ifndef UTILS_DMTIMER_REGCFG_H_
#define UTILS_DMTIMER_REGCFG_H_

#include <mcfw/src_bios6/utils/utils.h>

#define TIMER_TCLR_START_ONESHOT        0x1
#define TIMER_TCLR_START_CONTINUOUS     0x3
#define TIMER_TCLR_START_DYNAMIC        0x43
#define TIMER_TCLR_STOP_MASK            0xfffffffe
#define TIMER_TWPS_W_PEND_TMAR          0x10
#define TIMER_TWPS_W_PEND_TLDR          0x4
#define TIMER_TWPS_W_PEND_TCRR          0x2
#define TIMER_TWPS_W_PEND_TCLR          0x1
#define TIMER_IRQSTATUS_OVF_IT_FLAG     0x2
#define TIMER_IRQSTATUS_MAT_IT_FLAG     0x1
#define TIMER_TIOCP_CFG_SOFTRESET_FLAG  0x1

typedef volatile struct Utils_dmTimerRegs {
    UInt tidr;
    UInt empty[3];
    UInt tiocpCfg;
    UInt empty1[3];
    UInt irq_eoi;
    UInt irqstat_raw;
    UInt tisr;  /* irqstat */
    UInt tier;  /* irqen_set */
    UInt irqen_clr;
    UInt twer;  /* irqwaken; */
    UInt tclr;
    UInt tcrr;
    UInt tldr;
    UInt ttgr;
    UInt twps;
    UInt tmar;
    UInt tcar1;
    UInt tsicr;
    UInt tcar2;
} Utils_dmTimerRegs;




/** DMTimer base addresses */
#if (defined(TI_816X_BUILD) || defined(TI_814X_BUILD) || defined(TI_8107_BUILD))
#define  UTILS_DMTIMER_NUM_TIMERS                      (7)
#define  UTILS_DMTIMER_TIMER0_BASE_ADDR                (0x4802C000)
#define  UTILS_DMTIMER_TIMER1_BASE_ADDR                (0x4802E000)
#define  UTILS_DMTIMER_TIMER2_BASE_ADDR                (0x48040000)
#define  UTILS_DMTIMER_TIMER3_BASE_ADDR                (0x48042000)
#define  UTILS_DMTIMER_TIMER4_BASE_ADDR                (0x48044000)
#define  UTILS_DMTIMER_TIMER5_BASE_ADDR                (0x48046000)
#define  UTILS_DMTIMER_TIMER6_BASE_ADDR                (0x48048000)
#else
#error "DMTimer base address registers need to be set."
#error "If you see this error it means you are compiling DMTimer module for a new SOC."
#error "You need to set the BASE ADDR for the DMTimers on that SOC (Refer Chapter 4)."
#error "!!!DONT JUST COMMENT OUT THIS ERROR CHECK!!!"
#endif

#ifdef KHZ
#undef KHZ
#endif

#ifdef MHZ
#undef MHZ
#endif

#define KHZ                                           (1000u)
#define MHZ                                           (KHZ*KHZ)

#define UTILS_DMTIMER_FREQ_INVALID                    (0)
#define UTILS_DMTIMER_TIMER0_FREQUENCY                (UTILS_DMTIMER_FREQ_INVALID)
#define UTILS_DMTIMER_TIMER1_FREQUENCY                (UTILS_DMTIMER_FREQ_INVALID)
#define UTILS_DMTIMER_TIMER2_FREQUENCY                (27*MHZ)
#define UTILS_DMTIMER_TIMER3_FREQUENCY                (UTILS_DMTIMER_FREQ_INVALID)
#define UTILS_DMTIMER_TIMER4_FREQUENCY                (UTILS_DMTIMER_FREQ_INVALID)
#define UTILS_DMTIMER_TIMER5_FREQUENCY                (UTILS_DMTIMER_FREQ_INVALID)
#define UTILS_DMTIMER_TIMER6_FREQUENCY                (UTILS_DMTIMER_FREQ_INVALID)


#endif /* UTILS_DMTIMER_REGCFG_H_ */
