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

// grep '^#define igloo_ERROR_' include/igloo/error.h | sed 's/^#define igloo_ERROR_\([^ ]*\) \+[^ ]\+ */\1::/; s/::\/\* \+\([^:]\+\): \+\(.\+\) \+\*\//:\1:\2/; s/::\/\* \+\([^:]\+\) \+\*\//:\1:/' | while IFS=: read name message description; do uuid=$(uuid -v 5 c4ce7102-4f5b-4421-9fb7-adbb299e336e "$name"); p='    '; printf "$p{\n$p${p}ERROR_BASE(%s),\n" "$name"; [ -n "$message" ] && printf "$p$p.message = \"%s\",\n" "$message"; [ -n "$description" ] && printf "$p$p.description = \"%s\",\n" "$description"; printf "$p$p.uuid = \"%s\"\n$p},\n" "$uuid"; done | sed '$s/,$//'
static const igloo_error_desc_t __desc[] = {
    {
        ERROR_BASE(GENERIC),
        .message = "Generic error",
        .description = "A generic error occurred.",
        .uuid = "4f4dfebc-0b86-51e7-8233-73ff1c4707cf"
    },
    {
        ERROR_BASE(NONE),
        .message = "No error",
        .description = "The operation succeeded.",
        .uuid = "3f9fa7d9-3d35-5358-bfa4-0c708bc2d7c9"
    },
    {
        ERROR_BASE(FAULT),
        .message = "Invalid address",
        .uuid = "92ef9c60-654f-550d-a256-dc80a85ba0a0"
    },
    {
        ERROR_BASE(INVAL),
        .message = "Invalid argument",
        .uuid = "1146a208-c6aa-59be-93c5-e8c431e48892"
    },
    {
        ERROR_BASE(BUSY),
        .message = "Device or resource busy",
        .uuid = "5d867873-72ba-569c-bf62-f9c784b9ef3f"
    },
    {
        ERROR_BASE(AGAIN),
        .message = "Try again later",
        .uuid = "8b8d4d4b-ff8f-53e0-ad82-aa4d15a4c64b"
    },
    {
        ERROR_BASE(NOMEM),
        .message = "Not enough space",
        .description = "Memory allocation failed.",
        .uuid = "3c236243-fb2e-5f63-a8ff-255a496ff998"
    },
    {
        ERROR_BASE(NOENT),
        .message = "Node does not exist",
        .description = "File, directory, object, or node does not exist.",
        .uuid = "0bd73191-c0a8-5a60-8986-590aad6f937c"
    },
    {
        ERROR_BASE(EXIST),
        .message = "Node exists",
        .description = "Object, or node already exists.",
        .uuid = "2f763d4f-3bd9-5a2a-a5bd-c9da14b721bd"
    },
    {
        ERROR_BASE(PERM),
        .message = "Operation not permitted",
        .uuid = "bd371bfb-1e55-5819-aeb5-e8a085e287cf"
    },
    {
        ERROR_BASE(CONNECTED),
        .message = "Already connected",
        .description = "The operation can not be completed while object is connected.",
        .uuid = "eeabbbd0-ac81-5eb5-ac77-6e474bc6caee"
    },
    {
        ERROR_BASE(UNCONNECTED),
        .message = "Unconnected",
        .description = "The operation can not be completed while object is not connected.",
        .uuid = "7c2e9838-add7-5d2f-8b7a-ba04924075f7"
    },
    {
        ERROR_BASE(IO),
        .message = "Input/output error",
        .uuid = "25541978-3f69-5d63-bd06-c86e299d8f10"
    },
    {
        ERROR_BASE(CANNOTCONNECT),
        .message = "Can not connect",
        .description = "Can not connect to remote resource.",
        .uuid = "3f507c9c-e30b-5c81-91aa-e7c3a21bf3f6"
    },
    {
        ERROR_BASE(NOLOGIN),
        .message = "Can not login",
        .description = "Can not log into service.",
        .uuid = "6fcaf080-dfaf-5ab0-a800-9d67d2d9c64f"
    },
    {
        ERROR_BASE(NOTSECURE),
        .message = "Connection, or object not secure",
        .description = "Connection, or object is not on required security level.",
        .uuid = "393bc752-cb40-5781-b6ce-c23ccc3c46b0"
    },
    {
        ERROR_BASE(BADCERT),
        .message = "Bad certificate",
        .uuid = "07bbfd3a-d432-55b0-a294-a99278e6fa04"
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
