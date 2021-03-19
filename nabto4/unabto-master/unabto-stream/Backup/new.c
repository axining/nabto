//main_select.c
/******************************************/
#include <modules/cli/unabto_args.h>
#include <unabto/unabto_common_main.h>
#include <unabto/unabto_app.h>

#include "select_impl.h"

int main(int argc, char** argv) {
    nabto_main_setup * nms = unabto_init_context();
    uint8_t psk[PRE_SHARED_KEY_SIZE] = { 0 };

    const char * devId = "vbqh4jgc.aibspi.trial.nabto.net";
    const char * devKey = "01a0650ceb32c7d2263323923d84cdaa";

    printf("xm_unabto_run()\n", devId, devKey);

    nms->id = devId;

    if ( strlen(devKey) == 0 ) {
        printf("set no crypto.\n");
        unabto_set_no_crypto(nms);
    } else {
        if (!unabto_read_psk_from_hex(devKey, psk ,PRE_SHARED_KEY_SIZE)) {
            printf("Cannot read psk\n");
        }

        if(!unabto_set_aes_crypto(nms, psk, PRE_SHARED_KEY_SIZE)){
            printf("init_nms_crypto failed\n");
        }
    }
    printf("device key=:");
    int i=0;
    for(i=0;i<16;i++) printf("%d",nms->presharedKey[i]);
    printf("\n");
     
    select_start(nms);
    return 0;
}

static int s_count = 0;
int select_start(nabto_main_setup* nms) {
    stream_echo_init();
    
    if (!unabto_init()) {
        return 1;
    }
    
    unabto_time_auto_update(false);
    while(true) {
        unabto_time_update_stamp();
        wait_event();
        if(s_count >= 2){
            stream_test();
        }
    }
    
    stream_test();
    unabto_close();
}

void stream_test(){
    //unabto_config.h           NABTO_ENABLE_STREAM 1
    //                          NABTO_ENABLE_STREAM_EVENTS 1
    //unabto_config_defaults.h  NABTO_ENABLE_MICRO_STREAM 1
    printf("stream_test run(): \n");

    unabto_stream* stream = (unabto_stream*)malloc(sizeof(unabto_stream));
    unabto_stream_accept(stream);
    
    const char* message = "Hello, world too!";
    uint8_t* buf = "Hello, world too!";
    size_t st;
    unabto_stream_hint hint;
    st = unabto_stream_write(stream, buf, strlen(buf), &hint);
    printf("unabto_stream_write:st=%d\n", st);

    uint8_t* response;
    st = unabto_stream_read(stream, &response, &hint);
    printf("unabto_stream_read:st=%d\n", st);

    unabto_stream_ack(stream, buf, &hint);

    if(!unabto_stream_close(stream)){
        printf("unabto_stream_close fail\n");
    }
    else{
        printf("unabto_stream_close successfully\n");
    }

    unabto_stream_release(&stream);





    unabto_stream_accept(unabto_stream* stream)   //接收stream
    size_t unabto_stream_read(uanbto_stream* stream, uint8_t** buf, unabto_stream_hint* hint)  //从stream中读取，
    //返回可用的字节数，若为0，则可以读取状态提示以获取为读取任何内容的原因
    unabto_stream_ack(unabto_stream* stream, const uint8_t* buf, size_t used, unabto_stream_hint* hint)  //向framework确认字节已被消耗，可以释放
    size_t unabto_stream_write(unabto_stream* stream, uint8_t* buf, size_t size, unabto_stream_hint* hint)  //将数据写入stream，返回已写入的字节数
    bool unabto_stream_close(unabto_stream* stream)  //关闭stream
    voidunabto_stream_release(unabto_stream* stream)  //释放stream
    //unabto_config_defaults.h  NABTO_ENABLE_MICRO_STREAM 1.
    
}

//
void stream_test_run(){
    //unabto_config.h           NABTO_ENABLE_STREAM 1
    //                          NABTO_ENABLE_STREAM_EVENTS 1
    //unabto_config_defaults.h  NABTO_ENABLE_MICRO_STREAM 1
    printf("stream_test run(): \n");
    
    unabto_stream* stream = (unabto_stream*)malloc(sizeof(unabto_stream));
    unabto_stream_accept(stream);
    const char* message = "Hello, world too!";
    uint8_t* buf = "Hello, world too!";
    size_t st;
    unabto_stream_hint hint;
    st = unabto_stream_write(stream, buf, strlen(buf), &hint);
    printf("unabto_stream_write:st=%d\n", st);
    uint8_t* response;
    st = unabto_stream_read(stream, &response, &hint);
    printf("unabto_stream_read:st=%d\n", st);
    unabto_stream_ack(stream, buf, &hint);
    if(!unabto_stream_close(stream)){
        printf("unabto_stream_close fail\n");
    }
    else{
        printf("unabto_stream_close successfully\n");
    }
    unabto_stream_release(&stream);
    
}