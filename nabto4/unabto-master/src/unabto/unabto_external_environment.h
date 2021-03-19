/*
 * Copyright (C) Nabto - All Rights Reserved.
 */
#ifndef _UNABTO_EXTERNAL_ENVIRONMENT_H_
#define _UNABTO_EXTERNAL_ENVIRONMENT_H_

#if NABTO_SLIM
#include <unabto_platform_types.h>
#else
#include <unabto/unabto_env_base.h>
#endif
#include <unabto/unabto_endpoint.h>

#ifdef __cplusplus
extern "C" {
#endif

/********** Platform Random ***************************************************/
/** 
 * Fill buffer with random content.
 * @param buf  the buffer
 * @param len  the length of the buffer
 */
void nabto_random(uint8_t* buf, size_t len);

/*********** UDP init/close ****************************************************/
/**
 * Initialise a udp socket.  This function is called for every socket
 * uNabto creates, this will normally occur two times. One for local
 * connections and one for remote connections.
 *
 * @param localPort    The local port to bind to.
 *                     A port number of 0 gives a random port. The
 *                     pointer may not be NULL.
 * @param socket       To return the created socket descriptor.
 * @return             true iff successfull
 */
bool nabto_socket_init(uint16_t* localPort, nabto_socket_t* socket);

/**
 * Set the value of the socket to invalid. If this function has been
 * called on a socket, nabto_read, nabto_write, and nabto_socket_close
 * should all fail gracefully.
 *
 * @param socket  A reference to the socket to be set
 */
void nabto_socket_set_invalid(nabto_socket_t* socket);

/**
 * Check if two sockets are equal.
 * 
 * @param s1  first socket to check
 * @param s2  second socket to check
 * @return    true iff the two sockets are equal
 */
bool nabto_socket_is_equal(const nabto_socket_t* s1, const nabto_socket_t* s2);

/**
 * Close a socket. 
 * Close can be called on already closed sockets. And should tolerate this behavior.
 *
 * @param socket the socket to be closed
 */
void nabto_socket_close(nabto_socket_t* socket);

/***************** UDP Read/Write *********************************************/
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
                   struct nabto_ip_address*  addr,
                   uint16_t*      port);

/**
 * Write message to network (blocking) The memory allocation and
 * deallocation for the buffer is handled by the callee.
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
                    const struct nabto_ip_address*  addr,
                    uint16_t       port);


/**
 * Resolve an ipv4 address to an nabto_ip_address. One some systems an
 * ipv4 can be resolved to an ipv4 mapped nat64 address.
 */
void nabto_resolve_ipv4(uint32_t ipv4, struct nabto_ip_address* ip);

/**
 * Get the local ip address
 * This function is optional but is used if the
 * NABTO_ENABLE_GET_LOCAL_IP macro is defined to 1.
 * @param ip      ip in host byte order.
 * @return true   iff ip is set to the local ip address.
 */
bool nabto_get_local_ipv4(struct nabto_ip_address* ip);



#if NABTO_SET_TIME_FROM_ALIVE
/**
 * Set the time received from the GSP.
 * @param stamp  the unix UTC time (since epoch).
 */
void setTimeFromGSP(uint32_t stamp);
#endif

/*************** DNS related functions ***************************************/

typedef enum {
    NABTO_DNS_OK,
    NABTO_DNS_NOT_FINISHED,
    NABTO_DNS_ERROR
} nabto_dns_status_t;

void nabto_dns_resolver(void);

/**
 * start resolving an ip address 
 * afterwards nabto_resolve_dns will be called until the address is resolved
 */
void nabto_dns_resolve(const char* id);

/**
 * resolve an ipv4 dns address
 * if resolving fails in first attempt we call the function later to 
 * see if the address is resolved. The id is always constant for a device
 * meaning the address could be hardcoded but then devices will fail if 
 * the basestation gets a new ip address.
 *
 * The v4addr parameter is an array of size
 * NABTO_DNS_RESOLVED_IPS_MAX. The caller has allocated this array,
 * and the callee should fill up to NABTO_DNS_RESOLVED_IPS_MAX
 * addresses into the array. The minimum size of this array is 1.
 *
 * For example implementation see modules/network/dns/unix
 *
 * @param id      name controller hostname
 * @param v4addr  pointer to output ipaddresses array
 * @return false if address is not resolved yet
 */
nabto_dns_status_t nabto_dns_is_resolved(const char* id, struct nabto_ip_address* addr);

/*************** Time stamp related functions ********************************/
/**
 * Has stamp been passed?
 * @param stamp  pointer to the time stamp
 * @return       true if stamp has been passed or is equal
 */
bool nabtoIsStampPassed(nabto_stamp_t *stamp);

/**
 * Get Current time stamp
 * @return current time stamp
 */
nabto_stamp_t nabtoGetStamp(void);

/**
 * Calculate the difference between two time stamps.
 * @param stamp  pointer to the newest time stamp
 * @param stamp  pointer to the oldest time stamp
 * @return       difference between newest and oldest (newest - oldest)
 */
nabto_stamp_diff_t nabtoStampDiff(nabto_stamp_t * newest, nabto_stamp_t *oldest);

/**
 * Convert the time difference to milliseconds
 * @param stamp  the time difference
 * @return       the time in milliseconds
 */
int nabtoStampDiff2ms(nabto_stamp_diff_t diff);


/** Nabto Attachment state. */
typedef enum {
    NABTO_AS_IDLE,      /**< initial state                                          */
    NABTO_AS_WAIT_DNS,  /**< waiting for dns resolving                              */
    NABTO_AS_WAIT_BS,   /**< UINVITE request sent to Base Station(BS)               */
    NABTO_AS_WAIT_GSP,  /**< UINVITE request sent to GSP                            */
    NABTO_AS_ATTACHED   /**< UATTACH request received from and response sent to GSP */
} nabto_state;

#if NABTO_ENABLE_STATUS_CALLBACKS

/**
 * Inform the application of a state change in the uNabto state machine.
 * @param state  the new state.
 */
void unabto_attach_state_changed(nabto_state state);

#endif


#ifdef __cplusplus
} //extern "C"
#endif

#endif
