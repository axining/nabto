#include <unabto/unabto_stream.h>
#include <unabto/unabto_memory.h>
#include <stream_echo.h>

//只收不发

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

time_t t1, t2;

void stream_echo_init() {
    memset(echo_streams, 0, sizeof(echo_streams));
}

void unabto_stream_accept(unabto_stream* stream) {
    echo_stream* echo = &echo_streams[unabto_stream_index(stream)];
    UNABTO_ASSERT(echo->state == ECHO_STATE_IDLE);
    memset(echo, 0, sizeof(echo_stream));
    echo->state = ECHO_STATE_READ_COMMAND;
}

int sum = 0, sends = 0, maxs = 0;
int fa1 = 0, fa2 = 0, fa3 = 0;
void unabto_stream_event(unabto_stream* stream, unabto_stream_event_type type) {
    echo_stream* echo = &echo_streams[unabto_stream_index(stream)];

    if (echo->state == ECHO_STATE_IDLE) {
        printf ("The echo state turnd to ECHO_STATE_IDLE\n");
        return;
    }
    
    if (echo->state == ECHO_STATE_READ_COMMAND) {
        printf ("The echo state turnd to ECHO_STATE_READ_COMMAND\n");
        echo->state = ECHO_STATE_COMMAND_OK;
    }
    
    if (echo->state == ECHO_STATE_COMMAND_FAIL) {
        printf ("The echo state turnd to ECHO_STATE_COMMAND_FAIL\n");
        const char* failString = "stream has initialized failed";
        unabto_stream_hint hint;
        unabto_stream_write(stream, (uint8_t*)failString, strlen(failString), &hint);
        echo->state = ECHO_STATE_CLOSING;
    }

    if (echo->state == ECHO_STATE_COMMAND_OK) {
        printf ("The echo state turnd to ECHO_STATE_COMMAND_OK\n");
        const char* okString = "stream has initialized successfully";
        unabto_stream_hint hint;
        size_t wrote = unabto_stream_write(stream, (uint8_t*)okString, strlen(okString), &hint);
        if (wrote != strlen(okString)) {
            echo->state = ECHO_STATE_CLOSING;
        } else {
            echo->state = ECHO_STATE_FORWARDING;
            t1 = time(NULL);
            printf("************t1 = &ld\n",t1);
        }
    }

    if (echo->state == ECHO_STATE_FORWARDING) {
        printf ("The echo state turnd to ECHO_STATE_FORWARDING\n");
        const uint8_t* buf;
        unabto_stream_hint hint;
        size_t readLength = unabto_stream_read(stream, &buf, &hint);
        
        if (readLength > 0) {
            if(readLength > maxs) maxs = readLength;
            if (!unabto_stream_ack(stream, buf, readLength, &hint)) {
                echo->state = ECHO_STATE_CLOSING;
            }else {

                sum += readLength;
            }
      //      printf ("received stream %s\n", buf);

        } else {
            if (hint !=  UNABTO_STREAM_HINT_OK) {
                echo->state = ECHO_STATE_CLOSING;
            }
            fa1++;
        }
    }

    if (echo->state == ECHO_STATE_CLOSING) {
        printf ("The echo state turnd to ECHO_STATE_CLOSING\n");
        printf ("sum = %d, send =%d\n", sum, sends);
        printf ("fa1 = %d, fa2 = %d, fa3 = %d\n", fa1, fa2, fa3);
        printf ("maxs = %d\n", maxs);
        if (unabto_stream_close(stream)) {
            unabto_stream_release(stream);
            echo->state = ECHO_STATE_IDLE;
            printf("************t1 = %ld\n",t1);
            t2 = time(NULL);
            printf("************t2 = %ld\n",t2);
        }
    }
}

