#include <cstdlib>
#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>
#include <fcntl.h>
#include "my_hashmap.hpp"
#include "shared_mem.h"
#include <iostream>
#include <cassert>
#include <cstring>

void my_sem_wait(sem_t *sem) {
    int ret = sem_wait(sem);
    if(ret < 0) {
        perror("sem_wait failed");
        exit(1);
    }
}

void my_sem_post(sem_t *sem) {
    int ret = sem_post(sem);
    if(ret < 0) {
        perror("sem_post failed");
        exit(1);
    }
}

int main(int argc, char *argv[]) {

    // 1. Open the shared memory. The server must have been started already
    int shm_fd = shm_open(SHARED_MEM_NAME, O_RDWR, 0);
    if(shm_fd < 0) {
        perror("shm_open failed");
        exit(1);
    }

    // 2. Map the shared memory into this processes virtual address space.
    SharedMem *shmem = (SharedMem*) mmap(NULL, sizeof(SharedMem),
            PROT_READ | PROT_WRITE,
                 MAP_SHARED, shm_fd, 0);

    char op; string key;
    cout<<"The server must be started before the first client\n\n"
          "Please type one query per line.\n"
          "A query has the form of 'OP KEY'\n"
          "There are three type of operations:\n"
          "* Adding a key: '+ key', e.g. '+ asdf'\n"
          "* Removing a key: '- key', e.g. '- asdf'\n"
          "* Check if key is contained: '? key', e.g. '? asdf'\n"
          "Press Ctrl+C to exit.\n"<<endl;

    // REPL loop
    while(cin>>op>>key) {
        if(key.length() >= MAX_STRING_LENGTH) {
            cout<<"Sorry, this key is too long"<<endl;
            continue;
        }
        if(op!='?' && op!='+' && op!='-') {
            cout<<"Operation must be + or - or ?"<<endl;
            continue;
        }

        // Any operation on the shared memory need exclusive access
        my_sem_wait(&shmem->sem_access);

        // Advance the tail by one
        int new_tail = (shmem->buffer_tail+1) % BUFFER_SIZE;
        Query *query = &shmem->buffer[new_tail];
        if(query->state != STATE_EMPTY) {
            cerr<<"Buffer overrun!"<<endl;
            exit(1);
        }

        // Write the operation and the key into the query object (which resides in shared memory)
        query->operation = op;
        strncpy(query->key, key.c_str(), key.length()+1);
        query->is_contained = false;
        query->state = STATE_SENT_TO_SERVER;
        shmem->buffer_tail = new_tail;

        // Increase this semaphore to signal the server, that there is work to do
        my_sem_post(&shmem->sem_available);
        my_sem_post(&shmem->sem_access);

        // Wait for the response
        bool server_has_responded;
        do {
//            sched_yield();
            my_sem_wait(&shmem->sem_access);
            server_has_responded = shmem->buffer[new_tail].state == STATE_SENT_TO_CLIENT;
            my_sem_post(&shmem->sem_access);
        } while(!server_has_responded);

        //result has arrived
        my_sem_wait(&shmem->sem_access);
        cout<<"[Response] contains("<<key<<") == "<<query->is_contained<<endl;
        query->state = STATE_EMPTY;
        my_sem_post(&shmem->sem_access);
    }

    return 0;
};