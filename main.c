#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include "fs/operations.h"
#include <pthread.h>
#include <time.h>


/*------------------------ Macros ----------------------*/
#define MAX_COMMANDS 150000
#define MAX_INPUT_SIZE 100
#define MAX_SYNCSTRAT 3

#define MUTEX 1
#define RWLOCK 2
#define NOSYNC 3

/*---------------------- Global variables section ----------------------*/
int numthreads = 0;

char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
int numberCommands = 0;
int headQueue = 0;

char* input_file;
char* output_file;

int syncstrategy;

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;

/* Value: 1 for wrlock, 2 for rdlock */
int rwlock_state;             


/* --------------- Functions ---------------*/
int insertCommand(char* data) {
    if(numberCommands != MAX_COMMANDS) {
        strcpy(inputCommands[numberCommands++], data);
        return 1;
    }
    return 0;
}

char* removeCommand() {
    if(numberCommands > 0){
        numberCommands--;
        return inputCommands[headQueue++];
    }
    return NULL;
}

void errorParse(){
    fprintf(stderr, "Error: command invalid.\n");
    exit(EXIT_FAILURE);
}

void processInput(){

    char line[MAX_INPUT_SIZE];
    FILE *file;

    file = fopen(input_file, "r");

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
                return;

            case 'l':
                if(numTokens != 2)
                    errorParse();
                if(insertCommand(line))
                    break;
                return;

            case 'd':
                if(numTokens != 2)
                    errorParse();
                if(insertCommand(line))
                    break;
                return;

            case '#':
                break;

            default: { /* error */
                errorParse();
            }
        }
    }
    fclose(file);
}

void thread_sync_init();
void thread_sync_lock();
void thread_sync_unlock();
void thread_sync_destroy();


void* applyCommands(void* arg){

    while (numberCommands > 0){

        rwlock_state = 0; /* rwlock? then this is a write lock */
        thread_sync_lock();

        const char* command = removeCommand();
        if (command == NULL){
            thread_sync_unlock();
            continue;
        }

        char token, type;
        char name[MAX_INPUT_SIZE];
        int numTokens = sscanf(command, "%c %s %c", &token, name, &type);
        if (numTokens < 2) {
            fprintf(stderr, "Error: invalid command in Queue.\n");
            thread_sync_unlock();
            exit(EXIT_FAILURE);
        }
        thread_sync_unlock();

        rwlock_state = 1; /* rwlock? then this is a read lock */
        thread_sync_lock();
        int searchResult;
        switch (token) {
            case 'c':
                switch (type) {
                    case 'f':
                        printf("Create file: %s.\n", name);
                        thread_sync_unlock();
                        create(name, T_FILE);
                        break;
                    case 'd':
                        printf("Create directory: %s.\n", name);
                        thread_sync_unlock();
                        create(name, T_DIRECTORY);
                        break;
                    default:
                        fprintf(stderr, "Error: invalid node type.\n");
                        thread_sync_unlock();
                        exit(EXIT_FAILURE);
                }
                break;
            case 'l':
                thread_sync_unlock();
                searchResult = lookup(name);
                if (searchResult >= 0){
                    printf("Search: %s found.\n", name);
                  }
                else
                    printf("Search: %s not found.\n", name);
                break;
            case 'd':
                printf("Delete: %s.\n", name);
                thread_sync_unlock();
                delete(name);
                break;
            default: { /* error */
                fprintf(stderr, "Error: command to apply.\n");
                thread_sync_unlock();
                exit(EXIT_FAILURE);
            }
        }
    }
    return NULL;
}

void thread_sync_init(){

    switch(syncstrategy){

        case RWLOCK:
            if (pthread_rwlock_init(&rwlock, NULL) != 0){
                fprintf(stderr, "Error: rwlock initialization failed.\n");
                exit(EXIT_FAILURE);
            }
            break;

        case MUTEX:
            if (pthread_mutex_init(&lock, NULL) != 0){
                fprintf(stderr, "Error: mutex initialization failed.\n");
                exit(EXIT_FAILURE);
            }
            break;

        case NOSYNC:
            break;

        default:
            fprintf(stderr, "Error: wrong syncstrategy failed.\n");
            exit(EXIT_FAILURE);
    }
}

void thread_sync_destroy(){

    switch(syncstrategy){

        case RWLOCK:
            if (pthread_rwlock_destroy(&rwlock) != 0){
                fprintf(stderr, "Error: rwlock destroy failed.\n");
                exit(EXIT_FAILURE);
            }
            break;

        case MUTEX:
            if (pthread_mutex_destroy(&lock) != 0){
                fprintf(stderr, "Error: mutex destroy failed.\n");
                exit(EXIT_FAILURE);
            }
            break;

        case NOSYNC:
            break;

        default:
            fprintf(stderr, "Error: wrong syncstrategy failed.\n");
            exit(EXIT_FAILURE);
    }
}

