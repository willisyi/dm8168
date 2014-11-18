/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/


/*!
 *  @file       trace.c
 *
 *  @brief      Trace implementation.
 *
 *              This abstracts and implements the definitions for
 *              user side traces statements and also details
 *              of variable traces supported in existing
 *              implementation.
 *
 *  @date       15 Apr, 2009
 *
 *  @internal   15 Apr, 2009, Mugdha Kamoolkar, Revision 0001:
 *              [1] Original version.
 */


/* Trace header file */
#include "vps_types.h"
#include "trace.h"


#if defined (__cplusplus)
extern "C" {
#endif


/*!
 *  @brief      Global trace flag.
 */
Int32 curTrace = 0;


/*!
 *  @brief      Function to log the trace with zero parameters and just
 *              information string.
 *  @param      mask type of traces.
 *  @param      classtype One of three classes where this trace need
 *              to be enabed.
 *  @param      fileName    Where the condition has occured.
 *  @param      lineNo      Line number of the current file where this failure
 *                          has occured.
 *  @param      The debug string.
 */
Void
_GT_0trace (UInt32         mask,
            GT_TraceClass  classtype,
            Char *         fileName,
            UInt32         lineNo,
            Char *         infoString)
{
    /* Check if trace is enabled. */
    if (    ((mask & GT_TRACESTATE_MASK) >> GT_TRACESTATE_SHIFT)
        ==  GT_TraceState_Enable) {
        if ((classtype == GT_ENTER) || (classtype == GT_LEAVE)) {
            if ((mask & GT_TRACEENTER_MASK) == GT_TraceEnter_Enable) {
                Osal_printf (infoString);
            }
        }
        else {
            /* Check if specified class is enabled. */
            if ((mask & GT_TRACECLASS_MASK) >= classtype) {
                /* Print if specified class is greater than or equal to class
                 * for this specific print.
                 */
                if (!((classtype == GT_INFO) || (classtype == GT_INFO1)))
                {
                    Osal_printf ("%s @ Line %d: ", fileName, lineNo);
                }
                Osal_printf (infoString);
            }
        }
    }
}


/*!
 *  @brief      Function to log the trace with one additional parameter
 *  @param      mask type of traces
 *  @param      classtype One of three classes where this trace
 *              need to be enabed.
 *  @param      fileName    Where the condition has occured.
 *  @param      lineNo      Line number of the current file where this failure
 *                          has occured.
 *  @param      The debug string.
 *  @param      param The additional parameter which needs to be logged.
 */
Void
_GT_1trace (UInt32         mask,
            GT_TraceClass  classtype,
            Char *         fileName,
            UInt32         lineNo,
            Char *         infoString,
            UInt32         param)
{
    /* Check if trace is enabled. */
    if (    ((mask & GT_TRACESTATE_MASK) >> GT_TRACESTATE_SHIFT)
        ==  GT_TraceState_Enable) {
        if ((classtype == GT_ENTER) || (classtype == GT_LEAVE)) {
            if ((mask & GT_TRACEENTER_MASK) == GT_TraceEnter_Enable) {
                Osal_printf (infoString, param);
            }
        }
        else {
            /* Check if specified class is enabled. */
            if ((mask & GT_TRACECLASS_MASK) >= classtype) {
                /* Print if specified class is greater than or equal to class
                 * for this specific print.
                 */
                if (!((classtype == GT_INFO) || (classtype == GT_INFO1)))
                {
                    Osal_printf ("%s @ Line %d: ", fileName, lineNo);
                }
                Osal_printf (infoString, param);
            }
        }
    }
}


/*!
 *  @brief      Function to log the trace with two additional parameters
 *  @param      mask type of traces
 *  @param      classtype One of three classes where this trace
 *              need to be enabed.
 *  @param      fileName    Where the condition has occured.
 *  @param      lineNo      Line number of the current file where this failure
 *                          has occured.
 *  @param      The debug string.
 *  @param      param0 The first parameter which needs to be logged.
 *  @param      param1 The second parameter which needs to be logged.
 */
Void
_GT_2trace (UInt32         mask,
            GT_TraceClass  classtype,
            Char *         fileName,
            UInt32         lineNo,
            Char *         infoString,
            UInt32         param0,
            UInt32         param1)
{
    /* Check if trace is enabled. */
    if (    ((mask & GT_TRACESTATE_MASK) >> GT_TRACESTATE_SHIFT)
        ==  GT_TraceState_Enable) {
        if ((classtype == GT_ENTER) || (classtype == GT_LEAVE)) {
            if ((mask & GT_TRACEENTER_MASK) == GT_TraceEnter_Enable) {
                Osal_printf (infoString, param0, param1);
            }
        }
        else {
            /* Check if specified class is enabled. */
            if ((mask & GT_TRACECLASS_MASK) >= classtype) {
                /* Print if specified class is greater than or equal to class
                 * for this specific print.
                 */
                if (!((classtype == GT_INFO) || (classtype == GT_INFO1)))
                {
                    Osal_printf ("%s @ Line %d: ", fileName, lineNo);
                }
                Osal_printf (infoString, param0, param1);
            }
        }
    }
}


/*!
 *  @brief      Function to log the trace with three parameters.
 *  @param      mask type of traces
 *  @param      classtype One of three classes where this trace
 *              need to be enabed.
 *  @param      fileName    Where the condition has occured.
 *  @param      lineNo      Line number of the current file where this failure
 *                          has occured.
 *  @param      The debug string.
 *  @param      param0 The first parameter which needs to be logged.
 *  @param      param1 The second parameter which needs to be logged.
 *  @param      param2 The third parameter which needs to be logged.
 */
Void
_GT_3trace (UInt32         mask,
            GT_TraceClass  classtype,
            Char *         fileName,
            UInt32         lineNo,
            Char*          infoString,
            UInt32         param0,
            UInt32         param1,
            UInt32         param2)
{
    /* Check if trace is enabled. */
    if (    ((mask & GT_TRACESTATE_MASK) >> GT_TRACESTATE_SHIFT)
        ==  GT_TraceState_Enable) {
        if ((classtype == GT_ENTER) || (classtype == GT_LEAVE)) {
            if ((mask & GT_TRACEENTER_MASK) == GT_TraceEnter_Enable) {
                Osal_printf (infoString, param0, param1, param2);
            }
        }
        else {
            /* Check if specified class is enabled. */
            if ((mask & GT_TRACECLASS_MASK) >= classtype) {
                /* Print if specified class is greater than or equal to class
                 * for this specific print.
                 */
                if (!((classtype == GT_INFO) || (classtype == GT_INFO1)))
                {
                    Osal_printf ("%s @ Line %d: ", fileName, lineNo);
                }
                Osal_printf (infoString, param0, param1, param2);
            }
        }
    }
}


/*!
 *  @brief      Function to log the trace with four parameters.
 *  @param      mask type of traces
 *  @param      classtype One of three classes where this trace
 *              need to be enabed.
 *  @param      fileName    Where the condition has occured.
 *  @param      lineNo      Line number of the current file where this failure
 *                          has occured.
 *  @param      The debug string.
 *  @param      param0 The first parameter which needs to be logged.
 *  @param      param1 The second parameter which needs to be logged.
 *  @param      param2 The third parameter which needs to be logged.
 *  @param      param3 The fourth parameter which needs to be logged.
 */
Void
_GT_4trace (UInt32         mask,
            GT_TraceClass  classtype,
            Char *         fileName,
            UInt32         lineNo,
            Char*          infoString,
            UInt32         param0,
            UInt32         param1,
            UInt32         param2,
            UInt32         param3)
{
    /* Check if trace is enabled. */
    if (    ((mask & GT_TRACESTATE_MASK) >> GT_TRACESTATE_SHIFT)
        ==  GT_TraceState_Enable) {
        if ((classtype == GT_ENTER) || (classtype == GT_LEAVE)) {
            if ((mask & GT_TRACEENTER_MASK) == GT_TraceEnter_Enable) {
                Osal_printf (infoString, param0, param1, param2, param3);
            }
        }
        else {
            /* Check if specified class is enabled. */
            if ((mask & GT_TRACECLASS_MASK) >= classtype) {
                /* Print if specified class is greater than or equal to class
                 * for this specific print.
                 */
                if (!((classtype == GT_INFO) || (classtype == GT_INFO1)))
                {
                    Osal_printf ("%s @ Line %d: ", fileName, lineNo);
                }
                Osal_printf (infoString, param0, param1, param2, param3);
            }
        }
    }
}


/*!
 *  @brief      Function to log the trace with five parameters.
 *  @param      mask type of traces
 *  @param      classtype One of three classes where this trace
 *              need to be enabed.
 *  @param      fileName    Where the condition has occured.
 *  @param      lineNo      Line number of the current file where this failure
 *                          has occured.
 *  @param      The debug string.
 *  @param      param0 The first parameter which needs to be logged.
 *  @param      param1 The second parameter which needs to be logged.
 *  @param      param2 The third parameter which needs to be logged.
 *  @param      param3 The fourth parameter which needs to be logged.
 *  @param      param4 The fifth parameter which needs to be logged.
 */
Void
_GT_5trace (UInt32         mask,
            GT_TraceClass  classtype,
            Char *         fileName,
            UInt32         lineNo,
            Char*          infoString,
            UInt32         param0,
            UInt32         param1,
            UInt32         param2,
            UInt32         param3,
            UInt32         param4)
{
    /* Check if trace is enabled. */
    if (    ((mask & GT_TRACESTATE_MASK) >> GT_TRACESTATE_SHIFT)
        ==  GT_TraceState_Enable) {
        if ((classtype == GT_ENTER) || (classtype == GT_LEAVE)) {
            if ((mask & GT_TRACEENTER_MASK) == GT_TraceEnter_Enable) {
                Osal_printf (infoString,
                             param0,
                             param1,
                             param2,
                             param3,
                             param4);
            }
        }
        else {
            /* Check if specified class is enabled. */
            if ((mask & GT_TRACECLASS_MASK) >= classtype) {
                /* Print if specified class is greater than or equal to class
                 * for this specific print.
                 */
                if (!((classtype == GT_INFO) || (classtype == GT_INFO1)))
                {
                    Osal_printf ("%s @ Line %d: ", fileName, lineNo);
                }
                Osal_printf (infoString,
                             param0,
                             param1,
                             param2,
                             param3,
                             param4);
            }
        }
    }
}


/*!
 *  @brief      Function to report the vps failure and log the trace. This
 *              is mostly the fatal error and system can not recover without
 *              module restart.
 *  @param      mask        Indicates whether SetFailure is enabled.
 *  @param      func        Name of the function where this oc.cured
 *  @param      fileName    Where the condition has occured.
 *  @param      lineNo      Line number of the current file where this failure
 *                          has occured.
 *  @param      status      What was the code we got/set for this failure
 *  @param      msg         Any additional information which can be useful for
 *                          deciphering the error condition.
 */
Void _GT_setFailureReason (Int    mask,
                           Char * func,
                           Char * fileName,
                           UInt32 lineNo,
                           UInt32 status,
                           Char * msg)
{
    if (    ((mask & GT_TRACESETFAILURE_MASK) >> GT_TRACESETFAILURE_SHIFT)
        ==  GT_TraceState_Enable) {
        Osal_printf ("*** %s: %s\tError [0x%x] at Line no: %d in file %s\n",
                     func,
                     msg,
                     status,
                     lineNo,
                     fileName);
    }
}


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */

