/***
 * General Description:
 * This file contains RVMA Mailbox (hashmap) functions and associated initialization and freeing functions
 *
 * Authors: Ethan Shama, Nathan Kowal
 *
 * Reviewers: Nicholas Chivaran, Samantha Hawco
 *
 ***/

#include "rvma_mailbox_hashmap.h"

RVMA_Mailbox* setupMailbox(void *virtualAddress, int hashmapCapacity){
    RVMA_Mailbox *mailboxPtr;
    mailboxPtr = (RVMA_Mailbox*) malloc(sizeof(RVMA_Mailbox));

    if(!mailboxPtr) return NULL;

    RVMA_Buffer_Queue *bufferQueue;
    bufferQueue = createBufferQueue(QUEUE_CAPACITY);
    if(!bufferQueue) {
        print_error("setupMailbox: Buffer Queue failed to be created");
        free(mailboxPtr);
        return NULL;
    }

    RVMA_Buffer_Queue *retiredBufferQueue;
    retiredBufferQueue = createBufferQueue(1);
    if(!retiredBufferQueue) {
        print_error("setupMailbox: Retired Buffer Queue failed to be created");
        free(mailboxPtr);
        free(bufferQueue);
        return NULL;
    }

    mailboxPtr->bufferQueue = bufferQueue;
    mailboxPtr->retiredBufferQueue = retiredBufferQueue;
    mailboxPtr->virtualAddress = virtualAddress;
    mailboxPtr->key = hashFunction(virtualAddress, hashmapCapacity);

    return mailboxPtr;
}

Mailbox_HashMap* initMailboxHashmap(){
    Mailbox_HashMap* hashmapPtr;
    hashmapPtr = (Mailbox_HashMap*)malloc(sizeof(Mailbox_HashMap));
    if(!hashmapPtr) {
        print_error("initMailboxHashmap: hashmap failed to be allocated");
        return NULL;
    }

    hashmapPtr->capacity = HASHMAP_CAPACITY;
    hashmapPtr->numOfElements = 0;

    hashmapPtr->hashmap = (RVMA_Mailbox**)malloc(sizeof(RVMA_Mailbox*) * hashmapPtr->capacity);
    if(!hashmapPtr->hashmap) {
        print_error("initMailboxHashmap: mailboxs failed to be allocated");
        free(hashmapPtr);
        return NULL;
    }
    else{
        memset(hashmapPtr->hashmap, 0, hashmapPtr->capacity * sizeof(RVMA_Mailbox*));
    }

    return hashmapPtr;
}

RVMA_Status freeMailbox(RVMA_Mailbox** mailboxPtr){
    if (mailboxPtr && *mailboxPtr) {
        // Here you should also properly free your bufferQueues, which inside them free the possibly allocated buffers
        if ((*mailboxPtr)->bufferQueue) {
            freeBufferQueue(((*mailboxPtr)->bufferQueue));
        }
        if ((*mailboxPtr)->retiredBufferQueue) {
            freeBufferQueue(((*mailboxPtr)->retiredBufferQueue));
        }
        free(*mailboxPtr);
        *mailboxPtr = NULL;
    }
    return RVMA_SUCCESS;
}

RVMA_Status freeAllMailbox(Mailbox_HashMap** hashmapPtr){

    for(int i = 0; i < (*hashmapPtr)->capacity; i++){
        if((*hashmapPtr)->hashmap[i]) {
            if ((*hashmapPtr)->hashmap[i]->key == i) {
                freeMailbox(&((*hashmapPtr)->hashmap[i]));
            } else {
                (*hashmapPtr)->hashmap[i] = NULL;
            }
        }
    }

    free((*hashmapPtr)->hashmap);
    free(*hashmapPtr);
    *hashmapPtr = NULL;

    return RVMA_SUCCESS;
}

RVMA_Status freeHashmap(Mailbox_HashMap** hashmapPtr){

    if(hashmapPtr && *hashmapPtr){
        freeAllMailbox(hashmapPtr);
        free(*hashmapPtr);
        *hashmapPtr = NULL;
    }

    return RVMA_SUCCESS;
}

int hashFunction(void *virtualAddress, int capacity) {
    uint64_t addr = (uint64_t) virtualAddress;
    uint64_t largePrime = 2654435761;
    return (int) ((addr * largePrime) % capacity);
}

RVMA_Status newMailboxIntoHashmap(Mailbox_HashMap* hashMap, void *virtualAddress){
    int hashNum = hashFunction(virtualAddress, hashMap->capacity);
    RVMA_Mailbox* mailboxPtr;
    mailboxPtr = setupMailbox(virtualAddress, hashMap->capacity);

    if (hashMap->hashmap[hashNum] != NULL) {
        freeMailbox(&mailboxPtr);
        print_error("newMailboxIntoHashmap: Virtual address hashed to same hash number, Virtual address rejected....");
        return RVMA_ERROR;
    }
    else {
        hashMap->hashmap[hashNum] = mailboxPtr;
        hashMap->numOfElements = hashMap->numOfElements + 1;
        return RVMA_SUCCESS;
    }
}

RVMA_Mailbox* searchHashmap(Mailbox_HashMap* hashMap, void* key){

    if(hashMap == NULL) {
        print_error("searchHashmap: hashmap is null");
        return NULL;
    }
    if(key == NULL) {
        print_error("searchHashmap: key is null");
        return NULL;
    }

    // Getting the bucket index for the given key
    int hashNum = hashFunction(key, hashMap->capacity);

    // Head of the linked list present at bucket index
    RVMA_Mailbox* mailboxPtr = hashMap->hashmap[hashNum];
    if (mailboxPtr && mailboxPtr->key == hashNum) {
        return mailboxPtr;
    }
    else{
        // If no key found in the hashMap equal to the given key
        print_error("searchHashmap: No Key in Hashmap matches provided key");
        return NULL;
    }
}

RVMA_Status retireBuffer(RVMA_Mailbox* RVMA_Mailbox, RVMA_Buffer_Entry* entry){

    RVMA_Status res = dequeue(RVMA_Mailbox->bufferQueue, entry);

    if(res != RVMA_SUCCESS){
        print_error("retireBuffer: dequeue of entry failed");
        return RVMA_ERROR;
    }

    return enqueueRetiredBuffer(RVMA_Mailbox->retiredBufferQueue, entry);
}
