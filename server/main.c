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
int serv_sockfd;

pthread_rwlock_t lock = PTHREAD_RWLOCK_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t untilNotEmpty = PTHREAD_COND_INITIALIZER, untilNotFull = PTHREAD_COND_INITIALIZER;

int numberCommands = 0; // f
int headQueue = 0; // i
int nv = MAX_COMMANDS;

CircularQueue *queue;

////////////////////////////////////// Sync Functions ////////////////////////////////////////////
void init_mutex() {
    if(pthread_mutex_init(&mutex, NULL)){
        fprintf(stderr, "Error: Failed to init the mutex lock.\n");
        exit(EXIT_FAILURE);
    }
}

void mutex_lock(){
    if(pthread_mutex_lock(&mutex)){
        fprintf(stderr, "Error: Failed to lock mutex.\n");
        exit(EXIT_FAILURE);
    }
}

void mutex_unlock(){
     if(pthread_mutex_unlock(&mutex)){
        fprintf(stderr, "Error: Failed to unlock mutex.\n");
        exit(EXIT_FAILURE);
    }
}

void destroy_mutex(){
    if(pthread_mutex_destroy(&mutex)){
        fprintf(stderr, "Error: Failed to destroy mutex.\n");
        exit(EXIT_FAILURE);
    }
}

void wait(pthread_cond_t *varCond){
    if(pthread_cond_wait(varCond, &mutex)){
        fprintf(stderr, "Error: Failed to wait.\n");
        exit(EXIT_FAILURE);
    }
}

void signal(pthread_cond_t *varCond){
    if(pthread_cond_signal(varCond)){
        fprintf(stderr, "Error: Failed to signal.\n");
        exit(EXIT_FAILURE);
    }
}

void broadcast(pthread_cond_t *varCond){
    if(pthread_cond_broadcast(varCond)){
        fprintf(stderr, "Error: Failed to broadcast.\n");
        exit(EXIT_FAILURE);
    }
}

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
            fprintf(stderr, "Error: Socket %d with error %d", sockfd, "FILE_IS_OPEN");
            exit(EXIT_FAILURE);
        case TECNICOFS_ERROR_INVALID_MODE:
            fprintf(stderr, "Error: Socket %d with error %d", sockfd, "INVALID_MODE");
            exit(EXIT_FAILURE);
        case TECNICOFS_ERROR_OTHER:
            fprintf(stderr, "Error: Socket %d with error %d", sockfd, "OTHER");
            exit(EXIT_FAILURE);
        case TECNICOFS_ERROR_INVALID_COMMAND:
            fprintf(stderr, "Error: Socket %d with error %d", sockfd, "INVALID_COMMAND");
            exit(EXIT_FAILURE);
        default:
            fprintf(stderr, "Error: Socket %d with error %d", sockfd, "DEFAULT");
            exit(EXIT_FAILURE);
    }
}

void* processInput(int serv_sockfd){

    clock_gettime(CLOCK_MONOTONIC_RAW, &begin);

    /* break loop with ^Z or ^D */
    while (TRUE) {

        char token, type;
        char buffer[MAX_INPUT_SIZE];
        char * command = malloc(sizeof(char) * (MAX_INPUT_SIZE + 1));

        if (recvfrom(serv_sockfd, buffer, sizeof(buffer), 0, 0, 0) < 0) {
            socketError(serv_sockfd, TECNICOFS_ERROR_CONNECTION_ERROR);
        }

        strcpy(command, buffer);

        applyCommands(command, serv_sockfd);
    }
    return NULL;
}


