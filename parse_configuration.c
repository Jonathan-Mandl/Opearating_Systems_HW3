#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include "parse_configuration.h"

Program_Stats *parse_file(char *file_path)
{
    FILE *file;
    char *line = NULL;
    size_t len = 0;

    // Open the file in read mode
    file = fopen(file_path, "r");

    // Check if the file was opened successfully
    if (file == NULL)
    {
        perror("Failed to open the file.\n");
        exit(-1); // Exit the program with an error
    }

    Program_Stats *stats = malloc(sizeof(Program_Stats));
    if (stats == NULL)
    {
        perror("Memory allocation failed!");
        exit(-1);
    }
    // Allocate memory for Program_Stat
    stats->numProducers = 0; // initialize number of producer to 0
    stats->producers = NULL; // initialize producers array

    int producerId, numProducts, queueSize;
    int first_num, second_num, third_num;

    while (1)
    {
        // read first num. break from loop if file ended or line empty
        if (getline(&line, &len, file) == -1)
        {
            break;
        }
        if (line[0] == '\n' || line[0] == ' ')
        {
            break;
        }
        first_num = atoi(line); //convert string line to number

        // Read the second num. break from loop if file ended or line empty
        if (getline(&line, &len, file) == -1)
        {
            break;
        }
        if (line[0] == '\n' || line[0] == ' ')
        {
            break;
        }
        second_num = atoi(line);

        // Read the third number. break from loop if file ended or line empty
        if (getline(&line, &len, file) == -1)
        {
            break;
        }
        if (line[0] == '\n' || line[0] == ' ')
        {
            break;
        }
        third_num = atoi(line);

        producerId = first_num - 1;
        numProducts = second_num;
        queueSize = third_num;

        // Create a new producer Info instance
        Producer_Info producer;
        producer.producerId = producerId;
        producer.numProducts = numProducts;
        producer.queueSize = queueSize;

        stats->numProducers++;                                                                     // add 1 to number of producers
        stats->producers = realloc(stats->producers, stats->numProducers * sizeof(Producer_Info)); // Resize the producers array
        if (stats->producers == NULL)
        {
            perror("Memory allocation failed!");
            exit(-1);
        }

        // Add the new producer Info to the producers array
        stats->producers[stats->numProducers - 1] = producer;

        getline(&line, &len, file); // read space between every group of 3 lines
    }

    stats->coEditorQueueSize = first_num; // define co-editor queue size as the first number read before we broke from loop when file ended

    fclose(file); //close file

    free(line); //free dynamic memory

    return stats;
}