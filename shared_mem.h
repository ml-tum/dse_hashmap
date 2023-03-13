#ifndef DSE_HASHMAP_SHARED_MEM_H
#define DSE_HASHMAP_SHARED_MEM_H


#include <semaphore.h>

const char* SHARED_MEM_NAME = "/my_hashtable";
const int MAX_STRING_LENGTH = 1024;
const int BUFFER_SIZE = 128;

const int STATE_EMPTY = 0;
const int STATE_SENT_TO_SERVER = 1;
const int STATE_SENT_TO_CLIENT = 2;

struct Query
{
    /**
     * One of +, - or ? to denote the operation
     */
    char operation;
    /**
     * The key to save/erase/query from the hash table
     */
    char key[MAX_STRING_LENGTH];
    /**
     * The response of the server, whether the requested key is contained in the hash table.
     */
    bool is_contained;
    /**
     * The status of this entry. The state always goes from STATE_EMPTY -> STATE_SENT_TO_SERVER -> STATE_SENT_TO_CLIENT -> STATE_EMPTY -> ...
     */
    int state;
};
struct SharedMem
{
    /**
     * This semaphore protects access to the shared memory
     */
    sem_t sem_access;
    /**
     * The value of this semaphore is the number of non-empty queries.
     * Increase it when adding an entry and decrease is when taking an entry.
     * The purpose of this semaphore is to signal the server, when there is work to do.
     */
    sem_t sem_available;
    /**
     * Points to the last entry in the ring buffer
     */
    int buffer_tail;
    /**
     * A ring buffer of requests
     */
    Query buffer[BUFFER_SIZE];
};


#endif //DSE_HASHMAP_SHARED_MEM_H
