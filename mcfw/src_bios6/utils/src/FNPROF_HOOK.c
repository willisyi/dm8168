#include <stdint.h>
#include <xdc/std.h>
#include <xdc/runtime/System.h>
#include <xdc/runtime/Gate.h>
#include <xdc/runtime/Diags.h>
#include <xdc/runtime/Assert.h>
#include <xdc/runtime/Timestamp.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/IHeap.h>
#include <xdc/runtime/Memory.h>
#include <xdc/runtime/Types.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Swi.h>
#include <ti/sysbios/hal/Hwi.h>
#include <ti/sysbios/heaps/HeapMem.h>
#include <ti/sysbios/BIOS.h>
#include <ti/psp/vps/vps.h>
#include <mcfw/src_bios6/utils/utils_trace.h>
typedef Void (*VoidFcnPtr)(Void);

#define FNPROF_MAX_FXN_COUNT                         (1024u)
#define FNPROF_NUM_HASH_BUCKETS                      (128)
#define FNPROF_E_ENTRYNOTFOUND                       (~0u)
#define FNPROF_FXNS_CALLCNT_THRESHOLD_DEFAULT        (200u)
#define FNPROF_FXNS_MAX_FXNSRECORDS                  (32*1024u)
#define FNPROF_MAX_FXNDEPTH                          (16)
#define FNPROF_STG2_START_THRESHOLD_COUNT            (5)

#define FNPROF_PROFILE_EXTMEM_HEAP_SIZE    (2 * 1024 * 1024)
#define FNPROF_PROFILE_INTMEM_HEAP_SIZE    (64 * 1024)

#define FNPROF_HOOK_OVERHEAD_CALIBRATE_LOOP_LENGTH   (0)


typedef enum fnProfStage_e {
  FNPROF_STAGE_FXN_COUNT_ACCUMULATE, 
  FNPROF_STAGE_FXN_PROFILE,
  FNPROF_STAGE_INACTIVE,
  FNPROF_STAGE_INVALID = -1
} fnProfStage_e;

#ifndef __TI_TMS470_V7M3__
#pragma DATA_SECTION(cProfileHeapExtMem, ".far:fnProfFileHeapExtMem");
#pragma DATA_SECTION(cProfileHeapIntMem, ".far:fnProfFileHeapIntMem");
#else
#pragma DATA_SECTION(cProfileHeapExtMem, ".bss:fnProfFileHeapExtMem");
#pragma DATA_SECTION(cProfileHeapIntMem, ".bss:fnProfFileHeapIntMem");
#endif

#pragma DATA_ALIGN(cProfileHeapExtMem, 8);
char cProfileHeapExtMem[FNPROF_PROFILE_EXTMEM_HEAP_SIZE];

#pragma DATA_ALIGN(cProfileHeapIntMem, 8);
char cProfileHeapIntMem[FNPROF_PROFILE_INTMEM_HEAP_SIZE];



struct fnProfInfoStg1_s {
  VoidFcnPtr fxnAddr;
  Int count;
};

typedef struct fnProfInfoStg1_s FNPROF_FuncInfoStg1;

struct fnProfInfoStg1Tbl_s {
  UInt numFxns;
  struct fnProfInfoStg1_s threadFxnTbl[FNPROF_MAX_FXN_COUNT];
  UInt32 activeFxns[FNPROF_MAX_FXNDEPTH];
  Int    activeFxnIndex;
  struct fnProfInfoStg1Tbl_s * next;
  struct fnProfTaskContext_s * parentTaskContext;
};

typedef struct fnProfInfoStg1Tbl_s FNPROF_TaskInfoStg1;

struct fnProfInfoStg2_s {
  Ptr fxnAddr;
  UInt32 count;
  UInt32 totalFxnCycles;
  UInt32 maxFxnCycles;
  UInt32 entryTime;
  UInt32 curFxnCycles;
  Int    curNestingLevel;
} ;

typedef struct fnProfInfoStg2_s FNPROF_FuncInfoStg2;
struct fnProfStg2Bucket_s {
  UInt16 nStartIndex;
  UInt16 nCount;
};

typedef struct fnProfStg2Bucket_s FNPROF_Stg2Bucket;

struct fnProfInfoStg2HashTbl_s {
  struct fnProfStg2Bucket_s  buckets[FNPROF_NUM_HASH_BUCKETS];
  VoidFcnPtr * fxnInfoRecordKeyTbl;
  struct fnProfInfoStg2_s * fxnInfoRecordTbl;
  struct fnProfInfoStg2HashTbl_s * next;
  struct fnProfTaskContext_s     * parentTaskContext;
  UInt32 activeFxns[FNPROF_MAX_FXNDEPTH];
  Int    activeFxnIndex;
  UInt32 fxnsCount;
  UInt32 preemptTime;
  
};

typedef struct fnProfInfoStg2HashTbl_s FNPROF_TaskInfoStg2;


struct fnProfTaskContext_s {
  struct fnProfInfoStg1Tbl_s  *stg1Ctx;
  struct fnProfInfoStg2HashTbl_s *stg2Ctx;
  fnProfStage_e eStage;
  fnProfStage_e eNextStage;
  Bool          recursionGuard;
};

struct fnProfGblInfo_s {
  IHeap_Handle  hProfileHeapIntMem;
  IHeap_Handle  hProfileHeapExtMem;  
  Int           taskHId;
  Bool          initDone;
  Bool          firstSwitchDone;
  struct fnProfInfoStg1Tbl_s     * fnInfoTblStg1Head;
  struct fnProfInfoStg2HashTbl_s * fnInfoTblStg2Head;
  struct fnProfTaskContext_s     * curTaskCtx;
  UInt32                           fxnsCallCntThreshold;
  VoidFcnPtr *                     fxnRecordKeyArrStg2;
  FNPROF_FuncInfoStg2 *            fxnInfoRecordArrStg2;
  UInt32                           maxStg2fxnsRecCnt;
}; 

extern struct fnProfGblInfo_s FNPROF_GBLINFO;

#define TIMESTAMP_PROVIDER_GENERIC_SLOW                      1
#define TIMESTAMP_PROVIDER_DUCATI_FAST                       2

#define TIMESTAMP_PROVIDER                    (TIMESTAMP_PROVIDER_GENERIC_SLOW)

#if (TIMESTAMP_PROVIDER == TIMESTAMP_PROVIDER_DUCATI_FAST)
#define TIMESTAMP_GET32     Timestamp_get32_fast_ducati_ctm 
#ifndef __TI_TMS470_V7M3__
  #error "Below asm calls work only for cortex M3"
#endif

#include <ti/sysbios/family/arm/ducati/CTM.h>
#include <ti/sysbios/family/arm/ducati/Core.h>

#pragma NO_HOOKS(Timestamp_get32_fast_ducati_ctm);
static inline UInt32  Timestamp_get32_fast_ducati_ctm (Void)
{
  volatile UInt32 hi;
  UInt32 lo;

  if (Core_id == 0) {
      hi = CTM_ctm.CTCNTR[3]; /* read hi part to update lo shadow */
      lo = CTM_ctm.CTCNTR[2]; /* return lo shadow */
  }
  else {
      hi = CTM_ctm.CTCNTR[5]; /* read hi part to update lo shadow */
      lo = CTM_ctm.CTCNTR[4]; /* return lo shadow */
  }

  return lo;

}
#else
#define TIMESTAMP_GET32      Timestamp_get32
#endif 


#define CRITICAL_BEGIN                 Hwi_disable
#define CRITICAL_END                   Hwi_restore

Void FNPROF_hookOverheadCalibrateFxn();


#pragma NO_HOOKS(_FNPROF_InitHeap);
static inline Void _FNPROF_InitHeap()
{
  HeapMem_Params heapMemPrm;

  UTILS_assert((FNPROF_GBLINFO.hProfileHeapExtMem == NULL));
  /* create memory pool heap  */

  HeapMem_Params_init(&heapMemPrm);

  heapMemPrm.buf = cProfileHeapExtMem;
  heapMemPrm.size = sizeof(cProfileHeapExtMem);

  FNPROF_GBLINFO.hProfileHeapExtMem = HeapMem_Handle_upCast(HeapMem_create(&heapMemPrm, NULL));

  UTILS_assert((FNPROF_GBLINFO.hProfileHeapExtMem != NULL));

  UTILS_assert((FNPROF_GBLINFO.hProfileHeapIntMem == NULL));
  /* create memory pool heap  */

  HeapMem_Params_init(&heapMemPrm);

  heapMemPrm.buf = cProfileHeapIntMem;
  heapMemPrm.size = sizeof(cProfileHeapIntMem);

  FNPROF_GBLINFO.hProfileHeapIntMem = HeapMem_Handle_upCast(HeapMem_create(&heapMemPrm, NULL));

  UTILS_assert((FNPROF_GBLINFO.hProfileHeapIntMem != NULL));

}

#pragma NO_HOOKS(_FNPROF_init);
static inline Void _FNPROF_init()
{
    FNPROF_GBLINFO.curTaskCtx = NULL;
    FNPROF_GBLINFO.firstSwitchDone = FALSE;
    FNPROF_GBLINFO.fnInfoTblStg1Head = NULL;
    FNPROF_GBLINFO.fnInfoTblStg2Head = NULL;
    FNPROF_GBLINFO.hProfileHeapExtMem = NULL;
    FNPROF_GBLINFO.hProfileHeapIntMem = NULL;
    FNPROF_GBLINFO.fxnsCallCntThreshold = FNPROF_FXNS_CALLCNT_THRESHOLD_DEFAULT;
    FNPROF_GBLINFO.maxStg2fxnsRecCnt    = FNPROF_FXNS_MAX_FXNSRECORDS;
    FNPROF_GBLINFO.fxnRecordKeyArrStg2  = NULL;
    FNPROF_GBLINFO.fxnInfoRecordArrStg2 = NULL;
   _FNPROF_InitHeap();
}

#pragma NO_HOOKS(_FNPROF_initTaskContext);
static inline Void _FNPROF_initTaskContext(struct fnProfTaskContext_s  *tskContext)
{
  tskContext->recursionGuard = FALSE;
  tskContext->stg1Ctx = NULL;
  tskContext->stg2Ctx = NULL;
  tskContext->eStage  = FNPROF_STAGE_INACTIVE;
  tskContext->eNextStage = FNPROF_STAGE_INVALID;
  tskContext->recursionGuard = FALSE;
}


#pragma NO_HOOKS(_FNPROF_STG1_initContext);
static inline Void _FNPROF_STG1_initContext(struct fnProfInfoStg1Tbl_s *stg1Ctx,
                                            struct fnProfTaskContext_s  *parentTskContext)
{
    IArg key;

    stg1Ctx->numFxns = 0;
    stg1Ctx->parentTaskContext = parentTskContext;
    stg1Ctx->activeFxnIndex = 0;

    key = Hwi_disable();
    stg1Ctx->next    =     FNPROF_GBLINFO.fnInfoTblStg1Head;
    FNPROF_GBLINFO.fnInfoTblStg1Head = stg1Ctx;
    Hwi_restore(key);
}


#pragma NO_HOOKS(FNPROF_BiosTaskHook_CreateFxn);
/*
 *  ======== fnProfHookCreateFxnStg1 ========
 *  Create and initialize the HookContext for this Task.
 */
Void FNPROF_BiosTaskHook_CreateFxn(Task_Handle task, Error_Block *eb)
{
    struct fnProfTaskContext_s  *context;
    
    UTILS_assert((FNPROF_GBLINFO.hProfileHeapExtMem != NULL));
    /* Create the HookContext for this Task. */
    context = Memory_calloc(FNPROF_GBLINFO.hProfileHeapExtMem, 
                            sizeof(struct fnProfTaskContext_s), 
                            8, eb);
    if (context == NULL) {
        return;
    }
    _FNPROF_initTaskContext(context);
    
    context->stg1Ctx = Memory_calloc(FNPROF_GBLINFO.hProfileHeapExtMem, 
                            sizeof(struct fnProfInfoStg1Tbl_s), 
                            8, eb);

    if (context->stg1Ctx == NULL) {
        return;
    }
    _FNPROF_STG1_initContext(context->stg1Ctx, context);
    

    Task_setHookContext(task, FNPROF_GBLINFO.taskHId, context);
}

#pragma NO_HOOKS(FNPROF_BiosTaskHook_DeleteFxn);
/*
 *  ======== fnProfHookCreateFxnStg1 ========
 *  Create and initialize the HookContext for this Task.
 */
Void FNPROF_BiosTaskHook_DeleteFxn(Task_Handle task, Error_Block *eb)
{
    struct fnProfTaskContext_s  *context;

    context = Task_getHookContext(task, FNPROF_GBLINFO.taskHId);
    
    if (context->stg2Ctx) {
      Memory_free(FNPROF_GBLINFO.hProfileHeapIntMem,
                  context->stg2Ctx,
                  sizeof(FNPROF_TaskInfoStg2));
    }
    
    Memory_free(FNPROF_GBLINFO.hProfileHeapExtMem, 
                 context->stg1Ctx,
                 sizeof(struct fnProfInfoStg1Tbl_s));


    /* Create the HookContext for this Task. */
    Memory_free(FNPROF_GBLINFO.hProfileHeapExtMem, 
                context,
                sizeof(struct fnProfTaskContext_s));
}


