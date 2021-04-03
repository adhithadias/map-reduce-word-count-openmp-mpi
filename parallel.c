#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <omp.h>
#include "util/queue.h"
#include "util/hashTable.h"
#include "util/util.h"

#define NUM_THREADS 16
#define NUM_FILES 30

extern int errno ;

void populateQueue(struct Queue *q, char *file_name) {
    // file open operation
    FILE* filePtr;
    if ( (filePtr = fopen(file_name, "r")) == NULL) {
        fprintf(stderr, "could not open file: [%p], err: %d, %s\n", filePtr, errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

    // read line by line from the file and add to the queue
    size_t len = 0;
    char *line = NULL;
    int line_count = 0;
    while (getline(&line, &len, filePtr) != -1)
    {
        enQueue(q, line, len);
        line_count++;
    }
    // printf("line count %d, %s\n", line_count, file_name);
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

void reduce(struct hashtable **hash_tables, struct hashtable *final_table, int file_count, int location) {
    struct node *node = NULL;
    int i;
    for (i=0; i<file_count; i++) {
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
    omp_lock_t writelock;
    omp_init_lock(&writelock);

    double time = -omp_get_wtime();

    int file_count = 0;

    struct Queue *file_name_queue;
    file_name_queue = createQueue();
    file_count = get_file_list(file_name_queue);
    printf("file_count %d\n", file_count);

    struct Queue **queues;
    struct hashtable **hash_tables;

    queues = (struct Queue**) malloc(sizeof(struct Queue*)*file_count);
    hash_tables = (struct hashtable**) malloc(sizeof(struct hashtable*)*file_count);

    // consider allocating the memory before execution and during execution
    // there maybe few cache misses depending on the 2 different approaches
 
    int i;
    // consider using sections and run both the reading and mapping
    // at the same time, a variable can be used in the queue to 
    // indicate the reader status -- whether reading is finished or not
    #pragma omp parallel for shared(queues, hash_tables)
    for (i=0; i<file_count; i++) {
        queues[i] = createQueue();

        char file_name[30];
        omp_set_lock(&writelock);
        strcpy(file_name, file_name_queue->front->line);
        deQueue(file_name_queue);
        omp_unset_lock(&writelock);

        populateQueue(queues[i], file_name);
        queues[i]->finished = 1;

        hash_tables[i] = createtable(CAPACITY);
        populateHashMap(queues[i], hash_tables[i]);
    }
    omp_destroy_lock(&writelock);

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
            reduce(hash_tables, final_table, file_count, i);
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
    for (i=0; i<file_count; i++) {
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
