#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bounded_buffer.h"

void insert(struct Bounded_Buffer *queue, const char* message) {
    //check if queue is full.
    if (queue->rear == queue->capacity - 1) {
        printf("Queue is full. Cannot insert element.\n");
        return;
    }
    if (queue->front == -1)
        queue->front = 0;
    // increase rear value because new message was added.
    queue->rear++;
    queue->messages[queue->rear] = strdup(message);
}

const char* remove_message(struct Bounded_Buffer *queue) {
    //check if queue is empty.
    if (queue->front == -1 || queue->front > queue->rear) {
        printf("Queue is empty. Cannot remove element.\n");
        return NULL;
    }
    //add messages to messages array of queue.
    const char* item = queue->messages[queue->front];

    // Reorganize the array by shifting elements to the left
    for (int i = 1; i <= queue->rear; i++) {
        queue->messages[i - 1] = queue->messages[i];
    }

    queue->rear -= 1;
    queue->front = 0;

    return item;
}

Bounded_Buffer* create_bounded_buffer(int capacity) {
    Bounded_Buffer *queue = (Bounded_Buffer*)malloc(sizeof(Bounded_Buffer));
    queue->messages = (char**)malloc(capacity * sizeof(char*));
    queue->front = -1;
    queue->rear = -1;
    queue->capacity = capacity;
    queue->insert = &insert;
    queue->remove_message = &remove_message;
    return queue;
}

void destroy_bounded_buffer(struct Bounded_Buffer* queue) {
    if (queue == NULL) {
        return;
    }

    for (int i = 0; i <= queue->rear; i++) {
        free(queue->messages[i]);
        queue->messages[i] = NULL;
    }

    free(queue->messages);
    queue->messages = NULL;

    free(queue);
}


