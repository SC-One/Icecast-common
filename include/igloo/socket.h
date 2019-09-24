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

#ifndef _LIBIGLOO__SOCKET_H_
#define _LIBIGLOO__SOCKET_H_
/**
 * @file
 * Put a good description of this file here
 */

#ifdef __cplusplus
extern "C" {
#endif

/* Put stuff here */

#include <igloo/config.h>

#include "types.h"
#include "socketaddr.h"

igloo_RO_FORWARD_TYPE(igloo_socket_t);

typedef enum {
    igloo_SOCKET_SHUTDOWN_NONE          = 0x0,
    igloo_SOCKET_SHUTDOWN_RECEIVE       = 0x1,
    igloo_SOCKET_SHUTDOWN_SEND          = 0x2,
    igloo_SOCKET_SHUTDOWN_RECEIVESEND   = igloo_SOCKET_SHUTDOWN_RECEIVE|igloo_SOCKET_SHUTDOWN_SEND
} igloo_socket_shutdown_t;

/* Advanced control functions.
 */

typedef enum {
    igloo_SOCKET_CONTROL_NONE = 0,
    igloo_SOCKET_CONTROL_SET_NODELAY,
    igloo_SOCKET_CONTROL_SET_SEND_BUFFER,
    igloo_SOCKET_CONTROL_SET_RECEIVE_BUFFER
} igloo_socket_control_t;

typedef enum {
    igloo_SOCKET_ADDRESSOP_CLEAR,
    igloo_SOCKET_ADDRESSOP_ADD,
    igloo_SOCKET_ADDRESSOP_REMOVE,
    igloo_SOCKET_ADDRESSOP_REPLACE
} igloo_socket_addressop_t;

typedef enum {
    igloo_SOCKET_ENDPOINT_LOCAL,
    igloo_SOCKET_ENDPOINT_LOCAL_PHYSICAL,
    igloo_SOCKET_ENDPOINT_LOCAL_LOGICAL,
    igloo_SOCKET_ENDPOINT_PEER,
    igloo_SOCKET_ENDPOINT_PEER_PHYSICAL,
    igloo_SOCKET_ENDPOINT_PEER_LOGICAL
} igloo_socket_endpoint_t;

igloo_socket_t * igloo_socket_new(igloo_socketaddr_domain_t domain, igloo_socketaddr_type_t type, igloo_socketaddr_protocol_t protocol, const char *name, igloo_ro_t associated, igloo_ro_t instance, igloo_error_t *error);
igloo_socket_t * igloo_socket_new_simple(igloo_socket_endpoint_t endpoint, igloo_socketaddr_t *addr, igloo_error_t *error);
igloo_error_t igloo_socket_alter_address(igloo_socket_t *sock, igloo_socket_addressop_t op, igloo_socket_endpoint_t endpoint, igloo_socketaddr_t *addr);
igloo_socketaddr_t * igloo_socket_get_main_address(igloo_socket_t *sock, igloo_socket_endpoint_t endpoint, igloo_error_t *error);
igloo_list_t * igloo_socket_get_address(igloo_socket_t *sock, igloo_socket_endpoint_t endpoint, igloo_error_t *error);
igloo_error_t igloo_socket_connect(igloo_socket_t *sock);
igloo_error_t igloo_socket_listen(igloo_socket_t *sock, ssize_t backlog);
igloo_error_t igloo_socket_shutdown(igloo_socket_t *sock, igloo_socket_shutdown_t how);
igloo_socket_t * igloo_socket_accept(igloo_socket_t *sock, igloo_error_t *error);
igloo_error_t igloo_socket_control(igloo_socket_t *sock, igloo_socket_control_t control, ...);


#ifdef __cplusplus
}
#endif

#endif /* ! _LIBIGLOO__SOCKET_H_ */
