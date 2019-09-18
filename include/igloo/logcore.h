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

/* Routing classes allow controling the order in which messages should be routed to outputs */
typedef enum {
    /* All messages are routed to outputs of the ANY class.
     * However if one output accepts the message no additional ANY class outputs are tried.
     */
    igloo_LOGCORE_CLASS_ANY,
    /* In addition to all other classes all messages are routed to all outputs of the ALL class. */
    igloo_LOGCORE_CLASS_ALL,
    /* The DEFAULT class is part of the ANY class.
     * The only difference is that outputs of the DEFAULT class are always tried last.
     */
    igloo_LOGCORE_CLASS_DEFAULT
} igloo_logcore_routingclass_t;

/* Configuration structure of a single output */
typedef struct {
    /* Optional: An ID for the output. This can be used to later on reference the output */
    char *id;
    /* Optional: A filename for the output. If not NULL the file is opened and the formater is attached to it */
    char *filename;
    /* Required: Routing class for the output. */
    igloo_logcore_routingclass_t routingclass;
    /* Optional: Limit for the recent buffer of the output or -1 to use the default. */
    ssize_t recent_limit;
    /* Optional: A filter that the messages that are routed to this output must pass. */
    igloo_filter_t *filter;
    /* Required: The formater to pass the message to. */
    igloo_objecthandler_t *formater;
} igloo_logcore_output_t;

/* Type uses for callback called when a message is acknowledged.
 *
 * This callback can be used to generate a new message or trigger any action
 * when another one is acknowledged.
 *
 * Parameters:
 *  core
 *      The core that emited the event.
 *  object
 *      The message that was acknowledged.
 *  userdata
 *      A pointer that was given on igloo_logcore_set_acknowledge_cb()
 * Returns:
 *  A new message that will be pushed into the core or igloo_RO_NULL.
 *  A possible igloo_LOGMSG_OPT_ASKACK flag of the message will be ignored to avoid recursion.
 */
typedef igloo_ro_t (*igloo_logcore_acknowledge_t)(igloo_logcore_t *core, igloo_ro_t object, void *userdata);


/* Configure the core.
 *
 * Parameters:
 *  core
 *      The core to configure.
 *  recent_limit
 *      The limit for the recent message buffer or -1 for default.
 *  askack_limit
 *      The limit for the askack messages buffer or -1 for default.
 *  output_recent_limit
 *      The default limit for the per output recent message buffer or -1 for default.
 *  outputs
 *      The outputs to use.
 *  outputs_len
 *      The number of outputs to use.
 */
igloo_error_t   igloo_logcore_configure(igloo_logcore_t *core, ssize_t recent_limit, ssize_t askack_limit, ssize_t output_recent_limit, const igloo_logcore_output_t *outputs, size_t outputs_len);


/* Set acknowledg callback.
 *
 * See also igloo_logcore_acknowledge_t.
 *
 * Parameters:
 *  core
 *      The core to configure.
 *  callback
 *      The callback to use.
 *  userdata
 *      The userdata pointer to pass to the callback.
 */
igloo_error_t   igloo_logcore_set_acknowledge_cb(igloo_logcore_t *core, igloo_logcore_acknowledge_t callback, void *userdata);


/* Get a list of messages from the recent buffer.
 *
 * Parameters:
 *  core
 *      The core to access.
 *  type
 *      Filter messages to messages of this type or NULL to not filter.
 *  filter
 *      Filter messages using this filter or NULL to not filter.
 *  limit
 *      Limit messages to this amount or -1 to not limit.
 *  id
 *      Return messages from the output with this ID or NULL to return from the global buffer.
 * Returns:
 *  Prefiltered list of messages.
 */
igloo_list_t *  igloo_logcore_get_recent(igloo_logcore_t *core, const igloo_ro_type_t *type, igloo_filter_t *filter, ssize_t limit, const char *id);


/* Get a list of messages from the askack buffer.
 *
 * Parameters:
 *  core
 *      The core to access.
 *  type
 *      Filter messages to messages of this type or NULL to not filter.
 *  filter
 *      Filter messages using this filter or NULL to not filter.
 *  limit
 *      Limit messages to this amount or -1 to not limit.
 * Returns:
 *  Prefiltered list of messages.
 */
igloo_list_t *  igloo_logcore_get_askack(igloo_logcore_t *core, const igloo_ro_type_t *type, igloo_filter_t *filter, ssize_t limit);


/* Acknowledg a message in the askack buffer.
 *
 * Parameters:
 *  core
 *      The core to access.
 *  object
 *      The message to acknowledg.
 */
igloo_error_t   igloo_logcore_acknowledge(igloo_logcore_t *core, igloo_ro_t object);

#ifdef __cplusplus
}
#endif

#endif /* ! _LIBIGLOO__LOGCORE_H_ */
