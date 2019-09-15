/* Copyright (C) 2018       Marvin Scholz <epirat07@gmail.com>
 * Copyright (C) 2018       Philipp "ph3-der-loewe" Schafft <lion@lion.leolix.org>
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

#ifndef _LIBIGLOO__IGLOO_H_
#define _LIBIGLOO__IGLOO_H_
/**
 * @file
 * Put a good description of this file here
 */

#ifdef __cplusplus
extern "C" {
#endif

/* Put stuff here */

#include "ro.h"

/*
 * This initializes libigloo. This MUST BE called before any
 * other functions can be called.
 *
 * Returns a refobject on success or igloo_RO_NULL on failure.
 * This can be called multiple times (e.g. by the application
 * and by the libraries it uses independently).
 *
 * The library is deinitialized when the last reference
 * to a returned object is gone. This happens by
 * calling igloo_ro_unref() on the last reference.
 *
 * All igloo_ro_*() functions can be used on this object.
 */
igloo_ro_t     igloo_initialize(void);

/* Get per-instance error value
 *
 * Parameters:
 *  self
 *      An instance or any object that knows an instance.
 *  result
 *      Pointer to where to store the result.
 * Returns:
 *  igloo_ERROR_NONE if successful or error code otherwise.
 */
igloo_error_t igloo_instance_get_error(igloo_ro_t self, igloo_error_t *result);
/* Set per-instance error value
 *
 * Parameters:
 *  self
 *      An instance or any object that knows an instance.
 *  error
 *      The new error value.
 * Returns:
 *  igloo_ERROR_NONE if successful or error code otherwise.
 */
igloo_error_t igloo_instance_set_error(igloo_ro_t self, igloo_error_t error);

/* Set per-instance logger
 *
 * Note: The instance will only hold a weak reference. So the caller must
 * ensure that there will be a strong reference somewhere else. Otherwise
 * the logger will be released.
 *
 * Parameters:
 *  self
 *      An instance or any object that knows an instance.
 *  logger
 *      The logger to use.
 * Returns:
 *  igloo_ERROR_NONE if successful or error code otherwise.
 */
igloo_error_t igloo_instance_set_logger(igloo_ro_t self, igloo_objecthandler_t *logger);

/* Get per-instance logger
 *
 * Parameters:
 *  self
 *      An instance or any object that knows an instance.
 * Returns:
 *  A strong reference to a logger or igloo_RO_NULL.
 */
igloo_objecthandler_t * igloo_instance_get_logger(igloo_ro_t self);

/* Log a message with per-instance logger
 *
 * Note: This is much faster than using igloo_instance_get_logger() and
 * using it's return value for single log messages. If you want to push many
 * messages use igloo_instance_get_logger() and igloo_list_forward().
 *
 * Parameters:
 *  self
 *      An instance or any object that knows an instance.
 *  msg
 *      The message to log.
 * Returns:
 *  igloo_ERROR_NONE if successful or error code otherwise.
 */
igloo_error_t igloo_instance_log(igloo_ro_t self, igloo_ro_t msg);

#ifdef __cplusplus
}
#endif

#endif /* ! _LIBIGLOO__IGLOO_H_ */
