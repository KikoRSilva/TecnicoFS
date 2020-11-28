#include "tecnicofs-client-api.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <stdio.h>

#define error(msg) {fprintf(stderr, msg); exit(EXIT_FAILURE);}

int sockfd = -1;
struct sockaddr_un client_addr, server_addr;
socklen_t clientlen, serverlen;

int tfsCreate(char *filename, char nodeType) {

  if (sockfd == -1)
    return TECNICOFS_ERROR_NO_OPEN_SESSION;

  char buf[200];
  int res;

  if (sprintf(buf, "c %s %c", filename, nodeType) < 0)
    error("Error: tfsCreate - passing output to buffer\n");
  
  if (sendto(sockfd, buf, sizeof(buf)-1, 0, (struct sockaddr *) &server_addr, serverlen) < 0)
    return TECNICOFS_ERROR_CONNECTION_ERROR;

  if (recvfrom(sockfd, &res, sizeof(res), 0, 0, 0) < 0)
    return TECNICOFS_ERROR_CONNECTION_ERROR;

  return res;
}

int tfsDelete(char *path) {
  if (sockfd == -1)
    return TECNICOFS_ERROR_NO_OPEN_SESSION;
  
  char buf[200];
  int res;

  if (sprintf(buf, "d %s", path) < 0)
    error("Error: tfsDelete - passing output to buffer\n");
  
  if (sendto(sockfd, buf, sizeof(buf)-1, 0, (struct sockaddr *) &server_addr, serverlen) < 0)
    return TECNICOFS_ERROR_CONNECTION_ERROR;

  if (recvfrom(sockfd, &res, sizeof(res), 0, 0, 0) < 0)
    return TECNICOFS_ERROR_CONNECTION_ERROR;

  return res;
}

int tfsMove(char *from, char *to) {
  if (sockfd == -1)
    return TECNICOFS_ERROR_NO_OPEN_SESSION;
  
  char buf[200];
  int res;

  if (sprintf(buf, "m %s %s", from, to) < 0)
    error("Error: tfsMove - passing output to buffer\n");
  
  if (sendto(sockfd, buf, sizeof(buf)-1, 0, (struct sockaddr *) &server_addr, serverlen) < 0)
    return TECNICOFS_ERROR_CONNECTION_ERROR;

  if (recvfrom(sockfd, &res, sizeof(res), 0, 0, 0) < 0)
    return TECNICOFS_ERROR_CONNECTION_ERROR;

  return res;
}

int tfsLookup(char *path) {

  if (sockfd == -1)
    return TECNICOFS_ERROR_NO_OPEN_SESSION;
  
  char buf[200];
  int res;

  if (sprintf(buf, "l %s", path) < 0)
    error("Error: tfsLookup - passing output to buffer\n");
  
  if (sendto(sockfd, buf, sizeof(buf)-1, 0, (struct sockaddr *) &server_addr, serverlen) < 0)
    return TECNICOFS_ERROR_CONNECTION_ERROR;

  if (recvfrom(sockfd, &res, sizeof(res), 0, 0, 0) < 0)
    return TECNICOFS_ERROR_CONNECTION_ERROR;

  return res;
}

int tfsPrint(char * outputFile) {

  if (sockfd == -1)
    return TECNICOFS_ERROR_NO_OPEN_SESSION;
  
  char buf[200];
  int res;

  if (sprintf(buf, "p %s", outputFile) < 0)
    error("Error: tfsPrint - passing output to buffer\n");
  
  if (sendto(sockfd, buf, sizeof(buf)-1, 0, (struct sockaddr *) &server_addr, serverlen) < 0)
    return TECNICOFS_ERROR_CONNECTION_ERROR;

  if (recvfrom(sockfd, &res, sizeof(res), 0, 0, 0) < 0)
    return TECNICOFS_ERROR_CONNECTION_ERROR;
  
  return res;
}

/*
 * This function stablish a session with TecnicoFs server located in the sockPath
 * Returns error if the client already has a active session with this server
*/
int tfsMount(char * sockPath) {

  pid_t pid = getpid();

  if ((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
    return TECNICOFS_ERROR_OPEN_SESSION;
  }

  bzero((char *) &client_addr, sizeof(client_addr));
  client_addr.sun_family = AF_UNIX;

  char * clientpath = malloc(sizeof(char) * 100);
  sprintf(clientpath, "/tmp/client-%d", pid);
  strcpy(client_addr.sun_path, clientpath);
  clientlen = SUN_LEN(&client_addr);

  if (bind(sockfd, (struct sockaddr *) &client_addr, clientlen) < 0)
    return TECNICOFS_ERROR_CONNECTION_ERROR;

  bzero((char *) &server_addr, sizeof(server_addr));
  server_addr.sun_family = AF_UNIX;
  strcpy(server_addr.sun_path, sockPath);
  serverlen = SUN_LEN(&server_addr);

  return 0;
}

int tfsUnmount() {

  if (sockfd == -1)
    return TECNICOFS_ERROR_NO_OPEN_SESSION;

  if (close(sockfd) == -1)
    return TECNICOFS_ERROR_NO_OPEN_SESSION;  

  sockfd = -1;

  return 0;
}
