#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <omp.h>
#include "util/queue.h"
#include "util/hashTable.h"
#include "util/util.h"

extern int errno;

void populateQueue(struct Queue *q, char *file_name)
{
    // file open operation
    FILE *filePtr;
    if ((filePtr = fopen(file_name, "r")) == NULL)
    {
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

void populateHashMap(struct Queue *q, struct hashtable *hashMap)
{
    struct node *node = NULL;
    while (q->front)
    {
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

void reduce(struct hashtable **hash_tables, struct hashtable *final_table, int file_count, int location)
{
    struct node *node = NULL;
    int i;
    for (i = 0; i < file_count; i++)
    {
        if (hash_tables[i] == NULL || hash_tables[i]->table[location] == NULL)
        {
            continue;
        }
        // if (final_table->table[location] == NULL) {
        //     final_table->table[location] = hash_tables[i]->table[location];
        // }

        struct node *current = hash_tables[i]->table[location];
        if (current == NULL)
            continue;

        while (current != NULL)
        {
            node = add(final_table, current->key, 0);
            node->frequency += current->frequency;
            current = current->next;
        }
    }
}

int main(int argc, char **argv)
{
    char files_dir[] = "./files";  // TODO: This should be taken from argv
    int file_count;
    int DEBUG_MODE = 1;

    double global_time = -omp_get_wtime();
    double local_time;

    struct Queue *file_name_queue;
    file_name_queue = createQueue();

    if (DEBUG_MODE) printf("Files Directory: %s\n", files_dir);
    file_count = get_file_list(file_name_queue, files_dir);
    printf("Files in Queue: %d\n", file_count);
    /**********************************************************************************************************/

    /********************** Populating Queues and HashTables by reading files in the FilesQueue ***************/
    if (DEBUG_MODE) printf("\nPopulating Queues and HashTables by reading files in the FilesQueue\n");
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
    if (DEBUG_MODE) printf("Done Populating! Time taken: %f\n", local_time);
    /**********************************************************************************************************/

    /********************** Reducing the populated words in HashTable *****************************************/
    if (DEBUG_MODE) printf("\nReducing the populated words in HashTable\n");
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
    if (DEBUG_MODE) printf("Done Reducing! Time taken: %f\n", local_time);
    /**********************************************************************************************************/

    /********************** Writing Reduced counts to output file *********************************************/
    if (DEBUG_MODE) printf("\nWriting Reduced counts to output file\n");
    local_time = -omp_get_wtime();
    // printTable(final_table);
    writeFullTable(final_table, "./output/serial/0.txt");
    local_time += omp_get_wtime();
    if (DEBUG_MODE) printf("Done Writing! Time taken: %f\n", local_time);
    /**********************************************************************************************************/

    /********************** Clearing the heap allocations for Queues and HashTables ***************************/
    if (DEBUG_MODE) printf("\nClearing the heap allocations for Queues and HashTables\n");
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
    if (DEBUG_MODE) printf("Done Clearing! Time taken: %f\n", local_time);
    /**********************************************************************************************************/

    global_time += omp_get_wtime();
    printf("\nTotal time taken for the execution: %f\n", global_time);

    return EXIT_SUCCESS;
}
