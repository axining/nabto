#include <nabto_stream_log.h>

#include <time.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <sys/time.h>

const char* unabto_basename(const char* path)
{
    const char *p;
    char ch;

    p = path + strlen(path);
    while (p > path) {
        ch = *(p - 1);
        if (ch == '/' || ch == '\\' || ch == ':')
            break;
        --p;
    }
    return p;
}

int unabto_log_header(const char* file, unsigned int line)
{
    time_t sec;
    unsigned int ms;
    struct timeval tv;
    struct tm tm;
    gettimeofday(&tv, NULL);
    sec = tv.tv_sec;
    ms = tv.tv_usec/100;

    localtime_r(&sec, &tm);

    return printf("%02u:%02u:%02u:%04u %s(%u): ",
                  tm.tm_hour, tm.tm_min, tm.tm_sec, ms,
                  unabto_basename(file), (line));
}
void NABTO_LOG_TRACE(const char *fmt, ...)
{
    va_list args;
    unabto_log_header(__FILE__, __LINE__);
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf ("\n");
}