#pragma NO_HOOKS(FNPROF_BiosTaskHook_RegFxn);
/*
 *  ======== fnProfHookRegFxn ========
 *  The Task register hook is called once per hookset, before main and before
 *  any Task initialization has been done. 
 *  
 *  This function allows the Load module to store its hookset id, which is 
 *  passed to Task_get/setHookContext. The HookContext can be an arbitrary
 *  structure. The Load module has defined a HookContext to store thread
 *  statistics. 
 */
Void FNPROF_BiosTaskHook_RegFxn(Int id)
{
    FNPROF_GBLINFO.taskHId = id;
    if (FNPROF_GBLINFO.initDone == FALSE) {
      _FNPROF_init();
      FNPROF_GBLINFO.initDone = TRUE;
    }
}

    

#pragma NO_HOOKS(_FNPROF_STG1_initFxnProfEntry);
static inline Void _FNPROF_STG1_initFxnProfEntry(UInt32 nFxnProfIndex,VoidFcnPtr fxnAddr)
{
  struct fnProfInfoStg1_s *fnProfEntry = NULL;
  
  UTILS_assert((nFxnProfIndex < FNPROF_MAX_FXN_COUNT));
  fnProfEntry = &(FNPROF_GBLINFO.curTaskCtx->stg1Ctx->threadFxnTbl[nFxnProfIndex]);
  fnProfEntry->count = 0;
  fnProfEntry->fxnAddr = fxnAddr;
}

#pragma NO_HOOKS(_FNPROF_STG1_getFxnEntryIndex);
static inline UInt32 _FNPROF_STG1_getFxnEntryIndex (VoidFcnPtr fxnAddr)
{
  UInt32 fxnIndex = FNPROF_E_ENTRYNOTFOUND;
  Int    i;
  
  for (i = 0; i < FNPROF_GBLINFO.curTaskCtx->stg1Ctx->numFxns ; i++) {
    if (FNPROF_GBLINFO.curTaskCtx->stg1Ctx->threadFxnTbl[i].fxnAddr == fxnAddr) {
      fxnIndex = i;
      break;
    }
  }
  return fxnIndex;
}

#pragma NO_HOOKS(_FNPROF_STG1_getFreeFxnEntryIndex);
static inline UInt32 _FNPROF_STG1_getFreeFxnEntryIndex (VoidFcnPtr fxnAddr)
{
  UInt32 freeFxnEntryIndex;
  
  (Void)fxnAddr;
  if (FNPROF_GBLINFO.curTaskCtx->stg1Ctx->numFxns < FNPROF_MAX_FXN_COUNT) {
    freeFxnEntryIndex = FNPROF_GBLINFO.curTaskCtx->stg1Ctx->numFxns;
    FNPROF_GBLINFO.curTaskCtx->stg1Ctx->numFxns++;
  }
  else {
    freeFxnEntryIndex = FNPROF_E_ENTRYNOTFOUND;
  }
  return freeFxnEntryIndex;
}

#pragma NO_HOOKS(_FNPROF_STG1_fxnEntryProcess);
static inline Void _FNPROF_STG1_fxnEntryProcess(UInt32 nFxnProfIndex)
{
  struct fnProfInfoStg1_s *fnProfEntry = NULL;
  
  UTILS_assert((nFxnProfIndex < FNPROF_MAX_FXN_COUNT));
  fnProfEntry = &(FNPROF_GBLINFO.curTaskCtx->stg1Ctx->threadFxnTbl[nFxnProfIndex]);

  fnProfEntry->count++;
}

#pragma NO_HOOKS(_FNPROF_STG1_entryHook);
static inline Void _FNPROF_STG1_entryHook (VoidFcnPtr fcnAddr)
{
  if ((FNPROF_GBLINFO.curTaskCtx != NULL) &&
      (BIOS_getThreadType() == BIOS_ThreadType_Task)) {
    UInt32 fxnIndex = _FNPROF_STG1_getFxnEntryIndex(fcnAddr);
    
    if (FNPROF_E_ENTRYNOTFOUND == fxnIndex) {
      fxnIndex = _FNPROF_STG1_getFreeFxnEntryIndex((VoidFcnPtr)(fcnAddr));
      UTILS_assert ((fxnIndex != FNPROF_E_ENTRYNOTFOUND));
      _FNPROF_STG1_initFxnProfEntry(fxnIndex, (VoidFcnPtr)(fcnAddr));
    }
    FNPROF_GBLINFO.curTaskCtx->stg1Ctx->activeFxns[
        FNPROF_GBLINFO.curTaskCtx->stg1Ctx->activeFxnIndex] = fxnIndex;
      FNPROF_GBLINFO.curTaskCtx->stg1Ctx->activeFxnIndex++;
    _FNPROF_STG1_fxnEntryProcess(fxnIndex);
  } /* if (FNPROF_GBLINFO.curTaskCtx != NULL) */
}

#pragma NO_HOOKS(_FNPROF_STG1_exitHook);
static inline Void _FNPROF_STG1_exitHook(VoidFcnPtr fcnAddr)
{
  if ((FNPROF_GBLINFO.curTaskCtx != NULL) &&
      (BIOS_getThreadType() == BIOS_ThreadType_Task)) {
    UInt32 fxnIndex = _FNPROF_STG1_getFxnEntryIndex(fcnAddr);
    
    if (FNPROF_E_ENTRYNOTFOUND != fxnIndex) {
      //_FNPROF_STG1_fxnExitProcess(fxnIndex);
      FNPROF_GBLINFO.curTaskCtx->stg1Ctx->activeFxnIndex--;
      UTILS_assert ((FNPROF_GBLINFO.curTaskCtx->stg1Ctx->activeFxnIndex >= 0));
      UTILS_assert((FNPROF_GBLINFO.curTaskCtx->stg1Ctx->activeFxns[FNPROF_GBLINFO.curTaskCtx->stg1Ctx->activeFxnIndex] == fxnIndex));
    }
  }
}

#pragma NO_HOOKS(_FNPROF_STG2_TaskPrempt_Process);
static inline Void _FNPROF_STG2_TaskPrempt_Process (struct fnProfInfoStg2HashTbl_s *stg2TaskInfo,
                                                    UInt32 curTime)
{
  stg2TaskInfo->preemptTime = curTime;
}

