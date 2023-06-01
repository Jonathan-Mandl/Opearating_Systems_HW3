#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include "parse_configuration.h"
#include "bounded_buffer.h"
#include "unbounded_buffer.h"

// struct to store the arguments of producer function in order to call function with thread.
typedef struct
{
    int id;
    int num_products;
    int queue_size;
} producer_args;

// global variables from configuration file parsing.
extern Producer_Info *producers; // producers array stores the producer's information.
extern int coEditorQueueSize;    // size of coeditor queue
extern int numProducers;         // number of producers.

// global array to store the queues of all producers. needs to be accessed by producer thread and dispatcher thread.
Bounded_Buffer **Producer_Queues;

// global co-editor shared queue
Bounded_Buffer *co_Editor_Queue;

// global unbounded dispatcher queues
struct Unbounded_Buffer *N_queue;
struct Unbounded_Buffer *S_queue;
struct Unbounded_Buffer *W_queue;

// global arrays to store producers queues sempahores
sem_t *producer_mutex;
sem_t *producer_empty;
sem_t *producer_full;

// global  semaphores for dispatcher queques
sem_t N_mutex;
sem_t N_full;
sem_t S_mutex;
sem_t S_full;
sem_t W_mutex;
sem_t W_full;

// global semaphores for co-editor shared queue.
sem_t co_editor_mutex;
sem_t co_editor_empty;
sem_t co_editor_full;

// function for producer. called by producer thread.
void *producer(void *arg)
{

    // arguments struct of function.
    producer_args *args = (producer_args *)arg;
    // unpack arguments from struct.
    int id = args->id;
    int num_products = args->num_products;
    int queue_size = args->queue_size;

    // counter for messages types.
    int news = 0;
    int sports = 0;
    int weather = 0;

    for (int i = 0; i < num_products; i++)
    {
        char message[100];
        // create beginning of message.
        snprintf(message, sizeof(message), "Producer %d ", id);
        char type[10];
        // decide message type according to modulus function
        if (i % 3 == 0)
        {
            strcpy(type, "NEWS");
            snprintf(message + strlen(message), sizeof(message) - strlen(message), "%s %d\n", type, news);
            news++;
        }
        else if (i % 3 == 1)
        {
            strcpy(type, "SPORTS");
            snprintf(message + strlen(message), sizeof(message) - strlen(message), "%s %d\n", type, sports);
            sports++;
        }
        else
        {
            strcpy(type, "WEATHER");
            snprintf(message + strlen(message), sizeof(message) - strlen(message), "%s %d\n", type, weather);
            weather++;
        }

        // down to counting semaphore of producer_empty cells.
        if (sem_wait(&producer_empty[id]) == -1)
        {
            perror("Failed to wait for semaphore");
            exit(-1);
        }
        // lock producer_mutex
        if (sem_wait(&producer_mutex[id]) == -1)
        {
            perror("Failed to wait for semaphore");
            exit(-1);
        }
        // insert message to queue of producer.
        insert(Producer_Queues[id], message);

        // unlock producer_mutex
        if (sem_post(&producer_mutex[id]) == -1)
        {
            perror("Failed to post for semaphore");
            exit(-1);
        }
        // up to counting semaphore of producer_full cells.
        if (sem_post(&producer_full[id]) == -1)
        {
            perror("Failed to post for semaphore");
            exit(-1);
        }
    }

    // down to counting semaphore of empty cells.
    if (sem_wait(&producer_empty[id]) == -1)
    {
        perror("Failed to wait for semaphore");
        exit(-1);
    }
    // lock producer_mutex
    if (sem_wait(&producer_mutex[id]) == -1)
    {
        perror("Failed to wait for semaphore");
        exit(-1);
    }

    // send done message when producer finished sending all messages.
    insert(Producer_Queues[id], "DONE");

    // unlock producer mutex
    if (sem_post(&producer_mutex[id]) == -1)
    {
        perror("Failed to post for semaphore");
        exit(-1);
    }
    // up to counting semaphore of full cells.
    if (sem_post(&producer_full[id]) == -1)
    {
        perror("Failed to post for semaphore");
        exit(-1);
    }

    free(arg);
    return NULL;
}

