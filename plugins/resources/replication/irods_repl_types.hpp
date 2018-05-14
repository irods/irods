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
const std::string CHILD_LIST_PROP{"child_list"};
const std::string OBJECT_LIST_PROP{"object_list"};
const std::string HIERARCHY_PROP{"hierarchy"};
const std::string OPERATION_TYPE_PROP{"operation_type"};

#endif // _IRODS_REPL_TYPES_HPP_
