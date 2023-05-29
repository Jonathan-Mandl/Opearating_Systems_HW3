#ifndef PARSE_CONFIGURATION_H
#define PARSE_CONFIGURATION_H
  
typedef struct Producer_Info{
    int producerId;
    int numProducts;
    int queueSize;
} Producer_Info;

extern Producer_Info *producers;
extern int coEditorQueueSize;
extern int numProducers;

int read_line(int fd);
    
void parse_file(char* file_path);

#endif