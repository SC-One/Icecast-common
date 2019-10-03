/* Copyright (C) 2019       Philipp "ph3-der-loewe" Schafft <lion@lion.leolix.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#ifndef _LIBIGLOO__SOCKETADDR_H_
#define _LIBIGLOO__SOCKETADDR_H_
/**
 * @file
 * Put a good description of this file here
 */

#ifdef __cplusplus
extern "C" {
#endif

/* Put stuff here */

/* About thread safety:
 * This set of functions is not thread safe.
 */

#include <igloo/config.h>

#include "types.h"


igloo_RO_FORWARD_TYPE(igloo_socketaddr_t);

/* Domain of sockets */
typedef enum {
    /* Special value used for unspecified or not jet known domain. */
	igloo_SOCKETADDR_DOMAIN_UNSPEC = 0,
    /* Unix Domain Sockets, AKA AF_UNIX */
    igloo_SOCKETADDR_DOMAIN_UNIX,
    /* IPv4 Sockets, AKA AF_INET */
    igloo_SOCKETADDR_DOMAIN_INET4,
    /* IPv6 Sockets, AKA AF_INET6 */
    igloo_SOCKETADDR_DOMAIN_INET6
} igloo_socketaddr_domain_t;

/* Socket type */
typedef enum {
    /* Special value used for unspecified or not jet known type. */
	igloo_SOCKETADDR_TYPE_UNSPEC = 0,
    /* Stream sockets */
	igloo_SOCKETADDR_TYPE_STREAM,
    /* Datagram sockets */
	igloo_SOCKETADDR_TYPE_DGRAM,
    /* Sequenced and reliable datagram sockets */
	igloo_SOCKETADDR_TYPE_SEQPACK,
    /* Reliable datagram sockets */
    igloo_SOCKETADDR_TYPE_RDM
} igloo_socketaddr_type_t;

/* Specific socket protocol */
typedef enum {
    /* Special value used for unspecified or not jet known protocol. */
    igloo_SOCKETADDR_PROTOCOL_UNSPEC = 0,
    /* Transmission Control Protocol */
    igloo_SOCKETADDR_PROTOCOL_TCP,
    /* User Datagram Protocol */
    igloo_SOCKETADDR_PROTOCOL_UDP,
    /* Datagram Congestion Control Protocol */
    igloo_SOCKETADDR_PROTOCOL_DCCP,
    /* Stream Control Transmission Protocol */
    igloo_SOCKETADDR_PROTOCOL_SCTP,
    /* Lightweight User Datagram Protocol */
    igloo_SOCKETADDR_PROTOCOL_UDPLITE
} igloo_socketaddr_protocol_t;

/* This creates a new address object.
 * Parameters:
 *  name, associated, instance
 *      See igloo_ro_new().
 *  domain, type, protocol
 *      Domain, Type, and Protocol for the new address. Can be igloo_SOCKETADDR_*_UNSPEC.
 */
igloo_socketaddr_t *    igloo_socketaddr_new(igloo_socketaddr_domain_t domain, igloo_socketaddr_type_t type, igloo_socketaddr_protocol_t protocol, const char *name, igloo_ro_t associated, igloo_ro_t instance, igloo_error_t *error);

/* Gets base information on the address.
 * Parameters:
 *  addr
 *      The address to operate on.
 *  domain, type, protocol
 *      The address' Domain, Type, and Protocol.
 */
igloo_error_t           igloo_socketaddr_get_base(igloo_socketaddr_t *addr, igloo_socketaddr_domain_t *domain, igloo_socketaddr_type_t *type, igloo_socketaddr_protocol_t *protocol);

/* Get or set the address' node value.
 *
 * The Node is a protocol dependent representation of a node on the network.
 * This might be a nodename or hostname, an IP address, a MAC address or anything else
 * that identifies the node.
 *
 * Parameters:
 *  addr
 *      The address to operate on.
 *  node
 *      The node value to set or get.
 */

igloo_error_t           igloo_socketaddr_set_node(igloo_socketaddr_t *addr, const char *node);
igloo_error_t           igloo_socketaddr_get_node(igloo_socketaddr_t *addr, const char **node);
/* Get the IP address of the address
 *
 * This gets the currently used IP address for the address if any.
 * The IP address can not be set directly. Use igloo_socketaddr_set_node() to set the node.
 *
 * See also igloo_socketaddr_set_node().
 *
 *  addr
 *      The address to operate on.
 *  ip
 *      The ip address used in the address.
 */
igloo_error_t           igloo_socketaddr_get_ip  (igloo_socketaddr_t *addr, const char **ip);

/* Get or set the addresse' port number.
 *
 * This sets or get the port number for addresses of domains that use ports.
 *
 * See also igloo_socketaddr_set_service().
 *
 * Parameters:
 *  addr
 *      The address to operate on.
 *  port
 *      The port value to set or get.
 */
igloo_error_t           igloo_socketaddr_set_port(igloo_socketaddr_t *addr, uint16_t port);
igloo_error_t           igloo_socketaddr_get_port(igloo_socketaddr_t *addr, uint16_t *port);

/* Set the service to use for the address.
 * 
 * For port based domains this takes the port name or number as a string.
 * This is very useful for parsing user input.
 *
 * Parameters:
 *  addr
 *      The address to operate on.
 *  service
 *      The service to set.
 */
igloo_error_t           igloo_socketaddr_set_service(igloo_socketaddr_t *addr, const char *service);

/* Get or set the addresse' path.
 *
 * This sets or get the path for addresses of domains that use paths.
 *
 * Parameters:
 *  addr
 *      The address to operate on.
 *  path
 *      The path value to set or get.
 */
igloo_error_t           igloo_socketaddr_set_path(igloo_socketaddr_t *addr, const char *path);
igloo_error_t           igloo_socketaddr_get_path(igloo_socketaddr_t *addr, const char **path);

#ifdef __cplusplus
}
#endif

#endif /* ! _LIBIGLOO__SOCKETADDR_H_ */
