#include <stdio.h>
#include "circularqueue.h"

CircularQueue* initQueue() {
	CircularQueue *newQueue;
	newQueue = malloc(sizeof(CircularQueue));
	newQueue->front = -1;
	newQueue->rear = -1;
	newQueue->isCompleted = 0;
	return newQueue;
}

// change the state of the circular queue
void changeState(CircularQueue *queue) {

	if (queue->isCompleted == 0)
		queue->isCompleted = 1;
	else
		queue->isCompleted = 0;
}

// check if it is full
int isFull(CircularQueue *queue) {
	if ((queue->front == queue->rear +1) || (queue->front == 0 && queue->rear == SIZE - 1)) 
		return FULL;
	return NOTFULL;
}

// check if it is empty
int isEmpty(CircularQueue *queue) {
  if (queue->front == -1) return EMPTY;
  return NOTEMPTY;
}

// add elements
int enQueue(CircularQueue *queue, char* element) {

	if (isFull(queue))
		return FAIL;

    if (queue->front == -1) 
    	queue->front = 0;

    queue->rear = (queue->rear + 1) % SIZE;
    strcpy(queue->inputCommands[queue->rear], element);

    return SUCCESS;
  }
}

// remove elements
char* deQueue(CircularQueue *queue) {

  char* element;

  if (isEmpty(queue)) 
  	return NULL;
  else {

    element = queue->inputCommands[front];

    if (queue->front == queue->rear) {
      queue->front = -1;
      queue->rear = -1;
    } 
    // Q has only one element, so we reset the 
    // queue after dequeing it. ?
    else 
      queue->front = (queue->front + 1) % SIZE;
    return element;
  }
}

// Display the queue
void display(CircularQueue *queue) {

  int i;

  if (isEmpty(queue))

    printf("DEBUG (QUEUE): Empty Queue\n");

  else {

    printf("DEBUG (QUEUE): Front -> %d ", queue->front);
    printf("DEBUG (QUEUE): Items -> ");

    for (i = queue->front; i != queue->rear; i = (i + 1) % SIZE) {
      printf("%d ", queue->inputCommands[i]);
    }

    printf("%d ", queue->inputCommands[i]);
    printf("DEBUG (QUEUE): Rear -> %d \n", queue->rear);
  }
}

// destroy the queue
void destroyQueue(CircularQueue *queue) {
	free(queue);
}