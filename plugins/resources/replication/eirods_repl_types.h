/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#ifndef _EIRODS_REPL_TYPES_H_
#define _EIRODS_REPL_TYPES_H_

#include "eirods_resource_constants.h"
#include "eirods_object_oper.h"
#include "eirods_hierarchy_parser.h"
#include "eirods_resource_plugin_context.h"
#include "eirods_resource_redirect.h"

#include <vector>
#include <list>
#include <map>
#include <string>

// Define some types
typedef std::vector<eirods::hierarchy_parser> child_list_t;
typedef std::list<eirods::object_oper> object_list_t;
// define this so we sort children from highest vote to lowest
struct child_comp {
    bool operator()(float _lhs, float _rhs) const { return _lhs > _rhs; }
};
typedef std::multimap<float, eirods::hierarchy_parser, child_comp> redirect_map_t;

// define some constants
const std::string child_list_prop = "child_list";
const std::string object_list_prop = "object_list";
const std::string need_pdmo_prop = "Need_PDMO";
const std::string hierarchy_prop = "hierarchy";

const std::string write_oper  = eirods::EIRODS_WRITE_OPERATION;
const std::string unlink_oper = eirods::RESOURCE_OP_UNLINK;
const std::string create_oper = eirods::EIRODS_CREATE_OPERATION;
const std::string rename_oper = eirods::RESOURCE_OP_RENAME;

#endif // _EIRODS_REPL_TYPES_H_
