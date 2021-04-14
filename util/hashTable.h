// https://github.com/CodeRuben/Word-Frequency-Counter/blob/master/hashtable.h

#ifndef HASHTABLE_H_INCLUDED
#define HASHTABLE_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

extern int DEBUG_MODE;

struct node
{
    char *key;
    int frequency;
    struct node *next;
};

struct hashtable
{
    struct node **table;
    int currentsize;
    int tablesize;
};

typedef struct hashtable hashtable;

/**
 * Generates a hash code value created from a string value.
 * @param char *, the string to be hashed.
 * @return a positive integer value.
 */
int hashcode(char *);

/**
 * Deallocates the memory of a node.
 * @param struct node *, the node to be deallocated.
 */
void freenode(struct node *);

/**
 * Adds a new node to the hash table.
 * @param hashtable *, pointer to struct containing the hash table.
 * @param char *, the key value to be stored.
 * @param int, the value to be stored.
 * @return the newly created node. If the key value is already
 * in the table, return the node containing that key.
 */
struct node *add(hashtable *, char *, int);

/**
 * Checks is a key value is stored in the table.
 * @param hashtable *, pointer to struct containing the hash table.
 * @param char *, the key value to be searched for.
 * @return true if the key is in the table, false otherwise.
 */
bool contains(hashtable *, char *);

/**
 * Checks is a key value is stored in the table, returns the node if
 * it's found.
 * @param hashtable *, pointer to struct containing the hash table.
 * @param char *, the key value to be searched for.
 * @return the node containing the key value, NULL if the node is not
 * found.
 */
struct node *getnode(hashtable *, char *);

/**
 * Deletes a node from the hash table
 * @param hashtable *, pointer to struct containing the hash table
 * @param char *, the key value to be deleted
 */
void deletenode(hashtable *, char *);

/**
 * Displays the elements from the table that have a higher frequency.
 * value than the integer value given as a parameter.
 * @param hashtable *, pointer to struct containing the hash table.
 * @param int, minimum frequency value.
 */
void mostfrequent(hashtable *, int);

/**
 * Initializes a new hash table. Allocates memory for an array.
 * of pointers. Size parameter determines initial size of the array
 * @param int, the size of the array to be created.
 * @param hashtable *, pointer to the new hashtable.
 */
hashtable *createtable(int);

/**
 * Allocates memory and sets the values for a new node.
 * @param int, the value to be stored in the node.
 * @param char *, the key value to be stored in the node.
 * @return node *, pointer to the new node.
 */
struct node *nalloc(char *, int);

/**
 * Deallocates the memory associated with the hash table, including
 * all of the stored nodes.
 * @param hashtable *, pointer to the hashtable to be deallocated.
 */
void freetable(hashtable *);

/**
 * Generates a hash code value created from a string value.
 * @param char *, the string to be hashed.
 * @return a positive integer value.
 */
int hashcode(char *);

/**
 * Deallocates the memory of a node.
 * @param struct node *, the node to be deallocated.
 */
void freenode(struct node *);

void printTable(struct hashtable *h);

/* Adds a new node to the hash table*/
struct node *add(hashtable *h, char *key, int freq)
{
    struct node *newnode;
    int index = hashcode(key) % h->tablesize;
    struct node *current = h->table[index];

    /* Search for duplicate value */
    while (current != NULL)
    {
        if (strcmp(key, current->key) == 0)
            return current;
        current = current->next;
    }
    /* Create new node if no duplicate is found */
    newnode = nalloc(key, freq);
    newnode->next = h->table[index];
    h->table[index] = newnode;
    h->currentsize = h->currentsize + 1;
    return newnode;
}

/* Searches for a key, returns true if it finds it */
bool contains(hashtable *h, char *key)
{
    int index = hashcode(key) % h->tablesize;
    struct node *current = h->table[index];

    while (current != NULL)
    {
        if (strcmp(key, current->key) == 0)
            return true;
        current = current->next;
    }
    return false;
}

/* Returns a pointer to a node that matches the given key value */
struct node *getnode(hashtable *h, char *key)
{
    int index = hashcode(key) % h->tablesize;
    struct node *current = h->table[index];

    while (current != NULL)
    {
        if (strcmp(key, current->key) == 0)
            return current;
        current = current->next;
    }
    return NULL;
}