#pragma NO_HOOKS(_FNPROF_STG2_TaskRestore_Process);
static inline Void _FNPROF_STG2_TaskRestore_Process (struct fnProfInfoStg2HashTbl_s *stg2TaskInfo,
                                                     UInt32 curTime)
{
  UInt32 deltaTime;
  Int i;
  
  if (curTime  >= stg2TaskInfo->preemptTime) {
    deltaTime = curTime  - stg2TaskInfo->preemptTime;
  }
  else {
    deltaTime = curTime  + (0xFFFFFFFF - stg2TaskInfo->preemptTime);  
  }
  for (i = 0; i < stg2TaskInfo->activeFxnIndex; i++) {
    UInt32 fxnIndex = stg2TaskInfo->activeFxns[i];
    struct fnProfInfoStg2_s * fnRecord = 
                                  &(stg2TaskInfo->fxnInfoRecordTbl[fxnIndex]);
    fnRecord->curFxnCycles += deltaTime;
    fnRecord->entryTime    += deltaTime;
  }
}



#pragma NO_HOOKS(_FNPROF_BiosTaskHook_SwitchFxn);
/*
 *  ======== loadProfileHookSwitchFxn ========
 */
static inline Void _FNPROF_BiosTaskHook_SwitchFxn(Task_Handle curTask, Task_Handle nextTask)
{
    UInt32 curTime = TIMESTAMP_GET32();

    if (FNPROF_GBLINFO.firstSwitchDone) {
      UTILS_assert((Task_getHookContext(curTask,FNPROF_GBLINFO.taskHId) ==
                     FNPROF_GBLINFO.curTaskCtx));
      if ((FNPROF_GBLINFO.curTaskCtx) && 
          (FNPROF_GBLINFO.curTaskCtx->eStage == FNPROF_STAGE_FXN_PROFILE)) {

        UTILS_assert ((FNPROF_GBLINFO.curTaskCtx->stg2Ctx != NULL));
        _FNPROF_STG2_TaskPrempt_Process(FNPROF_GBLINFO.curTaskCtx->stg2Ctx, curTime);
      }
    }
    FNPROF_GBLINFO.curTaskCtx = Task_getHookContext(nextTask,
                                                    FNPROF_GBLINFO.taskHId); 
    if ((FNPROF_GBLINFO.curTaskCtx) && 
        (FNPROF_GBLINFO.curTaskCtx->eStage == FNPROF_STAGE_FXN_PROFILE))
    {

      UTILS_assert ((FNPROF_GBLINFO.curTaskCtx->stg2Ctx != NULL));
      _FNPROF_STG2_TaskRestore_Process(FNPROF_GBLINFO.curTaskCtx->stg2Ctx, curTime);
    
    }
}

#pragma NO_HOOKS(FNPROF_BiosTaskHook_SwitchFxn);
Void FNPROF_BiosTaskHook_SwitchFxn(Task_Handle curTask, Task_Handle nextTask)
{
  _FNPROF_BiosTaskHook_SwitchFxn (curTask, nextTask);
  if (FALSE == FNPROF_GBLINFO.firstSwitchDone) {
    FNPROF_GBLINFO.firstSwitchDone = TRUE;
  }
  FNPROF_GBLINFO.curTaskCtx = Task_getHookContext(nextTask,
                                                  FNPROF_GBLINFO.taskHId); 
}

#pragma NO_HOOKS(FNPROF_BiosSwiHook_BeginSwi);
/* ======== FNPROF_BiosSwiHook_BeginSwi ========
* invoked just before Swi func is run */
Void FNPROF_BiosSwiHook_BeginSwi(Swi_Handle swi)
{
  (Void) swi;
  if (FNPROF_GBLINFO.curTaskCtx) {
    if (FNPROF_GBLINFO.curTaskCtx->eStage == FNPROF_STAGE_FXN_PROFILE) {
      UInt32 curTime = TIMESTAMP_GET32();

      _FNPROF_STG2_TaskPrempt_Process(FNPROF_GBLINFO.curTaskCtx->stg2Ctx,
                                      curTime);
    }
  }
}

#pragma NO_HOOKS(FNPROF_BiosSwiHook_EndSwi);
/* ======== FNPROF_BiosSwiHook_EndSwi ========
* invoked after Swi func returns */
Void FNPROF_BiosSwiHook_EndSwi(Swi_Handle swi)
{
  (Void) swi;
  if (FNPROF_GBLINFO.curTaskCtx) {
    if (FNPROF_GBLINFO.curTaskCtx->eStage == FNPROF_STAGE_FXN_PROFILE) {
      UInt32 curTime = TIMESTAMP_GET32();

      _FNPROF_STG2_TaskRestore_Process(FNPROF_GBLINFO.curTaskCtx->stg2Ctx,
                                      curTime);
    }
  }
}

#pragma NO_HOOKS(FNPROF_BiosHwiHook_BeginHwi);
/* ======== FNPROF_BiosHwiHook_BeginHwi ========
* invoked just before Hwi func is run */
Void FNPROF_BiosHwiHook_BeginHwi(Hwi_Handle hwi)
{
  (Void) hwi;
  if (FNPROF_GBLINFO.curTaskCtx) {
    if (FNPROF_GBLINFO.curTaskCtx->eStage == FNPROF_STAGE_FXN_PROFILE) {
      UInt32 curTime = TIMESTAMP_GET32();

      _FNPROF_STG2_TaskPrempt_Process(FNPROF_GBLINFO.curTaskCtx->stg2Ctx,
                                      curTime);
    }
  }
}

#pragma NO_HOOKS(FNPROF_BiosHwiHook_EndHwi);
/* ======== FNPROF_BiosHwiHook_EndHwi ========
* invoked after Swi func returns */
Void FNPROF_BiosHwiHook_EndHwi(Hwi_Handle hwi)
{
  (Void) hwi;
  if (FNPROF_GBLINFO.curTaskCtx) {
    if (FNPROF_GBLINFO.curTaskCtx->eStage == FNPROF_STAGE_FXN_PROFILE) {
      UInt32 curTime = TIMESTAMP_GET32();

      _FNPROF_STG2_TaskRestore_Process(FNPROF_GBLINFO.curTaskCtx->stg2Ctx,
                                      curTime);
    }
  }
}


#pragma NO_HOOKS(_FNPROF_STG2_initFxnProfEntry);
static inline Void _FNPROF_STG2_initFxnProfEntry(FNPROF_FuncInfoStg2 *funcRecStg2,
                                                 VoidFcnPtr fxnAddr)
{
  funcRecStg2->fxnAddr = fxnAddr;
  funcRecStg2->count = 0;
  funcRecStg2->totalFxnCycles = 0;
  funcRecStg2->maxFxnCycles = 0;
  funcRecStg2->entryTime = 0;
  funcRecStg2->curFxnCycles = 0;
  funcRecStg2->curNestingLevel = 0;

}


