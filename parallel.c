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
    double global_time = -omp_get_wtime();
    double local_time;

    // Parsing User inputs from run command with getopt
    int arg_parse = process_args(argc, argv, files_dir, &repeat_files, &DEBUG_MODE, &PRINT_MODE, &HASH_SIZE,
                                 &QUEUE_TABLE_COUNT, &NUM_THREADS);
    QUEUE_TABLE_COUNT = 1; // Forcing the count to 1
    if (arg_parse == -1)
    {
        printf("Check inputs and rerun! Exiting!\n");
        return 1;
    }

    omp_set_num_threads(NUM_THREADS);
    omp_lock_t writelock;
    omp_init_lock(&writelock);

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

    // TODO: This should be an array as in Prof's lecture (Week 4: Uniform distribution of Elements)
    int files_per_qt = file_count / (QUEUE_TABLE_COUNT * NUM_THREADS);
    if (PRINT_MODE)
    {
        printf("\nNumber of Queues and Tables: %d\n", QUEUE_TABLE_COUNT * NUM_THREADS);
        printf("Number of files per Q&T: %d\n", files_per_qt);
    }
    /********************** Queuing Lines by reading files in the FilesQueue **********************************/
    if (PRINT_MODE)
        printf("\nQueuing Lines by reading files in the FilesQueue\n");
    local_time = -omp_get_wtime();
    struct Queue **queues;
    queues = (struct Queue **)malloc(sizeof(struct Queue *) * NUM_THREADS);
    #pragma omp parallel shared(queues) num_threads(NUM_THREADS)
    {
        int i = omp_get_thread_num();
        queues[i] = createQueue();
        char file_name[FILE_NAME_BUF_SIZE * 3];
        while(file_name_queue->front != NULL) {
            omp_set_lock(&writelock);
            if (file_name_queue->front == NULL) {
                omp_unset_lock(&writelock); 
                continue;
            }
            if (DEBUG_MODE)
                printf("thread: %d, filename: %s\n", i, file_name_queue->front->line);
            strcpy(file_name, file_name_queue->front->line);
            deQueue(file_name_queue);
            omp_unset_lock(&writelock);  

            populateQueue(queues[i], file_name);         
        }
        queues[i]->finished = 1; //TODO: What is this for?
    }
    omp_destroy_lock(&writelock); // TODO: Can keep this lock if another queue needs to be locked below
    local_time += omp_get_wtime();
    if (PRINT_MODE)
        printf("Done Populating lines! Time taken: %f\n", local_time);
    /**********************************************************************************************************/

    /********************** Hashing words by reading words in the LinesQueue **********************************/
    if (PRINT_MODE)
    {
        printf("\nHashing words by reading words in the LinesQueue\n");
        printf("HASH_SIZE: %d\n", HASH_SIZE);
    }
    local_time = -omp_get_wtime();
    struct hashtable **hash_tables;
    hash_tables = (struct hashtable **)malloc(sizeof(struct hashtable *) * NUM_THREADS);
    #pragma omp parallel for shared(queues, hash_tables)
    for (int i = 0; i < NUM_THREADS; i++)
    {
        hash_tables[i] = createtable(HASH_SIZE);
        populateHashMap(queues[i], hash_tables[i]);
    }
    local_time += omp_get_wtime();
    if (PRINT_MODE)
        printf("Done Hashing words! Time taken: %f\n", local_time);
    /**********************************************************************************************************/

    // consider allocating the memory before execution and during execution
    // there maybe few cache misses depending on the 2 different approaches
    // consider using sections and run both the reading and mapping
    // at the same time, a variable can be used in the queue to
    // indicate the reader status -- whether reading is finished or not

    /********************** Reducing the populated words in HashTable *****************************************/
    if (PRINT_MODE)
        printf("\nReducing the populated words in HashTable\n");
    local_time = -omp_get_wtime();
    struct hashtable *final_table = createtable(HASH_SIZE);
    #pragma omp parallel shared(final_table, hash_tables)
    {
        int threadn = omp_get_thread_num();
        int tot_threads = omp_get_num_threads();
        int interval = HASH_SIZE / tot_threads;
        int start = threadn * interval;
        int end = start + interval;
        // Is hash_tables[threadn]->tablesize the same?
        if (end > final_table->tablesize)
        {
            end = final_table->tablesize;
        }
        for (int i = start; i < end; i++)
        {
            reduce(hash_tables, final_table, NUM_THREADS, i);
        }
    }
    local_time += omp_get_wtime();
    if (PRINT_MODE)
        printf("Done Reducing! Time taken: %f\n", local_time);
    /**********************************************************************************************************/

    // printTable(final_table);
    // writeTable(final_table, "output/parallel/0.txt");

    /********************** Writing Reduced counts to output file *********************************************/
    if (PRINT_MODE)
        printf("\nWriting Reduced counts to output file\n");
    local_time = -omp_get_wtime();
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
    local_time += omp_get_wtime();
    if (PRINT_MODE)
        printf("Done Writing! Time taken: %f\n", local_time);
    /**********************************************************************************************************/

    /********************** Clearing the heap allocations for Queues and HashTables ***************************/
    if (DEBUG_MODE)
        printf("\nClearing the heap allocations for Queues and HashTables\n");
    local_time = -omp_get_wtime();
    #pragma omp parallel for
    for (int i = 0; i < QUEUE_TABLE_COUNT * NUM_THREADS; i++)
    {
        free(queues[i]);
        // printTable(hash_tables[i]);
        free(hash_tables[i]);
    }
    free(queues);
    free(hash_tables);
    local_time += omp_get_wtime();
    if (DEBUG_MODE)
        printf("Done Clearing! Time taken: %f\n", local_time);
    /**********************************************************************************************************/

    global_time += omp_get_wtime();
    printf("\nTotal time taken for the execution: %f\n", global_time);

    return EXIT_SUCCESS;
}