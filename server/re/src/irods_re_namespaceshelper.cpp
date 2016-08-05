#include "irods_re_namespaceshelper.hpp"

NamespacesHelper* NamespacesHelper::_instance = 0;
std::vector<std::string> NamespacesHelper::namespaces {};

NamespacesHelper* NamespacesHelper::Instance() {
    if (!_instance) {
        _instance = new NamespacesHelper;

        const auto& re_namespace_set = irods::get_server_property< const std::vector< boost::any >& >( irods::CFG_RE_NAMESPACE_SET_KW );
        for ( const auto& el : re_namespace_set ) {
            namespaces.push_back( boost::any_cast< const std::string& >(
                                  boost::any_cast< const std::unordered_map< std::string, boost::any >& >(el).at(irods::CFG_NAMESPACE_KW ) ) );
        }
    
        const auto& re_plugin_configs = irods::get_server_property< const std::vector< boost::any >& >( std::vector< std::string >{irods::CFG_PLUGIN_CONFIGURATION_KW, irods::PLUGIN_TYPE_RULE_ENGINE } );
        for ( const auto& el : re_plugin_configs ) {
            try {
                const auto& plugin_config = boost::any_cast< const std::unordered_map< std::string , boost::any >& >( el );
                const auto& plugin_spec_config = boost::any_cast< const std::unordered_map< std::string, boost::any >& > ( plugin_config.at( irods::CFG_PLUGIN_SPECIFIC_CONFIGURATION_KW ) );
                for ( const auto& el2 : boost::any_cast< const std::vector< boost::any >& >( plugin_spec_config.at( irods::CFG_RE_NAMESPACE_SET_KW ) ) ) {
                    namespaces.push_back( boost::any_cast< const std::string& >(
                                boost::any_cast< const std::unordered_map< std::string, boost::any >& >(el2).at(irods::CFG_NAMESPACE_KW ) ) );
                }
            } catch ( const boost::bad_any_cast& e ) {
                rodsLog( LOG_DEBUG, "[%s:%d] bad any cast", __FUNCTION__, __LINE__ );
            } catch ( const std::out_of_range& e ) {
                rodsLog( LOG_DEBUG, "[%s:%d] index out of range", __FUNCTION__, __LINE__ );
            }
        }
    }

    return _instance;
}

std::vector<std::string> NamespacesHelper::getNamespaces() {
    return namespaces;
}
