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

typedef enum {
	igloo_SOCKETADDR_DOMAIN_UNSPEC = 0,
    igloo_SOCKETADDR_DOMAIN_UNIX,
    igloo_SOCKETADDR_DOMAIN_INET4,
    igloo_SOCKETADDR_DOMAIN_INET6
} igloo_socketaddr_domain_t;

typedef enum {
	igloo_SOCKETADDR_TYPE_UNSPEC = 0,
	igloo_SOCKETADDR_TYPE_STREAM,
	igloo_SOCKETADDR_TYPE_DGRAM,
	igloo_SOCKETADDR_TYPE_SEQPACK,
    igloo_SOCKETADDR_TYPE_RDM
} igloo_socketaddr_type_t;

typedef enum {
    igloo_SOCKETADDR_PROTOCOL_UNSPEC = 0,
    igloo_SOCKETADDR_PROTOCOL_TCP,
    igloo_SOCKETADDR_PROTOCOL_UDP,
    igloo_SOCKETADDR_PROTOCOL_DCCP,
    igloo_SOCKETADDR_PROTOCOL_SCTP,
    igloo_SOCKETADDR_PROTOCOL_UDPLITE
} igloo_socketaddr_protocol_t;

igloo_socketaddr_t *    igloo_socketaddr_new(igloo_socketaddr_domain_t domain, igloo_socketaddr_type_t type, igloo_socketaddr_protocol_t protocol, const char *name, igloo_ro_t associated, igloo_ro_t instance);
igloo_error_t           igloo_socketaddr_get_base(igloo_socketaddr_t *addr, igloo_socketaddr_domain_t *domain, igloo_socketaddr_type_t *type, igloo_socketaddr_protocol_t *protocol);

igloo_error_t           igloo_socketaddr_set_node(igloo_socketaddr_t *addr, const char *node);
igloo_error_t           igloo_socketaddr_get_node(igloo_socketaddr_t *addr, const char **node);
igloo_error_t           igloo_socketaddr_get_ip  (igloo_socketaddr_t *addr, const char **ip);
igloo_error_t           igloo_socketaddr_set_port(igloo_socketaddr_t *addr, uint16_t port);
igloo_error_t           igloo_socketaddr_get_port(igloo_socketaddr_t *addr, uint16_t *port);
igloo_error_t           igloo_socketaddr_set_path(igloo_socketaddr_t *addr, const char *path);
igloo_error_t           igloo_socketaddr_get_path(igloo_socketaddr_t *addr, const char **path);

#ifdef __cplusplus
}
#endif

#endif /* ! _LIBIGLOO__SOCKETADDR_H_ */
