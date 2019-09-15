/* Copyright (C) 2018       Marvin Scholz <epirat07@gmail.com>
 * Copyright (C) 2012-2019  Philipp "ph3-der-loewe" Schafft <lion@lion.leolix.org>
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
#include <stdlib.h>
#include <string.h>

#include <igloo/ro.h>
#include <igloo/error.h>
#include "private.h"

/* This is not static as it is used by igloo_RO_TYPEDECL_NEW_NOOP() */
int igloo_ro_new__return_zero(igloo_ro_t self, const igloo_ro_type_t *type, va_list ap)
{
    (void)self, (void)type, (void)ap;
    return 0;
}

igloo_RO_PUBLIC_TYPE(igloo_ro_base_t,
        igloo_RO_TYPEDECL_NEW_NOOP()
        );

static inline int check_type(const igloo_ro_type_t *type)
{
    return type->control_length == sizeof(igloo_ro_type_t) && type->control_version == igloo_RO__CONTROL_VERSION &&
           type->type_length >= sizeof(igloo_ro_base_t);
}


static inline int igloo_RO_HAS_TYPE_raw_il(igloo_ro_t object, const igloo_ro_type_t *type)
{
    return !igloo_RO_IS_NULL(object) && igloo_RO_GET_TYPE(object) == type;
}
int             igloo_RO_HAS_TYPE_raw(igloo_ro_t object, const igloo_ro_type_t *type)
{
    return igloo_RO_HAS_TYPE_raw_il(object, type);
}
static inline int igloo_RO_IS_VALID_raw_li(igloo_ro_t object, const igloo_ro_type_t *type)
{
    return igloo_RO_HAS_TYPE_raw_il(object, type) && igloo_RO__GETBASE(object)->refc;
}
int             igloo_RO_IS_VALID_raw(igloo_ro_t object, const igloo_ro_type_t *type)
{
    return igloo_RO_IS_VALID_raw_li(object, type);
}
igloo_ro_t      igloo_RO_TO_TYPE_raw(igloo_ro_t object, const igloo_ro_type_t *type)
{
    return igloo_RO_IS_VALID_raw_li(object, type) ? object : igloo_RO_NULL;
}

igloo_ro_t      igloo_ro_new__raw(const igloo_ro_type_t *type, const char *name, igloo_ro_t associated, igloo_ro_t instance)
{
    igloo_ro_base_t *base;

    if (!check_type(type))
        return igloo_RO_NULL;

    base = calloc(1, type->type_length);
    if (!base)
        return igloo_RO_NULL;

    base->type = type;
    base->refc = 1;

    igloo_thread_mutex_create(&(base->lock));

    if (name) {
        base->name = strdup(name);
        if (!base->name) {
            igloo_ro_unref(base);
            return igloo_RO_NULL;
        }
    }

    if (!igloo_RO_IS_NULL(associated)) {
        if (igloo_ro_ref(associated) != igloo_ERROR_NONE) {
            igloo_ro_unref(base);
            return igloo_RO_NULL;
        }

        base->associated = associated;
    }

    if (!igloo_RO_IS_NULL(instance)) {
        if (!igloo_IS_INSTANCE(instance)) {
            /* In this case we're fine if this returns igloo_RO_NULL. */
            instance = igloo_ro_get_instance(instance);
        } else {
            if (igloo_ro_ref(instance) != igloo_ERROR_NONE) {
                igloo_ro_unref(base);
                return igloo_RO_NULL;
            }
        }

        base->instance = instance;
    }

    return (igloo_ro_t)base;
}

igloo_ro_t      igloo_ro_new__simple(const igloo_ro_type_t *type, const char *name, igloo_ro_t associated, igloo_ro_t instance, ...)
{
    igloo_ro_t ret;
    int res;
    va_list ap;

    if (!check_type(type))
        return igloo_RO_NULL;

    if (!type->type_newcb)
        return igloo_RO_NULL;

    ret = igloo_ro_new__raw(type, name, associated, instance);
    if (igloo_RO_IS_NULL(ret))
        return igloo_RO_NULL;

    va_start(ap, instance);
    res = type->type_newcb(ret, type, ap);
    va_end(ap);

    if (res != 0) {
        igloo_ro_unref(ret);
        return igloo_RO_NULL;
    }

    return ret;
}

