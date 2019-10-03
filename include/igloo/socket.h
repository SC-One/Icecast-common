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

#ifdef IGLOO_CTC_HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#ifdef IGLOO_CTC_HAVE_POLL
#include <poll.h>
#endif

#include "types.h"
#include "socketaddr.h"

igloo_RO_FORWARD_TYPE(igloo_socket_t);

/* Type used for socket shutdown. */
typedef enum {
    /* Do not shut down the socket. */
    igloo_SOCKET_SHUTDOWN_NONE          = 0x0,
    /* Shut the receiving end down. */
    igloo_SOCKET_SHUTDOWN_RECEIVE       = 0x1,
    /* Shut the sending end down. */
    igloo_SOCKET_SHUTDOWN_SEND          = 0x2,
    /* Shut down both ends. */
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

/* Address operations. */
typedef enum {
    /* Clear the list of addresses for the given endpoint. */
    igloo_SOCKET_ADDRESSOP_CLEAR,
    /* Add an address for the given endpoint. */
    igloo_SOCKET_ADDRESSOP_ADD,
    /* Remove an address from the given endpoint. */
    igloo_SOCKET_ADDRESSOP_REMOVE,
    /* Replace all addresses on an endpoint with the new address. */
    igloo_SOCKET_ADDRESSOP_REPLACE
} igloo_socket_addressop_t;

/* Endpoints of the socket.
 *
 * Physical addresses are as seen by the process.
 * Logical addresses are addresses as seen by the peer.
 *
 * E.g. when there is a port forwarding from A:a -> B:b
 * the local address is A:a and the phyical address is B:b.
 */
typedef enum {
    /* Local address. */
    igloo_SOCKET_ENDPOINT_LOCAL,
    /* Local physical address. */
    igloo_SOCKET_ENDPOINT_LOCAL_PHYSICAL,
    /* Local logical address. */
    igloo_SOCKET_ENDPOINT_LOCAL_LOGICAL,
    /* Peer address. */
    igloo_SOCKET_ENDPOINT_PEER,
    /* Peer's physical address. */
    igloo_SOCKET_ENDPOINT_PEER_PHYSICAL,
    /* Peer's logical address. */
    igloo_SOCKET_ENDPOINT_PEER_LOGICAL
} igloo_socket_endpoint_t;

/* Actions sockets can do. */
typedef enum {
    /* No action. */
    igloo_SOCKET_ACTION_NONE = 0,
    /* Connecting to the peer. */
    igloo_SOCKET_ACTION_CONNECT,
    /* Waiting and accepting new client connections. */
    igloo_SOCKET_ACTION_ACCEPT
} igloo_socket_action_t;

/* Create a new socket.
 *
 * Parameters:
 *  domain, type, protocol
 *      Domain, Type, and Protocol for the new socket.
 *  name, associated, instance
 *      See igloo_ro_new().
 *  error
 *      The error value if the socket can not be created.
 */
igloo_socket_t * igloo_socket_new(igloo_socketaddr_domain_t domain, igloo_socketaddr_type_t type, igloo_socketaddr_protocol_t protocol, const char *name, igloo_ro_t associated, igloo_ro_t instance, igloo_error_t *error);

/* Create a new socket based on an address.
 *
 * Parameters:
 *  endpoint
 *      The endpoint to set the address for.
 *      This is normally igloo_SOCKET_ENDPOINT_LOCAL for listen sockets and
 *      igloo_SOCKET_ENDPOINT_PEER for connectig sockets.
 *  addr
 *      The address to be set for the endpoint.
 *  error
 *      The error value if the socket can not be created.
 */
igloo_socket_t * igloo_socket_new_simple(igloo_socket_endpoint_t endpoint, igloo_socketaddr_t *addr, igloo_error_t *error);

/* Alter addresses for the socket.
 *
 * This can be used to set local and peer addresses for the socket.
 * What addresses are supported depends on the domain of the socket.
 *
 * Parameters:
 *  sock
 *      The socket to operate on.
 *  op
 *      The operation to perform. See igloo_socket_addressop_t.
 *  endpoint
 *      The endpoint to operate on. See igloo_socket_endpoint_t.
 *  addr
 *      The address to use for the operation or NULL.
 */
igloo_error_t igloo_socket_alter_address(igloo_socket_t *sock, igloo_socket_addressop_t op, igloo_socket_endpoint_t endpoint, igloo_socketaddr_t *addr);

/* Get the main address for an endpoint.
 *
 * This returns the main address for an endpoint.
 * It depends on the socket Domain, Type, and Protocol to define what the main address is.
 *
 * Parameters:
 *  sock
 *      The socket to operate on.
 *  endpoint
 *      The endpoint to operate on. See igloo_socket_endpoint_t.
 *  error
 *      The error in case the address can not be returned.
 */
igloo_socketaddr_t * igloo_socket_get_main_address(igloo_socket_t *sock, igloo_socket_endpoint_t endpoint, igloo_error_t *error);

/* Get the list of addresses for an endpoint.
 *
 * This returns the list of all known addresses for the given endpoint.
 *
 * Parameters:
 *  sock
 *      The socket to operate on.
 *  endpoint
 *      The endpoint to operate on. See igloo_socket_endpoint_t.
 *  error
 *      The error in case the address can not be returned.
 */
igloo_list_t * igloo_socket_get_address(igloo_socket_t *sock, igloo_socket_endpoint_t endpoint, igloo_error_t *error);

/* Connect the socket.
 *
 * This lets the socket connect to the peer. The operation is blocking.
 * If you want to connect non-blocking use igloo_socket_nonblocking() with igloo_SOCKET_ACTION_CONNECT.
 *
 * Parameters:
 *  sock
 *      The socket to operate on.
 */
igloo_error_t igloo_socket_connect(igloo_socket_t *sock);

/* This sets the socket into the listening state.
 *
 * Parameters:
 *  sock
 *      The socket to operate on.
 *  backlog
 *      The backlog for the listen queue or -1 for default.
 */
igloo_error_t igloo_socket_listen(igloo_socket_t *sock, ssize_t backlog);

/* Shuts the socket down.
 *
 * This allows shuting a socket down. This can be useful e.g. to signal EOF
 * to the peer once all writing has been done.
 *
 * Parameters:
 *  sock
 *      The socket to operate on.
 *  how
 *      How the socket is shut down. See igloo_socket_shutdown_t.
 */
igloo_error_t igloo_socket_shutdown(igloo_socket_t *sock, igloo_socket_shutdown_t how);

/* Accept a client on a listening socket.
 *
 * This accepts a client on a listening connection.
 * This can be called on blocking sockets or after using igloo_socket_nonblocking() with 
 * igloo_SOCKET_ACTION_ACCEPT to fetch a client from the queue.
 *
 * Parameters:
 *  sock
 *      The socket to operate on.
 *  name, associated
 *      See igloo_ro_new().
 *  error
 *      The error if no client can be accepted.
 */
igloo_socket_t * igloo_socket_accept(igloo_socket_t *sock, const char *name, igloo_ro_t associated, igloo_error_t *error);

/* Advanced socket control.
 *
 * Parameters:
 *  sock
 *      The socket to operate on.
 *  control
 *      The control to use. See igloo_socket_control_t.
 *  ...
 *      The parameters for the specific control if any.
 */
igloo_error_t igloo_socket_control(igloo_socket_t *sock, igloo_socket_control_t control, ...);

/* Request an socket action to be done in non-blocking mode.
 *
 * Parameters:
 * sock
 *      The socket to operate on.
 * action
 *      The action to be done.
 *      igloo_SOCKET_ACTION_NONE leaves the non-blocking mode.
 *      igloo_SOCKET_ACTION_CONNECT requests connect.
 *          igloo_socket_connect() must not be called. After this action the socket is set back into blocking mode.
 *      igloo_SOCKET_ACTION_ACCEPT requests accepts.
 *          igloo_socket_accept() must be called to fetch each client connection.
 */
igloo_error_t igloo_socket_nonblocking(igloo_socket_t *sock, igloo_socket_action_t action);
#ifdef IGLOO_CTC_HAVE_SYS_SELECT_H
/* Uses select() for non-blocking operations.
 *
 * This set of functions is used to fill sets for select().
 *
 * The basic idea is:
 * if ((error = igloo_socket_nonblocking_select_set(sock, &readfds, &writefds, &exceptfds, &maxfd)) != igloo_ERROR_NONE)
 *     return error;
 * select(maxfd + 1, &readfds, &writefds, &exceptfds, &timeout);
 * error = igloo_socket_nonblocking_select_result(sock, &readfds, &writefds, &exceptfds);
 * if (error == igloo_ERROR_NONE) {
 *     success...;
 * } else if (error == igloo_ERROR_AGAIN) {
 *     timeout...;
 * } else {
 *     return error;
 * }
 *
 * igloo_socket_nonblocking_select_clear() can be used to clear the socket from the sets in case the sets will be reused without the socket.
 */
igloo_error_t igloo_socket_nonblocking_select_set(igloo_socket_t *sock, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, int *maxfd);
igloo_error_t igloo_socket_nonblocking_select_clear(igloo_socket_t *sock, fd_set *readfds, fd_set *writefds, fd_set *exceptfds);
igloo_error_t igloo_socket_nonblocking_select_result(igloo_socket_t *sock, fd_set *readfds, fd_set *writefds, fd_set *exceptfds);
#endif
#ifdef IGLOO_CTC_HAVE_POLL
/* Uses poll() for non-blocking operations.
 *
 * This set is used to fill the structure for poll().
 *
 * The basic idea is:
 * struct pollfd fd[n];
 * if ((error = igloo_socket_nonblocking_poll_fill(sock, &(fd[m]))) != igloo_ERROR_NONE)
 *     return error;
 * poll(fd, n, timeout);
 * error = igloo_socket_nonblocking_poll_result(sock, &(fd[m]));
 * if (error == igloo_ERROR_NONE) {
 *     success...;
 * } else if (error == igloo_ERROR_AGAIN) {
 *     timeout...;
 * } else {
 *     return error;
 * }
 */
igloo_error_t igloo_socket_nonblocking_poll_fill(igloo_socket_t *sock, struct pollfd *fd);
igloo_error_t igloo_socket_nonblocking_poll_result(igloo_socket_t *sock, struct pollfd *fd);
#endif

#ifdef __cplusplus
}
#endif

#endif /* ! _LIBIGLOO__SOCKET_H_ */
