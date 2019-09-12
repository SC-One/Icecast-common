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
#include <igloo/logcore.h>
#include <igloo/list.h>
#include <igloo/filter.h>
#include <igloo/objecthandler.h>
#include <igloo/logmsg.h>
#include <igloo/stdio.h>
#include "private.h"

struct igloo_logcore_tag {
    igloo_ro_base_t __base;

    igloo_rwlock_t rwlock;

    igloo_list_t *global_recent;
    igloo_list_t *global_askack;
    size_t output_length;
    igloo_logcore_output_t *output;
    igloo_list_t **output_recent;
    igloo_logcore_acknowledge_t acknowledge_cb;
    void *acknowledge_cb_userdata;
};

static int __new(igloo_ro_t self, const igloo_ro_type_t *type, va_list ap);
static void __free(igloo_ro_t self);
static igloo_ro_t __get_interface_t(igloo_ro_t self, const igloo_ro_type_t *type, const char *name, igloo_ro_t associated, igloo_ro_t instance);

igloo_RO_PUBLIC_TYPE(igloo_logcore_t,
        igloo_RO_TYPEDECL_FREE(__free),
        igloo_RO_TYPEDECL_NEW(__new),
        igloo_RO_TYPEDECL_GET_INTERFACE(__get_interface_t)
        );

static int __copy_output(igloo_logcore_output_t *dst, const igloo_logcore_output_t *src)
{
    memset(dst, 0, sizeof(*dst));
    if (src->id)
        dst->id = strdup(src->id);
    if (src->filename)
        dst->filename = strdup(src->filename);
    dst->recent_limit = src->recent_limit;
    igloo_ro_ref(dst->filter = src->filter);
    igloo_ro_ref(dst->formater = src->formater);

    return 0;
}

static void __free_output(igloo_logcore_output_t *output)
{
    free(output->id);
    free(output->filename);
    igloo_ro_unref(output->filter);
    igloo_objecthandler_flush(output->formater);
    igloo_ro_unref(output->formater);
    memset(output, 0, sizeof(*output));
}

static void __free_locked(igloo_logcore_t *core)
{
    size_t i;

    igloo_ro_unref(core->global_recent);
    core->global_recent = NULL;
    igloo_ro_unref(core->global_askack);
    core->global_askack = NULL;

    for (i = 0; i < core->output_length; i++) {
        igloo_ro_unref(core->output_recent[i]);
        __free_output(&(core->output[i]));
    }

    free(core->output);
    core->output = NULL;
    free(core->output_recent);
    core->output_recent = NULL;
}

static int __new(igloo_ro_t self, const igloo_ro_type_t *type, va_list ap)
{
    igloo_logcore_t *core = igloo_RO_TO_TYPE(self, igloo_logcore_t);

    igloo_thread_rwlock_create(&(core->rwlock));

    return 0;
}

static void __free(igloo_ro_t self)
{
    igloo_logcore_t *core = igloo_RO_TO_TYPE(self, igloo_logcore_t);
    igloo_thread_rwlock_wlock(&(core->rwlock));
    __free_locked(core);
    igloo_thread_rwlock_unlock(&(core->rwlock));
    igloo_thread_rwlock_destroy(&(core->rwlock));
}

static igloo_filter_result_t __push_msg(igloo_logcore_t *core, igloo_ro_t msg, int allow_askack)
{
    igloo_filter_result_t ret = igloo_FILTER_RESULT_DROP;
    igloo_logmsg_t *logmsg;
    size_t i;

    for (i = 0; i < core->output_length; i++) {
        igloo_filter_result_t push = igloo_FILTER_RESULT_PASS;

        if (core->output[i].filter)
            push = igloo_filter_test(core->output[i].filter, msg);

        if (push == igloo_FILTER_RESULT_PASS)
            push = igloo_objecthandler_handle(core->output[i].formater, msg);

        igloo_list_push(core->output_recent[i], msg);

        if (push == igloo_FILTER_RESULT_PASS)
            ret = push;
    }

    if (ret == igloo_FILTER_RESULT_PASS)
        igloo_list_push(core->global_recent, msg);

    logmsg = igloo_RO_TO_TYPE(msg, igloo_logmsg_t);
    if (logmsg) {
        igloo_logmsg_opt_t opts = igloo_LOGMSG_OPT_NONE;

        if (igloo_logmsg_get_extra(logmsg, &opts, NULL) == 0) {
            if (opts & igloo_LOGMSG_OPT_ASKACK)
                igloo_list_push(core->global_askack, msg);
        }
    }

    return ret;
}

