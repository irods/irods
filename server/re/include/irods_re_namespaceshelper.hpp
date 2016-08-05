#ifndef IRODS_RE_NAMESPACESHELPER_HPP
#define IRODS_RE_NAMESPACESHELPER_HPP

#include "irods_server_properties.hpp"

#include "rodsLog.h"

#include <vector>
#include <string>

class NamespacesHelper {
public:
    static NamespacesHelper* Instance();
    std::vector<std::string> getNamespaces();
protected:
private:
    NamespacesHelper(){};
    static NamespacesHelper* _instance;
    static std::vector<std::string> namespaces;
};

#endif
