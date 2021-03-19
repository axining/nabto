#include <queue.h>
#include <string.h>
#include <stdlib.h>

streamNode_t* initNode(const uint8_t* buf) {
    streamNode_t* node = (streamNode_t*)malloc(sizeof(streamNode_t));
    node->buf = (uint8_t*)malloc((strlen(buf)+1)*sizeof(uint8_t));
    memcpy(node->buf, buf,strlen(buf));
    node->buf[strlen(buf)] = '\0';
 //   node->buf = buf;
    node->next = NULL;
    return node;
}

streamQueue_t* initQueue() {
    streamQueue_t* queue = (streamQueue_t*)malloc(sizeof(streamQueue_t));
    queue->frontNode = NULL;
    queue->rearNode = NULL;
    queue->size = 0;
    pthread_mutex_init(&(queue->queLock), NULL);
    return queue;
}

bool enQueue(streamQueue_t* queue, const uint8_t* buf) {
    streamNode_t* node = initNode(buf);
    pthread_mutex_lock(&(queue->queLock));
    if(queue->size == 0) {
        queue->frontNode = node;
        queue->rearNode = node;
    } else {
        queue->rearNode->next = node;
        queue->rearNode = node;
    }
    queue->size++;
    pthread_mutex_unlock(&(queue->queLock));
    return true;
}

bool DeQueue(streamQueue_t* queue) {
    pthread_mutex_lock(&(queue->queLock));
    if (queue->size == 0) {
        pthread_mutex_unlock(&(queue->queLock));
        printf ("empty queue, DeQueue failed!\n");
        return false;
    } else if (queue->size == 1) {
        streamNode_t* node = queue->frontNode;
        free(node->buf);
        free(node);
        queue->frontNode = NULL;
        queue->rearNode = NULL;
    } else {
        streamNode_t* node = queue->frontNode;
        queue->frontNode = queue->frontNode->next;
        free(node->buf);
        free(node);
    }
    queue->size--;
    pthread_mutex_unlock(&(queue->queLock));
    return true;
}

uint8_t* getFrontBuf(streamQueue_t* queue) {
    if (queue->size > 0) {
        return queue->frontNode->buf;
    } else {
        printf ("empty queue, getFrontBuf failed!\n");
        return NULL;
    }
}

/*
void nabtoEnQueue(streamQueue_t* queue, uint8_t* buf) {
    printf ("******************nabtoEnQueue(streamQueue_t* queue, uint8_t* buf)\n");
    printf ("strlen(buf) = %d\n", strlen(buf));
    uint8_t* st = buf;
    size_t cnt = 0;
    size_t i = strlen(buf)/1023;
    if(strlen(buf)%1023 != 0) i++;
    while(strlen(buf+cnt)>1023) {
        uint8_t* temp = (uint8_t*)malloc(1024*sizeof(uint8_t));
        strncpy(temp, buf+cnt, 1023);
        temp[1023] = '\0';
        enQueue(queue, temp);
        
        cnt += 1023;
        i--;
    }
    if(strlen(buf+cnt)>0){
        uint8_t* temp = (uint8_t*)malloc(strlen(buf+cnt)*sizeof(uint8_t));
        strcpy(temp, buf+cnt);
        enQueue(queue, temp);
    }
    printf ("*************queue->size = %d\n", queue->size);
   // free(st);
}

queue_t* initQueue() {
    queue_t* queue = (queue_t*)malloc(sizeof(queue_t));
    queue->next = NULL;
    return queue;
}

queue_t* enQueue(queue_t* rear, char* buf) {
    queue_t* enElem = (queue_t*)malloc(sizeof(queue_t));
    //enElem->buf = (char*)malloc(strlen(buf)*sizeof(char));
    //strcpy(enElem->buf, buf);
    enElem->buf = buf;
    enElem->next = NULL;
    rear->next = enElem;
    rear = enElem;
    return rear;
}

queue_t* DeQueue(queue_t* top, queue_t* rear) {
    if (top->next == NULL) {
        printf ("Queue is empty!\n");
        return rear;
    }
    queue_t* p = top->next;
    printf ("DeQueue buffer is %s\n", p->buf);
    top->next = p->next;
    if (rear == p) {
        rear = top;
    }
    //free(p->buf);
    free(p);
    return rear;
}
*/
