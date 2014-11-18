
#include <osa_eth_server.h>

int OSA_ETH_serverOpen(OSA_ETH_ServerObj *pObj, Uint32 port)
{
  struct sockaddr_in server;

  int option = 1; 
    
  pObj->serverPort = port;
  
  pObj->serverName[0]=0;
  
  gethostname(pObj->serverName, 255);
  
  pObj->serverSocketId = socket(AF_INET, SOCK_STREAM, 0);  
  if (pObj->serverSocketId < 0) {
    OSA_ERROR("OSA_ETH_SERVER(%d): Socket Open !!!\n", port);
    return OSA_EFAIL;
  }

  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(port);
  bzero(&(server.sin_zero),8);

  setsockopt ( pObj->serverSocketId, SOL_SOCKET, SO_REUSEADDR, &option, sizeof( option ) );  
  
  if (bind(pObj->serverSocketId, (struct sockaddr *)&server, sizeof(server)) == -1) {
    OSA_ERROR("OSA_ETH_SERVER(%s:%d): Socket Bind !!!\n", pObj->serverName, port);
    return OSA_EFAIL;
  }
  
  if(listen(pObj->serverSocketId, 5) < 0) {
    OSA_ERROR("OSA_ETH_SERVER(%s:%d): Socket Listen !!!\n", pObj->serverName, port);
    return OSA_EFAIL;
  }

  //OSA_printf(" OSA_ETH_SERVER(%s:%d): Server Open !!! \n", pObj->serverName, pObj->serverPort);
      
  return OSA_SOK;
}

int OSA_ETH_serverWaitConnect(OSA_ETH_ServerObj *pObj)
{
  unsigned int sin_size; 
  struct sockaddr_in client;  

  if (pObj->serverSocketId < 0) {
    OSA_ERROR("OSA_ETH_SERVER(%s:%d): Illegal server socket ID !!!\n", pObj->serverName, pObj->serverPort);  
    return OSA_EFAIL;
  }

  sin_size = sizeof(struct sockaddr_in);

  OSA_printf(" OSA_ETH_SERVER(%s:%d): Waiting for Client !!! \n", pObj->serverName, pObj->serverPort);

  pObj->connectedSocketId = accept(pObj->serverSocketId, (struct sockaddr *)&client, &sin_size);
  
  if(pObj->connectedSocketId<0){
    OSA_ERROR("OSA_ETH_SERVER(%s:%d): Illegal client socket ID !!!\n", pObj->serverName, pObj->serverPort);  
    return OSA_EFAIL;
  }

  OSA_printf(" OSA_ETH_SERVER(%s:%d): Connected to Client !!! \n", pObj->serverName, pObj->serverPort);
  
  return OSA_SOK;  
}

int OSA_ETH_serverClose(OSA_ETH_ServerObj *pObj)
{
  int status;
  
  if(pObj->serverSocketId < 0)
    return OSA_EFAIL;

  status = close(pObj->serverSocketId);
  
  //OSA_printf(" OSA_ETH_SERVER(%s:%d): Server Close !!! \n", pObj->serverName, pObj->serverPort);
    
  return status;
}

int OSA_ETH_serverSendData(OSA_ETH_ServerObj *pObj, Uint8 *dataBuf, Uint32 dataSize)
{
  int actDataSize=0; 
  
  while(dataSize > 0 ) {  
    actDataSize = send(pObj->connectedSocketId, dataBuf, dataSize, 0);    

    //OSA_printf(" OSA_ETH_SERVER(%s:%d): Sent Data %d B of %d B)\n", pObj->serverName, pObj->serverPort, actDataSize, dataSize);

    if(actDataSize<=0)
      break;
    dataBuf += actDataSize;
    dataSize -= actDataSize;
  }
  
  if( dataSize > 0 ) {
    OSA_ERROR("OSA_ETH_SERVER(%s:%d): Send Data !!!\n", pObj->serverName, pObj->serverPort);
    return OSA_EFAIL;
  }

  return OSA_SOK;
}





