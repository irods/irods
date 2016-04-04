

#ifndef SOCK_COMM_NETWORK_INTERFACE_HPP
#define SOCK_COMM_NETWORK_INTERFACE_HPP

// =-=-=-=-=-=-=
#include "irods_network_object.hpp"

// =-=-=-=-=-=-=-
// network plugin interface functions

irods::error readMsgHeader(
    irods::network_object_ptr, // network object
    msgHeader_t*,                   // header
    struct timeval* );              // time value
irods::error readMsgBody(
    irods::network_object_ptr, // network object
    msgHeader_t*,                   // header
    bytesBuf_t*,                    // input struct buf
    bytesBuf_t*,                    // stream buf
    bytesBuf_t*,                    // error buf
    irodsProt_t,                    // protocol
    struct timeval* );              // time value
irods::error writeMsgHeader(
    irods::network_object_ptr, // network object
    msgHeader_t* );                 // header structure
irods::error sendRodsMsg(
    irods::network_object_ptr, // network object,
    const char*,                    // message type
    bytesBuf_t*,                    // message buffer
    bytesBuf_t*,                    // stream buffer
    bytesBuf_t*,                    // error buffer
    int,                            // internal info?
    irodsProt_t );                  // protocol
irods::error readReconMsg(
    irods::network_object_ptr,
    reconnMsg_t** );
irods::error sendReconnMsg(
    irods::network_object_ptr,
    reconnMsg_t* );

// =-=-=-=-=-=-=-
// additional interfaces for network plugin support
// start and stop the network interface, client side
irods::error sockClientStart(
    irods::network_object_ptr,
    rodsEnv* );
irods::error sockClientStop(
    irods::network_object_ptr,
    rodsEnv* );

// =-=-=-=-=-=-=-
// start and stop the network interface, agent side
irods::error sockAgentStart( irods::network_object_ptr );
irods::error sockAgentStop( irods::network_object_ptr );

// =-=-=-=-=-=-=-
// other dependent functions
irods::error readVersion(
    irods::network_object_ptr, // network object
    version_t** );                  // version info
irods::error sendVersion(
    irods::network_object_ptr, // network object
    int,                            // version status
    int,                            // port for reconnection
    char*,                          // address for reconnection
    int );                          // shared cookie

#endif // SOCK_COMM_NETWORK_INTERFACE_HPP



