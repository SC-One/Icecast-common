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

#include <igloo/ro.h>
#include <igloo/io.h>
#include <igloo/error.h>
#include "private.h"

struct igloo_io_tag {
    igloo_interface_base(io)

    /* Mutex for igloo_ro_*(). */
    igloo_mutex_t lock;

    int touched;
#if defined(IGLOO_CTC_HAVE_SYS_SELECT_H) || defined(IGLOO_CTC_HAVE_POLL)
    int fd;
#endif
};

static void __free(igloo_ro_t self)
{
    igloo_io_t *io = igloo_RO_TO_TYPE(self, igloo_io_t);

    igloo_thread_mutex_lock(&(io->lock));
    igloo_thread_mutex_unlock(&(io->lock));
    igloo_thread_mutex_destroy(&(io->lock));
    igloo_interface_base_free(self);
}

igloo_RO_PUBLIC_TYPE(igloo_io_t,
        igloo_RO_TYPEDECL_FREE(__free)
        );


igloo_io_t * igloo_io_new(const igloo_io_ifdesc_t *ifdesc, igloo_ro_t backend_object, void *backend_userdata, const char *name, igloo_ro_t associated, igloo_ro_t instance)
{
    igloo_io_t *io = igloo_interface_base_new(igloo_io_t, ifdesc, backend_object, backend_userdata, name, associated, instance);
    if (!io)
        return NULL;

    igloo_thread_mutex_create(&(io->lock));

    io->touched = 1;

    return io;
}

#define __read_fun(x) \
ssize_t igloo_io_ ## x (igloo_io_t *io, void *buffer, size_t len, igloo_error_t *error) \
{ \
    ssize_t ret = -1; \
    igloo_error_t error_store; \
\
    if (!error) \
        error = &error_store; \
\
    if (!io || !buffer) {\
        *error = igloo_ERROR_FAULT; \
        return -1; \
    } \
\
    if (!len) \
        return 0; \
\
    igloo_thread_mutex_lock(&(io->lock)); \
    io->touched = 1; \
\
    if (io->ifdesc->x) {\
        ret = io->ifdesc->x(igloo_INTERFACE_BASIC_CALL(io), buffer, len, error); \
    } else { \
        *error = igloo_ERROR_GENERIC; \
    } \
    igloo_thread_mutex_unlock(&(io->lock)); \
\
    return ret; \
}

__read_fun(read)
__read_fun(peek)

ssize_t igloo_io_write(igloo_io_t *io, const void *buffer, size_t len, igloo_error_t *error)
{
    ssize_t ret = -1;
    igloo_error_t error_store;

    if (!error)
        error = &error_store;

    if (!io || !buffer) {
        *error = igloo_ERROR_FAULT;
        return -1;
    }

    if (!len)
        return 0;

    igloo_thread_mutex_lock(&(io->lock));
    io->touched = 1;

    if (io->ifdesc->write) {
        ret = io->ifdesc->write(igloo_INTERFACE_BASIC_CALL(io), buffer, len, error);
    } else {
        *error = igloo_ERROR_GENERIC;
    }
    igloo_thread_mutex_unlock(&(io->lock));

    return ret;
}

igloo_error_t igloo_io_flush(igloo_io_t *io, igloo_io_opflag_t flags)
{
    igloo_error_t ret = igloo_ERROR_GENERIC;

    if (!io)
        return igloo_ERROR_FAULT;

    igloo_thread_mutex_lock(&(io->lock));
    io->touched = 1;

    if (io->ifdesc->flush)
        ret = io->ifdesc->flush(igloo_INTERFACE_BASIC_CALL(io), flags);
    igloo_thread_mutex_unlock(&(io->lock));

    return ret;
}

igloo_error_t igloo_io_sync(igloo_io_t *io, igloo_io_opflag_t flags)
{
    igloo_error_t ret;

    if (!io)
        return igloo_ERROR_FAULT;

    igloo_thread_mutex_lock(&(io->lock));
    if (io->ifdesc->flush)
        io->ifdesc->flush(igloo_INTERFACE_BASIC_CALL(io), flags);

    if (io->ifdesc->sync) {
        ret = io->ifdesc->sync(igloo_INTERFACE_BASIC_CALL(io), flags);
        if (ret != igloo_ERROR_NONE) {
            igloo_thread_mutex_unlock(&(io->lock));
            return ret;
        }

        io->touched = 0;

        igloo_thread_mutex_unlock(&(io->lock));
        return ret;
    }
    igloo_thread_mutex_unlock(&(io->lock));

    return igloo_ERROR_GENERIC;
}

igloo_error_t igloo_io_set_blockingmode(igloo_io_t *io, libigloo_io_blockingmode_t mode)
{
    igloo_error_t ret = igloo_ERROR_GENERIC;

    if (!io)
        return igloo_ERROR_FAULT;

    if (mode != igloo_IO_BLOCKINGMODE_NONE && mode != igloo_IO_BLOCKINGMODE_FULL)
        return igloo_ERROR_INVAL;

    igloo_thread_mutex_lock(&(io->lock));
    io->touched = 1;

    if (io->ifdesc->set_blockingmode)
        ret = io->ifdesc->set_blockingmode(igloo_INTERFACE_BASIC_CALL(io), mode);
    igloo_thread_mutex_unlock(&(io->lock));

    return ret;
}
igloo_error_t igloo_io_get_blockingmode(igloo_io_t *io, libigloo_io_blockingmode_t *mode)
{
    libigloo_io_blockingmode_t ret = igloo_IO_BLOCKINGMODE_ERROR;
    igloo_error_t error = igloo_ERROR_GENERIC;

    if (!io || !mode)
        return igloo_ERROR_FAULT;

    igloo_thread_mutex_lock(&(io->lock));
    if (io->ifdesc->get_blockingmode)
        error = io->ifdesc->get_blockingmode(igloo_INTERFACE_BASIC_CALL(io), &ret);
    igloo_thread_mutex_unlock(&(io->lock));

    if (error != igloo_ERROR_NONE)
        return error;

    *mode = ret;

    return igloo_ERROR_NONE;
}

