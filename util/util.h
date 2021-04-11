
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include "queue.h"

extern int errno;
extern int DEBUG_MODE;

#define FILE_NAME_BUF_SIZE 50

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
        enQueue(file_name_queue, file_name, strlen(file_name));
        file_count++;
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
    for (int i = 0; i < file_count; i++)
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

int process_args(int argc, char **argv, char *files_dir, int *repeat_files, int *DEBUG_MODE, int *HASH_SIZE,
                 int *QUEUE_TABLE_COUNT)
{
    // https://stackoverflow.com/questions/17877368/getopt-passing-string-parameter-for-argument
    int opt;
    while ((opt = getopt(argc, argv, "d:r:h:q:g")) != -1)
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
        case 'g':
            printf("Running in debug mode\n");
            *DEBUG_MODE = 1;
            break;
        case ':':
            fprintf(stderr, "Option -%c requires an argument to be given\n", optopt);
            return -1;
        }
    }
    return 0;
}