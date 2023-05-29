#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include "parse_configuration.h"

Producer_Info* producers = NULL;
int coEditorQueueSize = 0;
int numProducers = 0;

int read_line(int fd) {
char buffer[100];
char c;
int bytesRead = 0;

while (read(fd, &c, 1) == 1 && c != '\n') {
    buffer[bytesRead++] = c;
}
buffer[bytesRead] = '\0';

if (bytesRead == 0) 
{
    return -1;
}

int number = atoi(buffer);
return number;
}


void parse_file(char* file_path)
{
int fd = open(file_path, O_RDONLY);
if (fd < 0) {
    perror("Failed to open the configuration file");
    exit(1);
}
int producerId, numProducts, queueSize;
char delimiter;
int first_num,second_num,third_num;

while (1) {
    first_num = read_line(fd);
    if (first_num == -1)
        break;

    // Read the second number
    second_num = read_line(fd);
    if (second_num == -1)
        break;

    // Read the third number
    third_num = read_line(fd);
    if (third_num == -1)
        break;

    producerId=first_num-1;
    numProducts=second_num;
    queueSize=third_num;

    // Create a new producer configuration
    Producer_Info producer;
    producer.producerId = producerId;
    producer.numProducts = numProducts;
    producer.queueSize = queueSize;

    // Resize the producers array
    numProducers++;
    producers = realloc(producers, numProducers * sizeof(Producer_Info));

    // Add the new producer configuration to the array
    producers[numProducers - 1] = producer;
    delimiter=read(fd, &delimiter, 1);
}

coEditorQueueSize=first_num;

close(fd);


}