char *ExtractMessageType(const char *message)
{
    char *messageType = NULL;

    // Find the third token (message type)
    char *token = strtok((char *)message, " ");
    for (int i = 0; i < 2; i++)
    {
        if (token == NULL)
            return NULL; // Invalid format

        token = strtok(NULL, " ");
    }

    // Extract the message type
    if (token != NULL)
    {
        messageType = token;
    }

    return messageType;
}

void *dispatcher()
{
    // stores how many producers are still active.
    int working_producers = numProducers;
    // array for status of each queue: done/not done
    int *producer_queue_status = malloc(numProducers * sizeof(int));

    // Initialize producer queue status
    for (int i = 0; i < numProducers; i++)
    {
        producer_queue_status[i] = 1; // 1 indicates the queue is active
    }
    // runs while there are still producers who are not done
    while (working_producers)
    {
        for (int i = 0; i < numProducers; i++)
        {
            if (producer_queue_status[i] == 0)
            {
                continue;
            }

            // Check if the queue is empty or not, and if it is empty continue to next iteration of loop.
            if (is_empty(Producer_Queues[i]))
            {
                continue;
            }

            // decrement counting semaphores of producer_full cells
            if (sem_wait(&producer_full[i]) == -1)
            {
                perror("Failed to wait for semaphore");
                exit(-1);
            }
            // lock producer_mutex.
            if (sem_wait(&producer_mutex[i]) == -1)
            {
                perror("Failed to wait for semaphore");
                exit(-1);
            }

            // Queue has message, process it
            char *message = remove_message(Producer_Queues[i]);

            // unlock producer_mutex
            if (sem_post(&producer_mutex[i]) == -1)
            {
                perror("Failed to post for semaphore");
                exit(-1);
            }
            // up to empty cells semaphore
            if (sem_post(&producer_empty[i]) == -1)
            {
                perror("Failed to post for semaphore");
                exit(-1);
            }

            // if producer sends done message, make producer as inactive and continue.
            if (strcmp(message, "DONE") == 0)
            {
                free(message);
                working_producers--;          // decrement number of ative producers.
                producer_queue_status[i] = 0; // Mark the queue as inactive
                continue;
            }

            char *message_unedited = strdup(message); // create copy of message because it will by changed by ExtractMessageType()

            // check message type.
            char *messageType = ExtractMessageType(message);

            // add message to appropriate dispatcher queue based on its type
            if (strcmp(messageType, "SPORTS") == 0) // for sports message type
            {
                if (sem_wait(&S_mutex) == -1)
                {
                    perror("Failed to wait for semaphore"); 
                    exit(-1);
                }

                insert_unbounded(S_queue, message_unedited); // insert meessage to S dispatcher queue

                if(sem_post(&S_mutex)==-1)
                {
                    perror("Failed to wait for semaphore");
                    exit(-1);
                }
                if (sem_post(&S_full)==-1)
                {
                    perror("Failed to post for semaphore");
                    exit(-1);
                }
            }
            else if (strcmp(messageType, "NEWS") == 0) // for news mesage type
            {
                if (sem_wait(&N_mutex) == -1)
                {
                    perror("Failed to wait for semaphore");
                    exit(-1);
                }

                insert_unbounded(N_queue, message_unedited); // insert meessage to N dispatcher queue

                if(sem_post(&N_mutex)==-1)
                {
                    perror("Failed to wait for semaphore");
                    exit(-1);
                }
                if (sem_post(&N_full)==-1)
                {
                    perror("Failed to post for semaphore");
                    exit(-1);
                }
            }
            else if (strcmp(messageType, "WEATHER") == 0) // for weather message type
            {
                if (sem_wait(&W_mutex) == -1)
                {
                    perror("Failed to wait for semaphore");
                    exit(-1);
                }

                insert_unbounded(W_queue, message_unedited); // insert meessage to W dispatcher queue

                if(sem_post(&W_mutex)==-1)
                {
                    perror("Failed to wait for semaphore");
                    exit(-1);
                }
                if (sem_post(&W_full)==-1)
                {
                    perror("Failed to post for semaphore");
                    exit(-1);
                }
            }
            else
            {

            }
            free(message);
            free(message_unedited);
        }
    }
    // if dispatacher finished loop it means that it recieved done message from all its producers.
    // disptacher then sends done message to all its queues.

    //send done message to S dispatcher queue
    if (sem_wait(&S_mutex)==-1)
    {
        perror("Failed to wait for semaphore");
        exit(-1);
    }
    insert_unbounded(S_queue, "DONE");
    if (sem_post(&S_mutex)==-1)
    {
        perror("Failed to post for semaphore");
        exit(-1);
    }
    if (sem_post(&S_full)==-1)
    {
        perror("Failed to post for semaphore");
        exit(-1);
    }

    //send done message to N dispatcher queue
    if (sem_wait(&N_mutex)==-1)
    {
        perror("Failed to wait for semaphore");
        exit(-1);
    }
    insert_unbounded(N_queue, "DONE");
    if (sem_post(&N_mutex)==-1)
    {
        perror("Failed to post for semaphore");
        exit(-1);
    }
    if (sem_post(&N_full)==-1)
    {
        perror("Failed to post for semaphore");
        exit(-1);
    }

    //send done message to W dispatcher queue
    if (sem_wait(&W_mutex)==-1)
    {
        perror("Failed to wait for semaphore");
        exit(-1);
    }

    insert_unbounded(W_queue, "DONE");
    
    if (sem_post(&W_mutex)==-1)
    {
        perror("Failed to post for semaphore");
        exit(-1);
    }
    if (sem_post(&W_full)==-1)
    {
        perror("Failed to post for semaphore");
        exit(-1);
    }

    // free dynamic memory.
    free(producer_queue_status);
    return NULL;
}

