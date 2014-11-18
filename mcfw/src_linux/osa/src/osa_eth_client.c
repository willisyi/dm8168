
#include <osa_eth_client.h>

int OSA_ETH_clientOpen(OSA_ETH_ClientObj *pObj, char *ip_addr, Uint32 port)
{
  int sin_size;
  struct hostent *host;
  struct sockaddr_in server;

  host = gethostbyname(ip_addr);

  if(host==NULL)
    return OSA_EFAIL;

  strcpy(pObj->ipAddr, ip_addr);

  pObj->serverPort = port;

  pObj->clientSocketId = socket(AF_INET, SOCK_STREAM, 0);
  if (pObj->clientSocketId < 0) {
    OSA_ERROR(" OSA_ETH_CLIENT(%s:%d): Socket Open !!!\n", pObj->ipAddr, pObj->serverPort);
    return OSA_EFAIL;
  }

  server.sin_family = AF_INET;
  server.sin_port = htons(port);
  server.sin_addr = *((struct in_addr *)host->h_addr);
  bzero(&(server.sin_zero),8);

  sin_size = sizeof(server);

  if (connect(pObj->clientSocketId, (struct sockaddr *)&server, sin_size) == -1)
  {
    //OSA_ERROR(" OSA_ETH_CLIENT(%s:%d): Server Connect !!!\n", pObj->ipAddr, pObj->serverPort);
    return OSA_EFAIL;
  }

  OSA_printf(" OSA_ETH_CLIENT(%s:%d): Connected to Server !!!\n", pObj->ipAddr, pObj->serverPort);

  return OSA_SOK;
}

int OSA_ETH_clientClose(OSA_ETH_ClientObj *pObj)
{
  int ret;

  if(pObj->clientSocketId < 0)
    return OSA_EFAIL;

  ret = close(pObj->clientSocketId);

  //OSA_printf(" OSA_ETH_CLIENT(%s:%d): Disconnected from Server !!!\n", pObj->ipAddr, pObj->serverPort);

  return ret;
}

int OSA_ETH_clientRecvData(OSA_ETH_ClientObj *pObj, Uint8 *dataBuf, Uint32 *dataSize)
{
  int actDataSize;

  actDataSize = recv(pObj->clientSocketId, dataBuf, *dataSize, 0);
  if(actDataSize<=0)
  {
    *dataSize = 0;
    return OSA_EFAIL;
  }

  *dataSize = actDataSize;

  return OSA_SOK;
}

