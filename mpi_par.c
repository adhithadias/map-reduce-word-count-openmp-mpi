#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <mpi.h>
#include "util/queue.h"

#define FILE_NAME_BUF_SIZE 50


void get_file_list(struct Queue *file_name_queue) {
    DIR* dir;
    struct dirent* in_file;

    const char dirname[] = "./files/";

    // reference: https://stackoverflow.com/questions/11736060/how-to-read-all-files-in-a-folder-using-c
    if ((dir = opendir (dirname)) == NULL)  {
        fprintf(stderr, "Error : Failed to open input directory - %s\n", strerror(errno));
        return;
    }
    while ((in_file = readdir(dir))) {
        /* we don't want current and parent directories */
        if (!strcmp (in_file->d_name, "."))
            continue;
        if (!strcmp (in_file->d_name, ".."))    
            continue;
        
        /* Open directory entry file for common operation */
        char *file_name = (char *) malloc(sizeof(char)*FILE_NAME_BUF_SIZE);
        strcpy(file_name, dirname);
        strcat(file_name, in_file->d_name);
        enQueue(file_name_queue, file_name, strlen(file_name));
       
    }
}

int main(int argc, char **argv) {

    MPI_Init(NULL, NULL);
    int size, pid, p_name_len;
    char p_name[MPI_MAX_PROCESSOR_NAME];
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &pid);
    MPI_Get_processor_name(p_name, &p_name_len);

    /* file outputs for processes */
    // declare a file
    FILE* outfile;
    // open a file whose name is based on the pid
    char buf[10]; 
    snprintf(buf, 10, "./pout/f%d", pid);
    outfile = fopen(buf, "w");

    if (pid == 0) {
        struct Queue *file_name_queue = createQueue();
        get_file_list(file_name_queue);

        FILE *entry_file;

        while(file_name_queue->front) {
            entry_file = fopen(file_name_queue->front->line, "r");
            if (entry_file == NULL)
            {
                fprintf(outfile, "File or directory - %s\n", file_name_queue->front->line);
                fprintf(outfile, "Error : Failed to open entry file - %s\n", strerror(errno));
            } else {
                fprintf(outfile, "File or directory - %s\n", file_name_queue->front->line);
                fclose(entry_file);
            }
            deQueue(file_name_queue);
        }
    } 

    MPI_Finalize();
    return 0;
}