igloo_error_t igloo_ro_ref(igloo_ro_t self)
{
    igloo_ro_base_t *base = igloo_RO__GETBASE(self);

    if (!base)
        return igloo_ERROR_FAULT;

    igloo_thread_mutex_lock(&(base->lock));
    if (!base->refc) {
        igloo_thread_mutex_unlock(&(base->lock));
        return igloo_ERROR_GENERIC;
    }
    base->refc++;
    igloo_thread_mutex_unlock(&(base->lock));

    return igloo_ERROR_NONE;
}

static inline void igloo_ro__destory(igloo_ro_base_t *base)
{
    igloo_thread_mutex_unlock(&(base->lock));
    igloo_thread_mutex_destroy(&(base->lock));

    free(base);
}

igloo_error_t igloo_ro_unref(igloo_ro_t self)
{
    igloo_ro_base_t *base = igloo_RO__GETBASE(self);

    if (!base)
        return igloo_ERROR_FAULT;

    igloo_thread_mutex_lock(&(base->lock));

    if (!base->refc) {
        igloo_thread_mutex_unlock(&(base->lock));
        return igloo_ERROR_GENERIC;
    }

    if (base->refc > 1) {
        base->refc--;
        igloo_thread_mutex_unlock(&(base->lock));
        return igloo_ERROR_NONE;
    }

    if (base->type->type_freecb)
        base->type->type_freecb(self);

    base->refc--;

    igloo_ro_unref(base->associated);
    igloo_ro_unref(base->instance);

    if (base->name)
        free(base->name);

    if (base->wrefc) {
        /* only clear the object */
        base->associated = igloo_RO_NULL;
        base->instance = igloo_RO_NULL;
        base->name = NULL;
        igloo_thread_mutex_unlock(&(base->lock));
    } else {
        igloo_ro__destory(base);
    }

    return igloo_ERROR_NONE;
}

igloo_error_t igloo_ro_weak_ref(igloo_ro_t self)
{
    igloo_ro_base_t *base = igloo_RO__GETBASE(self);

    if (!base)
        return igloo_ERROR_FAULT;

    igloo_thread_mutex_lock(&(base->lock));
    base->wrefc++;
    igloo_thread_mutex_unlock(&(base->lock));

    return igloo_ERROR_NONE;
}

igloo_error_t  igloo_ro_weak_unref(igloo_ro_t self)
{
    igloo_ro_base_t *base = igloo_RO__GETBASE(self);

    if (!base)
        return igloo_ERROR_FAULT;

    igloo_thread_mutex_lock(&(base->lock));
    base->wrefc--;

    if (base->refc || base->wrefc) {
        igloo_thread_mutex_unlock(&(base->lock));
        return igloo_ERROR_NONE;
    }

    igloo_ro__destory(base);

    return igloo_ERROR_NONE;
}

const char *    igloo_ro_get_name(igloo_ro_t self)
{
    igloo_ro_base_t *base = igloo_RO__GETBASE(self);
    const char *ret;

    if (!base)
        return NULL;

    igloo_thread_mutex_lock(&(base->lock));
    if (!base->refc) {
        igloo_thread_mutex_unlock(&(base->lock));
        return NULL;
    }
    ret = base->name;
    igloo_thread_mutex_unlock(&(base->lock));

    return ret;
}

