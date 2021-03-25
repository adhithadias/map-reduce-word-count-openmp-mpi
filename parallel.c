#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <omp.h>
#include "util/queue.h"
#include "util/hashTable.h"
#include "util/util.h"

#define NUM_THREADS 4
#define NUM_FILES 30

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

    // wait until queue is good to start
    while (q == NULL) {
        continue;
    }

    while (q->front || !q->finished) {
        
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

void reduce(struct hashtable **hash_tables, struct hashtable *final_table, int location) {
    struct node *node = NULL;
    int i;
    for (i=0; i<NUM_FILES; i++) {
        if (hash_tables[i] == NULL || hash_tables[i]->table[location] == NULL) {
            continue;
        }
        // if (final_table->table[location] == NULL) {
        //     final_table->table[location] = hash_tables[i]->table[location];
        // }

        struct node *current = hash_tables[i]->table[location];
        if(current == NULL) continue;

        while(current != NULL) {
            node = add(final_table, current->key, 0);
            node->frequency += current->frequency;
            current = current->next ;
        }
    }
}

int main(int argc, char **argv) {

    omp_set_num_threads(NUM_THREADS);

    double time = -omp_get_wtime();

    struct Queue **queues;
    struct hashtable **hash_tables;

    queues = (struct Queue**) malloc(sizeof(struct Queue*)*NUM_FILES);
    hash_tables = (struct hashtable**) malloc(sizeof(struct hashtable*)*NUM_FILES);

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
        queues[i]->finished = 1;

        hash_tables[i] = createtable(CAPACITY);
        populateHashMap(queues[i], hash_tables[i]);
    }

    struct hashtable *final_table = createtable(CAPACITY);
    // add reduction section here
    #pragma omp parallel shared(final_table, hash_tables)
    {
        int threadn = omp_get_thread_num();
        int tot_threads = omp_get_num_threads();
        int interval = CAPACITY / tot_threads;
        int start = threadn * interval;
        int end = start + interval;

        if (end > final_table->tablesize) {
          end = final_table->tablesize;
        }

        int i;
        for (i=start; i<end; i++) {
            reduce(hash_tables, final_table, i);
        }
    }

    // printTable(final_table);
    // writeTable(final_table, "output/parallel/0.txt");
    
    #pragma omp parallel shared(final_table)
    {
        int threadn = omp_get_thread_num();
        int tot_threads = omp_get_num_threads();
        int interval = CAPACITY / tot_threads;
        int start = threadn * interval;
        int end = start + interval;
        if (end > final_table->tablesize) {
          end = final_table->tablesize;
        }
        char *filename = (char*) malloc(sizeof(char)*30);
        sprintf(filename, "output/parallel/%d.txt", threadn);

        writePartialTable(final_table, filename, start, end);
    }

    // clear the heap allocations
    #pragma omp parallel for
    for (i=0; i<NUM_FILES; i++) {
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
