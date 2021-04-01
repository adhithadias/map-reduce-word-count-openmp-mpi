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
#define TAG_COMM_REQ_DATA 0
#define TAG_COMM_FILE_NAME 1


int get_file_list(struct Queue *file_name_queue) {
    DIR* dir;
    struct dirent* in_file;

    const char dirname[] = "./files/";
    int file_count = 0;

    // reference: https://stackoverflow.com/questions/11736060/how-to-read-all-files-in-a-folder-using-c
    if ((dir = opendir (dirname)) == NULL)  {
        fprintf(stderr, "Error : Failed to open input directory - %s\n", strerror(errno));
        return 0;
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
        file_count++;
    }
    closedir(dir);
    return file_count;
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
    // outfile = stdout;


    MPI_Request request;
    MPI_Status status;
    int recv_pid;
    int recv_len = 0;

    int count = 0;
    int done = 0;
    int file_count = 0;
    int done_sent_p_count = 0;

    struct Queue *file_name_queue;
    if (pid == 0) {
        file_name_queue = createQueue();
      file_count = get_file_list(file_name_queue);
    }
    MPI_Barrier(MPI_COMM_WORLD);

    while (!done) {
        /*
         * idling process request data from master process (0th process is the master process)
         */
        if (pid != 0) {
            fprintf(outfile, "requesting data to process %d from process 0\n", pid);
            MPI_Send(&pid, 1, MPI_INT, 0, TAG_COMM_REQ_DATA, MPI_COMM_WORLD);
            fprintf(outfile, "send finished\n");
        } else {      
            fprintf(outfile, "process 0 is waiting for a request..\n");
            MPI_Recv(&recv_pid, 1, MPI_INT, MPI_ANY_SOURCE,
                    TAG_COMM_REQ_DATA, MPI_COMM_WORLD, &status);
        }

        /*
         * Master process sends file name to the idling process and dequeue the file name
         * If all files are sent, then the master process sends only one character
         * If slave process receives only 1 character for file name, it knows that
         * master process has finished sending all the files to slave processes
         */
        if (pid == 0) {
            fprintf(outfile, "process 0 received request from process %d\n", recv_pid);

            // send back to the message received process
            fprintf(outfile, "sending file name to recv_pid %d from process 0\n", recv_pid);
            if (count < file_count) {
              MPI_Send(file_name_queue->front->line,
                       file_name_queue->front->len, MPI_CHAR, status.MPI_SOURCE,
                       TAG_COMM_FILE_NAME, MPI_COMM_WORLD);
              deQueue(file_name_queue);
              count++;

            } else {
              char end[] = "."; // send only 1 character - this indicates that there is no more work
              MPI_Send(end, 1, MPI_CHAR, status.MPI_SOURCE,
                       TAG_COMM_FILE_NAME, MPI_COMM_WORLD);
                done_sent_p_count++;
            }
            if (done_sent_p_count == size-1) {
              done = 1;
            }

        } else {
            fprintf(outfile, "receiving file name to process %d\n", pid);
            char *file_name = (char *) malloc(sizeof(char)*FILE_NAME_BUF_SIZE);
            MPI_Status status;
            MPI_Recv(file_name, FILE_NAME_BUF_SIZE, MPI_CHAR, 0, TAG_COMM_FILE_NAME, MPI_COMM_WORLD, &status);
            MPI_Get_count(&status, MPI_CHAR, &recv_len);
            fprintf(outfile, "received file name [%s] to pid %d from process 0, recv len %d\n", file_name, pid, recv_len);

            if (recv_len == 1) {
              done = 1;
            } else {
                // process file -- do work related to reading
            }
        }
        fprintf(outfile, "pid: %d, done: %d\n", pid, done);
        fflush(outfile);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    // this indicates the end of reading section of the MPI

    MPI_Finalize();
    return 0;
}