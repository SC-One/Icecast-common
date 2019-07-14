/* 
** Logging framework.
**
** Copyright (C) 2014       Michael Smith <msmith@icecast.org>,
**                          Ralph Giles <giles@xiph.org>,
**                          Ed "oddsock" Zaleski <oddsock@xiph.org>,
**                          Karl Heyes <karl@xiph.org>,
**                          Jack Moffitt <jack@icecast.org>,
**                          Thomas Ruecker <thomas@ruecker.fi>,
** Copyright (C) 2012-2018  Philipp "ph3-der-loewe" Schafft <lion@lion.leolix.org>
**
** This library is free software; you can redistribute it and/or
** modify it under the terms of the GNU Library General Public
** License as published by the Free Software Foundation; either
** version 2 of the License, or (at your option) any later version.
**
** This library is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** Library General Public License for more details.
**
** You should have received a copy of the GNU Library General Public
** License along with this library; if not, write to the
** Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
** Boston, MA  02110-1301, USA.
**
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif


#ifndef _WIN32
#include <pthread.h>
#else
#include <windows.h>
#endif

#include <igloo/log.h>

#include "../src/private.h"

#define LOG_MAXLOGS 25
#define LOG_MAXLINELEN 1024

#ifdef _WIN32
#define igloo_mutex_t CRITICAL_SECTION
#define snprintf _snprintf
#define vsnprintf _vsnprintf
#else
#define igloo_mutex_t pthread_mutex_t
#endif

static igloo_mutex_t igloo__logger_mutex;
static int igloo__initialized = 0;

typedef struct _log_entry_t
{
   char *line;
   unsigned int len;
   struct _log_entry_t *next;
} log_entry_t;


typedef struct log_tag
{
    int in_use;

    unsigned level;

    char *filename;
    FILE *logfile;
    off_t size;
    off_t trigger_level;
    int archive_timestamp;

    unsigned long total;
    unsigned int entries;
    unsigned int keep_entries;
    log_entry_t *log_head;
    log_entry_t **log_tail;
    
    char *buffer;
} log_t;

static log_t igloo_loglist[LOG_MAXLOGS];

static int _get_log_id(void);
static void _lock_logger(void);
static void _unlock_logger(void);


static int _log_open (int id)
{
    if (igloo_loglist [id] . in_use == 0)
        return 0;

    /* check for cases where an open of the logfile is wanted */
    if (igloo_loglist [id] . logfile == NULL || 
       (igloo_loglist [id] . trigger_level && igloo_loglist [id] . size > igloo_loglist [id] . trigger_level))
    {
        if (igloo_loglist [id] . filename)  /* only re-open files where we have a name */
        {
            struct stat st;

            if (igloo_loglist [id] . logfile)
            {
                char new_name [4096];
                fclose (igloo_loglist [id] . logfile);
                igloo_loglist [id] . logfile = NULL;
                /* simple rename, but could use time providing locking were used */
                if (igloo_loglist[id].archive_timestamp)
                {
                    char timestamp [128];
                    time_t now = time(NULL);

                    strftime (timestamp, sizeof (timestamp), "%Y%m%d_%H%M%S", localtime (&now));
                    snprintf (new_name,  sizeof(new_name), "%s.%s", igloo_loglist[id].filename, timestamp);
                }
                else {
                    snprintf (new_name,  sizeof(new_name), "%s.old", igloo_loglist [id] . filename);
                }
#ifdef _WIN32
                if (stat (new_name, &st) == 0)
                    remove (new_name);
#endif
                rename (igloo_loglist [id] . filename, new_name);
            }
            igloo_loglist [id] . logfile = fopen (igloo_loglist [id] . filename, "a");
            if (igloo_loglist [id] . logfile == NULL)
                return 0;
            setvbuf (igloo_loglist [id] . logfile, NULL, IO_BUFFER_TYPE, 0);
            if (stat (igloo_loglist [id] . filename, &st) < 0)
                igloo_loglist [id] . size = 0;
            else
                igloo_loglist [id] . size = st.st_size;
        }
        else
            igloo_loglist [id] . size = 0;
    }
    return 1;
}

