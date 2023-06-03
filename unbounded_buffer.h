#include <semaphore.h>
#ifndef UNBOUNDED_BUFFER_H
#define UNBOUNDED_BUFFER_H
struct Node {
    char message[100];
    struct Node* next;
};

typedef struct Unbounded_Buffer {
    struct Node* front;
    struct Node* rear;
    sem_t mutex;
    sem_t full;
} Unbounded_Buffer;

struct Unbounded_Buffer* unbounded_buffer();

void insert_unbounded(struct Unbounded_Buffer* buffer, const char* message);

char* remove_message_unbounded(struct Unbounded_Buffer* buffer);

void destroy_unbounded_buffer(struct Unbounded_Buffer* buffer);

#endif