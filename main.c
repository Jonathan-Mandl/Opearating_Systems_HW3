#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
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

// global array to store the queues of all producers. needs to be accessed by producer thread and dispatcher thread.
Bounded_Buffer **Producer_Queues;

// global co-editor shared queue
Bounded_Buffer *co_Editor_Queue;

// global unbounded dispatcher queues
Unbounded_Buffer *N_queue;
Unbounded_Buffer *S_queue;
Unbounded_Buffer *W_queue;

// function to get message type: sports, weather or news from  message.
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
// function for producer. called by producer thread.
void *producer(void *arg)
{
    // arguments struct of function.
    producer_args *args = (producer_args *)arg;
    // unpack arguments from struct.
    int id = args->id;
    int num_products = args->num_products;

    // counter for messages types.
    int news_counter = 0;
    int sports_counter = 0;
    int weather_counter = 0;

    // Initialize the random number generator
    srand(time(0));

    for (int i = 0; i < num_products; i++)
    {
        char message[100];
        // create beginning of message.
        snprintf(message, sizeof(message), "Producer %d ", id);
        char type[10]; //message type.

        // Generate a random number between 1 and 3
        int randomNum = (rand() % 3) + 1;

        // decide message type according random number generated. contcatenate message type to message.
        if (randomNum == 1)
        {
            strcpy(type, "NEWS");
            snprintf(message + strlen(message), sizeof(message) - strlen(message), "%s %d\n", type, news_counter);
            news_counter++;
        }
        else if (randomNum == 2)
        {
            strcpy(type, "SPORTS");
            snprintf(message + strlen(message), sizeof(message) - strlen(message), "%s %d\n", type, sports_counter);
            sports_counter++;
        }
        else
        {
            strcpy(type, "WEATHER");
            snprintf(message + strlen(message), sizeof(message) - strlen(message), "%s %d\n", type, weather_counter);
            weather_counter++;
        }
        // insert message to queue of producer.
        insert(Producer_Queues[id], message);
    }
    // send done message when producer finished sending all messages.
    insert(Producer_Queues[id], "DONE");

    free(arg); // free dynamic memory used by function
    return NULL;
}

