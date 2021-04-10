
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include "queue.h"

extern int errno;

#define FILE_NAME_BUF_SIZE 50

int get_file_list(struct Queue *file_name_queue, char* dirpath)
{
    DIR *dir;
    struct dirent *in_file;

    char dirname[256] = "./files/";
    // strcpy(dirname, dirpath);  // TODO: Currently dirpath variable is dummy. Use the input from this variable
    int file_count = 0;

    // reference:
    // https://stackoverflow.com/questions/11736060/how-to-read-all-files-in-a-folder-using-c
    if ((dir = opendir(dirname)) == NULL)
    {
        fprintf(stderr, "Error : Failed to open input directory - %s\n", strerror(errno));
        return 0;
    }
    while ((in_file = readdir(dir)))
    {
        /* we don't want current and parent directories */
        if (!strcmp(in_file->d_name, ".") || !strcmp(in_file->d_name, "..") ||
            !strcmp(in_file->d_name, "./") || !strcmp(in_file->d_name, "../"))
            continue;

        /* Open directory entry file for common operation */
        char *file_name = (char *)malloc(sizeof(char) * FILE_NAME_BUF_SIZE);
        strcpy(file_name, dirname);
        strcat(file_name, in_file->d_name);
        enQueue(file_name_queue, file_name, strlen(file_name));
        file_count++;
    }
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
    int i;
    for (i = 0; i < len; i++)
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