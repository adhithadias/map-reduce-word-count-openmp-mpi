#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <omp.h>
#include <time.h>
#include "util/queue.h"
#include "util/hashTable.h"
#include "util/util.h"

#define NUM_THREADS 16
#define NUM_FILES 30
#define HASH_CAPACITY 50000

extern int errno;
int DEBUG_MODE = 0;
int PRINT_MODE = 1;

void delay(int milis)
{
    // Storing start time
    clock_t start_time = clock();

    // looping till required time is not achieved
    while (clock() < start_time + milis)
        ;
}

void populateHashMap2(struct Queue *q, struct hashtable *hashMap)
{
    struct node *node = NULL;

    // wait until queue is good to start
    while (q == NULL)
    {
        delay(5);
        continue;
    }

    int count = 0;
    while (q->front != NULL || !q->finished)
    {
        if (q->front == NULL)
        {
            printf("map waiting for data ..\n");
            delay(5);
            continue;
        }
        count++;
        if (count % 10000 == 0)
        {
            printf(".");
        }

        char str[q->front->len];
        strcpy(str, q->front->line);
        char *token;
        char *rest = str;

        // https://www.geeksforgeeks.org/strtok-strtok_r-functions-c-examples/
        while ((token = strtok_r(rest, " ", &rest)))
        {

            char *word = format_string(token);

            if (strlen(word) > 0)
            {
                node = add(hashMap, word, 0);
                node->frequency++;
            }
            free(word);
        }

        deQueue(q);
    }
}

