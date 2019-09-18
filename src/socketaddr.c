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

#include <igloo/socketaddr.h>
#include <igloo/ro.h>

struct igloo_socketaddr_tag {
    igloo_ro_base_t __base;
};

static void __free(igloo_ro_t self);

igloo_RO_PUBLIC_TYPE(igloo_socketaddr_t,
        igloo_RO_TYPEDECL_FREE(__free),
        igloo_RO_TYPEDECL_NEW_NOOP()
        );

static void __free(igloo_ro_t self)
{
}
