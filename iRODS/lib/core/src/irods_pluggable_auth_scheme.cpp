// =-=-=-=-=-=-=-
#include "irods_pluggable_auth_scheme.hpp"

namespace irods {
    pluggable_auth_scheme& pluggable_auth_scheme::get_instance() {
        static pluggable_auth_scheme plug_auth_scheme;
        return plug_auth_scheme;
    }

    std::string pluggable_auth_scheme::get() const {
        return scheme_;
    }

    void pluggable_auth_scheme::set(
        const std::string& _s ) {
        scheme_ = _s;
    }

}; // namespace irods