void *coEditor_N()
{
    while (1)
    {
        sem_wait(&N_full);
        sem_wait(&N_mutex);
        // remove message from dispatcher queue
        char *message = remove_message_unbounded(N_queue);
        sem_post(&N_mutex);

        // if meesage is not "DONE", block
        if (strcmp(message, "DONE") != 0)
        {
            // block for 0.1 seconds
            usleep(100000);
        }

        sem_wait(&co_editor_empty);
        sem_wait(&co_editor_mutex);

        // send done message to co editors shared queue
        insert(co_Editor_Queue, message);

        sem_post(&co_editor_mutex);
        sem_post(&co_editor_full);

        // if message is done, break from loop
        if (strcmp(message, "DONE") == 0)
        {
            free(message);
            break;
        }
        free(message);
    }
    return NULL;
}
void *coEditor_S()
{
    while (1)
    {
        sem_wait(&S_full);
        sem_wait(&S_mutex);
        // remove message from dispatcher queue
        char *message = remove_message_unbounded(S_queue);
        sem_post(&S_mutex);

        // if meesage is not "DONE", block
        if (strcmp(message, "DONE") != 0)
        {
            // block for 0.1 seconds
            usleep(100000);
        }

        sem_wait(&co_editor_empty);
        sem_wait(&co_editor_mutex);

        // send done message to co editors shared queue
        insert(co_Editor_Queue, message);

        sem_post(&co_editor_mutex);
        sem_post(&co_editor_full);

        if (strcmp(message, "DONE") == 0)
        {
            free(message);
            break;
        }
        free(message);
    }
    return NULL;
}
void *coEditor_W()
{
    while (1)
    {
        sem_wait(&W_full);
        sem_wait(&W_mutex);
        // remove message from dispatcher queue
        char *message = remove_message_unbounded(W_queue);
        sem_post(&W_mutex);

        // if meesage is not "DONE", block
        if (strcmp(message, "DONE") != 0)
        {
            // block for 0.1 seconds
            usleep(100000);
        }

        sem_wait(&co_editor_empty);
        sem_wait(&co_editor_mutex);

        // send done message to co editors shared queue
        insert(co_Editor_Queue, message);

        sem_post(&co_editor_mutex);
        sem_post(&co_editor_full);

        if (strcmp(message, "DONE") == 0)
        {
            free(message);
            break;
        }
        free(message);
    }

    return NULL;
}

void *screen_manager()
{
    int done_counter = 0;
    while (done_counter < 3)
    {
        sem_wait(&co_editor_full);
        sem_wait(&co_editor_mutex);

        char *message = remove_message(co_Editor_Queue);

        sem_post(&co_editor_mutex);
        sem_post(&co_editor_empty);

        if (strcmp(message, "DONE") == 0)
        {
            done_counter++;
            free(message);
            continue;
        }

        write(1, message, strlen(message));

        free(message);
    }

    char *done = "DONE\n";
    write(1, done, strlen(done));


    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Usage: %s <file_path>\n", argv[0]);
        return 1;
    }

    // call function to parse configuration file.
    parse_file(argv[1]);

    // initialize global array of producer queues using dynamic memeory allocation with numProducer size.
    Producer_Queues = (Bounded_Buffer **)malloc(numProducers * sizeof(Bounded_Buffer *));

    // initialize global semaphore arrays
    producer_mutex = (sem_t *)malloc(numProducers * sizeof(sem_t));
    producer_full = (sem_t *)malloc(numProducers * sizeof(sem_t));
    producer_empty = (sem_t *)malloc(numProducers * sizeof(sem_t));

    // initialize producer semaphores and producers queues array;
    for (int i = 0; i < numProducers; i++)
    {
        int queue_size = producers[i].queueSize;
        // create queue for producer and store in global producer queues array that can be accessed also by dispacher
        Producer_Queues[i] = create_bounded_buffer(queue_size);
        // initialize producers queues semaphores.
        if (sem_init(&producer_mutex[i], 0, 1) == -1)
        {
            perror("Failed to initialize semaphore");
            exit(-1);
        }
        if (sem_init(&producer_full[i], 0, 0) == -1)
        {
            perror("Failed to initialize semaphore");
            exit(-1);
        }
        if (sem_init(&producer_empty[i], 0, queue_size) == -1)
        {
            perror("Failed to initialize semaphore");
            exit(-1);
        }
    }

    // initialize dispatcher queues semaphores.
    if (sem_init(&N_mutex, 0, 1) == -1)
    {
        perror("Failed to initialize semaphore");
        exit(-1);
    }
    if (sem_init(&N_full, 0, 0) == -1)
    {
        perror("Failed to initialize semaphore");
        exit(-1);
    }
    if (sem_init(&S_mutex, 0, 1) == -1)
    {
        perror("Failed to initialize semaphore");
        exit(-1);
    }
    if (sem_init(&S_full, 0, 0) == -1)
    {
        perror("Failed to initialize semaphore");
        exit(-1);
    }
    if (sem_init(&W_mutex, 0, 1) == -1)
    {
        perror("Failed to initialize semaphore");
        exit(-1);
    }
    if (sem_init(&W_full, 0, 0) == -1)
    {
        perror("Failed to initialize semaphore");
        exit(-1);
    }

    // initialize co-editor shared queue semaphores
    if (sem_init(&co_editor_mutex, 0, 1) == -1)
    {
        perror("Failed to initialize semaphore");
        exit(-1);
    }
    if (sem_init(&co_editor_full, 0, 0) == -1)
    {
        perror("Failed to initialize semaphore");
        exit(-1);
    }
    if (sem_init(&co_editor_empty, 0, coEditorQueueSize) == -1)
    {
        perror("Failed to initialize semaphore");
        exit(-1);
    }

    // initialize dispatcher queues
    N_queue = unbounded_buffer();
    S_queue = unbounded_buffer();
    W_queue = unbounded_buffer();

    // initialize co-editors shared queue.
    co_Editor_Queue = create_bounded_buffer(coEditorQueueSize);

    // initialize producer threads array
    pthread_t *producer_threads = (pthread_t *)malloc(numProducers * sizeof(pthread_t));

    // initialize dispatcher thread
    pthread_t dispatcher_thread;

    // initilaize coeditor threads.
    pthread_t n_co_editor_thread;
    pthread_t s_co_editor_thread;
    pthread_t w_co_editor_thread;

    // initialize screen manager thread
    pthread_t screen_manager_thread;

    // loop creates producer threads.
    for (int i = 0; i < numProducers; i++)
    {
        // stores producers function arguments is producer_args struct.
        producer_args *args = (producer_args *)malloc(sizeof(producer_args));
        args->id = producers[i].producerId;
        args->num_products = producers[i].numProducts;
        args->queue_size = producers[i].queueSize;

        // create thread for each producers
        if (pthread_create(&producer_threads[i], NULL, producer, args) != 0)
        {
            perror("Thread creation failed");
            return 1;
        }
    }

    // create thread for dispatcher.
    if (pthread_create(&dispatcher_thread, NULL, dispatcher, NULL) != 0)
    {
        perror("Thread creation failed");
        return 1;
    }

    // create co-editor threads
    if (pthread_create(&n_co_editor_thread, NULL, coEditor_N, NULL) != 0)
    {
        perror("Thread creation failed");
        return 1;
    }
    if (pthread_create(&s_co_editor_thread, NULL, coEditor_S, NULL) != 0)
    {
        perror("Thread creation failed");
        return 1;
    }
    if (pthread_create(&w_co_editor_thread, NULL, coEditor_W, NULL) != 0)
    {
        perror("Thread creation failed");
        return 1;
    }

    // create screen manager thread
    if (pthread_create(&screen_manager_thread, NULL, screen_manager, NULL) != 0)
    {
        perror("Thread creation failed");
        return 1;
    }

    // Join the producer threads
    for (int i = 0; i < numProducers; i++)
    {
        if (pthread_join(producer_threads[i], NULL) != 0)
        {
            perror("Thread join failed");
            return 1;
        }
    }

    // Join the dispatcher thread
    if (pthread_join(dispatcher_thread, NULL) != 0)
    {
        perror("Thread join failed");
        return 1;
    }

    // Join the co-editor threads
    if (pthread_join(n_co_editor_thread, NULL) != 0)
    {
        perror("Thread join failed");
        return 1;
    }
    if (pthread_join(s_co_editor_thread, NULL) != 0)
    {
        perror("Thread join failed");
        return 1;
    }
    if (pthread_join(w_co_editor_thread, NULL) != 0)
    {
        perror("Thread join failed");
        return 1;
    }

    // join screen manager thread
    if (pthread_join(screen_manager_thread, NULL) != 0)
    {
        perror("Thread join failed");
        return 1;
    }

    // free dispatcher queues
    destroy_unbounded_buffer(N_queue);
    destroy_unbounded_buffer(S_queue);
    destroy_unbounded_buffer(W_queue);

    // free all producer queues memory.
    for (int i = 0; i < numProducers; i++)
    {
        destroy_bounded_buffer(Producer_Queues[i]);
        sem_destroy(&producer_mutex[i]);
        sem_destroy(&producer_full[i]);
        sem_destroy(&producer_empty[i]);
    }

    // free co editors shared queue
    destroy_bounded_buffer(co_Editor_Queue);

    // free co editor shared queue samphores
    sem_destroy(&co_editor_mutex);
    sem_destroy(&co_editor_full);
    sem_destroy(&co_editor_empty);

    // free dispatcher queus semaphores
    sem_destroy(&N_mutex);
    sem_destroy(&N_full);
    sem_destroy(&S_mutex);
    sem_destroy(&S_full);
    sem_destroy(&W_mutex);
    sem_destroy(&W_full);

    // free dynamnic memory.
    free(Producer_Queues);
    free(producer_mutex);
    free(producer_full);
    free(producer_empty);
    free(producer_threads);
    free(producers);

    return 0;
}