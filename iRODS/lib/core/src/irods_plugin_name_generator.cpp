#include "irods_plugin_name_generator.hpp"
#include "irods_log.hpp"

#include "rodsErrorTable.hpp"

#ifndef BOOST_ASSERT_MSG
#define BOOST_ASSERT_MSG( cond, msg ) do \
{ if (!(cond)) { std::ostringstream str; str << msg; std::cerr << str.str(); std::abort(); } \
} while(0)
#endif
#include <boost/assert.hpp>


#include <boost/filesystem.hpp>

namespace irods {

// =-=-=-=-=-=-=-
// predicate to determine if a char is not alphanumeric
    static bool not_allowed_char( char _c ) {
        return !std::isalnum( _c ) && !( '_' == _c ) && !( '-' == _c );
    } // not_allowed_char


    plugin_name_generator::plugin_name_generator() {
        // TODO - stub
    }

    plugin_name_generator::~plugin_name_generator() {
        // TODO - stub
    }

    error plugin_name_generator::operator()(
        const std::string& _base_name,
        const std::string& _dir_name,
        std::string& _rtn_soname ) {
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
        if ( clean_plugin_name.empty() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Clean plugin name is empty.";
            result = ERROR( SYS_INVALID_INPUT_PARAM, msg.str() );
        }
        else {

            // construct the actual .so name
            _rtn_soname = _dir_name  + std::string( "lib" ) + clean_plugin_name + std::string( ".so" );
        }

        return result;
    }

    bool plugin_name_generator::exists(
        const std::string& _base_name,
        const std::string& _dir_name ) {
        bool result = true;
        std::string so_name;
        error ret = ( *this )( _base_name, _dir_name, so_name );
        if ( !ret.ok() ) {
            result = false;
        }
        else {
            boost::filesystem::path lib_path( so_name );
            if ( !boost::filesystem::exists( lib_path ) ) {
                result = false;
            }
        }

        return result;
    }

    error plugin_name_generator::list_plugins(
        const std::string& _dir_name,
        plugin_list_t& _list ) {
        error result = SUCCESS();
        if ( _dir_name.empty() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Directory name is empty.";
            result = ERROR( -1, msg.str() );
        }
        else {
            boost::filesystem::path so_dir( _dir_name );
            if ( boost::filesystem::exists( so_dir ) ) {
                _list.clear();
                for ( boost::filesystem::directory_iterator it( so_dir );
                        result.ok() && it != boost::filesystem::directory_iterator();
                        ++it ) {
                    boost::filesystem::path entry = it->path();
                    std::string plugin_name;
                    error ret = generate_plugin_name( entry.filename().string(), plugin_name );
                    if ( ret.ok() ) {
                        // plugin_name is empty if the dir entry was not a proper .so
                        if ( !plugin_name.empty() ) {
                            _list.push_back( plugin_name );
                        }
                    }
                    else {
                        std::stringstream msg;
                        msg << __FUNCTION__;
                        msg << " - An error occurred while generating plugin name from filename \"";
                        msg << entry.filename();
                        msg << "\"";
                        result = PASSMSG( msg.str(), ret );
                    }
                }
            }
            else {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Plugin directory \"";
                msg << _dir_name;
                msg << "\" does not exist.";
                result = ERROR( -1, msg.str() );
            }
        }
        return result;
    }

    error plugin_name_generator::generate_plugin_name(
        const std::string& filename,
        std::string& _rtn_name ) {
        error result = SUCCESS();
        _rtn_name.clear();
        int sub_length = filename.length() - 6;
        if ( sub_length > 0 &&
                filename.find( "lib" ) == 0 &&
                filename.find( ".so" ) == ( filename.length() - 3 ) ) {
            _rtn_name = filename.substr( 3, sub_length );
        }
        return result;
    }

}; // namespace irods
