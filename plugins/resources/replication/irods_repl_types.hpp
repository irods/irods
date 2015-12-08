#ifndef _IRODS_REPL_TYPES_HPP_
#define _IRODS_REPL_TYPES_HPP_

#include "irods_resource_constants.hpp"
#include "irods_object_oper.hpp"
#include "irods_hierarchy_parser.hpp"
#include "irods_plugin_context.hpp"
#include "irods_resource_redirect.hpp"

#include <vector>
#include <list>
#include <map>
#include <string>

// Define some types
typedef std::vector<irods::hierarchy_parser> child_list_t;
typedef std::list<irods::object_oper> object_list_t;
// define this so we sort children from highest vote to lowest
struct child_comp {
    bool operator()( float _lhs, float _rhs ) const {
        return _lhs > _rhs;
    }
};
typedef std::multimap<float, irods::hierarchy_parser, child_comp> redirect_map_t;

// define some constants
const std::string child_list_prop = "child_list";
const std::string write_child_list_prop = "write_child_list";
const std::string object_list_prop = "object_list";
const std::string need_pdmo_prop = "Need_PDMO";
const std::string hierarchy_prop = "hierarchy";
const std::string operation_type_prop = "operation_type";

const std::string write_oper  = irods::WRITE_OPERATION;
const std::string unlink_oper = irods::RESOURCE_OP_UNLINK;
const std::string create_oper = irods::CREATE_OPERATION;
const std::string rename_oper = irods::RESOURCE_OP_RENAME;

#endif // _IRODS_REPL_TYPES_HPP_