igloo_ro_t      igloo_ro_get_associated(igloo_ro_t self)
{
    igloo_ro_base_t *base = igloo_RO__GETBASE(self);
    igloo_ro_t ret;

    if (!base)
        return igloo_RO_NULL;

    igloo_thread_mutex_lock(&(base->lock));
    if (!base->refc) {
        igloo_thread_mutex_unlock(&(base->lock));
        return igloo_RO_NULL;
    }
    ret = base->associated;
    if (!igloo_RO_IS_NULL(ret)) {
        if (igloo_ro_ref(ret) != igloo_ERROR_NONE) {
            igloo_thread_mutex_unlock(&(base->lock));
            return igloo_RO_NULL;
        }
    }
    igloo_thread_mutex_unlock(&(base->lock));

    return ret;
}

igloo_error_t igloo_ro_set_associated(igloo_ro_t self, igloo_ro_t associated)
{
    igloo_ro_base_t *base = igloo_RO__GETBASE(self);
    igloo_ro_t old;
    igloo_error_t ret;

    if (!base)
        return igloo_ERROR_FAULT;

    /* We can not set ourself to be our associated. */
    if (base == igloo_RO__GETBASE(associated))
        return igloo_ERROR_GENERIC;

    if (!igloo_RO_IS_NULL(associated)) {
        if ((ret = igloo_ro_ref(associated)) != igloo_ERROR_NONE) {
            /* Could not get a reference on the new associated object. */
            return ret;
        }
    }

    igloo_thread_mutex_lock(&(base->lock));
    if (!base->refc) {
        igloo_ro_unref(associated);
        igloo_thread_mutex_unlock(&(base->lock));
        return igloo_ERROR_GENERIC;
    }
    old = base->associated;
    base->associated = associated;
    igloo_thread_mutex_unlock(&(base->lock));

    igloo_ro_unref(old);

    return igloo_ERROR_NONE;
}

igloo_ro_t      igloo_ro_get_instance(igloo_ro_t self)
{
    igloo_ro_base_t *base = igloo_RO__GETBASE(self);
    igloo_ro_t ret;

    if (!base)
        return igloo_RO_NULL;

    igloo_thread_mutex_lock(&(base->lock));
    if (!base->refc) {
        igloo_thread_mutex_unlock(&(base->lock));
        return igloo_RO_NULL;
    }

    if (igloo_IS_INSTANCE(base)) {
        ret = (igloo_ro_t)base;
    } else {
        ret = base->instance;
    }

    if (!igloo_RO_IS_NULL(ret)) {
        if (igloo_ro_ref(ret) != igloo_ERROR_NONE) {
            igloo_thread_mutex_unlock(&(base->lock));
            return igloo_RO_NULL;
        }
    }
    igloo_thread_mutex_unlock(&(base->lock));

    return ret;
}

igloo_ro_t      igloo_ro_clone(igloo_ro_t self, igloo_ro_cf_t required, igloo_ro_cf_t allowed, const char *name, igloo_ro_t associated)
{
    igloo_ro_base_t *base = igloo_RO__GETBASE(self);
    igloo_ro_t ret = igloo_RO_NULL;

    if (!base)
        return igloo_RO_NULL;

    allowed |= required;

    if (!allowed)
        allowed = igloo_RO_CF_DEFAULT;

    igloo_thread_mutex_lock(&(base->lock));
    if (!base->refc) {
        igloo_thread_mutex_unlock(&(base->lock));
        return igloo_RO_NULL;
    }

    if (base->type->type_clonecb)
        ret = base->type->type_clonecb(self, required, allowed, name, associated, base->instance);
    igloo_thread_mutex_unlock(&(base->lock));

    return ret;
}

static igloo_ro_t      igloo_ro_convert_ext__no_lock(igloo_ro_t self, const igloo_ro_type_t *type, igloo_ro_cf_t required, igloo_ro_cf_t allowed, const char *name, igloo_ro_t associated)
{
    igloo_ro_base_t *base = igloo_RO__GETBASE(self);
    igloo_ro_t ret = igloo_RO_NULL;

    if (!base || !type)
        return igloo_RO_NULL;

    if (!base->refc) {
        return igloo_RO_NULL;
    }

    if (base->type == type) {
        return igloo_ro_clone(self, required, allowed, name, associated);
    }

    allowed |= required;

    if (!allowed)
        allowed = igloo_RO_CF_DEFAULT;

    if (base->type->type_convertcb)
        ret = base->type->type_convertcb(self, type, required, allowed, name, associated, base->instance);

    if (igloo_RO_IS_NULL(ret))
        if (type->type_convertcb)
            ret = type->type_convertcb(self, type, required, allowed, name, associated, base->instance);

    return ret;
}

