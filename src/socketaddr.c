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

int igloo_socketaddr_get_sysid_domain(igloo_socketaddr_domain_t domain)
{
    switch (domain) {
        case igloo_SOCKETADDR_DOMAIN_UNSPEC:
            return AF_UNSPEC;
        break;
        case igloo_SOCKETADDR_DOMAIN_UNIX:
            return AF_UNIX;
        break;
        case igloo_SOCKETADDR_DOMAIN_INET4:
            return AF_INET;
        break;
        case igloo_SOCKETADDR_DOMAIN_INET6:
            return AF_INET6;
        break;
    }

    return -1;
}

igloo_socketaddr_domain_t igloo_socketaddr_get_id_domain(int domain)
{
    switch (domain) {
        case AF_UNSPEC:
            return igloo_SOCKETADDR_DOMAIN_UNSPEC;
        break;
        case AF_UNIX:
            return igloo_SOCKETADDR_DOMAIN_UNIX;
        break;
        case AF_INET:
            return igloo_SOCKETADDR_DOMAIN_INET4;
        break;
        case AF_INET6:
            return igloo_SOCKETADDR_DOMAIN_INET6;
        break;
        default:
            return igloo_SOCKETADDR_DOMAIN_UNSPEC;
        break;
    }
}

int igloo_socketaddr_get_sysid_type(igloo_socketaddr_type_t type)
{
    switch (type) {
        case igloo_SOCKETADDR_TYPE_STREAM:
            return SOCK_STREAM;
        break;
        case igloo_SOCKETADDR_TYPE_DGRAM:
            return SOCK_DGRAM;
        break;
        case igloo_SOCKETADDR_TYPE_SEQPACK:
            return SOCK_SEQPACKET;
        break;
#ifdef SOCK_RDM
        case igloo_SOCKETADDR_TYPE_RDM:
            return SOCK_RDM;
        break;
#endif
        case igloo_SOCKETADDR_TYPE_UNSPEC:
            return -1;
    }

    return -1;
}

int igloo_socketaddr_get_sysid_protocol(igloo_socketaddr_protocol_t protocol)
{
    switch (protocol) {
        case igloo_SOCKETADDR_PROTOCOL_UNSPEC:
            return 0;
        break;
        case igloo_SOCKETADDR_PROTOCOL_TCP:
            return IPPROTO_TCP;
        break;
        case igloo_SOCKETADDR_PROTOCOL_UDP:
            return IPPROTO_UDP;
        break;
#ifdef IPPROTO_DCCP
        case igloo_SOCKETADDR_PROTOCOL_DCCP:
            return IPPROTO_DCCP;
        break;
#endif
#ifdef IPPROTO_SCTP
        case igloo_SOCKETADDR_PROTOCOL_SCTP:
            return IPPROTO_SCTP;
        break;
#endif
#ifdef IPPROTO_UDPLITE
        case igloo_SOCKETADDR_PROTOCOL_UDPLITE:
            return IPPROTO_UDPLITE;
        break;
#endif
    }

    return -1;
}

igloo_error_t           igloo_socketaddr_domain2str(const char **str, igloo_socketaddr_domain_t domain)
{
    const char *ret = NULL;

    if (!str)
        return igloo_ERROR_FAULT;

    switch (domain) {
        case igloo_SOCKETADDR_DOMAIN_UNSPEC:
            ret = "unspec";
        break;
        case igloo_SOCKETADDR_DOMAIN_UNIX:
            ret = "unix";
        break;
        case igloo_SOCKETADDR_DOMAIN_INET4:
            ret = "inet4";
        break;
        case igloo_SOCKETADDR_DOMAIN_INET6:
            ret = "inet6";
        break;
    }

    if (ret) {
        *str = ret;
        return igloo_ERROR_NONE;
    } else {
        return igloo_ERROR_NOENT;
    }
}

