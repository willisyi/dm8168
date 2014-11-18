/** ==================================================================
 *  @file   iss_init.h                                                  
 *                                                                    
 *  @path   /ti/psp/iss/drivers/                                                  
 *                                                                    
 *  @desc   This  File contains.                                      
 * ===================================================================
 *  Copyright (c) Texas Instruments Inc 2011, 2012                    
 *                                                                    
 *  Use of this software is controlled by the terms and conditions found
 *  in the license agreement under which this software has been supplied
 * ===================================================================*/

/**
 * \file iss_init.h
 *
 * \brief ISS init  file
 * This file exposes init api's which initializes and de-initializes Hals,
 * Cores and other modules
 * drivers.
 *
 */

#ifndef _ISS_INIT_H
#define _ISS_INIT_H

/* ========================================================================== 
 */
/* Include Files */
/* ========================================================================== 
 */

/* None */

#ifdef __cplusplus
extern "C" {
#endif

    /* ========================================================================== 
     */
    /* Macros & Typedefs */
    /* ========================================================================== 
     */

    /* None */

    /* ========================================================================== 
     */
    /* Structure Declarations */
    /* ========================================================================== 
     */

    /* None */

    /* ========================================================================== 
     */
    /* Function Declarations */
    /* ========================================================================== 
     */

/**
 *  Iss_init
 *  \brief Function to initialize all ISS modules.
 *
 *  \param arg        Currently not used. For the future.
 *
 *  \return           Returns 0 on success else returns error value
 */
    Int32 Iss_init(Ptr arg);

/**
 *  Iss_deInit
 *  \brief Function to de-initialize all ISS modules.
 *
 *  \param arg        Currently not used. For the future.
 *
 *  \return           Returns 0 on success else returns error value
 */
    Int32 Iss_deInit(Ptr arg);

#ifdef __cplusplus
}
#endif
#endif                                                     /* End of #ifndef
                                                            * _ISS_INIT_H */
