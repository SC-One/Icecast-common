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

#ifndef _LIBIGLOO__ERROR_H_
#define _LIBIGLOO__ERROR_H_
/**
 * @file
 * Put a good description of this file here
 */

#ifdef __cplusplus
extern "C" {
#endif

/* Put stuff here */

#include <igloo/config.h>
#include <igloo/types.h>

typedef struct {
    igloo_error_t error;
    const char *uuid;
    const char *name;
    const char *message;
    const char *description;
} igloo_error_desc_t;

/*
 * NOTE: The following lines bust keep their exact formating as it is used for code generation!
 */

#define igloo_ERROR_GENERIC         ((igloo_error_t)-1) /* Generic error: A generic error occurred. */
#define igloo_ERROR_NONE            ((igloo_error_t)0)  /* No error: The operation succeeded. */

#define igloo_ERROR_FAULT           ((igloo_error_t)1)  /* Invalid address */
#define igloo_ERROR_INVAL           ((igloo_error_t)2)  /* Invalid argument */
#define igloo_ERROR_BUSY            ((igloo_error_t)3)  /* Device or resource busy */
#define igloo_ERROR_AGAIN           ((igloo_error_t)4)  /* Try again later */
#define igloo_ERROR_NOMEM           ((igloo_error_t)5)  /* Not enough space: Memory allocation failed. */
#define igloo_ERROR_NOENT           ((igloo_error_t)6)  /* Node does not exist: File, directory, object, or node does not exist. */
#define igloo_ERROR_EXIST           ((igloo_error_t)7)  /* Node exists: Object, or node already exists. */
#define igloo_ERROR_PERM            ((igloo_error_t)8)  /* Operation not permitted */
#define igloo_ERROR_CONNECTED       ((igloo_error_t)9)  /* Already connected: The operation can not be completed while object is connected. */
#define igloo_ERROR_UNCONNECTED     ((igloo_error_t)10) /* Unconnected: The operation can not be completed while object is not connected. */
#define igloo_ERROR_IO              ((igloo_error_t)11) /* Input/output error */
#define igloo_ERROR_CANNOTCONNECT   ((igloo_error_t)12) /* Can not connect: Can not connect to remote resource. */
#define igloo_ERROR_NOLOGIN         ((igloo_error_t)13) /* Can not login: Can not log into service. */
#define igloo_ERROR_NOTSECURE       ((igloo_error_t)14) /* Connection, or object not secure: Connection, or object is not on required security level. */
#define igloo_ERROR_BADCERT         ((igloo_error_t)15) /* Bad certificate */

const igloo_error_desc_t *      igloo_error_get_description(igloo_error_t error);
const igloo_error_desc_t *      igloo_error_getbyname(const char *name);

#ifdef __cplusplus
}
#endif

#endif /* ! _LIBIGLOO__ERROR_H_ */
