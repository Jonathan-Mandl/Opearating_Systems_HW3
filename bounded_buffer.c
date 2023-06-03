#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bounded_buffer.h"

void insert(struct Bounded_Buffer *queue, char* message) {
     if (sem_wait(&queue->empty) == -1)
        {
            perror("Failed to wait for semaphore");
            exit(-1);
        }
        // lock producer_mutex
        if (sem_wait(&queue->mutex) == -1)
        {
            perror("Failed to wait for semaphore");
            exit(-1);
        }

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

        // unlock producer_mutex
        if (sem_post(&queue->mutex) == -1)
        {
            perror("Failed to post for semaphore");
            exit(-1);
        }
        // up to counting semaphore of producer_full cells.
        if (sem_post(&queue->full) == -1)
        {
            perror("Failed to post for semaphore");
            exit(-1);
        }
}

char* remove_message(struct Bounded_Buffer *queue) {
    // decrement counting semaphores of producer_full cells
            if (sem_wait(&queue->full) == -1)
            {
                perror("Failed to wait for semaphore");
                exit(-1);
            }
            // lock producer_mutex.
            if (sem_wait(&queue->mutex) == -1)
            {
                perror("Failed to wait for semaphore");
                exit(-1);
            }

            //check if queue is empty.
            if (queue->front == -1 || queue->front > queue->rear) {
                printf("Queue is empty. Cannot remove element.\n");
                return NULL;
            }
            //add messages to messages array of queue.
            char* item = queue->messages[queue->front];

            // Reorganize the array by shifting elements to the left
            for (int i = 1; i <= queue->rear; i++) {
                queue->messages[i - 1] = queue->messages[i];
            }

            queue->rear -= 1;

            // unlock producer_mutex
            if (sem_post(&queue->mutex) == -1)
            {
                perror("Failed to post for semaphore");
                exit(-1);
            }
            // up to empty cells semaphore
            if (sem_post(&queue->empty) == -1)
            {
                perror("Failed to post for semaphore");
                exit(-1);
            }


    return item;
}

Bounded_Buffer* create_bounded_buffer(int capacity) {
    Bounded_Buffer *queue = (Bounded_Buffer*)malloc(sizeof(Bounded_Buffer));
    queue->messages = (char**)malloc(capacity * sizeof(char*));
    queue->front = -1;
    queue->rear = -1;
    queue->capacity = capacity;
    if (sem_init(&queue->mutex, 0, 1) == -1)
    {
        perror("Failed to initialize semaphore");
        exit(-1);
    }
    if (sem_init(&queue->full, 0, 0) == -1)
    {
        perror("Failed to initialize semaphore");
        exit(-1);
    }
    if (sem_init(&queue->empty, 0, capacity) == -1)
    {
        perror("Failed to initialize semaphore");
        exit(-1);
    }
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

int is_empty(struct Bounded_Buffer* queue){
    return queue->front == -1 || queue->front > queue->rear;
}