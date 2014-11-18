
#ifndef _OSA_ETH_SERVER_H_
#define _OSA_ETH_SERVER_H_

#include <osa.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

typedef struct {

  int serverSocketId;
  int serverPort;
  int connectedSocketId;
  
  char serverName[255];

} OSA_ETH_ServerObj;

int OSA_ETH_serverOpen(OSA_ETH_ServerObj *pObj, Uint32 port);
int OSA_ETH_serverWaitConnect(OSA_ETH_ServerObj *pObj);
int OSA_ETH_serverSendData(OSA_ETH_ServerObj *pObj, Uint8 *dataBuf, Uint32 dataSize);
int OSA_ETH_serverClose(OSA_ETH_ServerObj *pObj);

#endif
