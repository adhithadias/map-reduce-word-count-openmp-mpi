// https://www.geeksforgeeks.org/queue-linked-list-implementation/
// A C program to demonstrate linked list based implementation of queue 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>

struct QNode { 
	char* line; 
	size_t len;
	struct QNode* next; 
} node; 

struct Queue { 
	struct QNode *front, *rear; 
} queue; 

struct QNode* newNode(char* line, size_t len) 
{ 
	struct QNode* temp = (struct QNode*)malloc(sizeof(struct QNode)); 
	temp->line = (char*) malloc(len*sizeof(char));
	strcpy(temp->line, line);
	temp->len = len;
	temp->next = NULL; 
	return temp; 
} 

struct Queue* createQueue() 
{ 
	struct Queue* q = (struct Queue*)malloc(sizeof(struct Queue)); 
	q->front = q->rear = NULL; 
	return q; 
} 

void enQueue(struct Queue* q, char* line, size_t len) 
{ 
	// Create a new LL node 
	struct QNode* temp = newNode(line, len); 

	// If queue is empty, then new node is front and rear both 
	if (q->rear == NULL) { 
		q->front = q->rear = temp; 
		return; 
	} 

	// Add the new node at the end of queue and change rear 
	q->rear->next = temp; 
	q->rear = temp; 
} 

void deQueue(struct Queue* q) 
{ 
	// If queue is empty, return NULL. 
	if (q->front == NULL) 
		return; 

	// Store previous front and move front one node ahead 
	struct QNode* temp = q->front; 

	q->front = q->front->next; 

	// If front becomes NULL, then change rear also as NULL 
	if (q->front == NULL) 
		q->rear = NULL; 

	free(temp->line);
	free(temp); 
} 


