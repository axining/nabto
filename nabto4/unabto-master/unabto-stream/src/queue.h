#ifndef _QUEUE_H_
#define _QUEUE_H_

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

#define true 1
#define false 0

typedef struct streamNode {
    uint8_t* buf;
    struct streamNode* next;
} streamNode_t;

typedef struct streamQueue {
    size_t size;
    streamNode_t* frontNode;
    streamNode_t* rearNode;
    pthread_mutex_t queLock;
} streamQueue_t;

////////////////////////////////////////////////

streamQueue_t* initQueue();

bool enQueue(streamQueue_t* queue, const uint8_t* buf);

bool DeQueue(streamQueue_t* queue);

uint8_t* getFrontBuf(streamQueue_t* queue);

#endif