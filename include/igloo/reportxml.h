/* Icecast
 *
 * This program is distributed under the GNU General Public License, version 2.
 * A copy of this license is included with this source.
 *
 * Copyright 2018,      Philipp "ph3-der-loewe" Schafft <lion@lion.leolix.org>,
 */

/* This file contains the API for report XML document parsing, manipulation, and rendering. */

#ifndef _LIBIGLOO__REPORTXML_H_
#define _LIBIGLOO__REPORTXML_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <libxml/tree.h>

#include <igloo/ro.h>

igloo_RO_FORWARD_TYPE(igloo_reportxml_t);
igloo_RO_FORWARD_TYPE(igloo_reportxml_node_t);
igloo_RO_FORWARD_TYPE(igloo_reportxml_database_t);


/* XML Tag Types
 * While a hint of what the nodes are used for is given, see the specification for more details.
 */
typedef enum {
    /* This is a virtual type used to indicate error conditions */
    igloo_REPORTXML_NODE_TYPE__ERROR,
    /* <report> is the root element of report XML documents */
    igloo_REPORTXML_NODE_TYPE_REPORT,
    /* <definition> is used to define templates */
    igloo_REPORTXML_NODE_TYPE_DEFINITION,
    /* <incident> defines an event that is reported */
    igloo_REPORTXML_NODE_TYPE_INCIDENT,
    /* <state> defines the state an <incident> resulted in */
    igloo_REPORTXML_NODE_TYPE_STATE,
    /* <backtrace> provides helpful information about the location some event happend */
    igloo_REPORTXML_NODE_TYPE_BACKTRACE,
    /* <position> defines an element within <backtrace> */
    igloo_REPORTXML_NODE_TYPE_POSITION,
    /* <more> allows to skip <position>s in <backtrace> for any reason
     * (e.g. they are unknown or consider of no intrest)
     */
    igloo_REPORTXML_NODE_TYPE_MORE,
    /* <fix> provides a machine readable way to actually fix the problem */
    igloo_REPORTXML_NODE_TYPE_FIX,
    /* <action> defines a specific action to do */
    igloo_REPORTXML_NODE_TYPE_ACTION,
    /* <reason> allows to define why an event happend */
    igloo_REPORTXML_NODE_TYPE_REASON,
    /* <text> is used to provide messages to the user.
     * The content of <text> is not machine readable.
     */
    igloo_REPORTXML_NODE_TYPE_TEXT,
    /* <timestamp> provides a way to present a point in time an event happend */
    igloo_REPORTXML_NODE_TYPE_TIMESTAMP,
    /* <resource> names a resource that was involved in the event such as user input or the result */
    igloo_REPORTXML_NODE_TYPE_RESOURCE,
    /* <value> provides an actual value for a <resource> */
    igloo_REPORTXML_NODE_TYPE_VALUE,
    /* <reference> provides a way to refer to external documents such as documentation */
    igloo_REPORTXML_NODE_TYPE_REFERENCE,
    /* <extension> is used to allow application specific extensions */
    igloo_REPORTXML_NODE_TYPE_EXTENSION
} igloo_reportxml_node_type_t;


/* ---[ Document level ]--- */
/* The document object is NOT thread safe. */


/* This gets the root node of a report XML document */
igloo_reportxml_node_t *      igloo_reportxml_get_root_node(igloo_reportxml_t *report);
/* This selects a node by an attribute and it's value.
 * This is mostly useful to look for an object by using it's ID.
 * If more than one node matches the first one found is returned.
 * If the parameter include_definitions is true nodes from within
 * <definition> are also considered. If it is false nodes inside
 * <definition>s are skipped.
 */
igloo_reportxml_node_t *      igloo_reportxml_get_node_by_attribute(igloo_reportxml_t *report, const char *key, const char *value, int include_definitions);
/* This gets a node by it's type. Otherwise identical to igloo_reportxml_get_node_by_attribute() */
igloo_reportxml_node_t *      igloo_reportxml_get_node_by_type(igloo_reportxml_t *report, igloo_reportxml_node_type_t type, int include_definitions);
/* This function parses an XML document and returns the parst report XML document */
igloo_reportxml_t *           igloo_reportxml_parse_xmldoc(xmlDocPtr doc, igloo_ro_t instance);
/* This function renders an report XML document as XML structure */
xmlDocPtr               igloo_reportxml_render_xmldoc(igloo_reportxml_t *report);