#pragma NO_HOOKS(_FNPROF_STG2_initContext);
static inline Void _FNPROF_STG2_initContext(struct fnProfInfoStg2HashTbl_s *stg2Ctx,
                                            struct fnProfTaskContext_s  *parentTskContext)
{
    Int  i;
    IArg key;

    stg2Ctx->parentTaskContext = parentTskContext;
    /* Initialize entry count in each bucket */
    for(i = 0; i < FNPROF_NUM_HASH_BUCKETS; i++) {
        stg2Ctx->buckets[i].nCount = 0;
    }
    stg2Ctx->activeFxnIndex = 0;
    stg2Ctx->preemptTime    = 0;
    stg2Ctx->fxnsCount  = 0;
    key = Hwi_disable();
    stg2Ctx->next    =     FNPROF_GBLINFO.fnInfoTblStg2Head;
    FNPROF_GBLINFO.fnInfoTblStg2Head = stg2Ctx;
    Hwi_restore(key);
}

#define FNHASH_KNUTH_GOLDEN_RATIO_NUMBER (2654435761)

#pragma NO_HOOKS(_FNPROF_hashFuncAddr);
static inline UInt32 _FNPROF_hashFuncAddr(VoidFcnPtr fxnAddr)
{
  /* discard the last bit which is always 1 in thumb mode */
  UInt32 hash = (((UInt32)fxnAddr) >> 1);
  hash       *= FNHASH_KNUTH_GOLDEN_RATIO_NUMBER;
  hash       %= FNPROF_NUM_HASH_BUCKETS;
  return (hash);
}

#pragma NO_HOOKS(_FNPROF_STG2_init);
static inline Void _FNPROF_STG2_init()
{
    FNPROF_TaskInfoStg1 *taskInfoStg1;
    FNPROF_TaskInfoStg2 *taskInfoStg2;
    uint32_t totalFxnsCntStg2, curTaskFxnsCnt, fxnsCallCntThreshold;
    uint32_t i;
    FNPROF_FuncInfoStg1 *funcRecStg1;
    FNPROF_FuncInfoStg2 *funcRecStg2;
    uint16_t hashval;

    fxnsCallCntThreshold = FNPROF_GBLINFO.fxnsCallCntThreshold;


    /* PASS 1 : Find out the number of functions that need to 
       be profiled for each thread. Add up to find out total 
       number records required during stage 2 */

    totalFxnsCntStg2 = 0;
    
    for (taskInfoStg1 = FNPROF_GBLINFO.fnInfoTblStg1Head;
        taskInfoStg1 != NULL;
        taskInfoStg1 = taskInfoStg1->next) {

        curTaskFxnsCnt = 0;
        funcRecStg1 = & (taskInfoStg1->threadFxnTbl[0]);

        for (i = 0; i < taskInfoStg1->numFxns; i++, funcRecStg1++) {
            if(funcRecStg1->count >= fxnsCallCntThreshold) 
                curTaskFxnsCnt++;
        }

        totalFxnsCntStg2 += curTaskFxnsCnt;
    }
    UTILS_assert((totalFxnsCntStg2 <= FNPROF_GBLINFO.maxStg2fxnsRecCnt));

    /* Allocate global arrays for all stage2 records */

    FNPROF_GBLINFO.fxnRecordKeyArrStg2 = 
        (VoidFcnPtr *)Memory_calloc(FNPROF_GBLINFO.hProfileHeapIntMem,
                                    sizeof(VoidFcnPtr) * totalFxnsCntStg2,
                                    8,
                                    NULL);
    UTILS_assert((FNPROF_GBLINFO.fxnRecordKeyArrStg2 != NULL));

    FNPROF_GBLINFO.fxnInfoRecordArrStg2 = 
        (FNPROF_FuncInfoStg2 *)Memory_calloc(FNPROF_GBLINFO.hProfileHeapExtMem,
                                             sizeof(FNPROF_FuncInfoStg2) * totalFxnsCntStg2,
                                             8,
                                             NULL);
    UTILS_assert((FNPROF_GBLINFO.fxnInfoRecordArrStg2 != NULL));


    /* PASS 2 :
     */
    totalFxnsCntStg2 = 0;

    for (taskInfoStg1 = FNPROF_GBLINFO.fnInfoTblStg1Head;
        taskInfoStg1 != NULL;
        taskInfoStg1 = taskInfoStg1->next) {

        VoidFcnPtr *curTaskFxnKeyStart, *curFxnKeyPtr;
        FNPROF_FuncInfoStg2 *curTaskFxnInfoRecordStart;
        int16_t curBucketIndex[FNPROF_NUM_HASH_BUCKETS];

        taskInfoStg2 = (FNPROF_TaskInfoStg2 *)Memory_calloc(FNPROF_GBLINFO.hProfileHeapIntMem,
                                                             sizeof(FNPROF_TaskInfoStg2),
                                                             8,
                                                             NULL);
        UTILS_assert(taskInfoStg2 != NULL);
        taskInfoStg1->parentTaskContext->stg2Ctx = taskInfoStg2;
        /* Initialize start of record arrays for this thread */
        taskInfoStg2->fxnInfoRecordKeyTbl = curTaskFxnKeyStart
                                          = FNPROF_GBLINFO.fxnRecordKeyArrStg2
                                                + totalFxnsCntStg2;
        taskInfoStg2->fxnInfoRecordTbl   = curTaskFxnInfoRecordStart
                                         = FNPROF_GBLINFO.fxnInfoRecordArrStg2
                                                + totalFxnsCntStg2;

        _FNPROF_STG2_initContext(taskInfoStg2, taskInfoStg1->parentTaskContext);


        curTaskFxnsCnt = 0;
        funcRecStg1 = & taskInfoStg1->threadFxnTbl[0];

        for (i = 0; i < taskInfoStg1->numFxns; i++, funcRecStg1++) {
            if(funcRecStg1->count >= fxnsCallCntThreshold) {
                curTaskFxnsCnt++;
                hashval = _FNPROF_hashFuncAddr(funcRecStg1->fxnAddr);
                taskInfoStg2->buckets[hashval].nCount++;
            }
        }

        taskInfoStg2->fxnsCount = curTaskFxnsCnt;

        /* Initialize nStartIndex in each bucket */
        taskInfoStg2->buckets[0].nStartIndex = curBucketIndex[0] = 0;

        for(i = 1; i < FNPROF_NUM_HASH_BUCKETS; i++) {
            taskInfoStg2->buckets[i].nStartIndex = curBucketIndex[i] =
                taskInfoStg2->buckets[i-1].nStartIndex + taskInfoStg2->buckets[i-1].nCount;
        }

        /* Populate function records for stage 2 */
        funcRecStg1 = & taskInfoStg1->threadFxnTbl[0];

        for (i = 0; i < taskInfoStg1->numFxns; i++, funcRecStg1++) {
            if(funcRecStg1->count >= fxnsCallCntThreshold) {

                VoidFcnPtr fxnAddr = funcRecStg1->fxnAddr;
                int16_t curFxnRecIndex;
                
                /* Get record index for the function */
                hashval = _FNPROF_hashFuncAddr(fxnAddr);
                curFxnRecIndex = curBucketIndex[hashval];
                curBucketIndex[hashval]++;

                /* Get record addresses for the function */
                funcRecStg2 = curTaskFxnInfoRecordStart + curFxnRecIndex;
                curFxnKeyPtr = curTaskFxnKeyStart + curFxnRecIndex;
                
                /* Initialize the records */
                *curFxnKeyPtr = fxnAddr;
                _FNPROF_STG2_initFxnProfEntry(funcRecStg2,fxnAddr);
            }
        }

        totalFxnsCntStg2 += curTaskFxnsCnt;
    }
}