void igloo_log_initialize(void)
{
    int i;

    if (igloo__initialized) return;

    for (i = 0; i < LOG_MAXLOGS; i++) {
        igloo_loglist[i].in_use = 0;
        igloo_loglist[i].level = 2;
        igloo_loglist[i].size = 0;
        igloo_loglist[i].trigger_level = 1000000000;
        igloo_loglist[i].filename = NULL;
        igloo_loglist[i].logfile = NULL;
        igloo_loglist[i].buffer = NULL;
        igloo_loglist[i].total = 0;
        igloo_loglist[i].entries = 0;
        igloo_loglist[i].keep_entries = 0;
        igloo_loglist[i].log_head = NULL;
        igloo_loglist[i].log_tail = &igloo_loglist[i].log_head;
    }

    /* initialize mutexes */
#ifndef _WIN32
    pthread_mutex_init(&igloo__logger_mutex, NULL);
#else
    InitializeCriticalSection(&igloo__logger_mutex);
#endif

    igloo__initialized = 1;
}

int igloo_log_open_file(FILE *file)
{
    int log_id;

    if(file == NULL) return igloo_LOG_EINSANE;

    log_id = _get_log_id();
    if (log_id < 0) return igloo_LOG_ENOMORELOGS;

    igloo_loglist[log_id].logfile = file;
    igloo_loglist[log_id].filename = NULL;
    igloo_loglist[log_id].size = 0;

    return log_id;
}


int igloo_log_open(const char *filename)
{
    int id;
    FILE *file;

    if (filename == NULL) return igloo_LOG_EINSANE;
    if (strcmp(filename, "") == 0) return igloo_LOG_EINSANE;
    
    file = fopen(filename, "a");

    id = igloo_log_open_file(file);

    if (id >= 0)
    {
        struct stat st;

        setvbuf (igloo_loglist [id] . logfile, NULL, IO_BUFFER_TYPE, 0);
        igloo_loglist [id] . filename = strdup (filename);
        if (stat (igloo_loglist [id] . filename, &st) == 0)
            igloo_loglist [id] . size = st.st_size;
        igloo_loglist [id] . entries = 0;
        igloo_loglist [id] . log_head = NULL;
        igloo_loglist [id] . log_tail = &igloo_loglist [id] . log_head;
    }

    return id;
}


/* set the trigger level to trigger, represented in kilobytes */
void igloo_log_set_trigger(int id, unsigned trigger)
{
    if (id >= 0 && id < LOG_MAXLOGS && igloo_loglist [id] . in_use)
    {
         igloo_loglist [id] . trigger_level = trigger*1024;
    }
}


int igloo_log_set_filename(int id, const char *filename)
{
    if (id < 0 || id >= LOG_MAXLOGS)
        return igloo_LOG_EINSANE;
    /* NULL filename is ok, empty filename is not. */
    if ((filename && !strcmp(filename, "")) || igloo_loglist [id] . in_use == 0)
        return igloo_LOG_EINSANE;
     _lock_logger();
    if (igloo_loglist [id] . filename)
        free (igloo_loglist [id] . filename);
    if (filename)
        igloo_loglist [id] . filename = strdup (filename);
    else
        igloo_loglist [id] . filename = NULL;
     _unlock_logger();
    return id;
}

int igloo_log_set_archive_timestamp(int id, int value)
{
    if (id < 0 || id >= LOG_MAXLOGS)
        return igloo_LOG_EINSANE;
     _lock_logger();
     igloo_loglist[id].archive_timestamp = value;
     _unlock_logger();
    return id;
}


int igloo_log_open_with_buffer(const char *filename, int size)
{
    /* not implemented */
    return igloo_LOG_ENOTIMPL;
}


