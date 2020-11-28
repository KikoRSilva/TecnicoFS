#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include "fs/operations.h"
#include <time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/un.h>
#include "circularqueue/circularqueue.h"

////////////////////////////////////// Macros ////////////////////////////////////////////
#define MAX_COMMANDS 10
#define MAX_INPUT_SIZE 100
#define TRUE 1
#define FALSE 0

////////////////////////////////////// Global Variables ////////////////////////////////////////////
int numthreads;
char * namesocket;
int sockfd;

////////////////////////////////////// Functions ////////////////////////////////////////////

void errorParse(){
    fprintf(stderr, "Error: command invalid.\n");
    exit(EXIT_FAILURE);
}

void socketError(int sockfd, int errorConstant) {
    switch(errorConstant) {
        case TECNICOFS_ERROR_CONNECTION_ERROR:
            fprintf(stderr, "Error: Socket %d with error %s", sockfd, "CONNECTION_ERROR");
            exit(EXIT_FAILURE);
        case TECNICOFS_ERROR_OPEN_SESSION:
            fprintf(stderr, "Error: Socket %d with error %s", sockfd, "OPEN_SESSION");
            exit(EXIT_FAILURE);
        case TECNICOFS_ERROR_NO_OPEN_SESSION:
            fprintf(stderr, "Error: Socket %d with error %s", sockfd, "NO_OPEN_SESSION");
            exit(EXIT_FAILURE);
        case TECNICOFS_ERROR_FILE_ALREADY_EXISTS:
            fprintf(stderr, "Error: Socket %d with error %s", sockfd, "FILE_ALREADY_EXISTS");
            exit(EXIT_FAILURE);
        case TECNICOFS_ERROR_FILE_NOT_FOUND:
            fprintf(stderr, "Error: Socket %d with error %s", sockfd, "FILE_NOT_FOUND");
            exit(EXIT_FAILURE);
        case TECNICOFS_ERROR_PERMISSION_DENIED:
            fprintf(stderr, "Error: Socket %d with error %s", sockfd, "PERMISSION_DENIED");
            exit(EXIT_FAILURE);
        case TECNICOFS_ERROR_MAXED_OPEN_FILES:
            fprintf(stderr, "Error: Socket %d with error %s", sockfd, "MAXED_OPEN_FILES");
            exit(EXIT_FAILURE);
        case TECNICOFS_ERROR_FILE_NOT_OPEN:
            fprintf(stderr, "Error: Socket %d with error %s", sockfd, "FILE_NOT_OPEN");
            exit(EXIT_FAILURE);
        case TECNICOFS_ERROR_FILE_IS_OPEN:
            fprintf(stderr, "Error: Socket %d with error %s", sockfd, "FILE_IS_OPEN");
            exit(EXIT_FAILURE);
        case TECNICOFS_ERROR_INVALID_MODE:
            fprintf(stderr, "Error: Socket %d with error %s", sockfd, "INVALID_MODE");
            exit(EXIT_FAILURE);
        case TECNICOFS_ERROR_OTHER:
            fprintf(stderr, "Error: Socket %d with error %s", sockfd, "OTHER");
            exit(EXIT_FAILURE);
        case TECNICOFS_ERROR_INVALID_COMMAND:
            fprintf(stderr, "Error: Socket %d with error %s", sockfd, "INVALID_COMMAND");
            exit(EXIT_FAILURE);
        default:
            fprintf(stderr, "Error: Socket %d with error %s", sockfd, "DEFAULT");
            exit(EXIT_FAILURE);
    }
}

/**
 * @function                    applyCommand
 * @abstract                    run a function depending on the token of the command
 * @param       command         has a token, specific token args
 * @param       serv_sockfd     The server socket file descriptor
 * @return                      the return value of the function executed
*/
int applyCommand(char *command, int serv_sockfd){
    
    if (command == NULL)
        socketError(serv_sockfd, TECNICOFS_ERROR_INVALID_COMMAND);

    char token;
    char name[MAX_INPUT_SIZE], last_name[MAX_INPUT_SIZE];
    
    int numTokens = sscanf(command, "%c %s %s", &token, name, last_name);
    
    if (numTokens < 2) {
        fprintf(stderr, "Error: invalid command in Queue.\n");
        exit(EXIT_FAILURE);
    }

    switch (token) {
        case 'c':
            switch (last_name[0]) {
                case 'f':
                    return create(name, T_FILE);
                case 'd':
                    return create(name, T_DIRECTORY);
                    
                default:
                    fprintf(stderr, "Error: invalid node type.\n");
                    return TECNICOFS_ERROR_INVALID_COMMAND;
            }
        case 'm':
            return move(name, last_name); 
        case 'l':
            return search(name, LOOKUP);
        case 'd':
            return delete(name);
        case 'p':
            return print_tecnicofs_tree(name);
        default: {
            /* error */
            fprintf(stderr, "Error: command to apply.\n");
            return TECNICOFS_ERROR_INVALID_COMMAND;
        }
    }
}