#pragma NO_HOOKS(_FNPROF_STG2_getFxnEntryIndex);
static inline UInt32 _FNPROF_STG2_getFxnEntryIndex(VoidFcnPtr fxnAddr)
{
  UInt32 fxnIndex = FNPROF_E_ENTRYNOTFOUND;
  UInt32 hashval;
  FNPROF_Stg2Bucket *bucket;
  UInt32 startIdxInBucket, nCountInBucket;
  Int i;
  VoidFcnPtr *curFxnAddr;
  FNPROF_TaskInfoStg2 *taskContext = FNPROF_GBLINFO.curTaskCtx->stg2Ctx;
  
  hashval = _FNPROF_hashFuncAddr(fxnAddr);

  bucket = & (taskContext->buckets[hashval]);

  startIdxInBucket = bucket->nStartIndex;
  nCountInBucket   = bucket->nCount;

  for (i = 0, curFxnAddr = &(taskContext->fxnInfoRecordKeyTbl[startIdxInBucket]); 
       i < nCountInBucket; 
       i++, curFxnAddr++) {
    if (*curFxnAddr == fxnAddr) {
      fxnIndex = startIdxInBucket + i;
      return (fxnIndex);
    }
  }
  return fxnIndex;
}

#pragma NO_HOOKS(_FNPROF_STG2_fxnEntryProcess);
static inline Void _FNPROF_STG2_fxnEntryProcess(UInt32 nFxnProfIndex)
{
  struct fnProfInfoStg2_s *fnProfEntry = NULL;
  
  UTILS_assert((nFxnProfIndex < FNPROF_GBLINFO.curTaskCtx->stg2Ctx->fxnsCount));
  fnProfEntry = &(FNPROF_GBLINFO.curTaskCtx->stg2Ctx->fxnInfoRecordTbl[nFxnProfIndex]);
  if(fnProfEntry->curNestingLevel == 0)
  {
      fnProfEntry->curNestingLevel++;
      fnProfEntry->curFxnCycles = 0;
      fnProfEntry->entryTime = TIMESTAMP_GET32();
  }
}

#pragma NO_HOOKS(_FNPROF_STG2_entryHook);
static inline Void _FNPROF_STG2_entryHook (VoidFcnPtr fcnAddr)
{
  if ((FNPROF_GBLINFO.curTaskCtx != NULL) &&
      (BIOS_getThreadType() == BIOS_ThreadType_Task)) {
    UInt32 fxnIndex = _FNPROF_STG2_getFxnEntryIndex(fcnAddr);
    
    if (FNPROF_E_ENTRYNOTFOUND != fxnIndex) {
      UTILS_assert(((FNPROF_GBLINFO.curTaskCtx->stg2Ctx != NULL) &&
                     (FNPROF_GBLINFO.curTaskCtx->stg2Ctx->activeFxnIndex < FNPROF_MAX_FXNDEPTH)));
      UTILS_assert((FNPROF_GBLINFO.curTaskCtx->stg2Ctx->fxnInfoRecordTbl[fxnIndex].fxnAddr ==
                    fcnAddr));
      FNPROF_GBLINFO.curTaskCtx->stg2Ctx->activeFxns[
        FNPROF_GBLINFO.curTaskCtx->stg2Ctx->activeFxnIndex] = fxnIndex;
      FNPROF_GBLINFO.curTaskCtx->stg2Ctx->activeFxnIndex++;
      _FNPROF_STG2_fxnEntryProcess(fxnIndex);
    }

  } /* if (FNPROF_GBLINFO.curTaskCtx != NULL) */
}

#pragma NO_HOOKS(_FNPROF_STG2_fxnExitProcess);
static inline Void _FNPROF_STG2_fxnExitProcess(UInt32 nFxnProfIndex)
{
  struct fnProfInfoStg2_s *fnProfEntry = NULL;
  UInt32 nCurTime,nDeltaTime;
  
  UTILS_assert((nFxnProfIndex < FNPROF_GBLINFO.curTaskCtx->stg2Ctx->fxnsCount));

  fnProfEntry = &(FNPROF_GBLINFO.curTaskCtx->stg2Ctx->fxnInfoRecordTbl[nFxnProfIndex]);
  if (fnProfEntry->curNestingLevel == 1)
  {
      if (fnProfEntry->count > FNPROF_STG2_START_THRESHOLD_COUNT) {
        nCurTime = TIMESTAMP_GET32();
        if (nCurTime >= fnProfEntry->entryTime) {
          nDeltaTime = nCurTime - fnProfEntry->entryTime;
          if (fnProfEntry->maxFxnCycles < nDeltaTime) {
            fnProfEntry->maxFxnCycles = nDeltaTime;
          }
          fnProfEntry->totalFxnCycles += nDeltaTime;
          fnProfEntry->count++;
        }
      }
      else {
        fnProfEntry->count++;
      }
  }
  fnProfEntry->curNestingLevel--;
}

#pragma NO_HOOKS(_FNPROF_STG2_exitHook);
static inline Void _FNPROF_STG2_exitHook(VoidFcnPtr fcnAddr)
{
  if ((FNPROF_GBLINFO.curTaskCtx != NULL) &&
      (BIOS_getThreadType() == BIOS_ThreadType_Task)) {
    UInt32 fxnIndex = _FNPROF_STG2_getFxnEntryIndex(fcnAddr);
    
    if (FNPROF_E_ENTRYNOTFOUND != fxnIndex) {
      _FNPROF_STG2_fxnExitProcess(fxnIndex);
      FNPROF_GBLINFO.curTaskCtx->stg2Ctx->activeFxnIndex--;
      UTILS_assert ((FNPROF_GBLINFO.curTaskCtx->stg2Ctx->activeFxnIndex >= 0));
      UTILS_assert((FNPROF_GBLINFO.curTaskCtx->stg2Ctx->activeFxns[FNPROF_GBLINFO.curTaskCtx->stg2Ctx->activeFxnIndex] == fxnIndex));
    }
  }
}


