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

#ifndef _LIBIGLOO__LOGCORE_H_
#define _LIBIGLOO__LOGCORE_H_
/**
 * @file
 * Put a good description of this file here
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "ro.h"
#include "interface.h"

/* About thread safety:
 * This set of functions is thread safe.
 */

igloo_RO_FORWARD_TYPE(igloo_logcore_t);

typedef enum {
    igloo_LOGCORE_CLASS_ANY,
    igloo_LOGCORE_CLASS_ALL,
    igloo_LOGCORE_CLASS_DEFAULT
} igloo_logcore_routingclass_t;

typedef struct {
    char *id;
    char *filename;
    igloo_logcore_routingclass_t routingclass;
    ssize_t recent_limit;
    igloo_filter_t *filter;
    igloo_objecthandler_t *formater;
} igloo_logcore_output_t;

typedef igloo_ro_t (*igloo_logcore_acknowledge_t)(igloo_logcore_t *core, igloo_ro_t object, void *userdata);

igloo_error_t   igloo_logcore_configure(igloo_logcore_t *core, ssize_t recent_limit, ssize_t askack_limit, ssize_t output_recent_limit, const igloo_logcore_output_t *outputs, size_t outputs_len);
igloo_error_t   igloo_logcore_set_acknowledge_cb(igloo_logcore_t *core, igloo_logcore_acknowledge_t callback, void *userdata);
igloo_list_t *  igloo_logcore_get_recent(igloo_logcore_t *core, const igloo_ro_type_t *type, igloo_filter_t *filter, ssize_t limit, const char *id);
igloo_list_t *  igloo_logcore_get_askack(igloo_logcore_t *core, const igloo_ro_type_t *type, igloo_filter_t *filter, ssize_t limit);
igloo_error_t   igloo_logcore_acknowledge(igloo_logcore_t *core, igloo_ro_t object);

#ifdef __cplusplus
}
#endif

#endif /* ! _LIBIGLOO__LOGCORE_H_ */
