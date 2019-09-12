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

#define igloo_ERROR_GENERIC         ((igloo_error_t)-1)
#define igloo_ERROR_NONE            ((igloo_error_t)0)

const igloo_error_desc_t *      igloo_error_get_description(igloo_error_t error);
const igloo_error_desc_t *      igloo_error_getbyname(const char *name);

#ifdef __cplusplus
}
#endif

#endif /* ! _LIBIGLOO__ERROR_H_ */
