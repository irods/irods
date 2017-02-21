#include "irods_configuration_parser.hpp"
#include "irods_log.hpp"
#include "irods_exception.hpp"

#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/any.hpp>
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
        const std::unordered_map<std::string, boost::any>& _rhs ) {
        // more could possibly be done here
        root_.clear();
        root_ = _rhs;

        return SUCCESS();

    } // copy_and_swap

    bool configuration_parser::has_entry( const std::string& _key ) {
        return root_.count( _key ) != 0;

    } // has_entry

    error configuration_parser::load( const std::string& _file ) {
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


        irods::error ret = SUCCESS();
        try {
            if ( root_.empty() ) {
                root_ = boost::any_cast<std::unordered_map<std::string, boost::any> >(convert_json(json));
            } else {
                const auto json_object_any = convert_json(json);
                const auto& json_object = boost::any_cast<const std::unordered_map<std::string, boost::any>&>(json_object_any);
                for ( auto it = json_object.cbegin(); it != json_object.cend(); ++it ) {
                    root_[it->first] = it->second;
                }
            }
        } catch ( const irods::exception& e ) {
            ret = irods::error(e);
        } catch ( const boost::bad_any_cast& ) {
            ret = ERROR(INVALID_ANY_CAST, "configuration file did not contain a json object.");
        }
        json_decref( json );

        return ret;

    } // load_json_object

    boost::any configuration_parser::convert_json(json_t* _val) {
        switch( json_typeof( _val )) {
            case JSON_INTEGER:
            return boost::any((int)json_integer_value(_val));

            case JSON_STRING:
            return boost::any(std::string(json_string_value(_val)));

            case JSON_REAL:
            return boost::any(json_real_value(_val));

            case JSON_TRUE:
            return boost::any(json_true());
            
            case JSON_FALSE:
            return boost::any(json_false());

            case JSON_NULL:
            return boost::any(std::string("NULL"));

            case JSON_ARRAY:
            {
                std::vector<boost::any> vector;
                size_t  idx = 0;
                json_t* element = NULL;
                json_array_foreach(_val, idx, element) {
                    try {
                        vector.push_back(convert_json(element));
                    } catch (const irods::exception& e) {
                        irods::log(e);
                    }
                }
                return boost::any(vector);
            }

            case JSON_OBJECT:
            {
                std::unordered_map<std::string, boost::any> map;
                const char* key = NULL;
                json_t*     subval = NULL;
                json_object_foreach( _val, key, subval ) {
                    try {
                        map.insert(std::pair<std::string, boost::any>(std::string(key), convert_json(subval)));
                    } catch (const irods::exception& e) {
                        irods::log(e);
                    }
                }
                return boost::any(map);
            }
        }
        THROW( -1, (boost::format("unhandled type in json_typeof: %d") % json_typeof(_val) ).str());

    } // parse_json_object

    std::string to_env(
        const std::string& _v ) {
        return boost::to_upper_copy <
               std::string > ( _v );
    }

    void configuration_parser::remove(const std::string& _key) {
        if ( !root_.erase(_key) ) {
            THROW ( KEY_NOT_FOUND, (boost::format("key \"%s\" not found in map.") % _key).str() );
        }
    }

}; // namespace irods
