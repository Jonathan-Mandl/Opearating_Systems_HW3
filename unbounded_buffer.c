#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "unbounded_buffer.h"

// constructor for unbounded buffer
struct Unbounded_Buffer *unbounded_buffer()
{

    struct Unbounded_Buffer *buffer = (struct Unbounded_Buffer *)malloc(sizeof(struct Unbounded_Buffer));
    if (buffer==NULL)
    {
        perror("Memory allocation failed!");
        exit(-1);
    }
    buffer->front = NULL;
    buffer->rear = NULL;
    // initialize all semaphores.
    if (sem_init(&buffer->mutex, 0, 1) == -1)
    {
        perror("Failed to initialize semaphore");
        exit(-1);
    }
    if (sem_init(&buffer->full, 0, 0) == -1)
    {
        perror("Failed to initialize semaphore");
        exit(-1);
    }
    return buffer;
}

// Function to insert a message into the buffer
void insert_unbounded(struct Unbounded_Buffer *buffer, const char *message)
{
    // wait from mutex lock
    if (sem_wait(&buffer->mutex) == -1)
    {
        perror("Failed to wait for semaphore");
        exit(-1);
    }
    // Create a new node for the message
    struct Node *newNode = (struct Node *)malloc(sizeof(struct Node));
    if (newNode == NULL)
    {
        perror("Memory allocation failed!");
        exit(-1);
    }
    strcpy(newNode->message, message);
    newNode->next = NULL;

    if (buffer->rear == NULL)
    {
        // If the buffer is empty, make the new node the front and rear
        buffer->front = newNode;
        buffer->rear = newNode;
    }
    else
    {
        // Append the new node to the rear of the buffer
        buffer->rear->next = newNode;
        buffer->rear = newNode;
    }
    // unlock mutex
    if (sem_post(&buffer->mutex) == -1)
    {
        perror("Failed to wait for semaphore");
        exit(-1);
    }
    // increase semaphore counter of full cells
    if (sem_post(&buffer->full) == -1)
    {
        perror("Failed to post for semaphore");
        exit(-1);
    }
}

// Function to remove a message from the buffer
char *remove_message_unbounded(struct Unbounded_Buffer *buffer)
{
    // wait for semaphore of full cells
    if (sem_wait(&buffer->full) == -1)
    {
        perror("Failed to post for semaphore");
        exit(-1);
    }
    // lock mutex
    if (sem_wait(&buffer->mutex) == -1)
    {
        perror("Failed to wait for semaphore");
        exit(-1);
    }

    if (buffer->front == NULL)
    {
        // Buffer is empty, return NULL
        return NULL;
    }

    // Get the message from the front node
    char *message = strdup(buffer->front->message);

    // Remove the front node from the buffer
    struct Node *nodeToRemove = buffer->front;
    buffer->front = buffer->front->next;
    free(nodeToRemove);

    if (buffer->front == NULL)
    {
        // Buffer became empty, update the rear pointer
        buffer->rear = NULL;
    }
    // unlock mutex
    if (sem_post(&buffer->mutex) == -1)
    {
        perror("Failed to wait for semaphore");
        exit(-1);
    }

    return message;
}

// Function to destroy the buffer and free memory
void destroy_unbounded_buffer(struct Unbounded_Buffer *buffer)
{
    struct Node *current = buffer->front;
    while (current != NULL)
    {
        struct Node *next = current->next;
        free(current);
        current = next;
    }
    buffer->front = NULL;
    buffer->rear = NULL;


    if(sem_destroy(&buffer->mutex)!=0)
    {
        perror("Semaphore destruction failed");
        exit(-1);
    }
    if(sem_destroy(&buffer->full)!=0)
    {
        perror("Semaphore destruction failed");
        exit(-1);
    }

    free(buffer);
}