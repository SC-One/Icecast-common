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

#include <igloo/socket.h>

struct igloo_socket_tag {
    igloo_ro_base_t __base;
};

static int __new(igloo_ro_t self, const igloo_ro_type_t *type, va_list ap);
static void __free(igloo_ro_t self);
static igloo_ro_t __get_interface_t(igloo_ro_t self, const igloo_ro_type_t *type, const char *name, igloo_ro_t associated, igloo_ro_t instance);

igloo_RO_PUBLIC_TYPE(igloo_socket_t,
        igloo_RO_TYPEDECL_FREE(__free),
        igloo_RO_TYPEDECL_NEW(__new),
        igloo_RO_TYPEDECL_GET_INTERFACE(__get_interface_t)
        );

static int __new(igloo_ro_t self, const igloo_ro_type_t *type, va_list ap)
{
    return 0;
}

static void __free(igloo_ro_t self)
{
}

static igloo_ro_t __get_interface_t(igloo_ro_t self, const igloo_ro_type_t *type, const char *name, igloo_ro_t associated, igloo_ro_t instance)
{
    igloo_socket_t *socket = igloo_RO_TO_TYPE(self, igloo_socket_t);

    if (!socket)
        return igloo_RO_NULL;

    if (type != igloo_RO_GET_TYPE_BY_SYMBOL(igloo_io_t))
        return igloo_RO_NULL;

    return igloo_RO_NULL;
}

