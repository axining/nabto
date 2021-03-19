/*
 * Copyright (C) 2008-2014 Nabto - All Rights Reserved.
 */
/**
 * @file
 * Nabto uServer stream packet event - Prototypes.
 *
 * Handling data packets from a stream.
 */

#ifndef _UNABTO_STREAM_EVENT_H_
#define _UNABTO_STREAM_EVENT_H_
#include <unabto/unabto_stream.h>
#include <unabto/unabto_env_base.h>

#if NABTO_ENABLE_STREAM && NABTO_ENABLE_MICRO_STREAM

#include <unabto/unabto_connection.h>
#include <unabto/unabto_packet_util.h>
#include <unabto/unabto_stream_window.h>
#include <unabto/unabto_stream_types.h>
#include <unabto/unabto_memory.h>


#ifdef __cplusplus
extern "C" {
#endif

#if NABTO_ENABLE_DYNAMIC_MEMORY

#else
extern NABTO_THREAD_LOCAL_STORAGE struct nabto_stream_s stream__[NABTO_MEMORY_STREAM_MAX_STREAMS];
extern NABTO_THREAD_LOCAL_STORAGE uint8_t stream_buffer_data[(size_t)(NABTO_MEMORY_STREAM_MAX_STREAMS) * (size_t)(NABTO_MEMORY_STREAM_SEGMENT_SIZE) * (size_t)(NABTO_MEMORY_STREAM_SEND_WINDOW_SIZE)];

extern NABTO_THREAD_LOCAL_STORAGE x_buffer x_buffers[(size_t)(NABTO_MEMORY_STREAM_MAX_STREAMS) * (size_t)(NABTO_MEMORY_STREAM_SEND_WINDOW_SIZE)];
extern NABTO_THREAD_LOCAL_STORAGE r_buffer r_buffers[(size_t)(NABTO_MEMORY_STREAM_MAX_STREAMS) * (size_t)(NABTO_MEMORY_STREAM_RECEIVE_WINDOW_SIZE)];
#endif


/******************************************************************************/

/**
 * Handle a Nabto Stream Packet (internal usage).
 * @param con    the connection
 * @param hdr    the packet header
 * @param info   the window information payload
 * @param start  the decrypted payload
 * @param dlen   the decrypted payload
 */
void nabto_stream_event(nabto_connect*       con,
                        nabto_packet_header* hdr,
                        uint8_t*             info,
                        uint8_t*             dataStart,
                        int                  dataLength,
                        uint8_t*             sackStart,
                        uint16_t             sackLength);




/******************************************************************************/

/**
 * Return human readable name of state
 * @param stream  the stream
 */
text nabto_stream_state_name(unabto_stream* stream);

void stream_initial_config(struct nabto_stream_s* stream);

void stream_init_static_config(struct nabto_stream_s* stream);

bool build_and_send_rst_packet(nabto_connect* con, uint16_t tag, struct nabto_win_info* win);

void unabto_stream_send_stats(struct nabto_stream_s* stream, uint8_t event);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* NABTO_ENABLE_STREAM && NABTO_ENABLE_MICRO_STREAM */

#endif
