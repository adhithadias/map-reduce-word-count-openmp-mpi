#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <omp.h>
#include <unistd.h>
#include "util/queue.h"
#include "util/hashTable.h"
#include "util/util.h"

extern int errno;
int DEBUG_MODE = 0;
int PRINT_MODE = 1;
int HASH_SIZE = 50000;

int main(int argc, char **argv)
{
    char files_dir[FILE_NAME_BUF_SIZE] = "./files/";
    int file_count = 0;
    int repeat_files = 1;
    double global_time = -omp_get_wtime();
    double local_time;

    // Parsing User inputs from run command with getopt
    int arg_parse = process_args(argc, argv, files_dir, &repeat_files, &DEBUG_MODE);
    if (arg_parse == -1)
    {
        printf("Check inputs and rerun! Exiting!\n");
        return 1;
    }

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

    int queues_tables_count = 1; // This was initially equal to file_count
    // TODO: This should be an array as in Prof's lecture (Week 4: Uniform distribution of Elements)
    int files_per_qt = file_count / queues_tables_count;
    if (PRINT_MODE)
    {
        printf("\nNumber of Queues and Tables: %d\n", queues_tables_count);
        printf("Number of files per Q&T: %d\n", files_per_qt);
    }
    /********************** Queuing Lines by reading files in the FilesQueue **********************************/
    if (PRINT_MODE)
        printf("\nQueuing Lines by reading files in the FilesQueue\n");
    local_time = -omp_get_wtime();
    struct Queue **queues;
    queues = (struct Queue **)malloc(sizeof(struct Queue *) * queues_tables_count);
    for (int i = 0; i < queues_tables_count; i++)
    {
        queues[i] = createQueue();
        for (int j = 0; j < files_per_qt; j++)
        {
            populateQueue(queues[i], file_name_queue->front->line);
            queues[i]->finished = 1; //TODO: What is this for?
            deQueue(file_name_queue);
        }
    }
    local_time += omp_get_wtime();
    if (PRINT_MODE)
        printf("Done Populating lines! Time taken: %f\n", local_time);
    /**********************************************************************************************************/

    /********************** Hashing words by reading words in the LinesQueue **********************************/
    if (PRINT_MODE)
        printf("\nHashing words by reading words in the LinesQueue\n");
    local_time = -omp_get_wtime();
    struct hashtable **hash_tables;
    hash_tables = (struct hashtable **)malloc(sizeof(struct hashtable *) * queues_tables_count);
    for (int i = 0; i < queues_tables_count; i++)
    {
        hash_tables[i] = createtable(HASH_SIZE);
        populateHashMap(queues[i], hash_tables[i]);
    }
    local_time += omp_get_wtime();
    if (PRINT_MODE)
        printf("Done Hashing words! Time taken: %f\n", local_time);
    /**********************************************************************************************************/

    /********************** Reducing the populated words in HashTable *****************************************/
    if (PRINT_MODE)
        printf("\nReducing the populated words in HashTable\n");
    local_time = -omp_get_wtime();
    // struct hashtable *final_table = createtable(HASH_SIZE);
    for (int i = 0; i < HASH_SIZE; i++)
    {
        if (queues_tables_count == 1)
        {
            if (DEBUG_MODE)
                printf("Skipping reduce as the hash_table count is 1");
            break;
        }
        reduce(&hash_tables[1], hash_tables[0], queues_tables_count - 1, i);
    }
    local_time += omp_get_wtime();
    if (PRINT_MODE)
        printf("Done Reducing! Time taken: %f\n", local_time);
    /**********************************************************************************************************/

    /********************** Writing Reduced counts to output file *********************************************/
    if (PRINT_MODE)
        printf("\nWriting Reduced counts to output file\n");
    local_time = -omp_get_wtime();
    // printTable(final_table);
    writeFullTable(hash_tables[0], "./output/serial/0.txt");
    local_time += omp_get_wtime();
    if (PRINT_MODE)
        printf("Done Writing! Time taken: %f\n", local_time);
    /**********************************************************************************************************/

    /********************** Clearing the heap allocations for Queues and HashTables ***************************/
    if (DEBUG_MODE)
        printf("\nClearing the heap allocations for Queues and HashTables\n");
    local_time = -omp_get_wtime();
    for (int i = 0; i < queues_tables_count; i++)
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