/* Deletes a node whose key matches the string parameter */
void deletenode(hashtable *h, char *key)
{
    int index = hashcode(key) % h->tablesize;
    struct node *current = h->table[index];
    struct node *previous = NULL;

    if (current == NULL)
        return;
    /* Scan the linked list until  match is found or the end is reached */
    while (current != NULL && strcmp(key, current->key) != 0)
    {
        previous = current;
        current = current->next;
    }
    /* Item not found */
    if (current == NULL)
        return;
    /* Item is in the first position */
    else if (current == h->table[index])
        h->table[index] = current->next;
    /* Item is in the last position */
    else if (current->next == NULL)
        previous->next = NULL;
    /* Item is in the middle of the list */
    else
        previous->next = current->next;
    /* Deallocate memory for the deleted node */
    freenode(current);
}

/* Prints values that have a value higher than the integer parameter */
void mostfrequent(hashtable *h, int freq)
{
    struct node *current = NULL;
    int i;
    printf("\n  Word       Frequency\n");
    printf("  --------------------\n");
    for (i = 0; i < h->tablesize; i++)
    {
        current = h->table[i];
        while (current != NULL)
        {
            if (current->frequency > freq)
                printf("  %-14s %d\n", current->key, current->frequency);
            current = current->next;
        }
    }
}

/* Returns a positive integer hash value generated from a string value */
int hashcode(char *key)
{
    int i, hash = 7;
    int length = strlen(key);

    for (i = 0; i < length; i++)
        hash = (hash * 31) + *(key + i);
    return hash & 0x7FFFFFFF; /* Make value positive */
}

/* Returns a pointer to a newly allocated hash table */
struct hashtable *createtable(int size)
{
    int i;
    if (size < 1)
        return NULL;
    hashtable *table = (hashtable *)malloc(sizeof(hashtable));
    table->table = (struct node **)malloc(sizeof(struct node *) * size);

    if (table != NULL)
    {
        table->currentsize = 0;
        table->tablesize = size;
    }
    /* Set all pointers to NULL */
    for (i = 0; i < size; i++)
        table->table[i] = NULL;
    return table;
}

/* Allocates memory for a new node. Initializes the new node's members */
struct node *nalloc(char *key, int freq)
{
    struct node *p = (struct node *)malloc(sizeof(struct node));

    if (p != NULL)
    {
        p->key = strdup(key);
        p->frequency = freq;
        p->next = NULL;
    }
    return p;
}

/* Deallocates memory of the string stored in the node and the
   node itself */
void freenode(struct node *node)
{
    free(node->key);
    free(node);
}

/* Deallocates all of the memory associated with the hash table */
void freetable(hashtable *h)
{
    struct node *current = NULL;
    int i;

    for (i = 0; i < h->tablesize; i++)
    {
        current = h->table[i];
        if (current == NULL)
            continue;
        /* Deallocate memory of every node in the table */
        while (current->next != NULL)
        {
            h->table[i] = h->table[i]->next;
            freenode(current);
            current = h->table[i];
        }
        freenode(current);
    }
    /* Free the hash table */
    free(h->table);
    free(h);
}

void printTable(struct hashtable *h)
{
    /* Set all pointers to NULL */
    struct node *current = NULL;
    int i;

    for (i = 0; i < h->tablesize; i++)
    {
        current = h->table[i];
        if (current == NULL)
            continue;
        /* Deallocate memory of every node in the table */
        int spaces = 0;
        while (current != NULL)
        {
            int j;
            for (j = 0; j < spaces; j++)
            {
                printf("\t");
            }
            printf("i: %d, key: %s, frequency: %d\n", i, current->key, current->frequency);
            current = current->next;
            spaces++;
        }
    }
}

void writeTable(struct hashtable *h, FILE *fp, int start, int end)
{
    /* Set all pointers to NULL */
    struct node *current = NULL;

    // different threads may write different parts of the hash table
    // set the start end values for the loop accordingly
    int i;
    for (i = start; i < end; i++)
    {
        current = h->table[i];
        if (current == NULL)
            continue;
        /* Deallocate memory of every node in the table */
        int spaces = 0;
        while (current != NULL)
        {
            int j;
            for (j = 0; j < spaces; j++)
            {
                if (DEBUG_MODE)
                    fprintf(fp, "\t");
            }
            fprintf(fp, "key: %s, frequency: %d\n", current->key, current->frequency);
            if (DEBUG_MODE)
                fprintf(fp, "i: %d, key: %s, frequency: %d\n", i, current->key, current->frequency);
            current = current->next;
            spaces++;
        }
    }
}

void writePartialTable(struct hashtable *h, const char *filename, int start, int end)
{
    FILE *fp = fopen(filename, "w");
    writeTable(h, fp, start, end);
}

void writeFullTable(struct hashtable *h, const char *filename)
{
    writePartialTable(h, filename, 0, h->tablesize);
}

#endif // HASHTABLE_H_INCLUDED