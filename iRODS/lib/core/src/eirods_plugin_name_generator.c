/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#include "eirods_plugin_name_generator.h"

#include "rodsErrorTable.h"

#include <boost/filesystem.hpp>

namespace eirods {

    // =-=-=-=-=-=-=-
    // predicate to determine if a char is not alphanumeric
    static bool not_allowed_char( char _c ) {
        return ( !std::isalnum( _c ) && !( '_' == _c ) );
    } // not_allowed_char
    
    
    plugin_name_generator::plugin_name_generator() {
        // TODO - stub
    }

    plugin_name_generator::~plugin_name_generator() {
        // TODO - stub
    }

    error plugin_name_generator::operator() (
        const std::string& _base_name,
        const std::string& _dir_name,
        std::string& _rtn_soname)
    {
        error result = SUCCESS();
        
        // =-=-=-=-=-=-=-
        // strip out all non alphanumeric characters like spaces or such
        std::string clean_plugin_name = _base_name;
        clean_plugin_name.erase( std::remove_if( clean_plugin_name.begin(),
                                                 clean_plugin_name.end(),
                                                 not_allowed_char ),
                                 clean_plugin_name.end() );

        // =-=-=-=-=-=-=-
        // quick parameter check
        if( clean_plugin_name.empty() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Clean plugin name is empty.";
            result = ERROR(SYS_INVALID_INPUT_PARAM, msg.str());
        } else {

            // construct the actual .so name
            _rtn_soname = _dir_name  + std::string("lib") + clean_plugin_name + std::string(".so");
        }

        return result;
    }
    
    bool plugin_name_generator::exists(
        const std::string& _base_name,
        const std::string& _dir_name)
    {
        bool result = true;
        std::string so_name;
        error ret = (*this)(_base_name, _dir_name, so_name);
        if(!ret.ok()) {
            result = false;
        } else {
            boost::filesystem::path lib_path(so_name);
            if(!boost::filesystem::exists(lib_path)) {
                result = false;
            }
        }

        return result;
    }

}; // namespace eirods
