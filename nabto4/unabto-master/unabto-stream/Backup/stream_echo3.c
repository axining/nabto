#include <unabto/unabto_stream.h>
#include <unabto/unabto_memory.h>
#include <stream_echo.h>
#include <queue.h>
#include <unabto/unabto_stream_types.h>

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

//测试版本
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

pthread_t streamReadPthr;

pthread_mutex_t streamLock;

bool readFlag = false;

unabto_stream* stream_s = NULL;

unabto_stream_event_type type_s = 0;

size_t revBytes = 0, sendBytes = 0;

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
  //  unabto_stream* stream = (unabto_stream*)arg;
 
    while(stream_s == NULL) ;
    unabto_stream* stream = stream_s;

    for (;;) {
        if (queue->size != 0) {
            unabto_stream_hint hint;
            const uint8_t* buf = (const uint8_t*)getFrontBuf(queue);
            size_t writeLength = unabto_stream_write(stream, buf, strlen(buf), &hint);
            
            if (writeLength > 0) {
                NABTO_LOG_TRACE(("unabto stream wrote success!!!"));
                sendBytes += writeLength;
                DeQueue(queue);
            } else {
                if (hint != UNABTO_STREAM_HINT_OK) {
                    NABTO_LOG_TRACE(("unabto stream wrote failed!!!, hint = %d", hint));
                    break;
                }
                NABTO_LOG_TRACE(("unabto stream writeLength = 0"));
            }
           usleep(10000);
        } else {
            if (type_s == UNABTO_STREAM_EVENT_TYPE_WRITE_CLOSED) {
                NABTO_LOG_TRACE(("writting stream closed!"));
                break;
            }
        }
  //      printf ("*******queue->size = %d\n", queue->size);
  //      printf ("revBytes = %d,  sendBytes = %d\n", revBytes, sendBytes);
    }
    printf("***************\n\n\n");
    if (unabto_stream_close(stream)) {
        unabto_stream_release(stream);
    }
}

void unabto_stream_event(unabto_stream* stream, unabto_stream_event_type type) {
    NABTO_LOG_TRACE(("unabto_stream_event : unabto_stream_event_type = %d", type));
    echo_stream* echo = &echo_streams[unabto_stream_index(stream)];

    if (readFlag == false) {
        queue = initQueue();

        int ret = pthread_create(&streamReadPthr, NULL, streamWriteBuf, (void*)stream);
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

        if (readLength > 0) {
            if (!unabto_stream_ack(stream, buf, readLength, &hint)) {
                printf ("unabto_stream_ack failed!\n");
                type = UNABTO_STREAM_EVENT_TYPE_READ_CLOSED;
            } else { 
                NABTO_LOG_TRACE(("received stream = %s", buf));
                revBytes += readLength;
                enQueue(queue, buf);
            }
        } else {
            if (hint !=  UNABTO_STREAM_HINT_OK) {
                printf ("unabto_stream_read failed!\n");
                type = UNABTO_STREAM_EVENT_TYPE_READ_CLOSED;
            }
            printf ("readlength = %d, hint = UNABTO_STREAM_HINT_OK\n", readLength);
        }
      //  pthread_mutex_unlock(&(streamLock));
    }

    if (type == UNABTO_STREAM_EVENT_TYPE_READ_CLOSED) {
        printf ("The unabto_stream_event_type turnd to UNABTO_STREAM_EVENT_TYPE_CLOSED\n");
        type_s = UNABTO_STREAM_EVENT_TYPE_WRITE_CLOSED;
        int ret = pthread_join(streamReadPthr, NULL);
        if(ret != 0) {
            printf(" ERROR! pthread_join failed\n");
        } else {
            readFlag = false;
        }
        if (unabto_stream_close(stream)) {
            unabto_stream_release(stream);
        }
        NABTO_LOG_TRACE(("revBytes = %d,  sendBytes = %d", revBytes, sendBytes));
    }
}