igloo_ro_t      igloo_ro_convert_ext(igloo_ro_t self, const igloo_ro_type_t *type, igloo_ro_cf_t required, igloo_ro_cf_t allowed, const char *name, igloo_ro_t associated)
{
    igloo_ro_base_t *base = igloo_RO__GETBASE(self);
    igloo_ro_t ret;

    if (!base || !type)
        return igloo_RO_NULL;

    igloo_thread_mutex_lock(&(base->lock));
	ret = igloo_ro_convert_ext__no_lock(self, type, required, allowed, name, associated);
    igloo_thread_mutex_unlock(&(base->lock));

    return ret;
}

igloo_ro_t      igloo_ro_convert_simple(igloo_ro_t self, const igloo_ro_type_t *type, igloo_ro_cf_t required, igloo_ro_cf_t allowed)
{
    igloo_ro_base_t *base = igloo_RO__GETBASE(self);
    igloo_ro_t ret;

    if (!base || !type)
        return igloo_RO_NULL;

    igloo_thread_mutex_lock(&(base->lock));
	ret = igloo_ro_convert_ext__no_lock(self, type, required, allowed, base->name, base->associated);
    igloo_thread_mutex_unlock(&(base->lock));

    return ret;
}

igloo_ro_t igloo_ro_get_interface_ext(igloo_ro_t self, const igloo_ro_type_t *type, const char *name, igloo_ro_t associated)
{
    igloo_ro_base_t *base = igloo_RO__GETBASE(self);
    igloo_ro_t ret = igloo_RO_NULL;

    if (!base || !type)
        return igloo_RO_NULL;

    igloo_thread_mutex_lock(&(base->lock));
    if (!base->refc) {
        igloo_thread_mutex_unlock(&(base->lock));
        return igloo_RO_NULL;
    }
    /* create a temp reference
     * This is required so we can run type_get_interfacecb() in unlocked state.
     */
    base->refc++;
    igloo_thread_mutex_unlock(&(base->lock));

    if (base->type->type_get_interfacecb)
        ret = base->type->type_get_interfacecb(self, type, name, associated, base->instance);

    igloo_thread_mutex_lock(&(base->lock));
    /* remove temp reference
     * If refc == 0 the application has a bug.
     * TODO: We must tell someone about it.
     * Anyway, we can not just set refc = -1, that will just break things more.
     */
    if (base->refc)
        base->refc--;
    igloo_thread_mutex_unlock(&(base->lock));

    return ret;
}

static size_t igloo_ro_stringify__calc_object_strlen_short(igloo_ro_t self)
{
    igloo_ro_base_t *base = igloo_RO__GETBASE(self);

    if (!base)
        return 2;

    return strlen(base->type->type_name) + 2+(64/4) /* pointer rendering */ + 4 /* "{@}\0" */;
}

