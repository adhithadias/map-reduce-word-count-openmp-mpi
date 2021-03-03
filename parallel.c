#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <omp.h>
#include "util/queue.h"
#include "util/hashTable.h"
#include "util/util.h"

#define NUM_THREADS 4
#define NUM_FILES 15

extern int errno ;

void populateQueue(struct Queue *q, int i) {
    // format file name of the file to open
    char file_name[20] = "./files/";
    char buffer[3];
    sprintf(buffer,"%d",i);
    strcat(file_name, buffer);
    strcat(file_name, ".txt");

    // file open operation
    FILE* filePtr;
    if ( (filePtr = fopen(file_name, "r")) == NULL) {
        fprintf(stderr, "could not open file: [%p], err: %d, %s\n", filePtr, errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

    // read line by line from the file and add to the queue
    size_t len = 0;
    char *line = NULL;
    while (getline(&line, &len, filePtr) != -1) {
        enQueue(q, line, len); 
    }
    fclose(filePtr);
    free(line);
}

void populateHashMap(struct Queue *q, struct hashtable *hashMap) {
    struct node *node = NULL;

    while (q->front) {
        
        char str[q->front->len];
        strcpy(str,q->front->line);
        char *token;
        char *rest = str;

        // https://www.geeksforgeeks.org/strtok-strtok_r-functions-c-examples/
        while ((token = strtok_r(rest, " ", &rest))) {

            char *word = format_string(token);
            
            if(strlen(word) > 0){
                node = add(hashMap, word, 0);
                node->frequency++;
            }
            free(word); 
            
        }

        deQueue(q);
    }
}

int main(int argc, char **argv) {

    omp_set_num_threads(NUM_THREADS);

    double time = -omp_get_wtime();

    struct Queue **queues;
    struct hashtable **hash_tables;

    queues = (struct Queue**) malloc(sizeof(struct Queue*)*15);
    hash_tables = (struct hashtable**) malloc(sizeof(struct hashtable*)*15);

    // consider allocating the memory before execution and during execution
    // there maybe few cache misses depending on the 2 different approaches
 
    int i;
    // consider using sections and run both the reading and mapping
    // at the same time, a variable can be used in the queue to 
    // indicate the reader status -- whether reading is finished or not
    #pragma omp parallel for shared(queues, hash_tables)
    for (i=0; i<NUM_FILES; i++) {
        queues[i] = createQueue();
        populateQueue(queues[i], i+1);

        hash_tables[i] = createtable(50000);
        populateHashMap(queues[i], hash_tables[i]);
    }

    // add reduction section here

    // clear the heap allocations
    #pragma omp parallel for
    for (int i=0; i<NUM_FILES; i++) {
        free(queues[i]);
        // printTable(hash_tables[i]);
        free(hash_tables[i]);
    }
    free(queues);
    free(hash_tables);
    
    time += omp_get_wtime();
    printf("total time taken for the execution: %f\n", time);

    return EXIT_SUCCESS;
}
