
#include <ti/sdo/edma3/rm/edma3_rm.h>


// All EDMA channels allocated in region 2 will be assigned as follow:
// Channels used for transfers from DDR will be assigned to queue VICP_EDMA3_FROM_DDR_queue
// Channels used for transfers from DDR will be assigned to queue VICP_EDMA3_TO_DDR_queue
EDMA3_RM_EventQueue VICP_EDMA3_FROM_DDR_queue= 3;
EDMA3_RM_EventQueue VICP_EDMA3_TO_DDR_queue= 3;