/**
 * @function            processInput
 * @abstract            receives input commands from the client through a socket and @applyCommand
 * @param       arg     pointer to an argument
 * @return              NULL
*/
void* processInput(void *arg){

    clock_gettime(CLOCK_MONOTONIC_RAW, &begin);
    int server_sockfd = *((int *) arg);
    int result;
    /* break loop with ^Z or ^D */
    while (TRUE) {

        char in_buffer[MAX_INPUT_SIZE];
        struct sockaddr_un client_addr;
        socklen_t addrlen = sizeof(struct sockaddr_un);
        
        if (recvfrom(server_sockfd, in_buffer, sizeof(in_buffer)-1, 0, (struct sockaddr *) &client_addr, &addrlen) < 0)
            socketError(server_sockfd, TECNICOFS_ERROR_CONNECTION_ERROR);

        result = applyCommand(in_buffer, server_sockfd);

        if (sendto(sockfd, (int *) &result, sizeof(result)+1, 0, (struct sockaddr *) &client_addr, addrlen) < 0)
            socketError(server_sockfd, TECNICOFS_ERROR_CONNECTION_ERROR);
    }
    return NULL;
}

/**
 * @function            poolThreads
 * @abstract            initialize and create threads and make them @processInput
 * @param     nothing 
 * @return              nothing
*/
void poolThreads(){

    pthread_t consumer[numthreads];
    // create slave threads
    for (int i = 0; i < numthreads; i++) {
        if (pthread_create(&consumer[i], NULL, processInput, (void *) &sockfd) != 0) {
            fprintf(stderr, "Error: unable to create applyCommands thread.\n");
            exit(EXIT_FAILURE);
        }
    }
    // slave threads waiting to finish
    for (int i = 0; i < numthreads; i++) {
        if (pthread_join(consumer[i], NULL) != 0) {
            fprintf(stderr, "Error: unable to join applyCommands threads.\n");
            exit(EXIT_FAILURE);
        }
    }
}

/**
 * @function            assignArgs
 * @abstract            parse the IO arguments to global variables
 * @param       argc    number of arguments
 * @param       argv[]  array of arguments
 * @return              nothing
*/
void assignArgs(int argc, char* argv[]){

    /*  
        |  ./tecnicofs | numthreads | namesocket  |
        |      0       |    1       |    2        | TOTAL: 3
    */

    namesocket = malloc(sizeof(char) * 1024);
    if (argc == 3){

        numthreads = atoi(argv[1]);
        strcpy(namesocket, argv[2]);
        printf("argv[2]: %s e namesocket: %s\n", argv[2], namesocket);

        /* check the numthreads */
        if (numthreads <= 0){
            fprintf(stderr, "Error: invalid number of threads (>0).\n");
            exit(EXIT_FAILURE);
        }
    }
    else{
        fprintf(stderr, "Error: the command line must have 4 arguments.\nDisplay: ./tecnicofs <numthreads> <namesocket>\n");
        exit(EXIT_FAILURE);
    }
}

/**
 * @function            setSockAddrUn
 * @abstract            set the socket with the given name path and address
 * @param       path    socket path name
 * @param       addr    socket address
 * @return              an integer with the value of the length of the address
*/
int setSockAddrUn(char *path, struct sockaddr_un *addr) {

  if (addr == NULL)
    return 0;

  bzero((char *)addr, sizeof(struct sockaddr_un));
  addr->sun_family = AF_UNIX;
  strcpy(addr->sun_path, path);

  return SUN_LEN(addr);
}

int main(int argc, char* argv[]){
    
    socklen_t serverlen;
    struct sockaddr_un server_addr;
    
    /* parse the arguments */
    assignArgs(argc, argv);
    /* init filesystem */
    init_fs();
    /* init client socket */
    if ((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
        fprintf(stderr, "Error: Unable to create a server socket.\n");
        exit(EXIT_FAILURE);
    }

    unlink(namesocket);

    serverlen = setSockAddrUn(namesocket, &server_addr);

    if (bind(sockfd, (struct sockaddr *) &server_addr, serverlen) < 0) {
        fprintf(stderr, "Error: Unable to bind the server socket.\n");
        exit(EXIT_FAILURE);
    }
    /* create a pool of threads & process input & apply commands*/
    poolThreads();
    /* release allocated memory */
    free(namesocket);
    destroy_fs();
    /* ends clock and shows time*/
    clock_gettime(CLOCK_MONOTONIC_RAW, &end);
    printf("TecnicoFS completed in %0.4f seconds.\n", (end.tv_nsec - begin.tv_nsec) / 1000000000.0 +
            (end.tv_sec  - begin.tv_sec));
    /* Success */
    exit(EXIT_SUCCESS);

}
