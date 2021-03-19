#include <unabto/unabto_stream.h>
#include <unabto/unabto_memory.h>
#include <stream_echo.h>

//初始测试版本

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

const char* echoString = "echo";

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

int ind[100] = {0},ssize[100] = {0};
int cnt = 0,count = 0;
int sum = 0, sends = 0;
int fa1 = 0, fa2 = 0, fa3 = 0;
void unabto_stream_event(unabto_stream* stream, unabto_stream_event_type type) {
    count++;
    ind[unabto_stream_index(stream)]++;
    echo_stream* echo = &echo_streams[unabto_stream_index(stream)];

    if (echo->state == ECHO_STATE_IDLE) {
        return;
    }
    
    if (echo->state == ECHO_STATE_READ_COMMAND) {
        cnt++;

        echo->state = ECHO_STATE_COMMAND_OK;
/*
        const uint8_t* buf;
        unabto_stream_hint hint;
        size_t readLength = unabto_stream_read(stream, &buf, &hint);
        

        if (readLength > 0) {
            size_t i;
            size_t ackLength = 0;
            
            for (i = 0; i < readLength; i++) {
                ackLength++;
                if (echo->commandLength == strlen(echoString))-1 {
                  //  if (buf[i] == '\n') {
                        echo->state = ECHO_STATE_COMMAND_OK;
                        break;
                  //  }
                } else {
                    if (buf[i] != echoString[echo->commandLength] || echo->commandLength > strlen(echoString)) {
                        echo->state = ECHO_STATE_COMMAND_FAIL;
                        ackLength = readLength;
                        break;
                    }
                    echo->commandLength++;
                }
                
            }
            echo->state = ECHO_STATE_COMMAND_OK;
            if (!unabto_stream_ack(stream, buf, readLength, &hint)) {
                echo->state = ECHO_STATE_CLOSING;
            }
            
        } else {
            if (hint != UNABTO_STREAM_HINT_OK) {
                echo->state = ECHO_STATE_CLOSING;
            }
        }
        */
    }
    
    if (echo->state == ECHO_STATE_COMMAND_FAIL) {
        const char* failString = "-\n";
        unabto_stream_hint hint;
        unabto_stream_write(stream, (uint8_t*)failString, strlen(failString), &hint);
        echo->state = ECHO_STATE_CLOSING;
    }

    if (echo->state == ECHO_STATE_COMMAND_OK) {
        const char* okString = "+";
        unabto_stream_hint hint;
        size_t wrote = unabto_stream_write(stream, (uint8_t*)okString, strlen(okString), &hint);
        if (wrote != strlen(okString)) {
            echo->state = ECHO_STATE_CLOSING;
        } else {
            echo->state = ECHO_STATE_FORWARDING;
        }

    }

    if (echo->state == ECHO_STATE_FORWARDING) {
        const uint8_t* buf;
        unabto_stream_hint hint;
        size_t readLength = unabto_stream_read(stream, &buf, &hint);
        
        if(ssize[unabto_stream_index(stream)]<readLength) ssize[unabto_stream_index(stream)] = readLength;

        if (readLength > 0) {
            
            size_t writeLength = unabto_stream_write(stream, buf, readLength, &hint);
            if (writeLength > 0) {
                sends += writeLength;
                if (!unabto_stream_ack(stream, buf, writeLength, &hint)) {
                    echo->state = ECHO_STATE_CLOSING;
                }
                else sum += readLength;
            } else {
                if (hint != UNABTO_STREAM_HINT_OK) {
                    echo->state = ECHO_STATE_CLOSING;
                }
                fa1++;
            }
        } else {
            if (hint !=  UNABTO_STREAM_HINT_OK) {
                echo->state = ECHO_STATE_CLOSING;
            }
            fa2++;
        }
    }

    if (echo->state == ECHO_STATE_CLOSING) {
        printf ("cnt = %d, count = %d\n", cnt, count);
        int i;
        for (i = 0; i < 100; i++) {
            printf ("index[%d] = %d\n", i, ind[i]);
        }
        for (i = 0; i < 100; i++) {
            printf ("ssize[%d] = %d\n", i, ssize[i]);
        }
        printf ("sum = %d, send =%d\n", sum, sends);
        printf ("fa1 = %d, fa2 = %d, fa3 = %d\n", fa1, fa2, fa3);
        if (unabto_stream_close(stream)) {
            unabto_stream_release(stream);
            echo->state = ECHO_STATE_IDLE;
        }
    }
}
