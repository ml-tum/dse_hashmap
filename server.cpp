#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>
#include "my_hashmap.hpp"
#include "shared_mem.h"

/**
 * Wait for semaphore
 */
void my_sem_wait(sem_t *sem) {
    int ret = sem_wait(sem);
    if(ret < 0) {
        perror("sem_wait failed");
        exit(1);
    }
}

/**
 * Release semaphore
 */
void my_sem_post(sem_t *sem) {
    int ret = sem_post(sem);
    if(ret < 0) {
        perror("sem_post failed");
        exit(1);
    }
}

const int DEFAULT_BUCKET_COUNT = 32;

int main(int argc, char *argv[]) {

    int ret;
    cout<<"Make sure, that you start the server before the first client, \n"
          "and no other servers or clients are running, when the server is started"<<endl;


    // 1. Init the hash table
    int arg_bucket_count = argc >= 2 ? atoi(argv[1]) : 0;
    if(arg_bucket_count <= 0) {
        cout<<"Using default bucket count "<<DEFAULT_BUCKET_COUNT<<endl;
        arg_bucket_count = DEFAULT_BUCKET_COUNT;
    }

    MyHashtable<string> hashtable(arg_bucket_count);



    // 2. Init the POSIX shared memory region
    int shm_fd = shm_open(SHARED_MEM_NAME, O_CREAT|O_RDWR, 0600);
    if(shm_fd < 0) {
        perror("shm_open failed");
        exit(1);
    }

    // 3. Adjust the size of the shared memory
    ret = ftruncate(shm_fd, sizeof(SharedMem));
    if(ret < 0) {
        perror("ftruncate failed");
        exit(1);
    }

    // 4. Map the shared memory into this programs virtual memory
    SharedMem *shmem = (SharedMem*) mmap(NULL, sizeof(SharedMem),
            PROT_READ | PROT_WRITE,
                 MAP_SHARED, shm_fd, 0);

    // 5. Init both semaphores
    ret = sem_init(&shmem->sem_access, 1, 0);
    if(ret < 0) {
        perror("sem_init failed");
        exit(1);
    }

    ret = sem_init(&shmem->sem_available, 1, 0);
    if(ret < 0) {
        perror("sem_init failed");
        exit(1);
    }

    // 6. Clear the shared memory region
    for(int i=0; i<BUFFER_SIZE; i++) {
        Query &query = shmem->buffer[i];
        query.is_contained = false;
        query.state = STATE_EMPTY;
    }
    shmem->buffer_tail=0;

    // 7. Release the semaphore. Now clients can start to write into the shared memory
    my_sem_post(&shmem->sem_access);


    int tail=0; //Keeps track of the entries, we have seen so far
    while(1) {
        // This operation will block, until there is an entry in the ring buffer
        my_sem_wait(&shmem->sem_available);
        my_sem_post(&shmem->sem_available);

        // We need exclusive access for any operation on the shared memory
        my_sem_wait(&shmem->sem_access);

        // While there are unseen entries
        while(tail != shmem->buffer_tail) {
            tail = (tail+1) % BUFFER_SIZE;
            Query *query = &shmem->buffer[tail];

            // Execute the requested operation on the hash table
            if(query->operation == '+') {
                hashtable.insert(query->key);
                query->is_contained = true;
                cerr<<"Added "<<query->key<<endl;

            } else if(query->operation == '-') {
                hashtable.remove(query->key);
                query->is_contained = false;
                cerr<<"Removed "<<query->key<<endl;

            } else if(query->operation == '?') {
                bool result = hashtable.contains(query->key);
                query->is_contained = result;
                cerr<<"Query: contains("<<query->key<<") == "<<result<<endl;
            }

            query->state = STATE_SENT_TO_CLIENT; // the client can now read the result
            my_sem_wait(&shmem->sem_available); // decrease the number of work items by one
        }

        my_sem_post(&shmem->sem_access);
    }

    shm_unlink(SHARED_MEM_NAME);

    return 0;
};