int             igloo_logcore_configure(igloo_logcore_t *core, ssize_t recent_limit, ssize_t askack_limit, ssize_t output_recent_limit, const igloo_logcore_output_t *outputs, size_t outputs_len)
{
    igloo_list_t *global_recent;
    igloo_list_t *global_askack;
    igloo_logcore_output_t *output;
    igloo_list_t **output_recent;
    igloo_ro_t instance;
    size_t i;

    if (!igloo_RO_IS_VALID(core, igloo_logcore_t))
        return -1;

    if (outputs_len < 1 && !outputs)
        return -1;

    if (recent_limit < 0)
        recent_limit = 128;

    if (askack_limit < 0)
        askack_limit = 128;

    if (output_recent_limit < 0)
        output_recent_limit = 128;

    instance = igloo_ro_get_instance(core);

    global_recent = igloo_ro_new_ext(igloo_list_t, NULL, igloo_RO_NULL, core);
    igloo_list_set_policy(global_recent, igloo_LIST_POLICY_FIXED_PIPE, recent_limit);

    global_askack = igloo_ro_new_ext(igloo_list_t, NULL, igloo_RO_NULL, core);
    igloo_list_set_policy(global_askack, igloo_LIST_POLICY_FIXED_PIPE, askack_limit);

    output = calloc(outputs_len, sizeof(*output));
    output_recent = calloc(outputs_len, sizeof(*output_recent));

    for (i = 0; i < outputs_len; i++) {
        igloo_io_t *io;

        __copy_output(&(output[i]), &(outputs[i]));

        if (output[i].recent_limit < 0)
            output[i].recent_limit = output_recent_limit;

        if (output[i].filename) {
            io = igloo_stdio_new_file(output[i].filename, "ab", NULL, igloo_RO_NULL, instance);
            if (io) {
                igloo_objecthandler_set_backend(output[i].formater, io);
                igloo_ro_unref(io);
            }
        }

        output_recent[i] = igloo_ro_new_ext(igloo_list_t, NULL, igloo_RO_NULL, core);
        igloo_list_set_policy(output_recent[i], igloo_LIST_POLICY_FIXED_PIPE, output[i].recent_limit);
    }

    igloo_thread_rwlock_wlock(&(core->rwlock));
    __free_locked(core);

    core->global_recent = global_recent;
    core->global_askack = global_askack;
    core->output_length = outputs_len;
    core->output = output;
    core->output_recent = output_recent;
    igloo_thread_rwlock_unlock(&(core->rwlock));

    igloo_ro_unref(instance);

    return 0;
}

int             igloo_logcore_set_acknowledge_cb(igloo_logcore_t *core, igloo_logcore_acknowledge_t callback, void *userdata)
{
    if (!igloo_RO_IS_VALID(core, igloo_logcore_t))
        return -1;

    igloo_thread_rwlock_wlock(&(core->rwlock));
    core->acknowledge_cb = callback;
    core->acknowledge_cb_userdata = userdata;
    igloo_thread_rwlock_unlock(&(core->rwlock));

    return 0;
}

static igloo_list_t *__copy_filtered_list(igloo_list_t *list, const igloo_ro_type_t *type, igloo_filter_t *filter, ssize_t limit)
{
    igloo_list_iterator_storage_t storage;
    igloo_list_iterator_t *iterator;
    igloo_list_t *ret;
  
    if (!list)
        return NULL;

    iterator = igloo_list_iterator_start(list, &storage, sizeof(storage));
    ret = igloo_ro_new_ext(igloo_list_t, NULL, igloo_RO_NULL, list);

    if (iterator && ret) {
        while (limit) {
            igloo_ro_t element = igloo_list_iterator_next(iterator);

            if (igloo_RO_IS_NULL(element))
                break;

            if (type == NULL || igloo_RO_GET_TYPE(element) == type) {
                if (filter == NULL || igloo_filter_test(filter, element) == igloo_FILTER_RESULT_PASS) {
                    igloo_list_push(ret, element);
                }
            }

            igloo_ro_unref(element);

            if (limit > 0)
                limit--;
        }
    }

    igloo_list_iterator_end(iterator);

    return ret;
}

