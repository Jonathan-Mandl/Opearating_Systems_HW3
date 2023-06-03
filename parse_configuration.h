#ifndef PARSE_CONFIGURATION_H
#define PARSE_CONFIGURATION_H
  
typedef struct Producer_Info{
    int producerId;
    int numProducts;
    int queueSize;
} Producer_Info;

typedef struct Program_Stats{
    Producer_Info *producers;
    int coEditorQueueSize;
    int numProducers;
} Program_Stats;

Program_Stats* parse_file(char* file_path);

#endif