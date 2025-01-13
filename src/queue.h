#ifndef QUEUE_H
#define QUEUE_H

// Define a node structure for the linked list
typedef struct Node {
    void* data;
    struct Node* next;
} QueueNode;

// Define the queue structure
typedef struct Queue {
    QueueNode* front;
    QueueNode* rear;
} Queue;

QueueNode* createQueueNode(void* data);
Queue* createQueue(void);

int isEmpty(Queue* queue);
void enqueue(Queue* queue, void* data);
void* dequeue(Queue* queue);

#endif
