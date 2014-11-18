
SRCDIR += links_common/system links_common/ipcFramesIn links_common/ipcFramesOut \
			links_common/dup links_common/null links_common/system \
			links_common/merge links_common/ipcBitsOut \
			links_common/select

SRCS_COMMON += dupLink_tsk.c nullLink_tsk.c system_linkApi.c system_linkApi_local.c system_common.c \
		system_ipc.c system_ipc_listMP.c system_ipc_msgq.c system_ipc_notify.c \
		mergeLink_tsk.c ipcFramesInLink_tsk.c ipcFramesOutLink_tsk.c ipcBitsOutLink_tsk.c \
		selectLink_tsk.c system_tiler.c
		 
