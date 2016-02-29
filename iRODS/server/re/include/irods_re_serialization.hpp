

#ifndef IRODS_AVRO_SERIALIZATION_HPP
#define IRODS_AVRO_SERIALIZATION_HPP

#include "irods_hierarchy_parser.hpp"
#include "apiHeaderAll.h"
#include "icatStructs.hpp"

#include <boost/shared_ptr.hpp>
#include <boost/any.hpp>
#include <map>
#include <vector>
#include <typeindex>

typedef std::vector<rodsLong_t> leaf_bundle_t;

namespace irods {
    namespace re_serialization {
        typedef std::type_index                                          index_t;
        typedef std::map<std::string,std::string>                        serialized_parameter_t;
        typedef std::function<error(boost::any,serialized_parameter_t&)> operation_t;
        typedef std::map<index_t, operation_t>                           serialization_map_t;
        
        serialization_map_t& get_serialization_map();
        error add_operation(
            const index_t& _index,
            operation_t    _operation );
        error serialize_parameter(
            boost::any               _in_param,
            serialized_parameter_t&  _out_param );

    }; // re_serialization

}; // namespace irods

#endif // IRODS_AVRO_SERIALIZATION_HPP