char *          igloo_ro_stringify(igloo_ro_t self, igloo_ro_sy_t flags)
{
    igloo_ro_base_t *base = igloo_RO__GETBASE(self);
    char *ret = NULL;

    if (flags & igloo_RO_SY_DEFAULT)
        flags |= igloo_RO_SY_OBJECT|igloo_RO_SY_CONTENT;


    if (!base) {
        if (flags & igloo_RO_SY_OBJECT) {
            return strdup("{igloo_RO_NULL}");
        } else {
            return NULL;
        }
    }

    igloo_thread_mutex_lock(&(base->lock));
    if (!base->refc) {
        if (flags & igloo_RO_SY_OBJECT) {
            int len;
            char buf;

#define STRINGIFY_FORMAT_WEAK "{%s@%p, weak}", base->type->type_name, base
            len = snprintf(&buf, 1, STRINGIFY_FORMAT_WEAK);
            if (len > 2) {
                /* We add 2 bytes just to make sure no buggy interpretation of \0 inclusion could bite us. */
                ret = calloc(1, len + 2);
                if (ret) {
                    snprintf(ret, len + 1, STRINGIFY_FORMAT_WEAK);
                }
            }
        }

        igloo_thread_mutex_unlock(&(base->lock));
        return ret;
    }

    if (base->type->type_stringifycb && (flags & igloo_RO_SY_CONTENT)) {
        ret = base->type->type_stringifycb(self, flags);
    } else {
        if (flags & igloo_RO_SY_OBJECT) {
            static const char *format = "{%s@%p, strong, name=%# H, associated=%#P, instance=%#P}";
            size_t len = strlen(base->type->type_name) + 1*(2+64/4) + strlen(format) + igloo_private__vsnprintf_Hstrlen(base->name, 1, 1) + 1 +
                igloo_ro_stringify__calc_object_strlen_short(base->associated) +
                igloo_ro_stringify__calc_object_strlen_short(base->instance);

            ret = calloc(1, len);
            if (ret) {
                igloo_private__snprintf(ret, len + 1, format, base->type->type_name, base, base->name, base->associated, base->instance);
            }
        }
    }
    igloo_thread_mutex_unlock(&(base->lock));

    return ret;
}

igloo_ro_cr_t   igloo_ro_compare(igloo_ro_t a, igloo_ro_t b)
{
    igloo_ro_base_t *base_a = igloo_RO__GETBASE(a);
    igloo_ro_base_t *base_b = igloo_RO__GETBASE(b);
    igloo_ro_cr_t ret = igloo_RO_CR__ERROR;

    if (base_a == base_b)
        return igloo_RO_CR_SAME;

    if (!base_a || !base_b)
        return igloo_RO_CR__ERROR;

    igloo_thread_mutex_lock(&(base_a->lock));
    igloo_thread_mutex_lock(&(base_b->lock));

    if (!base_a->refc || !base_b->refc) {
        igloo_thread_mutex_unlock(&(base_b->lock));
        igloo_thread_mutex_unlock(&(base_a->lock));
        return igloo_RO_CR__ERROR;
    }

    if (base_a->type->type_comparecb)
        ret = base_a->type->type_comparecb(a, b);

    if (ret == igloo_RO_CR__ERROR) {
        ret = base_b->type->type_comparecb(b, a);

        /* we switched arguments here, so we need to reverse the result */
        switch (ret) {
            case igloo_RO_CR__ERROR:
            case igloo_RO_CR_EQUAL:
            case igloo_RO_CR_NOTEQUAL:
                /* those are the same in both directions */
            break;
            case igloo_RO_CR_ALESSTHANB:
                ret = igloo_RO_CR_AGREATERTHANB;
            break;
            case igloo_RO_CR_AGREATERTHANB:
                ret = igloo_RO_CR_ALESSTHANB;
            break;
            default:
                /* Cases we do not yet handle or that are not a valid result. */
                ret = igloo_RO_CR__ERROR;
            break;
        }
    }

    igloo_thread_mutex_unlock(&(base_b->lock));
    igloo_thread_mutex_unlock(&(base_a->lock));

    return ret;
}

igloo_error_t igloo_ro_get_error(igloo_ro_t self, igloo_error_t *result)
{
    igloo_ro_base_t *base = igloo_RO__GETBASE(self);
    igloo_error_t ret = igloo_ERROR_GENERIC;
    igloo_error_t res = igloo_ERROR_GENERIC;

    if (!base || !result)
        return igloo_ERROR_GENERIC;

    igloo_thread_mutex_lock(&(base->lock));
    if (base->type->type_get_errorcb)
        ret = base->type->type_get_errorcb(self, &res);
    igloo_thread_mutex_unlock(&(base->lock));

    if (ret == igloo_ERROR_NONE)
        *result = res;

    return ret;
}
