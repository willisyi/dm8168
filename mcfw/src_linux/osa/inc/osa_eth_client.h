
#ifndef _OSA_ETH__CLIENT_H_
#define _OSA_ETH__CLIENT_H_

#include <osa.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

typedef struct {

  int serverPort;
  int clientSocketId;

  char ipAddr[256];
  
} OSA_ETH_ClientObj;

int OSA_ETH_clientOpen(OSA_ETH_ClientObj *pObj, char *ip_addr, Uint32 port);
int OSA_ETH_clientRecvData(OSA_ETH_ClientObj *pObj, Uint8 *dataBuf, Uint32 *dataSize);
int OSA_ETH_clientClose(OSA_ETH_ClientObj *pObj);

#endif
