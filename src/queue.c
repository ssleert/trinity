#include "queue.h"
#include <stdio.h>
#include <stdlib.h>

// Function to create a new node
QueueNode* createQueueNode(void* data) {
    QueueNode* newNode = (QueueNode*)malloc(sizeof(QueueNode));
    if (!newNode) {
        return NULL;
    }
    newNode->data = data;
    newNode->next = NULL;
    return newNode;
}

// Function to initialize a queue
Queue* createQueue(void) {
    Queue* queue = (Queue*)malloc(sizeof(Queue));
    if (!queue) {
        return NULL;
    }
    queue->front = queue->rear = NULL;
    return queue;
}

// Function to check if the queue is empty
int isEmpty(Queue* queue) {
    return queue->front == NULL;
}

// Function to enqueue an element
void enqueue(Queue* queue, void* data) {
    QueueNode* newNode = createQueueNode(data);
    if (queue->rear == NULL) {
        queue->front = queue->rear = newNode;
        return;
    }
    queue->rear->next = newNode;
    queue->rear = newNode;
}

// Function to dequeue an element
void* dequeue(Queue* queue) {
    if (isEmpty(queue)) {
        printf("Queue is empty\n");
        return NULL;
    }
    QueueNode* temp = queue->front;
    void* data = temp->data;
    queue->front = queue->front->next;

    if (queue->front == NULL)
        queue->rear = NULL;

    free(temp);
    return data;
}

