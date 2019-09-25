/* Icecast
 *
 * This program is distributed under the GNU General Public License, version 2.
 * A copy of this license is included with this source.
 *
 * Copyright 2019,      Philipp "ph3-der-loewe" Schafft <lion@lion.leolix.org>,
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef _WIN32
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include <sys/un.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <fcntl.h>
#else
#include <winsock2.h>
#endif

#include <igloo/socket.h>
#include <igloo/io.h>
#include <igloo/error.h>
#include <igloo/list.h>
#include "private.h"

struct igloo_socket_tag {
    igloo_ro_base_t __base;

    igloo_socketaddr_domain_t domain;
    igloo_socketaddr_type_t type;
    igloo_socketaddr_protocol_t protocol;

    igloo_socketaddr_t *local_physical;
    igloo_socketaddr_t *peer_physical;

    int syssock;
};

static void __free(igloo_ro_t self);
static igloo_ro_t __get_interface_t(igloo_ro_t self, const igloo_ro_type_t *type, const char *name, igloo_ro_t associated, igloo_ro_t instance);

igloo_RO_PUBLIC_TYPE(igloo_socket_t,
        igloo_RO_TYPEDECL_FREE(__free),
        igloo_RO_TYPEDECL_GET_INTERFACE(__get_interface_t)
        );

static inline void __close_socket(int syssock)
{
#ifndef _WIN32
    close(syssock);
#else
    closesocket(syssock);
#endif
}

static void __free(igloo_ro_t self)
{
    igloo_socket_t *sock = igloo_RO_TO_TYPE(self, igloo_socket_t);

    __close_socket(sock->syssock);
}

static ssize_t __read(igloo_INTERFACE_BASIC_ARGS, void *buffer, size_t len, igloo_error_t *error)
{
    igloo_socket_t *sock = igloo_RO_TO_TYPE(*backend_object, igloo_socket_t);
    ssize_t ret = recv(sock->syssock, buffer, len, 0);

    if (ret < 0) {
        *error = igloo_ERROR_GENERIC;
    } else {
        *error = igloo_ERROR_NONE;
    }

    return ret;
}
static ssize_t __write(igloo_INTERFACE_BASIC_ARGS, const void *buffer, size_t len, igloo_error_t *error)
{
    igloo_socket_t *sock = igloo_RO_TO_TYPE(*backend_object, igloo_socket_t);
    ssize_t ret = send(sock->syssock, buffer, len, MSG_NOSIGNAL);

    if (ret < 0) {
        *error = igloo_ERROR_GENERIC;
    } else {
        *error = igloo_ERROR_NONE;
    }

    return ret;
}

static igloo_error_t __sync(igloo_INTERFACE_BASIC_ARGS, igloo_io_opflag_t flags)
{
    return igloo_ERROR_NONE;
}

igloo_error_t __set_blockingmode(igloo_INTERFACE_BASIC_ARGS, libigloo_io_blockingmode_t mode) {
    igloo_socket_t *sock = igloo_RO_TO_TYPE(*backend_object, igloo_socket_t);
#ifdef _WIN32
#ifdef __MINGW32__
    u_long varblock = 1;
#else
    int varblock = 1;
#endif
#else
    int flags;
#endif

#ifdef _WIN32
    if (block)
        varblock = 0;
    return ioctlsocket(sock->syssock, FIONBIO, &varblock) == 0 ? igloo_ERROR_NONE : igloo_ERROR_GENERIC;
#else
    flags = fcntl(sock->syssock, F_GETFL);
    if (flags == -1)
        return igloo_ERROR_GENERIC;

    flags |= O_NONBLOCK;
    if (mode == igloo_IO_BLOCKINGMODE_FULL) {
        flags -= O_NONBLOCK;
    }

    return fcntl(sock->syssock, F_SETFL, flags) == 0 ? igloo_ERROR_NONE : igloo_ERROR_GENERIC;
#endif
}

static igloo_error_t __get_blockingmode(igloo_INTERFACE_BASIC_ARGS, libigloo_io_blockingmode_t *mode)
{
#ifdef _WIN32
    *mode = igloo_IO_BLOCKINGMODE_ERROR;
    return igloo_ERROR_GENERIC;
#else
    igloo_socket_t *sock = igloo_RO_TO_TYPE(*backend_object, igloo_socket_t);
    int flags;

    flags = fcntl(sock->syssock, F_GETFL);
    if (flags == -1) {
        *mode = igloo_IO_BLOCKINGMODE_ERROR;
        return igloo_ERROR_GENERIC;
    }

    if (flags & O_NONBLOCK) {
        *mode = igloo_IO_BLOCKINGMODE_NONE;
    } else {
        *mode = igloo_IO_BLOCKINGMODE_FULL;
    }

    return igloo_ERROR_NONE;
#endif
}

static igloo_error_t __get_fd_for_systemcall(igloo_INTERFACE_BASIC_ARGS, int *fd)
{
    igloo_socket_t *sock = igloo_RO_TO_TYPE(*backend_object, igloo_socket_t);
    *fd = sock->syssock;
    return igloo_ERROR_NONE;
}

static const igloo_io_ifdesc_t igloo_socket_io_ifdesc = {
    igloo_INTERFACE_DESCRIPTION_BASE(igloo_io_ifdesc_t),
    .read = __read,
    .write = __write,
    .sync = __sync,
    .get_blockingmode = __get_blockingmode,
    .get_fd_for_systemcall = __get_fd_for_systemcall
};

static igloo_ro_t __get_interface_t(igloo_ro_t self, const igloo_ro_type_t *type, const char *name, igloo_ro_t associated, igloo_ro_t instance)
{
    igloo_socket_t *sock = igloo_RO_TO_TYPE(self, igloo_socket_t);

    if (!sock)
        return igloo_RO_NULL;

    if (type != igloo_RO_GET_TYPE_BY_SYMBOL(igloo_io_t))
        return igloo_RO_NULL;

    return (igloo_ro_t)igloo_io_new(&igloo_socket_io_ifdesc, self, NULL, name, associated, instance);
}

static igloo_error_t __bind_or_connect(igloo_socket_t *sock, igloo_socketaddr_t *addr, int do_connect)
{
    union {
        struct sockaddr sa;
        struct sockaddr_un un;
    } sysaddr_store;
    struct sockaddr *sa;
    socklen_t sa_len;
    igloo_socketaddr_domain_t domain;
    igloo_socketaddr_type_t type;
    igloo_socketaddr_protocol_t protocol;
    igloo_error_t error;
    int sysaf;
    const char *value;
    uint16_t port;
    struct addrinfo hints, *res, *ai;
    char service[10];
    int already_done = 0;
    int ret = -1;

    memset(&sysaddr_store, 0, sizeof(sysaddr_store));

    error = igloo_socketaddr_get_base(addr, &domain, &type, &protocol);
    if (error != igloo_ERROR_NONE)
        return error;

    sysaf = igloo_socketaddr_get_sysid_domain(domain);

    switch (domain) {
        case igloo_SOCKETADDR_DOMAIN_UNIX:
            error = igloo_socketaddr_get_path(addr, &value);
            if (error != igloo_ERROR_NONE)
                return error;

            if (!value || strlen(value) > sizeof(sysaddr_store.un.sun_path))
                return igloo_ERROR_GENERIC;

            sysaddr_store.un.sun_family = sysaf;
            strncpy(sysaddr_store.un.sun_path, value, sizeof(sysaddr_store.un.sun_path));
            sa_len = sizeof(sysaddr_store.un);
            sa = &(sysaddr_store.sa);
        break;
        case igloo_SOCKETADDR_DOMAIN_INET4:
        case igloo_SOCKETADDR_DOMAIN_INET6:
            error = igloo_socketaddr_get_ip(addr, &value);
            if (error != igloo_ERROR_NONE)
                return error;

            error = igloo_socketaddr_get_port(addr, &port);
            if (error != igloo_ERROR_NONE)
                return error;

            memset(&hints, 0, sizeof(hints));

            hints.ai_family = igloo_socketaddr_get_sysid_domain(domain);
            hints.ai_socktype = igloo_socketaddr_get_sysid_type(type);
            hints.ai_protocol = igloo_socketaddr_get_sysid_protocol(protocol);
            hints.ai_flags = AI_ADDRCONFIG|AI_NUMERICSERV|AI_NUMERICHOST;

            if (!do_connect) {
                hints.ai_flags |= AI_PASSIVE;
            }

            snprintf(service, sizeof(service), "%u", (unsigned int)port);

            if ((ret = getaddrinfo(value, service, &hints, &res)) != 0) {
                return igloo_ERROR_GENERIC;
            }

            ai = res;

            do {
                if (do_connect) {
                    ret = connect(sock->syssock, ai->ai_addr, ai->ai_addrlen);
                } else {
                    ret = bind(sock->syssock, ai->ai_addr, ai->ai_addrlen);
                }
            } while (ret != 0 && (ai = ai->ai_next));

            freeaddrinfo(res);
            already_done = 1;
        break;
        default:
            return igloo_ERROR_GENERIC;
        break;
    }


    if (!already_done) {
        if (do_connect) {
            ret = connect(sock->syssock, sa, sa_len);
        } else {
            ret = bind(sock->syssock, sa, sa_len);
        }
    }

    if (ret == 0) {
        return igloo_ERROR_NONE;
    } else {
        return igloo_ERROR_GENERIC;
    }
}

igloo_socket_t * igloo_socket_new__real(igloo_socketaddr_domain_t domain, igloo_socketaddr_type_t type, igloo_socketaddr_protocol_t protocol, const char *name, igloo_ro_t associated, igloo_ro_t instance, igloo_error_t *error, int syssock)
{
    igloo_socket_t *sock = igloo_ro_new_raw(igloo_socket_t, name, associated, instance);

    if (!sock) {
        if (error)
            *error = igloo_ERROR_NOMEM;
        return NULL;
    }

    sock->domain = domain;
    sock->type = type;
    sock->protocol = protocol;

    if (syssock >= 0) {
        sock->syssock = syssock;
    } else {
        sock->syssock = socket(igloo_socketaddr_get_sysid_domain(domain), igloo_socketaddr_get_sysid_type(type), igloo_socketaddr_get_sysid_protocol(protocol));
        if (sock->syssock == -1) {
            igloo_ro_unref(sock);
            if (error)
                *error = igloo_ERROR_GENERIC;
            return NULL;
        }
    }

    return sock;
}

igloo_socket_t * igloo_socket_new(igloo_socketaddr_domain_t domain, igloo_socketaddr_type_t type, igloo_socketaddr_protocol_t protocol, const char *name, igloo_ro_t associated, igloo_ro_t instance, igloo_error_t *error)
{
    return igloo_socket_new__real(domain, type, protocol, name, associated, instance, error, -1);
}

igloo_socket_t * igloo_socket_new_simple(igloo_socket_endpoint_t endpoint, igloo_socketaddr_t *addr, igloo_error_t *error)
{
    igloo_socketaddr_domain_t domain;
    igloo_socketaddr_type_t type;
    igloo_socketaddr_protocol_t protocol;
    igloo_error_t err;
    igloo_socket_t *sock;

    err = igloo_socketaddr_get_base(addr, &domain, &type, &protocol);
    if (err != igloo_ERROR_NONE) {
        if (error)
            *error = err;
        return NULL;
    }

    sock = igloo_socket_new(domain, type, protocol, NULL, igloo_RO_NULL, addr, &err);
    if (!sock) {
        if (error)
            *error = err;
        return NULL;
    }

    err = igloo_socket_alter_address(sock, igloo_SOCKET_ADDRESSOP_ADD, endpoint, addr);
    if (err != igloo_ERROR_NONE) {
        igloo_ro_unref(sock);
        if (error)
            *error = err;
        return NULL;
    }

    return sock;
}

static inline igloo_error_t _replace_addr(igloo_socketaddr_t **storage, igloo_socketaddr_t *addr)
{
    igloo_error_t ret;

    if (addr) {
        ret = igloo_ro_ref(addr);
        if (ret != igloo_ERROR_NONE)
            return ret;
    }

    igloo_ro_unref(*storage);
    *storage = addr;

    return igloo_ERROR_NONE;
}

igloo_error_t igloo_socket_alter_address(igloo_socket_t *sock, igloo_socket_addressop_t op, igloo_socket_endpoint_t endpoint, igloo_socketaddr_t *addr)
{
    igloo_socketaddr_t **storage = NULL;

    if (!igloo_RO_IS_VALID(sock, igloo_socket_t))
        return igloo_ERROR_FAULT;

    switch (endpoint) {
        case igloo_SOCKET_ENDPOINT_LOCAL:
        case igloo_SOCKET_ENDPOINT_LOCAL_PHYSICAL:
            storage = &(sock->local_physical);
        break;
        case igloo_SOCKET_ENDPOINT_PEER:
        case igloo_SOCKET_ENDPOINT_PEER_PHYSICAL:
            storage = &(sock->peer_physical);
        break;
        default:
            return igloo_ERROR_GENERIC;
        break;
    }

    switch (op) {
        case igloo_SOCKET_ADDRESSOP_CLEAR:
            return _replace_addr(storage, NULL);
        break;
        case igloo_SOCKET_ADDRESSOP_REPLACE:
            return _replace_addr(storage, addr);
        break;
        case igloo_SOCKET_ADDRESSOP_ADD:
            if (*storage)
                return igloo_ERROR_GENERIC;
            return _replace_addr(storage, addr);
        break;
        case igloo_SOCKET_ADDRESSOP_REMOVE:
            if (*storage != addr)
                return igloo_ERROR_GENERIC;
            return _replace_addr(storage, NULL);
        break;
        default:
            return igloo_ERROR_GENERIC;
        break;
    }
}

igloo_socketaddr_t * igloo_socket_get_main_address(igloo_socket_t *sock, igloo_socket_endpoint_t endpoint, igloo_error_t *error)
{
    igloo_socketaddr_t *ret = NULL;
    igloo_error_t err;

    if (!igloo_RO_IS_VALID(sock, igloo_socket_t)) {
        if (error)
            *error = igloo_ERROR_FAULT;
        return NULL;
    }

    switch (endpoint) {
        case igloo_SOCKET_ENDPOINT_LOCAL:
        case igloo_SOCKET_ENDPOINT_LOCAL_PHYSICAL:
            ret = sock->local_physical;
        break;
        case igloo_SOCKET_ENDPOINT_PEER:
        case igloo_SOCKET_ENDPOINT_PEER_PHYSICAL:
            ret = sock->peer_physical;
        break;
        default:
            if (error)
                *error = igloo_ERROR_GENERIC;
            return NULL;
        break;
    }

    if (ret) {
        err = igloo_ro_ref(ret);
        if (err != igloo_ERROR_NONE) {
            if (error)
                *error = err;
            return NULL;
        }
    }

    if (error)
        *error = igloo_ERROR_NONE;
    return ret;
}

igloo_list_t * igloo_socket_get_address(igloo_socket_t *sock, igloo_socket_endpoint_t endpoint, igloo_error_t *error)
{
    igloo_error_t err;
    igloo_socketaddr_t *addr;
    igloo_list_t *list;

    addr = igloo_socket_get_main_address(sock, endpoint, &err);
    if (err != igloo_ERROR_NONE) {
        if (error)
            *error = err;
        return NULL;
    }

    list = igloo_ro_new_ext(igloo_list_t, NULL, igloo_RO_NULL, addr);
    if (!list) {
        igloo_ro_unref(addr);
        if (error)
            *error = igloo_ERROR_NOMEM;
        return NULL;
    }

    if (igloo_list_push(list, addr) != 0) {
        igloo_ro_unref(addr);
        igloo_ro_unref(list);
        if (error)
            *error = igloo_ERROR_GENERIC;
        return NULL;
    }

    igloo_ro_unref(addr);

    if (error)
        *error = igloo_ERROR_NONE;

    return list;
}

igloo_error_t igloo_socket_connect(igloo_socket_t *sock)
{
    if (!igloo_RO_IS_VALID(sock, igloo_socket_t))
        return igloo_ERROR_FAULT;

    if (sock->local_physical) {
        igloo_error_t error;
        error = __bind_or_connect(sock, sock->local_physical, 0);
        if (error != igloo_ERROR_NONE)
            return error;
    }

    return __bind_or_connect(sock, sock->peer_physical, 1);
}

igloo_error_t igloo_socket_listen(igloo_socket_t *sock, ssize_t backlog)
{
    if (!igloo_RO_IS_VALID(sock, igloo_socket_t))
        return igloo_ERROR_FAULT;

    if (backlog < 1)
        backlog = 16;

    if (sock->local_physical) {
        igloo_error_t error;
        error = __bind_or_connect(sock, sock->local_physical, 0);
        if (error != igloo_ERROR_NONE)
            return error;
    }

    if (listen(sock->syssock, backlog) != 0)
        return igloo_ERROR_GENERIC;

    return igloo_ERROR_NONE;
}

igloo_error_t igloo_socket_shutdown(igloo_socket_t *sock, igloo_socket_shutdown_t how)
{
    int syshow;

    if (!igloo_RO_IS_VALID(sock, igloo_socket_t))
        return igloo_ERROR_FAULT;

    switch (how) {
        case igloo_SOCKET_SHUTDOWN_NONE:
            return igloo_ERROR_NONE;
        break;
        case igloo_SOCKET_SHUTDOWN_RECEIVE:
            syshow = SHUT_RD;
        break;
        case igloo_SOCKET_SHUTDOWN_SEND:
            syshow = SHUT_WR;
        break;
        case igloo_SOCKET_SHUTDOWN_RECEIVESEND:
            syshow = SHUT_RDWR;
        break;
        default:
            return igloo_ERROR_INVAL;
        break;
    }

    if (shutdown(sock->syssock, syshow) != 0)
        return igloo_ERROR_GENERIC;

    return igloo_ERROR_NONE;
}

igloo_socket_t * igloo_socket_accept(igloo_socket_t *sock, const char *name, igloo_ro_t associated, igloo_error_t *error)
{
    struct sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);
    igloo_socket_t *ret;
    igloo_socketaddr_t *peer;
    int res;
    int domain;

    if (!igloo_RO_IS_VALID(sock, igloo_socket_t)) {
        if (error)
            *error = igloo_ERROR_FAULT;
        return NULL;
    }

    res = accept(sock->syssock, (struct sockaddr*)&addr, &addr_len);
    if (res < 0) {
        if (error)
            *error = igloo_ERROR_GENERIC;
        return NULL;
    }

    if (addr_len >= sizeof(struct sockaddr)) {
        domain = ((struct sockaddr*)&addr)->sa_family;
    } else {
        domain = igloo_socketaddr_get_sysid_domain(sock->domain);
    }

    ret = igloo_socket_new__real(domain, sock->type, sock->protocol, name, associated, sock, error, res);
    if (ret == NULL) {
        __close_socket(res);
        return NULL;
    }

    if (domain == igloo_socketaddr_get_sysid_domain(sock->domain)) {
        igloo_socket_alter_address(ret, igloo_SOCKET_ADDRESSOP_REPLACE, igloo_SOCKET_ENDPOINT_LOCAL_PHYSICAL, sock->local_physical);
    }

    peer = igloo_socketaddr_new_from_sockaddr(sock->domain, sock->type, sock->protocol, &addr, addr_len, NULL, igloo_RO_NULL, sock, NULL);
    if (peer) {
        igloo_socket_alter_address(ret, igloo_SOCKET_ADDRESSOP_REPLACE, igloo_SOCKET_ENDPOINT_PEER_PHYSICAL, peer);
        igloo_ro_unref(peer);
    }

    if (error)
        *error = igloo_ERROR_NONE;
    return ret;
}

igloo_error_t igloo_socket_control(igloo_socket_t *sock, igloo_socket_control_t control, ...)
{
    if (!igloo_RO_IS_VALID(sock, igloo_socket_t))
        return igloo_ERROR_FAULT;

    return igloo_ERROR_GENERIC;
}