igloo_error_t           igloo_socketaddr_str2domain(igloo_socketaddr_domain_t *domain, const char *str)
{
    if (!domain || !str)
        return igloo_ERROR_FAULT;

    if (strcasecmp(str, "unspec") == 0) {
        *domain = igloo_SOCKETADDR_DOMAIN_UNSPEC;
    } else if (strcasecmp(str, "unix") == 0) {
        *domain = igloo_SOCKETADDR_DOMAIN_UNIX;
    } else if (strcasecmp(str, "inet4") == 0 || strcasecmp(str, "legacy-inet") == 0) {
        *domain = igloo_SOCKETADDR_DOMAIN_INET4;
    } else if (strcasecmp(str, "inet6") == 0) {
        *domain = igloo_SOCKETADDR_DOMAIN_INET6;
    }

    return igloo_ERROR_NOENT;
}

igloo_error_t           igloo_socketaddr_type2str(const char **str, igloo_socketaddr_type_t type)
{
    const char *ret = NULL;

    if (!str)
        return igloo_ERROR_FAULT;

    switch (type) {
        case igloo_SOCKETADDR_TYPE_UNSPEC:
            ret = "unspec";
        break;
        case igloo_SOCKETADDR_TYPE_STREAM:
            ret = "stream";
        break;
        case igloo_SOCKETADDR_TYPE_DGRAM:
            ret = "dgram";
        break;
        case igloo_SOCKETADDR_TYPE_SEQPACK:
            ret = "seqpack";
        break;
        case igloo_SOCKETADDR_TYPE_RDM:
            ret = "rdm";
        break;
    }

    if (ret) {
        *str = ret;
        return igloo_ERROR_NONE;
    } else {
        return igloo_ERROR_NOENT;
    }
}
igloo_error_t           igloo_socketaddr_str2type(igloo_socketaddr_type_t *type, const char *str)
{
    if (!type || !str)
        return igloo_ERROR_FAULT;

    if (strcasecmp(str, "unspec") == 0) {
        *type = igloo_SOCKETADDR_TYPE_UNSPEC;
    } else if (strcasecmp(str, "stream") == 0) {
        *type = igloo_SOCKETADDR_TYPE_STREAM;
    } else if (strcasecmp(str, "dgram") == 0 || strcasecmp(str, "datagram") == 0) {
        *type = igloo_SOCKETADDR_TYPE_DGRAM;
    } else if (strcasecmp(str, "seqpack") == 0 || strcasecmp(str, "seqpacket") == 0) {
        *type = igloo_SOCKETADDR_TYPE_SEQPACK;
    } else if (strcasecmp(str, "rdm") == 0) {
        *type = igloo_SOCKETADDR_TYPE_RDM;
    }

    return igloo_ERROR_NOENT;
}

igloo_error_t           igloo_socketaddr_protocol2str(const char **str, igloo_socketaddr_protocol_t protocol)
{
    const char *ret = NULL;

    if (!str)
        return igloo_ERROR_FAULT;

    switch (protocol) {
        case igloo_SOCKETADDR_PROTOCOL_UNSPEC:
            ret = "unspec";
        break;
        case igloo_SOCKETADDR_PROTOCOL_TCP:
            ret = "tcp";
        break;
        case igloo_SOCKETADDR_PROTOCOL_UDP:
            ret = "udp";
        break;
        case igloo_SOCKETADDR_PROTOCOL_DCCP:
            ret = "dccp";
        break;
        case igloo_SOCKETADDR_PROTOCOL_SCTP:
            ret = "sctp";
        break;
        case igloo_SOCKETADDR_PROTOCOL_UDPLITE:
            ret = "udplite";
        break;
    }

    if (ret) {
        *str = ret;
        return igloo_ERROR_NONE;
    } else {
        return igloo_ERROR_NOENT;
    }
}
igloo_error_t           igloo_socketaddr_str2protocol(igloo_socketaddr_protocol_t *protocol, const char *str)
{
    if (!protocol || !str)
        return igloo_ERROR_FAULT;

    if (strcasecmp(str, "unspec") == 0) {
        *protocol = igloo_SOCKETADDR_PROTOCOL_UNSPEC;
    } else if (strcasecmp(str, "tcp") == 0) {
        *protocol = igloo_SOCKETADDR_PROTOCOL_TCP;
    } else if (strcasecmp(str, "udp") == 0) {
        *protocol = igloo_SOCKETADDR_PROTOCOL_UDP;
    } else if (strcasecmp(str, "dccp") == 0) {
        *protocol = igloo_SOCKETADDR_PROTOCOL_DCCP;
    } else if (strcasecmp(str, "sctp") == 0) {
        *protocol = igloo_SOCKETADDR_PROTOCOL_SCTP;
    } else if (strcasecmp(str, "udplite") == 0 || strcasecmp(str, "udp-lite") == 0) {
        *protocol = igloo_SOCKETADDR_PROTOCOL_UDPLITE;
    }

    return igloo_ERROR_NOENT;
}


