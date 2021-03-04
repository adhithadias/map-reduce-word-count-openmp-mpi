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

<<<<<<< HEAD
void reduce(struct hashtable **hash_tables, struct hashtable *final_table, int location) {
    struct node *node = NULL;
    for (int i=0; i<NUM_FILES; i++) {
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

=======
>>>>>>> efdc11fe9964de1ff44605c78dea8c132fddbc25
int main(int argc, char **argv) {

    omp_set_num_threads(NUM_THREADS);

    double time = -omp_get_wtime();

    struct Queue **queues;
    struct hashtable **hash_tables;

<<<<<<< HEAD
    queues = (struct Queue**) malloc(sizeof(struct Queue*)*NUM_FILES);
    hash_tables = (struct hashtable**) malloc(sizeof(struct hashtable*)*NUM_FILES);
=======
    queues = (struct Queue**) malloc(sizeof(struct Queue*)*15);
    hash_tables = (struct hashtable**) malloc(sizeof(struct hashtable*)*15);
>>>>>>> efdc11fe9964de1ff44605c78dea8c132fddbc25

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

<<<<<<< HEAD
    // #pragma omp parallel sections
    // {
    //     #pragma omp parallel section // reading
    //     {
    //         for (i=0; i<NUM_FILES; i++) {
    //             queues[i] = createQueue();
    //             populateQueue(queues[i], i+1);
    //         }

    //     }
    //     #pragma omp parallel section // mappint
    //     {
    //         for (i=0; i<NUM_FILES; i++) {
    //             hash_tables[i] = createtable(50000);
    //             populateHashMap(queues[i], hash_tables[i]);
    //         }

    //     }

    // }

    struct hashtable *final_table = createtable(50000);
    // add reduction section here
    #pragma omp parallel shared(final_table, hash_tables)
    {
        int threadn = omp_get_thread_num();
        int tot_threads = omp_get_num_threads();
        for (int i=threadn; i<50000; i+=tot_threads) {
            // int location_in_table = i*omp_get_thread_num();
            // printf("i: %d, threadn: %d, tot_threads: %d\n", i, threadn, tot_threads);
            if (i<50000) {
                reduce(hash_tables, final_table, i);
            }
        }
    }

    // struct hashtable *final_table = createtable(50000);
    // // int threadn = omp_get_thread_num();
    // // int tot_threads = omp_get_num_threads();
    // for (int i=0; i<50000; i++) {
    //     // int location_in_table = i*omp_get_thread_num();
    //     // printf("i: %d, threadn: %d, tot_threads: %d\n", i, threadn, tot_threads);
    //     if (i<50000) {
    //         reduce(hash_tables, final_table, i);
    //     }
    // }
=======
    // add reduction section here
>>>>>>> efdc11fe9964de1ff44605c78dea8c132fddbc25

    // clear the heap allocations
    #pragma omp parallel for
    for (int i=0; i<NUM_FILES; i++) {
        free(queues[i]);
        // printTable(hash_tables[i]);
        free(hash_tables[i]);
    }
    free(queues);
    free(hash_tables);
<<<<<<< HEAD

    // printTable(final_table);

    time += omp_get_wtime();
    printf("total time taken for the execution: %f\n", time);
    

=======
    
    time += omp_get_wtime();
    printf("total time taken for the execution: %f\n", time);
>>>>>>> efdc11fe9964de1ff44605c78dea8c132fddbc25

    return EXIT_SUCCESS;
}
