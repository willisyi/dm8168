/*
* HELLOWORLDALG_TI_priv.h
*/

#ifndef HELLOWORLDALG_TI_PRIV_H_
#define HELLOWORLDALG_TI_PRIV_H_


#include <xdc/std.h>
#include <ti/xdais/xdas.h>
#include <ti/sdo/fc/dskt2/dskt2.h>

#define HELLOWORLDALG_TI_VERSIONSTRING "0.00.10.00"

#define HELLOWORLDALG_MEM_REC_INTERNAL             1
#define HELLOWORLDALG_MEM_REC_EXTERNAL             2
#define HELLOWORLDALG_MEM_REC_HELLOWORLDALGPRM     3
#define HELLOWORLDALG_MEM_REC_MAX          \
	             (HELLOWORLDALG_MEM_REC_HELLOWORLDALGPRM + 1)

#define HELLOWORLDALG_MEM_BYTES_INTERNAL   (100*1024)
#define HELLOWORLDALG_MEM_BYTES_EXTERNAL   (1*1024*1024)


/* DSP Algorith specific parameters here */
typedef struct
{
	S16	maxWidth;
    S16 maxHeight;		
	S16	maxStride;	
	S08	otherPrm1;	
	S08	otherPrm2;	
} HELLOWORLDALG_Params;


// Data structure
typedef struct HELLOWORLDALG_TI_Obj {
    IALG_Obj    				alg;            /* MUST be first field of all XDAIS algs */
    void            			*helloWorldAlg; /* Pointer to the private static structure of hello world alg */
} HELLOWORLDALG_TI_Obj;

// externs


// functions

int HELLOWORLDALG_TI_numAlloc(void);
int HELLOWORLDALG_TI_alloc(const IALG_Params *algParams, IALG_Fxns **pf, IALG_MemRec memTab[]);
int HELLOWORLDALG_TI_free(IALG_Handle handle, IALG_MemRec memTab[]);
int HELLOWORLDALG_TI_initObj(IALG_Handle handle, const IALG_MemRec memTab[], IALG_Handle parent, const IALG_Params *algParams);

#endif /* HELLOWORLDALG_TI_PRIV_H_ */

/*
*  @(#) ti.xdais.dm.examples.vidanalytics_copy; 1, 0, 0,16; 11-25-2007 20:45:08; /db/wtree/library/trees/dais-i23x/src/
*/
