#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <omp.h>
#include "util/queue.h"
#include "util/hashTable.h"
#include "util/util.h"

extern int errno;
int DEBUG_MODE = 0;
int PRINT_MODE = 1;

int main(int argc, char **argv)
{
    int NUM_THREADS = 16;
    int HASH_SIZE = 50000;
    int QUEUE_TABLE_COUNT = 1;
    char files_dir[FILE_NAME_BUF_SIZE] = "./files/";
    int file_count = 0;
    int repeat_files = 1;
    double local_time;

    // Parsing User inputs from run command with getopt
    int arg_parse = process_args(argc, argv, files_dir, &repeat_files, &DEBUG_MODE, &HASH_SIZE,
                                 &QUEUE_TABLE_COUNT, &NUM_THREADS);
    if (arg_parse == -1)
    {
        printf("Check inputs and rerun! Exiting!\n");
        return 1;
    }

    omp_set_num_threads(NUM_THREADS);
    omp_lock_t writelock;
    omp_init_lock(&writelock);

    double global_time = -omp_get_wtime();

    /********************** Creating and populating FilesQueue ************************************************/
    struct Queue *file_name_queue;
    file_name_queue = createQueue();

    if (DEBUG_MODE)
    {
        printf("\nQueuing files in Directory: %s\n", files_dir);
        printf("Queuing files %d time(s)\n", repeat_files);
    }
    local_time = -omp_get_wtime();
    for (int i = 0; i < repeat_files; i++)
    {
        int files = get_file_list(file_name_queue, files_dir);
        if (files == -1)
        {
            printf("Check input directory and rerun! Exiting!\n");
            return 1;
        }
        file_count += files;
    }
    local_time += omp_get_wtime();
    if (PRINT_MODE)
        printf("Done Queuing %d files! Time taken: %f\n", file_count, local_time);
    /**********************************************************************************************************/

    struct Queue **queues;
    struct hashtable **hash_tables;

    queues = (struct Queue **)malloc(sizeof(struct Queue *) * file_count);
    hash_tables = (struct hashtable **)malloc(sizeof(struct hashtable *) * file_count);

    // consider allocating the memory before execution and during execution
    // there maybe few cache misses depending on the 2 different approaches

    int i;
    // consider using sections and run both the reading and mapping
    // at the same time, a variable can be used in the queue to
    // indicate the reader status -- whether reading is finished or not
    #pragma omp parallel for shared(queues, hash_tables)
    for (i = 0; i < file_count; i++)
    {
        queues[i] = createQueue();

        char file_name[30];
        omp_set_lock(&writelock);
        strcpy(file_name, file_name_queue->front->line);
        deQueue(file_name_queue);
        omp_unset_lock(&writelock);

        populateQueue(queues[i], file_name);
        queues[i]->finished = 1;

        hash_tables[i] = createtable(HASH_SIZE);
        populateHashMap(queues[i], hash_tables[i]);
    }
    omp_destroy_lock(&writelock);

    struct hashtable *final_table = createtable(HASH_SIZE);
    // add reduction section here
    #pragma omp parallel shared(final_table, hash_tables)
    {
        int threadn = omp_get_thread_num();
        int tot_threads = omp_get_num_threads();
        int interval = HASH_SIZE / tot_threads;
        int start = threadn * interval;
        int end = start + interval;

        if (end > final_table->tablesize)
        {
            end = final_table->tablesize;
        }

        int i;
        for (i = start; i < end; i++)
        {
            reduce(hash_tables, final_table, file_count, i);
        }
    }

    // printTable(final_table);
    // writeTable(final_table, "output/parallel/0.txt");

    #pragma omp parallel shared(final_table)
    {
        int threadn = omp_get_thread_num();
        int tot_threads = omp_get_num_threads();
        int interval = HASH_SIZE / tot_threads;
        int start = threadn * interval;
        int end = start + interval;
        if (end > final_table->tablesize)
        {
            end = final_table->tablesize;
        }
        char *filename = (char *)malloc(sizeof(char) * 30);
        sprintf(filename, "output/parallel/%d.txt", threadn);

        writePartialTable(final_table, filename, start, end);
    }

    // clear the heap allocations
    #pragma omp parallel for
    for (i = 0; i < file_count; i++)
    {
        free(queues[i]);
        // printTable(hash_tables[i]);
        free(hash_tables[i]);
    }
    free(queues);
    free(hash_tables);

    global_time += omp_get_wtime();
    printf("total time taken for the execution: %f\n", global_time);

    return EXIT_SUCCESS;
}
