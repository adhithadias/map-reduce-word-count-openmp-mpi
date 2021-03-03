#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <omp.h>
#include "util/queue.h"
#include "util/hashTable.h"
#include "util/util.h"

extern int errno ;

void populateQueue(struct Queue *q, int i) {
    // format file name of the file to open
    char file_name[20] = "./files/";
    char buffer[3];
    sprintf(buffer,"%d",i);
    strcat(file_name, buffer);
    strcat(file_name, ".txt");

    // file open operation
    FILE* filePtr;
    if ( (filePtr = fopen(file_name, "r")) == NULL) {
        fprintf(stderr, "could not open file: [%p], err: %d, %s\n", filePtr, errno, strerror(errno));
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
        strcpy(str,q->front->line);
        char *token;
        char *rest = str;

        // https://www.geeksforgeeks.org/strtok-strtok_r-functions-c-examples/
        while ((token = strtok_r(rest, " ", &rest))) {

            char *word = format_string(token);
            
            if(strlen(word) > 0){
                node = add(hashMap, word, 0);
                node->frequency++;
            }
            free(word); 
            
        }

        deQueue(q);
    }
}

int main(int argc, char **argv) {

    double time = -omp_get_wtime();

    struct Queue *q = createQueue(); 
    for (int i=1; i<16; i++) {
        populateQueue(q, i);
    }

    hashtable *hashMap = createtable(50000);
    populateHashMap(q, hashMap);

    free(q);
    // printTable(hashMap);
    freetable(hashMap);
    
    time += omp_get_wtime();
    printf("total time taken for the execution: %f\n", time);

    return EXIT_SUCCESS;
}
