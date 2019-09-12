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

#include <string.h>

#include <igloo/error.h>
#include <igloo/types.h>

#define ERROR_BASE(x) .error = igloo_ERROR_ ## x, .name = # x

static const igloo_error_desc_t __desc[] = {
    {
        ERROR_BASE(GENERIC),
        .uuid = "953b7275-21f3-4697-8d30-33f346f9833e",
        .message = "Generic error",
        .description = "A generic error occurred."
    },
    {
        ERROR_BASE(NONE),
        .uuid = "4fa39862-0647-46ea-bfdf-fd2a5c7f6e9d",
        .message = "No error",
        .description = "The operation succeeded."
    }
};

const igloo_error_desc_t *      igloo_error_get_description(igloo_error_t error)
{
    size_t i;

    for (i = 0; i < (sizeof(__desc)/sizeof(*__desc)); i++)
        if (__desc[i].error == error)
            return &(__desc[i]);

    return NULL;
}

const igloo_error_desc_t *      igloo_error_getbyname(const char *name)
{
    size_t i;

    if (!name)
        return NULL;

    for (i = 0; i < (sizeof(__desc)/sizeof(*__desc)); i++) {
        if (__desc[i].name && strcmp(__desc[i].name, name) == 0) {
            return &(__desc[i]);
        }
    }

    return NULL;
}