void *dispatcher(void *arg)
{
    int *numProducers = (int *)arg;
    // stores how many producers are still active.
    int working_producers = *numProducers;
    // array for status of each queue: done/not done
    int *producer_queue_status = malloc(*numProducers * sizeof(int));

    if(producer_queue_status==NULL){
           perror("Memory allocation failed!");
           exit(-1);
    }

    // Initialize producer queue status
    for (int i = 0; i < *numProducers; i++)
    {
        producer_queue_status[i] = 1; // 1 indicates the queue is active
    }
    // runs while there are still producers who are not done
    while (working_producers)
    {
        for (int i = 0; i < *numProducers; i++)
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
            // Queue has message, process it
            char *message = remove_message(Producer_Queues[i]);

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
                insert_unbounded(S_queue, message_unedited); // insert meessage to S dispatcher queue
            }
            else if (strcmp(messageType, "NEWS") == 0) // for news mesage type
            {
                insert_unbounded(N_queue, message_unedited); // insert meessage to N dispatcher queue
            }
            else if (strcmp(messageType, "WEATHER") == 0) // for weather message type
            {
                insert_unbounded(W_queue, message_unedited); // insert meessage to W dispatcher queue
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

    // send done message to S dispatcher queue
    insert_unbounded(S_queue, "DONE");
    // send done message to N dispatcher queue
    insert_unbounded(N_queue, "DONE");
    // send done message to W dispatcher queue
    insert_unbounded(W_queue, "DONE");
    // free dynamic memory.
    free(producer_queue_status);
    return NULL;
}

void *coEditor(void *type)
{
    Unbounded_Buffer *queue;
    char *queue_type = (char *)type;

    if (strcmp(queue_type, "N") == 0)
    {
        queue = N_queue;
    }
    else if (strcmp(queue_type, "S") == 0)
    {
        queue = S_queue;
    }
    else
    {
        queue = W_queue;
    }

    while (1)
    {
        // remove message from dispatcher queue
        char *message = remove_message_unbounded(queue);

        // if meesage is not "DONE", block
        if (strcmp(message, "DONE") != 0)
        {
            // block for 0.1 seconds
            usleep(100000);
        }

        // send done message to co editors shared queue
        insert(co_Editor_Queue, message);

        // if message is done, break from loop
        if (strcmp(message, "DONE") == 0)
        {
            free(message);
            break;
        }
        free(message); // free dynamic memory message
    }
    return NULL;
}

void *screen_manager()
{
    int done_counter = 0; // counts number of done messages received
    while (done_counter < 3)
    {
        char *message = remove_message(co_Editor_Queue); // remove message from co-editor shared queue

        if (strcmp(message, "DONE") == 0)
        {
            done_counter++; // up to number of done messages recieved
            free(message);  // free dynamic message memory
            continue;
        }
        write(1, message, strlen(message)); // print message on screen

        free(message); // free message from dynamic memroy
    }

    char *done = "DONE\n";
    write(1, done, strlen(done)); // print done message

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
    Program_Stats *stats = parse_file(argv[1]); // function returns pointer to program_stats struct.

    int numProducers = stats->numProducers;
    int coEditorQueueSize = stats->coEditorQueueSize;
    Producer_Info *producers = stats->producers;

    // initialize global array of producer queues using dynamic memeory allocation with numProducer size.
    Producer_Queues = (Bounded_Buffer **)malloc(numProducers * sizeof(Bounded_Buffer *));
    if(Producer_Queues==NULL)
    {
           perror("Memory allocation failed!");
           exit(-1);
    }

    // initialize producer producers queues array;
    for (int i = 0; i < numProducers; i++)
    {
        int queue_size = producers[i].queueSize;
        // create queue for producer and store in global producer queues array that can be accessed also by dispacher
        Producer_Queues[i] = create_bounded_buffer(queue_size);
    }

    // initialize dispatcher queues
    N_queue = unbounded_buffer();
    S_queue = unbounded_buffer();
    W_queue = unbounded_buffer();

    // initialize co-editors shared queue.
    co_Editor_Queue = create_bounded_buffer(coEditorQueueSize);

    // initialize producer threads array
    pthread_t *producer_threads = (pthread_t *)malloc(numProducers * sizeof(pthread_t));
    if (producer_threads==NULL)
    {
           perror("Memory allocation failed!");
           exit(-1);
    }

    // loop creates producer threads.
    for (int i = 0; i < numProducers; i++)
    {
        // stores producers function arguments is producer_args struct.
        producer_args *args = (producer_args *)malloc(sizeof(producer_args));
        if(args==NULL)
        {
            perror("Memory allocation failed!");
            exit(-1);
        }
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

    // create dispatcher thread
    pthread_t dispatcher_thread;
    if (pthread_create(&dispatcher_thread, NULL, dispatcher, &numProducers) != 0)
    {
        perror("Thread creation failed");
        exit(-1);
    }

    // create N co-editor thread
    pthread_t n_co_editor_thread;
    char N_type[] = "N";
    if (pthread_create(&n_co_editor_thread, NULL, coEditor, (void *)&N_type) != 0)
    {
        perror("Thread creation failed");
        exit(-1);
    }

    // create S co-editor thread
    pthread_t s_co_editor_thread;
    char S_type[] = "S";
    if (pthread_create(&s_co_editor_thread, NULL, coEditor, (void *)&S_type) != 0)
    {
        perror("Thread creation failed");
        exit(-1);
    }

    // create W co-editor thread
    pthread_t w_co_editor_thread;
    char W_type[] = "W";
    if (pthread_create(&w_co_editor_thread, NULL, coEditor, (void *)&W_type) != 0)
    {
        perror("Thread creation failed");
        exit(-1);
    }

    // create screen manager thread
    pthread_t screen_manager_thread;
    if (pthread_create(&screen_manager_thread, NULL, screen_manager, NULL) != 0)
    {
        perror("Thread creation failed");
        exit(-1);
    }

    // Join the producer threads
    for (int i = 0; i < numProducers; i++)
    {
        if (pthread_join(producer_threads[i], NULL) != 0)
        {
            perror("Thread join failed");
            exit(-1);
        }
    }

    // Join the dispatcher thread
    if (pthread_join(dispatcher_thread, NULL) != 0)
    {
        perror("Thread join failed");
        exit(-1);
    }

    // Join the co-editor threads
    if (pthread_join(n_co_editor_thread, NULL) != 0)
    {
        perror("Thread join failed");
        exit(-1);
    }
    if (pthread_join(s_co_editor_thread, NULL) != 0)
    {
        perror("Thread join failed");
        exit(-1);
    }
    if (pthread_join(w_co_editor_thread, NULL) != 0)
    {
        perror("Thread join failed");
        exit(-1);
    }

    // join screen manager thread
    if (pthread_join(screen_manager_thread, NULL) != 0)
    {
        perror("Thread join failed");
        exit(-1);
    }

    // free dispatcher queues
    destroy_unbounded_buffer(N_queue);
    destroy_unbounded_buffer(S_queue);
    destroy_unbounded_buffer(W_queue);

    // free all producer queues memory.
    for (int i = 0; i < numProducers; i++)
    {
        destroy_bounded_buffer(Producer_Queues[i]);
    }
    // free co editors shared queue
    destroy_bounded_buffer(co_Editor_Queue);

    // free dynamnic memory.
    free(Producer_Queues);
    free(producer_threads);
    free(producers);
    free(stats);

    return 0;
}