void igloo_log_set_lines_kept (int log_id, unsigned int count)
{
    if (log_id < 0 || log_id >= LOG_MAXLOGS) return;
    if (igloo_loglist[log_id].in_use == 0) return;

    _lock_logger ();
    igloo_loglist[log_id].keep_entries = count;
    while (igloo_loglist[log_id].entries > count)
    {
        log_entry_t *to_go = igloo_loglist [log_id].log_head;
        igloo_loglist [log_id].log_head = to_go->next;
        igloo_loglist [log_id].total -= to_go->len;
        free (to_go->line);
        free (to_go);
        igloo_loglist [log_id].entries--;
    }
    _unlock_logger ();
}


void igloo_log_set_level(int log_id, unsigned level)
{
    if (log_id < 0 || log_id >= LOG_MAXLOGS) return;
    if (igloo_loglist[log_id].in_use == 0) return;

    igloo_loglist[log_id].level = level;
}

void igloo_log_flush(int log_id)
{
    if (log_id < 0 || log_id >= LOG_MAXLOGS) return;
    if (igloo_loglist[log_id].in_use == 0) return;

    _lock_logger();
    if (igloo_loglist[log_id].logfile)
        fflush(igloo_loglist[log_id].logfile);
    _unlock_logger();
}

void igloo_log_reopen(int log_id)
{
    if (log_id < 0 || log_id >= LOG_MAXLOGS)
        return;
    if (igloo_loglist [log_id] . filename && igloo_loglist [log_id] . logfile)
    {
        _lock_logger();

        fclose (igloo_loglist [log_id] . logfile);
        igloo_loglist [log_id] . logfile = NULL;

        _unlock_logger();
    }
}

void igloo_log_close(int log_id)
{
    if (log_id < 0 || log_id >= LOG_MAXLOGS) return;

    _lock_logger();

    if (igloo_loglist[log_id].in_use == 0)
    {
        _unlock_logger();
        return;
    }

    igloo_loglist[log_id].in_use = 0;
    igloo_loglist[log_id].level = 2;
    if (igloo_loglist[log_id].filename) free(igloo_loglist[log_id].filename);
    if (igloo_loglist[log_id].buffer) free(igloo_loglist[log_id].buffer);

    if (igloo_loglist [log_id] . logfile)
    {
        fclose (igloo_loglist [log_id] . logfile);
        igloo_loglist [log_id] . logfile = NULL;
    }
    while (igloo_loglist[log_id].entries)
    {
        log_entry_t *to_go = igloo_loglist [log_id].log_head;
        igloo_loglist [log_id].log_head = to_go->next;
        igloo_loglist [log_id].total -= to_go->len;
        free (to_go->line);
        free (to_go);
        igloo_loglist [log_id].entries--;
    }
    _unlock_logger();
}

void igloo_log_shutdown(void)
{
    /* destroy mutexes */
#ifndef _WIN32
    pthread_mutex_destroy(&igloo__logger_mutex);
#else
    DeleteCriticalSection(&igloo__logger_mutex);
#endif 

    igloo__initialized = 0;
}


static int igloo_create_log_entry (int log_id, const char *pre, const char *line)
{
    log_entry_t *entry;

    if (igloo_loglist[log_id].keep_entries == 0)
        return fprintf (igloo_loglist[log_id].logfile, "%s%s\n", pre, line); 
    
    entry = calloc (1, sizeof (log_entry_t));
    entry->len = strlen (pre) + strlen (line) + 2;
    entry->line = malloc (entry->len);
    snprintf (entry->line, entry->len, "%s%s\n", pre, line);
    igloo_loglist [log_id].total += entry->len;
    fprintf (igloo_loglist[log_id].logfile, "%s", entry->line);

    *igloo_loglist [log_id].log_tail = entry;
    igloo_loglist [log_id].log_tail = &entry->next;

    if (igloo_loglist [log_id].entries >= igloo_loglist [log_id].keep_entries)
    {
        log_entry_t *to_go = igloo_loglist [log_id].log_head;
        igloo_loglist [log_id].log_head = to_go->next;
        igloo_loglist [log_id].total -= to_go->len;
        free (to_go->line);
        free (to_go);
    }
    else
        igloo_loglist [log_id].entries++;
    return entry->len;
}