void thread_sync_lock(){

    switch(syncstrategy){

        case RWLOCK:
            if(rwlock_state == 0)
                if (pthread_rwlock_wrlock(&rwlock) != 0){
                    fprintf(stderr, "Error: write lock failed.\n");
                    exit(EXIT_FAILURE);
                }
            if(rwlock_state == 1)
                if (pthread_rwlock_rdlock(&rwlock) != 0){
                    fprintf(stderr, "Error: read lock failed.\n");
                    exit(EXIT_FAILURE);
                }
            break;

        case MUTEX:
            if (pthread_mutex_lock(&lock) != 0){
                fprintf(stderr, "Error: mutex lock failed.\n");
                exit(EXIT_FAILURE);
            }
            break;

        case NOSYNC:
            break;

        default:
            fprintf(stderr, "Error: wrong syncstrategy failed.\n");
            exit(EXIT_FAILURE);
    }
}

void thread_sync_unlock(){

    switch(syncstrategy){

        case RWLOCK:
            pthread_rwlock_unlock(&rwlock);
            break;

        case MUTEX:
            pthread_mutex_unlock(&lock);
            break;

        case NOSYNC:
            break;

        default:
            fprintf(stderr, "Error: wrong syncstrategy failed.\n");
            exit(EXIT_FAILURE);
    }
}


/* this function create a pool of threads */
void poolThreads(){

    pthread_t tid[numthreads];

    /* create slave (threads :) ) */
    for (int i = 0; i < numthreads; i++)
        if(pthread_create(&tid[i], NULL, &applyCommands, NULL) != 0){
            fprintf(stderr, "Error: unable to create thread.\n");
            exit(EXIT_FAILURE);
        }

    /* threads waiting to finish */
    for (int i = 0; i < numthreads; i++)
        pthread_join(tid[i], NULL);
}

/* this function gets the command line arguments and parses them to their corresponding variables */
void assignArgs(int argc, char* argv[]){

    /*  |  ./tecnicofs | input_file | output_file | numthreads | syncstrategy  |
        |      0       |    1       |    2        |    3       |     4         | TOTAL: 5
    */
    if (argc == 5){

        input_file = argv[1];
        output_file = argv[2];
        numthreads = atoi(argv[3]);

        /* check the numthreads */
        if (numthreads <= 0){
            fprintf(stderr, "Error: invalid number of threads (>0).\n");
            exit(EXIT_FAILURE);
        }

        /* check the syncstrategy */
        if (strcmp(argv[4], "mutex") == 0){
            if (numthreads == 1){
                fprintf(stderr, "Error: number of threads for mutex must be greater than 1.\n");
                exit(EXIT_FAILURE);
            }
            syncstrategy = MUTEX;
        }
        else if (strcmp(argv[4], "rwlock") == 0){
            if (numthreads == 1){
                fprintf(stderr, "Error: number of threads for rwlock must be greater than 1.\n");
                exit(EXIT_FAILURE);
            }
            syncstrategy = RWLOCK;
        }
        else if (strcmp(argv[4], "nosync") == 0){
            /* NOSYNC requires 1 thread */
            if (numthreads != 1){
                fprintf(stderr, "Error: number of threads for nosync must be 1.\n");
                exit(EXIT_FAILURE);
            }
            syncstrategy = NOSYNC;
        }
        else{
            fprintf(stderr, "Error: invalid syncstrategy.\n");
            exit(EXIT_FAILURE);
        }
    }
    else{
        fprintf(stderr, "Error: the command line must have 5 arguments.\n");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char* argv[]){

    FILE *file;
    char aux[50];

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

    /* init filesystem */
    init_fs();

    /* process input and save it in a buffer*/
    processInput();

    /* start the syncstrategy */
    thread_sync_init();

    /* starts the clock */
    clock_t begin = clock();

    /* create a pool of threads */
    poolThreads();

    /* write the output in the output_file and close it */
    print_tecnicofs_tree(file);
    fclose(file);

    /* finishes the syncstrategy */
    thread_sync_destroy();

    /* release allocated memory */
    destroy_fs();

    /* ends clock and shows time*/
    clock_t end = clock();
    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    sprintf(aux,"%.4f", time_spent);
    printf("TecnicoFS completed in %s seconds.\n", aux);
	
    /* tudo correu bem */
    exit(EXIT_SUCCESS);

}
