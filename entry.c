#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include "queue.h"
#include "hashTable.h"

extern int errno ;

/**
 * Format string with only lower case alphabetic letters
 */
void format_string(char *original, char *word, int len) {
    int c=0;
    for(int i=0;i<len;i++)
    {
        if(isalnum(original[i]) || original[i]=='\'')
        {
            word[c]=tolower(original[i]);
            c++;
        }
    }
    word[c]='\0';
    len=strlen(word);
}

int main(int argc, char *argv) {


    FILE* filePtr;
    if ( (filePtr = fopen("./files/text1.txt", "r")) == NULL) {
        fprintf(stderr, "could not open file: [%p], err: %d, %s\n", filePtr, errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

    struct Queue* q = createQueue(); 

    size_t len = 0;
    char *line = NULL;
    while (getline(&line, &len, filePtr) != -1) {
        enQueue(q, line, len); 
    }
    fclose(filePtr);

    hashtable *hashMap = createtable(50000);
    struct node *node = NULL;

    free(line);
    while (q->front) {
        
        char str[q->front->len];
        strcpy(str,q->front->line);
        char *token;
        char *rest = str;
        int length = 0;

        // https://www.geeksforgeeks.org/strtok-strtok_r-functions-c-examples/
        while ((token = strtok_r(rest, " ", &rest))) {

            int len=strlen(token);
            char *word = malloc(len*sizeof(char));
            format_string(token, word, len);
            
            if(strlen(word) > 0){
                node = add(hashMap, word, 0);
                node->frequency++;
            }
            free(word); 
            
        }

        deQueue(q);
    }
    free(q);

    printTable(hashMap);
    freetable(hashMap);

    return EXIT_SUCCESS;
}

// // Driver Program to test anove functions 
// int main() 
// { 
//     printf("starting\n");
// 	struct Queue* q = createQueue(); 
//     printf("queue created\n");
	
//     char* line = malloc(100*sizeof(char));
//     memset(line, '\0', sizeof(line));
//     strcpy(line, "Adhitha Dias");
//     enQueue(q, line, 12); 
	
//     line = malloc(100*sizeof(char));
//     memset(line, '\0', sizeof(line));
//     strcpy(line, "Kariyawasam");
//     enQueue(q, line, 20); 
// 	deQueue(q); 
// 	// deQueue(q); 
// 	// enQueue(q, 30); 
// 	// enQueue(q, 40); 
// 	// enQueue(q, 50); 
// 	// deQueue(q); 
// 	printf("Queue Front : %s \n", q->front->line); 
// 	printf("Queue Rear : %s \n", q->rear->line); 
	
//     while (q->front) {
//         deQueue(q);
//     }
//     free(q);
    
//     return 0; 
// } 