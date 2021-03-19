/*
 * Copyright (C) Nabto - All Rights Reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <unabto/unabto_common_main.h>
#include <unabto/unabto_env_base.h>
#include <unabto/unabto_external_environment.h>

#include "RAK415.h"
#include "systimer.h"

#define N_SOCKETS 2

// Static remote IP: 195.249.159.188
uint32_t remote_addr = 0xC3F99FBC;

typedef struct
{
  uint8_t isOpen;
  uint8_t hasPrev;
} internal_socket;
static internal_socket sockets[N_SOCKETS];

bool platform_initialize(void)
{
  memset(sockets, 0, sizeof(sockets));
  return true;
}

/**
 * Fill buffer with random content.
 * @param buf  the buffer
 * @param len  the length of the buffer
 */
void nabto_random(uint8_t* buf, size_t len)
{
  uint8_t i;
  if (NULL == buf) {
    return;
  }
  for (i=0; i < len; i++) {
    *buf++ = rand();
  }
}

void nabto_socket_set_invalid(nabto_socket_t* socket)
{
    socket = NABTO_INVALID_SOCKET;
}

/**
 * Initialise a udp socket.  This function is called for every socket
 * uNabto creates, this will normally occur two times. One for local
 * connections and one for remote connections.
 *
 * @param localPort    The local port to bind to.
 *                     A port number of 0 gives a random port.
 * @param socket       To return the created socket descriptor.
 * @return             true iff successfull
 */
bool nabto_socket_init(uint16_t* localPort, nabto_socket_t* socket)
{
    uint8_t i;

    for (i = 0; i < N_SOCKETS; i++)
    {
        if (sockets[i].isOpen == false)
        {
            uint16_t port = *localPort;
            if (port == 0)
            {
                port = 25000 + i;
            }

            // Set SOCKETA_ID or SOCKETB_ID
            *socket = (nabto_socket_t)i;
            sockets[i].isOpen = true;

            NABTO_LOG_DEBUG(("nabto_socket_init %u: port=%u", *socket, port));

            return true;
        }
    }
    return false;
}

bool nabto_socket_is_equal(const nabto_socket_t* s1, const nabto_socket_t* s2)
{
    return *s1==*s2;
}

/**
 * Close a socket.
 * Close can be called on already closed sockets. And should tolerate this behavior.
 *
 * @param socket the socket to be closed
 */
void nabto_socket_close(nabto_socket_t* socket)
{
    NABTO_LOG_DEBUG(("nabto_close_socket %u", *socket));
    sockets[*socket].isOpen = false;
}

/**
 * Read message from network (non-blocking).
 * Memory management is handled by the callee.
 *
 * @param socket  the UDP socket
 * @param buf     destination of the received bytes
 * @param len     length of destination buffer
 * @param addr    the senders IP address (host byte order)
 * @param port    the senders UDP port (host byte order)
 * @return        the number of bytes received
 */
