#include "tecnicofs-client-api.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <stdio.h>

int sockfd = -1;

int tfsCreate(char *filename, char nodeType) {
  return -1;
}

int tfsDelete(char *path) {
  return -1;
}

int tfsMove(char *from, char *to) {
  return -1;
}

int tfsLookup(char *path) {
  return -1;
}

/*
 * This function stablish a session with TecnicoFs server located in the sockPath
 * Returns error if the client already has a active session with this server
*/
int tfsMount(char * sockPath) {

  struct sockaddr_un client_addr;
  socklen_t clientlen;
  char buffer[1024];

  if ((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
    return TECNICOFS_ERROR_OPEN_SESSION;
  }

  // memory reset
  bzero((char *) &client_addr, sizeof(client_addr));

  client_addr.sun_family = AF_UNIX;
  strcpy(client_addr.sun_path, sockPath);
  clientlen = SUN_LEN(&client_addr);

  if (bind(sockfd, (struct sockaddr *) &client_addr, clientlen) < 0) {
    return TECNICOFS_ERROR_CONNECTION_ERROR;
  };

  char * message;
  strcpy(message, "Hello World\n");

  // TEST MESSAGE: "HELLO WORLD!"
  if (sendto(sockfd, message, strlen(message)+1, 0, (struct sockadrr *)&client_addr, clientlen) < 0)
    return TECNICOFS_ERROR_CONNECTION_ERROR;
  

  return 0;
}

int tfsUnmount() {
  if (close(sockfd))
    return TECNICOFS_ERROR_NO_OPEN_SESSION;
  return 0;
}
