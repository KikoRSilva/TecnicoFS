#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include "fs/operations.h"
#include <time.h>
#include "circularqueue/circularqueue.h"


////////////////////////////////////// Macros ////////////////////////////////////////////
#define MAX_COMMANDS 10
#define MAX_INPUT_SIZE 100
#define TRUE 1
#define FALSE 0

////////////////////////////////////// Global Variables ////////////////////////////////////////////
int numthreads = 0;
pthread_rwlock_t lock = PTHREAD_RWLOCK_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t untilNotEmpty = PTHREAD_COND_INITIALIZER, untilNotFull = PTHREAD_COND_INITIALIZER;

char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
int numberCommands = 0; // f
int headQueue = 0; // i
int nv = MAX_COMMANDS;

char* input_file;
char* output_file;

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

int insertCommand(char* data) {

    mutex_lock();               // start of critical section

    while (isFull(queue)) {
        wait(&untilNotFull);
    } 

    enQueue(queue, data);
    numberCommands++;
    signal(&untilNotEmpty);

    mutex_unlock();             // end of critical section

    return 1;
}

char* removeCommand() {

    mutex_lock();               // start of critical section

    char* removeCommand;

    if (isEmpty(queue) && queue->isCompleted) {
        mutex_unlock();
        return NULL;
    }

    while(isEmpty(queue) && !queue->isCompleted) {
        wait(&untilNotEmpty);
    }
       

    removeCommand = deQueue(queue);
    numberCommands--;
    if (isEmpty(queue))
    signal(&untilNotFull);

    mutex_unlock();             // end of critical section

    return removeCommand;
}

void errorParse(){
    fprintf(stderr, "Error: command invalid.\n");
    exit(EXIT_FAILURE);
}

void* processInput(){

    char line[MAX_INPUT_SIZE];
    FILE *file;


    file = fopen(input_file, "r");
    clock_gettime(CLOCK_MONOTONIC_RAW, &begin);

    if (file == NULL){
        fprintf(stderr, "Error: unable to open the file.\n");
        exit(EXIT_FAILURE);
    }

    /* break loop with ^Z or ^D */
    while (fgets(line, sizeof(line)/sizeof(char), file) != NULL) {

        char token, type;
        char name[MAX_INPUT_SIZE];

        int numTokens = sscanf(line, "%c %s %c", &token, name, &type);

        /* perform minimal validation */
        if (numTokens < 1) {
            continue;
        }
        switch (token) {
            case 'c':
                if(numTokens != 3)
                    errorParse();
                if(insertCommand(line))
                    break;
                return NULL;
            case 'm':
                if(numTokens != 3)
                    errorParse();
                if(insertCommand(line))                    
                    break;
                return NULL;
            case 'l':
                if(numTokens != 2)
                    errorParse();
                if(insertCommand(line))
                    break;
                return NULL;
            case 'd':
                if(numTokens != 2)
                    errorParse();
                if(insertCommand(line))
                    break;
                return NULL;
            case '#':
                break;
            default: { /* error */
                errorParse();
            }
        }
    }
    fclose(file);
    changeState(queue);
    broadcast(&untilNotEmpty);
    return NULL;
}


void* applyCommands(){

    while (TRUE){

        const char* command = removeCommand();

        if (command == NULL)
            return NULL;

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
            default: { /* error */
                fprintf(stderr, "Error: command to apply.\n");
                exit(EXIT_FAILURE);
            }
        }
    }
    return NULL;
}

/* this function create a pool of threads */
void poolThreads(){

    pthread_t consumer[numthreads];

    // create slave threads
    for (int i = 0; i < numthreads; i++) {
        if (pthread_create(&consumer[i], NULL, &applyCommands, NULL) != 0) {
            fprintf(stderr, "Error: unable to create applyCommands thread.\n");
            exit(EXIT_FAILURE);
        }
        
    }
    processInput();
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

    /*  |  ./tecnicofs | input_file | output_file | numthreads |
        |      0       |    1       |    2        |    3       | TOTAL: 4
    */
    if (argc == 4){

        input_file = argv[1];
        output_file = argv[2];
        numthreads = atoi(argv[3]);

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


int main(int argc, char* argv[]){

    FILE *file;

    /* parse the arguments */
    assignArgs(argc, argv);
    /* open the output_file to write the final tecnicofs */
    file = fopen(output_file, "w");

    if (file == NULL){
        fprintf(stderr, "Error: unable to open the output file.\n");
        exit(EXIT_FAILURE);
    }
    /* open the output_file to write the final tecnicofs */
    file = fopen(output_file, "w");
    /* initialize condition variables and the mutex lock */
    pthread_cond_init(&untilNotEmpty, NULL);
    pthread_cond_init(&untilNotFull,NULL);
    init_mutex();
    /* init filesystem */
    init_fs();
    /* init CircularQueue */
    queue = initQueue();
    /* create a pool of threads & process input & apply commands*/
    poolThreads();
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
