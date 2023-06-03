#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include "unbounded_buffer.h"

//constructor fo runbounded buffer
struct Unbounded_Buffer* unbounded_buffer() {
    struct Unbounded_Buffer* buffer = (struct Unbounded_Buffer*)malloc(sizeof(struct Unbounded_Buffer));
    buffer->front = NULL;
    buffer->rear = NULL;
    return buffer;
}

// Function to insert a message into the buffer
void insert_unbounded(struct Unbounded_Buffer* buffer, const char* message) {
    // Create a new node for the message
    struct Node* newNode = (struct Node*)malloc(sizeof(struct Node));
    strcpy(newNode->message, message);
    newNode->next = NULL;

    if (buffer->rear == NULL) {
        // If the buffer is empty, make the new node the front and rear
        buffer->front = newNode;
        buffer->rear = newNode;
    } else {
        // Append the new node to the rear of the buffer
        buffer->rear->next = newNode;
        buffer->rear = newNode;
    }
}

// Function to remove a message from the buffer
char* remove_message_unbounded(struct Unbounded_Buffer* buffer) {
    if (buffer->front == NULL) {
        // Buffer is empty, return NULL
        return NULL;
    }

    // Get the message from the front node
    char* message = strdup(buffer->front->message);

    // Remove the front node from the buffer
    struct Node* nodeToRemove = buffer->front;
    buffer->front = buffer->front->next;
    free(nodeToRemove);

    if (buffer->front == NULL) {
        // Buffer became empty, update the rear pointer
        buffer->rear = NULL;
    }

    return message;
}


// Function to destroy the buffer and free memory
void destroy_unbounded_buffer(struct Unbounded_Buffer* buffer) {
    struct Node* current = buffer->front;
    while (current != NULL) {
        struct Node* next = current->next;
        free(current);
        current = next;
    }
    buffer->front = NULL;
    buffer->rear = NULL;

      free(buffer);


}
