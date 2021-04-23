
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include "queue.h"
#include <time.h>

extern int errno;
extern int DEBUG_MODE;

#define FILE_NAME_BUF_SIZE 50

void delay(int milli_seconds)
{  
    // Storing start time
    clock_t start_time = clock();
    // looping till required time is not achieved
    while (clock() < start_time + milli_seconds);
}

int get_file_list(struct Queue *file_name_queue, char *dirpath)
{
    DIR *dir;
    struct dirent *in_file;

    char dirname[FILE_NAME_BUF_SIZE];
    // Assuming Linux only. Null character needs to be added to avoid garbage
    char directory_seperator[2] = "/\0";
    strcpy(dirname, dirpath);
    int file_count = 0;

    // reference:
    // https://stackoverflow.com/questions/11736060/how-to-read-all-files-in-a-folder-using-c
    if ((dir = opendir(dirname)) == NULL)
    {
        fprintf(stderr, "Error : Failed to open input directory - %s\n", strerror(errno));
        return -1;
    }
    while ((in_file = readdir(dir)))
    {
        /* we don't want current and parent directories */
        if (!strcmp(in_file->d_name, ".") || !strcmp(in_file->d_name, "..") ||
            !strcmp(in_file->d_name, "./") || !strcmp(in_file->d_name, "../"))
            continue;

        /* Open directory entry file for common operation */
        // mallocing 3 times the directory buffer size for file_name
        char *file_name = (char *)malloc(sizeof(char) * FILE_NAME_BUF_SIZE * 3);
        strcpy(file_name, dirname);
        strcat(file_name, directory_seperator);
        strcat(file_name, in_file->d_name);
        if (DEBUG_MODE)
            printf("Queing file: %s\n", file_name);
        #pragma omp critical
        {
            // To be executed only by one thread at a time as there is a single queue
            enQueue(file_name_queue, file_name, strlen(file_name));
            file_count++;
        }
    }
    if (DEBUG_MODE)
        printf("Done Queing all files\n\n");
    closedir(dir);
    return file_count;
}

/**
 * Format string with only lower case alphabetic letters
 */
char *format_string(char *original)
{
    int len = strlen(original);
    char *word = (char *)malloc(len * sizeof(char));
    int c = 0;
    for (int i = 0; i < len; i++)
    {
        if (isalnum(original[i]) || original[i] == '\'')
        {
            word[c] = tolower(original[i]);
            c++;
        }
    }
    word[c] = '\0';
    return word;
}

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
    // wait until queue is good to start. Useful for parallel accesses.
    while (q == NULL)
        continue;
    while (q->front || !q->finished)
    {
        if (q->front == NULL) {
            delay(10);
            continue;
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

void populateQueueWL(struct Queue *q, char *file_name, omp_lock_t *queuelock)
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
        // separated out the node creation to save some time lost due to locking
        struct QNode *temp = newNode(line, len);

        // enQueue section should be locked --------------------------------------------------------------------- lock this
        omp_set_lock(queuelock);
        enQueueData(q, temp);
        omp_unset_lock(queuelock);

        line_count++;
    }
    // printf("line count %d, %s\n", line_count, file_name);
    fclose(filePtr);
    free(line);
}

void populateQueueWL_ML(struct Queue *q, char *file_name, omp_lock_t *queuelock)
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
    int file_done = 0;
    int lines_per_iter = 30;
    int actual_lines;
    struct QNode **temp_nodes;
    temp_nodes = (struct QNode **) malloc(sizeof(struct QNode *) * lines_per_iter);
    while (file_done != 1)
    {
        actual_lines = 0;
        for (int i=0; i<lines_per_iter; i++){
            if (getline(&line, &len, filePtr) == -1) {
                file_done = 1;
                break;
            } else {
                // separated out the node creation to save some time lost due to locking
                temp_nodes[i] = newNode(line, len);
                actual_lines++;
                line_count++;
            }
        }
        omp_set_lock(queuelock);
        for (int i=0; i<actual_lines; i++){
            if (temp_nodes[i] != NULL)
                enQueueData(q, temp_nodes[i]);
        }
        omp_unset_lock(queuelock);

    }
    // printf("line count %d, %s\n", line_count, file_name);
    fclose(filePtr);
    free(line);
}

void populateHashMapWL(struct Queue *q, struct hashtable *hashMap, omp_lock_t *queuelock)
{
    struct node *node = NULL;
    // wait until queue is good to start. Useful for parallel accesses.
    while (q == NULL)
        continue;
    while (q->front || !q->finished)
    {
        // this block should be locked ------------------------------------------------------------------------------//
        omp_set_lock(queuelock);
        if (q->front == NULL) {
            omp_unset_lock(queuelock);
            continue;
        }
        char str[q->front->len];
        strcpy(str, q->front->line);
        struct QNode *temp = deQueueData(q);
        omp_unset_lock(queuelock);

        // separated out freeing part to save some time lost due to locking
        if (temp != NULL) {
            free(temp->line);
            free(temp);
        }

        
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
    }
}

void reduce(struct hashtable **hash_tables, struct hashtable *final_table, int num_hashtables, int location)
{
    struct node *node = NULL;
    for (int i = 0; i < num_hashtables; i++)
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

int process_args(int argc, char **argv, char *files_dir, int *repeat_files, int *DEBUG_MODE, int *PRINT_MODE, 
                 int *HASH_SIZE, int *QUEUE_TABLE_COUNT, int *NUM_THREADS)
{
    // https://stackoverflow.com/questions/17877368/getopt-passing-string-parameter-for-argument
    int opt;
    while ((opt = getopt(argc, argv, "d:r:h:q:t:gp")) != -1)
    {
        switch (opt)
        {
        case 'd':
            printf("Files Directory given: \"%s\"\n", optarg);
            strcpy(files_dir, optarg);
            break;
        case 'r':
            printf("Files to be repeated: %s time(s)\n", optarg);
            *repeat_files = (int)atol(optarg);
            break;
        case 'h':
            printf("Hash Size to use: %s\n", optarg);
            *HASH_SIZE = (int)atol(optarg);
            break;
        case 'q':
            printf("Queue_Table_count to use: %s\n", optarg);
            *QUEUE_TABLE_COUNT = (int)atol(optarg);
            break;
        case 't':
            printf("Threads to use: %s\n", optarg);
            *NUM_THREADS = (int)atol(optarg);
            break;
        case 'g':
            printf("Running in debug mode\n");
            *DEBUG_MODE = 1;
            break;
        case 'p':
            printf("Running in print mode\n");
            *PRINT_MODE = 1;
            break;
        case ':':
            fprintf(stderr, "Option -%c requires an argument to be given\n", optopt);
            return -1;
        }
    }
    return 0;
}