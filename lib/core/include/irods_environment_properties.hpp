#ifndef IRODS_ENVIRONMENT_PROPERTIES_HPP_
#define IRODS_ENVIRONMENT_PROPERTIES_HPP_


#include "irods_configuration_parser.hpp"
#include "irods_configuration_keywords.hpp"
#include "irods_exception.hpp"

#include <map>

namespace irods {

    static const std::string IRODS_JSON_ENV_FILE = "/.irods/irods_environment.json";

    error get_json_environment_file( std::string& _env_file, std::string& _session_file);

    class environment_properties {

        public:
            /// @brief  type alias for iterating over configuration properties

            /**
             * @brief Access method for the singleton
             */
            static environment_properties& instance();

            /**
             * @brief Read environment configuration and fill environment_properties::properties
             */
            void capture( );

            /**
             * @brief Get a property from the map if it exists.  catch the exception in the case where
             * the template types may not match and return success/fail
             */
            template< typename T >
            error get_property( const std::string& _key, T& _val ) {
                try {
                    _val = get_property<T>( _key );
                } catch ( const irods::exception& e ) {
                    return irods::error(e);
                }
                return SUCCESS();
            }


            template< typename T >
            T& get_property( const std::string& _key ) {
                return config_props_.get< T >( _key );
            }

            template< typename T >
            T& set_property( const std::string& _key, const T& _val ) {
                return config_props_.set< T >( _key, _val );
            }

            template< typename T >
            T remove( const std::string& _key ) {
                return config_props_.remove(_key);
            }

            void remove( const std::string& _key );

            std::unordered_map<std::string, boost::any>& map() {
                return config_props_.map();
            }
        private:
            // Disable constructors
            environment_properties( environment_properties const& );
            environment_properties( );
            void operator=( environment_properties const& );

            /**
             * @brief capture irods_environment.json
             */
            void capture_json( const std::string& );

            /**
             * @brief properties lookup table
             */
            configuration_parser config_props_;

    }; // class environment_properties

    template< typename T >
    T& get_environment_property( const std::string& _prop ) {
        return irods::environment_properties::instance().get_property<T>(_prop);
    }

    template< typename T >
    T& set_environment_property( const std::string& _prop, const T& _val ) {
        return irods::environment_properties::instance().set_property<T>(_prop, _val);
    }
} // namespace irods

#endif /* IRODS_ENVIRONMENT_PROPERTIES_HPP_ */
