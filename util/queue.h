// https://www.geeksforgeeks.org/queue-linked-list-implementation/
// A C program to demonstrate linked list based implementation of queue

#ifndef QUEUE_H_INCLUDED
#define QUEUE_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct QNode
{
	char *line;
	size_t len;
	struct QNode *next;
} node;

struct Queue
{
	int finished;
	struct QNode *front, *rear;
} queue;

struct QNode *newNode(char *line, size_t len)
{
	struct QNode *temp = (struct QNode *)malloc(sizeof(struct QNode));
	temp->line = (char *)malloc(len * sizeof(char));
	strcpy(temp->line, line);
	temp->len = len;
	temp->next = NULL;
	return temp;
}

struct Queue *createQueue()
{
	struct Queue *q = (struct Queue *)malloc(sizeof(struct Queue));
	q->front = q->rear = NULL;
	q->finished = 0;
	return q;
}

void enQueueData(struct Queue *q, struct QNode *temp)
{
	// If queue is empty, then new node is front and rear both
	if (q->rear == NULL)
	{
		q->front = q->rear = temp;
		return;
	}

	// Add the new node at the end of queue and change rear
	q->rear->next = temp;
	q->rear = temp;
}

struct QNode *deQueueData(struct Queue *q)
{
	// If queue is empty, return NULL.
	if (q->front == NULL)
		return NULL;

	// Store previous front and move front one node ahead
	struct QNode *temp = q->front;

	q->front = q->front->next;

	// If front becomes NULL, then change rear also as NULL
	if (q->front == NULL)
		q->rear = NULL;

	return temp;
}

void enQueue(struct Queue *q, char *line, size_t len)
{
	// Create a new LL node
	struct QNode *temp = newNode(line, len);
	enQueueData(q, temp);
}

void deQueue(struct Queue *q)
{
	struct QNode *temp = deQueueData(q);
	if (temp == NULL) {
		return;
	}

	free(temp->line);
	free(temp);
}

#endif