#pragma NO_HOOKS(_FNPROF_STG1_printProfileInfo);
static inline Void _FNPROF_STG1_printProfileInfo(Void)
{
  FNPROF_TaskInfoStg1 *taskInfoStg1;

  Vps_printf("\r\n FXNADDR #  COUNT  ");
  for (taskInfoStg1 = FNPROF_GBLINFO.fnInfoTblStg1Head;
      taskInfoStg1 != NULL;
      taskInfoStg1 = taskInfoStg1->next) {
    Int i;
    
    for (i = 0; i < taskInfoStg1->numFxns; i++) {
      Vps_printf("\r\n %08x#% 10u",
                    taskInfoStg1->threadFxnTbl[i].fxnAddr,
                    taskInfoStg1->threadFxnTbl[i].count);
    }
  }
  Vps_printf("\n");
}

#pragma NO_HOOKS(_FNPROF_STG2_printProfileInfo);
static inline Void _FNPROF_STG2_printProfileInfo(Void)
{
  FNPROF_TaskInfoStg2 *taskInfoStg2;
  Types_FreqHz freqTs;
  Types_FreqHz freqCpu;
  UInt32 fxnHookOverhead = 0;

  Timestamp_getFreq(&freqTs);
  UTILS_assert(freqTs.hi == 0);
  BIOS_getCpuFreq(&freqCpu);
  UTILS_assert(freqCpu.hi == 0);

  if (FNPROF_GBLINFO.curTaskCtx != NULL)
  {
    UInt32 fxnIndex = _FNPROF_STG2_getFxnEntryIndex(&FNPROF_hookOverheadCalibrateFxn);

    UTILS_assert(FNPROF_E_ENTRYNOTFOUND != fxnIndex);
    UTILS_assert(((FNPROF_GBLINFO.curTaskCtx->stg2Ctx != NULL) &&
                   (FNPROF_GBLINFO.curTaskCtx->stg2Ctx->activeFxnIndex < FNPROF_MAX_FXNDEPTH)));
    UTILS_assert((FNPROF_GBLINFO.curTaskCtx->stg2Ctx->fxnInfoRecordTbl[fxnIndex].fxnAddr ==
                  &FNPROF_hookOverheadCalibrateFxn));
    fxnHookOverhead = FNPROF_GBLINFO.curTaskCtx->stg2Ctx->fxnInfoRecordTbl[fxnIndex].totalFxnCycles /
                      (FNPROF_GBLINFO.curTaskCtx->stg2Ctx->fxnInfoRecordTbl[fxnIndex].count -
                       FNPROF_STG2_START_THRESHOLD_COUNT);
    UTILS_assert(fxnHookOverhead >= ((FNPROF_HOOK_OVERHEAD_CALIBRATE_LOOP_LENGTH * freqTs.lo)/freqCpu.lo));
    fxnHookOverhead -= ((FNPROF_HOOK_OVERHEAD_CALIBRATE_LOOP_LENGTH * freqTs.lo)/freqCpu.lo);
  }
  Vps_printf("\r\n  FXNHOOKOVERHEAD:%u",fxnHookOverhead);
  Vps_printf("\r\n  CPU_FREQ:%u",freqCpu.lo);
  Vps_printf("\r\n  TIMER_FREQ:%u",freqTs.lo);
  Vps_printf("\r\n  FXNADDR #COUNT#MAXCYCLES#TOTALCYCLES\n");
  for (taskInfoStg2 = FNPROF_GBLINFO.fnInfoTblStg2Head;
      taskInfoStg2 != NULL;
      taskInfoStg2 = taskInfoStg2->next) {
    Int i,j;
    
    for (i = 0; i < FNPROF_NUM_HASH_BUCKETS; i++) {
      UInt32 nStartIndex = taskInfoStg2->buckets[i].nStartIndex;

      for (j = 0; j < taskInfoStg2->buckets[i].nCount;j++) {
        struct fnProfInfoStg2_s *fxnStg2Info = &(taskInfoStg2->fxnInfoRecordTbl[nStartIndex + j]);
        UInt32 maxFxnCycles = fxnStg2Info->maxFxnCycles;
        UInt32 totalFxnCycles = fxnStg2Info->totalFxnCycles;
        UInt32 count = ((fxnStg2Info->count >= FNPROF_STG2_START_THRESHOLD_COUNT) ?
                        (fxnStg2Info->count - FNPROF_STG2_START_THRESHOLD_COUNT)  :
                        fxnStg2Info->count);
        //UInt32 totalFxnHookOverhead = fxnHookOverhead * count;

        //maxFxnCycles = (maxFxnCycles > fxnHookOverhead) ?
        //               (maxFxnCycles - fxnHookOverhead) : 0;

        //totalFxnCycles = (totalFxnCycles > totalFxnHookOverhead) ?
        //                 (totalFxnCycles - totalFxnHookOverhead) : 0;
        if ((count) && fxnStg2Info->fxnAddr)
        {
            Vps_printf("\r\n %08x#% 5u#% 9u#% 11u",
                        fxnStg2Info->fxnAddr,
                        count,
                        maxFxnCycles,
                        totalFxnCycles);
        }
      }
    }
  }
}


#pragma NO_HOOKS(FNPROF_entryHook);
Void FNPROF_entryHook(VoidFcnPtr fcnAddr)
{
  IArg key;
  
  if ((FNPROF_GBLINFO.curTaskCtx == NULL) 
      || 
      ((FNPROF_GBLINFO.curTaskCtx->eStage == FNPROF_STAGE_INACTIVE) &&
	   (FNPROF_GBLINFO.curTaskCtx->eNextStage == FNPROF_STAGE_INVALID))
	  ||
      (FNPROF_GBLINFO.curTaskCtx->recursionGuard == TRUE ) ) {
    return;
  }
  FNPROF_GBLINFO.curTaskCtx->recursionGuard = TRUE;
  key = CRITICAL_BEGIN();
  
  if (!FNPROF_GBLINFO.initDone) {
    _FNPROF_init();
  }
  if (FNPROF_GBLINFO.curTaskCtx->eNextStage != FNPROF_STAGE_INVALID) {
    Int curNestingLevel = 0;
    
    if(FNPROF_GBLINFO.curTaskCtx->eStage == FNPROF_STAGE_FXN_COUNT_ACCUMULATE) {
      curNestingLevel = FNPROF_GBLINFO.curTaskCtx->stg1Ctx->activeFxnIndex;
    }
    if(FNPROF_GBLINFO.curTaskCtx->eStage == FNPROF_STAGE_FXN_PROFILE) {
      curNestingLevel = FNPROF_GBLINFO.curTaskCtx->stg2Ctx->activeFxnIndex;
    }
    if (curNestingLevel == 0) {
      FNPROF_GBLINFO.curTaskCtx->eStage = FNPROF_GBLINFO.curTaskCtx->eNextStage;
      FNPROF_GBLINFO.curTaskCtx->eNextStage = FNPROF_STAGE_INVALID;
    }
  }
  if(FNPROF_GBLINFO.curTaskCtx->eStage == FNPROF_STAGE_FXN_COUNT_ACCUMULATE) {
    _FNPROF_STG1_entryHook(fcnAddr);
  }
  if(FNPROF_GBLINFO.curTaskCtx->eStage == FNPROF_STAGE_FXN_PROFILE) {
    _FNPROF_STG2_entryHook(fcnAddr);
  }
  CRITICAL_END(key);
  FNPROF_GBLINFO.curTaskCtx->recursionGuard = FALSE;
}

