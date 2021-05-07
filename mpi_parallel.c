#include <dirent.h>
#include <errno.h>
#include <mpi.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <omp.h>

#include "util/hashTable.h"
#include "util/queue.h"
#include "util/util.h"

#define TAG_COMM_REQ_DATA 0
#define TAG_COMM_FILE_NAME 1
#define TAG_COMM_PAIR_LIST 3
#define WORD_MAX_LENGTH 50
#define HASH_CAPACITY 50000

#define REPEAT_FILES 10
#define BREAK_RM 0

extern int errno;
int DEBUG_MODE = 0;
int PRINT_MODE = 1;

typedef struct
{
    int hash;
    int count;
    char word[WORD_MAX_LENGTH];
} pair;


int args_parse(int argc, char **argv, char *files_dir, int *repeat_files)
{
    // https://stackoverflow.com/questions/17877368/getopt-passing-string-parameter-for-argument
    int opt;
    while ((opt = getopt(argc, argv, "d:r:g")) != -1)
    {
        switch (opt)
        {
        case 'd':
            strcpy(files_dir, optarg);
            break;
        case 'r':
            *repeat_files = (int)atol(optarg);
            break;
        case ':':
            fprintf(stderr, "Option -%c requires an argument to be given\n", optopt);
            return -1;
        }
    }
    return 0;
}

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);
    int size, pid, p_name_len;
    char p_name[MPI_MAX_PROCESSOR_NAME];
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &pid);
    MPI_Get_processor_name(p_name, &p_name_len);

    double global_time = 0;
    double local_time = 0;
    char csv_out[400] = "";
    char tmp_out[200] = "";

    char files_dir[] = "./files"; // TODO: This should be taken from argv
    int repeat_files = REPEAT_FILES;

    int arg_parse = args_parse(argc, argv, files_dir, &repeat_files);

    local_time = -omp_get_wtime();
    FILE *outfile;
    // open a file whose name is based on the pid
    char buf[16];
    // TODO: Create output directories automatically and not hard coded
    snprintf(buf, 16, "./pout/f%03d.txt", pid);
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
    struct Queue *files_to_read;
    MPI_Barrier(MPI_COMM_WORLD);

    char *file_names = (char *)malloc(sizeof(char) * FILE_NAME_BUF_SIZE * 1000);

    if (pid == 0)
    {
        file_name_queue = createQueue();

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

        int num_files_to_send = file_count / size;
        int spare = file_count % size;

        // iterate for all process ids in the comm world
        for (int i = 0; i < size; i++)
        {
            char *concat_files =
                (char *)malloc(sizeof(char) * FILE_NAME_BUF_SIZE * num_files_to_send);
            int len = 0;
            int send_file_count = num_files_to_send;
            if (spare>0) {
                send_file_count += 1;
                spare--;
            }

            // concat file names to one big char array for ease of sending
            for (int j = 0; j < send_file_count; j++)
            {
                if (j == 0)
                {
                    strcpy(concat_files, file_name_queue->front->line);
                }
                else
                {
                    strcat(concat_files, file_name_queue->front->line);
                }
                len += file_name_queue->front->len + 1;
                if (j != send_file_count - 1)
                {
                    strcat(concat_files, ",");
                }
                deQueue(file_name_queue);
            }
            if (i == 0)
            { // send to other processes
                strcpy(file_names, concat_files);
                recv_len = len;
            }
            else
            {
                MPI_Send(concat_files, len + 1, MPI_CHAR, i, TAG_COMM_FILE_NAME,
                         MPI_COMM_WORLD);
            }
        }
    }
    else
    {
        MPI_Status status;
        MPI_Recv(file_names, FILE_NAME_BUF_SIZE * 1000, MPI_CHAR, 0,
                 TAG_COMM_FILE_NAME, MPI_COMM_WORLD, &status);
        MPI_Get_count(&status, MPI_CHAR, &recv_len);
    }

    // fprintf(outfile, "received file name [%s] to pid %d from process 0, recv len %d\n", file_names, pid, recv_len);

    MPI_Barrier(MPI_COMM_WORLD);
    local_time += omp_get_wtime();
    global_time += local_time;
    sprintf(tmp_out, "%d, %d, %d, %.4f, ", file_count, HASH_CAPACITY, size, local_time);
    strcat(csv_out, tmp_out);

    // this indicates the end of reading section of the MPI

    struct Queue *queue = createQueue();
    struct hashtable *hash_table = createtable(HASH_CAPACITY);

    // this can be run with multiple threads -- can be changed later
    char *file;
    if (BREAK_RM) {
        local_time = -omp_get_wtime();
        while ((file = strtok_r(file_names, ",", &file_names)))
        {
            if (strlen(file) > 0)
            {
                queue->finished = 0;
                populateQueue(queue, file);         // read file
                queue->finished = 1;
            }
        }
        MPI_Barrier(MPI_COMM_WORLD);
        local_time += omp_get_wtime();
        global_time += local_time;
        sprintf(tmp_out, "%.4f, ", local_time);
        strcat(csv_out, tmp_out);

        local_time = -omp_get_wtime();
        populateHashMap(queue, hash_table); // map file
        MPI_Barrier(MPI_COMM_WORLD);
        local_time += omp_get_wtime();
        global_time += local_time;
        sprintf(tmp_out, "%.4f, ", local_time);
        strcat(csv_out, tmp_out);

    } else {
        local_time = -omp_get_wtime();
        while ((file = strtok_r(file_names, ",", &file_names)))
        {
            if (strlen(file) > 0)
            {
                queue->finished = 0;
                populateQueue(queue, file);         // read file
                queue->finished = 1;
                populateHashMap(queue, hash_table); // map file
            }
        }

        MPI_Barrier(MPI_COMM_WORLD);
        local_time += omp_get_wtime();
        global_time += local_time;
        sprintf(tmp_out, "%.4f, ", local_time);
        strcat(csv_out, tmp_out);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    /*******************************************************************************
    * add reduction - hashtable should be communicated amoung the
    * processes to come up with the final reduction
    ********************************************************************************/
    local_time = -omp_get_wtime();
    int h_space = HASH_CAPACITY / size;
    int h_start = h_space * pid;
    int h_end = h_space * (pid + 1);
    // fprintf(outfile, "start [%d] end [%d]\n", h_start, h_end);
    // [0, HASH_CAPACITY/size] values from all the processes should be sent to 0th
    // process [HASH_CAPACITY/size, HASH_CAPACITY/size*2] values from all the ps should be
    // sent to 1st process likewise all data should be shared among the processes

    // --------- DEFINE THE STRUCT DATA TYPE TO SEND
    const int nfields = 3;
    MPI_Aint disps[nfields];
    int blocklens[] = {1, 1, WORD_MAX_LENGTH};
    MPI_Datatype types[] = {MPI_INT, MPI_INT, MPI_CHAR};

    disps[0] = offsetof(pair, hash);
    disps[1] = offsetof(pair, count);
    disps[2] = offsetof(pair, word);

    MPI_Datatype istruct;
    MPI_Type_create_struct(nfields, blocklens, disps, types, &istruct);
    MPI_Type_commit(&istruct);

    // ---

    int k = 0;
    for (k = 0; k < size; k++)
    {
        if (pid != k)
        {
            int j = 0;
            pair pairs[HASH_CAPACITY];
            struct node *current = NULL;
            for (int i = h_space * k; i < h_space * (k + 1); i++)
            {
                current = hash_table->table[i];
                if (current == NULL)
                    continue;
                while (current != NULL)
                {
                    pairs[j].count = current->frequency;
                    pairs[j].hash = i;
                    strcpy(pairs[j].word, current->key);
                    j++;
                    current = current->next;
                }
            }

            // fprintf(outfile, "total words to send: %d\n", j);
            MPI_Send(pairs, j, istruct, k, TAG_COMM_PAIR_LIST, MPI_COMM_WORLD);
            // fprintf(outfile, "total words sent: %d\n", j);
        }
        else if (pid == k)
        {
            for (int pr = 0; pr < size - 1; pr++)
            {
                int recv_j = 0;
                pair recv_pairs[HASH_CAPACITY];
                MPI_Recv(recv_pairs, HASH_CAPACITY, istruct, MPI_ANY_SOURCE,
                         TAG_COMM_PAIR_LIST, MPI_COMM_WORLD, &status);
                MPI_Get_count(&status, istruct, &recv_j);
                // fprintf(outfile, "total words to received: %d from source: %d\n",
                //         recv_j, status.MPI_SOURCE);

                for (int i = 0; i < recv_j; i++)
                {
                    pair recv_pair = recv_pairs[i];
                    int frequency = recv_pair.count;

                    struct node *node = add(hash_table, recv_pair.word, 0);
                    node->frequency += recv_pair.count;
                }
            }
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);
    local_time += omp_get_wtime();
    global_time += local_time;
    sprintf(tmp_out, "%.4f, ", local_time);
    strcat(csv_out, tmp_out);

    // --------------------------------------------------------------
    local_time = -omp_get_wtime();
    // write function should be only called for the respective section of the
    writeTable(hash_table, outfile, h_start, h_end);
    // writeTable(hash_table, outfile, 0, hash_table->tablesize);
    local_time += omp_get_wtime();
    global_time += local_time;
    sprintf(tmp_out, "%.4f, ", local_time);
    strcat(csv_out, tmp_out);
    sprintf(tmp_out, "%.4f", global_time);
    strcat(csv_out, tmp_out);
    
    // fprintf(stdout, "total time taken for the execution: %f\n", global_time);
    if (pid == 0) {
        if (BREAK_RM) {
            fprintf(stdout, "Num_Files, Hash_size, Num_Processes, FS_Time, Read, Map, Reduce, Write, Total\n%s\n\n", 
                csv_out);
        } else {
            fprintf(stdout, "Num_Files, Hash_size, Num_Processes, FS_Time, Read_Map, Reduce, Write, Total\n%s\n\n", 
                csv_out);
        }
    }

    MPI_Finalize();
    return 0;
}