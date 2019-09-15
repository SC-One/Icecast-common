/* Copyright (C) 2018       Philipp "ph3-der-loewe" Schafft <lion@lion.leolix.org>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>

#include "../include/igloo/typedef.h"
typedef struct igloo_instance_tag igloo_instance_t;

#define igloo_RO_PRIVATETYPES igloo_RO_TYPE(igloo_instance_t)

#include "../include/igloo/ro.h"
#include "../include/igloo/error.h"
#include "../include/igloo/objecthandler.h"
#include "private.h"

struct igloo_instance_tag {
    igloo_ro_base_t __base;
    igloo_mutex_t lock;
    igloo_error_t error;
    igloo_objecthandler_t *logger;
};

static size_t igloo_initialize__refc;
static igloo_ro_t default_instance = igloo_RO_NULL;


static void igloo_initialize__free(igloo_ro_t self);
igloo_RO_PRIVATE_TYPE(igloo_instance_t,
        igloo_RO_TYPEDECL_FREE(igloo_initialize__free)
        );

/* Internal forwarding */
const igloo_ro_type_t **igloo_instance_type = &igloo_ro__type__igloo_instance_t;

static void igloo_initialize__free(igloo_ro_t self)
{
    igloo_instance_t *instance = igloo_RO_TO_TYPE(self, igloo_instance_t);

    if (igloo_RO_IS_SAME(default_instance, self)) {
        /* This is hacky, but needed to avoid free-before-unlock. */
        igloo_RO__GETBASE(default_instance)->wrefc--;
        default_instance = igloo_RO_NULL;
    }


    igloo_thread_mutex_lock(&(instance->lock));
    igloo_ro_weak_unref(instance->logger);
    igloo_thread_mutex_unlock(&(instance->lock));
    igloo_thread_mutex_destroy(&(instance->lock));

    igloo_initialize__refc--;
    if (igloo_initialize__refc)
        return;

    igloo_resolver_shutdown();
    igloo_sock_shutdown();
    igloo_thread_shutdown();
    igloo_log_shutdown();
}

igloo_ro_t     igloo_initialize(void)
{
    igloo_instance_t *ret;
    char name[128];

    if (!igloo_initialize__refc) {
        igloo_log_initialize();
        igloo_thread_initialize();
        igloo_sock_initialize();
        igloo_resolver_initialize();
    }

    snprintf(name, sizeof(name), "<libigloo instance %zu>", igloo_initialize__refc);

    ret = igloo_ro_new_raw(igloo_instance_t, name, igloo_RO_NULL, igloo_RO_NULL);
    if (!ret)
        return igloo_RO_NULL;

    igloo_thread_mutex_create(&(ret->lock));

    igloo_initialize__refc++;

    if (igloo_RO_IS_NULL(default_instance)) {
        if (igloo_ro_weak_ref(ret) == igloo_ERROR_NONE) {
            default_instance = (igloo_ro_t)ret;
        }
    }

    return (igloo_ro_t)ret;
}

igloo_ro_t igloo_get_default_instance(void)
{
    if (igloo_RO_IS_NULL(default_instance))
        return igloo_RO_NULL;

    if (igloo_ro_ref(default_instance) != igloo_ERROR_NONE)
        return igloo_RO_NULL;

    return default_instance;
}

static igloo_instance_t * igloo_instance_get_instance(igloo_ro_t self)
{
    igloo_instance_t *instance;

    if (igloo_RO_IS_NULL(self))
        self = default_instance;

    instance = igloo_RO_TO_TYPE(igloo_ro_get_instance(self), igloo_instance_t);

    return instance;
}

igloo_error_t igloo_instance_get_error(igloo_ro_t self, igloo_error_t *result)
{
    igloo_instance_t *instance = igloo_instance_get_instance(self);

    if (!instance)
        return igloo_ERROR_GENERIC;

    igloo_thread_mutex_lock(&(instance->lock));
    if (result)
        *result = instance->error;
    igloo_thread_mutex_unlock(&(instance->lock));

    igloo_ro_unref(instance);

    return igloo_ERROR_NONE;
}

igloo_error_t igloo_instance_set_error(igloo_ro_t self, igloo_error_t error)
{
    igloo_instance_t *instance = igloo_instance_get_instance(self);

    if (!instance)
        return igloo_ERROR_GENERIC;

    igloo_thread_mutex_lock(&(instance->lock));
    instance->error = error;
    igloo_thread_mutex_unlock(&(instance->lock));

    igloo_ro_unref(instance);

    return igloo_ERROR_NONE;
}

igloo_error_t igloo_instance_set_logger(igloo_ro_t self, igloo_objecthandler_t *logger)
{
    igloo_instance_t *instance;
    igloo_error_t ret;

    if (!igloo_RO_IS_NULL(logger) && !igloo_RO_IS_VALID(logger, igloo_objecthandler_t))
        return igloo_ERROR_GENERIC;

    instance = igloo_instance_get_instance(self);

    if (!instance)
        return igloo_ERROR_GENERIC;

    if (!igloo_RO_IS_NULL(logger)) {
        ret = igloo_ro_weak_ref(logger);
        if (ret != igloo_ERROR_NONE) {
            igloo_ro_unref(instance);
            return ret;
        }
    }

    igloo_thread_mutex_lock(&(instance->lock));
    igloo_ro_weak_unref(instance->logger);
    instance->logger = logger;
    igloo_thread_mutex_unlock(&(instance->lock));

    igloo_ro_unref(instance);

    return igloo_ERROR_NONE;
}

igloo_objecthandler_t * igloo_instance_get_logger(igloo_ro_t self)
{
    igloo_instance_t *instance = igloo_instance_get_instance(self);
    igloo_objecthandler_t *ret = NULL;

    if (!instance)
        return NULL;

    igloo_thread_mutex_lock(&(instance->lock));
    if (igloo_ro_ref(instance->logger) == igloo_ERROR_NONE) {
        ret = instance->logger;
    }
    igloo_thread_mutex_unlock(&(instance->lock));

    igloo_ro_unref(instance);

    return ret;
}

igloo_error_t igloo_instance_log(igloo_ro_t self, igloo_ro_t msg)
{
    igloo_instance_t *instance = igloo_instance_get_instance(self);
    igloo_error_t ret;

    if (!instance)
        return igloo_ERROR_FAULT;

    igloo_thread_mutex_lock(&(instance->lock));
    switch (igloo_objecthandler_handle(instance->logger, msg)) {
        case igloo_FILTER_RESULT_PASS:
            ret = igloo_ERROR_NONE;
        break;
        case igloo_FILTER_RESULT_DROP:
            ret = igloo_ERROR_GENERIC;
        break;
        case igloo_FILTER_RESULT_ERROR:
        default:
            ret = igloo_ERROR_GENERIC;
        break;
    }
    igloo_thread_mutex_unlock(&(instance->lock));

    igloo_ro_unref(instance);

    return ret;
}
