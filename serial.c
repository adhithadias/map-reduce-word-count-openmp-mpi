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

int main(int argc, char **argv)
{
    int opt;
    char files_dir[FILE_NAME_BUF_SIZE] = "./files/"; // TODO: This should be taken from argv
    int file_count = 0;
    int repeat_files = 1;
    double global_time = -omp_get_wtime();
    double local_time;

    /********************** Parsing User inputs from run command with getopt **********************************/
    // https://stackoverflow.com/questions/17877368/getopt-passing-string-parameter-for-argument
    while ((opt = getopt(argc, argv, "d:r:g")) != -1)
    {
        switch (opt)
        {
        case 'd':
            printf("Files Directory given: \"%s\"\n", optarg);
            strcpy(files_dir, optarg);
            break;
        case 'r':
            printf("Files to be repeated: %s time(s)\n", optarg);
            repeat_files = (int)atol(optarg);
            break;
        case 'g':
            printf("Running in debug mode\n");
            DEBUG_MODE = 1;
            break;
        case ':':
            fprintf(stderr, "Option -%c requires an argument to be given\n", optopt);
            return 1;
        default:
            break;
        }
    }
    /**********************************************************************************************************/

    /********************** Creating and populating FilesQueue ************************************************/
    struct Queue *file_name_queue;
    file_name_queue = createQueue();

    if (PRINT_MODE)
    {
        printf("\nReading files in Directory: %s\n", files_dir);
        printf("Reading files %d time(s)\n", repeat_files);
    }
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
    printf("Files in Queue: %d\n", file_count);
    /**********************************************************************************************************/

    /********************** Populating Queues and HashTables by reading files in the FilesQueue ***************/
    if (PRINT_MODE)
        printf("\nPopulating Queues and HashTables by reading files in the FilesQueue\n");
    local_time = -omp_get_wtime();
    struct Queue **queues;
    struct hashtable **hash_tables;
    queues = (struct Queue **)malloc(sizeof(struct Queue *) * file_count);
    hash_tables = (struct hashtable **)malloc(sizeof(struct hashtable *) * file_count);
    for (int i = 0; i < file_count; i++)
    {
        queues[i] = createQueue();
        populateQueue(queues[i], file_name_queue->front->line);
        queues[i]->finished = 1;
        deQueue(file_name_queue);
        hash_tables[i] = createtable(50000);
        populateHashMap(queues[i], hash_tables[i]);
    }
    local_time += omp_get_wtime();
    if (PRINT_MODE)
        printf("Done Populating! Time taken: %f\n", local_time);
    /**********************************************************************************************************/

    /********************** Reducing the populated words in HashTable *****************************************/
    if (PRINT_MODE)
        printf("\nReducing the populated words in HashTable\n");
    local_time = -omp_get_wtime();
    struct hashtable *final_table = createtable(50000);
    for (int i = 0; i < 50000; i++)
    {
        if (i < 50000)
        {
            reduce(hash_tables, final_table, file_count, i);
        }
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
    writeFullTable(final_table, "./output/serial/0.txt");
    local_time += omp_get_wtime();
    if (PRINT_MODE)
        printf("Done Writing! Time taken: %f\n", local_time);
    /**********************************************************************************************************/

    /********************** Clearing the heap allocations for Queues and HashTables ***************************/
    if (PRINT_MODE)
        printf("\nClearing the heap allocations for Queues and HashTables\n");
    local_time = -omp_get_wtime();
    for (int i = 0; i < file_count; i++)
    {
        free(queues[i]);
        // printTable(hash_tables[i]);
        free(hash_tables[i]);
    }
    free(queues);
    free(hash_tables);
    local_time += omp_get_wtime();
    if (PRINT_MODE)
        printf("Done Clearing! Time taken: %f\n", local_time);
    /**********************************************************************************************************/

    global_time += omp_get_wtime();
    printf("\nTotal time taken for the execution: %f\n", global_time);

    return EXIT_SUCCESS;
}