igloo_list_t *  igloo_logcore_get_recent(igloo_logcore_t *core, const igloo_ro_type_t *type, igloo_filter_t *filter, ssize_t limit, const char *id)
{
    igloo_list_t *list = NULL;
    igloo_list_t *ret = NULL;

    if (!igloo_RO_IS_VALID(core, igloo_logcore_t))
        return NULL;

    igloo_thread_rwlock_rlock(&(core->rwlock));
    if (id) {
        size_t i;

        for (i = 0; i < core->output_length; i++) {
            if (core->output[i].id && strcmp(core->output[i].id, id) == 0) {
                list = core->output_recent[i];
            }
        }
    } else {
        list = core->global_recent;
    }

    ret = __copy_filtered_list(list, type, filter, limit);
    igloo_thread_rwlock_unlock(&(core->rwlock));

    return ret;
}

igloo_list_t *  igloo_logcore_get_askack(igloo_logcore_t *core, const igloo_ro_type_t *type, igloo_filter_t *filter, ssize_t limit)
{
    igloo_list_t *ret = NULL;

    if (!igloo_RO_IS_VALID(core, igloo_logcore_t))
        return NULL;

    igloo_thread_rwlock_rlock(&(core->rwlock));
    ret = __copy_filtered_list(core->global_askack, type, filter, limit);
    igloo_thread_rwlock_unlock(&(core->rwlock));

    return ret;
}

int             igloo_logcore_acknowledge(igloo_logcore_t *core, igloo_ro_t object)
{
    int ret;

    if (!igloo_RO_IS_VALID(core, igloo_logcore_t))
        return -1;

    igloo_thread_rwlock_wlock(&(core->rwlock));
    ret = igloo_list_remove(core->global_askack, object);

    if (ret == 0 && core->acknowledge_cb) {
        igloo_ro_t msg = core->acknowledge_cb(core, object, core->acknowledge_cb_userdata);

        if (!igloo_RO_IS_NULL(msg)) {
            __push_msg(core, msg, 0);
        }
    }

    igloo_thread_rwlock_unlock(&(core->rwlock));
    return ret;
}

static igloo_filter_result_t __handle(igloo_INTERFACE_BASIC_ARGS, igloo_ro_t object)
{
    igloo_logcore_t *core = igloo_RO_TO_TYPE(*backend_object, igloo_logcore_t);
    igloo_filter_result_t ret;

    if (!core)
        return igloo_FILTER_RESULT_ERROR;

    igloo_thread_rwlock_wlock(&(core->rwlock));
    ret = __push_msg(core, object, 1);
    igloo_thread_rwlock_unlock(&(core->rwlock));

    return ret;
}

static int __flush(igloo_INTERFACE_BASIC_ARGS)
{
    igloo_logcore_t *core = igloo_RO_TO_TYPE(*backend_object, igloo_logcore_t);
    size_t i;

    if (!core)
        return -1;

    igloo_thread_rwlock_rlock(&(core->rwlock));
    for (i = 0; i < core->output_length; i++) {
        igloo_objecthandler_flush(core->output[i].formater);
    }
    igloo_thread_rwlock_unlock(&(core->rwlock));

    return 0;
}

static const igloo_objecthandler_ifdesc_t igloo_logcore__igloo_objecthandler_ifdesc = {
    igloo_INTERFACE_DESCRIPTION_BASE(igloo_objecthandler_ifdesc_t),
    .is_thread_safe = 1,
    .handle = __handle,
    .flush = __flush,
    .set_backend = NULL /* No, this is not possible */
};

static igloo_ro_t __get_interface_t(igloo_ro_t self, const igloo_ro_type_t *type, const char *name, igloo_ro_t associated, igloo_ro_t instance)
{
    igloo_logcore_t *core = igloo_RO_TO_TYPE(self, igloo_logcore_t);

    if (!core)
        return igloo_RO_NULL;

    if (type != igloo_RO_GET_TYPE_BY_SYMBOL(igloo_objecthandler_t))
        return igloo_RO_NULL;

    return (igloo_ro_t)igloo_objecthandler_new(&igloo_logcore__igloo_objecthandler_ifdesc, core, NULL, name, associated, instance);
}