void* applyCommands(char *command, int serv_sockfd){
    
    if (command == NULL)
        socketError(serv_sockfd, TECNICOFS_ERROR_INVALID_COMMAND);

    char token;
    char name[MAX_INPUT_SIZE], last_name[MAX_INPUT_SIZE];
    
    int numTokens = sscanf(command, "%c %s %s", &token, name, last_name);
    
    if (numTokens < 2) {
        fprintf(stderr, "Error: invalid command in Queue.\n");
        exit(EXIT_FAILURE);
    }

    int searchResult;
    switch (token) {
        case 'c':
            switch (last_name[0]) {
                case 'f':
                    printf("Create file: %s.\n", name);
                    create(name, T_FILE);
                    break;
                case 'd':
                    printf("Create directory: %s.\n", name);
                    create(name, T_DIRECTORY);
                    break;
                default:
                    fprintf(stderr, "Error: invalid node type.\n");
                    exit(EXIT_FAILURE);
            }
            break;
        case 'm':
            printf("Move directory: %s.\n", name);
            move(name, last_name);
            break;
        case 'l':
            searchResult = search(name, LOOKUP);
            if (searchResult >= 0){
                printf("Search: %s found.\n", name);
                }
            else
                printf("Search: %s not found.\n", name);
            break;
        case 'd':
            printf("Delete: %s.\n", name);
            delete(name);
            break;
        default: {
            /* error */
            fprintf(stderr, "Error: command to apply.\n");
            exit(EXIT_FAILURE);
        }
    }
return NULL;
}

/* this function create a pool of threads */
void poolThreads(int serv_sockfd){

    pthread_t consumer[numthreads];

    // create slave threads
    for (int i = 0; i < numthreads; i++) {
        if (pthread_create(&consumer[i], NULL, processInput, (void *)serv_sockfd) != 0) {
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

/* this function gets the command line arguments and parses them to their corresponding variables */
void assignArgs(int argc, char* argv[]){

    /*  |  ./tecnicofs | numthreads | namesocket  |
        |      0       |    1       |    2        | TOTAL: 4
    */
    if (argc == 3){

        numthreads = atoi(argv[1]);
        strcpy(namesocket, argv[2]);

        /* check the numthreads */
        if (numthreads <= 0){
            fprintf(stderr, "Error: invalid number of threads (>0).\n");
            exit(EXIT_FAILURE);
        }
    }
    else{
        fprintf(stderr, "Error: the command line must have 4 arguments.\n");
        exit(EXIT_FAILURE);
    }
}

int createSocket(char * sockPath) {
    socklen_t servlen;
    struct sockaddr_un serv_addr;
    int serv_sockfd;

    if (serv_sockfd = socket(AF_UNIX, SOCK_DGRAM, 0) < 0) {
        fprintf(stderr, "Error: Unable to create a server socket.\n");
        exit(EXIT_FAILURE);
    }

    // perguntar ao prof!!
    unlink(sockPath);

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sun_family = AF_UNIX;
    strcpy(serv_addr.sun_path, sockPath);
    servlen = SUN_LEN(&serv_addr);

    if (bind(serv_sockfd, (struct sockaddr *) &serv_addr, servlen) < 0) {
        fprintf(stderr, "Error: Unable to bind the server socket.\n");
        exit(EXIT_FAILURE);
    }

    return serv_sockfd;
}


int main(int argc, char* argv[]){

    FILE *file;

    /* parse the arguments */
    assignArgs(argc, argv);

    /* initialize condition variables and the mutex lock */
    pthread_cond_init(&untilNotEmpty, NULL);
    pthread_cond_init(&untilNotFull,NULL);
    init_mutex();
    /* init filesystem */
    init_fs();
    /* init CircularQueue */
    queue = initQueue();
    /* init server socket */
    serv_sockfd = createSocket(namesocket);
    /* create a pool of threads & process input & apply commands*/
    poolThreads(serv_sockfd);
    /* destroy the circular queue */
    destroyQueue(queue);
    /* write the output in the output_file and close it */
    print_tecnicofs_tree(file);
    fclose(file);
    /* release allocated memory */
    destroy_fs();
    /* destroy the mutex lock and condition variables*/
    pthread_cond_destroy(&untilNotFull);
    pthread_cond_destroy(&untilNotEmpty);
    destroy_mutex();

    /* ends clock and shows time*/
    clock_gettime(CLOCK_MONOTONIC_RAW, &end);
    printf("TecnicoFS completed in %0.4f seconds.\n", (end.tv_nsec - begin.tv_nsec) / 1000000000.0 +
            (end.tv_sec  - begin.tv_sec));

    /* Success */
    exit(EXIT_SUCCESS);

}