#pragma NO_HOOKS(FNPROF_exitHook);
Void FNPROF_exitHook(VoidFcnPtr fcnAddr)
{
  IArg key;

  if ((FNPROF_GBLINFO.curTaskCtx == NULL) || 
      (FNPROF_GBLINFO.curTaskCtx->eStage == FNPROF_STAGE_INACTIVE) ||
      (FNPROF_GBLINFO.curTaskCtx->recursionGuard == TRUE ) ) {
    return;
  }

  FNPROF_GBLINFO.curTaskCtx->recursionGuard = TRUE;
  key = CRITICAL_BEGIN();
  UTILS_assert((TRUE == FNPROF_GBLINFO.initDone));
  if(FNPROF_GBLINFO.curTaskCtx->eStage ==  FNPROF_STAGE_FXN_COUNT_ACCUMULATE) {
    _FNPROF_STG1_exitHook(fcnAddr);
  }
  if(FNPROF_GBLINFO.curTaskCtx->eStage ==  FNPROF_STAGE_FXN_PROFILE) {
    _FNPROF_STG2_exitHook(fcnAddr);
  }
  CRITICAL_END(key);
  FNPROF_GBLINFO.curTaskCtx->recursionGuard = FALSE;
}

#pragma NO_HOOKS(_FNPROF_STG1_enableProfile)
static inline Void _FNPROF_STG1_enableProfile()
{
  FNPROF_TaskInfoStg1 *taskInfoStg1;

  for (taskInfoStg1 = FNPROF_GBLINFO.fnInfoTblStg1Head;
      taskInfoStg1 != NULL;
      taskInfoStg1 = taskInfoStg1->next) {
    taskInfoStg1->parentTaskContext->eNextStage = 
                                             FNPROF_STAGE_FXN_COUNT_ACCUMULATE;
  }
}

#pragma NO_HOOKS(_FNPROF_STG2_enableProfile)
static inline Void _FNPROF_STG2_enableProfile()
{
  FNPROF_TaskInfoStg2 *taskInfoStg2;

  for (taskInfoStg2 = FNPROF_GBLINFO.fnInfoTblStg2Head;
      taskInfoStg2 != NULL;
      taskInfoStg2 = taskInfoStg2->next) {
    taskInfoStg2->parentTaskContext->eNextStage = FNPROF_STAGE_FXN_PROFILE;
  }
}

#pragma NO_HOOKS(FNPROF_STG1_enableProfile)
Void FNPROF_STG1_enableProfile()
{
  IArg key;
  
  key = Hwi_disable();

  _FNPROF_STG1_enableProfile();

  Hwi_restore(key);
}

#pragma NO_HOOKS(FNPROF_printProfileInfo)
Void FNPROF_STG2_setFxnCallCntThreshold(UInt32 fxnCallCntThreshold)
{
  IArg key;

  key = Hwi_disable();

  FNPROF_GBLINFO.fxnsCallCntThreshold = fxnCallCntThreshold;

  Hwi_restore(key);
}


#pragma NO_HOOKS(FNPROF_STG2_enableProfile)
Void FNPROF_STG2_enableProfile()
{
  IArg key;
  
  key = Hwi_disable();
  
  _FNPROF_STG2_init();
  _FNPROF_STG2_enableProfile();

  Hwi_restore(key);
}

#pragma NO_HOOKS(FNPROF_printProfileInfo)
Void FNPROF_printProfileInfo()
{
  IArg key;

  key = Hwi_disable();

  _FNPROF_STG1_printProfileInfo();
  _FNPROF_STG2_printProfileInfo();

  Hwi_restore(key);

}

#pragma NO_HOOKS(FNPROF_disableProfiling)
Void FNPROF_disableProfiling()
{
  FNPROF_TaskInfoStg1 *taskInfoStg1;

  for (taskInfoStg1 = FNPROF_GBLINFO.fnInfoTblStg1Head;
      taskInfoStg1 != NULL;
      taskInfoStg1 = taskInfoStg1->next)
  {
    taskInfoStg1->parentTaskContext->eNextStage =
                                             FNPROF_STAGE_INACTIVE;
  }
}

#include <ti/sysbios/timers/dmtimer/Timer.h>
#pragma FUNC_EXT_CALLED(FNPROF_hookOverheadCalibrateFxn)
#pragma NO_HOOKS(FNPROF_hookOverheadCalibrateFxn)
Void FNPROF_hookOverheadCalibrateFxn()
{
    FNPROF_entryHook(&FNPROF_hookOverheadCalibrateFxn);
    #if (FNPROF_HOOK_OVERHEAD_CALIBRATE_LOOP_LENGTH)
    ti_sysbios_timers_dmtimer_Timer_spinLoop__I(FNPROF_HOOK_OVERHEAD_CALIBRATE_LOOP_LENGTH);
    #endif
    FNPROF_exitHook(&FNPROF_hookOverheadCalibrateFxn);
}


struct fnProfGblInfo_s FNPROF_GBLINFO = 
{
  .hProfileHeapIntMem    = NULL,
  .hProfileHeapExtMem    = NULL,
  .taskHId               = 0,
  .initDone              = FALSE,
  .firstSwitchDone       = FALSE,
  .fnInfoTblStg1Head     = NULL,
  .fnInfoTblStg2Head     = NULL,
  .curTaskCtx            = NULL,
  .fxnsCallCntThreshold  = FNPROF_FXNS_CALLCNT_THRESHOLD_DEFAULT,
  .maxStg2fxnsRecCnt     = FNPROF_FXNS_MAX_FXNSRECORDS,
};
