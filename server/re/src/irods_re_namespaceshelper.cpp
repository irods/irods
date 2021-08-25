#include "irods_re_namespaceshelper.hpp"

NamespacesHelper* NamespacesHelper::_instance = 0;
std::vector<std::string> NamespacesHelper::namespaces {};

NamespacesHelper* NamespacesHelper::Instance() {
    if (!_instance) {
        _instance = new NamespacesHelper;

        const auto& re_namespace_set = irods::get_server_property<const nlohmann::json&>(irods::CFG_RE_NAMESPACE_SET_KW);
        for ( const auto& el : re_namespace_set ) {
            namespaces.push_back(el.get<std::string>());
        }
    }

    return _instance;
}

std::vector<std::string> NamespacesHelper::getNamespaces() {
    return namespaces;
}
