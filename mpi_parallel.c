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

typedef struct {
  int hash;
  int count;
  char word[WORD_MAX_LENGTH];
} pair;

void populateQueue(struct Queue *q, char *file_name) {
  // file open operation
  FILE *filePtr;
  if ((filePtr = fopen(file_name, "r")) == NULL) {
    fprintf(stderr, "could not open file: [%p], err: %d, %s\n", filePtr, errno,
            strerror(errno));
    exit(EXIT_FAILURE);
  }

  // read line by line from the file and add to the queue
  size_t len = 0;
  char *line = NULL;
  while (getline(&line, &len, filePtr) != -1) {
    enQueue(q, line, len);
  }
  fclose(filePtr);
  free(line);
}

void populateHashMap(struct Queue *q, struct hashtable *hashMap) {
  struct node *node = NULL;

  while (q->front) {
    char str[q->front->len];
    strcpy(str, q->front->line);
    char *token;
    char *rest = str;

    // https://www.geeksforgeeks.org/strtok-strtok_r-functions-c-examples/
    while ((token = strtok_r(rest, " ", &rest))) {
      char *word = format_string(token);

      if (strlen(word) > 0) {
        node = add(hashMap, word, 0);
        node->frequency++;
      }
      free(word);
    }

    deQueue(q);
  }
}

int main(int argc, char **argv) {
  MPI_Init(NULL, NULL);
  int size, pid, p_name_len;
  char p_name[MPI_MAX_PROCESSOR_NAME];
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Comm_rank(MPI_COMM_WORLD, &pid);
  MPI_Get_processor_name(p_name, &p_name_len);

  double time = -omp_get_wtime();

  /* file outputs for processes */
  // declare a file
  FILE *outfile;
  // open a file whose name is based on the pid
  char buf[16];
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

  if (pid == 0) {
    file_name_queue = createQueue();
    file_count = get_file_list(file_name_queue);

    int num_files_to_send = file_count / size;

    // iterate for all process ids in the comm world
    for (int i = 0; i < size; i++) {
      char *concat_files =
          (char *)malloc(sizeof(char) * FILE_NAME_BUF_SIZE * num_files_to_send);
      int len = 0;
      // concat file names to one big char array for ease of sending
      for (int j = 0; j < num_files_to_send; j++) {
        if (j == 0) {
          strcpy(concat_files, file_name_queue->front->line);
        } else {
          strcat(concat_files, file_name_queue->front->line);
        }
        len += file_name_queue->front->len + 1;
        if (j != num_files_to_send - 1) {
          strcat(concat_files, ",");
        }
        deQueue(file_name_queue);
      }
      if (i == 0) {  // send to other processes
        strcpy(file_names, concat_files);
        recv_len = len;
      } else {
        MPI_Send(concat_files, len + 1, MPI_CHAR, i, TAG_COMM_FILE_NAME,
                 MPI_COMM_WORLD);
      }
    }
  } else {
    MPI_Status status;
    MPI_Recv(file_names, FILE_NAME_BUF_SIZE * 1000, MPI_CHAR, 0,
             TAG_COMM_FILE_NAME, MPI_COMM_WORLD, &status);
    MPI_Get_count(&status, MPI_CHAR, &recv_len);
  }

  fprintf(outfile,
          "received file name [%s] to pid %d from process 0, recv len %d\n",
          file_names, pid, recv_len);

  MPI_Barrier(MPI_COMM_WORLD);
  // this indicates the end of reading section of the MPI

  struct Queue *queue = createQueue();
  struct hashtable *hash_table = createtable(CAPACITY);

  // this can be run with multiple threads -- can be changed later
  char *file;
  while ((file = strtok_r(file_names, ",", &file_names))) {
    if (strlen(file) > 0) {
      fprintf(outfile, "file [%s]\n", file);

      populateQueue(queue, file);          // read file
      populateHashMap(queue, hash_table);  // map file
    }
  }

  MPI_Barrier(MPI_COMM_WORLD);

  // ---------------------------------------------------------------------

  /*
   * add reduction - hashtable should be communicated amoung the
   * processes to come up with the final reduction
   */
  int h_space = CAPACITY / size;
  int h_start = h_space * pid;
  int h_end = h_space * (pid + 1);
  fprintf(outfile, "start [%d] end [%d]\n", h_start, h_end);
  // [0, CAPACITY/size] values from all the processes should be sent to 0th
  // process [CAPACITY/size, CAPACITY/size*2] values from all the ps should be
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
  for (k = 0; k < size; k++) {
    if (pid != k) {
      int j = 0;
      pair pairs[CAPACITY];
      struct node *current = NULL;
      for (int i = h_space * k; i < h_space * (k + 1); i++) {
        current = hash_table->table[i];
        if (current == NULL) continue;
        while (current != NULL) {
          pairs[j].count = current->frequency;
          pairs[j].hash = i;
          strcpy(pairs[j].word, current->key);
          j++;
          current = current->next;
        }
      }

      fprintf(outfile, "total words to send: %d\n", j);

      MPI_Send(pairs, j, istruct, k, TAG_COMM_PAIR_LIST, MPI_COMM_WORLD);
      fprintf(outfile, "total words sent: %d\n", j);
    } else if (pid == k) {
      for (int pr = 0; pr < size - 1; pr++) {
        int recv_j = 0;
        pair recv_pairs[CAPACITY];
        MPI_Recv(recv_pairs, CAPACITY, istruct, MPI_ANY_SOURCE,
                 TAG_COMM_PAIR_LIST, MPI_COMM_WORLD, &status);
        MPI_Get_count(&status, istruct, &recv_j);
        fprintf(outfile, "total words to received: %d from source: %d\n",
                recv_j, status.MPI_SOURCE);

        for (int i = 0; i < recv_j; i++) {
          pair recv_pair = recv_pairs[i];
          int frequency = recv_pair.count;

          struct node *node = add(hash_table, recv_pair.word, 0);
          node->frequency += recv_pair.count;
        }
      }
    }
  }

  MPI_Barrier(MPI_COMM_WORLD);

  // --------------------------------------------------------------

  // write function should be only called for the respective section of the
  writeTable(hash_table, outfile, h_start, h_end);
  // writeTable(hash_table, outfile, 0, hash_table->tablesize);
  time += omp_get_wtime();
  fprintf(outfile, "total time taken for the execution: %f\n", time);

  MPI_Finalize();
  return 0;
}