ssize_t nabto_read(nabto_socket_t socket,
                   uint8_t*       buf,
                   size_t         len,
                   struct nabto_ip_address*      addr,
                   uint16_t*      port)
{
    int eff_data_len = 0;
    uint16_t recvLen = 0;

    while (eff_data_len = rakmgr_sockdata_poll_at_resp()) {
        // Ignore AT response (OK*\r\n/ERROR*\r\n)
        rakmgr_sockdata_prg(eff_data_len);
    }

    eff_data_len = rakmgr_sockdata_poll_at_recvdata();
    if (! eff_data_len) {
        return 0;
    }

    NABTO_LOG_DEBUG(("nabto_read socket: %u", socket));

    do {
        *port = uCmdRspFrame.recvdataFrame.recvdata.destPort[0] |
                uCmdRspFrame.recvdataFrame.recvdata.destPort[1]<<8;

        addr->type = NABTO_IP_V4;
        addr->addr.ipv4 = uCmdRspFrame.recvdataFrame.recvdata.destIp[0] |
                uCmdRspFrame.recvdataFrame.recvdata.destIp[1]<<8  |
                uCmdRspFrame.recvdataFrame.recvdata.destIp[2]<<16 |
                uCmdRspFrame.recvdataFrame.recvdata.destIp[3]<<24;

        recvLen = uCmdRspFrame.recvdataFrame.recvdata.recDataLen[0]+uCmdRspFrame.recvdataFrame.recvdata.recDataLen[1]*256;

        if (socket != uCmdRspFrame.recvdataFrame.recvdata.socketID) {
            NABTO_LOG_DEBUG(("nabto_read=%u read on wrong socket"));
            // Just return 0 for the requested socket if socket ID doesn't match.
            return 0;
        }
        if (recvLen < 1)
        {
            NABTO_LOG_DEBUG(("nabto_read=%u empty", socket));
            break;
        }
        if (recvLen > len)
        {
            NABTO_LOG_DEBUG(("nabto_read=%u oversize frame: %u vs %u", socket, recvLen, len));
            break;
        }

        memcpy(buf, uCmdRspFrame.recvdataFrame.recvdata.recvdataBuf, recvLen);

        NABTO_LOG_DEBUG(("Received UDP packet from %i.%i.%i.%i: port=%u length=%u", (addr->addr.ipv4 >> 24) & 0xFF, (addr->addr.ipv4 >> 16) & 0xFF, (addr->addr.ipv4 >> 8) & 0xFF, addr->addr.ipv4 & 0xFF, *port, recvLen));
    }
    while (0);

    rakmgr_sockdata_prg(eff_data_len);

    return recvLen;
}

/**
 * Write message to network (blocking) The memory allocation and
 * deallocation for the buffer is handled by the caller.
 *
 * @param socket  the UDP socket
 * @param buf   the bytes to be sent
 * @param len   number of bytes to be sent
 * @param addr  the receivers IP address (host byte order)
 * @param port  the receivers UDP port (host byte order)
 * @return      true when success
 */
ssize_t nabto_write(nabto_socket_t socket,
                    const uint8_t* buf,
                    size_t         len,
                    struct nabto_ip_address*       addr,
                    uint16_t       port)
{
    NABTO_LOG_DEBUG(("nabto_write: %i.%i.%i.%i: port=%u length=%u", (addr->addr.ipv4 >> 24) & 0xFF, (addr->addr.ipv4 >> 16) & 0xFF, (addr->addr.ipv4 >> 8) & 0xFF, addr->addr.ipv4 & 0xFF, port, len));
    if (addr->type != NABTO_IP_V4) {
        return 0;
    }
    rak_send_Data(socket, port, addr->addr.ipv4, (uint8_t *)buf, len);
    return true;
}

/**
 * Get Current time stamp
 * @return current time stamp
 */
nabto_stamp_t nabtoGetStamp(void)
{
    return systimer_tick();
}

bool nabtoIsStampPassed(nabto_stamp_t *stamp)
{
    return (nabtoGetStamp() - *stamp) > 0;
}

nabto_stamp_diff_t nabtoStampDiff(nabto_stamp_t * newest, nabto_stamp_t * oldest)
{
    return (*newest - *oldest);
}

int nabtoStampDiff2ms(nabto_stamp_diff_t diff)
{
    return (int) diff;
}

void nabto_resolve_ipv4(uint32_t ipv4, struct nabto_ip_address* ip) {
    ip->type = NABTO_IP_V4;
    ip->addr.ipv4 = ipv4;
}

/**
 * start resolving an ip address
 * afterwards nabto_resolve_dns will be called until the address is resolved
 */
void nabto_dns_resolve(const char* id)
{
    // FIXME: Do DNS lookup
}

/**
 * resolve an ipv4 dns address
 * if resolving fails in first attempt we call the function later to
 * see if the address is resolved. The id is always constant for a device
 * meaning the address could be hardcoded but then devices will fail if
 * the basestation gets a new ip address.
 * @param id      name controller hostname
 * @param v4addr  pointer to ipaddress
 * @return false if address is not resolved yet
 */
nabto_dns_status_t nabto_dns_is_resolved(const char *id, struct nabto_ip_address* v4addr)
{
    // FIXME: DNS resolve is not supported on RAK module yet
    v4addr->type = NABTO_IP_V4;
    v4addr->addr.ipv4 = remote_addr;
    return NABTO_DNS_OK;
}