int main(int argc, char **argv)
{
    char files_dir[] = "./files"; // TODO: This should be taken from argv

    // omp_set_num_threads(NUM_THREADS*2);
    omp_lock_t readlock;
    omp_init_lock(&readlock);

    double time = -omp_get_wtime();

    int file_count = 0;
    struct Queue *file_name_queue;
    file_name_queue = createQueue();
    file_count = get_file_list(file_name_queue, files_dir);
    printf("file_count %d\n", file_count);

    struct Queue **queues;
    struct hashtable **hash_tables;

    queues = (struct Queue **)malloc(sizeof(struct Queue *) * NUM_THREADS);
    hash_tables = (struct hashtable **)malloc(sizeof(struct hashtable *) * NUM_THREADS);

    // consider allocating the memory before execution and during execution
    // there maybe few cache misses depending on the 2 different approaches

    // have to have the queues otherwise seg fault occurs if map runs first
    for (int i = 0; i < NUM_THREADS; i++)
    {
        queues[i] = createQueue();
    }

    int i;
    omp_set_nested(1); /* make sure nested parallism is on */
#pragma omp parallel default(none) shared(queues, file_name_queue, hash_tables, readlock) num_threads(2)
#pragma omp single
    {
#pragma omp task // reading
        {
#pragma omp parallel for num_threads(NUM_THREADS) shared(queues, file_name_queue, readlock)
            for (int i = 0; i < NUM_THREADS; i++)
            { // turn this to threads
                // get file from queue file name and add reading data to the queue
                int threadn = omp_get_thread_num();
                printf("read section thread %d, i %d\n", threadn, i);

                while (file_name_queue->front != NULL)
                {
                    // printf("read section thread %d, i %d\n", threadn, i);
                    char file_name[30];
                    omp_set_lock(&readlock);
                    strcpy(file_name, file_name_queue->front->line);
                    deQueue(file_name_queue);
                    omp_unset_lock(&readlock);
                    // printf("read section thread %d, i %d, f: %s\n", threadn, i, file_name);

                    // printf("read section thread %d, i %d, %s\n", threadn, i, file_name);
                    populateQueue(queues[i], file_name);
                    // printf("read section thread %d, i %d, f: %s\n", threadn, i, file_name);
                }
                queues[i]->finished = 1;
            }
            // int threadn = omp_get_thread_num();
            // printf("2 read section thread %d\n", threadn);
        }
#pragma omp task // mapping
        {
#pragma omp parallel for num_threads(NUM_THREADS) shared(queues, hash_tables)
            for (int i = 0; i < NUM_THREADS; i++)
            {
                int threadn = omp_get_thread_num();
                hash_tables[i] = createtable(50000);
                printf("map section thread %d, i %d\n", threadn, i);
                populateHashMap2(queues[i], hash_tables[i]);
            }
            // int threadn = omp_get_thread_num();
            // printf("2 read section thread %d\n", threadn);
        }
    }
    printf("destroying the lock\n");
    // #pragma omp barrier
    omp_destroy_lock(&readlock);

    // --------------------------------------------------------------------------------------------------------------------

    // --------------------------------------------------------------------------------------------------------------------

    // #pragma omp parallel default(none) shared(queues, file_name_queue, hash_tables, readlock) num_threads(2)
    // #pragma omp single
    // {
    //     #pragma omp task // mapping
    //     {
    //         #pragma omp parallel for num_threads(NUM_THREADS) shared(queues, hash_tables)
    //         for (int i=0; i<NUM_THREADS; i++)  {
    //             int threadn = omp_get_thread_num();
    //             hash_tables[threadn] = createtable(50000);
    //             printf("map section thread %d, i %d\n", threadn, i);
    //             populateHashMap2(queues[i], hash_tables[i]);

    //         }
    //         // int threadn = omp_get_thread_num();
    //         // printf("2 read section thread %d\n", threadn);
    //     }
    // }
    // printf("mapping............ done\n");

    // --
    // #pragma omp parallel for num_threads(NUM_THREADS) shared(hash_tables)
    // for (int i=0; i<NUM_THREADS; i++) {
    //     int threadn = omp_get_thread_num();
    //     printf("reduce section thread %d\n", threadn);

    //     char *filename = (char*) malloc(sizeof(char)*30);
    //     sprintf(filename, "output/parallel/%d.txt", threadn);

    //     writePartialTable(hash_tables[threadn], filename, 0, HASH_CAPACITY);
    // }

    //----------

    struct hashtable *final_table = createtable(HASH_CAPACITY);
// add reduction section here
#pragma omp parallel for shared(final_table, hash_tables) num_threads(NUM_THREADS)
    for (int i = 0; i < NUM_THREADS; i++)
    {
        int threadn = omp_get_thread_num();
        int tot_threads = omp_get_num_threads();
        int interval = HASH_CAPACITY / tot_threads;
        int start = threadn * interval;
        int end = start + interval;
        printf("reduce section thread %d, start %d\n", threadn, start);

        if (end > final_table->tablesize)
        {
            end = final_table->tablesize;
        }

        int i;
        for (i = start; i < end; i++)
        {
            reduce(hash_tables, final_table, NUM_THREADS, i);
        }
    }

#pragma omp parallel for num_threads(NUM_THREADS) shared(final_table)
    for (int i = 0; i < NUM_THREADS; i++)
    {
        int threadn = omp_get_thread_num();
        int tot_threads = omp_get_num_threads();
        int interval = HASH_CAPACITY / tot_threads;
        int start = threadn * interval;
        int end = start + interval;
        if (end > hash_tables[threadn]->tablesize)
        {
            end = final_table->tablesize;
        }
        printf("write thread %d\n", threadn);

        char *filename = (char *)malloc(sizeof(char) * 30);
        sprintf(filename, "output/parallel/%d.txt", threadn);

        writePartialTable(final_table, filename, start, end);
    }

// clear the heap allocations
#pragma omp parallel for
    for (i = 0; i < NUM_THREADS; i++)
    {
        free(queues[i]);
        // printTable(hash_tables[i]);
        free(hash_tables[i]);
    }
    free(queues);
    free(hash_tables);

    // printTable(final_table);

    time += omp_get_wtime();
    printf("total time taken for the execution: %f\n", time);

    return EXIT_SUCCESS;
}
