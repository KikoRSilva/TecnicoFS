#ifndef QUEUE
#define QUEUE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SIZE 10
#define MAX_INPUT_SIZE 100A
#define FULL 1
#define NOTFULL 0
#define EMPTY 1
#define NOTEMPTY 0

typedef struct Queue {

	int rear, front;								// initially both are -1
	int size;										// 10
	int isCompleted; 								// state
	char inputCommands[SIZE][MAX_INPUT_SIZE];
} CircularQueue;

CircularQueue* initQueue();
void changeState(CircularQueue *queue)
void display(CircularQueue *queue);
char* deQueue(CircularQueue *queue);
void enQueue(CircularQueue *queue, char* element);
int isEmpty(CircularQueue *queue); 
int isFull(CircularQueue *queue);

#endif