/* ---[ Node level ]--- */
/* The node object is NOT thread safe. */


/* This creates a new node of type type.
 * It's id, definition, and akindof attributes can be given as parameters.
 */
igloo_reportxml_node_t *      igloo_reportxml_node_new(igloo_reportxml_node_type_t type, const char *id, const char *definition, const char *akindof, igloo_ro_t instance);
/* This parses an XML node and returns the resulting report XML node */
igloo_reportxml_node_t *      igloo_reportxml_node_parse_xmlnode(xmlNodePtr xmlnode, igloo_ro_t instance);
/* Copy an report XML node (and it's children) */
igloo_reportxml_node_t *      igloo_reportxml_node_copy(igloo_reportxml_node_t *node);
/* Renders an report XML node as XML node */
xmlNodePtr              igloo_reportxml_node_render_xmlnode(igloo_reportxml_node_t *node);
/* This gets the type of an report XML node */
igloo_reportxml_node_type_t   igloo_reportxml_node_get_type(igloo_reportxml_node_t *node);
/* Gets and Sets attribute values */
int                     igloo_reportxml_node_set_attribute(igloo_reportxml_node_t *node, const char *key, const char *value);
char *                  igloo_reportxml_node_get_attribute(igloo_reportxml_node_t *node, const char *key);
/* Adds, counts, and get child nodes */
int                     igloo_reportxml_node_add_child(igloo_reportxml_node_t *node, igloo_reportxml_node_t *child);
ssize_t                 igloo_reportxml_node_count_child(igloo_reportxml_node_t *node);
igloo_reportxml_node_t *      igloo_reportxml_node_get_child(igloo_reportxml_node_t *node, size_t idx);
/* This gets an child by it's value of the given attribute. See igloo_reportxml_get_node_by_attribute() for more details. */
igloo_reportxml_node_t *      igloo_reportxml_node_get_child_by_attribute(igloo_reportxml_node_t *node, const char *key, const char *value, int include_definitions);
/* This gets a child by it's type. Otherwise identical to igloo_reportxml_node_get_child_by_attribute() */
igloo_reportxml_node_t *      igloo_reportxml_node_get_child_by_type(igloo_reportxml_node_t *node, igloo_reportxml_node_type_t type, int include_definitions);
/* This gets and sets the text content of an node (used for <text>) */
int                     igloo_reportxml_node_set_content(igloo_reportxml_node_t *node, const char *value);
char *                  igloo_reportxml_node_get_content(igloo_reportxml_node_t *node);
/* Adds, counts, and gets XML childs (used for <extension>) */
int                     igloo_reportxml_node_add_xml_child(igloo_reportxml_node_t *node, xmlNodePtr child);
ssize_t                 igloo_reportxml_node_count_xml_child(igloo_reportxml_node_t *node);
xmlNodePtr              igloo_reportxml_node_get_xml_child(igloo_reportxml_node_t *node, size_t idx);


/* ---[ Database level ]--- */
/* The database object is thread safe. */


/* Add an report to the database */
int                     igloo_reportxml_database_add_report(igloo_reportxml_database_t *db, igloo_reportxml_t *report);
/* Build a node (copy) from the data in the database based on the given ID (using "definition" and "defines" attributes)
 * depth may be used to select how many recursions may be used to resolve definitions within defines.
 * The default value is selected by passing -1 (recommended).
 */
igloo_reportxml_node_t *      igloo_reportxml_database_build_node(igloo_reportxml_database_t *db, const char *id, ssize_t depth);
/* This does the same as igloo_reportxml_database_build_node() except that a new report document is returned. */
igloo_reportxml_t *           igloo_reportxml_database_build_report(igloo_reportxml_database_t *db, const char *id, ssize_t depth);

#ifdef __cplusplus
}
#endif

#endif
