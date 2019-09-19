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

#include <stdlib.h>
#include <string.h>

#ifndef _WIN32
#ifndef HAVE_INET_PTON
#include <netinet/in.h>
#endif
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#else
#include <winsock2.h>
#endif

#include <igloo/socketaddr.h>
#include <igloo/ro.h>
#include <igloo/error.h>

/* Shouldn't this be 40? Taken from the old sock.h -- ph3-der-loewe, 2019-09-19 */
#define MAX_ADDR_LEN 46

#if !(defined(HAVE_GETNAMEINFO) && defined(HAVE_GETADDRINFO))
static igloo_mutex_t igloo__resolver_mutex;
#endif

#define SET_PORT            0x0001U

struct igloo_socketaddr_tag {
    igloo_ro_base_t __base;

    igloo_socketaddr_domain_t domain;
    igloo_socketaddr_type_t type;
    igloo_socketaddr_protocol_t protocol;

    unsigned int set;

    char *node;
    char *ip;
    char *path;
    uint16_t port;
};

static void __free(igloo_ro_t self);

igloo_RO_PUBLIC_TYPE(igloo_socketaddr_t,
        igloo_RO_TYPEDECL_FREE(__free),
        igloo_RO_TYPEDECL_NEW_NOOP()
        );

static void __free(igloo_ro_t self)
{
    igloo_socketaddr_t *addr = igloo_RO_TO_TYPE(self, igloo_socketaddr_t);
    free(addr->node);
    free(addr->ip);
    free(addr->path);
}

void igloo_socketaddr_initialize(void)
{
#if !(defined(HAVE_GETNAMEINFO) && defined(HAVE_GETADDRINFO))
    igloo_thread_mutex_create(&igloo__resolver_mutex);
#endif

    /* keep dns connects (TCP) open */
#ifdef HAVE_SETHOSTENT
    sethostent(1);
#endif
}

void igloo_socketaddr_shutdown(void)
{
#if !(defined(HAVE_GETNAMEINFO) && defined(HAVE_GETADDRINFO))
    igloo_thread_mutex_destroy(&igloo__resolver_mutex);
#endif

#ifdef HAVE_ENDHOSTENT
    endhostent();
#endif
}

static inline igloo_error_t _replace_string(char **rep, const char *str)
{
    char *val = NULL;

    if (str) {
        val = strdup(str);
        if (!val)
            return igloo_ERROR_NOMEM;
    }

    free(*rep);
    *rep = val;

    return igloo_ERROR_NONE;
}

#ifdef HAVE_INET_PTON
static inline int _is_ip(const char *what)
{
    union {
        struct in_addr v4addr;
        struct in6_addr v6addr;
    } addr_u;

    if (inet_pton(AF_INET, what, &addr_u.v4addr) <= 0)
        return inet_pton(AF_INET6, what, &addr_u.v6addr) > 0 ? 1 : 0;

    return 1;
}

#else
static inline int _is_ip(const char *what)
{
    struct in_addr inp;

    return inet_aton(what, &inp);
}
#endif

