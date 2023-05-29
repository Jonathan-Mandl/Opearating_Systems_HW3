#ifndef BOUNDED_BUFFER_H
#define BOUNDED_BUFFER_H
typedef struct Bounded_Buffer {
    char **messages;
    int front;
    int rear;
    int capacity;
    void (*insert)(struct Bounded_Buffer*, const char*);
    const char* (*remove_message)(struct Bounded_Buffer*);
} Bounded_Buffer;

void insert(struct Bounded_Buffer *queue, const char* message);

const char* remove_message(struct Bounded_Buffer *queue);

Bounded_Buffer* create_bounded_buffer(int capacity);

void destroy_bounded_buffer(Bounded_Buffer *queue);


#endif