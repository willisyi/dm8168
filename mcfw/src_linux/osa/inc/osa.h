

#ifndef _OSA_H_
#define _OSA_H_

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <mcfw/interfaces/ti_media_std.h>

#define OSA_DEBUG_MODE // enable OSA_printf, OSA_assert
#define OSA_DEBUG_FILE // enable printf's during OSA_fileXxxx
#define OSA_PRF_ENABLE // enable profiling APIs

#define OSA_SOK      0  ///< Status : OK
#define OSA_EFAIL   -1  ///< Status : Generic error

#ifndef _TI_STD_TYPES
#define _TI_STD_TYPES

/* unsigned quantities */
typedef unsigned long long Uint64;      ///< Unsigned 64-bit integer
typedef unsigned int Uint32;            ///< Unsigned 32-bit integer
typedef unsigned short Uint16;          ///< Unsigned 16-bit integer
typedef unsigned char Uint8;            ///< Unsigned  8-bit integer

/* signed quantities */
typedef long long Int64;               ///< Signed 64-bit integer

typedef unsigned int Uns;              ///< Unsigned int

typedef   void *  OSA_PTR;

#define OSA_SUSPEND     (0xFFFFFFFF)
#define OSA_NO_SUSPEND  (0)


#endif /* _TI_STD_TYPES */

#ifndef KB
#define KB ((Uint32)1024)
#endif

#ifndef MB
#define MB (KB*KB)
#endif

#define OSA_TIMEOUT_NONE        ((Uint32) 0)  // no timeout
#define OSA_TIMEOUT_FOREVER     ((Uint32)-1)  // wait forever

#define OSA_memAlloc(size)      (void*)malloc((size))
#define OSA_memFree(ptr)        free(ptr)

#define OSA_align(value, align)   ((( (value) + ( (align) - 1 ) ) / (align) ) * (align) )

#define OSA_floor(value, align)   (( (value) / (align) ) * (align) )
#define OSA_ceil(value, align)    OSA_align(value, align)

#define OSA_SNPRINTF(sbuf,...)                                               \
                                do {                                           \
                                    snprintf (sbuf, sizeof(sbuf) - 1,          \
                                              __VA_ARGS__);                    \
                                    sbuf[sizeof(sbuf) - 1] = 0;                \
                                } while (0)

#define OSA_ARRAYSIZE(array)             ((sizeof(array)/sizeof((array)[0])))

#define OSA_ARRAYINDEX(elem,array)       ((elem) - &((array)[0]))

#define OSA_ARRAYISVALIDENTRY(elem,array) ((OSA_ARRAYINDEX(elem,array) <   \
                                             OSA_ARRAYSIZE(array))           \
                                             ? TRUE                            \
                                             : FALSE)
#define OSA_DIV(num,den)                  (((den) != 0) ? ((num)/(den)) : 0)

#define OSA_ISERROR(status)               ((status < 0) ? TRUE : FALSE)
#include <osa_debug.h>

Uint32 OSA_getCurTimeInMsec();
void   OSA_waitMsecs(Uint32 msecs);
int    OSA_attachSignalHandler(int sigId, void (*handler)(int ) );
Int32 OSA_mapMem(UInt32 physAddr, UInt32 memSize , Ptr *pMemVirtAddrPtr);
Int32 OSA_unmapMem(Ptr pMemVirtAddrPtr,UInt32 memSize);

int xstrtoi(char *hex);

#endif /* _OSA_H_ */