#ifdef IGLOO_CTC_HAVE_SYS_SELECT_H
igloo_error_t igloo_io_select_set(igloo_io_t *io, fd_set *set, int *maxfd, igloo_io_opflag_t flags)
{
    igloo_error_t ret;

    if (!io || !set || !maxfd)
        return igloo_ERROR_FAULT;

    igloo_thread_mutex_lock(&(io->lock));
    if (!io->ifdesc->get_fd_for_systemcall) {
        igloo_thread_mutex_unlock(&(io->lock));
        return igloo_ERROR_GENERIC;
    }

    if (io->touched || !(flags & igloo_IO_OPFLAG_NOWRITE)) {
        igloo_thread_mutex_unlock(&(io->lock));
        ret = igloo_io_sync(io, igloo_IO_OPFLAG_DEFAULTS|(flags & igloo_IO_OPFLAG_NOWRITE));

        if (ret != igloo_ERROR_NONE)
            return ret;

        igloo_thread_mutex_lock(&(io->lock));
        if (io->touched) {
            igloo_thread_mutex_unlock(&(io->lock));
            return igloo_ERROR_GENERIC;
        }
    }

    ret = io->ifdesc->get_fd_for_systemcall(igloo_INTERFACE_BASIC_CALL(io), &(io->fd));
    if (ret != igloo_ERROR_NONE)
        return ret;
    if (io->fd < 0)
        return igloo_ERROR_GENERIC;

    FD_SET(io->fd, set);
    if (*maxfd < io->fd)
        *maxfd = io->fd;

    igloo_thread_mutex_unlock(&(io->lock));

    return igloo_ERROR_NONE;
}

igloo_error_t igloo_io_select_clear(igloo_io_t *io, fd_set *set)
{
    if (!io || !set)
        return igloo_ERROR_FAULT;

    igloo_thread_mutex_lock(&(io->lock));
    if (io->touched || io->fd < 0) {
        igloo_thread_mutex_unlock(&(io->lock));
        return igloo_ERROR_GENERIC;
    }

    FD_CLR(io->fd, set);
    igloo_thread_mutex_unlock(&(io->lock));

    return igloo_ERROR_GENERIC;
}

int igloo_io_select_isset(igloo_io_t *io, fd_set *set)
{
    int ret = -1;

    if (!io || !set)
        return -1;

    igloo_thread_mutex_lock(&(io->lock));
    if (!io->touched && io->fd >= 0)
        ret = FD_ISSET(io->fd, set) ? 1 : 0;
    igloo_thread_mutex_unlock(&(io->lock));

    return ret;
}
#endif

#ifdef IGLOO_CTC_HAVE_POLL
igloo_error_t igloo_io_poll_fill(igloo_io_t *io, struct pollfd *fd, short events)
{
    static const short safe_events = POLLIN|POLLPRI
#ifdef POLLRDHUP
        |POLLRDHUP
#endif
#ifdef POLLRDNORM
        |POLLRDNORM
#endif
    ;
    int is_safe;
    int ret;

    if (!io || !fd)
        return igloo_ERROR_FAULT;

    is_safe = !((events|safe_events) - safe_events);

    igloo_thread_mutex_lock(&(io->lock));
    if (!io->ifdesc->get_fd_for_systemcall) {
        igloo_thread_mutex_unlock(&(io->lock));
        return igloo_ERROR_GENERIC;
    }

    if (io->touched || !is_safe) {
        igloo_thread_mutex_unlock(&(io->lock));
        ret = igloo_io_sync(io, igloo_IO_OPFLAG_DEFAULTS|(is_safe ? 0 : igloo_IO_OPFLAG_NOWRITE));

        if (ret != 0)
            return ret;

        igloo_thread_mutex_lock(&(io->lock));
        if (io->touched) {
            igloo_thread_mutex_unlock(&(io->lock));
            return igloo_ERROR_GENERIC;
        }
    }

    ret = io->ifdesc->get_fd_for_systemcall(igloo_INTERFACE_BASIC_CALL(io), &(io->fd));
    if (io->fd < 0) {
        ret = igloo_ERROR_GENERIC;
    }
    if (ret != igloo_ERROR_NONE) {
        igloo_thread_mutex_unlock(&(io->lock));
        return igloo_ERROR_GENERIC;
    }

    memset(fd, 0, sizeof(*fd));
    fd->fd = io->fd;
    fd->events = events;

    igloo_thread_mutex_unlock(&(io->lock));

    return 0;
}
#endif

igloo_error_t igloo_io_control(igloo_io_t *io, igloo_io_opflag_t flags, igloo_io_control_t control, ...)
{
#ifdef IGLOO_CTC_STDC_HEADERS
    igloo_error_t ret = igloo_ERROR_GENERIC;
    va_list ap;

    if (!io)
        return igloo_ERROR_FAULT;

    igloo_thread_mutex_lock(&(io->lock));
    if (io->ifdesc->control) {
        va_start(ap, control);
        ret = io->ifdesc->control(igloo_INTERFACE_BASIC_CALL(io), flags, control, ap);
        va_end(ap);
    }
    igloo_thread_mutex_unlock(&(io->lock));

    return ret;
#else
    return igloo_ERROR_GENERIC;
#endif
}
