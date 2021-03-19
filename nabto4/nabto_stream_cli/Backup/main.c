// main.c
#include "nabto_client_api.h"

#include <stdio.h> 
#include <string.h>
#include <string>
#include <iostream>
#include <memory>
#include <chrono>
#include <iomanip>
#include <fstream>

#include <thread>

int main(int argc, char** argv)
{
    nabto_status_t st = nabtoStartup(NULL);
    if (st != NABTO_OK) {
        printf("初始化失败：1\n"); // handle error and stop  
    }else{
        printf("初始化成功：1\n");
    }

    //nabto_session_t session;
    nabto_handle_t session;
    st = nabtoOpenSession(&session, "8888611@163.com", "xinxin.123");
    if (st != NABTO_OK) {
        printf("初始化失败：2\n"); // handle error and stop  
    }else{
        printf("初始化成功：2\n");
    }

    //context_t* context = (context_t*)malloc(sizeof(context_t));
    //memset(context, 0, sizeof(context_t));

    nabto_stream_t stream;
    const char* host = "vbqh4jgc.aibspi.trial.nabto.net";
    st = nabtoStreamOpen(&stream, session, host);
    if (st != NABTO_OK) {
        printf("初始化失败：3\n"); // handle error and stop  
    }else{
        printf("初始化成功：3\n");
    }

    const char* message = "Hello, world!";
    st = nabtoStreamWrite(stream, message, strlen(message));
    if (st != NABTO_OK) {
        printf("初始化失败：4\n"); // handle error and stop  
    }else{
        printf("初始化成功：4\n");
    }
    
    char* response;
    size_t actual;
    st = nabtoStreamRead(stream, &response, &actual);
    if (st != NABTO_OK) {
        printf("初始化失败：5\n"); // handle error and stop  
    }else{
        printf("初始化成功：5\n");
    }

    /* use response for something */
    printf("read stream example is: !\n", response);
    
    nabtoFree(response);
    nabtoStreamClose(stream);

    nabtoCloseSession(session);
    nabtoShutdown();

    return 0;
}
    nabto_status_t st;
    /* in all of the below: handle error and abort if st != NABTO_OK */
    st = nabtoStartup(NULL);
    nabto_handle_t session;
    nabto_status_t st = nabtoOpenSession(&session, "user@example.org", "12345678");
    nabto_stream_t stream;
    st = nabtoStreamOpen(&stream, session, query);
    const char* message = "Hello, world!";
    st = nabtoStreamWrite(stream, message, strlen(message));
    char* response;
    size_t actual;
    st = nabtoStreamRead(stream, &response, &actual);
    /* use response for something */
    nabtoFree(response);
    nabtoStreamClose(stream);
    nabtoCloseSession(session);
    nabtoShutdown();


    #define CHUNK_SIZE 8192
    #define BUF_SIZE 16384
    typedef struct {
    char da ta_[BUF_SIZE];
    size_t length_;
    nabto_async_resource_t ressource_;
    bool done_;
    } context_t;
    void NABTOAPI callback(nabto_async_status_t status, void* arg, void* userData) {
    context_t* context = (context_t*)userData;
    if (status == NAS_CHUNK_READY) {
    char chunk[CHUNK_SIZE];
    size_t actualSize;
    do {
    nabtoGetAsyncData(context - >resource_ , chunk, CHUNK_SIZE, &actualSize);
    assert(context- >length_ + actualSize < BUF_SIZE);
    memcpy(context- >data_ + context - >length_, chunk, actualSize);
    context- >length_ += actualSize;
    } while (actualSize > 0);
    } else if (status == NAS_CLOSED) {
    context- >done_ = true;
    }
    }







int main(void) {
nabto_status_t st = nabtoStartup(NULL);
if (st != NABTO_OK) { /* handle error and stop */ }
nabto_session_t session;
st = nabtoOpenSession(&session, "user@example.org", "12345678");
if (st != NABTO_OK) { /* handle error and stop */ }
context_t* context = (context_ t*)malloc(sizeof(context_t));
memset(context, 0, sizeof(context_t));
const char* nabtoUrl =
"nabto://weatherdemo.nabto.net/house_temperature?sensor_id=3";
st = nabtoAsyncInit(session, &(context- >ressource_), nabtoUrl);
if (st != NABTO_OK) { /* handle error and stop */ }
st = nabtoAsyncFetch(resource, &callback, context);
if (st != NABTO_OK) { /* handle error and stop */ }
/* do other stuff, data is retrieved in the background and callback() is invok ed when
ready */
while (!context- >done_) {
sleep(1);
}
/* use data retrieved, see nabtoFetchUrl() example */
nabtoAsyncClose(resource);
nabtoCloseSession(session);
nabtoShutdown();
}

/*
            nabto_stream_t streams[100];
            for (i=0; i<testCount; i++){
                status = nabtoStreamOpen(&streams[i], session, host);
                if (status == NABTO_OK) {
                    std::cout << "nabtoStreamOpen()  ["<<i<<"]  succeeded, stream = " << streams[i] <<  std::endl;
                }
                else break;
            }
            writeStartTime = time(NULL);
          //  for (j=0;j<10;j++){
            for (i = 0; i < testCount; i++) {
             //   std::string testCase = testCase1;
                for (j=0;j<10;j++){
                    status = nabtoStreamWrite(streams[i], testCase1.c_str(), testCase1.size());
                    if (status != NABTO_OK) {
                        std::cout << "Stream wrote failed!\n " <<  std::endl;
                        nabtoStreamClose(stream);
                     break;
                        //
                    }
                    writeTotalBytes = writeTotalBytes + testCase1.size();
                    printf ("*j = %d\n", i);
                }
                printf ("********i = %d\n", j);
            }
          //      printf ("********j = %d\n", j);
           // }
            writeEndTime = time(NULL);
            for (i=0; i<testCount; i++){
                nabtoStreamClose(streams[i]);
            }
*/

/*
        nabto_status_t status;
        nabto_handle_t session;
        const char* host = "vbqh4jgc.aibspi.trial.nabto.net";
        int i;
        nabto_stream_t streams[100];
        for (i=0; i<100; i++){
            status = nabtoStreamOpen(&streams[i], session, host);
            if (status == NABTO_OK) {
                std::cout << "nabtoStreamOpen()  ["<<i<<"]  succeeded, stream = " << streams[i] <<  std::endl;
            }
            else {
                printf("nabtoStreamOpen()  [%d]  failed!\n", i);
                break;
            }
        }
        size_t totalBytes = 0, usedTime;
        time_t start, end;
        start = time(NULL);

        for(;;) {
            for(i = 0; i < 100; i++) {
                size_t actual = 0; // actual size (in bytes) of response 
                char* response;
                size_t len = nabtoStreamRead(streams[i], &response, &actual);
                if (len > 0) {
                    totalBytes = totalBytes + actual;
                    nabtoFree(response);
                } else {
                    printf ("len = %d\n", len);
                    break;
                }
            }
            if (status != NABTO_OK) break;
        }
        for (i=0; i<100; i++){
            nabtoStreamClose(streams[i]);
        }


*/