#ifndef __IRODS_AUTH_SCHEME_HPP__
#define __IRODS_AUTH_SCHEME_HPP__

// =-=-=-=-=-=-=-
// stl includs
#include <string>

namespace irods {
    class pluggable_auth_scheme {
        private:
            pluggable_auth_scheme() {}
            std::string scheme_;
        public:
            std::string get() const;
            void set( const std::string& );
            static pluggable_auth_scheme& get_instance();
    };


}; // namespace irods

#endif // __IRODS_AUTH_SCHEME_HPP__



