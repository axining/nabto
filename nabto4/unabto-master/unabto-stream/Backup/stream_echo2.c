#include <unabto/unabto_stream.h>
#include <unabto/unabto_memory.h>
#include <stream_echo.h>
#include <queue.h>

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

//新版本，只收不发

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

void stream_echo_init() {
    memset(echo_streams, 0, sizeof(echo_streams));
}

void unabto_stream_accept(unabto_stream* stream) {
    echo_stream* echo = &echo_streams[unabto_stream_index(stream)];
    UNABTO_ASSERT(echo->state == ECHO_STATE_IDLE);
    memset(echo, 0, sizeof(echo_stream));
    echo->state = ECHO_STATE_READ_COMMAND;
}

int sum = 0;
int ll=0;
unabto_stream* st = NULL;
void unabto_stream_event(unabto_stream* stream, unabto_stream_event_type type) {
    echo_stream* echo = &echo_streams[unabto_stream_index(stream)];

    if (echo->state == ECHO_STATE_IDLE) {
        printf ("The echo state turnd to ECHO_STATE_IDLE\n");
        return;
    }

    if(st!=stream) {
        ll++;
        st=stream;
    }

    if (type == UNABTO_STREAM_EVENT_TYPE_DATA_READY) {
        printf ("The unabto_stream_event_type turnd to UNABTO_STREAM_EVENT_TYPE_DATA_READY\n");
        const uint8_t* buf;
        unabto_stream_hint hint;
        size_t readLength = unabto_stream_read(stream, &buf, &hint);

        if (readLength > 0) {
            printf ("received stream %s\n", buf);
            if (!unabto_stream_ack(stream, buf, readLength, &hint)) {
                type = UNABTO_STREAM_EVENT_TYPE_READ_CLOSED;
            } else { 
              //  rearNode = enQueue(rearNode, (char*)buf);
                sum += readLength;
            }
        } else {
            if (hint !=  UNABTO_STREAM_HINT_OK) {
                type = UNABTO_STREAM_EVENT_TYPE_READ_CLOSED;
            }
            else printf ("readlength = %d, hint = UNABTO_STREAM_HINT_OK\n", readLength);
        }

    }

    if (type == UNABTO_STREAM_EVENT_TYPE_READ_CLOSED) {
        printf ("The unabto_stream_event_type turnd to UNABTO_STREAM_EVENT_TYPE_CLOSED\n");
        printf ("ll= %d, sum = %d\n", ll, sum);
        if (unabto_stream_close(stream)) {
            unabto_stream_release(stream);
        }
    }
}

