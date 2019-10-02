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
igloo_error_t           igloo_socketaddr_get_base(igloo_socketaddr_t *addr, igloo_socketaddr_domain_t *domain, igloo_socketaddr_type_t *type, igloo_socketaddr_protocol_t *protocol);

igloo_error_t           igloo_socketaddr_set_node(igloo_socketaddr_t *addr, const char *node);
igloo_error_t           igloo_socketaddr_get_node(igloo_socketaddr_t *addr, const char **node);
igloo_error_t           igloo_socketaddr_get_ip  (igloo_socketaddr_t *addr, const char **ip);
igloo_error_t           igloo_socketaddr_set_port(igloo_socketaddr_t *addr, uint16_t port);
igloo_error_t           igloo_socketaddr_get_port(igloo_socketaddr_t *addr, uint16_t *port);
igloo_error_t           igloo_socketaddr_set_service(igloo_socketaddr_t *addr, const char *service);
igloo_error_t           igloo_socketaddr_set_path(igloo_socketaddr_t *addr, const char *path);
igloo_error_t           igloo_socketaddr_get_path(igloo_socketaddr_t *addr, const char **path);

#ifdef __cplusplus
}
#endif

#endif /* ! _LIBIGLOO__SOCKETADDR_H_ */
