#include "irods_configuration_parser.hpp"
#include "irods_log.hpp"
#include "irods_exception.hpp"

#include <fstream>
#include <algorithm>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/any.hpp>

namespace fs = boost::filesystem;
using json = nlohmann::json;

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
            return ERROR( SYS_INVALID_INPUT_PARAM, "file is empty" );
        }

        error ret = load_json_object( _file );

        return ret;

    } // load

    error configuration_parser::load_json_object(const std::string& _file)
    {
        json config;

        {
            std::ifstream in{_file};

            if (!in) {
                std::string msg{"failed to open file ["};
                msg += _file;
                msg += ']';
                return ERROR( -1, msg );
            }

            try {
                in >> config;
            }
            catch (const json::parse_error& e) {
                return ERROR(-1, boost::format("failed to parse json [file=%s, error=%s].") % _file % e.what());
            }
        }

        irods::error ret = SUCCESS();
        try {
            if ( root_.empty() ) {
                root_ = boost::any_cast<std::unordered_map<std::string, boost::any>>(convert_json(config));
            } else {
                const auto json_object_any = convert_json(config);
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

        return ret;

    } // load_json_object

    boost::any configuration_parser::convert_json(const json& _json)
    {
        switch (_json.type()) {
            case json::value_t::string:
                return _json.get<std::string>();

            case json::value_t::number_float:
                return _json.get<double>();

            case json::value_t::number_integer:
            case json::value_t::number_unsigned:
                return _json.get<int>();

            case json::value_t::boolean:
                return _json.get<bool>();

            case json::value_t::array: {
                std::vector<boost::any> array;

                for (auto&& jj : _json) {
                    array.push_back(convert_json(jj));
                }

                return array;
            }

            case json::value_t::object: {
                std::unordered_map<std::string, boost::any> object;

                //for (auto&& [k, v] : j.items()) { // Supported in later versions :(
                for (auto it = std::begin(_json); it != std::end(_json); ++it) {
                    object.insert({it.key(), convert_json(it.value())});
                }

                return object;
            }

            case json::value_t::null:
                return std::string{"NULL"};

            default:
                const auto type = static_cast<std::uint8_t>(_json.type());
                THROW(-1, (boost::format("unhandled type in json_typeof: %d") % type));
        }
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