void igloo_log_contents (int log_id, char **_contents, unsigned int *_len)
{
    int remain;
    log_entry_t *entry;
    char *ptr;

    if (log_id < 0) return;
    if (log_id >= LOG_MAXLOGS) return; /* Bad log number */

    _lock_logger ();
    remain = igloo_loglist [log_id].total + 1;
    *_contents = malloc (remain);
    **_contents= '\0';
    *_len = igloo_loglist [log_id].total;

    entry = igloo_loglist [log_id].log_head;
    ptr = *_contents;
    while (entry)
    {
        int len = snprintf (ptr, remain, "%s", entry->line);
        if (len > 0)
        {
            ptr += len;
            remain -= len;
        }
        entry = entry->next;
    }
    _unlock_logger ();
}

void igloo_log_write(int log_id, unsigned priority, const char *cat, const char *func, 
        const char *fmt, ...)
{
    static const char *prior[] = { "EROR", "WARN", "INFO", "DBUG" };
    int datelen;
    time_t now;
    char pre[256];
    char line[LOG_MAXLINELEN];
    va_list ap;

    if (log_id < 0 || log_id >= LOG_MAXLOGS) return; /* Bad log number */
    if (igloo_loglist[log_id].level < priority) return;
    if (!priority || priority > sizeof(prior)/sizeof(prior[0])) return; /* Bad priority */


    va_start(ap, fmt);
    igloo_private__vsnprintf(line, sizeof(line), fmt, ap);
    va_end(ap);

    now = time(NULL);
    datelen = strftime (pre, sizeof (pre), "[%Y-%m-%d  %H:%M:%S]", localtime(&now)); 
    snprintf (pre+datelen, sizeof (pre)-datelen, " %s %s%s ", prior [priority-1], cat, func);

    _lock_logger();
    if (_log_open (log_id))
    {
        int len = igloo_create_log_entry (log_id, pre, line);
        if (len > 0)
            igloo_loglist[log_id].size += len;
    }
    _unlock_logger();
}

void igloo_log_write_direct(int log_id, const char *fmt, ...)
{
    va_list ap;
    char line[LOG_MAXLINELEN];

    if (log_id < 0 || log_id >= LOG_MAXLOGS) return;
    
    va_start(ap, fmt);

    _lock_logger();
    igloo_private__vsnprintf(line, LOG_MAXLINELEN, fmt, ap);
    if (_log_open (log_id))
    {
        int len = igloo_create_log_entry (log_id, "", line);
        if (len > 0)
            igloo_loglist[log_id].size += len;
    }
    _unlock_logger();

    va_end(ap);

    fflush(igloo_loglist[log_id].logfile);
}

static int _get_log_id(void)
{
    int i;
    int id = -1;

    /* lock mutex */
    _lock_logger();

    for (i = 0; i < LOG_MAXLOGS; i++)
        if (igloo_loglist[i].in_use == 0) {
            igloo_loglist[i].in_use = 1;
            id = i;
            break;
        }

    /* unlock mutex */
    _unlock_logger();

    return id;
}

static void _lock_logger(void)
{
#ifndef _WIN32
    pthread_mutex_lock(&igloo__logger_mutex);
#else
    EnterCriticalSection(&igloo__logger_mutex);
#endif
}

static void _unlock_logger(void)
{
#ifndef _WIN32
    pthread_mutex_unlock(&igloo__logger_mutex);
#else
    LeaveCriticalSection(&igloo__logger_mutex);
#endif    
}