void igloo_socketaddr_complete(igloo_socketaddr_t *addr)
{
    if (!igloo_RO_IS_VALID(addr, igloo_socketaddr_t))
        return;

    if (addr->type == igloo_SOCKETADDR_TYPE_UNSPEC) {
        switch (addr->protocol) {
            case igloo_SOCKETADDR_PROTOCOL_TCP:
                addr->type = igloo_SOCKETADDR_TYPE_STREAM;
            break;
            case igloo_SOCKETADDR_PROTOCOL_UDP:
            case igloo_SOCKETADDR_PROTOCOL_UDPLITE:
            case igloo_SOCKETADDR_PROTOCOL_DCCP:
                addr->type = igloo_SOCKETADDR_TYPE_STREAM;
            break;
            default:
                /* noop */
            break;
        }
    }
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
static const char *_get_ip(igloo_socketaddr_t *addr, const char *name, char *buff, size_t len)
{
    struct addrinfo *head, hints;
    const char *ret = NULL;

    if (_is_ip(name)) {
        strncpy(buff, name, len);
        buff[len-1] = '\0';
        return buff;
    }

    memset(&hints, 0, sizeof (hints));
    hints.ai_family = igloo_socketaddr_get_sysid_domain(addr->domain);
    hints.ai_socktype = igloo_socketaddr_get_sysid_type(addr->type);
    hints.ai_protocol = igloo_socketaddr_get_sysid_protocol(addr->protocol);

    if (getaddrinfo(name, NULL, &hints, &head))
        return NULL;

    if (head) {
        if (getnameinfo(head->ai_addr, head->ai_addrlen, buff, len, NULL, 0, NI_NUMERICHOST) == 0)
            ret = buff;
        freeaddrinfo(head);
    }

    return ret;
}

static igloo_error_t _get_service(igloo_socketaddr_t *addr, const char *name, uint16_t *port)
{
    struct addrinfo *head, hints;
    igloo_error_t ret;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = igloo_socketaddr_get_sysid_domain(addr->domain);
    hints.ai_socktype = igloo_socketaddr_get_sysid_type(addr->type);
    hints.ai_protocol = igloo_socketaddr_get_sysid_protocol(addr->protocol);

    if (getaddrinfo(NULL, name, &hints, &head))
        return igloo_ERROR_GENERIC;

    if (head) {
        ret = igloo_ERROR_NONE;
        switch (head->ai_addr->sa_family) {
            case AF_INET:
                *port = ntohs(((struct sockaddr_in*)head->ai_addr)->sin_port);
            break;
            case AF_INET6:
                *port = ntohs(((struct sockaddr_in6*)head->ai_addr)->sin6_port);
            break;
            default:
                ret = igloo_ERROR_GENERIC;
            break;
        }
        freeaddrinfo(head);
        return ret;
    }

    return igloo_ERROR_GENERIC;
}
#else

static const char *_get_ip(igloo_socketaddr_t *addr, const char *name, char *buff, size_t len)
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

static igloo_error_t _get_service(igloo_socketaddr_t *addr, const char *name, uint16_t *port)
{
    const char *proto = NULL;
    struct servent *res;
    igloo_error_t ret = igloo_ERROR_GENERIC;

    switch (addr->protocol) {
        case igloo_SOCKETADDR_PROTOCOL_TCP:
            proto = "tcp";
        break;
        case igloo_SOCKETADDR_PROTOCOL_UDP:
            proto = "udp";
        break;
        case igloo_SOCKETADDR_PROTOCOL_DCCP:
            proto = "dccp";
        break;
        case igloo_SOCKETADDR_PROTOCOL_SCTP:
            proto = "sctp";
        break;
        case igloo_SOCKETADDR_PROTOCOL_UDPLITE:
            proto = "udplite";
        break;
        default:
            return igloo_ERROR_INVAL;
        break;
    }

    igloo_thread_mutex_lock(&igloo__resolver_mutex);
    res = getservbyname(name, proto);
    if (res) {
        *port = ntohs(res->s_port);
        ret = igloo_ERROR_NONE;
    }
    igloo_thread_mutex_unlock(&igloo__resolver_mutex);

    return ret;
}

#endif

igloo_socketaddr_t *    igloo_socketaddr_new(igloo_socketaddr_domain_t domain, igloo_socketaddr_type_t type, igloo_socketaddr_protocol_t protocol, const char *name, igloo_ro_t associated, igloo_ro_t instance, igloo_error_t *error)
{
    igloo_socketaddr_t *addr = igloo_ro_new_raw(igloo_socketaddr_t, name, associated, instance);

    if (!addr) {
        if (error)
            *error = igloo_ERROR_NOMEM;
        return NULL;
    }

    addr->domain = domain;
    addr->type = type;
    addr->protocol = protocol;

    igloo_socketaddr_complete(addr);

    if (error)
        *error = igloo_ERROR_NONE;

    return addr;
}

igloo_socketaddr_t *    igloo_socketaddr_new_from_sockaddr(igloo_socketaddr_domain_t domain_hint, igloo_socketaddr_type_t type_hint, igloo_socketaddr_protocol_t protocol_hint, const void * /* const struct sockaddr * */ addr, size_t /* socklen_t */ addr_len, const char *name, igloo_ro_t associated, igloo_ro_t instance, igloo_error_t *error)
{
    igloo_socketaddr_t *ret;
    const struct sockaddr *sysaddr = addr;
    char ip[80];
    char service[80];

    if (addr_len < sizeof(*sysaddr)) {
        if (error)
            *error = igloo_ERROR_FAULT;
        return NULL;
    }

    domain_hint = igloo_socketaddr_get_id_domain(sysaddr->sa_family);

    ret = igloo_socketaddr_new(domain_hint, type_hint, protocol_hint, name, associated, instance, error);
    if (ret == NULL)
        return NULL;

    if (getnameinfo(sysaddr, addr_len, ip, sizeof(ip), service, sizeof(service), NI_NUMERICHOST|NI_NUMERICSERV) == 0) {
        igloo_socketaddr_set_node(ret, ip);
        igloo_socketaddr_set_service(ret, service);
    }

    return ret;
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
        _replace_string(&(addr->ip), _get_ip(addr, addr->node, ip, sizeof(ip)));
    }

    if (!addr->ip)
        return igloo_ERROR_GENERIC;

    if (ip)
        *ip = addr->ip;

    igloo_socketaddr_complete(addr);

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

    igloo_socketaddr_complete(addr);

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

igloo_error_t           igloo_socketaddr_set_service(igloo_socketaddr_t *addr, const char *service)
{
    igloo_error_t ret;
    uint16_t port;

    if (!igloo_RO_IS_VALID(addr, igloo_socketaddr_t))
        return igloo_ERROR_FAULT;

    ret = _get_service(addr, service, &port);
    if (ret != igloo_ERROR_NONE)
        return ret;

    return igloo_socketaddr_set_port(addr, port);
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
