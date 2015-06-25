#include "irods_configuration_parser.hpp"
#include "irods_log.hpp"

#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

namespace irods {

    configuration_parser::configuration_parser() {

    } // ctor

    configuration_parser::~configuration_parser() {

    } // dtor

    configuration_parser::configuration_parser(
        const configuration_parser& _rhs ) {

        irods::error ret = copy_and_swap( _rhs.root_ );
        if ( !ret.ok() ) {
            irods::log( PASS( ret ) );
        }

    } // cctor

    configuration_parser::configuration_parser(
        const std::string& _file ) {

        load( _file );

    } // ctor

    configuration_parser& configuration_parser::operator=(
        const configuration_parser& _rhs ) {

        copy_and_swap( _rhs.root_ );

        return *this;

    } // operator=

    void configuration_parser::clear() {

        root_.clear();

    } // clear

    error configuration_parser::copy_and_swap(
        const configuration_parser::object_t& _rhs ) {
        // more could possibly be done here
        root_.clear();
        root_ = _rhs;

        return SUCCESS();

    } // copy_and_swap

    bool configuration_parser::has_entry(
        const std::string& _key ) {
        bool ret = root_.has_entry( _key );
        return ret;

    } // has_entry

    size_t configuration_parser::erase(
        const std::string& _key ) {
        size_t ret = root_.erase( _key );
        return ret;

    } // erase

    error configuration_parser::load(
        const std::string& _file ) {
        if ( _file.empty() ) {
            return ERROR(
                       SYS_INVALID_INPUT_PARAM,
                       "file is empty" );
        }

        error ret = load_json_object( _file );

        return ret;

    } // load

    error configuration_parser::load_json_object(
        const std::string&              _file ) {
        json_error_t error;

        json_t *json = json_load_file( _file.c_str(), 0, &error );
        if ( !json ) {
            std::string msg( "failed to load file [" );
            msg += _file;
            msg += "] json error [";
            msg += error.text;
            msg += "]";
            return ERROR( -1, msg );
        }

        irods::error ret = parse_json_object( json, root_ );
        json_decref( json );

        return ret;

    } // load_json_object

    error configuration_parser::parse_json_object(
        json_t*                         _obj,
        configuration_parser::object_t& _obj_out ) {

        const char* key = 0;
        json_t*     val = 0;
        json_object_foreach( _obj, key, val ) {
            int type = json_typeof( val );
            if ( JSON_INTEGER == type ) {
                _obj_out.set< int >( key, json_integer_value( val ) );

            }
            else if ( JSON_STRING == type ) {
                _obj_out.set< std::string >( key, json_string_value( val ) );

            }
            else if ( JSON_REAL == type ) {
                _obj_out.set< double >( key, json_real_value( val ) );

            }
            else if ( JSON_TRUE == type ||
                      JSON_FALSE == type )  {
                _obj_out.set< bool >( key, json_boolean( val ) );

            }
            else if ( JSON_NULL == type ) {
                _obj_out.set< std::string >( key, "NULL" );

            }
            else if ( JSON_ARRAY  == type ) {
                array_t arr;
                size_t  idx = 0;
                json_t* obj = 0;
                json_array_foreach( val, idx, obj ) {
                    object_t lt;
                    irods::error err = parse_json_object(
                                           obj,
                                           lt );
                    if ( err.ok() ) {
                        arr.push_back( lt );

                    }
                    else {
                        irods::log( PASS( err ) );

                    }

                } // array foreach

                _obj_out.set< array_t >( key, arr );

            }
            else if ( JSON_OBJECT == type ) {
                object_t lt;
                irods::error err = parse_json_object(
                                       val,
                                       lt );
                if ( err.ok() ) {
                    _obj_out.set< object_t >( key, lt );

                }
                else {
                    irods::log( PASS( err ) );

                }

            }
            else {
                rodsLog(
                    LOG_NOTICE,
                    "parse_json_object :: unhandled type %d",
                    type );
            }

        } // foreach

        return SUCCESS();

    } // parse_json_object

    std::string to_env(
        const std::string& _v ) {
        return boost::to_upper_copy <
               std::string > ( _v );
    }

}; // namespace irods
