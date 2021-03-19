#include <unabto/unabto_stream.h>
#include <unabto/unabto_memory.h>
#include <stream_echo.h>
#include <queue.h>

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

//新版本，双线程，只发不收
/**
 * Stream echo server
 *
 * The echo server first receives an ``echo'' command, then it sends a
 * ``+'' if it accepts the request. When the command is accepted the
 * state is switches to an echo state.
 *
 * recv: echo\n
 * send: +\n
 * echo data until close
 */

typedef enum {
    ECHO_STATE_IDLE,
    ECHO_STATE_READ_COMMAND,
    ECHO_STATE_COMMAND_FAIL,
    ECHO_STATE_COMMAND_OK,
    ECHO_STATE_FORWARDING,
    ECHO_STATE_CLOSING
} echo_state;

typedef struct {
    uint16_t commandLength;
    bool commandOk;
    echo_state state;
} echo_stream;

echo_stream echo_streams[NABTO_MEMORY_STREAM_MAX_STREAMS];

//////////////////////////////////
//add

streamQueue_t* queue;

pthread_t streamWeadPthr;

pthread_mutex_t streamLock;

bool readFlag = false;

unabto_stream* stream_s = NULL;

unabto_stream_event_type type_s = 0;

//////////////////////////////////

void stream_echo_init() {
    memset(echo_streams, 0, sizeof(echo_streams));
}

static int i = 0;
void unabto_stream_accept(unabto_stream* stream) {
    echo_stream* echo = &echo_streams[unabto_stream_index(stream)];
    UNABTO_ASSERT(echo->state == ECHO_STATE_IDLE);
    memset(echo, 0, sizeof(echo_stream));
    echo->state = ECHO_STATE_READ_COMMAND;

    i++;
    if(i == 2) stream_s = stream;
    
}

void* streamWriteBuf(void* arg) {
 //   unabto_stream* stream = (unabto_stream*)arg;
 
    while(stream_s == NULL) ;
    unabto_stream* stream = stream_s;
/*
    int maxs = 0;
    for (;;) {
        if (queue->size != 0) {
            unabto_stream_hint hint;
            const uint8_t* buf = (const uint8_t*)getFrontBuf(queue);
            if(strlen(buf) > maxs) maxs = strlen(buf);
        //    pthread_mutex_lock(&(streamLock));
            size_t writeLength = unabto_stream_write(stream, buf, strlen(buf), &hint);
        //    pthread_mutex_unlock(&(streamLock));
            
            if (writeLength > 0) {
                printf ("unabto stream wrote success!!!\n");
                DeQueue(queue);
            } else {
                if (hint != UNABTO_STREAM_HINT_OK) {
                    printf ("unabto stream wrote failed!!!, hint = %d\n", hint);
                    break;
                }
                printf ("unabto stream writeLength = 0\n");
            }
        } else {
            if (type_s == UNABTO_STREAM_EVENT_TYPE_WRITE_CLOSED) {
                printf ("writting stream closed!\n");
                break;
            }
        }
    }
    printf (" maxs = %d\n\n\n\n", maxs);
*/
    size_t i = 0;
    uint8_t* buf = (uint8_t*)malloc(1001*sizeof(uint8_t));
    uint8_t* tmp = "unabtoTest";
    for (i = 0; i < 100; ++i) {
        strcat(buf, tmp);
    }
    for(i = 0; i <= 1000;) {
        unabto_stream_hint hint;
        size_t writeLength = unabto_stream_write(stream, buf, strlen(buf), &hint);
        if (writeLength > 0) {
            printf ("unabto stream wrote success!!!\n");
            i++;
        //    DeQueue(queue);
        } else {
            if (hint != UNABTO_STREAM_HINT_OK) {
                printf ("unabto stream wrote failed!!!, hint = %d\n", hint);
                break;
            }
            printf ("unabto stream writeLength = 0\n");
        }
        printf ("************i = %d, writeLength = %d\n", i, writeLength);
    }
    
}

void unabto_stream_event(unabto_stream* stream, unabto_stream_event_type type) {
    printf ("*******unabto_stream_event : unabto_stream_event_type = %d\n", type);
    echo_stream* echo = &echo_streams[unabto_stream_index(stream)];

    if (echo->state == ECHO_STATE_IDLE) {
        printf ("The echo state turnd to ECHO_STATE_IDLE\n");
        return;
    }

    if (readFlag == false) {
        queue = initQueue();

        int ret = pthread_create(&streamWeadPthr, NULL, streamWriteBuf, (void*)stream);
        if(ret != 0) {
            printf(" ERROR! pthread_create failed\n");
        }

        pthread_mutex_init(&(streamLock), NULL);

        readFlag = true;
    }

    if (type == UNABTO_STREAM_EVENT_TYPE_DATA_READY) {
        printf ("The unabto_stream_event_type turnd to UNABTO_STREAM_EVENT_TYPE_DATA_READY\n");
        const uint8_t* buf;
        unabto_stream_hint hint;

      //  pthread_mutex_lock(&(streamLock));
        size_t readLength = unabto_stream_read(stream, &buf, &hint);

        if(strcmp(buf, "endding") == 0) {
            printf ("received endding!\n");
            type = UNABTO_STREAM_EVENT_TYPE_READ_CLOSED;
            readLength = 0;
        }

        if (readLength > 0) {
            if (!unabto_stream_ack(stream, buf, readLength, &hint)) {
                printf ("unabto_stream_ack failed!\n");
                type = UNABTO_STREAM_EVENT_TYPE_READ_CLOSED;
            } else { 
                printf ("received stream %s\n", buf);
                enQueue(queue, ( uint8_t*)buf);
            }
        } else {
            if (hint !=  UNABTO_STREAM_HINT_OK) {
                printf ("unabto_stream_read failed!\n");
                type = UNABTO_STREAM_EVENT_TYPE_READ_CLOSED;
            } else {
                printf ("readlength = %d, hint = UNABTO_STREAM_HINT_OK\n", readLength);
            }
        }
      //  pthread_mutex_unlock(&(streamLock));
    }

    if (type == UNABTO_STREAM_EVENT_TYPE_READ_CLOSED) {
        printf ("The unabto_stream_event_type turnd to UNABTO_STREAM_EVENT_TYPE_CLOSED\n");
        type_s = UNABTO_STREAM_EVENT_TYPE_WRITE_CLOSED;
        int ret = pthread_join(streamWeadPthr, NULL);
        if(ret != 0) {
            printf(" ERROR! pthread_join failed\n");
        } else {
            readFlag = false;
        }
        if (unabto_stream_close(stream)) {
            unabto_stream_release(stream);
            
        }
    }
}