#if defined(HAVE_GETNAMEINFO) && defined(HAVE_GETADDRINFO)
static const char *_get_ip(const char *name, char *buff, size_t len)
{
    struct addrinfo *head, hints;
    const char *ret = NULL;

    if (_is_ip(name)) {
        strncpy(buff, name, len);
        buff[len-1] = '\0';
        return buff;
    }

    memset(&hints, 0, sizeof (hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(name, NULL, &hints, &head))
        return NULL;

    if (head) {
        if (getnameinfo(head->ai_addr, head->ai_addrlen, buff, len, NULL, 0, NI_NUMERICHOST) == 0)
            ret = buff;
        freeaddrinfo(head);
    }

    return ret;
}

#else

static const char *_get_ip(const char *name, char *buff, size_t len)
{
    struct hostent *host;
    const char *ret = NULL;

    if (_is_ip(name)) {
        strncpy(buff, name, len);
        buff[len-1] = '\0';
        return buff;
    }

    igloo_thread_mutex_lock(&igloo__resolver_mutex);
    host = gethostbyname(name);
    if (host) {
        char *temp = inet_ntoa(*(struct in_addr *)host->h_addr);
        ret = strncpy(buff, temp, len);
        buff[len-1] = '\0';
    }
    igloo_thread_mutex_unlock(&igloo__resolver_mutex);

    return ret;
}
#endif

igloo_socketaddr_t *    igloo_socketaddr_new(igloo_socketaddr_domain_t domain, igloo_socketaddr_type_t type, igloo_socketaddr_protocol_t protocol, const char *name, igloo_ro_t associated, igloo_ro_t instance)
{
    igloo_socketaddr_t *addr = igloo_ro_new_raw(igloo_socketaddr_t, name, associated, instance);

    if (!addr)
        return NULL;

    addr->domain = domain;
    addr->type = type;
    addr->protocol = protocol;

    return addr;
}

igloo_error_t           igloo_socketaddr_get_base(igloo_socketaddr_t *addr, igloo_socketaddr_domain_t *domain, igloo_socketaddr_type_t *type, igloo_socketaddr_protocol_t *protocol)
{
    if (!igloo_RO_IS_VALID(addr, igloo_socketaddr_t))
        return igloo_ERROR_FAULT;

    if (domain)
        *domain = addr->domain;
    if (type)
        *type = addr->type;
    if (protocol)
        *protocol = addr->protocol;

    return igloo_ERROR_NONE;
}

igloo_error_t           igloo_socketaddr_set_node(igloo_socketaddr_t *addr, const char *node)
{
    if (!igloo_RO_IS_VALID(addr, igloo_socketaddr_t))
        return igloo_ERROR_FAULT;

    if (addr->domain == igloo_SOCKETADDR_DOMAIN_UNIX)
        return igloo_ERROR_GENERIC;

    if (!node) {
        _replace_string(&(addr->node), NULL);
        _replace_string(&(addr->ip), NULL);
        return igloo_ERROR_NONE;
    }

    if (_is_ip(node)) {
        return _replace_string(&(addr->ip), node);
    } else {
        return _replace_string(&(addr->node), node);
    }
}

igloo_error_t           igloo_socketaddr_get_ip(igloo_socketaddr_t *addr, const char **ip)
{
    if (!igloo_RO_IS_VALID(addr, igloo_socketaddr_t))
        return igloo_ERROR_FAULT;

    if (!addr->ip && addr->node) {
        char ip[MAX_ADDR_LEN];
        _replace_string(&(addr->ip), _get_ip(addr->node, ip, sizeof(ip)));
    }

    if (!addr->ip)
        return igloo_ERROR_GENERIC;

    if (ip)
        *ip = addr->ip;

    return igloo_ERROR_NONE;
}

igloo_error_t           igloo_socketaddr_set_port(igloo_socketaddr_t *addr, uint16_t port)
{
    if (!igloo_RO_IS_VALID(addr, igloo_socketaddr_t))
        return igloo_ERROR_FAULT;

    if (addr->domain == igloo_SOCKETADDR_DOMAIN_UNIX)
        return igloo_ERROR_GENERIC;

    addr->port = port;
    addr->set |= SET_PORT;

    return igloo_ERROR_NONE;
}

igloo_error_t           igloo_socketaddr_get_port(igloo_socketaddr_t *addr, uint16_t *port)
{
    if (!igloo_RO_IS_VALID(addr, igloo_socketaddr_t))
        return igloo_ERROR_FAULT;

    if (!(addr->set & SET_PORT))
        return igloo_ERROR_GENERIC;

    if (port)
        *port = addr->port;

    return igloo_ERROR_NONE;
}

igloo_error_t           igloo_socketaddr_set_path(igloo_socketaddr_t *addr, const char *path)
{
    if (!igloo_RO_IS_VALID(addr, igloo_socketaddr_t))
        return igloo_ERROR_FAULT;

    if (addr->domain != igloo_SOCKETADDR_DOMAIN_UNIX)
        return igloo_ERROR_GENERIC;

    return _replace_string(&(addr->path), path);
}

#define _get_string(key) \
igloo_error_t           igloo_socketaddr_get_ ## key (igloo_socketaddr_t *addr, const char **key) \
{ \
    if (!igloo_RO_IS_VALID(addr, igloo_socketaddr_t)) \
        return igloo_ERROR_FAULT; \
\
    if (!addr->key) \
        return igloo_ERROR_GENERIC; \
\
    if (key) \
        *key = addr->key; \
\
    return igloo_ERROR_NONE; \
}

_get_string(node)
_get_string(path)
