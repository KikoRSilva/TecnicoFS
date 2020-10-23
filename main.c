#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include "fs/operations.h"
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


void* applyCommands(void* arg){

    while (numberCommands > 0){

        if (pthread_rwlock_init(&lock, NULL) != 0){
            fprintf(stderr, "Error: rwlock initialization failed.\n");
            exit(EXIT_FAILURE);
        }
        
        pthread_rwlock_wrlock(&lock);
        const char* command = removeCommand();
        if (command == NULL){
            pthread_rwlock_unlock(&lock);
            continue;
        }

        char token, type;
        char name[MAX_INPUT_SIZE];
        int numTokens = sscanf(command, "%c %s %c", &token, name, &type);
        if (numTokens < 2) {
            fprintf(stderr, "Error: invalid command in Queue.\n");
            pthread_rwlock_unlock(&lock);
            exit(EXIT_FAILURE);
        }
        pthread_rwlock_unlock(&lock);

        int searchResult;
        switch (token) {
            case 'c':
                switch (type) {
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
            case 'l':
                searchResult = lookup(name);
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

    /* starts the clock */
    clock_t begin = clock();

    /* create a pool of threads */
    poolThreads();

    /* write the output in the output_file and close it */
    print_tecnicofs_tree(file);
    fclose(file);

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