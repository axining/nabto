#ifndef _NABTO_STREAM_LOG_H_
#define _NABTO_STREAM_LOG_H_

//#define NABTO_LOG_TRACE(message) nabto_log_trace(__FILE__, __LINE__, message)

#ifdef __cplusplus
extern "C" {
#endif

void NABTO_LOG_TRACE(const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif