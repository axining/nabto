#ifndef _UNABTO_CONNECTION_UTIL_H_
#define _UNABTO_CONNECTION_UTIL_H_

#include <unabto/unabto_packet_util.h>
#include <unabto/unabto_connection.h>

// Read client id from packet and insert it into the connection,
// return true if ok or false if client id not found or unsuccessful.
bool unabto_connection_util_read_client_id(const nabto_packet_header* header, nabto_connect* connection);

// read fingerprint payload from connect packet.
bool unabto_connection_util_read_fingerprint(const nabto_packet_header* header, nabto_connect* connection);

// read unencrypted client nonce
bool unabto_connection_util_read_nonce_client(const nabto_packet_header* header, nabto_connect* connection);

// read random from client
bool unabto_connection_util_read_random_client(const uint8_t* payloadsBegin, const uint8_t* payloadsEnd, nabto_connect* connection);

bool unabto_connection_util_read_and_validate_nonce_client(const uint8_t* payloadsBegin, const uint8_t* payloadsEnd, nabto_connect* connection);

// read key id
bool unabto_connection_util_read_key_id(const nabto_packet_header* header, nabto_connect* connection);

// read capabilities
bool unabto_connection_util_read_capabilities(const nabto_packet_header* header, struct unabto_payload_capabilities_read* capabilities);


bool unabto_connection_util_verify_capabilities(nabto_connect* connection, struct unabto_payload_capabilities_read* capabilities);




// Precondition: the keyid, client id and fingerprint has been read
// from the packet.
// PostCondition: the connections crypto context is
// initialized with the appropriate psk.
bool unabto_connection_util_psk_connect_init_key(nabto_connect* connection);

// verify the integrity of a connect request
bool unabto_psk_connection_util_verify_connect(const nabto_packet_header* header, nabto_connect* connection);

uint8_t* unabto_psk_connection_insert_nonce_device();

#endif
